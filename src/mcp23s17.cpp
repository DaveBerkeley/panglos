
#include <string.h>

#include "panglos/debug.h"

#include "panglos/spi.h"
#include "panglos/i2c.h"
#include "panglos/mcp23s17.h"

namespace panglos {

    /*
     *
     */

class ExpandedGpio : public GPIO
{
public:
    MCP23S17 *chip;
    MCP23S17::Port port;
    int bit;
    uint8_t mask;
    bool output;
    bool auto_flush;

    // IRQ handler
    void (*fn_irq)(void *arg);
    void *arg;

    MCP23S17::Register gpio_reg;

    ExpandedGpio(MCP23S17 *_chip, MCP23S17::Port _port, int _bit, MCP23S17::Mode mode, bool _auto_flush);
    ~ExpandedGpio();

    void set_mask(MCP23S17::Register reg, bool flush=false);
    void clr_mask(MCP23S17::Register reg, bool flush=false);

    virtual void set(bool state);
    virtual bool get();
    virtual void toggle();
    virtual bool flush();

    virtual void set_interrupt_handler(void (*fn)(void *arg), void *arg);

    virtual void on_interrupt();
};

    /*
     *
     */

class MCP23S17::Handler : public Dispatch::Callback
{
    MCP23S17 *chip;

    virtual void execute()
    {
        chip->_on_interrupt();
    }

public:
    Handler(MCP23S17 *_chip) : chip(_chip) { }
};

    /*
     *
     */

Cache::Cache(Mutex *m)
: mutex(m), data(0), dirty(false)
{
}

void Cache::set_mask(uint8_t mask)
{
    Lock lock(mutex);

    const uint8_t next = data | mask;
    if (next != data)
    {
        data = next;
        dirty = true;
    }
}

void Cache::clr_mask(uint8_t mask)
{
    Lock lock(mutex);

    const uint8_t next = (uint8_t) (data & ~mask);
    if (next != data)
    {
        data = next;
        dirty = true;
    }
}

void Cache::write(uint8_t _data)
{
    Lock lock(mutex);
    dirty = data != _data;
    data = _data;
}

uint8_t Cache::read()
{
    return data;
}

    /**
     * @brief ctor for the MCP23S17 class
     *
     * @param spi the SPI device the chip is attached to
     * @param cs the GPIO line used as a chip-select
     * @param addr the address of the device (0..7)
     */

MCP23S17::MCP23S17()
: cache_mutex(0), handler(0)
{
    cache_mutex = Mutex::create();

    // set the cache
    for (int i = 0; i < NUM_CACHES; i++)
    {
        gpio_cache[i] = new Cache(cache_mutex);
    }

    // clear the GPIO pointers
    memset(pins, 0, sizeof(pins));

    handler = new Handler(this);
}

void MCP23S17::delete_gpios()
{
    for (size_t i = 0; i < (sizeof(pins) / sizeof(pins[0])); i++)
    {
        if (pins[i])
        {
            delete pins[i];
        }
        ASSERT(pins[i] == 0);
    }
}

MCP23S17::~MCP23S17()
{
    // set the cache
    for (int i = 0; i < NUM_CACHES; i++)
    {
        delete gpio_cache[i];
    }

    delete cache_mutex;
    delete handler;

}

    /**
     * @brief internal function used to access the register's Cache (if it has one)
     */

Cache *MCP23S17::get_cache(Register reg)
{
    switch (reg)
    {
        case R_GPIOA : return gpio_cache[0];
        case R_GPIOB : return gpio_cache[1];
        default      : return 0;
    }
}

    /**
     * @brief hardware write to SPI
     * @param reg the register to write to
     * @param data data to write
     */

void MCP23S17::write(Register reg, uint8_t data)
{
    reg_write(reg, data);

    Cache *cache = get_cache(reg);
    if (cache)
    {
        cache->write(data);
    }
}

    /**
     * @brief read from the register's cache
     */

uint8_t MCP23S17::read_cache(Register reg)
{
    Cache *cache = get_cache(reg);
    ASSERT(cache);

    return cache->read();
}

    /**
     * @brief write to the register's cache
     */

void MCP23S17::write_cache(Register reg, uint8_t data)
{
    Cache *cache = get_cache(reg);
    ASSERT(cache);
    cache->write(data);
}

    /**
     * @brief flush any dirty cache contents to the SPI bus
     */

bool MCP23S17::flush_cache(Register reg)
{
    Cache *cache = get_cache(reg);

    if (!cache)
    {
        // no cache on this register
        return false;
    }

    // lock the cache
    Lock lock(cache_mutex);

    if (!cache->is_dirty())
    {
        // don't bother if the data is unchanged
        return true;
    }

    // write to the hardware
    reg_write(reg, cache->data);
    // cache is no longer dirty
    cache->dirty = false;
    return true;
}

