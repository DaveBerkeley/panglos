
#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/deque.h>

using namespace panglos;

typedef struct Item {
    struct Item *next;
    int value;
}   Item;

static Item **next_fn(Item* item)
{
    return & item->next;
}

TEST(Deque, Test)
{
    Deque<Item*> deque(next_fn);

    EXPECT_EQ(0, deque.size(0));
    EXPECT_TRUE(deque.empty());

    Item item1;

    bool okay;

    okay = deque.find(& item1, 0);
    EXPECT_FALSE(okay);

    // item should appear at head and tail of deque
    deque.push_head(& item1, 0);
    EXPECT_EQ(1, deque.size(0));
    EXPECT_EQ(& item1, deque.head);
    EXPECT_EQ(& item1, deque.tail);
    EXPECT_FALSE(deque.empty());

    okay = deque.find(& item1, 0);
    EXPECT_TRUE(okay);

    Item item2;

    // item2 at head, item1 at tail
    deque.push_head(& item2, 0);
    EXPECT_EQ(2, deque.size(0));
    EXPECT_EQ(& item2, deque.head);
    EXPECT_EQ(& item1, deque.tail);
    EXPECT_FALSE(deque.empty());

    Item item3;

    okay = deque.find(& item1, 0);
    EXPECT_TRUE(okay);
    okay = deque.find(& item2, 0);
    EXPECT_TRUE(okay);
    okay = deque.find(& item3, 0);
    EXPECT_FALSE(okay);

    // item3 at head, item1 at tail
    deque.push_head(& item3, 0);
    EXPECT_EQ(3, deque.size(0));
    EXPECT_EQ(& item3, deque.head);
    EXPECT_EQ(& item1, deque.tail);
    EXPECT_FALSE(deque.empty());

    Item item4;

    okay = deque.find(& item1, 0);
    EXPECT_TRUE(okay);
    okay = deque.find(& item2, 0);
    EXPECT_TRUE(okay);
    okay = deque.find(& item3, 0);
    EXPECT_TRUE(okay);
    okay = deque.find(& item4, 0);
    EXPECT_FALSE(okay);

    // item3 at head, item4 at tail
    deque.push_tail(& item4, 0);
    EXPECT_EQ(4, deque.size(0));
    EXPECT_EQ(& item3, deque.head);
    EXPECT_EQ(& item4, deque.tail);
    EXPECT_FALSE(deque.empty());

    okay = deque.find(& item1, 0);
    EXPECT_TRUE(okay);
    okay = deque.find(& item2, 0);
    EXPECT_TRUE(okay);
    okay = deque.find(& item3, 0);
    EXPECT_TRUE(okay);
    okay = deque.find(& item4, 0);
    EXPECT_TRUE(okay);

    // the list is item3, item2, item1, item4
    Item* p;

    // leaves : item2, item1, item4
    p = deque.pop_head(0);
    EXPECT_EQ(& item3, (Item*) p);
    EXPECT_EQ(3, deque.size(0));
    EXPECT_FALSE(deque.empty());

    okay = deque.find(& item1, 0);
    EXPECT_TRUE(okay);
    okay = deque.find(& item2, 0);
    EXPECT_TRUE(okay);
    okay = deque.find(& item3, 0);
    EXPECT_FALSE(okay);
    okay = deque.find(& item4, 0);
    EXPECT_TRUE(okay);

    // leaves : item1, item4
    p = deque.pop_head(0);
    EXPECT_EQ(& item2, (Item*) p);
    EXPECT_EQ(2, deque.size(0));
    EXPECT_FALSE(deque.empty());

    okay = deque.find(& item1, 0);
    EXPECT_TRUE(okay);
    okay = deque.find(& item2, 0);
    EXPECT_FALSE(okay);
    okay = deque.find(& item3, 0);
    EXPECT_FALSE(okay);
    okay = deque.find(& item4, 0);
    EXPECT_TRUE(okay);

    // leaves : item4
    p = deque.pop_head(0);
    EXPECT_EQ(& item1, (Item*) p);
    EXPECT_EQ(1, deque.size(0));
    EXPECT_FALSE(deque.empty());

    okay = deque.find(& item1, 0);
    EXPECT_FALSE(okay);
    okay = deque.find(& item2, 0);
    EXPECT_FALSE(okay);
    okay = deque.find(& item3, 0);
    EXPECT_FALSE(okay);
    okay = deque.find(& item4, 0);
    EXPECT_TRUE(okay);

    // leaves : (empty)
    p = deque.pop_head(0);
    EXPECT_EQ(& item4, (Item*) p);
    EXPECT_EQ(0, deque.size(0));
    EXPECT_TRUE(deque.empty());

    okay = deque.find(& item1, 0);
    EXPECT_FALSE(okay);
    okay = deque.find(& item2, 0);
    EXPECT_FALSE(okay);
    okay = deque.find(& item3, 0);
    EXPECT_FALSE(okay);
    okay = deque.find(& item4, 0);
    EXPECT_FALSE(okay);

    // leaves : (empty)
    p = deque.pop_head(0);
    EXPECT_EQ(0, (Item*) p);
    EXPECT_TRUE(deque.empty());

    // leaves : item1
    deque.push_tail(& item1, 0);
    EXPECT_EQ(1, deque.size(0));
    EXPECT_EQ(& item1, deque.head);
    EXPECT_EQ(& item1, deque.tail);
    EXPECT_FALSE(deque.empty());
 
    // leaves : (empty)
    p = deque.pop_head(0);
    EXPECT_EQ(& item1, (Item*) p);
    EXPECT_EQ(0, deque.size(0));
    EXPECT_TRUE(deque.empty());
}

    /*
     *
     */

static int visit_set(Item *item, void *arg)
{
    ASSERT(arg);
    int *i = (int*) arg;

    item->value = *i;
    *i += 1;
    return 0;
}

static int visit_mid(Item *item, void *arg)
{
    ASSERT(arg);
    int *i = (int*) arg;

    if (item->value < *i)
    {
        item->value = -1;
        return 0;
    }

    return 1;
}

TEST(Deque, Visit)
{
    Deque<Item*> deque(next_fn);

    Item item1;
    Item item2;
    Item item3;
    Item item4;
    Item item5;

    deque.push_head(& item1, 0);
    deque.push_head(& item2, 0);
    deque.push_head(& item3, 0);
    deque.push_head(& item4, 0);
    deque.push_head(& item5, 0);

    int i = 0;
    deque.visit(visit_set, & i, 0);
    EXPECT_EQ(4, item1.value);
    EXPECT_EQ(3, item2.value);
    EXPECT_EQ(2, item3.value);
    EXPECT_EQ(1, item4.value);
    EXPECT_EQ(0, item5.value);

    i = 2;
    deque.visit(visit_mid, & i, 0);
    EXPECT_EQ(-1, item5.value);
    EXPECT_EQ(-1, item4.value);
    EXPECT_EQ(2, item3.value);
    EXPECT_EQ(3, item2.value);
    EXPECT_EQ(4, item1.value);
}

//  FIN
