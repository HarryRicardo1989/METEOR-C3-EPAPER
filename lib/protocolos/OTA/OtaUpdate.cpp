#include "OtaUpdate.hpp"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "sleep_timer.h"
#include "rw_data.h"
OtaUpdate *OtaUpdate::instance = nullptr;

OtaUpdate::OtaUpdate() : crc(0)
{
    // Empt...
}

esp_err_t OtaUpdate::start(const char *ota_url, const char *expected_crc_hex)
{
    OtaUpdate::instance = this;
    this->crc = 0;                                               // Reinicia o CRC para zero
    this->expected_crc = strtoul(expected_crc_hex, nullptr, 16); // Converte CRC de hex para uint32_t

    esp_http_client_config_t config = {};
    config.url = ota_url;
    config.event_handler = http_event_handler;

    esp_https_ota_config_t ota_config = {};
    ota_config.http_config = &config;

    ESP_LOGI("OtaUpdate", "Attempting to download update from %s", config.url);
    esp_https_ota_begin(&ota_config, &ota_handle);
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE("OtaUpdate", "OTA failed with error: %s", esp_err_to_name(ret));
        return ret;
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
        esp_ota_mark_app_invalid_rollback_and_reboot();
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
        ESP_LOGI("OtaUpdate", "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI("OtaUpdate", "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI("OtaUpdate", "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI("OtaUpdate", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI("OtaUpdate", "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            ESP_LOGI("OtaUpdate", "Calculating CRC for data chunk. Current CRC: %" PRIu32, OtaUpdate::instance->crc);
            ESP_LOGI("OtaUpdate", " Expected CRC: %" PRIu32, OtaUpdate::instance->expected_crc);
            OtaUpdate::instance->crc = esp_crc32_le(OtaUpdate::instance->crc, (uint8_t *)evt->data, evt->data_len);
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI("OtaUpdate", "HTTP_EVENT_ON_FINISH");
        if (OtaUpdate::instance->crc != OtaUpdate::instance->expected_crc)
        {
            ESP_LOGE("OtaUpdate", "CRC mismatch: Calculated CRC = %" PRIu32 ", Expected CRC = %" PRIu32, OtaUpdate::instance->crc, OtaUpdate::instance->expected_crc);
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
        else
        {
            ESP_LOGI("OtaUpdate", "CRC match: Update is valid.");
            ESP_LOGI("OtaUpdate", " Expected CRC: %" PRIu32, OtaUpdate::instance->expected_crc);
            ESP_LOGI("OtaUpdate", " Current CRC:  %" PRIu32, OtaUpdate::instance->crc);

            esp_ota_mark_app_valid_cancel_rollback();
            OtaUpdate::instance->validateAndUpdatePartition();
        }
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI("OtaUpdate", "HTTP_EVENT_DISCONNECTED");
        if (OtaUpdate::instance->crc != OtaUpdate::instance->expected_crc)
        {
            ESP_LOGE("OtaUpdate", "CRC mismatch: Calculated CRC = %" PRIu32 ", Expected CRC = %" PRIu32, OtaUpdate::instance->crc, OtaUpdate::instance->expected_crc);
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
        else
        {
            ESP_LOGI("OtaUpdate", "CRC match: Update is valid.");
            ESP_LOGI("OtaUpdate", " Expected CRC: %" PRIu32, OtaUpdate::instance->expected_crc);
            ESP_LOGI("OtaUpdate", " Current CRC:  %" PRIu32, OtaUpdate::instance->crc);

            esp_ota_mark_app_valid_cancel_rollback();
            OtaUpdate::instance->validateAndUpdatePartition();
        }
        break;
    default:
        ESP_LOGI("OtaUpdate", "Unhandled HTTP Event: %d", evt->event_id);
        break;
    }
    return ESP_OK;
}
