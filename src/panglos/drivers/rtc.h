
namespace panglos {

class RTC
{
public:
    virtual ~RTC() {}

    virtual bool init() = 0;

    typedef struct {
        uint8_t sec;
        uint8_t min;
        uint8_t hour;
        uint8_t day;
        uint8_t month;
        uint16_t year;
    }   DateTime;

    static bool validate(const DateTime &dt);

    virtual bool set(const DateTime *dt) = 0;
    virtual bool get(DateTime *dt) = 0;

    static bool parse_time(const char *s, DateTime *dt, const char *fmt=0);
};

}   //  namespace panglos

//  FIN
