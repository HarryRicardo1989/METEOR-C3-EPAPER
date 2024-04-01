#include "MqttInit.hpp"
#include <cstring>
#include <string>
namespace PROTOCOL
{

    MqttInit::MqttInit()
    {
    }
    MqttInit::~MqttInit()
    {
    }

    void MqttInit::connect()
    {

        auto &mqtt_client_instance = MqttClient::getInstance();
        mqtt_client_instance.connect();
    }
    void MqttInit::disconnect()
    {

        auto &mqtt_client_instance = MqttClient::getInstance();
        mqtt_client_instance.disconnect();
    }
    void MqttInit::subscribe()
    {
        auto &mqtt_client_instance = MqttClient::getInstance();
        char *client_id = read_nvs_string_var(CLIENT_ID);
        std::string mqtt_subscription(client_id);
        mqtt_subscription += MQTT_SUBTOPIC_OTA;
        mqtt_client_instance.subscribe(mqtt_subscription.c_str());
        free(client_id);
    }
    void MqttInit::unsubscribe()
    {
        auto &mqtt_client_instance = MqttClient::getInstance();
        mqtt_client_instance.unsubscribe(mqttsub_scription.data());
    }

    void MqttInit::publish_full_data()
    {
        auto &mqtt_publish_instance = PROTOCOL::MqttPublish::getInstance();

        mqtt_publish_instance.publish_all();
    }

}
