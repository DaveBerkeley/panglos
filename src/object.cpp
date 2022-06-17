
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

    List<struct Object*> objects;

public:
    Objects_()
    :   objects(next_fn)
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
     *  Factory Method
     */

Objects *Objects::create()
{
    return new Objects_;
}

}   //  namespace panglos

//  FIN
