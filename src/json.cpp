    /*
     *
     */

#include <string.h>

#include "panglos/debug.h"

#include "panglos/json.h"

namespace panglos {

static const char *whitespace = " \t\r\n";
static const char *numeric = "0123456789+-.eE";

bool Section::skip(char c)
{
    bool found = c == '\0';

    while (*s)
    {
        found |= *s == c;
        if ((!strchr(whitespace, *s)) && (*s != c))
        {
            return found;
        }
        //PO_DEBUG("skip '%c'", *s);
        s += 1;
    }
    return found;
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
    memset(& err_sec, 0, sizeof(err_sec));
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
    {   "MISSING_COLON", Parser::MISSING_COLON },
    {   "KEY_EXPECTED", Parser::KEY_EXPECTED },
    {   "MISSING_CLOSE_BRACE", Parser::MISSING_CLOSE_BRACE },
    {   "MISSING_CLOSE_BRACKET", Parser::MISSING_CLOSE_BRACKET },
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

bool Parser::object(Section *sec)
{
    //PO_DEBUG("OBJECT %-*s", int(sec->e - sec->s), sec->s);

    ASSERT((*sec->s) == '{');
    sec->s += 1;

    // An Object contains a sequence of one or more key : value pairs,
    handler->on_object(true);

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
            return error(sec, MISSING_COLON);
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
        return error(sec, MISSING_CLOSE_BRACE);
    }

    handler->on_object(false);
    return true;
}

bool Parser::array(Section *sec)
{
    //PO_DEBUG("ARRAY %-*s", int(sec->e - sec->s), sec->s);
    ASSERT((*sec->s) == '[');
    sec->s += 1;

    // Array is a list of zero or more "," seperated values
    handler->on_array(true);

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
        return error(sec, MISSING_CLOSE_BRACKET);
    }

    handler->on_array(false);
    return true;
}

bool Parser::number(Section *sec)
{
    //PO_DEBUG("NUMBER %-*s", int(sec->e - sec->s), sec->s);

    const char *s = sec->s;
    Section num = { .s = s, .e = 0 };

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

    handler->on_number(& num);
    return true;
}

bool Parser::string(Section *sec, bool key)
{
    // called wth ->s pointing to leading '"'
    //PO_DEBUG("STRING %-*s", int(sec->e - sec->s), sec->s);
    ASSERT(sec->s[0] == '"');

    const char *s = sec->s + 1;
    bool escape = false;

    Section str = { .s = s, .e = 0 };
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

    handler->on_string(& str, key);
    return true;
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
        if (!strchr(" ,}]", sec->s[n]))
        {
            continue;
        }
        Section primitive = { s, s+n };
        handler->on_primitive(& primitive);
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

}   //  namespace panglos

//  FIN
