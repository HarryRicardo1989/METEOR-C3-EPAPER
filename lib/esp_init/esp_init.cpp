#include "esp_init.hpp"
#include "esp_cleanup.h"

WiFiManager *wifi = nullptr;
PROTOCOL::MqttInit *mqtt_initialize = nullptr;
PROTOCOL::I2c *i2c = nullptr;
PROTOCOL::Spi *spi = nullptr;
EpaperDisplay *epd_display = nullptr;
EPD_UPDATE *epd_update = nullptr;
DeviceInfo *device_info_raw = nullptr;

uint8_t image_bw[EPD_W_BUFF_SIZE * EPD_H];

void init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    if (esp_reset_reason() == ESP_RST_DEEPSLEEP)
    {
        printf("\n\nACORDEI\n\n");

        deisolate_gpio();
    }
    int updateStatus = read_nvs_int8_var(UPDATE_STATUS);
    if (updateStatus != 0 && updateStatus != 1)
    {
        save_nvs_int8_var(UPDATE_STATUS, 0);
    }

    if (read_nvs_int8_var(UPDATE_STATUS))
    {
        ESP_LOGW("OTA-STATUS", "UpdateStatus true");
        otaInit();
        while (true)
        {
            vTaskDelay(5 * PORT_TICK_PERIOD_SECONDS);
        }
    }
    device_info_raw = new DeviceInfo;

    init_i2c();
    ESP_LOGW("init_i2c", "Init");
    init_SPI();
    ESP_LOGW("init_SPI", "Init");
    init_epDisplay();
    ESP_LOGW("init_epDisplay", "Init");
}
void init_restarted()
{
    ESP_LOGW("init_restarted", "Init");

    epd_update->display_make_lines();
    epd_update->display_make();
    // epd_update->display_connections(device_info_raw);
    init_routines();
}
void init_fom_timer(void)
{
    init_routines();
}
void init_routines(void)
{
    battery_things();
    generate_client_ID();
    capture_data();

    // tryConnectToWiFi();
    vTaskDelay(1 * PORT_TICK_PERIOD_SECONDS);

    if (wifi->isConnected())
    {
        esp_ip4_addr_t ip = wifi->getIP();
        ESP_LOGW("WIFI-STATUS", "Connected at IP: %d.%d.%d.%d", IP2STR(&ip));
        mqtt_initialize->connect();
        blink_led_custom(0, 100, 0, 20, 50, 1);
        mqtt_initialize->subscribe();
        blink_led_custom(100, 100, 100, 20, 50, 3);
        // mqtt_task_pub_init();
    }
}
void capture_data(void)
{

    CPPBME280::BME280 bme280i2c;
    bme280i2c.InitI2c(i2c, 0x76);
    bme280i2c.Init();
    bme280i2c.SetMode(3);
    bme280i2c.SetConfigFilter(1);
    float i2cTemperature;
    float i2cPressure;
    float i2cDewPoint;
    int i2cHumidity;
    float i2cAltitude;
    // int i2cId;
    // i2cId = bme280i2c.GetDeviceID();
    bme280i2c.GetAllResults(&i2cTemperature, &i2cHumidity, &i2cPressure, &i2cDewPoint, &i2cAltitude);
    int bat_level = read_nvs_int8_var(BATTERY_PERCENT_VALUE);
    uint32_t bat_mv = read_nvs_uint32_var(BATTERY_VALUE);
    save_nvs_string_var(TEMPERATURE, convert_float_to_string(i2cTemperature));
    save_nvs_string_var(HUMIDITY, convert_value_to_string(i2cHumidity));
    save_nvs_string_var(PRESSURE, convert_float_to_string(i2cPressure));
    save_nvs_string_var(ALTITUDE, convert_float_to_string(i2cAltitude));

    device_info_raw->Altitude = i2cAltitude;
    device_info_raw->Temperature = i2cTemperature;
    device_info_raw->DewPoint = i2cDewPoint;
    device_info_raw->Humidity = i2cHumidity;
    device_info_raw->Pressure = i2cPressure;
    device_info_raw->bat_mv = bat_mv;
    device_info_raw->bat_level = bat_level;
    device_info_raw->Pressure = i2cPressure;
    epd_update->display_partial(device_info_raw);

    if (wifi->isConnected())
    {
        char buffer[20];
        esp_ip4_addr_t ip = wifi->getIP();
        sprintf(buffer, "IP:%d.%d.%d.%d", IP2STR(&ip));
        device_info_raw->IPstring = buffer;
        device_info_raw->wifi_ssid = read_nvs_string_var(WIFISSID);
    }
    else
    {
        device_info_raw->IPstring = (char *)"Unconnected";
        device_info_raw->wifi_ssid = (char *)"Unconnected";
    }
    delete device_info_raw;
    device_info_raw = nullptr;
}

