
#include <gtest/gtest.h>
#include <fcntl.h>

#include "panglos/debug.h"

    /*
     *
     */

#include "panglos/json.h"

using namespace panglos;
using namespace json;

    /*
     *
     */

class P : public Handler
{
public:
    P() { }

    virtual enum Error on_object(bool push) override
    {
        IGNORE(push);
        return OKAY;
    }

    virtual enum Error on_array(bool push) override
    {
        IGNORE(push);
        return OKAY;
    }

    virtual enum Error on_number(Section *sec) override
    {
        IGNORE(sec);
        return OKAY;
    }

    virtual enum Error on_string(Section *sec, bool key) override
    {
        IGNORE(sec);
        IGNORE(key);
        return OKAY;
    }

    virtual enum Error on_primitive(Section *sec) override
    {
        IGNORE(sec);
        return OKAY;
    }
};

    /*
     *
     */

static void set_section(Section *sec, const char *str)
{
    ASSERT(sec);
    sec->s = str;
    sec->e = & str[strlen(str)-1];
}

    /*
     *
     */

TEST(Json, SectionMatch)
{
    Section sec;
    set_section(& sec, "hello");
    EXPECT_TRUE(sec.match("hello"));
    EXPECT_FALSE(sec.match("hell"));
    EXPECT_FALSE(sec.match("ello"));
    EXPECT_FALSE(sec.match("other"));
    EXPECT_FALSE(sec.match(""));
    EXPECT_FALSE(sec.match(" hello"));
    EXPECT_FALSE(sec.match(" hell"));
    EXPECT_FALSE(sec.match(0));
}

TEST(Json, SectionCpy)
{
    Section sec;
    const char *hello = "hello world!";

    {
        set_section(& sec, hello);
        char buff[64];
        char *s = sec.strncpy(buff, sizeof(buff));
        EXPECT_EQ(s, buff);
        EXPECT_STREQ(buff, hello);
    }
    {
        set_section(& sec, hello);
        char buff[4];
        char *s = sec.strncpy(buff, sizeof(buff));
        EXPECT_EQ(s, buff);
        EXPECT_STREQ(buff, "hel");
    }
    {
        set_section(& sec, "");
        char buff[4];
        char *s = sec.strncpy(buff, sizeof(buff));
        EXPECT_EQ(s, buff);
        EXPECT_STREQ(buff, "");
    }
}

TEST(Json, SectionDup)
{
    Section sec;
    const char *hello = "hello world!";

    set_section(& sec, hello);
    char *s = sec.strdup();
    EXPECT_STREQ(hello, s);
    free(s);
}

TEST(Json, SectionSkip)
{
    bool ok;
    Section sec("  {}");

    sec.skip();
    ok = sec.match("{}");
    EXPECT_TRUE(ok);
    ok = sec.skip('{');
    EXPECT_TRUE(ok);
    ok = sec.match("}");
    EXPECT_TRUE(ok);
    ok = sec.skip('}');
    EXPECT_TRUE(ok);
    ok = sec.match("");
    EXPECT_TRUE(ok);
}

TEST(Json, SectionFind)
{
    {
        const char *s = "hello|world";
        json::Section sec(s);

        const char *f = sec.find('|');
        EXPECT_TRUE(f);
        EXPECT_EQ(*f, '|');
    }
    {
        const char *s = "hello world";
        json::Section sec(s);

        const char *f = sec.find('|');
        EXPECT_FALSE(f);
    }
    {
        const char *s = "|";
        json::Section sec(s);

        const char *f = sec.find('|');
        EXPECT_TRUE(f);
        EXPECT_EQ(*f, '|');
    }
}

TEST(Json, SectionSplit)
{
    {
        const char *s = "hello|world";
        json::Section sec(s);
        json::Section head;

        bool ok;
        ok = sec.split(& head, '|');
        EXPECT_TRUE(ok);
        ok = head.match("hello");
        EXPECT_TRUE(ok);
        ok = sec.match("world");
        EXPECT_TRUE(ok);

        ok = sec.split(& head, '|');
        EXPECT_FALSE(ok);
        ok = head.match("world");
        EXPECT_TRUE(ok);
        ok = sec.empty();
        EXPECT_TRUE(ok);
    }
    {
        const char *s = "hello world";
        json::Section sec(s);
        json::Section head;

        bool ok;
        ok = sec.split(& head, '|');
        EXPECT_FALSE(ok);
        ok = head.match("hello world");
        EXPECT_TRUE(ok);
        ok = sec.empty();
        EXPECT_TRUE(ok);
    }
    {
        const char *s = "|world";
        json::Section sec(s);
        json::Section head;

        bool ok;
        ok = sec.split(& head, '|');
        EXPECT_TRUE(ok);
        ok = head.match("");
        EXPECT_TRUE(ok);
        ok = head.empty();
        EXPECT_TRUE(ok);
        ok = sec.match("world");
        EXPECT_TRUE(ok);
    }
    {
        const char *s = "world|";
        json::Section sec(s);
        json::Section head;

        bool ok;
        ok = sec.split(& head, '|');
        EXPECT_TRUE(ok);
        ok = head.match("world");
        EXPECT_TRUE(ok);
        ok = sec.empty();
        EXPECT_TRUE(ok);
        ok = sec.split(& head, '|');
        EXPECT_FALSE(ok);
        ok = head.empty();
        EXPECT_TRUE(ok);
    }
}

    /*
     *
     */

