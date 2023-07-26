
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
     *  Utility class to print a Section
     */

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
    P() { }

    virtual void on_object(bool push) override
    {
        IGNORE(push);
    }

    virtual void on_array(bool push) override
    {
        IGNORE(push);
    }

    virtual void on_number(Section *sec) override
    {
        IGNORE(sec);
    }

    virtual void on_string(Section *sec, bool key) override
    {
        IGNORE(sec);
        IGNORE(key);
    }

    virtual void on_primitive(Section *sec) override
    {
        IGNORE(sec);
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

    /*
     *
     */

class TestMatch : public Match
{
    const char *cmp;
public:

    bool result;
    int found;

    TestMatch(const char *_cmp)
    :   cmp(_cmp),
        result(false),
        found(0)
    {
    }
    void on_match(Section *sec, enum Type type)
    {
        IGNORE(type);
        //Printer p(sec);
        //PO_DEBUG("%s %s", p.get(), cmp);
        result = sec->match(cmp);
        found += 1;
    }
};

static void on_match(void *arg, Section *sec, enum Match::Type type)
{
    ASSERT(arg);
    TestMatch *tm = (TestMatch*) arg;
    tm->on_match(sec, type);
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
        TestMatch tm("xx");
        const char *m[] = { "sun", 0 };
        json::Match::Item item = { m, on_match, & tm };
        tm.add_item(& item);
        Section sec;
        set_section(& sec, json);
        Parser p(& tm);
        p.parse(& sec);
        EXPECT_FALSE(tm.result);
        EXPECT_EQ(2, tm.found); // finds all elements of "sun"
    }
    {
        TestMatch tm("0.4673593289770038");
        const char *m[] = { "moon", "phase", 0 };
        json::Match::Item item = { m, on_match, & tm };
        tm.add_item(& item);
        Section sec;
        set_section(& sec, json);
        Parser p(& tm);
        p.parse(& sec);
        EXPECT_TRUE(tm.result);
        EXPECT_EQ(1, tm.found);
    }
    {
        TestMatch tm("5.141014099121094");
        const char *m[] = { "jupiter", "az", 0 };
        json::Match::Item item = { m, on_match, & tm };
        tm.add_item(& item);
        Section sec;
        set_section(& sec, json);
        Parser p(& tm);
        p.parse(& sec);
        EXPECT_TRUE(tm.result);
        EXPECT_EQ(1, tm.found);
    }
    {
        TestMatch tm("5.141014099121094");
        const char *m[] = { "jupiterx", "az", 0 };
        json::Match::Item item = { m, on_match, & tm };
        tm.add_item(& item);
        Section sec;
        set_section(& sec, json);
        Parser p(& tm);
        p.parse(& sec);
        EXPECT_FALSE(tm.result);
        EXPECT_EQ(0, tm.found);
    }
}

//  FIN
