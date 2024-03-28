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
        ESP_LOGI("WAKEUP", "WAKE");

        deisolate_gpio();
    }
    int ota_status = read_nvs_int8_var(OTA_STATUS_SUCCESS);
    if (ota_status != 0 && ota_status != 1)
    {
        save_nvs_int8_var(OTA_STATUS_SUCCESS, 1);
    }
    if (ota_status)
    {
        save_nvs_int8_var(UPDATE_STATUS, false);
        save_nvs_int8_var(OTA_STATUS_SUCCESS, false);
        ESP_ERROR_CHECK(esp_ota_mark_app_valid_cancel_rollback());
        ESP_LOGI("OtaUpdate", "Atualização bem-sucedida, configurando a nova partição como válida de boot");
    }
    else
    {
        ESP_LOGI("OtaUpdate", "No OTA");
    }

    int init_counter = read_nvs_int8_var(INIT_COUNTER);
    if (init_counter <= 0 && init_counter >= INIT_COUNTER_NUMBER + 1)
    {
        save_nvs_int8_var(INIT_COUNTER, 0);
    }

    int updateStatus = read_nvs_int8_var(UPDATE_STATUS);
    if (updateStatus != 0 && updateStatus != 1)
    {
        save_nvs_int8_var(UPDATE_STATUS, 0);
    }

    if (read_nvs_int8_var(UPDATE_STATUS))
    {
        ESP_LOGI("OTA-STATUS", "UpdateStatus true");
        otaInit();
        while (true)
        {
            vTaskDelay(5 * PORT_TICK_PERIOD_SECONDS);
        }
    }
    device_info_raw = new DeviceInfo;

    init_i2c();
    ESP_LOGI("init_i2c", "Init");
    init_SPI();
    ESP_LOGI("init_SPI", "Init");
    init_epDisplay();
    ESP_LOGI("init_epDisplay", "Init");
}
void init_restarted()
{
    ESP_LOGI("init_restarted", "Init");
    epd_update->display_make_lines();
}
void init_fom_timer(void)
{

    int init_counter = read_nvs_int8_var(INIT_COUNTER);
    if (init_counter >= INIT_COUNTER_NUMBER)
    {
        save_nvs_int8_var(INIT_COUNTER, 0);
        battery_things();
        generate_client_ID();
        epd_update->display_make();
        init_all_data();
        ESP_LOGI("init_fom_timer", "init_all_data");
    }
    else
    {
        battery_things();
        generate_client_ID();
        epd_update->display_make();
        capture_data();

        ESP_LOGI("init_fom_timer", "init_only_display Counter:%d", init_counter);
        init_counter++;
        save_nvs_int8_var(INIT_COUNTER, init_counter);
    }
}
void init_all_data(void)
{
    capture_data();

    tryConnectToWiFi();

    vTaskDelay(1 * PORT_TICK_PERIOD_SECONDS);

    if (wifi->isConnected())
    {
        esp_ip4_addr_t ip = wifi->getIP();
        ESP_LOGI("WIFI-STATUS", "Connected at IP: %d.%d.%d.%d", IP2STR(&ip));
        mqtt_initialize->connect();
        mqtt_initialize->subscribe();
        mqtt_initialize->publish_full_data();
    }
    else
    {
        save_nvs_string_var(WIFISSID, (char *)"Unconnected");
        save_nvs_string_var(WIFI_IP, (char *)"Unconnected");
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
    save_nvs_string_var(DEWPOINT, convert_value_to_string(i2cDewPoint));
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
    device_info_raw->IPstring = read_nvs_string_var(WIFI_IP);
    device_info_raw->wifi_ssid = read_nvs_string_var(WIFISSID);
    device_info_raw->charged = read_nvs_int8_var(BATTERY_CHARGED_STATUS);
    device_info_raw->charging = read_nvs_int8_var(BATTERY_CHARGING_STATUS);

    epd_update->display_partial(device_info_raw);
}

void tryConnectToWiFi()
{
    if (!wifi->isConnected())
    {
        wifi = new WiFiManager();
        mqtt_initialize = new PROTOCOL::MqttInit();
        const size_t wifiCredentialsCount = sizeof(wifiCredentials) / sizeof(wifiCredentials[0]);

        for (int i = 0; i < wifiCredentialsCount; i++)
        {
            wifi->connect(wifiCredentials[i].ssid, wifiCredentials[i].password);
            int attempt = 0;
            while (!wifi->isConnected() && attempt < 6)
            {
                ESP_LOGI("WIFI-STATUS", "Attempting to connect to %s", wifiCredentials[i].ssid);
                vTaskDelay(500 * portTICK_PERIOD_MS);
                attempt++;
            }
            if (wifi->isConnected())
            {
                char buffer[20];
                esp_ip4_addr_t ip = wifi->getIP();
                ESP_LOGI("WIFI-STATUS", "Connected to %s", wifiCredentials[i].ssid);
                sprintf(buffer, "IP:%d.%d.%d.%d", IP2STR(&ip));
                save_nvs_string_var(WIFISSID, (char *)wifiCredentials[i].ssid);
                save_nvs_string_var(WIFI_IP, buffer);

                return; // Conexão bem-sucedida, sai da função
            }

            wifi->disconnect();
        }
    }

    save_nvs_string_var(WIFI_IP, (char *)"Unconnected");
    save_nvs_string_var(WIFISSID, (char *)"Unconnected");
    ESP_LOGE("WIFI-STATUS", "Failed to connect to any network");
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
    bat_mv = bat_status.battery_read(AVERAGE_BATTERY_COUNTER);
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
        epd_update = new EPD_UPDATE(epd_display, EPD_COLOR_WHITE, EPD_COLOR_BLACK);
    }
    ESP_LOGI("EPD-INIT", "OK");
}
void init_i2c(void)
{
    i2c = new PROTOCOL::I2c(I2C_NUM_0);
    i2c->InitMaster(SDA_PIN, SCL_PIN, I2C_CLK_SPEED_HZ, true, true);
    ESP_LOGI("I2C-INIT", "OK");
}

