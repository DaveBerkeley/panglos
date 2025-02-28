

#if defined(ESP32)
#include <driver/gpio.h>
#include <esp_netif.h>

#include "nvs_flash.h"
#endif // ESP32

#include "panglos/debug.h"

#include "panglos/device.h"
#include "panglos/object.h"
#include "panglos/time.h"
#include "panglos/mqtt.h"
#include "panglos/storage.h"

#if defined(ESP32)
#include "panglos/hal.h"
#include "panglos/esp32/gpio.h"
#include "panglos/esp32/i2c.h"
#include "panglos/esp32/uart.h"
#endif

#include "panglos/drivers/keyboard.h"
#include "panglos/drivers/mcp23s17.h"
#include "panglos/drivers/i2c.h"
#include "panglos/drivers/i2c_bitbang.h"
#include "panglos/drivers/motor.h"
#include "panglos/drivers/pwm.h"
#include "panglos/drivers/7-segment.h"

#include "panglos/app/devices.h"

namespace panglos {

#define esp_error(err)    ASSERT_ERROR((err) == ESP_OK, "err=%s", lut(panglos::err_lut, (err)));

    /*
     *
     */

bool dummy_init(Device *dev, void *arg)
{
    PO_DEBUG("");
    IGNORE(dev);
    IGNORE(arg);
    return true;
}

    /*
     *
     */

#if defined(ESP32)

bool gpio_init(Device *dev, void *arg)
{
    PO_DEBUG("");
    ASSERT(arg);
    struct GPIO_DEF *def= (struct GPIO_DEF*) arg;

    ESP_GPIO *gpio = new ESP_GPIO(def->pin, def->mode, def->initial_state, false, dev->name);
    dev->add(Objects::objects, gpio);
    return true;    
}

bool reset_init(Device *dev, void *arg)
{
    PO_DEBUG("");

    const char *s = dev->find_has("reset");
    ASSERT(s);
    panglos::GPIO *gpio = (panglos::GPIO*) Objects::objects->get(s);
    ASSERT(gpio);

    gpio->set(0);
    Time::msleep(100);
    gpio->set(1);
    Time::msleep(10);

    return true;    
}

bool pwm_timer_init(Device *dev, void *arg)
{
    PO_DEBUG("");

    ASSERT(arg);
    PWM_TIMER_DEF *def = (PWM_TIMER_DEF*) arg;

    PWM_Timer *pwm = new PWM_Timer(def->timer, 5000, def->pins, def->verbose);

    for (int i = 0; i < (sizeof(def->pins)/sizeof(def->pins[0])); i++)
    {
        if (def->pins[i] != -1)
            PO_INFO("pin[%d]=%d", i, def->pins[i]);
    }

    dev->add(Objects::objects, pwm);

    return true;
}

bool i2c_init(Device *dev, void *arg)
{
    ASSERT(arg);
    struct I2C_DEF *def = (struct I2C_DEF *) arg;

    PO_DEBUG("chan=%d scl=%d sda=%d", def->chan, (int) def->scl, (int) def->sda);

    Mutex *m = Mutex::create();
    I2C *i2c = new ESP_I2C(def->chan, def->scl, def->sda, m, false);
    dev->add(Objects::objects, i2c);

    return true;
}

    /*
     *
     */

bool mqtt_init(Device *dev, void *arg)
{
    const char *net = arg ? (const char *) arg : "net";
    PO_DEBUG("net=%s", net);

    if (!Objects::objects->get(net))
    {
        PO_WARNING("No network, skipping MQTT init");
        return false;
    }

    MqttClient *mqtt = new MqttClient;
    dev->add(Objects::objects, mqtt);

    return true;
}

    /*
     *
     */

bool uart_init(panglos::Device *dev, void *arg)
{
    ASSERT(arg);
    struct UART_DEF *def = (struct UART_DEF*) arg;
    PO_DEBUG("chan=%d rx=%d tx=%d baud=%d", def->chan, def->rx, def->tx, def->baud);

    UART *uart = ESP_UART::create(def->chan, def->rx, def->tx, def->baud, false);
    dev->add(Objects::objects, uart);
    return true;
}

#endif  //  ESP32

    /*
     *
     */

bool seven_seg_init(Device *dev, void *arg)
{
    const char *name = arg ? (const char *) arg : "i2c";
    PO_DEBUG("i2c=%s", name);

    I2C *i2c = (I2C*) Objects::objects->get(name);
    ASSERT(i2c);
 
    SevenSegment *display = new SevenSegment(i2c);

    if (!display->probe())
    {
        PO_WARNING("No 7-segment display");
        delete display;
        return false;
    }

    PO_INFO("Found 7-segment display");
    display->probe(); // TODO : this should not be required, bug in i2c driver?
    display->init();
    display->write("");

    dev->add(Objects::objects, display);
    return true;
}

    /*
     *
     */

static void wait(void *arg)
{
    IGNORE(arg);
    // Wait function for bitbanged I2C interface
    for (volatile int i = 0; i < 100; i++)
    {
    }
}

bool i2c_bitbang_init(Device *dev, void *arg)
{
    PO_DEBUG("");

    IGNORE(arg);

    const char *s = dev->find_has("scl");
    ASSERT(s);
    panglos::GPIO *scl = (panglos::GPIO *) Objects::objects->get(s);
    ASSERT(scl);

    s = dev->find_has("sda");
    ASSERT(s);
    panglos::GPIO *sda = (panglos::GPIO *) Objects::objects->get(s);
    ASSERT(scl);

    I2C *i2c = new BitBang_I2C(0, scl, sda, wait, 0);
    dev->add(Objects::objects, i2c);

    return true;
}

bool port_init(Device *dev, void *)
{
    const char *s = dev->find_has("i2c");
    ASSERT(s);
    I2C *i2c = (I2C*) Objects::objects->get(s);
    ASSERT(i2c);

    const uint8_t addr = 0x20;

    if (!i2c->probe(addr, 1))
    {
        PO_ERROR("No MCP23S17 found at %#x", addr);
        return false;
    }

    void *port = new I2C_MCP23S17(i2c, addr);
    dev->add(Objects::objects, port);
    return true;
}

bool keyboard_init(Device *dev, void *arg)
{
    PO_DEBUG("");
    IGNORE(arg);

    const char *s;
    panglos::GPIO *gpio = 0;

    s = dev->find_has("port");
    ASSERT(s);
    MCP23S17 *port = (MCP23S17*) Objects::objects->get(s);
    if (!port)
    {
        PO_WARNING("can't create keyboard without MCP23S17");
        return false;
    }

    s = dev->find_has("irq");
    if (s)
    {
        gpio = (panglos::GPIO*) Objects::objects->get(s);
        ASSERT(gpio);
    }

    Keyboard *keyboard = new Keyboard(port, gpio);
    keyboard->init();

    dev->add(Objects::objects, keyboard);

    return true;
}

}   //  namespace panglos

//  FIN
