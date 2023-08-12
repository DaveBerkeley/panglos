
#include "panglos/list.h"

namespace panglos {

    /*
     *  https://www.json.org/json-en.html
     */

class Mutex;

namespace json {

    /*
     *  Text slice : points to start and end of the slice
     */

class Section
{
public:
    const char *s;
    const char *e;

    bool skip(char c='\0');
    bool match(const char *s);
    char *strncpy(char *buff, size_t s);

    Section()
    :   s(0),
        e(0)
    {
    }

    Section(const char *t)
    :   s(t),
        e(& t[strlen(t)-1])
    {
    }

    Section(const char *_s, const char *_e)
    :   s(_s),
        e(_e)
    {
    }
};

    /*
     *  Utility class for easy printing of Section
     */

class Printer
{
    char *buff;
    size_t limit;
public:
    Printer(Section *sec, size_t _limit=80);
    ~Printer();
    const char *get();
};

    /*
     *  Callback class
     */

class Handler
{
public:
    virtual void on_object(bool push) = 0;
    virtual void on_array(bool push) = 0;
    virtual void on_number(Section *sec) = 0;
    virtual void on_string(Section *sec, bool key) = 0;
    virtual void on_primitive(Section *sec) = 0;
};

    /*
     *
     */

class Parser
{
public:
    enum Error {
        OKAY = 0,
        COLON_EXPECTED,
        KEY_EXPECTED,
        CLOSE_BRACE_EXPECTED,
        CLOSE_BRACKET_EXPECTED,
        UNTERMINATED_STRING,
    };

private:
    Handler *handler;
    bool verbose;
    enum Error err;
    Section err_sec;

    bool string(Section *sec, bool key);
    bool number(Section *sec);
    bool primitive(Section *sec);

    bool object(Section *sec);
    bool array(Section *sec);
    bool value(Section *sec);

    bool error(Section *sec, enum Error _err);

public:
    Parser(Handler *handler, bool verbose=true);

    bool parse(Section *sec);
 
    static const LUT err_lut[];
    enum Error get_error(Section *sec);
};

    /*
     *
     */

class Match : public Handler
{
public:
    enum Type { STRING, NUMBER, PRIMITIVE };

    struct Item {
        const char **keys;
        void (*on_match)(void *arg, Section *sec, enum Type type, const char **keys);
        void *arg;

        struct Item *next;
        static struct Item **get_next(struct Item *d) { return & d->next; }
    };

private:
    class Level;

    void check(Section *sec, enum Type type);

    // Handler used to check match
    virtual void on_object(bool push) override;
    virtual void on_array(bool push) override;
    virtual void on_number(Section *sec) override;
    virtual void on_string(Section *sec, bool key) override;
    virtual void on_primitive(Section *sec) override;

    int nest;
    int max_nest;
    Level *levels;

    typedef List<Item*> Items;
    Items items;

public:
    void add_item(struct Item *item, Mutex *m=0);

    Match(int nlevels=10);
    ~Match();
};

}   //  namespace json
}   //  namespace panglos

//  FIN
