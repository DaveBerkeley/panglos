
#include <stdlib.h>
#include <string.h>

#include "esp_netif.h"
#include "esp_wifi.h"
#include "mdns.h"

#include "panglos/debug.h"
#include "panglos/esp32/hal.h"

#include "panglos/object.h"
#include "panglos/storage.h"
#include "panglos/semaphore.h"
#include "panglos/network.h"

    /*
     *
     */

namespace panglos {

void show_err(esp_err_t err, const char *fn, int line)
{
    if (err != ESP_OK)
    {
        PO_ERROR("err=%s %s +%d", lut(err_lut, err), fn, line);
    }
}

#define SHOW_ERR(err) show_err(err, __FUNCTION__, __LINE__)

#define TAG "wifix: sta"

static LUT event_id_lut[] = {
    { "IP_EVENT_STA_GOT_IP", IP_EVENT_STA_GOT_IP, },
    { "IP_EVENT_STA_LOST_IP", IP_EVENT_STA_LOST_IP, },
    { "IP_EVENT_AP_STAIPASSIGNED", IP_EVENT_AP_STAIPASSIGNED, },
    { "IP_EVENT_GOT_IP6", IP_EVENT_GOT_IP6, },
    { "IP_EVENT_ETH_GOT_IP", IP_EVENT_ETH_GOT_IP, },
    { "IP_EVENT_ETH_LOST_IP", IP_EVENT_ETH_LOST_IP, },
    { "IP_EVENT_PPP_GOT_IP", IP_EVENT_PPP_GOT_IP, },
    { "IP_EVENT_PPP_LOST_IP", IP_EVENT_PPP_LOST_IP, },
    { 0, 0 },
};

    /*
     *
     */

class Waiter : public Connection
{
    Semaphore *sem;
    Interface *iface;

    virtual void on_connect(Interface *i) override
    {
        PO_DEBUG("");
        iface = i;
        sem->post();
    }
    virtual void on_disconnect(Interface *i) override
    {
        PO_DEBUG("");
        iface = i;
        sem->post();
    }
public:
    Waiter()
    :   sem(0)
    {
        sem = Semaphore::create();
    }
    ~Waiter()
    {
        delete sem;
    }
    bool wait()
    {
        PO_DEBUG("");
        sem->wait();
        const bool connected = iface ? iface->is_connected() : false;
        PO_DEBUG("%s", connected ? "connected" : "not connected");
        return connected;
    }
};

    /*
     *
     */

class Esp_WiFiInterface : public WiFiInterface
{
    State state;

    virtual bool is_connected(State *s) override
    {
        if (s)
        {
            *s = state;
        }
        return state.ip.v4.sin_family != 0;
    }

    void connect(const char *ssid, const char *pw)
    {
        wifi_config_t config;
        memset(& config, 0, sizeof(config));

        strncpy((char*) config.sta.ssid, ssid, sizeof(config.sta.ssid));
        strncpy((char*) config.sta.password, pw, sizeof(config.sta.password));
        //config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        config.sta.scan_method = WIFI_FAST_SCAN;
        config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;

        esp_err_t err;
        PO_DEBUG("Connecting to %s ...", ssid);
        err = esp_wifi_set_config(WIFI_IF_STA, & config);
        SHOW_ERR(err);
        err = esp_wifi_start();
        SHOW_ERR(err);

        // Start the connection process in the background
        err = esp_wifi_connect();
        SHOW_ERR(err);
    }

    static int do_connect(AccessPoint *ap, void *arg)
    {
        ASSERT(arg);
        Esp_WiFiInterface *iface = (Esp_WiFiInterface*) arg;

        Waiter waiter;
        iface->add_connection(& waiter);
        iface->connect(ap->ssid, ap->pw);
        const bool ok = waiter.wait();
        iface->del_connection(& waiter);
        return ok;
    }

    virtual void connect() override
    {
        PO_DEBUG("");
        access_points.visit(do_connect, this, ap_mutex);
    }