    /**
     * @brief provides access to the Dispatch::Callback interface
     */

Dispatch::Callback *MCP23S17::get_interrupt_handler()
{
    return handler;
}

    /**
     * @brief internal function : sets pins[] 
     */

void MCP23S17::_set_pin(GPIO *gpio, Port port, int bit)
{
    int idx = bit;
    if (port == PORTB){
        idx += 8;
    }

    ASSERT((idx >= 0) && (idx < 16));

    pins[idx] = gpio;
}

    /*
     *
     */

ExpandedGpio::ExpandedGpio(MCP23S17 *_chip, MCP23S17::Port _port, int _bit, MCP23S17::Mode mode, bool _auto_flush)
: chip(_chip), port(_port), bit(_bit), mask(0), output(false), auto_flush(_auto_flush), fn_irq(0), arg(0)
{
    ASSERT(chip);
    ASSERT((bit >= 0) && (bit <= 7));

    mask = (uint8_t) (1 << bit);
    MCP23S17::Register iodir_reg = (MCP23S17::Register) 0;
    MCP23S17::Register gppu_reg = (MCP23S17::Register) 0;
    MCP23S17::Register gpinten_reg = (MCP23S17::Register) 0;
    MCP23S17::Register defval_reg = (MCP23S17::Register) 0;

    switch (port)
    {
        case MCP23S17::PORTA :
        {
            iodir_reg = MCP23S17::R_IODIRA;
            gppu_reg  = MCP23S17::R_GPPUA;
            gpio_reg  = MCP23S17::R_GPIOA;
            gpinten_reg = MCP23S17::R_GPINTENA;
            defval_reg = MCP23S17::R_DEFVALA;
            break;
        }
        case MCP23S17::PORTB :
        {
            iodir_reg = MCP23S17::R_IODIRB;
            gppu_reg  = MCP23S17::R_GPPUB;
            gpio_reg  = MCP23S17::R_GPIOB;
            gpinten_reg = MCP23S17::R_GPINTENB;
            defval_reg = MCP23S17::R_DEFVALB;
            break;
        }
        default :
        {
            ASSERT(0);
        }
    }

    const bool flush = true;

    switch (mode)
    {
        case  MCP23S17::INPUT :
        {
            set_mask(iodir_reg, flush);
            clr_mask(gppu_reg, flush);
            break;
        }
        case  MCP23S17::INPUT_PU :
        {
            set_mask(iodir_reg, flush);
            set_mask(gppu_reg, flush);
            break;
        }
        case  MCP23S17::OUTPUT :
        {
            output = true;
            clr_mask(iodir_reg, flush);
            break;
        }
        case  MCP23S17::IT_RISING :
        {
            set_mask(iodir_reg, flush);   // Input
            set_mask(gppu_reg, flush);    // enable pull-up
            set_mask(defval_reg, flush);
            set_mask(gpinten_reg, flush); // enable interrupt
            break;
        }
        case  MCP23S17::IT_FALLING :
        {
            set_mask(iodir_reg, flush);   // Input
            set_mask(gppu_reg, flush);    // enable pull-up
            clr_mask(defval_reg, flush);
            set_mask(gpinten_reg, flush); // enable interrupt
            break;
        }
        default :
        {
            ASSERT(0);
        }
    }

    _chip->_set_pin(this, port, bit);
}

ExpandedGpio::~ExpandedGpio()
{
    // remove pin from the chip's store
    chip->_set_pin(0, port, bit);

    const bool flush = true;

    // clear IRQ enable bit
    switch (port)
    {
        case MCP23S17::PORTA :
        {
            clr_mask(MCP23S17::R_GPINTENA, flush); // disable interrupt
            break;
        }
        case MCP23S17::PORTB :
        {
            clr_mask(MCP23S17::R_GPINTENB, flush); // disable interrupt
            break;
        }
        default :
        {
            ASSERT(0);
        }
    }
}

void ExpandedGpio::set_mask(MCP23S17::Register reg, bool flush)
{
    Cache *cache = chip->get_cache(reg);

    if (cache)
    {
        cache->set_mask(mask);

        if (flush || auto_flush)
        {
            chip->flush_cache(reg);
        }
        return;
    }

    uint8_t data = chip->read(reg);
    chip->write(reg, data | mask);
}

void ExpandedGpio::clr_mask(MCP23S17::Register reg, bool flush)
{
    Cache *cache = chip->get_cache(reg);

    if (cache)
    {
        cache->clr_mask(mask);

        if (flush || auto_flush)
        {
            chip->flush_cache(reg);
        }
        return;
    }

    uint8_t data = chip->read(reg);
    chip->write(reg, (uint8_t) (data & ~mask));
}

void ExpandedGpio::set(bool state)
{
    if (state)
    {
        set_mask(gpio_reg);
    }
    else
    {
        clr_mask(gpio_reg);
    }
}

bool ExpandedGpio::get()
{
    uint8_t data;

    if (output)
    {
        data = chip->read_cache(gpio_reg);
    }
    else
    {
        data = chip->read(gpio_reg);
    }

    return data & mask;
}

void ExpandedGpio::toggle()
{
    set(!get());
}

bool ExpandedGpio::flush()
{
    if (output)
    {
        return chip->flush_cache(gpio_reg);
    }

    return false;
}

void ExpandedGpio::set_interrupt_handler(void (*fn)(void *arg), void *_arg)
{
    fn_irq = fn;
    arg = _arg;
}

void ExpandedGpio::on_interrupt()
{
    //PO_DEBUG("");

    if (fn_irq)
    {
        fn_irq(arg);
    }
}

