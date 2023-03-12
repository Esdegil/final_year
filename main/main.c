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
#include "led_service.h"
#include "chess_engine.h"

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_err.h"
#include "esp_event.h"
#include "driver/gpio.h"



#define TEST_PIN 34
#define TAG "MAIN"
#define VERSION_NUMBER_X 0
#define VERSION_NUMBER_Y 2

//#define LED_TEST

typedef struct local_data {

    esp_event_loop_handle_t handle;

} local_data_t;

static local_data_t local_data;

ESP_EVENT_DEFINE_BASE(TEST_EVENTS);

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

     if (led_service_init() != ESP_OK){
        ESP_LOG(ERROR, TAG, "Failed to init one of the services. Aborting.");
        return ESP_FAIL;
    }

    if (device_init() != ESP_OK){
        ESP_LOG(ERROR, TAG, "Failed to init one of the services. Aborting.");
        return ESP_FAIL;
    }
    if (chess_engine_init() != ESP_OK){
        ESP_LOG(ERROR, TAG, "Failed to init one of the services. Aborting.");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void main_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data){

   /* if (!args){
        ESP_LOG(ERROR, "ARGS are empty. Aborting");
        return;
    }

    int test = (int*)args;
 */
    ESP_LOG(ERROR, TAG, "A MESSAGE IN EVENT HANDLER");

    ESP_LOG(WARN, TAG, "//////////////////////Event %ld occured.", id);


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

    // TESTING EVENTS

    esp_event_loop_args_t loop_without_args = {
        .queue_size = 5,
        .task_name = NULL
    };

    esp_err_t ret = esp_event_loop_create(&loop_without_args, &local_data.handle);
    ESP_LOG(ERROR, TAG,"Error code: %d", ret);



    //ret = esp_event_handler_register_with(local_data.handle, TEST_EVENTS, EVENT_MATRIX_SWITCH_CLOSED, main_event_handler, NULL ); // TODO: finish this    

    ret = esp_event_handler_register(TEST_EVENTS, EVENT_MATRIX_SWITCH_CLOSED, main_event_handler, NULL);

    ESP_LOG(ERROR, TAG, "Error code: %d", ret);

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


    ESP_LOG(WARN, TAG, "Entering main loop. Posting test event");

    if (local_data.handle == NULL){
        ESP_LOG(ERROR, TAG, "Handle is null");
    }

    if (esp_event_post_to(local_data.handle, TEST_EVENTS, EVENT_MATRIX_SWITCH_CLOSED, NULL, 0, portMAX_DELAY) != ESP_OK) {
        ESP_LOG(ERROR, TAG, "Failed to post event");
    }

    while(1) {

        ret = esp_event_post(TEST_EVENTS, EVENT_MATRIX_SWITCH_CLOSED, NULL, 0, portMAX_DELAY); 
        if (ret == ESP_OK){
            ESP_LOG(INFO, TAG, "Posted test event");
        } else {
            ESP_LOG(ERROR, TAG, "Failed to post event. ret: %d", ret);
        }

        /*if (esp_event_post_to(local_data.handle, TEST_EVENTS, EVENT_MATRIX_SWITCH_CLOSED, NULL, 0, portMAX_DELAY) != ESP_OK) {
            ESP_LOG(ERROR, TAG, "Failed to post event");
        }*/
        
        if (reboot_reqested){
            main_restart_esp();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);


#ifdef LED_TEST
        led_test();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        led_test2();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        led_test3();
#endif
    }

    


}
