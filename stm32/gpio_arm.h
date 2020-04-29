
#if !defined(__GPIO_ARM_H__)
#define __GPIO_ARM_H__

namespace panglos {

GPIO * gpio_create(GPIO_TypeDef *_port, uint16_t _pin, uint32_t mode);

} // namespace panglos

#endif // __GPIO_ARM_H__

// FIN