    virtual void disconnect() override
    {
        PO_DEBUG("");
    }

public:
    Esp_WiFiInterface()
    :   WiFiInterface("wifi")
    {
        PO_DEBUG("");
        esp_err_t err;

        //  Network stuff : create event handler task
        err = esp_netif_init();
        if (err == ESP_OK)
        {
            err = esp_event_loop_create_default();
        }
        SHOW_ERR(err);

        wifi_init_config_t wconfig = WIFI_INIT_CONFIG_DEFAULT();
        err = esp_wifi_init(& wconfig);
        SHOW_ERR(err);

        esp_netif_inherent_config_t config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
        config.if_desc = TAG;
        config.route_prio = 128;
        esp_netif_create_wifi(WIFI_IF_STA, & config);

        esp_wifi_set_default_wifi_sta_handlers();
        register_handlers();

        err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
        SHOW_ERR(err);

        err = esp_wifi_set_mode(WIFI_MODE_STA);
        SHOW_ERR(err);
        
    }


    void _on_disconnect(esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        // ESP-IDF sys_evt thread callback
        PO_DEBUG("id=%s", lut(event_id_lut, event_id));
        on_disconnect();
        state.ip.v4.sin_family = 0;
    }

    static bool is_our_netif(const char *prefix, esp_netif_t *netif)
    {
        //PO_DEBUG("got='%s' expect='%s'", esp_netif_get_desc(netif), prefix);
        return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
    }

    static void get_ip(IpAddr *ipaddr, esp_ip4_addr_t *esp_ip)
    {
        // convert ESP-IDF data into posix format
        ipaddr->v4.sin_family = AF_INET;
        ipaddr->v4.sin_port = 0;
        memcpy(& ipaddr->v4.sin_addr, esp_ip, sizeof(ipaddr->v4.sin_addr));
    } 

    void _on_connect(esp_event_base_t event_base, int32_t event_id, ip_event_got_ip_t *event)
    {
        // ESP-IDF sys_evt thread callback
        ASSERT(event_id == IP_EVENT_STA_GOT_IP);

        if (!is_our_netif(TAG, event->esp_netif))
        {
            PO_DEBUG("Got IPv4 from another interface \"%s\": ignored", esp_netif_get_desc(event->esp_netif));
            return;
        }
        PO_DEBUG("got %s", esp_netif_get_desc(event->esp_netif));
        PO_DEBUG("ip=" IPSTR " mask=" IPSTR " gw=" IPSTR, 
                IP2STR(& event->ip_info.ip),
                IP2STR(& event->ip_info.netmask),
                IP2STR(& event->ip_info.gw));

        // fill out the Connection data
        get_ip(& state.ip, & event->ip_info.ip);
        get_ip(& state.gw, & event->ip_info.gw);
        get_ip(& state.mask, & event->ip_info.netmask);
 
        on_connect();
    }

    static void _on_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        ASSERT(arg);
        Esp_WiFiInterface *iface = (Esp_WiFiInterface*) arg;
        iface->_on_disconnect(event_base, event_id, event_data);
    }

    static void _on_connect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        ASSERT(arg);
        Esp_WiFiInterface *iface = (Esp_WiFiInterface*) arg;
        iface->_on_connect(event_base, event_id, (ip_event_got_ip_t *) event_data);
    }

    void register_handlers()
    {
        esp_err_t err;
        err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, & _on_disconnect, this);
        SHOW_ERR(err);
        err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, & _on_connect, this);
        SHOW_ERR(err);
    }

    void unregister_handlers()
    {
        esp_err_t err;
        err = esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, & _on_disconnect);
        SHOW_ERR(err);
        err = esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, & _on_connect);
        SHOW_ERR(err);
    }

};

WiFiInterface *WiFiInterface::create()
{
    return new Esp_WiFiInterface;
}

    /*
     *  MDNS service
     */

void Network::start_mdns(const char *name)
{
    PO_DEBUG("%s", name);

    esp_err_t err = mdns_init();
    if (err != ESP_OK)
    {
        PO_ERROR("");
        return;
    }

    mdns_hostname_set(name);
}

}   //  namespace panglos


//   FIN
