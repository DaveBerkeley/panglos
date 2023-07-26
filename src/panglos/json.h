
namespace panglos {

    /*
     *  https://www.json.org/json-en.html
     */

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
    class Level;

    void check(Section *sec);

    virtual void on_object(bool push) override;
    virtual void on_array(bool push) override;
    virtual void on_number(Section *sec) override;
    virtual void on_string(Section *sec, bool key) override;
    virtual void on_primitive(Section *sec) override;

    const char **keys;
    int nest;
    int max_nest;
    Level *levels;

public:
    virtual void on_match(Section *sec) = 0;

    Match(const char **keys, int nlevels=10);
    ~Match();
};

}   //  namespace json
}   //  namespace panglos

//  FIN