void scanI2CDevices(int sdaPin, int sclPin)
{
    printf("Scan init...\n");
    printf("Scan prepare...\n");

    printf("Scanning I2C bus...\n");
    for (uint8_t address = 1; address < 0x7F; address++)
    {

        if (i2c->testConnection(address))
        {
            printf("I2C device found at address 0x%2X \n", address);
        }
    }
}

void tryConnectToWiFi()
{
    wifi = new WiFiManager();
    mqtt_initialize = new PROTOCOL::MqttInit();
    const size_t wifiCredentialsCount = sizeof(wifiCredentials) / sizeof(wifiCredentials[0]);

    for (int i = 0; i < wifiCredentialsCount; i++)
    {
        wifi->connect(wifiCredentials[i].ssid, wifiCredentials[i].password);
        int attempt = 0;
        while (!wifi->isConnected() && attempt < 4)
        {
            ESP_LOGW("WIFI-STATUS", "Attempting to connect to %s", wifiCredentials[i].ssid);
            vTaskDelay(30 * portTICK_PERIOD_MS);
            device_info_raw->IPstring = (char *)"Unconnected";
            device_info_raw->wifi_ssid = (char *)"Unconnected";
            attempt++;
        }
        if (wifi->isConnected())
        {
            ESP_LOGI("WIFI-STATUS", "Connected to %s", wifiCredentials[i].ssid);
            save_nvs_string_var(WIFISSID, (char *)wifiCredentials[i].ssid);
            char buffer[20];
            esp_ip4_addr_t ip = wifi->getIP();
            sprintf(buffer, "IP:%d.%d.%d.%d", IP2STR(&ip));
            device_info_raw->IPstring = buffer;
            epd_update->display_connections(device_info_raw);

            return; // Conexão bem-sucedida, sai da função
        }

        wifi->disconnect();
    }

    ESP_LOGW("WIFI-STATUS", "Failed to connect to any network");
}
void generate_client_ID(void)
{
    char clientID_raw[30];
    char mac_str[18];
    char *clientID;
    char *mac_address;
    uint8_t raw_mac[6];

    esp_read_mac(raw_mac, ESP_MAC_WIFI_STA);

    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", raw_mac[0], raw_mac[1], raw_mac[2], raw_mac[3], raw_mac[4], raw_mac[5]);
    sprintf(clientID_raw, "METEOR-%02X:%02X:%02X:%02X:%02X:%02X", raw_mac[0], raw_mac[1], raw_mac[2], raw_mac[3], raw_mac[4], raw_mac[5]);
    mac_address = mac_str;
    clientID = clientID_raw;
    save_nvs_string_var(CLIENT_ID, clientID);
    save_nvs_string_var(MAC_ADDRESS, mac_address);
}
extern "C"
{
    void cleanup()
    {
        if (wifi != nullptr)
        {
            delete wifi;
            wifi = nullptr;
        }

        if (mqtt_initialize != nullptr)
        {
            delete mqtt_initialize;
            mqtt_initialize = nullptr;
        }
        if (i2c != nullptr)
        {

            delete i2c;
            i2c = nullptr;
        }
        if (spi != nullptr)
        {

            delete spi;
            spi = nullptr;
        }
        if (epd_display != nullptr)
        {

            delete epd_display;
            epd_display = nullptr;
        }
        if (epd_display != nullptr)
        {

            delete epd_update;
            epd_update = nullptr;
        }
        if (device_info_raw != nullptr)
        {

            delete device_info_raw;
            device_info_raw = nullptr;
        }
    }
}

