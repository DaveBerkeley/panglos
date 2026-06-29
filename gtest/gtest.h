
#if defined(__cplusplus)

    /*
     *
     */

#include <string.h>

#include "panglos/debug.h"
#include "panglos/logger.h"

class Test
{
    const char *group;
    const char *name;
    void (*fn)();
    Test *next;

    typedef void (*visitor)(Test *, void *arg);
 
    friend void v_run(Test *test,  void *arg);
    friend void v_list(Test *test,  void *arg);
 
public:
    Test(const char *_group, const char *_name, void (*_fn)());

    static void run(const char *group, const char *name, void (*idle)(void *)=0, void *arg=0);
    static void visit(const char *group, const char *name, visitor fn, void *arg);
    static void add(Test *test);
};

#define __STRINGISE(x)  #x
#define STRINGISE(x)    __STRINGISE(x)

#define EXPECT_EQ(a, b)     ASSERT_PRINT((a) == (b), "Failed EXPECT_EQ %s, %s", STRINGISE(a), STRINGISE(b))
#define EXPECT_NE(a, b)     ASSERT_PRINT((a) != (b), "Failed EXPECT_NE %s, %s", STRINGISE(a), STRINGISE(b))
#define EXPECT_GT(a, b)     ASSERT_PRINT((a) > (b),  "Failed EXPECT_GT %s, %s", STRINGISE(a), STRINGISE(b))
#define EXPECT_GE(a, b)     ASSERT_PRINT((a) >= (b), "Failed EXPECT_GE %s, %s", STRINGISE(a), STRINGISE(b))
#define EXPECT_LT(a, b)     ASSERT_PRINT((a) < (b),  "Failed EXPECT_LT %s, %s", STRINGISE(a), STRINGISE(b))
#define EXPECT_LE(a, b)     ASSERT_PRINT((a) <= (b), "Failed EXPECT_LE %s, %s", STRINGISE(a), STRINGISE(b))
#define EXPECT_TRUE(a)      ASSERT_PRINT((a),        "Failed EXPECT_TRUE %s",   STRINGISE(a))
#define EXPECT_FALSE(a)     ASSERT_PRINT(!(a),       "Failed EXPECT_FALSE %s",  STRINGISE(a))
#define EXPECT_STREQ(a, b)  ASSERT_PRINT(!strcmp((a), (b)), "Failed EXPECT_STREQ %s, %s", STRINGISE(a), STRINGISE(b))

#define TEST(group,fn) \
    extern void test_ ##group ##fn(); \
    static Test group ## fn ##_xx(STRINGISE(group), STRINGISE(fn), test_ ##group ##fn); \
    void test_ ##group ##fn()

#endif  //  __cplusplus

#if defined(__cplusplus)
extern "C" {
#endif

typedef void (*test_visitor)(const char *group, const char *name, void *arg);

void test_list(const char *group, const char *name, test_visitor fn, void *arg);
void test_run(const char *group, const char *name);

#if defined(__cplusplus)
}
#endif

//  FIN
