#include <stdio.h>
#include "device.h"


esp_err_t device_get_pin_level(int pin, uint8_t *level){
    *level = gpio_get_level(pin);
    
    printf("DEVICE: reading pin %d level %d\n", pin, *level);
    return ESP_OK;
}

esp_err_t device_set_pin_level(int pin, uint8_t level) {
    gpio_set_level(pin, level);
    return ESP_OK;
}

