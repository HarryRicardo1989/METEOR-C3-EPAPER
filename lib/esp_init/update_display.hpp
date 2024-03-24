#pragma once
#include "EpaperDisplay.hpp"
#include "esp_log.h"
struct DeviceInfo
{
    float Temperature = 0;
    float Pressure = 0;
    float DewPoint = 0;
    int Humidity = 0;
    float Altitude = 0;
    char *IPstring = (char *)"Unconnected";
    char *wifi_ssid = (char *)"Unconnected";
    int bat_level = 0;
    uint32_t bat_mv = 0;
    bool charging = false;
    bool charged = false;
};

class EPD_UPDATE

{
private:
    EpaperDisplay *_epd_instance;
    DeviceInfo *_device_info;
    uint8_t image_bw[EPD_W_BUFF_SIZE * EPD_H];
    uint16_t _colorBackgr;
    uint16_t _colorLetter;

public:
    EPD_UPDATE(EpaperDisplay *epd_instance, uint16_t colorBackgr, uint16_t colorLetter);
    ~EPD_UPDATE();

    void displayClear();
    void displayUpdate();
    void displayUpdatePartial();
    void display_total(DeviceInfo *device_info);
    void display_partial(DeviceInfo *device_info);

    void display_make();
    void display_level_box();
    void display_voltage_box();
    void display_connections(DeviceInfo *device_info);
    void display_lines();
    void display_make_lines();
    void sleep();
    void wakeup();
    void updatingfw(char *msg);
};
