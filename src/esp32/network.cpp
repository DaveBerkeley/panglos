
#include <stdlib.h>
#include <string.h>

#include "esp_netif.h"
#include "esp_wifi.h"

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
    {  "IP_EVENT_STA_GOT_IP", IP_EVENT_STA_GOT_IP, },
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

class Esp_WiFiInterface : public WiFiInterface
{
    IpAddr ip_addr;

    virtual bool is_connected(IpAddr *ip) override
    {
        PO_DEBUG("TODO");
        if (ip)
        {
            *ip = ip_addr;
        }
        return true;
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

    virtual void connect() override
    {
        PO_DEBUG("");

        AccessPoint *ap = access_points.head;
        if (ap)
        {
            connect(ap->ssid, ap->pw);
        }
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

    static int xconnect(Connection *con, void *arg)
    {
        PO_DEBUG("");
        ASSERT(arg);
        Esp_WiFiInterface *wifi = (Esp_WiFiInterface*) arg;
        con->on_connect(wifi);
        return 0;
    }

    static int xdisconnect(Connection *con, void *arg)
    {
        PO_DEBUG("");
        ASSERT(arg);
        Esp_WiFiInterface *wifi = (Esp_WiFiInterface*) arg;
        con->on_disconnect(wifi);
        return 0;
    }

    void on_disconnect(esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        PO_DEBUG("id=%s", lut(event_id_lut, event_id));
        connections.visit(xdisconnect, this, mutex);
        ip_addr.v4.sin_family = 0;
    }

    static bool is_our_netif(const char *prefix, esp_netif_t *netif)
    {
        //PO_DEBUG("got='%s' expect='%s'", esp_netif_get_desc(netif), prefix);
        return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
    }

    void on_connect(esp_event_base_t event_base, int32_t event_id, ip_event_got_ip_t *event)
    {
        ASSERT(event_id == IP_EVENT_STA_GOT_IP);

        if (!is_our_netif(TAG, event->esp_netif))
        {
            PO_DEBUG("Got IPv4 from another interface \"%s\": ignored", esp_netif_get_desc(event->esp_netif));
            return;
        }
        PO_DEBUG("got %s", esp_netif_get_desc(event->esp_netif));
        PO_DEBUG("ip=" IPSTR " mask=" IPSTR " gw=" IPSTR, 
                IP2STR(&event->ip_info.ip),
                IP2STR(&event->ip_info.netmask),
                IP2STR(&event->ip_info.gw));

        // fill out the Connection data
        ip_addr.v4.sin_family = AF_INET;
        ip_addr.v4.sin_port = 0;
        memcpy(& ip_addr.v4.sin_addr, & event->ip_info.ip, sizeof(ip_addr.v4.sin_addr));
 
        connections.visit(xconnect, this, mutex);
    }

    static void on_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        ASSERT(arg);
        Esp_WiFiInterface *iface = (Esp_WiFiInterface*) arg;
        iface->on_disconnect(event_base, event_id, event_data);
    }

    static void on_connect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        ASSERT(arg);
        Esp_WiFiInterface *iface = (Esp_WiFiInterface*) arg;
        iface->on_connect(event_base, event_id, (ip_event_got_ip_t *) event_data);
    }

    void register_handlers()
    {
        esp_err_t err;
        err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, & on_disconnect, this);
        SHOW_ERR(err);
        err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, & on_connect, this);
        SHOW_ERR(err);
    }

    void unregister_handlers()
    {
        esp_err_t err;
        err = esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, & on_disconnect);
        SHOW_ERR(err);
        err = esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, & on_connect);
        SHOW_ERR(err);
    }

};

WiFiInterface *WiFiInterface::create()
{
    return new Esp_WiFiInterface;
}

}   //  namespace panglos


//   FIN
