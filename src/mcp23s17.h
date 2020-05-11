
#if !defined(__MCP23S17_H__)
#define __MCP23S17_H__

#include "spi.h"
#include "gpio.h"
#include "mutex.h"
#include "dispatch.h"

namespace panglos {

    /**
     * @brief Cache class used to cache the registers of the device
     *
     * This class is only exposed for the purposes of the unit tests.
     */

class Cache
{
public:
    Mutex *mutex;
    uint8_t data;
    bool dirty;

    Cache(Mutex *m);

    void set_mask(uint8_t mask);
    void clr_mask(uint8_t mask);
    void write(uint8_t _data);
    uint8_t read();
    bool is_dirty() { return dirty; }
    void mark_dirty() { dirty = true; }
};

    /**
     * @brief Interface code for the MCP23S17 SPI 16-bt port expander
     *
     * https://www.microchip.com/wwwproducts/en/MCP23S17
     */

class MCP23S17
{
private:
    /// the SPI device
    SpiDevice *dev;
    /// number of registers that are cached
    enum { NUM_CACHES = 2 };
    /// register caches (for the GPIO regs)
    Cache *gpio_cache[NUM_CACHES];
    /// Mutex used to protect the cache 
    Mutex *cache_mutex;
    /// GPIO objects for each pin created with make_gpio()
    GPIO *pins[16];
    /// the code/addr used to start each SPI transaction
    uint8_t addr_cmd;
    /// used to expose the Dispatch::Callback interface
    Dispatch::Callback *handler;

    class Handler;

public:
    MCP23S17(SPI *spi, GPIO *cs, uint8_t addr);
    ~MCP23S17();

    /// MCP23S17 has two 8-bit ports
    enum Port {
        PORTA = 1,
        PORTB
    };

    /// each GPIO supports these modes
    enum Mode {
        OUTPUT,
        INPUT,
        /// input with internal pull-ups
        INPUT_PU,
        /// interrupt on falling edge
        IT_FALLING, 
        /// interrupt on rising edge
        IT_RISING,  
    };

    /// Hardware map of device's registers
    enum Register
    {
        R_IODIRA    = 0x00, // IO Direction
        R_IODIRB    = 0x01,
        R_GPINTENA  = 0x04, // Interrupt Enable
        R_GPINTENB  = 0x05,
        R_DEFVALA   = 0x06, // Interrupt compare value
        R_DEFVALB   = 0x07,
        R_ICON      = 0x0a, // Config
        R_GPPUA     = 0x0c, // Pull-ups
        R_GPPUB     = 0x0d,
        R_INTFA     = 0x0e, // Interrupt flag
        R_INTFB     = 0x0f,
        R_GPIOA     = 0x12, // IO
        R_GPIOB     = 0x13,
    };
 
    uint8_t read(Register reg);
    void write(Register reg, uint8_t data);

    Cache *get_cache(Register reg);

    uint8_t read_cache(Register reg);
    void write_cache(Register reg, uint8_t data);
    bool flush_cache(Register reg);

    GPIO * make_gpio(Port port, int bit, Mode mode, bool auto_flush);

    // implement Dispatch interface : called from Task on GPIO irq
    Dispatch::Callback *get_interrupt_handler();

    void _on_interrupt();
    void _set_pin(GPIO *pin, Port port, int bit);
};

} // namespace panglos

#endif // __MCP23S17_H__

//  FIN
