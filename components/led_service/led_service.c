#include <stdio.h>
#include "led_service.h"

#define TAG "LED_SERVICE"
#define TASK_NAME "led_service_task"

typedef struct local_data{

    bool initialised;

    TaskHandle_t task_handle;

    SemaphoreHandle_t lock;

    

} local_data_t;

static local_data_t local_data;

static void led_service_task(void *args);
static bool access_lock();
static bool release_lock();



esp_err_t led_service_init(){

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

    xTaskCreate(led_service_task, TASK_NAME, configMINIMAL_STACK_SIZE*2, &params, tskIDLE_PRIORITY, &local_data.task_handle);

    if (local_data.task_handle == NULL){
        ESP_LOG(ERROR, TAG, "Failed to create task: %s. Aborting.", TASK_NAME);
        vTaskDelete(local_data.task_handle);
        return ESP_FAIL;
    }

    local_data.initialised = true;

    return ESP_OK;
}

static void led_service_task(void *args){

    while (1){
        ESP_LOG(INFO, TAG, "%s running.", TASK_NAME);
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }

}


static bool access_lock(){
    if (!local_data.initialised){
    ESP_LOG(ERROR, TAG, "device service is not initialised.");
    return false;
    }

    if (xSemaphoreTake(local_data.lock, SECOND_TICK) == pdTRUE){
        return true;
    } 
    return false;
}

static bool release_lock(){
    if (!local_data.initialised){
        ESP_LOG(ERROR, TAG, "device service is not initialised.");
        return false;
    }

    if (xSemaphoreGive(local_data.lock) == pdTRUE){
        return true;
    }
    return false;
}