char *convert_value_to_string(int value)
{
    std::string stringValue = std::to_string(value);
    char *string = new char[stringValue.length() + 1];
    strcpy(string, stringValue.c_str());

    return string;
}

char *convert_float_to_string(float value)
{
    // Utiliza stringstream para controle de formato
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << value; // Limita a 2 casas decimais
    std::string stringValue = stream.str();

    // Aloca memória para o resultado
    char *string = new char[stringValue.length() + 1];
    strcpy(string, stringValue.c_str());

    return string;
}

void battery_things(void)
{
    BATTERY::BatteryStatus bat_status = BATTERY::BatteryStatus();
    uint32_t bat_mv;
    uint8_t bat_level;
    bat_mv = bat_status.battery_read(100);
    bat_level = bat_status.battery_percent(bat_mv);
    bool charged = bat_status.battery_charged();
    bool charging = bat_status.battery_charging();
    save_nvs_u32_var(BATTERY_VALUE, bat_mv);
    save_nvs_int8_var(BATTERY_PERCENT_VALUE, bat_level);
    save_nvs_int8_var(BATTERY_CHARGED_STATUS, charged);
    save_nvs_int8_var(BATTERY_CHARGING_STATUS, charging);
    ESP_LOGI("BATTERY", "%ld", bat_mv);
}
void init_epDisplay(void)
{
    if (epd_display == nullptr)
    {
        epd_display = new EpaperDisplay(spi, DISP_DC, DISP_RES, DISP_BUSY);
        epd_update = new EPD_UPDATE(epd_display);
    }
    ESP_LOGW("EPD-INIT", "OK");
}
void init_i2c(void)
{
    i2c = new PROTOCOL::I2c(I2C_NUM_0);
    i2c->InitMaster(SDA_PIN, SCL_PIN, I2C_CLK_SPEED_HZ, true, true);
    ESP_LOGW("I2C-INIT", "OK");
}

void init_SPI(void)
{
    spi = new PROTOCOL::Spi;
    spi->Init(SPI2_HOST, SPI_MISO, SPI_SDA, SPI_SCL);
    spi->RegisterDevice(MODE, SPI_CS, ADDR_LENGTH, SPI_DataSize, CLOCK_SPEED);
    ESP_LOGW("SPI-INIT", "OK");
}

