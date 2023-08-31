
#if defined(ESP32)

// ESP32 API
#include "mqtt_client.h"

#include "panglos/debug.h"

#include "panglos/mqtt.h"

namespace panglos {

    /*
     *  List utils
     */

struct Match 
{
    const char *topic;
    int len;
};

static int subs_match(MqttSub *item, void *arg)
{
    ASSERT(arg);
    struct Match *match = (struct Match *) arg;

    if (strlen(item->topic) != match->len)
    {
        return false;
    }
    return strncmp(match->topic, item->topic, match->len) == 0;
}

    /*
     *
     */

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ASSERT(handler_args);
    MqttClient *handler = (MqttClient *) handler_args;
    ASSERT(event_data);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    MqttEvent *mevent = (MqttEvent*) event;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        handler->on_connected(mevent);
        break;
    case MQTT_EVENT_DISCONNECTED:
        handler->on_disconnected(mevent);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        handler->on_subscribed(mevent);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        handler->on_unsubscribed(mevent);
        break;
    case MQTT_EVENT_PUBLISHED:
        PO_INFO("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        handler->on_data(mevent);
        break;
    case MQTT_EVENT_ERROR:
        handler->on_error(mevent);
        break;
    case MQTT_EVENT_BEFORE_CONNECT:
        PO_INFO("MQTT_EVENT_BEFORE_CONNECT");
        break;
    case MQTT_EVENT_DELETED:
        PO_INFO("MQTT_EVENT_DELETED");
        break;
    default:
        PO_INFO("Other event id:%d", event->event_id);
        break;
    }
}

    /*
     *
     */

void MqttClient::app_start(const char *uri)
{
    PO_DEBUG("uri=%s", uri);

    esp_mqtt_client_config_t mqtt_cfg;
    memset(& mqtt_cfg, 0, sizeof(mqtt_cfg));
    mqtt_cfg.uri = uri;

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(& mqtt_cfg);

    handle = (Handle*) client;

    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, this);
    esp_mqtt_client_start(client);
}

    /*
     *
     */

MqttClient::MqttClient()
:   handle(0),
    connected(false),
    next(0), 
    subs(MqttSub::get_next)
{
}

void MqttClient::subscribe(MqttSub *sub)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t) handle;
    int msg_id = esp_mqtt_client_subscribe(client, sub->topic, 0);
    PO_INFO("topic=%s msg_id=%d", sub->topic, msg_id);
}

void MqttClient::add(MqttSub *sub)
{
    PO_DEBUG("%s", sub->topic);
    subs.append(sub, 0);

    if (connected)
    {
        subscribe(sub);
    }
}

MqttSub *MqttClient::find(const char *topic, int len)
{
    struct Match match = { .topic = topic, .len = len };
    return subs.find(subs_match, (void*) & match, 0);
}

    /*
     *  Event Handlers
     */

void MqttClient::on_connected(MqttEvent *mevent)
{
    MqttSub *item = subs.head;

    if (!item)
    {
        PO_INFO("no subscriptions");
    }

    if (item)
    {
        subscribe(item);
        next = item->next;
    }
}

void MqttClient::on_subscribed(MqttEvent *event)
{
    if (next)
    {
        subscribe(next);
        next = next->next;
    }

    // set true when finished subscribing to handlers
    // in the queue when starting.
    connected = next == 0;
}

void MqttClient::on_data(MqttEvent *mevent)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) mevent;
    MqttSub *item = find(event->topic, event->topic_len);

    if (!item)
    {
        PO_ERROR("no handler for topic '%.*s'", event->topic_len, event->topic);
        return;
    }
    ASSERT(item->handler);
    item->handler(item->arg, event->data, event->data_len);
}

void MqttClient::on_disconnected(MqttEvent *event)
{
    PO_DEBUG("ev=%p", event);
    connected = false;
    // What to do?
}

void MqttClient::on_unsubscribed(MqttEvent *event)
{
    PO_DEBUG("ev=%p", event);
}

void MqttClient::on_error(MqttEvent *mevent)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) mevent;
    PO_DEBUG("ev=%p", event);
    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
    {
        PO_ERROR("errno=%s", strerror(event->error_handle->esp_transport_sock_errno));
    }
}

    /*
     *
     */

int MqttClient::publish(const char *topic, const char *data, int len, int qos, int retain)
{
    esp_mqtt_client_handle_t h = (esp_mqtt_client_handle_t) handle;
    return esp_mqtt_client_publish(h, topic, data, len, qos, retain);
}

}   //  namespace panglos

#endif  //  ESP32

//  FIN
