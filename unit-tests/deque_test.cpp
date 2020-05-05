
#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/deque.h>

using namespace panglos;

typedef struct Item {
    struct Item *next;
}   Item;

static pList *next_fn(pList i)
{
    Item *item = (Item*) i;
    return (pList*) & item->next;
}

TEST(Deque, Test)
{
    Deque deque(next_fn);

    EXPECT_EQ(0, deque.size(0));
    EXPECT_TRUE(deque.empty());

    Item item1;

    // item should appear at head and tail of deque
    deque.push_head((pList) & item1, 0);
    EXPECT_EQ(1, deque.size(0));
    EXPECT_EQ((pList) & item1, deque.head);
    EXPECT_EQ((pList) & item1, deque.tail);
    EXPECT_FALSE(deque.empty());

    Item item2;

    // item2 at head, item1 at tail
    deque.push_head((pList) & item2, 0);
    EXPECT_EQ(2, deque.size(0));
    EXPECT_EQ((pList) & item2, deque.head);
    EXPECT_EQ((pList) & item1, deque.tail);
    EXPECT_FALSE(deque.empty());

    Item item3;

    // item3 at head, item1 at tail
    deque.push_head((pList) & item3, 0);
    EXPECT_EQ(3, deque.size(0));
    EXPECT_EQ((pList) & item3, deque.head);
    EXPECT_EQ((pList) & item1, deque.tail);
    EXPECT_FALSE(deque.empty());

    Item item4;

    // item3 at head, item4 at tail
    deque.push_tail((pList) & item4, 0);
    EXPECT_EQ(4, deque.size(0));
    EXPECT_EQ((pList) & item3, deque.head);
    EXPECT_EQ((pList) & item4, deque.tail);
    EXPECT_FALSE(deque.empty());

    // the list is item3, item2, item1, item4
    pList p;

    // leaves : item2, item1, item4
    p = deque.pop_head(0);
    EXPECT_EQ(& item3, (Item*) p);
    EXPECT_EQ(3, deque.size(0));
    EXPECT_FALSE(deque.empty());

    // leaves : item1, item4
    p = deque.pop_head(0);
    EXPECT_EQ(& item2, (Item*) p);
    EXPECT_EQ(2, deque.size(0));
    EXPECT_FALSE(deque.empty());

    // leaves : item4
    p = deque.pop_head(0);
    EXPECT_EQ(& item1, (Item*) p);
    EXPECT_EQ(1, deque.size(0));
    EXPECT_FALSE(deque.empty());

    // leaves : (empty)
    p = deque.pop_head(0);
    EXPECT_EQ(& item4, (Item*) p);
    EXPECT_EQ(0, deque.size(0));
    EXPECT_TRUE(deque.empty());

    // leaves : (empty)
    p = deque.pop_head(0);
    EXPECT_EQ(0, (Item*) p);
    EXPECT_TRUE(deque.empty());

    // leaves : item1
    deque.push_tail((pList) & item1, 0);
    EXPECT_EQ(1, deque.size(0));
    EXPECT_EQ((pList) & item1, deque.head);
    EXPECT_EQ((pList) & item1, deque.tail);
    EXPECT_FALSE(deque.empty());
 
    // leaves : (empty)
    p = deque.pop_head(0);
    EXPECT_EQ(& item1, (Item*) p);
    EXPECT_EQ(0, deque.size(0));
    EXPECT_TRUE(deque.empty());
}

//  FIN
