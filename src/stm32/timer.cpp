
#include <FreeRTOS.h>
#include <task.h>

#include "../panglos/stm32/stm32fxxx_hal.h"

#include "../panglos/debug.h"
#include "../panglos/mutex.h"
#include "../panglos/timer.h"

    /*
     *
     */

static TIM_HandleTypeDef htimx[2];

static TIM_HandleTypeDef *clock = & htimx[0];
static TIM_HandleTypeDef *timer = & htimx[1];

#if defined(STM32F4xx)

#define TIM_CLOCK TIM2
#define TIM_TIMER TIM5

#define TIMER_CLK_ENABLE __TIM2_CLK_ENABLE

static const uint16_t prescaler = 128;

static void Init_Timer_Irq()
{
    __HAL_RCC_TIM5_CLK_ENABLE();
    /* TIM5 interrupt Init */
    HAL_NVIC_SetPriority(TIM5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM5_IRQn);
}

#endif // STM32F4xx

    /*
     *
     */

#if defined(STM32F1xx)

#define TIM_CLOCK TIM2
#define TIM_TIMER TIM3

#define TIMER_CLK_ENABLE __HAL_RCC_TIM2_CLK_ENABLE

static const uint16_t prescaler = 1024;

static void Init_Timer_Irq()
{
    __HAL_RCC_TIM3_CLK_ENABLE();
    /* TIM3 interrupt Init */
    HAL_NVIC_SetPriority(TIM3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

#endif // STM32F1xx

namespace panglos {

/*
 *  Use hardware timer to get the current time
 */

timer_t timer_now()
{
    // Use TIM2 32-bit counter as a time reference
    const uint32_t t = __HAL_TIM_GET_COUNTER(clock);
    return t;
}

static TaskHandle_t event_h = 0;

void timer_init()
{
    event_h = xTaskGetCurrentTaskHandle();

    // start the timer
    timer->Init.Period = 1000;
    HAL_TIM_Base_Init(timer);
    HAL_TIM_Base_Start_IT(timer);
}

static Mutex *timer_mutex = Mutex::create();

void timer_set(d_timer_t dt)
{
    Lock lock(timer_mutex);

    __HAL_TIM_SET_AUTORELOAD(timer, dt);
    __HAL_TIM_SET_COUNTER(timer, 0);
    HAL_TIM_Base_Start_IT(timer);
}

void timer_wait(panglos::d_timer_t dt)
{
    timer_set(dt ? dt : 100000);
    // wait for signal
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

static void timer_irq()
{
    // signal waiting task
    if (event_h)
    {
        BaseType_t woken = pdFALSE;
        vTaskNotifyGiveFromISR(event_h, & woken);
        portYIELD_FROM_ISR(woken);
    }
}

void *get_task_id()
{
    return xTaskGetCurrentTaskHandle();
}

} // namespace panglos

#if defined(STM32F4xx)
extern "C" void TIM5_IRQHandler(void)
#endif
#if defined(STM32F1xx)
extern "C" void TIM3_IRQHandler(void)
#endif
{
    panglos::timer_irq();
    HAL_TIM_IRQHandler(timer);
}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */

static void hw_timer_init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    timer->Instance = TIM_TIMER;
    timer->Init.Prescaler = prescaler;
    timer->Init.CounterMode = TIM_COUNTERMODE_UP;
    timer->Init.Period = 0;
    timer->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(timer) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(timer, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_OnePulse_Init(timer, TIM_OPMODE_SINGLE) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(timer, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    Init_Timer_Irq();
}

/**
  * @brief Timer Initialization Function
  * @param None
  * @retval None
  */

static void hw_clock_init(void)
{
    // Timer 2 is used as a free-running time reference
    clock->Instance = TIM_CLOCK;
    clock->Init.Prescaler = prescaler;
    clock->Init.CounterMode = TIM_COUNTERMODE_UP;
    clock->Init.Period = 0xFFFFFFFF;
    clock->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    clock->Init.RepetitionCounter = 0;

    // Configure timer 2
    TIMER_CLK_ENABLE();
    HAL_TIM_Base_Init(clock);
    HAL_TIM_Base_Start(clock);
}

void Timer_Init()
{
    hw_clock_init();
    hw_timer_init();
}

//  FIN
