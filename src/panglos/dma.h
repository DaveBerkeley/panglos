
namespace panglos {

class DMA
{
public:
#if defined(STM32F4xx)
    typedef DMA_Stream_TypeDef Instance;
#endif
#if defined(STM32F1xx)
    typedef DMA_Channel_TypeDef Instance;
#endif

    DMA_HandleTypeDef handle;

    DMA();

    virtual ~DMA() {}

    virtual IRQn_Type get_irq() = 0;
    virtual void clock_enable() = 0;
    virtual void link() = 0;

    void init(int irq_level);
};

}   //  namespace panglos

//  FIN
