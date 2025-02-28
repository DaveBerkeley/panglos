
    /*
     *
     */

#pragma once

#include <stdint.h>

#include "panglos/device.h"

namespace panglos {

struct GPIO_DEF {
    int pin;
    int mode;
    bool initial_state;
};

struct I2C_DEF {
    int chan;
    uint32_t scl;
    uint32_t sda;
};

struct PWM_TIMER_DEF {
    int timer;
    int pins[6];
    bool verbose;
};

struct UART_DEF {
    int chan;
    int rx;
    int tx;
    int baud;
};

    /*
     *
     */

bool gpio_init(Device *dev, void *arg);
bool pwm_timer_init(Device *dev, void *arg);
bool seven_seg_init(Device *dev, void *arg);
bool i2c_init(Device *dev, void *arg);
bool i2c_bitbang_init(Device *dev, void *arg);
bool port_init(Device *dev, void *arg);
bool keyboard_init(Device *dev, void *arg);
bool mqtt_init(Device *dev, void *arg);
bool uart_init(Device *dev, void *arg);
bool dummy_init(Device *dev, void *arg);
bool reset_init(Device *dev, void *arg);

}   //  namespace panglos

//  FIN
