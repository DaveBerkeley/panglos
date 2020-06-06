
namespace panglos {

class DMA
{
public:
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
