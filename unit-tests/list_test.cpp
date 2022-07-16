
#include <unistd.h>
#include <stdlib.h>

#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/thread.h>
#include <panglos/mutex.h>
#include <panglos/list.h>

using namespace panglos;

typedef struct Item
{
    struct Item *next;
    int value;
    bool visited;
}   Item;

static Item ** item_next(Item *item)
{
    return & item->next;
}

static int cmp(Item* a, Item* b)
{
    return b->value - a->value;
}

static int visit_clr(Item* a, void *arg)
{
    ASSERT(!arg);
    return a->visited = 0;
}

static int visit_set(Item* a, void *arg)
{
    ASSERT(arg);
    int *i = (int*) arg;
    if (a->value == *i)
    {
        a->visited = true;
    }
    return 0;
}

static int visit_match(Item* a, void *arg)
{
    ASSERT(arg);
    int *i = (int*) arg;
    return a->value == *i;
}

    /*
     *
     */

TEST(List, AddRemove)
{
    List<Item*> list(item_next);

    EXPECT_EQ(0, list.size(0));
    EXPECT_TRUE(list.empty());

    // i1
    Item i1;
    EXPECT_FALSE(list.has(& i1, 0));
    list.push(& i1, 0);
    EXPECT_EQ(1, list.size(0));
    EXPECT_FALSE(list.empty());
    EXPECT_TRUE(list.has(& i1, 0));

    // i2, i1
    Item i2;
    EXPECT_FALSE(list.has(& i2, 0));
    list.push(& i2, 0);
    EXPECT_EQ(2, list.size(0));
    EXPECT_FALSE(list.empty());
    EXPECT_TRUE(list.has(& i2, 0));

    // i3, i2, i1
    Item i3;
    EXPECT_FALSE(list.has(& i3, 0));
    list.push(& i3, 0);
    EXPECT_EQ(3, list.size(0));
    EXPECT_FALSE(list.empty());
    EXPECT_TRUE(list.has(& i3, 0));

    // i3, i2, i1, i4
    Item i4 = { & i3, 0, 0 }; // check bad data in next doesn't cause harm
    EXPECT_FALSE(list.has(& i4, 0));
    list.append(& i4, 0);
    EXPECT_EQ(4, list.size(0));
    EXPECT_FALSE(list.empty());
    EXPECT_TRUE(list.has(& i4, 0));

    EXPECT_TRUE(list.has(& i1, 0));
    EXPECT_TRUE(list.has(& i2, 0));
    EXPECT_TRUE(list.has(& i3, 0));
    EXPECT_TRUE(list.has(& i4, 0));

    Item *item;
    bool okay;

    // i2, i1, i4
    item = list.pop(0);
    EXPECT_EQ(& i3, item);
    EXPECT_EQ(3, list.size(0));
    EXPECT_FALSE(list.empty());

    EXPECT_TRUE(list.has(& i1, 0));
    EXPECT_TRUE(list.has(& i2, 0));
    EXPECT_FALSE(list.has(& i3, 0));
    EXPECT_TRUE(list.has(& i4, 0));

    // i1, i4
    item = list.pop(0);
    EXPECT_EQ(& i2, item);
    EXPECT_EQ(2, list.size(0));
    EXPECT_FALSE(list.empty());

    EXPECT_TRUE(list.has(& i1, 0));
    EXPECT_FALSE(list.has(& i2, 0));
    EXPECT_FALSE(list.has(& i3, 0));
    EXPECT_TRUE(list.has(& i4, 0));

    // i1
    okay = list.remove(& i4, 0);
    EXPECT_TRUE(okay);
    EXPECT_EQ(1, list.size(0));
    EXPECT_FALSE(list.empty());

    EXPECT_TRUE(list.has(& i1, 0));
    EXPECT_FALSE(list.has(& i2, 0));
    EXPECT_FALSE(list.has(& i3, 0));
    EXPECT_FALSE(list.has(& i4, 0));

    // empty
    okay = list.remove(& i1, 0);
    EXPECT_TRUE(okay);
    EXPECT_EQ(0, list.size(0));
    EXPECT_TRUE(list.empty());

    EXPECT_FALSE(list.has(& i1, 0));
    EXPECT_FALSE(list.has(& i2, 0));
    EXPECT_FALSE(list.has(& i3, 0));
    EXPECT_FALSE(list.has(& i4, 0));

    Item i5;
    
    // i1, i2, i3, i4, i5
    list.push(& i5, 0);
    list.push(& i4, 0);
    list.push(& i3, 0);
    list.push(& i2, 0);
    list.push(& i1, 0);
    EXPECT_EQ(5, list.size(0));
    EXPECT_FALSE(list.empty());

    // remove head
    // i2, i3, i4, i5
    okay = list.remove(& i1, 0);
    EXPECT_TRUE(okay);
    EXPECT_EQ(4, list.size(0));
    EXPECT_FALSE(list.empty());

    // remove tail
    // i2, i3, i4
    okay = list.remove(& i5, 0);
    EXPECT_TRUE(okay);
    EXPECT_EQ(3, list.size(0));
    EXPECT_FALSE(list.empty());

    // remove mid
    // i2, i4
    okay = list.remove(& i3, 0);
    EXPECT_TRUE(okay);
    EXPECT_EQ(2, list.size(0));
    EXPECT_FALSE(list.empty());

    // try to remove wrong item
    // i2, i4
    okay = list.remove(& i3, 0);
    EXPECT_FALSE(okay);
    EXPECT_EQ(2, list.size(0));
    EXPECT_FALSE(list.empty());
}

    /*
     *
     */

