#pragma once
#include <cstddef>
struct WiFiCredential
{
    const char *ssid;
    const char *password;
};

const WiFiCredential wifiCredentials[] = {
    {"Automacao", "Meunomeinteirotudojunto"},
    {"AAB-DEV", "AAB_DEV1"},
    {"Cheline - vivo", "sumiu#2825"},
    {"AAGuest", "udi@1009"},
    {"AAMobi", "$$aab$$@2017@"},
    {"ADVOGADOSPLENOS", "AGB#780$$"},
    {"Cheline4", "Sumiu2825"},
    {"WECLIXWIFI", "meunomeInteiroTudojunto"},
    {"Weclix", "meunomeinteirotudojunto"},
    {"ALCANSWIFI", "meunomeInteiroTudojunto"},
};
const size_t wifiCredentialsCount = sizeof(wifiCredentials) / sizeof(wifiCredentials[0]);

#define MQTT_URI "20.127.210.50"
#define MQTT_PORT 1883
#define MQTT_QOS 2
#define MQTT_RETAIN false

#define MQTT_CLIENT_USER "pcd_meteor"
#define MQTT_CLIENT_PASS "pcd_meteor@123"

#define MQTT_TOPIC (char *)"pcd-meteor"
#define MQTT_TOPIC_TO_SITE (char *)"pcd-meteor/site"

#define MQTT_SUBTOPIC_OTA (char *)"/OTA"
#define MQTT_SUBTOPIC_STATUS (char *)"/status"
#define MQTT_SUBTOPIC_BATTERY (char *)"/battery"
#define MQTT_SUBTOPIC_INFO (char *)"/info"
#define MQTT_SUBTOPIC_EVENT (char *)"/event"

#define MQTT_SUBTOPIC_TEMPERATURE (char *)"/temp"
#define MQTT_SUBTOPIC_HUMIDITY (char *)"/humidity"
#define MQTT_SUBTOPIC_PRESSURE (char *)"/pressure"
#define MQTT_SUBTOPIC_DEWPOINT (char *)"/dewpoint"
#define MQTT_SUBTOPIC_BATTERY_LEVEL (char *)"/level"
#define MQTT_SUBTOPIC_BATTERY_CRITICAL (char *)"/critical"
#define MQTT_SUBTOPIC_BATTERY_VOLTAGE (char *)"/voltage"
#define MQTT_SUBTOPIC_BATTERY_CHARGING (char *)"/charging"
#define MQTT_SUBTOPIC_BATTERY_CHARGED (char *)"/charged"
#define MQTT_SUBTOPIC_INFO_MANUFACTORY (char *)"/manufactory"
#define MQTT_SUBTOPIC_INFO_FIRMWARE_VERSION (char *)"/fwVer"
#define MQTT_SUBTOPIC_INFO_MODEL (char *)"/model"
#define MQTT_SUBTOPIC_INFO_MAC_ADDRESS (char *)"/MACaddr"
#define MQTT_SUBTOPIC_INFO_CLIENTID (char *)"/clientID"