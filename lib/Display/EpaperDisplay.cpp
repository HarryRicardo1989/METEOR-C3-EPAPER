#include "EpaperDisplay.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"

EpaperDisplay::EpaperDisplay(PROTOCOL::Spi *spi, gpio_num_t pin_dc, gpio_num_t pin_rst, gpio_num_t pin_busy)
    : spi(spi), pin_dc(pin_dc), pin_rst(pin_rst), pin_busy(pin_busy)
{
    gpio_pin_dc = Perifericos::Gpio(pin_dc, GPIO_MODE_OUTPUT, GPIO_PULLUP_ONLY);
    gpio_pin_rst = Perifericos::Gpio(pin_rst, GPIO_MODE_OUTPUT, GPIO_PULLUP_ONLY);
    gpio_pin_busy = Perifericos::Gpio(pin_busy, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY);
}

void EpaperDisplay::init()
{
    vTaskDelay(pdMS_TO_TICKS(10)); // Delay após o reset hardware
                                   // HW Reset
    reset();

    sendCommand(0x12); // Software reset
    waitForIdle();     // Espera pelo pino busy

    // Set display size e driver output control
    sendCommand(0x01);
    sendData((const uint8_t[]){0xF9, 0x00, 0x00}, 3);
    waitForIdle(); // Espera pelo pino busy

    sendCommand(0x0C); // BOOSTER_SOFT_START_CONTROL
    sendData((const uint8_t[]){0xD7, 0xD6, 0x9D}, 3);

    sendCommand(0x2C); // WRITE_VCOM_REGISTER
    sendData((const uint8_t[]){0xA8}, 1);

    sendCommand(0x3A); // SET_DUMMY_LINE_PERIOD
    sendData((const uint8_t[]){0x1A}, 1);

    sendCommand(0x3B); // SET_GATE_TIME
    sendData((const uint8_t[]){0x08}, 1);

    sendCommand(0x11); // DATA_ENTRY_MODE_SETTING
    sendData((const uint8_t[]){0x01}, 1);
    loadLUT();
}

void EpaperDisplay::reset()
{
    gpio_pin_rst.set_level(true);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_pin_rst.set_level(false);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_pin_rst.set_level(true);
    vTaskDelay(pdMS_TO_TICKS(10));
}

void EpaperDisplay::sendCommand(uint8_t command)
{
    gpio_pin_dc.set_level(false);     // DC low for command
    spi->transferByte(0, command, 0); // Sending command
}

void EpaperDisplay::sendData(const uint8_t *data, size_t length)
{
    gpio_pin_dc.set_level(true); // DC high for data

    uint8_t *tx_buffer = new uint8_t[length];
    memcpy(tx_buffer, data, length);
    uint8_t *rx_buffer = nullptr;
    esp_err_t result = spi->transferMultiplesBytes(0, tx_buffer, rx_buffer, length, 0);

    if (result != ESP_OK)
    {
        ESP_LOGE("EPAPER", "Falha ao enviar dados para o display e-paper: %s", esp_err_to_name(result));
    }
    delete[] tx_buffer;
}

void EpaperDisplay::display(const uint8_t *image, int width, int height)
{
    const size_t bufferSize = width * height / 8;
    // Assuming the image data is prepared for display

    // Example of setting X and Y counters assuming a certain addressing scheme
    sendCommand(0x4e); // SET_RAM_X_ADDRESS_COUNTER
    sendData((const uint8_t[]){0x01}, 1);
    sendCommand(0x4f); // SET_RAM_Y_ADDRESS_COUNTER
    sendData((const uint8_t[]){0xF9, 0x00}, 2);

    sendCommand(0x24); // WRITE_RAM
    for (size_t i = 0; i < bufferSize; i++)
    {
        sendData(&image[i], 1);
    }
    // Trigger the display update
    sendCommand(0x20); // MASTER_ACTIVATION
    waitForIdle();
}
void EpaperDisplay::clear(int width, int height)
{
    const size_t bufferSize = width * height;
    uint8_t *buffer = new uint8_t[bufferSize];
    memset(buffer, 0x00, bufferSize); // Set all white
    display(buffer, width, height);
    delete[] buffer;
}

void EpaperDisplay::sleep()
{
    sendCommand(0x10); // Enter deep sleep mode
    sendData((const uint8_t[]){0x01}, 1);
}

void EpaperDisplay::waitForIdle()
{
    ESP_LOGI("EP_IDLE", "WAITING");
    while (gpio_pin_busy.get_level() == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGI("EP_IDLE", "IDLE");
}

void EpaperDisplay::loadLUT()
{
    // Exemplo genérico para carregar a LUT
    // Os valores reais devem ser substituídos pelos especificados no datasheet do seu display

    // LUT para atualização padrão
    static const uint8_t lut_vcom_dc[] = {
        0x00,
        0x00,
        0x00,
        0x12,
        0x12,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    };

    static const uint8_t lut_ww[] = {
        // LUT para a parte branca
        0x10,
        0x14,
        0x0A,
        0x14,
        0x14,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    };

    static const uint8_t lut_bw[] = {
        // LUT para a transição de preto para branco
        0x10,
        0x14,
        0x0A,
        0x14,
        0x14,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    };

    static const uint8_t lut_bb[] = {
        // LUT para a parte preta
        0x00,
        0x00,
        0x00,
        0x12,
        0x12,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    };

    static const uint8_t lut_wb[] = {
        // LUT para a transição de branco para preto
        0x00,
        0x00,
        0x00,
        0x12,
        0x12,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    };

    // Enviando as LUTs para o display
    sendCommand(0x20); // comando para LUT de VCOM
    for (uint8_t i = 0; i < sizeof(lut_vcom_dc); i++)
    {
        sendData(&lut_vcom_dc[i], 1);
    }

    sendCommand(0x21); // comando para LUT de branco para branco
    for (uint8_t i = 0; i < sizeof(lut_ww); i++)
    {
        sendData(&lut_ww[i], 1);
    }

    sendCommand(0x22); // comando para LUT de preto para branco
    for (uint8_t i = 0; i < sizeof(lut_bw); i++)
    {
        sendData(&lut_bw[i], 1);
    }

    sendCommand(0x23); // comando para LUT de preto para preto
    for (uint8_t i = 0; i < sizeof(lut_bb); i++)
    {
        sendData(&lut_bb[i], 1);
    }

    sendCommand(0x24); // comando para LUT de branco para preto
    for (uint8_t i = 0; i < sizeof(lut_wb); i++)
    {
        sendData(&lut_wb[i], 1);
    }
}