
#include <FreeRTOS.h>
#include <task.h>

#include "../panglos/stm32/stm32fxxx_hal.h"

#include "../panglos/debug.h"
#include "../panglos/mutex.h"
#include "../panglos/timer.h"

    /*
     *
     */

namespace panglos {

static TIM_HandleTypeDef htimx[2];

static TIM_HandleTypeDef *clock = & htimx[0];
static TIM_HandleTypeDef *timer = & htimx[1];

    /*
     *
     */

static const uint16_t prescaler = 800;

#if defined(STM32F4xx)

// The F4xx has 2 32-bit timers
#define TIM_CLOCK TIM2
#define TIM_TIMER TIM5

static void Init_Clock()
{
    __TIM2_CLK_ENABLE();
}

static void Init_Timer()
{
    __HAL_RCC_TIM5_CLK_ENABLE();
    /* TIM5 interrupt Init */
    HAL_NVIC_SetPriority(TIM5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM5_IRQn);
}

timer_t timer_now()
{
    // Use TIM2 32-bit counter as a time reference
    const uint32_t t = __HAL_TIM_GET_COUNTER(clock);
    return t;
}

#define TIMER_IRQ_HANDLER TIM5_IRQHandler

#endif // STM32F4xx

    /*
     *
     */

#if defined(STM32F1xx)

// The STM32F1xx doesn't have 32-bit timers
// so we have to use 16-bit ones and extend timer overflow
#define TIM_CLOCK TIM4
#define TIM_TIMER TIM2

static void Init_Clock()
{
    __HAL_RCC_TIM4_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM4_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);

    HAL_TIM_Base_Start_IT(clock);
}

static void Init_Timer()
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

static volatile uint16_t clock_overflow = 0;

timer_t timer_now()
{
    // Use TIM4 16-bit counter as a time reference
    const uint16_t o1 = clock_overflow;
    const uint32_t t1 = __HAL_TIM_GET_COUNTER(clock);
    const uint16_t o2 = clock_overflow;

    if (o1 == o2)
    {
        // usual case, timer is still counting up
        return t1 + (o1 << 16);
    }

    // timer has rolled over : need new reading
    const uint32_t t2 = __HAL_TIM_GET_COUNTER(clock);
    return t2 + (o2 << 16);
}

#define TIMER_IRQ_HANDLER TIM2_IRQHandler

#define _IRQ_HANDLER(n) TIM ## n ## _IRQHandler

extern "C" void _IRQ_HANDLER(4)(void)
{
    clock_overflow += 1;
    HAL_TIM_IRQHandler(clock);
}

#endif // STM32F1xx

    /*
     *  Use hardware timer to get the current time
     */

static TaskHandle_t event_h = 0;

void timer_init()
{
    HAL_StatusTypeDef status;

    event_h = xTaskGetCurrentTaskHandle();
    ASSERT(event_h);

    // start the timer
    timer->Init.Period = 1000;
    status = HAL_TIM_Base_Init(timer);
    ASSERT(status == HAL_OK);
    status = HAL_TIM_Base_Start_IT(timer);
    ASSERT(status == HAL_OK);

    PO_DEBUG("handle=%p", event_h);
}

    /*
     *
     */

static Mutex *timer_mutex = Mutex::create();

static d_timer_t last_dt = 0;

void timer_set(d_timer_t dt)
{
    Lock lock(timer_mutex);

#if defined(STM32F1xx)
    // The Cortex M3 only has 16-bit timers,
    // so we need to clip the timer interrupt period
    // then try again later ...
    if (dt > 0xFFFF)
    {
        dt = 0xFFFF;
    }
#endif
 
    if (last_dt && (last_dt < dt))
    {
        // we are already waiting for a more imminent event
        return;
    }

    last_dt = dt;

    HAL_StatusTypeDef status;

    status = HAL_TIM_Base_Stop(timer);
    ASSERT(status == HAL_OK);
    status = HAL_TIM_Base_Stop_IT(timer);
    ASSERT(status == HAL_OK);
    __HAL_TIM_SET_AUTORELOAD(timer, dt);
    __HAL_TIM_SET_COUNTER(timer, 0);
    status = HAL_TIM_Base_Start_IT(timer);
    ASSERT(status == HAL_OK);
    //PO_DEBUG("dt=%#x t=%#x", dt, dt + timer_now());
}

    /*
     *
     */

void timer_wait(panglos::d_timer_t dt)
{
    //PO_DEBUG("td=%#x", dt);
    if (dt == 0)
    {
        return;
    }
    timer_set(dt);
    // wait for signal
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

static void timer_irq()
{
    // clear the last scheduled wake-up
    last_dt = 0;

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

extern "C" void TIMER_IRQ_HANDLER(void)
{
    timer_irq();
    HAL_TIM_IRQHandler(timer);
}

    /**
      * @brief TIM5 Initialization Function
      * @param None
      * @retval None
      */

static void hw_timer_init(void)
{
    HAL_StatusTypeDef status;
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    Init_Timer();

    timer->Instance = TIM_TIMER;
    timer->Init.Prescaler = prescaler;
    timer->Init.CounterMode = TIM_COUNTERMODE_UP;
    timer->Init.Period = 0;
    timer->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer->Init.RepetitionCounter = 0; 
    //timer->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_OnePulse_MspInit(timer);

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    status = HAL_TIM_ConfigClockSource(timer, &sClockSourceConfig);
    ASSERT(status == HAL_OK);

    status = HAL_TIM_OnePulse_Init(timer, TIM_OPMODE_SINGLE);
    ASSERT(status == HAL_OK);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    status = HAL_TIMEx_MasterConfigSynchronization(timer, &sMasterConfig);
    ASSERT(status == HAL_OK);

    status = HAL_TIM_Base_Init(timer);
    ASSERT(status == HAL_OK);
}

    /**
      * @brief Timer Initialization Function
      * @param None
      * @retval None
      */

static void hw_clock_init(void)
{
    HAL_StatusTypeDef status;

    // TIMx is used as a free-running time reference
    clock->Instance = TIM_CLOCK;
    clock->Init.Prescaler = prescaler;
    clock->Init.CounterMode = TIM_COUNTERMODE_UP;
    clock->Init.Period = 0xFFFFFFFF;
    clock->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    clock->Init.RepetitionCounter = 0;

    // Configure TIMx
    Init_Clock();
    status = HAL_TIM_Base_Init(clock);
    ASSERT(status == HAL_OK);
    status = HAL_TIM_Base_Start(clock);
    ASSERT(status == HAL_OK);
}

} // namespace panglos

    /*
     *
     */

void Timer_Init()
{
    panglos::hw_clock_init();
    panglos::hw_timer_init();
}

//  FIN