struct IntPair {
    const char *text;
    int n;
};

static void check_ints(const struct IntPair *pairs, bool state)
{
    for (; pairs->text; pairs++)
    {
        Section sec(pairs->text);

        bool ok;
        int n = 0;
        ok = sec.to_int(& n);
        EXPECT_EQ(ok, state);
        if (ok)
        {
            EXPECT_EQ(n, pairs->n);
        }
    }
}

TEST(Json, SectionInt)
{
    const struct IntPair good[] = {
        {   "1234", 1234, },
        {   "-1234", -1234, },
        {   "0", 0, },
        {   "-0", 0, },
        {   "123456789", 123456789, },
        {   "0x1234", 0x1234, },
        {   0, 0    },
    };

    check_ints(good, true);

    const struct IntPair bad[] = {
        {   "1234.0", 0, },
        {   "--2", 0, },
        //{   " 2", 0, }, // this passes!
        {   "ab12", 0, },
        {   "12a", 0, },
        {   ".123", 0, },
        {   0, 0    },
    };

    check_ints(bad, false);
}

struct DoublePair {
    const char *text;
    double n;
};

static void check_doubles(const struct DoublePair *pairs, bool state)
{
    for (; pairs->text; pairs++)
    {
        Section sec(pairs->text);

        bool ok;
        double n = 0;
        ok = sec.to_double(& n);
        EXPECT_EQ(ok, state);
        if (ok)
        {
            char a[24];
            char b[24];
            snprintf(a, sizeof(a), "%f", n);
            snprintf(b, sizeof(b), "%f", pairs->n);
            EXPECT_STREQ(a, b);
            //EXPECT_DOUBLE_EQ(n, pairs->n);
        }
    }
}

TEST(Json, SectionDouble)
{
    const struct DoublePair good[] = {
        {   "123", 123   },
        {   "123.25", 123.25   },
        {   "-123.25", -123.25   },
        {   ".25", 0.25   },
        {   "1e2", 100   },
        {   "2.5e-2", 0.025   },
        {   " 2.5e-2", 0.025   }, // leading spaces pass
        {   0, 0    },
    };

    check_doubles(good, true);

    const struct DoublePair bad[] = {
        {   "x123", 0   },
        {   "1e2 ", 0   }, // trailing spaces fail
        {   0, 0    },
    };

    check_doubles(bad, false);
}

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

    for (int i = 0; tests[i]; i++)
    {

        const char *json = tests[i];
        //PO_DEBUG("-------- %s", json);
        P handler;
        Section sec;
        set_section(& sec, json);
        Parser p(& handler);
        bool ok = p.parse(& sec);
        EXPECT_TRUE(ok);
    }
}

    /*
     *
     */

class PrimitiveState
{
public:
    bool t;
    bool f;
    bool n;

    PrimitiveState()
    :   t(false), f(false), n(false)
    {
    }
};

class PrimitiveHandler : public P
{
    PrimitiveState &state;
public:

    virtual enum Error on_primitive(Section *sec)
    {
        if (sec->match("true"))
        {
            state.t = true;
            return OKAY;
        }
        if (sec->match("false"))
        {
            state.f = true;
            return OKAY;
        }
        if (sec->match("null"))
        {
            state.n = true;
            return OKAY;
        }

        return P::on_primitive(sec);
    }

    PrimitiveHandler(PrimitiveState & s)
    :   state(s)
    {
    }
};