void display_meteor(float temperature, float pressure, int humidity, float i2cDewPoint, int battery_level, u_int32_t battery_voltage, float altitude)
{
    char buffer[100];
    epd_display->epd_paint_newimage(image_bw, EPD_W, EPD_H, EPD_ROTATE_0, EPD_COLOR_WHITE);
    epd_display->epd_paint_selectimage(image_bw);
    epd_display->epd_paint_clear(EPD_COLOR_WHITE);

    // x=0
    // x=27
    // x=46
    // x=65
    // x=84
    // x=103
    //
    epd_display->epd_paint_showString(20, 0, (uint8_t *)&"METEOR-MOBILE", EPD_FONT_SIZE24x12, EPD_COLOR_BLACK);
    // info esquerda
    sprintf(buffer, "Temp:%.2fC", temperature); // Formata a temperatura
    epd_display->epd_paint_showString(0, 27, (uint8_t *)buffer, EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    sprintf(buffer, "Press:%.2fhPa", pressure); // Formata a pressão
    epd_display->epd_paint_showString(0, 46, (uint8_t *)buffer, EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    sprintf(buffer, "DewP:%.2fC", i2cDewPoint); // Formata o ponto de orvalho
    epd_display->epd_paint_showString(0, 65, (uint8_t *)buffer, EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);

    // info Direita
    sprintf(buffer, "Humid:%d%%", humidity); // Formata a umidade
    epd_display->epd_paint_showString(126, 27, (uint8_t *)buffer, EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);
    sprintf(buffer, "Alt:%.2fm", altitude); // Formata o ponto de altitude
    epd_display->epd_paint_showString(126, 46, (uint8_t *)buffer, EPD_FONT_SIZE16x8, EPD_COLOR_BLACK);

    // battery Percent box
    sprintf(buffer, "%d%%", battery_level); // Formata o nível da bateria
    epd_display->epd_paint_showString(225, 2, (uint8_t *)buffer, EPD_FONT_SIZE8x6, EPD_COLOR_BLACK);
    epd_display->epd_paint_drawLine(225, 1, 249, 1, EPD_COLOR_BLACK);   // superior
    epd_display->epd_paint_drawLine(225, 11, 249, 11, EPD_COLOR_BLACK); // inferior
    epd_display->epd_paint_drawLine(224, 0, 224, 11, EPD_COLOR_BLACK);  // esquerda
    epd_display->epd_paint_drawLine(249, 0, 249, 11, EPD_COLOR_BLACK);  // direita
    epd_display->epd_paint_drawLine(223, 3, 223, 9, EPD_COLOR_BLACK);   // positivo1
    epd_display->epd_paint_drawLine(222, 3, 222, 9, EPD_COLOR_BLACK);   // positivo2

    // battery voltage box
    sprintf(buffer, "4200mV"); // Formata a tensao da bateria
    // sprintf(buffer, "%ldmV", battery_voltage); // Formata a tensao da bateria
    epd_display->epd_paint_showString(211, 15, (uint8_t *)buffer, EPD_FONT_SIZE8x6, EPD_COLOR_BLACK);
    epd_display->epd_paint_drawLine(210, 13, 249, 13, EPD_COLOR_BLACK); // superior
    epd_display->epd_paint_drawLine(210, 24, 249, 24, EPD_COLOR_BLACK); // inferior
    epd_display->epd_paint_drawLine(209, 13, 209, 24, EPD_COLOR_BLACK); // esquerda
    epd_display->epd_paint_drawLine(249, 13, 249, 24, EPD_COLOR_BLACK); // direita
    epd_display->epd_paint_drawLine(208, 16, 208, 22, EPD_COLOR_BLACK); // positivo1
    epd_display->epd_paint_drawLine(207, 16, 207, 22, EPD_COLOR_BLACK); // positivo2

    // lines
    epd_display->epd_paint_drawLine(125, 27, 125, 122, EPD_COLOR_BLACK);
    epd_display->epd_paint_drawLine(1, 26, 249, 26, EPD_COLOR_BLACK);
    // epd_display->epd_paint_drawLine(1, 111, 249, 111, EPD_COLOR_BLACK);

    epd_display->epd_displayBW_partial(image_bw);

    epd_display->sleep(EPD_DEEPSLEEP_MODE1);
}
void otaInit(void)
{
    ESP_LOGW("WIFI-STATUS", "passou 1");
    tryConnectToWiFi();
    ESP_LOGW("WIFI-STATUS", "passou 2");

    if (wifi->isConnected())
    {
        ESP_LOGW("WIFI-STATUS", "passou 3");
        esp_ip4_addr_t ip = wifi->getIP();
        ESP_LOGW("WIFI-STATUS", "Connected at IP: %d.%d.%d.%d", IP2STR(&ip));
        OtaUpdate otaUpdater;
        otaUpdater.start(read_nvs_string_var(OTA_URL));
        vTaskDelay(1 * PORT_TICK_PERIOD_SECONDS);

        esp_restart();
    }
}
