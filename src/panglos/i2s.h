
namespace panglos {

class I2S
{
public:
    enum ID
    {
        I2S_1 = 1,
        I2S_2 = 2,
    };

    virtual int rx(uint16_t* data, int len) = 0;

    static I2S *create(ID id);
};

}   //  namespace panglos

// FIN
