#pragma once

#include "SPI/cppspi.hpp"
#include <cstring>
#include "gpio/gpio.hpp"
#include "driver/gpio.h"
#include "interfaces_map.h"

class EpaperDisplay
{
public:
    EpaperDisplay(PROTOCOL::Spi *spi, gpio_num_t pin_dc, gpio_num_t pin_rst, gpio_num_t pin_busy);
    void init();
    void clear(int width, int height);
    void display(const uint8_t *image, int width, int height);
    void sleep();

private:
    PROTOCOL::Spi *spi;
    Perifericos::Gpio gpio_pin_dc;
    Perifericos::Gpio gpio_pin_rst;
    Perifericos::Gpio gpio_pin_busy;

    gpio_num_t pin_dc;
    gpio_num_t pin_rst;
    gpio_num_t pin_busy;

    void reset();
    void sendCommand(uint8_t command);
    void epd_write_reg(uint8_t command);
    void epd_write_data(uint8_t command);
    void sendData(const uint8_t *data, size_t length);
    void waitForIdle();
    void loadLUTFull();
    void loadLUTPartial();
};
