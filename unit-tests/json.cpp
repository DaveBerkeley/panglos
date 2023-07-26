
#include <gtest/gtest.h>
#include <fcntl.h>

#include "panglos/debug.h"

    /*
     *
     */

#include "panglos/json.h"

using namespace panglos;

class Printer
{
    char *buff;
    size_t limit;
public:
    Printer(Section *sec, size_t _limit=80)
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

    ~Printer()
    {
        delete buff;
    }

    const char *get()
    {
        return buff;
    }
};

    /*
     *
     */

class P : public Handler
{
public:
    int depth;

    struct Level {
        enum Type { NONE, OBJECT, ARRAY, };

        int index;
        Section key;
        enum Type type;
    };

    struct Match {
        enum Type { NONE, KEY, INDEX };
        enum Type type;
        const char *key;
        int idex;
    };

    const static int max_levels = 10;
    struct Level levels[max_levels];

    struct Match *match;

    P(struct Match *_match=0)
    :   depth(0),
        match(_match)
    {
        memset(levels, 0, sizeof(levels));
        levels[0].type = Level::NONE;
    }

    void push(Level::Type type)
    {
        depth += 1;
        ASSERT(depth <= max_levels);
        struct Level *level = & levels[depth];
        level->type = type;
        level->key.s = 0;
        level->key.e = 0;
        level->index = 0;
    }

    void pop()
    {
        depth -= 1;
        ASSERT(depth >= 0);
    }

    void print_level(struct Level *level)
    {
        switch (level->type)
        {
            case Level::OBJECT :
            {
                char key[64] = { 0 };
                strncpy(key, level->key.s, 1 + level->key.e - level->key.s);
                printf("%s/", key);
                break;
            }
            case Level::ARRAY :
            {
                printf("[%d]", level->index);
                break;
            }
            case Level::NONE :
            {
                break;
            }
            default :
            {
                ASSERT(0);
            }
        }
    }

    void check(Section *sec)
    {
        for (int i = 0; i <= depth; i++)
        {
            print_level(& levels[i]);
        }
        Printer p(sec);
        printf(" %s \n", p.get());

        if (match)
        {
            for (int i = 0; i < depth; i++)
            {
                Match *m = & match[i];
                Level *l = & levels[i+1];
                PO_DEBUG("m=%d l=%d", m->type, l->type);
            }
        }
    }

    void on_item(Section *sec, bool key)
    {
        struct Level *level = & levels[depth];
        if (level->type == Level::ARRAY)
        {
            level->index += 1;
        }

        if (key)
        {
            level->key = *sec;
        }
        else
        {
            check(sec);
        }
    }

    virtual void on_object(bool _push) override
    {
        PO_DEBUG("%d", _push);
        if (_push)
        {
            push(Level::OBJECT);
        }
        else
        {
            pop();
        }
    }

    virtual void on_array(bool _push) override
    {
        PO_DEBUG("%d", _push);
        if (_push)
        {
            push(Level::ARRAY);
        }
        else
        {
            pop();
        }
    }

    virtual void on_number(Section *sec) override
    {
        Printer p(sec);
        PO_DEBUG("'%s'", p.get());

        on_item(sec, false);
    }

    virtual void on_string(Section *sec, bool key) override
    {
        Printer p(sec);
        PO_DEBUG("'%s' key=%d", p.get(), key);

        on_item(sec, key);
    }

    virtual void on_primitive(Section *sec) override
    {
        Printer p(sec);
        PO_DEBUG("'%s'", p.get());

        on_item(sec, false);
    }
};

TEST(Json, Int)
{
    const char *tests[] = {
        "1234",
        "12.34e-7",
        "\"text string\\\\this\"",
        "{\"label\": 1234, \"other\": 3456, \"thing\" : true, \"abcd\": 12.34 }",
        "{\"sun\": {\"alt\": 0.9233672, \"az\": 3.902},"
            " \"moon\": {\"alt\": 0.1942122578, \"az\": 2.1646382808685303, \"phase\": 0.4673593289770038},"
            " \"mercury\": {\"alt\": 0.9328691363334656, \"az\": 3.231799840927124},"
            " \"mars\": {\"alt\": 0.8334254026412964, \"az\": 2.8750030994415283},"
            " \"venus\": {\"alt\": 0.834154486656189, \"az\": 3.184415102005005},"
            " \"jupiter\": {\"alt\": -0.0059446850791573524, \"az\": 5.141014099121094},"
            " \"saturn\": {\"alt\": -0.8723456859588623, \"az\": 6.078963279724121},"
            " \"time\": \"2023/07/25 14:07:57\", \"z\": \"local\"}",
        "{\"test\": [ 123, 12.34, \"hello\", 456, \"world\" ], \"z\": \"local\"}",
        "{   }",
        "{}",
        "[ 123, 12.34, \"hello\", 456, \"world\" ]",
        "[  ] ",
        " \"hello\\uabcd world\" ",
        " -0 ",
        0,
    };

    struct P::Match match[] = {
        {   P::Match::KEY, "moon" },
        {   P::Match::KEY, "phase" },
        {   P::Match::NONE },
    };

    for (int i = 0; tests[i]; i++)
    {

        const char *json = tests[i];
        PO_DEBUG("-------- %s", json);
        P handler(match);
        Section sec = { json, & json[strlen(json)-1] };
        Parser p(& handler);
        bool ok = p.parse(& sec);
        EXPECT_TRUE(ok);
    }
}


TEST(Json, File)
{
    // Check against test data from : 
    // https://microsoftedge.github.io/Demos/json-dummy-data/

    struct Item {
        const char *path;
        enum Parser::Error err;
    };

    const struct Item tests[] = {
        { "unit-tests/data/5MB.json", Parser::OKAY },
        { "unit-tests/data/5MB-min.json", Parser::OKAY },
        { "unit-tests/data/missing-colon.json", Parser::MISSING_COLON },
        { "unit-tests/data/unterminated.json", Parser::UNTERMINATED_STRING },
        { "unit-tests/data/binary-data.json", Parser::KEY_EXPECTED },
        { 0, Parser::OKAY },
    };

    for (int i = 0; tests[i].path; i++)
    {
        const char *path = tests[i].path;
        PO_DEBUG("-------- %s", path);

        int fd = open(path, O_RDONLY);
        ASSERT_ERROR(fd > 0, "error opening file '%s'", path);

        struct stat stat;
        int err = fstat(fd, & stat);
        ASSERT_ERROR(err >= 0, "err=%d", err);

        PO_DEBUG("size=%ld", stat.st_size);

        char *data = (char *) malloc(stat.st_size);
        ASSERT(data);

        ssize_t s = read(fd, data, stat.st_size);
        ASSERT_ERROR(s == stat.st_size, "s=%d", (int) s);

        close(fd);

        P handler;
        Section sec = { data, & data[stat.st_size-1] };
        Parser p(& handler);
        bool ok = p.parse(& sec);
        enum Parser::Error e = p.get_error(0);
        EXPECT_EQ(e, tests[i].err);
        EXPECT_EQ(ok, tests[i].err == Parser::OKAY);
    }
}

//  FIN
