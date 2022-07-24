
#include <string.h>

#include "panglos/debug.h"

#include "panglos/object.h"
#include "panglos/device.h"

namespace panglos {

Device::Device(const char *_name, const char **_needs, bool (*fn)(Device *, void*), void *_arg, uint16_t _flags)
:   needs(_needs),
    init(fn),
    arg(_arg),
    flags(_flags),
    next(0),
    name(_name)
{
}

    /*
     *  List utils
     */

int Device::match_name(Device *dev, void *arg)
{
    ASSERT(arg);
    const char *name = (const char *) arg;
    return strcmp(name, dev->name) == 0;
}

    /*
     *  Search the 'needs' list for a partial string match.
     *
     *  eg. find_has("scl") could return "scl_i2c1"
     */

const char *Device::find_has(const char *part)
{
    if (!needs)
    {
        return 0;
    }
 
    for (const char **needed = needs; *needed; needed += 1)
    {
        if (strstr(*needed, part))
        {
            return *needed;
        }
    }

    return 0;
}

    /*
     *
     */

bool Device::init_device(List<Device *> & done, List<Device *> & todo, bool verbose, int nest)
{
    if (verbose) PO_DEBUG("name=%s", name);

    if (!nest)
    {
        PO_ERROR("nest limit exceeded. Loop in device precursors?");
        return false;
    }

    // Initialise all the devices that this device needs
    if (needs)
    {
        for (const char **needed = needs; *needed; needed += 1)
        {
            if (verbose) PO_DEBUG("%s needs=%s", name, *needed);
        }

        for (const char **needed = needs; *needed; needed += 1)
        {
            Device *dev = done.find(Device::match_name, (void*) *needed, 0);

            if (dev)
            {
                if (verbose) PO_DEBUG("%s already initialised", *needed);
                continue;
            }

            dev = todo.find(Device::match_name, (void*) *needed, 0);

            if (!dev)
            {
                PO_ERROR("%s not found", *needed);
                return false;
            }

            if (!dev->init_device(done, todo, verbose, nest - 1))
            {
                return false;
            }
        }
    }

    // init this device
    ASSERT(init);
    if (!init(this, arg))
    {
        PO_ERROR("failed to init device %s", name);
        if (!(flags & F_CAN_FAIL))
        {
            return false;
        }
    }

    // Move to the done list
    bool ok = todo.remove(this, 0);
    ASSERT(ok);
    done.push(this, 0);

    return true;
}

bool Device::init_devices(List<Device *> & todo, bool verbose, int loops)
{
    List<Device *> done(Device::get_next);
    int loop_detect = 0;

    while (!todo.empty())
    {
        Device *dev = todo.head;

        if (!dev->init_device(done, todo, verbose, loops))
        {
            return false;
        }

        if (++loop_detect > loops)
        {
            PO_ERROR("loop not terminating");
            return false;
        }
    }

    // Restore the todo list
    todo.head = done.head;

    return true;
}

void Device::add(Objects *objects)
{
    ASSERT(objects);

    if (!(flags & F_DONT_REGISTER))
    {
        objects->add(name, this);
    }
}

}   //  namespace panglos

//  FIN
