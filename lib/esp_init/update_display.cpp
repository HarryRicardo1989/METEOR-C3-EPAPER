#include "update_display.hpp"

EPD_UPDATE
::EPD_UPDATE(EpaperDisplay *epd_instance)
{
    this->_epd_instance = epd_instance;
    this->_epd_instance->epd_paint_newimage(image_bw, EPD_W, EPD_H, EPD_ROTATE_0, EPD_COLOR_WHITE);
    this->_epd_instance->epd_paint_selectimage(this->image_bw);
    this->_epd_instance->init();
    ESP_LOGI("EPD_UPDATE", "Instanciado");
}

EPD_UPDATE
::~EPD_UPDATE()
{
}

void EPD_UPDATE::displayClear()
{
    this->_epd_instance->epd_paint_clear(EPD_COLOR_WHITE);
    displayUpdate();
}

void EPD_UPDATE::displayUpdate()
{
    this->_epd_instance->epd_displayBW(this->image_bw);
}

void EPD_UPDATE::displayUpdatePartial()
{
    this->_epd_instance->epd_displayBW_partial(this->image_bw);
}

void EPD_UPDATE::display_total(DeviceInfo *device_info)
{
    this->_device_info = device_info;
    wakeup();
    this->_epd_instance->epd_paint_clear(EPD_COLOR_WHITE);

    display_make();
    ESP_LOGI("EPD_UPDATE", "display_make");
    display_level_box();
    ESP_LOGI("EPD_UPDATE", "display_level_box");
    display_voltage_box();
    ESP_LOGI("EPD_UPDATE", "display_voltage_box");
    display_lines();
    ESP_LOGI("EPD_UPDATE", "display_lines");
    displayUpdate();
    ESP_LOGI("EPD_UPDATE", "displayUpdatePartial");
    sleep();
    display_connections(this->_device_info);
    this->_device_info = nullptr;
}

