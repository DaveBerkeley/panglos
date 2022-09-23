
namespace panglos {

class RTC
{
public:
    virtual bool init() = 0;

    typedef struct {
        uint8_t sec;
        uint8_t min;
        uint8_t hour;
        uint8_t day;
        uint8_t month;
        uint16_t year;
    }   DateTime;

    virtual bool set(DateTime *dt) = 0;
    virtual bool get(DateTime *dt) = 0;
};

}   //  namespace panglos

//  FIN
