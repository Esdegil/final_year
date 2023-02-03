#include <stdio.h>
#include "device.h"

#define TAG "DEVICE"
#define TASK_NAME "device_task"

typedef struct local_data{

    bool initialised;

    TaskHandle_t task_handle;

    SemaphoreHandle_t lock;

} local_data_t;

local_data_t local_data;


static void device_task(void *args);
static bool acquire_lock();
static bool release_lock();



esp_err_t device_init(){

    if (local_data.initialised){
        ESP_LOG(ERROR, TAG, "%s already initialised. Aborting.", TASK_NAME);
        return ESP_FAIL;
    }

    uint8_t params;

    local_data.task_handle = NULL;
    local_data.lock = NULL;

    local_data.lock = xSemaphoreCreateMutex();

    if (local_data.lock == NULL){
        ESP_LOG(ERROR, TAG, "Failed to create mutex lock. Aborting");
        return ESP_FAIL;
    }

    xTaskCreate(device_task, TASK_NAME, configMINIMAL_STACK_SIZE*2, &params, tskIDLE_PRIORITY, &local_data.task_handle);

    if (local_data.task_handle == NULL){
        ESP_LOG(ERROR, TAG, "Failed to create task: %s. Aborting.", TASK_NAME);
        vTaskDelete(local_data.task_handle);
        return ESP_FAIL;
    }

    return ESP_OK;

}


static void device_task(){

    

    while(1){


        ESP_LOG(INFO, TAG, "Testing task.");
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }


}



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

static bool acquire_lock(){
    if (xSemaphoreTake(local_data.lock, SECOND_TICK) == pdTRUE){
        return true;
    } 
    return false;
}

static bool release_lock(){
    if (xSemaphoreGive(local_data.lock) == pdTRUE){
        return true;
    }
    return false;
}
