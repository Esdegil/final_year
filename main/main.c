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
#include "logger_service.h"
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

#include "../ESP32_LED_STRIP/components/led_strip/inc/led_strip/led_strip.h"

#define TEST_PIN 34
#define TAG "MAIN"
#define VERSION_NUMBER_X 0
#define VERSION_NUMBER_Y 1

void main_restart_esp();

void main_restart_esp() {

    for (int i = 10; i >= 0; i--) {
        ESP_LOG(WARN, TAG, "Restarting in %d seconds...", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOG(WARN, TAG, "Restarting now.");
    fflush(stdout);
    esp_restart();
}

esp_err_t init_services(){

    ESP_LOG(WARN, TAG, "Initialising services...");

    if (device_init() != ESP_OK){
        ESP_LOG(ERROR, TAG, "Failed to init one of the services. Aborting.");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOG(INFO, TAG,"This is Vlads Final Year Project!̣");
    ESP_LOG(INFO, TAG, "Software version: v%d.%d", VERSION_NUMBER_X, VERSION_NUMBER_Y);


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

    ESP_LOG(INFO, TAG, "Min task size %d", configMINIMAL_STACK_SIZE);

    if (init_services() != ESP_OK){
        reboot_reqested = true;
    }

gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);

#define LED_STRIP_LENGTH 17U
#define LED_STRIP_RMT_INTR_NUM 19U

static struct led_color_t led_strip_buf_1[LED_STRIP_LENGTH];
static struct led_color_t led_strip_buf_2[LED_STRIP_LENGTH];


    struct led_strip_t led_strip = {
    .rgb_led_type = RGB_LED_TYPE_WS2812,
    .rmt_channel = RMT_CHANNEL_1,
    .rmt_interrupt_num = LED_STRIP_RMT_INTR_NUM,
    .gpio = GPIO_NUM_21,
    .led_strip_buf_1 = led_strip_buf_1,
    .led_strip_buf_2 = led_strip_buf_2,
    .led_strip_length = LED_STRIP_LENGTH
};
led_strip.access_semaphore = xSemaphoreCreateBinary();

bool led_init_ok = led_strip_init(&led_strip);

led_strip_set_pixel_rgb(&led_strip, 3, 5, 1, 1);
led_strip_set_pixel_rgb(&led_strip, 5, 1, 7, 1);

led_strip_show(&led_strip);

    uint8_t level = 15;
    gpio_num_t num = GPIO_NUM_34;
    gpio_num_t num2 = GPIO_NUM_32;
    gpio_mode_t mode = GPIO_MODE_INPUT;
    gpio_mode_t mode2 = GPIO_MODE_OUTPUT;

    gpio_set_direction(num, mode);
    gpio_set_direction(num2, mode2);
    device_set_pin_level(num2, 1);

    device_get_pin_level(num, &level);

    ESP_LOG(WARN, TAG, "This is a test message without args.");
    ESP_LOG(ERROR, TAG, "This is a test message with argument: %d", TEST_VALUE);
    ESP_LOG(INFO, TAG, "Another test multiple args %d %d", 99, 23);


    ESP_LOG(WARN, TAG, "Entering main loop");
    while(1) {

        
        /*if (ESP_OK == device_get_pin_level(num, &level)){
            ESP_LOG(DEBUG, TAG, "current level %d", level);
        }*/
        
        if (reboot_reqested){
            main_restart_esp();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    


}
