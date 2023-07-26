
namespace panglos {

/*
 *  https://www.json.org/json-en.html
 */

class Section
{
public:
    const char *s;
    const char *e;

    bool skip(char c='\0');
};

    /*
     *
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
        MISSING_COLON,
        KEY_EXPECTED,
        MISSING_CLOSE_BRACE,
        MISSING_CLOSE_BRACKET,
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

}   //  namespace panglos

//  FIN
