
#include <stddef.h>

#include "panglos/thread.h"

namespace panglos {

    /*
     *  ThreadPool
     */

ThreadPool::ThreadPool(const char *name, int n, size_t stack, Thread::Priority p)
:   threads(0),
    count(n)
{
    threads = new Thread* [n];

    for (int i = 0; i < count; i++)
    {
        threads[i] = Thread::create(name, stack, p);
    }
}

ThreadPool::~ThreadPool()
{
    for (int i = 0; i < count; i++)
    {
        delete threads[i];
    }
    delete[] threads;
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
