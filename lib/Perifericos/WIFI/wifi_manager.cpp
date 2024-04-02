// wifi_manager.cpp
#include "wifi_manager.hpp"

esp_ip4_addr_t WiFiManager::ip;
bool WiFiManager::connected = false;
WiFiManager *WiFiManager::instance = nullptr;

WiFiManager::WiFiManager()
{
    instance = this;
    this->connected = false;
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler, NULL));
    ESP_LOGI("WiFiManager", "Constructor");
    esp_err_t ret = esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    if (ret == ESP_OK)
    {
        ESP_LOGI("WiFiManager", "Wi-Fi power save mode set to MAX_MODEM.\n");
    }
    else
    {
        ESP_LOGI("WiFiManager", "Failed to set Wi-Fi power save mode.\n");
    }
    memset(currentSSID, 0, sizeof(currentSSID));
}
bool WiFiManager::isConnected()
{
    return connected;
}
WiFiManager::~WiFiManager()
{
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler));
    esp_wifi_stop();
    esp_wifi_deinit();
    ESP_LOGI("WiFiManager", "Destructor");
}

void WiFiManager::connect(const char *ssid, const char *password)
{
    ESP_ERROR_CHECK(esp_wifi_stop());     // Certifique-se de parar o WiFi antes de reconfigurá-lo.
    vTaskDelay(100 / portTICK_PERIOD_MS); // Pequeno delay para garantir a parada do WiFi.

    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.channel = 0;
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    wifi_config.sta.failure_retry_cnt = 2;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());   // Inicia o WiFi após configurar.
    ESP_ERROR_CHECK(esp_wifi_connect()); // Agora tenta se conectar.

    ESP_LOGI("WiFiManager", "Tentando conectar a: %s", ssid);
}

void WiFiManager::disconnect()
{
    ESP_LOGI("WiFiManager", "Disconnect");
    esp_wifi_disconnect();
    ESP_LOGI("WiFiManager", "Disconnected");
}
char *WiFiManager::getSSID()
{
    return currentSSID;
}
void WiFiManager::scanAndConnect()
{
    if (!connected)
    { // Verifica se já não está conectado.
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI("WiFiManager", "Iniciando scan de redes WiFi...");
        esp_err_t result = esp_wifi_scan_start(nullptr, true); // Bloqueia até o scan completar.

        if (result == ESP_OK)
        {
            uint16_t numFound = 0;
            esp_wifi_scan_get_ap_num(&numFound);

            if (numFound == 0)
            {
                ESP_LOGI("WiFiManager", "Nenhuma rede encontrada.");
                return;
            }

            wifi_ap_record_t *apRecords = new wifi_ap_record_t[numFound];
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&numFound, apRecords));
            for (int i = 0; i < numFound; ++i)
            {
                ESP_LOGI("WiFiManager", "  %d: %s", i + 1, (char *)apRecords[i].ssid);
            }
            bool connectedToKnownNetwork = false;
            for (int i = 0; i < numFound && !connectedToKnownNetwork; ++i)
            {
                for (size_t j = 0; j < wifiCredentialsCount; ++j)
                {
                    if (strcmp((char *)apRecords[i].ssid, wifiCredentials[j].ssid) == 0)
                    {
                        ESP_LOGI("WiFiManager", "Tentando se conectar a: %s", wifiCredentials[j].ssid);
                        connect(wifiCredentials[j].ssid, wifiCredentials[j].password);
                        connectedToKnownNetwork = true;
                        break;
                    }
                }
            }

            if (!connectedToKnownNetwork)
            {
                ESP_LOGI("WiFiManager", "Nenhuma rede conhecida encontrada.");
            }

            delete[] apRecords;
        }
        else
        {
            ESP_LOGE("WiFiManager", "Scan falhou com erro: %d", result);
        }
    }
    else
    {
        ESP_LOGI("WiFiManager", "Já conectado. Não é necessário escanear.");
    }
}

esp_ip4_addr_t WiFiManager::getIP()
{
    return ip;
}

void WiFiManager::eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI("WiFiManager", "eventHandler called with event_base=%s, event_id=%ld", event_base, event_id);
    if (event_base == WIFI_EVENT && instance)
    {
        switch (event_id)
        {
        case WIFI_EVENT_WIFI_READY:
            ESP_LOGI("WiFiManager-WIFI_EVENT", "WIFI_EVENT_WIFI_READY");
            break;
        case WIFI_EVENT_SCAN_DONE:
            ESP_LOGI("WiFiManager-WIFI_EVENT", "WIFI_EVENT_SCAN_DONE");
            break;
        case WIFI_EVENT_STA_START:
            ESP_LOGI("WiFiManager-WIFI_EVENT", "WIFI_EVENT_STA_START");
            instance->connected = false;
            break;
        case WIFI_EVENT_STA_STOP:
            ESP_LOGI("WiFiManager-WIFI_EVENT", "WIFI_EVENT_STA_STOP");
            break;

        case WIFI_EVENT_STA_CONNECTED:
        {
            ESP_LOGI("WiFiManager-WIFI_EVENT", "WIFI_EVENT_STA_CONNECTED");
            wifi_event_sta_connected_t *event = reinterpret_cast<wifi_event_sta_connected_t *>(event_data);
            strncpy(instance->currentSSID, reinterpret_cast<const char *>(event->ssid), sizeof(instance->currentSSID) - 1);
            instance->currentSSID[sizeof(instance->currentSSID) - 1] = '\0';
            instance->connected = true;
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI("WiFiManager-WIFI_EVENT", "WIFI_EVENT_STA_DISCONNECTED");
            instance->connected = false;
            memset(instance->currentSSID, 0, sizeof(instance->currentSSID)); // Limpa o SSID na desconexão
            // Aqui você pode escolher reintroduzir uma lógica de reconexão, se desejado.
            break;
            // Adicione mais tratamentos de eventos de WIFI_EVENT conforme necessário
        }
    }
    else if (event_base == IP_EVENT && instance)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
        {
            ip_event_got_ip_t *event = reinterpret_cast<ip_event_got_ip_t *>(event_data);
            // Verifica se o endereço IP obtido é diferente de 0.0.0.0
            if (event->ip_info.ip.addr != 0)
            {
                ESP_LOGI("WIFI-STATUS", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
                instance->ip = event->ip_info.ip; // Atualiza o endereço IP da instância
                instance->connected = true;       // Confirma a conexão após receber um IP válido
            }
            else
            {
                ESP_LOGE("WIFI-STATUS", "Received invalid IP address: 0.0.0.0");
                instance->connected = false; // Marca como desconectado se o IP for inválido
            }
            break;
        }
        case IP_EVENT_STA_LOST_IP:
            ESP_LOGI("WiFiManager-IP_EVENT", "IP_EVENT_STA_LOST_IP");
            instance->connected = false; // Marca como desconectado se o IP foi perdido
            break;
            // Outros eventos IP...
        }
    }
}