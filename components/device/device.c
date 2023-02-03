#include <stdio.h>
#include "device.h"

#define TAG "DEVICE"

esp_err_t device_get_pin_level(int pin, uint8_t *level){
    *level = gpio_get_level(pin);
    
    ESP_LOG(INFO, TAG, "reading pin %d level %d", pin, *level);
    return ESP_OK;
}

esp_err_t device_set_pin_level(int pin, uint8_t level) {
    ESP_LOG(INFO, TAG,"setting pin %d to level %d", pin, level);
    if (gpio_set_level(pin, level) != ESP_OK){
        ESP_LOG(ERROR, TAG, "error occured");
        return ESP_FAIL;
    }
    return ESP_OK;
}

