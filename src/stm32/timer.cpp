
#include <string.h>

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

void Timer::alloc(Timer::ID id)
{
    static bool allocated[20];

    ASSERT(id >= 0);
    ASSERT(id < sizeof(allocated));

    ASSERT_ERROR(!allocated[id], "Timer %d already used", id+1);
    allocated[id] = true;
}

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
    Timer::alloc(Timer::TIMER_2);
    __TIM2_CLK_ENABLE();
}

static void Init_Timer()
{
    Timer::alloc(Timer::TIMER_5);
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
    Timer::alloc(Timer::TIMER_4);
    __HAL_RCC_TIM4_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM4_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);

    HAL_TIM_Base_Start_IT(clock);
}

static void Init_Timer()
{
    Timer::alloc(Timer::TIMER_2);
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

    /*
     *
     */

class Arm_Timer : public Timer
{
    TIM_HandleTypeDef handle;
    ID id;

public:
    Arm_Timer(ID _id)
    :   id(_id)
    {
        alloc(id);
        memset(& handle, 0, sizeof(handle));

        // Set the instance
        switch (id)
        {
            case TIMER_1 : handle.Instance = TIM1;  break;
            case TIMER_2 : handle.Instance = TIM2;  break;
            case TIMER_3 : handle.Instance = TIM3;  break;
            case TIMER_4 : handle.Instance = TIM4;  break;
            case TIMER_5 : handle.Instance = TIM5;  break;
            case TIMER_6 : handle.Instance = TIM6;  break;
            case TIMER_7 : handle.Instance = TIM7;  break;
            case TIMER_8 : handle.Instance = TIM8;  break;
            case TIMER_9 : handle.Instance = TIM9;  break;
            default : ASSERT_ERROR(0, "timer %d not supported", id+1);
        }

        // Enable the clock
        switch (id)
        {
            case TIMER_1 : __TIM1_CLK_ENABLE();  break;
            case TIMER_2 : __TIM2_CLK_ENABLE();  break;
            case TIMER_3 : __TIM3_CLK_ENABLE();  break;
            case TIMER_4 : __TIM4_CLK_ENABLE();  break;
            case TIMER_5 : __TIM5_CLK_ENABLE();  break;
            case TIMER_6 : __TIM6_CLK_ENABLE();  break;
            case TIMER_7 : __TIM7_CLK_ENABLE();  break;
            case TIMER_8 : __TIM8_CLK_ENABLE();  break;
            case TIMER_9 : __TIM9_CLK_ENABLE();  break;
            default : ASSERT_ERROR(0, "timer %d not supported", id+1);
        }
    }

    virtual ID get_id() { return id; }

    virtual uint32_t get_counter()
    {
        return __HAL_TIM_GET_COUNTER(& handle);
    }
 
    virtual void init(uint32_t prescaler, uint32_t period, Chan chan) 
    {   
        HAL_StatusTypeDef status;

        // clock is 125 nS pulse (8MHz)
        // prescaler 0 gives 125 nS

        handle.Init.Prescaler = prescaler;
        handle.Init.CounterMode = TIM_COUNTERMODE_UP;
        handle.Init.Period = period;
        handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

        status = HAL_TIM_PWM_Init(& handle);
        ASSERT(status == HAL_OK);

        TIM_MasterConfigTypeDef sMasterConfig = { 0 };

        uint32_t trigger_out = TIM_TRGO_RESET;
        switch (chan)
        {
            case CHAN_1 : trigger_out = TIM_TRGO_OC1REF;  break;
            case CHAN_2 : trigger_out = TIM_TRGO_OC2REF;  break;
            case CHAN_3 : trigger_out = TIM_TRGO_OC3REF;  break;
            case CHAN_4 : trigger_out = TIM_TRGO_OC4REF;  break;
            default : ASSERT(0);
        }
        sMasterConfig.MasterOutputTrigger = trigger_out;

        sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
        status = HAL_TIMEx_MasterConfigSynchronization(& handle, & sMasterConfig);
        ASSERT(status == HAL_OK);
    }

    virtual void start()
    {
        HAL_StatusTypeDef status = HAL_TIM_Base_Start(& handle);
        ASSERT(status == HAL_OK);
    }    

    static uint32_t channel(Chan chan)
    {
        switch (chan)
        {
            case CHAN_1 : return TIM_CHANNEL_1;
            case CHAN_2 : return TIM_CHANNEL_2;
            case CHAN_3 : return TIM_CHANNEL_3;
            case CHAN_4 : return TIM_CHANNEL_4;
            default : ASSERT(0);
        }
        return 0;
    } 

    virtual void enable_dma()
    {
        //__HAL_TIM_ENABLE_DMA(& handle, TIM_DMA_TRIGGER);
        __HAL_TIM_ENABLE_DMA(& handle, TIM_DMA_UPDATE);
        /// TIMx_CC3_DMA_INST
    }

    virtual void enable_dma(Chan chan)
    {
        switch (chan)
        {
            case CHAN_1 : __HAL_TIM_ENABLE_DMA(& handle, TIM_DMA_CC1);  break;
            case CHAN_2 : __HAL_TIM_ENABLE_DMA(& handle, TIM_DMA_CC2);  break;
            case CHAN_3 : __HAL_TIM_ENABLE_DMA(& handle, TIM_DMA_CC3);  break;
            case CHAN_4 : __HAL_TIM_ENABLE_DMA(& handle, TIM_DMA_CC4);  break;
            default : ASSERT(0);
        }
    }

    virtual void start_oc(Chan chan)
    {
        HAL_StatusTypeDef okay;
        okay = HAL_TIM_OC_Start(& handle, channel(chan));
        ASSERT(okay == HAL_OK);
    }

    virtual void start_pwm(Chan chan, uint32_t value)
    {
        HAL_StatusTypeDef status;
        TIM_OC_InitTypeDef sConfigOC;

        sConfigOC.OCMode = TIM_OCMODE_PWM1;
        sConfigOC.Pulse = value;
        sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        status = HAL_TIM_PWM_ConfigChannel(& handle, &sConfigOC, channel(chan));
        ASSERT(status == HAL_OK);

        status = HAL_TIM_PWM_Start(& handle, channel(chan));
        ASSERT(status == HAL_OK);
    }    

    virtual void event()
    {
        HAL_StatusTypeDef status;
        status = HAL_TIM_GenerateEvent(& handle, TIM_EVENTSOURCE_TRIGGER);
        ASSERT(status == HAL_OK);
    }    
};

extern "C" void TIM3_IRQHandler()
{
    ASSERT(0);
}

    /*
     *
     */

Timer *Timer::create(Timer::ID id)
{
    return new Arm_Timer(id);
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
