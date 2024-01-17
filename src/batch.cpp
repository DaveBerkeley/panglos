
#include <stdlib.h>

#include "panglos/debug.h"
#include "panglos/object.h"
#include "panglos/thread.h"
#include "panglos/semaphore.h"

#include "panglos/batch.h"

namespace panglos {

    /*
     *
     */

class Queue::Message { };

class BatchEvent : public panglos::Queue::Message
{
public:
    BatchTask::Job *job;
};

    /*
     *
     */

BatchTask::BatchTask(Thread *t)
:   thread(t),
    queue(0)
{
    queue = Queue::create(sizeof(BatchEvent), 10, 0);
    ASSERT(queue);
}

BatchTask::~BatchTask()
{
    execute(0);
    thread->join();
    delete thread;
    delete queue;
    Objects::objects->remove("batch_task");
}

void BatchTask::run()
{
    PO_DEBUG("");

    while (true)
    {
        BatchEvent event;

        if (!queue->get(& event, 100))
        {
            continue;
        }

        if (!event.job)
        {
            break;
        }

        //PO_DEBUG("running event %p", event.job);
        event.job->run();
    }
}

void BatchTask::execute(Job *job)
{
    BatchEvent event;
    event.job = job;
    queue->put(& event);
}

    /*
     *
     */

void BatchTask::run(void *arg)
{
    PO_DEBUG("");

    ASSERT(arg);
    BatchTask *task = (BatchTask *) arg;
    task->run();
}

BatchTask *BatchTask::start()
{
    BatchTask *task = (BatchTask*) Objects::objects->get("batch_task");

    if (task)
    {
        PO_INFO("Already running");
        return task;
    }
 
    Thread *thread = Thread::create("batch");
    task = new BatchTask(thread);
    Objects::objects->add("batch_task", task);
    thread->start(BatchTask::run, task);
    return task;
}

    /*
     *
     */

BatchTask::WaitJob::WaitJob()
:   semaphore(0)
{
    semaphore = Semaphore::create();
}

BatchTask::WaitJob::~WaitJob()
{
    delete semaphore;
}

void BatchTask::WaitJob::run()
{
    semaphore->post();
}

void BatchTask::WaitJob:: wait()
{
    semaphore->wait();
}

}   //namespace [panglos

//  FIN
