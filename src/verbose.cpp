
#include <stdlib.h>

#include "panglos/debug.h"
#include "panglos/storage.h"

#include "panglos/verbose.h"

namespace panglos {

struct Verbose *Verbose::verboses;

void verbose_init()
{
    Storage db("verbose");

    for (struct Verbose *v = Verbose::verboses; v; v = v->next)
    {
        int32_t value = v->verbose;
        if (db.get(v->name, & value))
        {
            PO_DEBUG("setting %s %d", v->name, value);
            v->verbose = value;
        }
    }
}

}   //  namespace panglos

//  FIN