TEST(List, Sorted)
{
    List<Item*> list(item_next);

    EXPECT_EQ(0, list.size(0));
    EXPECT_TRUE(list.empty());

    Item i1, i2, i3, i4, i5;
    i1.value = 100;
    i2.value = 200;
    i3.value = 300;
    i4.value = 400;
    i5.value = 500;

    // i1, i2, i3, i4, i5
    list.add_sorted(& i3, cmp, 0);
    list.add_sorted(& i2, cmp, 0);
    list.add_sorted(& i1, cmp, 0);
    list.add_sorted(& i5, cmp, 0);
    list.add_sorted(& i4, cmp, 0);
    EXPECT_EQ(5, list.size(0));

    Item *item;
    bool okay;

    // i2, i3, i4, i5
    item = list.pop(0);
    EXPECT_EQ(& i1, item);
    EXPECT_EQ(4, list.size(0));

    // i3, i4, i5
    item = list.pop(0);
    EXPECT_EQ(& i2, item);
    EXPECT_EQ(3, list.size(0));

    // i3, i5
    okay = list.remove(& i4, 0);
    EXPECT_TRUE(okay);
    EXPECT_EQ(2, list.size(0));

    // i5
    item = list.pop(0);
    EXPECT_EQ(& i3, item);
    EXPECT_EQ(1, list.size(0));

    // (empty)
    item = list.pop(0);
    EXPECT_EQ(& i5, item);
    EXPECT_EQ(0, list.size(0));
    
    item = list.pop(0);
    EXPECT_EQ(0, item);
    EXPECT_EQ(0, list.size(0));
}

    /*
     *
     */

TEST(List, VisitFind)
{
    List<Item*> list(item_next);

    EXPECT_EQ(0, list.size(0));
    EXPECT_TRUE(list.empty());

    Item i1, i2, i3, i4, i5;
    i1.value = 100;
    i2.value = 200;
    i3.value = 300;
    i4.value = 400;
    i5.value = 500;

    // i1, i2, i3, i4, i5
    list.add_sorted(& i3, cmp, 0);
    list.add_sorted(& i2, cmp, 0);
    list.add_sorted(& i1, cmp, 0);
    list.add_sorted(& i5, cmp, 0);
    list.add_sorted(& i4, cmp, 0);
    EXPECT_EQ(5, list.size(0));

    // clear the 'visited' flag for all 
    list.visit(visit_clr, 0, 0);
    EXPECT_EQ(0, i1.visited);
    EXPECT_EQ(0, i2.visited);
    EXPECT_EQ(0, i3.visited);
    EXPECT_EQ(0, i4.visited);
    EXPECT_EQ(0, i5.visited);

    // set the 'visited' flag for matching item
    int match = 300;
    list.visit(visit_set, & match, 0);
    EXPECT_EQ(0, i1.visited);
    EXPECT_EQ(0, i2.visited);
    EXPECT_EQ(1, i3.visited);
    EXPECT_EQ(0, i4.visited);
    EXPECT_EQ(0, i5.visited);

    Item *item;

    item = list.find(visit_match, & match, 0);
    EXPECT_EQ(& i3, item);

    match = 400;
    item = list.find(visit_match, & match, 0);
    EXPECT_EQ(& i4, item);

    match = 123;
    item = list.find(visit_match, & match, 0);
    EXPECT_EQ(0, item);

    list.remove(& i4, 0);
    match = 400;
    item = list.find(visit_match, & match, 0);
    EXPECT_EQ(0, item);
}

    /*
     *
     */

typedef struct {
    Thread *thread;
    List<Item*> *list;
    Mutex *mutex;
    int n;
}   PushInfo;

static void push_thread(void *arg)
{
    ASSERT(arg);
    PushInfo *pi = (PushInfo*) arg;

    for (int i = 0; i < pi->n; i++)
    {
        Item *item = new Item;
        item->value = i;
        item->visited = false;
        pi->list->add_sorted(item, cmp, pi->mutex);
    }
}

static void remove_thread(void *arg)
{
    ASSERT(arg);
    PushInfo *pi = (PushInfo*) arg;
    List<Item*> *list = pi->list;
    ASSERT(list);

    for (int i = 0; i < pi->n; i++)
    {
        while (true)
        {
            Item *item;

            {
                Lock lock(pi->mutex);
                item = list->find(visit_match, & i, 0);
                if (item)
                {
                    bool okay = list->remove(item, 0);
                    EXPECT_TRUE(okay);
                    delete item;
                    break;
                }
            }
            
            if (!item)
            {
                // not in the list yet?
                usleep(1000);
            }
        }
    }
}

    /*
     *
     */

TEST(List, Thread)
{
    List<Item*> list(item_next);
    Mutex *mutex = Mutex::create();

#if defined(ARCH_LINUX)
    const int num = 50;
#else
    const int num = 5;
#endif
    const int adds = 100;
    PushInfo push[num];
    PushInfo pop[num];

    for (int i = 0; i < num; i++)
    {
        PushInfo *pi = & push[i];
        pi->n = adds;
        pi->mutex = mutex;
        pi->list = & list;
        pi->thread = Thread::create("push");
        pi->thread->start(push_thread, pi);
    }

    for (int i = 0; i < num; i++)
    {
        PushInfo *pi = & pop[i];
        pi->n = adds;
        pi->mutex = mutex;
        pi->list = & list;
        pi->thread = Thread::create("pop");
        pi->thread->start(remove_thread, pi);
    }

    for (int i = 0; i < num; i++)
    {
        panglos::Thread *thread;

        thread = push[i].thread;
        thread->join();
        delete thread;

        thread = pop[i].thread;
        thread->join();
        delete thread;
    }

    delete mutex;
}

//  FIN
