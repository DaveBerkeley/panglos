
#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "panglos/thread.h"

namespace panglos {

    /*
     *  ThreadPool
     */

ThreadPool::ThreadPool(const char *name, int n, size_t stack, Thread::Priority p)
:   threads(0),
    count(n),
    names(0)
{
    threads = new Thread* [count];

    if (strstr(name, "%d"))
    {
        size_t len = strlen(name) + 8;
        names = new char* [count];
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
