/*
    Final year project
    Vladislavas Putys
*/
/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */


#include "common_components.h"
#include "device.h"

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_err.h"
#include "driver/gpio.h"

#define TEST_PIN 34

void main_restart_esp();

void main_restart_esp() {

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

void app_main(void)
{
    printf("This is Vlads Final Year Project!̣\n");
    printf("Test val: %d\n", TEST_VALUE);


    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    bool reboot_reqested = false;
    uint8_t level = 15;
    gpio_num_t num = GPIO_NUM_34;
    gpio_mode_t mode = GPIO_MODE_INPUT;

    gpio_set_direction(num, mode);

    printf("Entering main loop\n");
    while(1) {

        
        if (ESP_OK == device_get_pin_level(num, &level)){
            printf("current level %d\n", level);
        }
        
        if (reboot_reqested){
            main_restart_esp();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    


}
