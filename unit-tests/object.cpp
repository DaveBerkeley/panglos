
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

//  FIN
