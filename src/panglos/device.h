
#if !defined(__PANGLOS_DEVICE__)
#define __PANGLOS_DEVICE__

#include "panglos/list.h"

namespace panglos {

class Objects;

class Device
{
    bool init_device(List<Device *> & done, List<Device *> & todo, bool verbose, int nest);
    static int match_name(Device *dev, void *arg);

public:
    const char *name;
    const char **needs;
    bool (*init)(Device *dev, void *arg);
    void *arg;
    uint16_t flags;
    Device *next;
 
    typedef enum {
        F_NONE = 0,
        F_CAN_FAIL = 1 << 0,
        F_DONT_REGISTER = 1 << 1,
    }   Flags;

    Device(const char *name, const char **needs, bool (*fn)(Device *, void *), void *arg, uint16_t flags=0);
    Device();

    const char *find_has(const char *part);

    // List utils
    static Device **get_next(Device *d) { return & d->next; }

    static bool init_devices(List<Device *> & todo, bool verbose=false, int loops=100);
    void add(Objects* list, void *obj);
};

}   //  namespace panglos

#endif  //  __PANGLOS_DEVICE__

//  FIN
