
namespace panglos {

class I2C;

class BMP280
{
    I2C *i2c;

    class Cal;
    Cal *cal;

public:
    BMP280(I2C *i2c);
    ~BMP280();

    struct Data {
        double temp;
        double pressure;
    };

    bool probe();
    bool init();

    uint8_t get_id();
    bool read(struct Data *);
};

}   //  namespace panglos
