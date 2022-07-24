
#include <stdlib.h>
#include <limits.h>

#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/object.h"
#include "panglos/list.h"

#include "panglos/device.h"

    /*
     *
     */

using namespace panglos;

struct DevInit {
    Objects *objects;
    bool verbose;
};

static bool hardware_init(const char *name, Device *dev, struct DevInit *di)
{
    // simulate hardware initialisation
    ASSERT(di);
    ASSERT(di->objects);

    if (di->verbose) PO_DEBUG("%s %s", name, dev->name);

    dev->add(di->objects, dev);

    return true;
}

static struct DevInit * get_di(void *arg)
{
    ASSERT(arg);
    return (struct DevInit *) arg;
}

class Anything;

static void check_has(Device *dev, Objects *objects, const char **names)
{
    ASSERT(objects);
    ASSERT(names);

    for (const char **n = names; *n; n++)
    {
        //PO_DEBUG("dev=%s n=%s", dev->name, *n);
        const char *s = dev->find_has(*n);
        EXPECT_TRUE(s);
        Anything *thing = (Anything*) objects->get(s);
        EXPECT_TRUE(thing);
    }
}

    /*
     *
     */

static bool init_gpio(Device *dev, void *arg)
{
    struct DevInit *di = get_di(arg);
    return hardware_init("gpio", dev, di);
}

static bool init_i2c(Device *dev, void *arg)
{
    struct DevInit *di = get_di(arg);
    Objects *objects = di->objects;

    const char *ensure[] = { "scl", "sda", 0 };
    check_has(dev, objects, ensure);

    return hardware_init("i2c", dev, di);
}

static bool init_mcp23s17(Device *dev, void *arg)
{
    struct DevInit *di = get_di(arg);
    Objects *objects = di->objects;

    const char *ensure[] = { "i2c", 0 };
    check_has(dev, objects, ensure);

    return hardware_init("mcp23s17", dev, di);
}

static bool init_keyboard(Device *dev, void *arg)
{
    struct DevInit *di = get_di(arg);
    Objects *objects = di->objects;

    const char *ensure[] = { "mcp23s17", "reset", "irq", 0 };
    check_has(dev, objects, ensure);

    return hardware_init("keyboard", dev, di);
}

    /*
     *
     */

static void add_devices(List<Device*> &devices, struct DevInit *di)
{
    static const char *needs_i2c[] = { "sda0", "scl0", 0 };
    static const char *needs_mcp23s17[] = { "i2c0", 0 };
    static const char *needs_keyboard[] = { "mcp23s17", "key_reset", "key_irq", 0 };

    static Device sda("sda0", 0, init_gpio, di);
    static Device scl("scl0", 0, init_gpio, di);
    static Device i2c("i2c0", needs_i2c, init_i2c, di);
    static Device mcp23s17("mcp23s17", needs_mcp23s17, init_mcp23s17, di);
    static Device keyboard("keyboard", needs_keyboard, init_keyboard, di);
    static Device reset("key_reset", 0, init_gpio, di);
    static Device irq("key_irq", 0, init_gpio, di);

    Mutex *m = 0;
    devices.push(& sda, m);
    devices.push(& keyboard, m);
    devices.push(& scl, m);
    devices.push(& mcp23s17, m);
    devices.push(& i2c, m);
    devices.push(& reset, m);
    devices.push(& irq, m);
}

    /*
     *
     */

TEST(Device, FindHas)
{
    const char *needs[] = { "sda0", "scl0", "xx_reset", "xmiddley", 0 };
    Device i2c("i2c0", needs, 0, 0);

    const char *s;

    s = i2c.find_has("sda");
    EXPECT_STREQ(s, "sda0");
    s = i2c.find_has("sdx");
    EXPECT_FALSE(s);
    s = i2c.find_has("scl");
    EXPECT_STREQ(s, "scl0");
    s = i2c.find_has("reset");
    EXPECT_STREQ(s, "xx_reset");
    s = i2c.find_has("middle");
    EXPECT_STREQ(s, "xmiddley");
}

    /*
     *
     */

