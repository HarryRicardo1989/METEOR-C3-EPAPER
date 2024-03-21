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

    // Ram data entry mode
    sendCommand(0x11);
    sendData((const uint8_t[]){0x03}, 1);

    // Set Ram X address
    sendCommand(0x44);
    sendData((const uint8_t[]){0x01, 0x10}, 2);

    // Set Ram Y address
    sendCommand(0x45);
    sendData((const uint8_t[]){0xF9, 0x00, 0x00, 0x00}, 4);

    // Set border
    sendCommand(0x3C);
    sendData((const uint8_t[]){0xC0}, 1);

    // Set VCOM value
    sendCommand(0x2C);
    sendData((const uint8_t[]){0x70}, 1);

    // Gate voltage setting
    sendCommand(0x03);
    sendData((const uint8_t[]){0x17}, 1);

    // Source voltage setting
    sendCommand(0x04);
    sendData((const uint8_t[]){0x41, 0x00, 0x32}, 3);
    loadLUTPartial();
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

void EpaperDisplay::epd_write_reg(uint8_t command)
{

    gpio_pin_dc.set_level(false);     // DC low for command
    spi->transferByte(0, command, 0); // Sending command
    gpio_pin_dc.set_level(true);      // DC high for data
}
void EpaperDisplay::epd_write_data(uint8_t command)
{

    gpio_pin_dc.set_level(true);      // DC low for command
    spi->transferByte(0, command, 0); // Sending command
    gpio_pin_dc.set_level(true);      // DC high for data
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
    sendCommand(0x21); // MASTER_ACTIVATION
    waitForIdle();
}
void EpaperDisplay::clear(int width, int height)
{
    const size_t bufferSize = width * height;
    uint8_t *buffer = new uint8_t[bufferSize];
    memset(buffer, 1, bufferSize); // Set all white
    display(buffer, width, height);
    delete[] buffer;
}

void EpaperDisplay::sleep()
{
    sendCommand(0x10); // Enter deep sleep mode
    sendData((const uint8_t[]){0x11}, 1);
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

void EpaperDisplay::loadLUTPartial()
{

    static const uint8_t lut_partial[] = {
        0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x80, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x40, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0A, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x0, 0x0, 0x0};
    sendCommand(0x32);                          // Comando para carregar a LUT
    sendData(lut_partial, sizeof(lut_partial)); // Enviando a LUT para o display
    // Enviando as LUTs para o display
}