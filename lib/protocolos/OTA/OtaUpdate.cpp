#include "OtaUpdate.hpp"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "sleep_timer.h"
#include "rw_data.h"

OtaUpdate::OtaUpdate()
{
    // Construtor vazio...
}

esp_err_t OtaUpdate::start(const char *ota_url)
{

    esp_http_client_config_t config = {};
    config.url = ota_url;
    config.event_handler = http_event_handler;
    // Aqui você pode inicializar todos os outros campos conforme necessário...

    esp_https_ota_config_t ota_config = {};
    ota_config.http_config = &config;

    ESP_LOGI("OtaUpdate", "Attempting to download update from %s", config.url);
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK)
    {
        ESP_LOGI("OtaUpdate", "OTA Update succeeded");
        save_nvs_int8_var(UPDATE_STATUS, false);
        ESP_LOGI("UPDATE_STATUS", "false");
        validateAndUpdatePartition();
    }
    else
    {
        ESP_LOGE("OtaUpdate", "OTA FAIL");
    }
    return ret;
}
void OtaUpdate::validateAndUpdatePartition()
{
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running)
    {
        ESP_LOGI("OtaUpdate", "Configurado para boot de uma partição diferente da que está em execução");
    }
    else
    {
        ESP_LOGI("OtaUpdate", "Atualização bem-sucedida, configurando a nova partição como válida de boot");
        esp_ota_mark_app_valid_cancel_rollback();
    }
}
esp_err_t OtaUpdate::http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD("OtaUpdate", "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD("OtaUpdate", "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD("OtaUpdate", "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD("OtaUpdate", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD("OtaUpdate", "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD("OtaUpdate", "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD("OtaUpdate", "HTTP_EVENT_DISCONNECTED");
        break;
    default:
        break;
    }
    return ESP_OK;
}