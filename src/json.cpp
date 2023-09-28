    /*
     *
     */

#include <string.h>
#include <stdlib.h>

#include "panglos/debug.h"

#include "panglos/json.h"

namespace panglos {
namespace json {

#define WHITESPACE " \t\r\n"
static const char *whitespace = WHITESPACE;
static const char *numeric = "0123456789+-.eE";

bool Section::skip(char c)
{
    while (s <= e)
    {
        if (c == *s)
        {
            s += 1;
            return true;
        }
        if (!strchr(whitespace, *s))
        {
            return false;
        }
        //PO_DEBUG("skip '%c'", *s);
        s += 1;
    }
    return false;
}

bool Section::match(const char *match)
{
    if (!match) return false;
    const int size = (int) strlen(match);
    if (size != (1 + e - s)) return false;
    return strncmp(match, s, size) == 0;
}

char *Section::strncpy(char *buff, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        buff[i] = '\0';
        if (& s[i] > e)
        {
            return buff;
        }
        buff[i] = s[i];
    }
    buff[size-1] = '\0'; // always terminate the string
    return buff;
}

char *Section::strdup()
{
    size_t size = 1 + e - s;
    char *str = (char *) malloc(size+1);
    ::strncpy(str, s, size);
    str[size] = '\0';
    return str;
}

bool Section::to_int(int *d, int base)
{
    ASSERT(d);
    char *end = 0;
    *d = (int) strtol(s, & end, base);
    return end == (e + 1);
}

bool Section::to_double(double *d)
{
    ASSERT(d);
    char *end = 0;
    *d = strtof(s, & end);
    return end == (e + 1);
}

bool Section::to_bool(bool *d)
{
    ASSERT(d);
    if (match("true"))
    {
        *d = true;
        return true;
    }
    if (match("false"))
    {
        *d = false;
        return true;
    }
    return false;
}

const char *json::Section::find(char c)
{
    for (const char *t = s; t <= e; t++)
    {
        if (c == *t)
        {
            return t;
        }
    }
    return 0;
}

bool json::Section::split(json::Section *head, char c)
{
    ASSERT(head);
    const char *f = 0;
    while (!!(f = find(c)))
    {
        head->s = s;
        head->e = f-1;
        s = f+1;
        return true;
    }

    *head = *this;
    clear();
    return false;
}

bool json::Section::empty()
{
    if ((s == 0) || (e == 0))
    {
        return true;
    }
    return s > e;
}

    /*
     *  Printer class
     */

Printer::Printer(json::Section *sec, size_t _limit)
:   buff(0),
    limit(_limit)
{
    size_t size = 2 + sec->e - sec->s;
    if (size > limit)
        size = limit;
    buff = new char[size];
    memcpy(buff, sec->s, size-1);
    buff[size-1] = '\0';
}

Printer::~Printer()
{
    delete[] buff;
}

const char *Printer::get()
{
    return buff;
}

    /*
     *
     */

const LUT *Handler::get_lut()
{
    static const LUT table[] = {
        { 0, 0 },
    };
    return table;
}

    /*
     *
     */

Parser::Parser(Handler *h, bool v)
:   handler(h),
    verbose(v),
    err(OKAY)
{
    ASSERT(handler);
}

enum Parser::Error Parser::get_error(Section *sec)
{
    if (sec)
    {
        *sec = err_sec;
    }
    return err;
}

const LUT Parser::err_lut[] = {
    {   "OKAY", Parser::OKAY },
    {   "COLON_EXPECTED", Parser::COLON_EXPECTED },
    {   "KEY_EXPECTED", Parser::KEY_EXPECTED },
    {   "CLOSE_BRACE_EXPECTED", Parser::CLOSE_BRACE_EXPECTED },
    {   "CLOSE_BRACKET_EXPECTED", Parser::CLOSE_BRACKET_EXPECTED },
    {   "UNTERMINATED_STRING", Parser::UNTERMINATED_STRING },
    {   0, 0 },
};

bool Parser::error(Section *sec, enum Error _err)
{
    err = _err;
    err_sec = *sec;
    if (verbose)
    {
        PO_ERROR("%s", lut(err_lut, err));
    }
    return false;
}

bool Parser::user_error(Section *sec, enum Handler::Error _err)
{
    err = USER_ERROR;
    err_sec = *sec;
    if (verbose)
    {
        PO_ERROR("%s", lut(handler->get_lut(), _err));
    }
    return false;
}

bool Parser::error_check(Section *sec, enum Handler::Error err)
{
    if (err != Handler::OKAY)
    {
        return user_error(sec, err);
    }
    return true;
}