void EPD_UPDATE::display_partial(DeviceInfo *device_info)
{
    this->_device_info = device_info;

    wakeup();

    char buffer[40];
    this->_epd_instance->epd_paint_showString(46, 27, (uint8_t *)"    ", EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    this->_epd_instance->epd_paint_showString(46, 46, (uint8_t *)"      ", EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    this->_epd_instance->epd_paint_showString(46, 65, (uint8_t *)"    ", EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);

    this->_epd_instance->epd_paint_showString(175, 27, (uint8_t *)"   ", EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    this->_epd_instance->epd_paint_showString(161, 46, (uint8_t *)"       ", EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);

    this->_epd_instance->epd_paint_showString(224, 2, (uint8_t *)"   ", EPD_FONT_SIZE8x6, EPD_COLOR_BLACK);
    this->_epd_instance->epd_paint_showString(211, 15, (uint8_t *)"      ", EPD_FONT_SIZE8x6, EPD_COLOR_BLACK);

    this->_epd_instance->epd_displayBW_partial(this->image_bw);

    // info Esquerda
    sprintf(buffer, "%.2f", this->_device_info->Temperature); // Formata a temperatura
    this->_epd_instance->epd_paint_showString(46, 27, (uint8_t *)buffer, EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    sprintf(buffer, "%.2f", this->_device_info->Pressure); // Formata a pressão
    this->_epd_instance->epd_paint_showString(46, 46, (uint8_t *)buffer, EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    sprintf(buffer, "%.2f", this->_device_info->DewPoint); // Formata o ponto de orvalho
    this->_epd_instance->epd_paint_showString(46, 65, (uint8_t *)buffer, EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);

    // info Direita
    sprintf(buffer, "%d", this->_device_info->Humidity); // Formata a umidade
    this->_epd_instance->epd_paint_showString(178, 27, (uint8_t *)buffer, EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    sprintf(buffer, "%.2f", this->_device_info->Altitude); // Formata o ponto de altitude
    this->_epd_instance->epd_paint_showString(168, 46, (uint8_t *)buffer, EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    sprintf(buffer, "%d", this->_device_info->bat_level); // Formata o nível da bateria
    this->_epd_instance->epd_paint_showString(224, 2, (uint8_t *)buffer, EPD_FONT_SIZE8x6, EPD_COLOR_BLACK);
    sprintf(buffer, "%ld", this->_device_info->bat_mv); // Formata a tensao da bateria
    this->_epd_instance->epd_paint_showString(211, 15, (uint8_t *)buffer, EPD_FONT_SIZE8x6, EPD_COLOR_BLACK);
    this->_epd_instance->epd_displayBW_partial(this->image_bw);

    display_connections(this->_device_info);

    sleep();
    this->_device_info = nullptr;
}

void EPD_UPDATE::display_make()
{
    this->_epd_instance->epd_paint_showString(20, 0, (uint8_t *)&"METEOR-MOBILE", EPD_FONT_SIZE24x12, EPD_COLOR_BLACK);
    // info esquerda
    this->_epd_instance->epd_paint_showString(0, 27, (uint8_t *)"Temp:      C", EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    this->_epd_instance->epd_paint_showString(0, 46, (uint8_t *)"Press:      hPa", EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    this->_epd_instance->epd_paint_showString(0, 65, (uint8_t *)"DewP:      C", EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);

    // info Direita
    this->_epd_instance->epd_paint_showString(129, 27, (uint8_t *)"Humid:    %", EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    this->_epd_instance->epd_paint_showString(129, 46, (uint8_t *)"Alt:        mts", EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    this->_epd_instance->epd_paint_showString(224, 2, (uint8_t *)"   %", EPD_FONT_SIZE8x6, EPD_COLOR_BLACK);
    this->_epd_instance->epd_paint_showString(211, 15, (uint8_t *)"    mV", EPD_FONT_SIZE8x6, EPD_COLOR_BLACK);
}

void EPD_UPDATE::display_level_box()
{
    this->_epd_instance->epd_paint_drawLine(225, 1, 249, 1, EPD_COLOR_BLACK);   // superior
    this->_epd_instance->epd_paint_drawLine(225, 11, 249, 11, EPD_COLOR_BLACK); // inferior
    this->_epd_instance->epd_paint_drawLine(224, 0, 224, 11, EPD_COLOR_BLACK);  // esquerda
    this->_epd_instance->epd_paint_drawLine(249, 0, 249, 11, EPD_COLOR_BLACK);  // direita
    this->_epd_instance->epd_paint_drawLine(223, 3, 223, 9, EPD_COLOR_BLACK);   // positivo1
    this->_epd_instance->epd_paint_drawLine(222, 3, 222, 9, EPD_COLOR_BLACK);   // positivo2
}

void EPD_UPDATE::display_voltage_box()
{
    this->_epd_instance->epd_paint_drawLine(210, 13, 249, 13, EPD_COLOR_BLACK); // superior
    this->_epd_instance->epd_paint_drawLine(210, 24, 249, 24, EPD_COLOR_BLACK); // inferior
    this->_epd_instance->epd_paint_drawLine(209, 13, 209, 24, EPD_COLOR_BLACK); // esquerda
    this->_epd_instance->epd_paint_drawLine(249, 13, 249, 24, EPD_COLOR_BLACK); // direita
    this->_epd_instance->epd_paint_drawLine(208, 16, 208, 22, EPD_COLOR_BLACK); // positivo1
    this->_epd_instance->epd_paint_drawLine(207, 16, 207, 22, EPD_COLOR_BLACK); // positivo2
}

void EPD_UPDATE::display_connections(DeviceInfo *device_info)
{
    this->_device_info = device_info;

    wakeup();
    char buffer[22];
    sprintf(buffer, "%s", this->_device_info->IPstring); // Formata a temperatura
    this->_epd_instance->epd_paint_showString(0, 113, (uint8_t *)"                    ", EPD_FONT_SIZE8x6, EPD_COLOR_BLACK);
    this->_epd_instance->epd_displayBW_partial(this->image_bw);
    this->_epd_instance->epd_paint_showString(0, 113, (uint8_t *)buffer, EPD_FONT_SIZE8x6, EPD_COLOR_BLACK);
    this->_epd_instance->epd_displayBW_partial(this->image_bw);

    sprintf(buffer, "SSID:%s", this->_device_info->wifi_ssid); // Formata a temperatura
    this->_epd_instance->epd_paint_showString(129, 113, (uint8_t *)"                    ", EPD_FONT_SIZE8x6, EPD_COLOR_BLACK);
    this->_epd_instance->epd_displayBW_partial(this->image_bw);
    this->_epd_instance->epd_paint_showString(129, 113, (uint8_t *)buffer, EPD_FONT_SIZE8x6, EPD_COLOR_BLACK);
    this->_epd_instance->epd_displayBW_partial(this->image_bw);
    sleep();
    this->_device_info = nullptr;
}

void EPD_UPDATE::display_lines()
{
    this->_epd_instance->epd_paint_drawLine(128, 27, 128, 122, EPD_COLOR_BLACK);
    this->_epd_instance->epd_paint_drawLine(1, 111, 249, 111, EPD_COLOR_BLACK);
    this->_epd_instance->epd_paint_drawLine(1, 26, 249, 26, EPD_COLOR_BLACK);

    this->_epd_instance->epd_paint_drawLine(1, 44, 249, 44, EPD_COLOR_BLACK);
    this->_epd_instance->epd_paint_drawLine(1, 63, 249, 63, EPD_COLOR_BLACK);
    this->_epd_instance->epd_paint_drawLine(1, 82, 128, 82, EPD_COLOR_BLACK);
}
void EPD_UPDATE::display_make_lines()
{
    wakeup();
    this->_epd_instance->epd_paint_clear(EPD_COLOR_WHITE);
    display_lines();
    display_level_box();
    display_voltage_box();
    displayUpdate();
    sleep();
}
void EPD_UPDATE::sleep()
{

    this->_epd_instance->sleep(EPD_DEEPSLEEP_MODE1);
}

void EPD_UPDATE::wakeup()
{
    this->_epd_instance->init();
}

void EPD_UPDATE::updatingfw(char *msg)
{
    wakeup();
    this->_epd_instance->epd_paint_showString(10, 113, (uint8_t *)msg, EPD_FONT_SIZE24x12, EPD_COLOR_BLACK);
    displayUpdate();

    sleep();
}

// x=0
// x=27
// x=46
// x=65
// x=84
// x=103
