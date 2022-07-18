
#if !defined(__PANGLOS_DEVICE__)
#define __PANGLOS_DEVICE__

#include "panglos/list.h"

namespace panglos {

class Device
{
    bool init_device(List<Device *> & done, List<Device *> & todo, bool verbose, int nest);
    static int match_name(Device *dev, void *arg);

    const char **needs;
    bool (*init)(Device *dev, void *arg);
    void *arg;
    Device *next;
public:
    const char *name;

    Device(const char *name, const char **needs, bool (*fn)(Device *, void *), void *arg);

    const char *find_has(const char *part);

    // List utils
    static Device **get_next(Device *d) { return & d->next; }

    static bool init_devices(List<Device *> & todo, bool verbose=false, int loops=100);
};

}   //  namespace panglos

#endif  //  __PANGLOS_DEVICE__

//  FIN