TEST(Json, Primitive)
{
    struct Test
    {
        const char *json;
        enum STATE { T, F, N, X };
        enum STATE state;
    };
    struct Test tests[] = {
        { "true",           Test::T, },
        { "true\n",         Test::T, },
        { "true ",          Test::T, },
        { " false",         Test::F, },
        { " false   ",      Test::F, },
        { " false",         Test::F, },
        { "false",          Test::F, },
        { "false\r\t",      Test::F, },
        { "null\r\t",       Test::N, },
        { "\r\nnull\r\t",   Test::N, },
        { 0,  Test::T, },
    };

    for (struct Test *test = tests; test->json; test++)
    {
        PrimitiveState state;
        PrimitiveHandler handler(state);
        Section sec(test->json);
        Parser p(& handler);
        bool ok = p.parse(& sec);
        EXPECT_TRUE(ok);
        switch (test->state)
        {
            case Test::T : { EXPECT_TRUE(state.t); break; }
            case Test::F : { EXPECT_TRUE(state.f); break; }
            case Test::N : { EXPECT_TRUE(state.n); break; }
            default : { ASSERT(0); }
        }
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
        { "unit-tests/data/missing-colon.json", Parser::COLON_EXPECTED },
        { "unit-tests/data/unterminated.json", Parser::UNTERMINATED_STRING },
        { "unit-tests/data/binary-data.json", Parser::KEY_EXPECTED },
        { 0, Parser::OKAY },
    };

    for (int i = 0; tests[i].path; i++)
    {
        const char *path = tests[i].path;
        //PO_DEBUG("-------- %s", path);

        int fd = open(path, O_RDONLY);
        ASSERT_ERROR(fd > 0, "error opening file '%s'", path);

        struct stat stat;
        int err = fstat(fd, & stat);
        ASSERT_ERROR(err >= 0, "err=%d", err);

        char *data = (char *) malloc(size_t(stat.st_size));
        ASSERT(data);

        ssize_t s = read(fd, data, size_t(stat.st_size));
        ASSERT_ERROR(s == stat.st_size, "s=%d", (int) s);

        close(fd);

        P handler;
        Section sec = { data, & data[stat.st_size-1] };
        Parser p(& handler);
        bool ok = p.parse(& sec);
        enum Parser::Error e = p.get_error(0);
        EXPECT_EQ(e, tests[i].err);
        EXPECT_EQ(ok, tests[i].err == Parser::OKAY);

        free(data);
    }
}

    /*
     *
     */

struct Matcher {
    const char *cmp;
    enum Match::Type type;
    int checked;
    bool same;
};

static void on_match(void *arg, Section *sec, enum Match::Type type, const char **keys)
{
    for (int i = 0; keys[i]; i++)
    {
        //PO_DEBUG("%s", keys[i]);
    }
    ASSERT(arg);

    struct Matcher *m = (struct Matcher *) arg;
    m->checked += 1;
    m->same = sec->match(m->cmp);
    m->type = type;
}

TEST(Json, Match)
{
    const char *json = "{\"sun\": {\"alt\": 0.9233672, \"az\": 3.902},"
            " \"moon\": {\"alt\": 0.1942122578, \"az\": 2.1646382808685303, \"phase\": 0.4673593289770038},"
            " \"mercury\": {\"alt\": 0.9328691363334656, \"az\": 3.231799840927124},"
            " \"mars\": {\"alt\": 0.8334254026412964, \"az\": 2.8750030994415283},"
            " \"venus\": {\"alt\": 0.834154486656189, \"az\": 3.184415102005005},"
            " \"jupiter\": {\"alt\": -0.0059446850791573524, \"az\": 5.141014099121094},"
            " \"saturn\": {\"alt\": -0.8723456859588623, \"az\": 6.078963279724121},"
            " \"time\": \"2023/07/25 14:07:57\", \"z\": \"local\"}";

    {
        json::Match tm;
        const char *m[] = { "moon", "phase", 0 };
        struct Matcher mm = { "0.4673593289770038", Match::NUMBER };
        json::Match::Item item = { m, on_match, (void*) & mm };
        tm.add_item(& item);
        Section sec;
        set_section(& sec, json);
        Parser p(& tm);
        p.parse(& sec);
        EXPECT_EQ(1, mm.checked);
        EXPECT_EQ(true, mm.same);
    }
    {
        json::Match tm;
        const char *m[] = { "jupiter", "az", 0 };
        struct Matcher mm = { "5.141014099121094", Match::NUMBER };
        json::Match::Item item = { m, on_match, (void*) & mm };
        tm.add_item(& item);
        Section sec;
        set_section(& sec, json);
        Parser p(& tm);
        p.parse(& sec);
        EXPECT_EQ(1, mm.checked);
        EXPECT_EQ(true, mm.same);
    }
    {
        json::Match tm;
        const char *m[] = { "jupiterx", "az", 0 };
        struct Matcher mm = { "5.141014099121094", Match::NUMBER };
        json::Match::Item item = { m, on_match, (void*) & mm };
        tm.add_item(& item);
        Section sec;
        set_section(& sec, json);
        Parser p(& tm);
        p.parse(& sec);
        EXPECT_EQ(0, mm.checked);
        EXPECT_EQ(false, mm.same);
    }
    {
        json::Match tm;

        const char *m0[] = { "moon", "phase", 0 };
        struct Matcher mm0 = { "0.4673593289770038", Match::NUMBER };
        json::Match::Item item0 = { m0, on_match, (void*) & mm0 };
        tm.add_item(& item0);

        const char *m1[] = { "jupiter", "az", 0 };
        struct Matcher mm1 = { "5.141014099121094", Match::NUMBER };
        json::Match::Item item1 = { m1, on_match, (void*) & mm1 };
        tm.add_item(& item1);

        Section sec;
        set_section(& sec, json);
        Parser p(& tm);
        p.parse(& sec);
        EXPECT_EQ(1, mm0.checked);
        EXPECT_EQ(true, mm0.same);
        EXPECT_EQ(1, mm1.checked);
        EXPECT_EQ(true, mm1.same);
    }
}

//  FIN
