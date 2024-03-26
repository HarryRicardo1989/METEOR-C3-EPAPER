#pragma once

#include "tasks_common.h"

#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_crc.h"
#include <inttypes.h>

class OtaUpdate
{
private:
    static OtaUpdate *instance; // Ponteiro para armazenar a instância da classe
    uint32_t crc = 0;           // Variável para armazenar o CRC acumulado
    uint32_t expected_crc = 0;  // Adiciona membro para armazenar o CRC esperado
    esp_https_ota_handle_t ota_handle = nullptr;

public:
    OtaUpdate();
    esp_err_t start(const char *ota_url, const char *expected_crc_hex);
    static esp_err_t http_event_handler(esp_http_client_event_t *evt);
    void validateAndUpdatePartition();
    // Getter para o CRC (opcional)
    uint32_t getCRC() const { return crc; }
};