void init_SPI(void)
{
    spi = new PROTOCOL::Spi;
    spi->Init(SPI2_HOST, SPI_MISO, SPI_SDA, SPI_SCL);
    spi->RegisterDevice(MODE, SPI_CS, ADDR_LENGTH, SPI_DataSize, CLOCK_SPEED);
    ESP_LOGI("SPI-INIT", "OK");
}

void otaInit(void)
{
    tryConnectToWiFi();

    if (wifi->isConnected())
    {
        esp_ip4_addr_t ip = wifi->getIP();
        ESP_LOGI("WIFI-STATUS", "Connected at IP: %d.%d.%d.%d", IP2STR(&ip));
        init_SPI();
        ESP_LOGI("init_SPI", "Init");
        init_epDisplay();
        ESP_LOGI("init_epDisplay", "Init");
        OtaUpdate otaUpdater;
        create_sleep_timer(120);
        epd_update->updatingfw(OTA_MSG_UPDATE);
        char *url = nullptr;
        char *crc = nullptr;
        parseJson(read_nvs_string_var(OTA_URL), &url, &crc);
        esp_err_t err = otaUpdater.start(url, crc);
        if (err == ESP_OK)
        {
            save_nvs_int8_var(UPDATE_STATUS, false);
            epd_update->updatingfw(OTA_MSG_SUCCESS);
            vTaskDelay(1 * PORT_TICK_PERIOD_SECONDS);
            esp_restart();
        }
        else
        {
            save_nvs_int8_var(UPDATE_STATUS, false);
            epd_update->updatingfw(OTA_MSG_FAIL);
            esp_restart();
        }

        vTaskDelay(1 * PORT_TICK_PERIOD_SECONDS);
    }
}

void parseJson(const char *jsonString, char **url, char **crc)
{
    cJSON *json = cJSON_Parse(jsonString);
    if (json == nullptr)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != nullptr)
        {
            ESP_LOGI("JSONERROR", "Error before: %s\n", error_ptr);
            save_nvs_int8_var(UPDATE_STATUS, false);
            epd_update->updatingfw(OTA_MSG_FAIL);
            esp_restart();
        }
        return;
    }

    const cJSON *urlJson = cJSON_GetObjectItemCaseSensitive(json, "url");
    const cJSON *crcJson = cJSON_GetObjectItemCaseSensitive(json, "crc");

    if (cJSON_IsString(urlJson) && (urlJson->valuestring != nullptr))
    {
        *url = strdup(urlJson->valuestring);
        ESP_LOGI("URL_parse", "%s", *url);
    }

    if (cJSON_IsString(crcJson) && (crcJson->valuestring != nullptr))
    {
        *crc = strdup(crcJson->valuestring);
        ESP_LOGI("CRC_parse", "%s", *crc);
    }

    cJSON_Delete(json);
}