
#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "panglos/debug.h"
#include "panglos/thread.h"

namespace panglos {

    /*
     *
     */

struct ByName
{
    const char *name;
    Thread *thread;
};

static int visit_by_name(Thread *thread, void *arg)
{
    ASSERT(arg);
    struct ByName *bn = (struct ByName *) arg;
    if (!strcmp(bn->name, thread->get_name()))
    {
        bn->thread = thread;
    }
    return !!bn->thread;
}

Thread *Thread::get_by_name(const char *name)
{
    struct ByName arg = {
        .name = name,
        .thread = 0
    };
    Thread::visit(visit_by_name, & arg);
    return arg.thread;
}

    /*
     *  ThreadPool
     */

ThreadPool::ThreadPool(const char *name, int n, size_t stack, Thread::Priority p)
:   threads(0),
    count(n),
    names(0)
{
    threads = new Thread* [size_t(count)];

    if (strstr(name, "%d"))
    {
        size_t len = strlen(name) + 8;
        names = new char* [size_t(count)];
        for (int i = 0; i < count; i++)
        {
            names[i] = new char [len];
            snprintf(names[i], len, name, i);
        }
    }

    for (int i = 0; i < count; i++)
    {
        const char *label = names ? names[i] : name;
        threads[i] = Thread::create(label, stack, p);
    }
}

ThreadPool::~ThreadPool()
{
    for (int i = 0; i < count; i++)
    {
        delete threads[i];
    }
    delete[] threads;

    if (names)
    {
        for (int i = 0; i < count; i++)
        {
            delete[] names[i];
        }
        delete[] names;
    }        
}

void ThreadPool::start(void (*fn)(void *), void *arg)
{
    for (int i = 0; i < count; i++)
    {
        threads[i]->start(fn, arg);
    }
}

void ThreadPool::join()
{
    for (int i = 0; i < count; i++)
    {
        threads[i]->join();
    }
}

}   //  namespace panglos

//  FIN
