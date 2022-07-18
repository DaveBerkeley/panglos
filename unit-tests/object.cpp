
#include <gtest/gtest.h>

#include <panglos/debug.h>

#include <panglos/object.h>

#include "mock.h"

using namespace panglos;

TEST(Objects, Add)
{
    Objects *g = Objects::create();
    ASSERT(g);

    g->add("hello", (void*) "hello");
    g->add("world", (void*) "world");

    void *v;

    v = g->get("hello");
    EXPECT_STREQ("hello", (const char*) v);
    v = g->get("world");
    EXPECT_STREQ("world", (const char*) v);
    v = g->get("nothing");
    EXPECT_FALSE(v);

    delete g;
}

TEST(Objects, Remove)
{
    Objects *g = Objects::create();
    ASSERT(g);

    g->add("hello", (void*) "xhello");
    g->add("mid", (void*) "xmid");
    g->add("world", (void*) "xworld");

    void *v;

    v = g->get("mid");
    EXPECT_STREQ("xmid", (const char*) v);
    
    bool ok;
    
    ok = g->remove("mid");
    EXPECT_TRUE(ok);

    v = g->get("mid");
    EXPECT_FALSE(v);

    v = g->get("hello");
    EXPECT_STREQ("xhello", (const char*) v);
    v = g->get("world");
    EXPECT_STREQ("xworld", (const char*) v);
    v = g->get("nothing");
    EXPECT_FALSE(v);

    delete g;
}

TEST(Objects, Leak)
{
    Objects *g = Objects::create();
    ASSERT(g);

    g->add("hello", (void*) "hello");
    g->add("world", (void*) "world");

    delete g;
}

    /*
     *
     */

static void test_visit(const char *name, void *obj, void *arg)
{
    ASSERT(name);
    ASSERT(obj);
    ASSERT(arg);

    int num = 0;
    int n = sscanf(name, "obj_%d", & num);
    EXPECT_EQ(1, n);
    const char *s = (const char *) obj;
    EXPECT_EQ(s, name);

    bool *found = (bool*) arg;
    found[num] = true;
}

TEST(Objects, Visit)
{
    Objects *g = Objects::create();
    ASSERT(g);

    const int num = 10;
    char name[num][16];
    bool found[num];

    for (int i = 0; i < num; i++)
    {
        found[i] = false;
        snprintf(name[i], sizeof(name[i]), "obj_%d", i);
        g->add(name[i], (void*) name[i]);
    }

    Objects::visit(g, test_visit, found);

    for (int i = 0; i < num; i++)
    {
        EXPECT_TRUE(found[i]);
    }

    delete g;
}

//  FIN
