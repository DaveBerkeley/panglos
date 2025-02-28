
#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/mutex.h"

#include "panglos/app/event.h"

using namespace panglos;

TEST(Event, Unlink)
{
    EventHandler::add_handler(Event::KEY, 0, 0);
    EventHandler::add_handler(Event::KEY, 0, 0);
    EventHandler::add_handler(Event::KEY, 0, 0);
    EventHandler::unlink_handlers(Event::KEY);
    // Valgrind should report no leaks
}

static bool _handler(void *arg, Event *event, Event::Queue *q)
{
    EXPECT_TRUE(arg);
    EXPECT_TRUE(event);
    EXPECT_EQ(event->type, Event::KEY);
    EXPECT_FALSE(q);
    bool *b = (bool*) arg;
    return *b;
}

TEST(Event, Handle)
{
    bool doit = true;
    bool ok;

    EventHandler::add_handler(Event::KEY, _handler, & doit);

    Event event = { .type = Event::DATE_TIME };

    ok = EventHandler::handle_event(0, & event);
    EXPECT_FALSE(ok);
    
    event.type = Event::KEY;
    ok = EventHandler::handle_event(0, & event);
    EXPECT_TRUE(ok);

    doit = false;
    ok = EventHandler::handle_event(0, & event);
    EXPECT_FALSE(ok);

    EventHandler::unlink_handlers(Event::KEY);
}

//  FIN
