
#include "gtest/gtest.h"

#include "panglos/storage.h"

using namespace panglos;

TEST(Storage, String)
{
    Storage db("test");

    bool ok;

    const char *hello = "hello";

    ok = db.set("hello", hello);
    EXPECT_TRUE(ok);

    char buff[64];
    size_t sz = sizeof(buff);

    ok = db.get("hello", buff, & sz);
    EXPECT_TRUE(ok);
    EXPECT_STREQ(buff, hello);
}

TEST(Storage, Int32)
{
    Storage db("test");

    bool ok;

    ok = db.set("hello", 1234);
    EXPECT_TRUE(ok);

    int32_t v = 0;

    ok = db.get("hello", & v);
    EXPECT_TRUE(ok);
    EXPECT_EQ(v, 1234);
}

TEST(Storage, Int16)
{
    Storage db("test");

    bool ok;

    const int16_t i16 = 1234;
    ok = db.set("hello", i16);
    EXPECT_TRUE(ok);

    int16_t v = 0;

    ok = db.get("hello", & v);
    EXPECT_TRUE(ok);
    EXPECT_EQ(v, 1234);
}

TEST(Storage, Int8)
{
    Storage db("test");

    bool ok;

    const int8_t i8 = 34;
    ok = db.set("hello", i8);
    EXPECT_TRUE(ok);

    int8_t v = 0;

    ok = db.get("hello", & v);
    EXPECT_TRUE(ok);
    EXPECT_EQ(v, 34);
}

TEST(Storage, Erase)
{
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

//  FIN
