
#include "esp_init.hpp"
#include "connections.h"

/*******DEV-LIB-THINGS**********/
#include "ADC_control/adc.hpp"
/**********************/

extern "C"
{
    void app_main(void);
}

void app_main()
{
    init();
    create_sleep_timer(30);
    ESP_LOGI("Version", "%s", FIRMWARE_VERSION);

    switch (wakeup_cause())
    {
    case WAKEUP_TOUCHPAD:
        ESP_LOGI("ESP-WAKE-UP", "FROM Touch");
        ////create_sleep_timer(0);
        break;
    case WAKEUP_TIMER:
        ESP_LOGI("ESP-WAKE-UP", "FROM TIMER");
        init_fom_timer();
        vTaskDelay(1 * PORT_TICK_PERIOD_SECONDS);
        create_sleep_timer(0);

        break;
    case WAKEUP_BT:
        ESP_LOGI("ESP-WAKE-UP", "WAKEUP_BT");
        // create_sleep_timer(0);
        break;
    case WAKEUP_EXT0:
        ESP_LOGI("ESP-WAKE-UP", "WAKEUP_EXT0");
        // create_sleep_timer(0);
        break;
    case WAKEUP_EXT1:
        ESP_LOGI("ESP-WAKE-UP", "WAKEUP_EXT1");
        // create_sleep_timer(0);
        break;
    case WAKEUP_ULP:
        ESP_LOGI("ESP-WAKE-UP", "WAKEUP_ULP");
        // create_sleep_timer(0);
        break;
    case WAKEUP_GPIO:
        ESP_LOGI("ESP-WAKE-UP", "WAKEUP_GPIO");

        // create_sleep_timer(0);
        break;
    case WAKEUP_UART:
        ESP_LOGI("ESP-WAKE-UP", "WAKEUP_UART");
        // create_sleep_timer(0);
        break;
    case WAKEUP_WIFI:
        ESP_LOGI("ESP-WAKE-UP", "WAKEUP_WIFI");
        // create_sleep_timer(0);
        break;
    case WAKEUP_COCPU:
        ESP_LOGI("ESP-WAKE-UP", "WAKEUP_COCPU");
        // create_sleep_timer(0);
        break;
    case WAKEUP_ALL:
        ESP_LOGI("ESP-WAKE-UP", "WAKEUP_ALL");
        // create_sleep_timer(0);
        break;
    case WAKEUP_UNDEFINED:
        ESP_LOGI("ESP-WAKE-UP", "WAKEUP_UNDEFINED");
        create_sleep_timer(10);
        save_nvs_string_var(MODEL, DEFAULT_MODEL);
        save_nvs_u32_var(TIME_TO_WAKE_UP, 30);
        save_nvs_string_var(WIFISSID, (char *)"Unconnected");
        save_nvs_string_var(WIFI_IP, (char *)"Unconnected");
        init_restarted();

        break;

    default:
        ESP_LOGI("ESP-WAKE-UP", "RESET ERROR %d", wakeup_cause());
        init_restarted();
        create_sleep_timer(0);

        // create_sleep_timer(0);
        break;
    }
    while (true)
    {
        vTaskDelay(3 * PORT_TICK_PERIOD_SECONDS);
    }
}
