
#pragma once

#include "panglos/queue.h"

    /*
     *
     */

namespace panglos {

class Thread;
class Semaphore;

class BatchTask
{
    panglos::Thread *thread;
public:
    class Job
    {
    public:
        virtual void run() = 0;
    };

    class WaitJob : public Job
    {
        Semaphore *semaphore;
        virtual void run() override;
    public:
        WaitJob();
        ~WaitJob();
        void wait();
    };

private:
    panglos::Queue *queue;

    void run();

public:
    BatchTask(panglos::Thread *t);
    ~BatchTask();

    static void run(void *);

    void execute(Job *job);

    static BatchTask *start();
};

    /*
     *
     */

}   //  namespace panglos

//  FIN
