
#if !defined(__PANGLOS_DEVICE__)
#define __PANGLOS_DEVICE__

#include "panglos/list.h"

namespace panglos {

class Device
{
    bool init_device(List<Device *> & done, List<Device *> & todo, bool verbose);
    static int match_name(Device *dev, void *arg);
public:
    const char *name;
    const char **needs;
    bool (*init)(Device *dev, void *arg);
    void *arg;
    Device *next;

    Device(const char *name, const char **needs, bool (*fn)(Device *, void *), void *arg);

    const char *find_has(const char *part);

    // List utils
    static Device **get_next(Device *d);

    static bool init_devices(List<Device *> & todo, bool verbose=false, int loops=1000);
};

}   //  namespace panglos

#endif  //  __PANGLOS_DEVICE__

//  FIN
