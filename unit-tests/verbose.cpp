
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/verbose.h"

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
