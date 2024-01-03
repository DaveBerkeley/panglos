
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/wifi.h"

using namespace panglos;

class TestWiFi : public WiFiInterface
{
    struct AP {
        const char *ssid;
        const char *pw;
        struct AP *next;
        static struct AP **get_next(struct AP *s) { return & s->next; }
        static int match(struct AP *ap, void *arg)
        {
            ASSERT(arg);
            const char *s = (const char *) arg;
            return strcmp(ap->ssid, s) == 0;
        }
    };

    panglos::List<struct AP*> aps;

    virtual void connect(Notify *s, const char *ssid, const char *pw) override
    {
        PO_DEBUG("%s %s", ssid, pw);
        struct AP *ap = aps.find(AP::match, (void*) ssid, 0);
        if (!ap)
        {
            PO_DEBUG("ap not found");
            if (s) s->on_disconnect(0);
            return;
        }
        const bool match = strcmp(ap->pw, pw) == 0;
        if (match)
        {
            PO_DEBUG("pw match");
            Connection con;
            memset(& con, 0, sizeof(con));
            con.ip.v4.sin_family = AF_INET;
            if (s) s->on_connect(& con);
        }
        else
        {
            PO_DEBUG("no pw match");
            if (s) s->on_disconnect(0);
        }
    }

    virtual void disconnect(Notify *) override
    {
        ASSERT(0);
    }
public:
    TestWiFi() : aps(AP::get_next) {}

    ~TestWiFi()
    {
        while (!aps.empty())
        {
            struct AP *ap = aps.pop(0);
            delete ap;
        }
    }

    void add(const char *ssid, const char *pw)
    {
        struct AP *ap= new struct AP;
        ap->ssid = ssid;
        ap->pw = pw;
        ap->next = 0;
        aps.push(ap, 0);
    }
};

    /*
     *
     */

TEST(WiFi, Test)
{
    TestWiFi interface;
    interface.add("other", "xx");
    interface.add("more", "xx");
    interface.add("hello1", "xx");
    interface.add("end", "xx");

    WiFi wifi(& interface);

    wifi.add_ap("hello", "world");
    wifi.add_ap("hello1", "world1");

    bool ok;
    ok = wifi.connect();
    EXPECT_FALSE(ok);

    interface.add("hello1", "world1");
    ok = wifi.connect();
    EXPECT_TRUE(ok);
}

//  FIN
