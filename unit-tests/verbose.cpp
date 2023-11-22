
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/verbose.h"

    /*
     *  Simulate Storage API
     */

#include "panglos/storage.h"

namespace panglos {

Storage::Storage(const char *ns, bool verbose)
{
    EXPECT_STREQ(ns, "verbose");
    IGNORE(verbose);
}

Storage::~Storage()
{
}

bool Storage::get(char const *key, int32_t *v)
{
    if (!strcmp(key, "one"))
    {
        *v = !*v;
        return true;
    }
    if (!strcmp(key, "two"))
    {
        *v = !*v;
        return true;
    }
    return false;
}

};

    /*
     *
     */

using namespace panglos;

static VERBOSE(test, "test", true);
static VERBOSE(one, "one", true);
static VERBOSE(two, "two", false);

TEST(Verbose, Test)
{
    for (struct Verbose *v = Verbose::verboses; v; v = v->next)
    {
        PO_DEBUG("%s %d", v->name, v->verbose);
    }

    verbose_init();

    for (struct Verbose *v = Verbose::verboses; v; v = v->next)
    {
        PO_DEBUG("%s %d", v->name, v->verbose);
    }
}

//  FIN
