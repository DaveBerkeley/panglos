
#if !defined(__PANGLOS_ACTION__)
#define __PANGLOS_ACTION__

#include "panglos/list.h"

namespace panglos {

class Action
{
public:
    Action *next;

    virtual ~Action() { }
    virtual void execute() = 0;

    static Action **get_next(Action *);
};

class Mutex;

class Actions
{
public:
    List<Action*> actions;
    Mutex *mutex;

    Actions();
    ~Actions();

    bool add(Action *);
    bool remove(Action *);
};

}   //  namespace panglos

#endif  //  __PANGLOS_ACTION__

//  FIN
