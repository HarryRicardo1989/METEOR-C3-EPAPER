#include "update_display.hpp"

EPD_UPDATE::EPD_UPDATE(EpaperDisplay *epd_instance, uint16_t colorBackgr, uint16_t colorLetter)
{
    this->_epd_instance = epd_instance;
    this->_epd_instance->epd_paint_newimage(image_bw, EPD_W, EPD_H, EPD_ROTATE_0, EPD_COLOR_WHITE);
    this->_epd_instance->epd_paint_selectimage(this->image_bw);
    this->_epd_instance->init();
    this->_colorBackgr = colorBackgr;
    this->_colorLetter = colorLetter;
    ESP_LOGI("EPD_UPDATE", "Instanciado");
}

EPD_UPDATE
::~EPD_UPDATE()
{
}

void EPD_UPDATE::displayClear()
{
    this->_epd_instance->epd_paint_clear(this->_colorBackgr);
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
    this->_epd_instance->epd_paint_clear(this->_colorBackgr);

    display_make();
    display_level_box();
    display_voltage_box();
    display_lines();
    displayUpdate();
    sleep();
}

void EPD_UPDATE::display_partial(DeviceInfo *device_info)
{
    this->_device_info = device_info;

    wakeup();

    char buffer[40];
    this->_epd_instance->epd_paint_showString(46, 27, (uint8_t *)"    ", EPD_FONT_SIZE16x8, this->_colorLetter);   // Temp
    this->_epd_instance->epd_paint_showString(46, 46, (uint8_t *)"      ", EPD_FONT_SIZE16x8, this->_colorLetter); // Press
    this->_epd_instance->epd_paint_showString(46, 65, (uint8_t *)"    ", EPD_FONT_SIZE16x8, this->_colorLetter);   // DewP

    this->_epd_instance->epd_paint_showString(177, 27, (uint8_t *)"   ", EPD_FONT_SIZE16x8, this->_colorLetter);    // humid
    this->_epd_instance->epd_paint_showString(160, 46, (uint8_t *)"      ", EPD_FONT_SIZE16x8, this->_colorLetter); // alt

    this->_epd_instance->epd_paint_showString(224, 2, (uint8_t *)"   ", EPD_FONT_SIZE8x6, this->_colorLetter);   // bat %
    this->_epd_instance->epd_paint_showString(211, 15, (uint8_t *)"    ", EPD_FONT_SIZE8x6, this->_colorLetter); // bat mv
    this->_epd_instance->epd_paint_showString(195, 2, (uint8_t *)"    ", EPD_FONT_SIZE8x6, this->_colorLetter);  // Charging

    this->_epd_instance->epd_paint_showString(20, 113, (uint8_t *)"                 ", EPD_FONT_SIZE8x6, this->_colorLetter); // IP
    this->_epd_instance->epd_paint_showString(159, 113, (uint8_t *)"              ", EPD_FONT_SIZE8x6, this->_colorLetter);   // SSID
    this->_epd_instance->epd_paint_showString(165, 101, (uint8_t *)"          ", EPD_FONT_SIZE8x6, this->_colorLetter);       // counts

    this->_epd_instance->epd_displayBW_partial(this->image_bw);

    // info Esquerda
    sprintf(buffer, "%.2f", this->_device_info->Temperature); // Formata a temperatura
    this->_epd_instance->epd_paint_showString(48, 27, (uint8_t *)buffer, EPD_FONT_SIZE16x8, this->_colorLetter);
    sprintf(buffer, "%.2f", this->_device_info->Pressure); // Formata a pressão
    this->_epd_instance->epd_paint_showString(48, 46, (uint8_t *)buffer, EPD_FONT_SIZE16x8, this->_colorLetter);
    sprintf(buffer, "%.2f", this->_device_info->DewPoint); // Formata o ponto de orvalho
    this->_epd_instance->epd_paint_showString(48, 65, (uint8_t *)buffer, EPD_FONT_SIZE16x8, this->_colorLetter);

    // info Direita
    sprintf(buffer, "%d", this->_device_info->Humidity); // Formata a umidade
    this->_epd_instance->epd_paint_showString(177, 27, (uint8_t *)buffer, EPD_FONT_SIZE16x8, this->_colorLetter);
    sprintf(buffer, "%.2f", this->_device_info->Altitude); // Formata o ponto de altitude
    this->_epd_instance->epd_paint_showString(160, 46, (uint8_t *)buffer, EPD_FONT_SIZE16x8, this->_colorLetter);
    sprintf(buffer, "%d", this->_device_info->bat_level); // Formata o nível da bateria
    this->_epd_instance->epd_paint_showString(224, 2, (uint8_t *)buffer, EPD_FONT_SIZE8x6, this->_colorLetter);
    sprintf(buffer, "%ld", this->_device_info->bat_mv); // Formata a tensao da bateria
    this->_epd_instance->epd_paint_showString(211, 15, (uint8_t *)buffer, EPD_FONT_SIZE8x6, this->_colorLetter);
    if (_device_info->charging)
    {
        this->_epd_instance->epd_paint_showString(195, 2, (uint8_t *)"CHG+", EPD_FONT_SIZE8x6, this->_colorLetter);
    }
    if (_device_info->charged)
    {
        this->_epd_instance->epd_paint_showString(195, 2, (uint8_t *)"FULL", EPD_FONT_SIZE8x6, this->_colorLetter);
    }

    sprintf(buffer, "%s", this->_device_info->IPstring); // Formata a temperatura
    this->_epd_instance->epd_paint_showString(20, 113, (uint8_t *)buffer, EPD_FONT_SIZE8x6, this->_colorLetter);
    sprintf(buffer, "%s", this->_device_info->wifi_ssid); // Formata a temperatura
    this->_epd_instance->epd_paint_showString(159, 113, (uint8_t *)buffer, EPD_FONT_SIZE8x6, this->_colorLetter);
    sprintf(buffer, "%ld", _device_info->wake_counts);
    this->_epd_instance->epd_paint_showString(165, 101, (uint8_t *)buffer, EPD_FONT_SIZE8x6, this->_colorLetter);

    this->_epd_instance->epd_displayBW_partial(this->image_bw);

    sleep();
    ESP_LOGI("EPD_UPDATE", "display_partial ok");

    return;
}

