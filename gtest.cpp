
#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/logger.h"
#include "panglos/time.h"

static Test *tests = 0;

Test::Test(const char *_group, const char *_name, void (*_fn)())
:   group(_group),
    name(_name),
    fn(_fn),
    next(0)
{
    // Link this Test into the global list
    next = tests;
    tests = this;
}

struct Idle
{
    void (*fn)(void*);
    void *arg;
};

void Test::visit(const char *group, const char *name, visitor fn, void *arg)
{
    struct Idle *idle = (struct Idle *) arg;
    if (idle) ASSERT(idle->fn);

    ASSERT(fn);
    for (Test *test = tests; test; test = test->next)
    {
        if (group && strcmp(group, test->group))
        {
            // group set but no match
            continue;
        }
        if (name && strcmp(name, test->name))
        {
            // name set but no match
            continue;
        }
        fn(test, 0);

        if (idle) idle->fn(idle->arg);
    }
}

void v_run(Test *test, void *arg)
{
    UNUSED(arg);
    PO_DEBUG("%s", test->name);
 
    static const char * disabledPrefix = "DISABLED_";

    if (strncmp(disabledPrefix, test->name, strlen(disabledPrefix)) == 0)
    {
        PO_INFO("test %s.%s DISABLED", test->group, test->name);
    }
    else
    {
        PO_INFO("run test %s.%s", test->group, test->name);
        test->fn();
        PO_INFO("run test %s.%s OKAY", test->group, test->name);
    }
}

void Test::run(const char *group, const char *name, void (*fn)(void*), void *arg)
{
    PO_INFO("Running tests ...");
    struct Idle idle = { .fn = fn, .arg = arg };
    visit(group, name, v_run, fn ? & idle : 0);
    PO_INFO("Tests completed");
}

    /*
     *
     */

extern "C" void test_run(const char *group, const char *name)
{
    Test::run(group, name);
}

    /*
     *
     */

struct v_list_t {
    void *arg;
    test_visitor fn;
};

void v_list(Test *test, void *arg)
{
    ASSERT(test);
    ASSERT(arg);
    struct v_list_t *info = (struct v_list_t *) arg;

    info->fn(test->group, test->name, info->arg);
}

extern "C" void test_list(const char *group, const char *name, test_visitor fn, void *arg)
{
    ASSERT(fn);
    struct v_list_t info = { .arg = arg, .fn = fn };
    Test::visit(group, name, v_list, & info);
}

//  FIN
