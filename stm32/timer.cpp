
#include <FreeRTOS.h>
#include <task.h>

#include "stm32f4xx_hal.h"

#include "../debug.h"
#include "../mutex.h"
#include "../timer.h"

static TIM_HandleTypeDef htim2;
static TIM_HandleTypeDef htim5;

namespace panglos {

/*
 *  Use hardware timer to get the current time
 */

timer_t timer_now()
{
    // Use TIM2 32-bit counter as a time reference
    const uint32_t t = __HAL_TIM_GET_COUNTER(& htim2);
    return t;
}

static TaskHandle_t event_h = 0;

void timer_init()
{
    event_h = xTaskGetCurrentTaskHandle();

    // start the timer
    htim5.Init.Period = 1000;
    HAL_TIM_Base_Init(& htim5);
    HAL_TIM_Base_Start_IT(& htim5);
}

static Mutex *timer_mutex = Mutex::create();

void timer_set(d_timer_t dt)
{
    Lock lock(timer_mutex);

    __HAL_TIM_SET_AUTORELOAD(& htim5, dt);
    __HAL_TIM_SET_COUNTER(& htim5, 0);
    HAL_TIM_Base_Start_IT(& htim5);
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

extern "C" void TIM5_IRQHandler(void)
{
    panglos::timer_irq();
    HAL_TIM_IRQHandler(& htim5);
}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */

static const uint16_t prescaler = 128;

static void Timer5_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim5.Instance = TIM5;
    htim5.Init.Prescaler = prescaler;
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 0;
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    //htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_OnePulse_Init(&htim5, TIM_OPMODE_SINGLE) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief Timer Initialization Function
  * @param None
  * @retval None
  */

static void Timer2_Init(void)
{
    // Timer 2 is used as a free-running time reference
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = prescaler;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 0xFFFFFFFF;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.RepetitionCounter = 0;

    // Configure timer 2
    __TIM2_CLK_ENABLE();
    HAL_TIM_Base_Init(& htim2);
    HAL_TIM_Base_Start(& htim2);
}

void Timer_Init()
{
    Timer2_Init();
    Timer5_Init();
}

//  FIN
