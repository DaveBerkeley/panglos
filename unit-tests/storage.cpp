
#include "gtest/gtest.h"

#include "panglos/storage.h"

using namespace panglos;

TEST(Storage, String)
{
    Storage::clear_all();
    Storage db("test");

    bool ok;

    const char *hello = "hello";

    ok = db.set("hello", hello);
    EXPECT_TRUE(ok);
    EXPECT_EQ(Storage::VAL_STR, db.get_type("hello"));
    EXPECT_EQ("test", db.get_ns());

    char buff[64];
    size_t sz = sizeof(buff);

    ok = db.get("hello", buff, & sz);
    EXPECT_TRUE(ok);
    EXPECT_STREQ(buff, hello);
}

TEST(Storage, Int32)
{
    Storage::clear_all();
    Storage db("test");

    bool ok;

    ok = db.set("hello", 1234);
    EXPECT_TRUE(ok);
    EXPECT_EQ(Storage::VAL_INT32, db.get_type("hello"));
    EXPECT_EQ("test", db.get_ns());

    int32_t v = 0;

    ok = db.get("hello", & v);
    EXPECT_TRUE(ok);
    EXPECT_EQ(v, 1234);
}

TEST(Storage, Int16)
{
    Storage::clear_all();
    Storage db("test");

    bool ok;

    const int16_t i16 = 1234;
    ok = db.set("hello", i16);
    EXPECT_TRUE(ok);
    EXPECT_EQ(Storage::VAL_INT16, db.get_type("hello"));
    EXPECT_EQ("test", db.get_ns());

    int16_t v = 0;

    ok = db.get("hello", & v);
    EXPECT_TRUE(ok);
    EXPECT_EQ(v, 1234);
}

TEST(Storage, Int8)
{
    Storage::clear_all();
    Storage db("test");

    bool ok;

    const int8_t i8 = 34;
    ok = db.set("hello", i8);
    EXPECT_TRUE(ok);
    EXPECT_EQ(Storage::VAL_INT8, db.get_type("hello"));
    EXPECT_EQ("test", db.get_ns());

    int8_t v = 0;

    ok = db.get("hello", & v);
    EXPECT_TRUE(ok);
    EXPECT_EQ(v, 34);
}

TEST(Storage, Erase)
{
    Storage::clear_all();
    Storage db("test");

    bool ok;

    const int8_t i8 = 34;
    ok = db.set("hello", i8);
    EXPECT_TRUE(ok);

    int8_t v = 0;

    ok = db.get("hello", & v);
    EXPECT_TRUE(ok);
    EXPECT_EQ(v, 34);

    db.erase("hello");

    ok = db.get("hello", & v);
    EXPECT_FALSE(ok);
}

TEST(Storage, Mixed)
{
    Storage::clear_all();
    // same key in 2 different ns
    const char *key = "hello";
    bool ok;
    {
        Storage db("t1");
        const int8_t i8 = 34;
        ok = db.set(key, i8);
        EXPECT_TRUE(ok);
    }
    {
        Storage db("t2");
        const int8_t i8 = 12;
        ok = db.set(key, i8);
        EXPECT_TRUE(ok);
    }

    {
        Storage db("t1");
        int8_t v = 0;

        ok = db.get(key, & v);
        EXPECT_TRUE(ok);
        EXPECT_EQ(v, 34);
    }
    {
        Storage db("t2");
        int8_t v = 0;

        ok = db.get(key, & v);
        EXPECT_TRUE(ok);
        EXPECT_EQ(v, 12);
    }
}

TEST(Storage, List)
{
    Storage::clear_all();
    {
        Storage db("test");
        bool ok;

        const int8_t i8 = 34;
        ok = db.set("i8", i8);
        EXPECT_TRUE(ok);

        const int16_t i16 = 1234;
        ok = db.set("i16", i16);
        EXPECT_TRUE(ok);

        const int32_t i32 = 12345678;
        ok = db.set("i32", i32);
        EXPECT_TRUE(ok);
    }
    {
        Storage db("other");
        bool ok;

        ok = db.set("str", "string");
        EXPECT_TRUE(ok);
    }

    Storage::List list;

    char ns[24];
    char key[24];
    Storage::Type type;

    struct Check
    {
        const char *ns;
        const char *key;
        Storage::Type type;
        bool found;
    };

    struct Check check[] = {
        {   "test", "i8", Storage::VAL_INT8, },
        {   "test", "i16", Storage::VAL_INT16, },
        {   "test", "i32", Storage::VAL_INT32, },
        {   "other", "str", Storage::VAL_STR, },
        { 0 },
    };

    int count = 0;

    while (list.get(ns, key, & type, sizeof(ns)))
    {
        count += 1;
        for (struct Check *ck = check; ck->ns; ck++)
        {
            if (strcmp(ns, ck->ns)) continue;
            if (strcmp(key, ck->key)) continue;
            if (type != ck->type) continue;
            // no duplicates
            EXPECT_FALSE(ck->found);
            ck->found = true;
        }
    }

    // check that we found them all
    for (struct Check *ck = check; ck->ns; ck++)
    {
        EXPECT_TRUE(ck->found);
        count -= 1;
    }

    // All accounted for
    EXPECT_EQ(0, count);
}

TEST(Storage, Blob)
{
    Storage::clear_all();
    Storage db("test");

    bool ok;

    struct X {
        struct A { int i; double f ; };
        struct A a;
        struct A b;
        int x;
    };

    struct X x = { { 0 } };
    x.a.f = 1234.56;
    x.b.f = 1.01;
    x.b.i = 1234;

    ok = db.set_blob("hello", & x, sizeof(x));
    EXPECT_TRUE(ok);
    EXPECT_EQ(Storage::VAL_BLOB, db.get_type("hello"));
    EXPECT_EQ("test", db.get_ns());

    struct X y;
    size_t sz = sizeof(y);

    ok = db.get_blob("hello", & y, & sz);
    EXPECT_TRUE(ok);
    EXPECT_EQ(sz, sizeof(y));
    EXPECT_EQ(x.a.i, y.a.i);
    EXPECT_EQ(x.a.f, y.a.f);
    EXPECT_EQ(x.b.i, y.b.i);
    EXPECT_EQ(x.b.f, y.b.f);
}

//  FIN
