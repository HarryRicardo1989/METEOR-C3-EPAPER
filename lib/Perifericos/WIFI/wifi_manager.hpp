#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include <cstring>
#include "esp_log.h"
#include "connections.h"

class WiFiManager
{
public:
    static WiFiManager *instance;

    WiFiManager();
    ~WiFiManager();
    void connect(const char *ssid, const char *password);
    void disconnect();
    esp_ip4_addr_t getIP();
    bool isConnected();
    void scanAndConnect(); // Supondo que essa função já esteja implementada
    char *getSSID();       // Método para obter o SSID

private:
    static void eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static esp_ip4_addr_t ip;
    static bool connected;
    char currentSSID[33]; // Espaço para armazenar o SSID (32 chars + terminador nulo)
};