    /*
     *
     */

bool Parser::object(Section *sec)
{
    //PO_DEBUG("OBJECT %-*s", int(sec->e - sec->s), sec->s);

    ASSERT((*sec->s) == '{');
    sec->s += 1;

    // An Object contains a sequence of one or more key : value pairs,
    Handler::Error err = handler->on_object(true);
    if (err != Handler::OKAY)
    {
        return user_error(sec, err);
    }

    while (sec->s <= sec->e)
    {
        sec->skip();

        if ((*sec->s) == '}')
        {
            break;
        }

        if ((*sec->s) != '"')
        {
            return error(sec, KEY_EXPECTED);
        }

        if (!string(sec, true)) // key
        {
            return false;
        }

        if (!sec->skip(':'))
        {
            return error(sec, COLON_EXPECTED);
        }

        if (!value(sec))
        {
            return false;
        }

        if (!sec->skip(','))
        {
            break;
        }
    }

    if (!sec->skip('}'))
    {
        return error(sec, CLOSE_BRACE_EXPECTED);
    }

    return error_check(sec, handler->on_object(false));
}

bool Parser::array(Section *sec)
{
    //PO_DEBUG("ARRAY %-*s", int(sec->e - sec->s), sec->s);
    ASSERT((*sec->s) == '[');
    sec->s += 1;

    // Array is a list of zero or more "," seperated values
    Handler::Error err = handler->on_array(true);
    if (err != Handler::OKAY)
    {
        return user_error(sec, err);
    }

    while (sec->s <= sec->e)
    {
        sec->skip();

        if ((*sec->s) == ']')
        {
            break;
        }

        if (!value(sec))
        {
            return false;
        }

        if (!sec->skip(','))
        {
            break;
        }
    }

    if (!sec->skip(']'))
    {
        return error(sec, CLOSE_BRACKET_EXPECTED);
    }

    return error_check(sec, handler->on_array(false));
}

bool Parser::number(Section *sec)
{
    //PO_DEBUG("NUMBER %-*s", int(sec->e - sec->s), sec->s);

    const char *s = sec->s;
    Section num(s, 0);

    while (sec->s <= sec->e)
    {
        if (!strchr(numeric, *s))
        {
            break;
        }
        num.e = s;
        s += 1;
    }
    sec->s = s;

    return error_check(sec, handler->on_number(& num));
}

bool Parser::string(Section *sec, bool key)
{
    // called wth ->s pointing to leading '"'
    //PO_DEBUG("STRING %-*s", int(sec->e - sec->s), sec->s);
    ASSERT(sec->s[0] == '"');

    const char *s = sec->s + 1;
    bool escape = false;

    Section str(s, 0);
    while (s <= sec->e)
    {
        if ((*s == '"') && !escape)
        {
            // end of string
            str.e = s - 1;
            break;
        }
        escape = escape ? false : (*s == '\\');
        s += 1;
    }

    if (!str.e)
    {
        return error(sec, UNTERMINATED_STRING);
    }

    // step past the string
    sec->s = s + 1;

    return error_check(sec, handler->on_string(& str, key));
}

bool Parser::primitive(Section *sec)
{
    static const char *primitives[] = {
        "false", 
        "true",
        "null",
        0,
    };

    for (int i = 0; primitives[i]; i++)
    {
        const char *s = primitives[i];
        const int n = (int) strlen(s);
        if (strncmp(s, sec->s, n))
        {
            continue;
        }
        // check for trailing chars
        if (!strchr(WHITESPACE ",}]", sec->s[n]))
        {
            continue;
        }
        Section primitive = { s, s+n-1 };
        Handler::Error err = handler->on_primitive(& primitive);
        if (err != Handler::OKAY)
        {
            return user_error(sec, err);
        }
        sec->s += n;
        return true;
    }

    return false;
}

bool Parser::value(Section *sec)
{
    //PO_DEBUG("VALUE %-*s", int(sec->e - sec->s), sec->s);

    sec->skip();

    if (strchr(numeric, *sec->s))
    {
        return number(sec);
    }

    switch (*sec->s)
    {
        case '{' : return object(sec);
        case '[' : return array(sec);
        case '"' : return string(sec, false);
        default  : return primitive(sec);
    }
}

bool Parser::parse(Section *sec)
{
    //PO_DEBUG("'%s'", sec->s);
    return value(sec);
}