static int list_randomise(Device *a, Device *b)
{
    IGNORE(a);
    IGNORE(b);
    int i = rand();
    i = (i > (INT_MAX/2)) ? 1 : -1;
    return i;
}

static void list_shuffle(List<Device*> & list, Mutex *m=0)
{
    const int size = list.size(0);

    for (int i = 0; i < size; i++)
    {
        Device *dev = list.pop(0);
        ASSERT(dev);
        list.add_sorted(dev, list_randomise, m);
    }
}

TEST(Device, Test)
{
    bool verbose = false;

    for (int i = 0; i < 3; i++)
    {
        if (verbose) PO_DEBUG("");
        List<Device*> devices(Device::get_next);

        Objects *objects = Objects::create();

        struct DevInit di = {
            .objects = objects,
            .verbose = verbose,
        };

        add_devices(devices, & di);

        list_shuffle(devices);

        bool ok = Device::init_devices(devices, verbose);
        EXPECT_TRUE(ok);

        // Check all the objects have been initialised
        EXPECT_TRUE(objects->get("scl0"));
        EXPECT_TRUE(objects->get("sda0"));
        EXPECT_TRUE(objects->get("i2c0"));
        EXPECT_TRUE(objects->get("keyboard"));
        EXPECT_TRUE(objects->get("mcp23s17"));

        delete objects;
    }
}

TEST(Device, Loop)
{
    bool verbose = false;
    List<Device*> devices(Device::get_next);

    Objects *objects = Objects::create();

    struct DevInit di = {
        .objects = objects,
        .verbose = verbose,
    };

    static const char *needs_a[] = { "b", 0 };
    static const char *needs_b[] = { "a", 0 };

    static Device a("a", needs_a, init_gpio, & di);
    static Device b("b", needs_b, init_gpio, & di);

    Mutex *m = 0;
    devices.push(& a, m);
    devices.push(& b, m);

    bool ok = Device::init_devices(devices, verbose);
    EXPECT_FALSE(ok);

    delete objects;
}

TEST(Device, NoSuchDevice)
{
    bool verbose = false;
    List<Device*> devices(Device::get_next);

    Objects *objects = Objects::create();

    struct DevInit di = {
        .objects = objects,
        .verbose = verbose,
    };

    static const char *needs_a[] = { "b", 0 };

    static Device a("a", needs_a, init_gpio, & di);

    Mutex *m = 0;
    devices.push(& a, m);

    bool ok = Device::init_devices(devices, verbose, 5);
    EXPECT_FALSE(ok);

    delete objects;
}

TEST(Device, DontRegister)
{
    bool verbose = false;
    List<Device*> devices(Device::get_next);

    Objects *objects = Objects::create();

    struct DevInit di = {
        .objects = objects,
        .verbose = verbose,
    };

    static Device a("a", 0, init_gpio, & di, Device::F_DONT_REGISTER);

    Mutex *m = 0;
    devices.push(& a, m);

    bool ok = Device::init_devices(devices, verbose);
    EXPECT_TRUE(ok);

    ok = objects->get("a");
    EXPECT_FALSE(ok);
    
    delete objects;
}

static bool init_fail(Device *dev, void *arg)
{
    IGNORE(arg);
    IGNORE(dev);
    return false;
}

TEST(Device, CanFail)
{
    bool verbose = false;
    List<Device*> devices(Device::get_next);

    Objects *objects = Objects::create();

    static Device a("a", 0, init_fail, 0, Device::F_CAN_FAIL);

    Mutex *m = 0;
    devices.push(& a, m);

    bool ok = Device::init_devices(devices, verbose);
    EXPECT_TRUE(ok);

    ok = objects->get("a");
    EXPECT_FALSE(ok);
    
    delete objects;
}

//  FIN
