
#include "panglos/io.h"

namespace panglos {

class JsonOut : public FmtOut
{
    int objects;

    void key(const char *label);
public:
    JsonOut(Out *out);

    void start_obj();
    void key_value(const char *label, const char *text);
    void key_value(const char *label, const char *fmt, double d);
    void key_value(const char *label, int d);
    void key_dt(const char *key, const char *zone=0);
    void key_dt(const char *key, const struct tm *tm, const char *zone=0);
    void end_obj();
};

}   //  namespace panglos

//  FIN