    /**
     * @brief create function used to make a GPIO object mapped to a pin of the device
     *
     * @param port the port 
     * @param bit the bit of the port
     * @param auto_flush set true if flush() to hardware required on every write
     */

GPIO * MCP23S17::make_gpio(Port port, int bit, Mode mode, bool auto_flush)
{
    ExpandedGpio *gpio = new ExpandedGpio(this, port, bit, mode, auto_flush);
    return gpio;
}


    /*
     *
     */

SPI_MCP23S17::SPI_MCP23S17(SPI *spi, GPIO *cs, uint8_t _addr)
:   MCP23S17(), 
    dev(0),
    addr_cmd(0)
{
    ASSERT(_addr <= 7);

    addr_cmd = (uint8_t) (0x40 + (_addr << 1));
    dev = new SpiDevice(spi, cs, addr_cmd);

    // assume the chip is initialised ...

    // enable the HAEN bit in ICON, to enable A0..2 addressing
    const uint8_t icon = 0x08 /* HAEN */ | 0x40 /* MIRROR */ ;
    uint8_t cmd0 = 0x40;

    if (_addr & 0x04)
    {
        // See MCP23S17 Rev. A Silicon Errata
        cmd0 = 0x48;
    }

    uint8_t cmd[] = { cmd0, R_IOCON, icon };
    spi->write(cmd, sizeof(cmd));

}

SPI_MCP23S17::~SPI_MCP23S17()
{
    delete dev;
}

    /**
     * @brief hardware (SPI) read of register
     */

uint8_t SPI_MCP23S17::read(Register reg)
{
    uint8_t data;
    dev->read(reg, & data);
    return data;
}

    /**
     * @brief internal function used to dispatch any GPIO interrupts
     * and clear the hardware status.
     */

void SPI_MCP23S17::_on_interrupt()
{
    // find out which port/pin caused the interrupt
    // clear the interrupt
    // call the pin's interrupt handler

    // read the INTFx registers. Also read ... GPIOx to clear the irq
    const uint8_t wr[] = { uint8_t(addr_cmd | 0x01), R_INTFA, 0, 0, 0, 0, 0, 0 };
    uint8_t rd[sizeof(wr)];

    dev->read(wr, rd, sizeof(wr));

    int idx = 0;
    for (int port = 0; port < 2; port++)
    {
        for (uint8_t mask = 0x01; mask; mask = (uint8_t) (mask << 0x01))
        {
            const bool irq = rd[2+port] & mask;
            if (irq)
            {
                // call any interrupt handler assigned on this pin
                GPIO *gpio = pins[idx];
                if (gpio)
                {
                    gpio->on_interrupt();
                }
            }
            idx += 1;
        }
    }
}

void SPI_MCP23S17::reg_write(Register reg, uint8_t data)
{
    dev->write(reg, data);
}

    /*
     *  I2C MCP23017 controller
     */

I2C_MCP23S17::I2C_MCP23S17(I2C *i2c, uint8_t _addr)
:   dev(i2c),
    addr(_addr)
{
    ASSERT(i2c);
    PO_DEBUG("cmd=%#x", addr);
}

I2C_MCP23S17::~I2C_MCP23S17()
{
}

void I2C_MCP23S17::_on_interrupt()
{
    ASSERT(0);
}

uint8_t I2C_MCP23S17::read(Register reg)
{
    uint8_t wr = reg;
    uint8_t rd;
    dev->write_read(addr, & wr, 1, & rd, 1);
    return rd;
}

void I2C_MCP23S17::reg_write(Register reg, uint8_t data)
{
    uint8_t cmd[] = { reg, data };
    dev->write(addr, cmd, sizeof(cmd));
}

}   //  namespace panglos

//  FIN
