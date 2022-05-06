/* Basic ESPNOW Example - Slave

   This is a very basic example of sending data to the complementary Master example.
   By default, it sends an unencrypted data packet containing a random value and the state of a button to the broadcast mac address.

   To customise for your data:
    - Alter my_data_t in espnow_basic_config
    - Alter my_data_receive my_master.c
    - Alter my_data_populate my_slave.c

   For basic demonstration, do not edit this file
*/

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "espnow_basic_config.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

//ADC Channels
#define ADC1_CHAN1          ADC1_CHANNEL_4
#define ADC1_CHAN2          ADC1_CHANNEL_5
#define ADC1_CHAN3          ADC1_CHANNEL_6
#define ADC1_CHAN4          ADC1_CHANNEL_7
#define ADC1_CHAN5          ADC1_CHANNEL_3
//ADC Attenuation
#define ADC_EXAMPLE_ATTEN           ADC_ATTEN_DB_11
//ADC Calibration
#define ADC_EXAMPLE_CALI_SCHEME     ESP_ADC_CAL_VAL_EFUSE_VREF

#define SSID                "NhatHoang"
#define PASS                "01217818548"
#define MY_ESPNOW_WIFI_MODE WIFI_MODE_STA
#define MY_ESPNOW_WIFI_IF   ESP_IF_WIFI_STA

static int adc_raw[5];
static esp_adc_cal_characteristics_t adc1_chars;

static const char *TAG = "ESP32-RF-REMOTE";
static EventGroupHandle_t s_evt_group;

///ADC
static bool adc_calibration_init(void);
void adc_task(void *pvParameters);

void my_data_populate(my_data_t *data);
//ADC
static bool adc_calibration_init(void)
{
    esp_err_t ret;
    bool cali_enable = false;

    ret = esp_adc_cal_check_efuse(ADC_EXAMPLE_CALI_SCHEME);
    if (ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW("ADC_CALIB", "Calibration scheme not supported, skip software calibration");
    } else if (ret == ESP_ERR_INVALID_VERSION) {
        ESP_LOGW("ADC_CALIB", "eFuse not burnt, skip software calibration");
    } else if (ret == ESP_OK) {
        cali_enable = true;
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_EXAMPLE_ATTEN, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
    } else {
        ESP_LOGE("ADC_CALIB", "Invalid arg");
    }

    return cali_enable;
}

void adc_task(void *pvParameters)
{
    uint32_t voltage = 0;
    bool cali_enable = adc_calibration_init();
    //ADC1 config
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHAN4, ADC_EXAMPLE_ATTEN));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHAN1, ADC_EXAMPLE_ATTEN));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHAN2, ADC_EXAMPLE_ATTEN));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHAN3, ADC_EXAMPLE_ATTEN));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHAN5, ADC_EXAMPLE_ATTEN));
    while (1) {
        adc_raw[0]= adc1_get_raw(ADC1_CHAN1);
        //ESP_LOGI("ADC1_CHAN1 - LY", "raw  data: %d", adc_raw[0]);
        if (cali_enable) {
            voltage = esp_adc_cal_raw_to_voltage(adc_raw[0], &adc1_chars);
            //ESP_LOGI("ADC1_CHAN1 - LY", "cali data: %d mV", voltage);
        }
        adc_raw[1]= adc1_get_raw(ADC1_CHAN2);
        //ESP_LOGI("ADC1_CHAN2 - RX ", "raw  data: %d", adc_raw[1]);
        if (cali_enable) {
            voltage = esp_adc_cal_raw_to_voltage(adc_raw[1], &adc1_chars);
            //ESP_LOGI("ADC1_CHAN2 - RX ", "cali data: %d mV", voltage);
        }
        adc_raw[2]= adc1_get_raw(ADC1_CHAN3);
        //ESP_LOGI("ADC1_CHAN3 - TRIM1", "raw  data: %d", adc_raw[2]);
        if (cali_enable) {
            voltage = esp_adc_cal_raw_to_voltage(adc_raw[2], &adc1_chars);
            //ESP_LOGI("ADC1_CHAN3 - TRIM1", "cali data: %d mV", voltage);
        }
        adc_raw[3]= adc1_get_raw(ADC1_CHAN4);
        //ESP_LOGI("ADC1_CHAN4 - TRIM2", "raw  data: %d", adc_raw[3]);
        if (cali_enable) {
            voltage = esp_adc_cal_raw_to_voltage(adc_raw[3], &adc1_chars);
            //ESP_LOGI("ADC1_CHAN4 - TRIM2", "cali data: %d mV", voltage);
        }
        adc_raw[4]= adc1_get_raw(ADC1_CHAN5);
        //ESP_LOGI("ADC1_CHAN4 - TRIM2", "raw  data: %d", adc_raw[3]);
        if (cali_enable) {
            voltage = esp_adc_cal_raw_to_voltage(adc_raw[4], &adc1_chars);
            //ESP_LOGI("ADC1_CHAN4 - TRIM2", "cali data: %d mV", voltage);
        }

        ESP_LOGI("ADC","LY=%d--RX=%d--TRIM1=%d--TRIM2=%d--TRIM3=%d", adc_raw[0],adc_raw[1],adc_raw[2],adc_raw[3],adc_raw[4]);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
static void packet_sent_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    assert(status == ESP_NOW_SEND_SUCCESS || status == ESP_NOW_SEND_FAIL);
    xEventGroupSetBits(s_evt_group, BIT(status));
}

