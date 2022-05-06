#include "esp_system.h"
#include "esp_log.h"

#include "espnow_basic_config.h"

static const char *TAG = "My_Slave";

// Your function to populate a my_data_t to send
void my_data_populate(my_data_t *data)
{
    ESP_LOGI(TAG, "Populating my_data_t");
    data->LY = esp_random();
    data->RX=esp_random();
    data->TRIM1=esp_random();
    data->TRIM2=esp_random();
    data->TRIM3=esp_random();
}