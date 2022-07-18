
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/action.h"

using namespace panglos;

class TestAction : public Action
{
public:
    int count;

    TestAction()
    :   count(0)
    {
    }

    virtual void execute() override
    {
        PO_DEBUG("%d", count++);
    }
};

TEST(Action, Add)
{
    Actions actions;

    for (int i = 0; i < 100; i++)
    {
        Action *action = new TestAction;
        actions.add(action);
    }

    while (actions.actions.head)
    {
        Action *action = actions.actions.head; 
        bool ok = actions.remove(action);
        ASSERT(ok);
        delete action;
    }
}

TEST(Action, Execute)
{
    Actions actions;

    for (int i = 0; i < 100; i++)
    {
        Action *action = new TestAction;
        actions.add(action);
    }

    while (actions.actions.head)
    {
        TestAction *action = (TestAction*) actions.actions.head; 
        bool ok = actions.remove(action);
        ASSERT(ok);
        delete action;
    }
}

//  FIN