static void init_espnow_slave(void)
{
    const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {
    .sta = {
        .ssid = SSID,
        .password = PASS,
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
    },
    };
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    ESP_ERROR_CHECK( esp_netif_init() );
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(MY_ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_LOGI("Wifi", "wifi_init_sta finished.");
    ESP_ERROR_CHECK( esp_wifi_connect());
    ESP_LOGI("Wifi", "esp_wifi_connect finished.");
#if MY_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(MY_ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(packet_sent_cb) );
    ESP_ERROR_CHECK( esp_now_set_pmk((const uint8_t *)MY_ESPNOW_PMK) );

    // Alter this if you want to specify the gateway mac, enable encyption, etc
    const esp_now_peer_info_t broadcast_destination = {
        .peer_addr = MY_RECEIVER_MAC,
        .channel = MY_ESPNOW_CHANNEL,
        .ifidx = MY_ESPNOW_WIFI_IF
    };
    ESP_ERROR_CHECK( esp_now_add_peer(&broadcast_destination) );
}

static esp_err_t send_espnow_data(void)
{
    const uint8_t destination_mac[] = MY_RECEIVER_MAC;
    static my_data_t data;

    // Go to the user function to populate the data to send
    //my_data_populate(&data);
    data.LY=adc_raw[0];
    data.RX=adc_raw[1];
    data.TRIM1=adc_raw[2];
    data.TRIM2=adc_raw[3];
    data.TRIM3=adc_raw[4];

    // Send it
    ESP_LOGI(TAG, "Sending %u bytes to " MACSTR, sizeof(data), MAC2STR(destination_mac));
    esp_err_t err = esp_now_send(destination_mac, (uint8_t*)&data, sizeof(data));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Send error (%d)", err);
        return ESP_FAIL;
    }

    // Wait for callback function to set status bit
    EventBits_t bits = xEventGroupWaitBits(s_evt_group, BIT(ESP_NOW_SEND_SUCCESS) | BIT(ESP_NOW_SEND_FAIL), pdTRUE, pdFALSE, 2000 / portTICK_PERIOD_MS);
    if ( !(bits & BIT(ESP_NOW_SEND_SUCCESS)) )
    {
        if (bits & BIT(ESP_NOW_SEND_FAIL))
        {
            ESP_LOGE(TAG, "Send error");
            return ESP_FAIL;
        }
        ESP_LOGE(TAG, "Send timed out");
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGI(TAG, "Sent!");
    return ESP_OK;
}

void app_main(void)
{
    s_evt_group = xEventGroupCreate();
    assert(s_evt_group);

    init_espnow_slave();
    xTaskCreate(&adc_task, "adc_task", 4096, NULL, 2, NULL);

    while(1){
        send_espnow_data();
        vTaskDelay(10/ portTICK_PERIOD_MS);
    }

    //esp_deep_sleep(1000ULL * MY_SLAVE_DEEP_SLEEP_TIME_MS);
}