
#include "panglos/debug.h"

#include "panglos/mutex.h"

#include "panglos/action.h"

namespace panglos {

Actions::Actions()
:   actions(Action::get_next),
    mutex(0)
{
    mutex = Mutex::create();
}

Actions::~Actions()
{
    delete mutex;
}

bool Actions::add(Action *action)
{
    actions.push(action, mutex);
    return true;
}

bool Actions::remove(Action *action)
{
    return actions.remove(action, mutex);
}

}   //  namespace panglos
