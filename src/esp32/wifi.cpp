
#include "esp_netif.h"
#include "esp_wifi.h"

#include "panglos/debug.h"
#include "panglos/esp32/hal.h"

#include "panglos/esp32/wifi.h"

using namespace panglos;

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

void ESP_WiFi::on_disconnect(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    PO_DEBUG("id=%s", lut(event_id_lut, event_id));

    if (notify)
    {
        if (con.ip.v4.sin_family == 0)
        {
            notify->on_disconnect(0);
        }
        else
        {
            notify->on_disconnect(& con);
        }
    }
}

static bool is_our_netif(const char *prefix, esp_netif_t *netif)
{
    //PO_DEBUG("got='%s' expect='%s'", esp_netif_get_desc(netif), prefix);
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

void ESP_WiFi::on_connect(esp_event_base_t event_base, int32_t event_id, ip_event_got_ip_t *event)
{
    ASSERT(event_id == IP_EVENT_STA_GOT_IP);

    if (!is_our_netif(TAG, event->esp_netif))
    {
        PO_DEBUG("Got IPv4 from another interface \"%s\": ignored", esp_netif_get_desc(event->esp_netif));
        return;
    }
    //PO_DEBUG("got %s", esp_netif_get_desc(event->esp_netif));
    //PO_DEBUG("ip=" IPSTR " mask=" IPSTR " gw=" IPSTR, 
    //        IP2STR(&event->ip_info.ip),
    //        IP2STR(&event->ip_info.netmask),
    //        IP2STR(&event->ip_info.gw));

    // fill out the Connection data
    con.ip.v4.sin_family = AF_INET;
    con.ip.v4.sin_port = 0;
    memcpy(& con.ip.v4.sin_addr, & event->ip_info.ip, sizeof(con.ip.v4.sin_addr));

    if (notify)
    {
        notify->on_connect(& con);
    }
}

    /*
     *  Event Callback functions
     */

void ESP_WiFi::on_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ASSERT(arg);
    ESP_WiFi *iface = (ESP_WiFi*) arg;
    iface->on_disconnect(event_base, event_id, event_data);
}

void ESP_WiFi::on_connect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ASSERT(arg);
    ESP_WiFi *iface = (ESP_WiFi*) arg;
    iface->on_connect(event_base, event_id, (ip_event_got_ip_t *) event_data);
}

    /*
     *
     */

void ESP_WiFi::connect(Notify *n, const char *ssid, const char *pw)
{
    notify = n;
    memset(& con, 0, sizeof(con));

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

void ESP_WiFi::disconnect(Notify *n)
{
    notify = n;
    // check it is connected?
    esp_err_t err = esp_wifi_stop();
    SHOW_ERR(err);
}

    /*
     *  Register / unregister callbacks
     */

void ESP_WiFi::register_handlers()
{
    esp_err_t err;
    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, & on_disconnect, this);
    SHOW_ERR(err);
    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, & on_connect, this);
    SHOW_ERR(err);
}

void ESP_WiFi::unregister_handlers()
{
    esp_err_t err;
    err = esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, & on_disconnect);
    SHOW_ERR(err);
    err = esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, & on_connect);
    SHOW_ERR(err);
}

    /*
     *
     */

ESP_WiFi::ESP_WiFi()
:   notify(0)
{
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

ESP_WiFi::~ESP_WiFi()
{
    unregister_handlers();
}

WiFiInterface *WiFiInterface::create()
{
    return new ESP_WiFi;
}

//  FIN