void EPD_UPDATE::display_make()
{

    wakeup();
    this->_epd_instance->epd_paint_clear(this->_colorBackgr);
    display_level_box();
    display_lines();
    display_voltage_box();
    this->_epd_instance->epd_paint_showString(15, 0, (uint8_t *)&"METEOR-MOBILE", EPD_FONT_SIZE24x12, this->_colorLetter);
    // info esquerda
    this->_epd_instance->epd_paint_showString(2, 27, (uint8_t *)"Temp:      C", EPD_FONT_SIZE16x8, this->_colorLetter);
    this->_epd_instance->epd_paint_showString(2, 46, (uint8_t *)"Press:      hPa", EPD_FONT_SIZE16x8, this->_colorLetter);
    this->_epd_instance->epd_paint_showString(2, 65, (uint8_t *)"DewP:      C", EPD_FONT_SIZE16x8, this->_colorLetter);

    // info Direita
    this->_epd_instance->epd_paint_showString(129, 27, (uint8_t *)"Humid:   %", EPD_FONT_SIZE16x8, this->_colorLetter);
    this->_epd_instance->epd_paint_showString(129, 46, (uint8_t *)"Alt:      mts", EPD_FONT_SIZE16x8, this->_colorLetter);
    this->_epd_instance->epd_paint_showString(224, 2, (uint8_t *)"   %", EPD_FONT_SIZE8x6, this->_colorLetter);
    this->_epd_instance->epd_paint_showString(211, 15, (uint8_t *)"    mV", EPD_FONT_SIZE8x6, this->_colorLetter);
    this->_epd_instance->epd_paint_showString(129, 101, (uint8_t *)"Count:", EPD_FONT_SIZE8x6, this->_colorLetter);
    this->_epd_instance->epd_paint_showString(2, 113, (uint8_t *)"IP:", EPD_FONT_SIZE8x6, this->_colorLetter);
    this->_epd_instance->epd_paint_showString(129, 113, (uint8_t *)"SSID:", EPD_FONT_SIZE8x6, this->_colorLetter);

    this->_epd_instance->epd_displayBW_partial(this->image_bw);
    sleep();
}

void EPD_UPDATE::display_level_box()
{
    this->_epd_instance->epd_paint_drawLine(225, 1, 249, 1, this->_colorLetter);   // superior
    this->_epd_instance->epd_paint_drawLine(225, 11, 249, 11, this->_colorLetter); // inferior
    this->_epd_instance->epd_paint_drawLine(224, 0, 224, 11, this->_colorLetter);  // esquerda
    this->_epd_instance->epd_paint_drawLine(249, 0, 249, 11, this->_colorLetter);  // direita
    this->_epd_instance->epd_paint_drawLine(223, 3, 223, 9, this->_colorLetter);   // positivo1
    this->_epd_instance->epd_paint_drawLine(222, 3, 222, 9, this->_colorLetter);   // positivo2
}

void EPD_UPDATE::display_voltage_box()
{
    this->_epd_instance->epd_paint_drawLine(210, 13, 249, 13, this->_colorLetter); // superior
    this->_epd_instance->epd_paint_drawLine(210, 24, 249, 24, this->_colorLetter); // inferior
    this->_epd_instance->epd_paint_drawLine(209, 13, 209, 24, this->_colorLetter); // esquerda
    this->_epd_instance->epd_paint_drawLine(249, 13, 249, 24, this->_colorLetter); // direita
    this->_epd_instance->epd_paint_drawLine(208, 16, 208, 22, this->_colorLetter); // positivo1
    this->_epd_instance->epd_paint_drawLine(207, 16, 207, 22, this->_colorLetter); // positivo2
}

void EPD_UPDATE::display_lines()
{
    this->_epd_instance->epd_paint_drawRectangle(1, 26, 249, 44, this->_colorLetter, 0);   // Box Temp/ Humid
    this->_epd_instance->epd_paint_drawRectangle(1, 44, 249, 63, this->_colorLetter, 0);   // Box Press / Alt
    this->_epd_instance->epd_paint_drawRectangle(1, 63, 249, 82, this->_colorLetter, 0);   // Box DewP  /
    this->_epd_instance->epd_paint_drawRectangle(1, 82, 249, 98, this->_colorLetter, 0);   // Box       /
    this->_epd_instance->epd_paint_drawRectangle(1, 98, 249, 110, this->_colorLetter, 0);  // Box       / Count
    this->_epd_instance->epd_paint_drawRectangle(1, 110, 249, 122, this->_colorLetter, 0); // Box IP    / SSID

    this->_epd_instance->epd_paint_drawLine(128, 27, 128, 122, this->_colorLetter); // linha vertical
}
void EPD_UPDATE::display_make_lines()
{
    wakeup();
    this->_epd_instance->epd_paint_clear(this->_colorBackgr);
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
    this->_epd_instance->epd_paint_clear(this->_colorBackgr);
    this->_epd_instance->epd_paint_showString(10, 48, (uint8_t *)msg, EPD_FONT_SIZE24x12, this->_colorLetter);
    displayUpdate();

    sleep();
}

// x=0
// x=27
// x=46
// x=65
// x=84
// x=103
