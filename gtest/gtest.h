
#if !defined(__PANGLOS_GTEST__)
#define __PANGLOS_GTEST__

#include <string.h>

#include "panglos/debug.h"

#if defined(__cplusplus)
extern "C" {
#endif

    /*
     *
     */

struct TestDef {
    void (*fn)();
    const char *group;
    const char *name;
    struct TestDef *next;
};

extern struct TestDef *test_head;

#define TEST(group,name) \
        void fn_ ## group ## name(); \
        struct TestDef def_ ## group ## name = { \
            fn_ ## group ## name, \
            #group, \
            #name, \
            0, \
        }; \
        __attribute__((constructor)) static void add_ ## group ## name() \
        {  \
            def_ ## group ## name .next = test_head, \
            test_head = & def_ ## group ## name; \
        } \
        void fn_ ## group ## name()

    /*
     *
     */

void test_list();
void test_fail(const char *file, int line, const char *err);
void test_run();

    /*
     *
     */

#define EXPECT_TRUE(a)      if (!(a)) { test_fail(__FILE__, __LINE__, #a); }
#define EXPECT_FALSE(a)     if ((a)) { test_fail(__FILE__, __LINE__, #a); }

#define EXPECT_EQ(a,b) if ((a) != (b)) { test_fail(__FILE__, __LINE__, #a "!=" #b); }

#define EXPECT_STREQ(a, b) if (strcmp((a), (b))) { test_fail(__FILE__, __LINE__, #a "!=" #b); }

#if defined(__cplusplus)
}
#endif

#endif  //  __PANGLOS_GTEST__

//  FIN
