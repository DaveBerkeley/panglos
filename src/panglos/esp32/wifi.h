
#pragma once

#include "esp_event.h"
#include "esp_netif_types.h"

#include "panglos/wifi.h"

namespace panglos {

class ESP_WiFi : public WiFiInterface
{
    Notify *notify;
    Connection con;
 
    void on_disconnect(esp_event_base_t event_base, int32_t event_id, void *event_data);
    void on_connect(esp_event_base_t event_base, int32_t event_id, ip_event_got_ip_t *event);

    static void on_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void on_connect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

    virtual void connect(Notify *n, const char *ssid, const char *pw) override;
    virtual void disconnect(Notify *n) override;

    void register_handlers();
    void unregister_handlers();

public:

    ESP_WiFi();
    ~ESP_WiFi();
};

}   //  namespace panglos

//  FIN
