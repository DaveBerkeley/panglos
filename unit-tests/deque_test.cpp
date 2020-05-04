
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
    Deque deque;

    deque_init(& deque);
    EXPECT_EQ(0, deque.head);
    EXPECT_EQ(0, deque.tail);

    // empty deque
    EXPECT_EQ(0, deque_size(& deque, next_fn, 0));

    Item item1;

    // item should appear at head and tail of deque
    deque_push_head(& deque, (pList) & item1, next_fn, 0);
    EXPECT_EQ(1, deque_size(& deque, next_fn, 0));
    EXPECT_EQ((pList) & item1, deque.head);
    EXPECT_EQ((pList) & item1, deque.tail);

    Item item2;

    // item2 at head, item1 at tail
    deque_push_head(& deque, (pList) & item2, next_fn, 0);
    EXPECT_EQ(2, deque_size(& deque, next_fn, 0));
    EXPECT_EQ((pList) & item2, deque.head);
    EXPECT_EQ((pList) & item1, deque.tail);

    Item item3;

    // item3 at head, item1 at tail
    deque_push_head(& deque, (pList) & item3, next_fn, 0);
    EXPECT_EQ(3, deque_size(& deque, next_fn, 0));
    EXPECT_EQ((pList) & item3, deque.head);
    EXPECT_EQ((pList) & item1, deque.tail);

    Item item4;

    // item3 at head, item4 at tail
    deque_push_tail(& deque, (pList) & item4, next_fn, 0);
    EXPECT_EQ(4, deque_size(& deque, next_fn, 0));
    EXPECT_EQ((pList) & item3, deque.head);
    EXPECT_EQ((pList) & item4, deque.tail);

    // the list is item3, item2, item1, item4
    pList p;

    // leaves : item2, item1, item4
    p = deque_pop_head(& deque, next_fn, 0);
    EXPECT_EQ(& item3, (Item*) p);
    EXPECT_EQ(3, deque_size(& deque, next_fn, 0));

    // leaves : item1, item4
    p = deque_pop_head(& deque, next_fn, 0);
    EXPECT_EQ(& item2, (Item*) p);
    EXPECT_EQ(2, deque_size(& deque, next_fn, 0));

    // leaves : item4
    p = deque_pop_head(& deque, next_fn, 0);
    EXPECT_EQ(& item1, (Item*) p);
    EXPECT_EQ(1, deque_size(& deque, next_fn, 0));

    // leaves : (empty)
    p = deque_pop_head(& deque, next_fn, 0);
    EXPECT_EQ(& item4, (Item*) p);
    EXPECT_EQ(0, deque_size(& deque, next_fn, 0));

    // leaves : (empty)
    p = deque_pop_head(& deque, next_fn, 0);
    EXPECT_EQ(0, (Item*) p);

    // leaves : item1
    deque_push_tail(& deque, (pList) & item1, next_fn, 0);
    EXPECT_EQ(1, deque_size(& deque, next_fn, 0));
    EXPECT_EQ((pList) & item1, deque.head);
    EXPECT_EQ((pList) & item1, deque.tail);
 
    // leaves : (empty)
    p = deque_pop_head(& deque, next_fn, 0);
    EXPECT_EQ(& item1, (Item*) p);
    EXPECT_EQ(0, deque_size(& deque, next_fn, 0));
}

//  FIN
