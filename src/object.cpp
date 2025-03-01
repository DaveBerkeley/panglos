
#include <string.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/list.h"

#include "panglos/object.h"

namespace panglos {

    /*
     *  List of objects
     */

struct Object
{
    struct Object *next;
    const char *name;
    void *obj;
};

struct Object ** next_fn(struct Object *obj)
{
    return & obj->next;
}

static int match_name(struct Object *item, void *arg)
{
    ASSERT(arg);
    ASSERT(item);
    const char *name = (const char *) arg;
    return strcmp(name, item->name) == 0;
}

    /*
     *
     */

class Objects_ : public Objects 
{
    Mutex *mutex;
    bool verbose;

public:
    List<struct Object*> objects;

    Objects_(bool _verbose)
    :   mutex(0),
        verbose(_verbose),
        objects(next_fn)
    {
        mutex = Mutex::create();
    }

    ~Objects_()
    {
        // delete all the objects in the list
        while (true)
        {
            struct Object *item = objects.pop(mutex);
            if (!item)
            {
                break;
            }
            delete item;
        }
        
        delete mutex;
    }

    virtual void add(const char *name, void *obj) override
    {
        if (verbose) PO_DEBUG("name=%s obj=%p", name, obj);
        struct Object *item = new struct Object;
        item->next = 0;
        item->name = name;
        item->obj = obj;

        objects.push(item, mutex);
    }

    virtual void *get(const char *name) override
    {
        struct Object *item = objects.find(match_name, (void*) name, mutex);
        return item ? item->obj : 0;
    }

    virtual bool remove(const char *name) override
    {
        if (verbose) PO_DEBUG("name=%s", name);
        Lock lock(mutex);

        struct Object *item = objects.find(match_name, (void*) name, 0);
        if (!item)
        {
            return false;
        }
        const bool ok = objects.remove(item, 0);
        delete item;
        return ok;
    }
};

    /*
     *
     */

struct VisitArg
{
    Objects::visitor fn;
    void *arg;
};

static int obj_visitor(Object *obj, void *arg)
{
    ASSERT(arg);
    struct VisitArg *va = (struct VisitArg*) arg;
    va->fn(obj->name, obj->obj, va->arg);
    return 0;
}

void Objects::visit(Objects *objects, visitor fn, void *arg)
{
    ASSERT(objects);
    Objects_ *objs = (Objects_ *) objects;
    ASSERT(fn);

    struct VisitArg va = { .fn = fn, .arg = arg, };
    objs->objects.visit(obj_visitor, & va, 0);
}

    /*
     *  Factory Method
     */

Objects *Objects::create(bool verbose)
{
    return new Objects_(verbose);
}

Objects *Objects::objects = 0;

}   //  namespace panglos

//  FIN