    /*
     *
     */

class Match::Level
{
public:
    enum Type { OBJECT, ARRAY, NONE };
    Section key;
    enum Type type;
    int idx;

    Level()
    :   type(NONE),
        idx(-1)
    {
        set_key(0);
    }

    void init(enum Type t)
    {
        type = t;
        set_key(0);
        idx = -1;
    }
    void set_key(Section *sec)
    {
        if (sec)
        {
            key = *sec;
        }
        else
        {
            key.s = key.e = 0;
        }
    }
};

Match::Match(bool _verbose, int nlevels)
:   verbose(_verbose),
    nest(0),
    max_nest(nlevels),
    levels(0),
    items(Item::get_next)
{
    if (verbose) PO_DEBUG("");
    levels = new Level[nlevels];
}

Match::~Match()
{
    delete[] levels;
}

LUT Match::type_lut[] = {
    {   "STRING", STRING, },
    {   "NUMBER", NUMBER, },
    {   "PRIMITIVE", PRIMITIVE, },
    { 0, 0 },
};

void Match::add_item(struct Item *item, Mutex *m)
{
    if (verbose) PO_DEBUG("");
    items.push(item, m);
}

void Match::check(Section *sec, enum Type type)
{
    if (verbose)
    {
        json::Printer p(sec);
        PO_DEBUG("%s", p.get());
    }
    if (nest >= max_nest) return;

    struct Item *item = 0;

    for (item = items.head; item; item = item->next)
    {
        const char **keys = item->keys;

        for (int i = 1; i <= nest; i++)
        {
            Level *level = & levels[i];
            const char *key = keys[i-1];
            //PO_DEBUG("nest=%d i=%d %s", nest, i, key);
            if (!key)
                break;
            if (!level->key.match(key))
                break;

            if (keys[i] == 0)
            {
                ASSERT(item->on_match);
                item->on_match(item->arg, sec, type, item->keys);
                break;
            }
        }
    }
}

enum Match::Error Match::on_object(bool push)
{
    nest += push ? 1 : -1;
    //PO_DEBUG("nest=%d push=%d", nest, push);
    if (nest >= max_nest) return OKAY;
    if (push)
    {
        levels[nest].init(Level::OBJECT);
    }
    return OKAY;
}

enum Match::Error Match::on_array(bool push)
{
    nest += push ? 1 : -1;
    //PO_DEBUG("nest=%d push=%d", nest, push);
    if (nest >= max_nest) return OKAY;
    if (push)
    {
        levels[nest].init(Level::ARRAY);
    }
    return OKAY;
}

enum Match::Error Match::on_number(Section *sec)
{
    if (nest >= max_nest) return OKAY;
    check(sec, Match::NUMBER);
    levels[nest].set_key(0);
    return OKAY;
}

enum Match::Error Match::on_string(Section *sec, bool key)
{
    if (nest >= max_nest) return OKAY;
    if (key)
    {
        if (verbose)
        {
            json::Printer p(sec);
            PO_DEBUG("key:%s", p.get());
        }
        ASSERT(levels[nest].type == Level::OBJECT);
        levels[nest].set_key(sec);
    }
    else
    {
        check(sec, Match::STRING);
        levels[nest].set_key(0);
    }
    return OKAY;
}

enum Match::Error Match::on_primitive(Section *sec)
{
    if (nest >= max_nest) return OKAY;
    check(sec, Match::PRIMITIVE);
    levels[nest].set_key(0);
    return OKAY;
}

}   //  namespace json
}   //  namespace panglos

//  FIN
