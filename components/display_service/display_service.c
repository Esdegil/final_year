#include <stdio.h>
#include "display_service.h"


#define TAG "DISPLAY_SERVICE"
#define TASK_NAME "display_service_task"

typedef struct local_data{

    bool initialised;

    TaskHandle_t task_handle;

    SemaphoreHandle_t lock;

    SSD1306_t display;


} local_data_t;

static local_data_t local_data;

static void display_service_task(void *args);
static bool access_lock();
static bool release_lock();


esp_err_t display_service_init() {


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


    xTaskCreate(display_service_task, TASK_NAME, configMINIMAL_STACK_SIZE*2, &params, tskIDLE_PRIORITY, &local_data.task_handle);

    if (local_data.task_handle == NULL){
        ESP_LOG(ERROR, TAG, "Failed to create task: %s. Aborting.", TASK_NAME);
        goto DELETE_TASK;
    }

    local_data.initialised = true;

    ESP_LOG(ERROR, TAG, "Initialised %d", local_data.initialised);
    return ESP_OK;

DELETE_TASK:
    vTaskDelete(local_data.task_handle);

    return ESP_FAIL;

}

static void display_service_task(void *args) {

#if CONFIG_I2C_INTERFACE
    ESP_LOG(INFO, TAG, "I2C interface");
    ESP_LOG(INFO, TAG, "configured SDA GPIO: %d", CONFIG_SDA_GPIO);
    ESP_LOG(INFO, TAG, "configured SCL GPIO: %d", CONFIG_SCL_GPIO);
    ESP_LOG(INFO, TAG, "configured RESET GPIO: %d", CONFIG_RESET_GPIO);
    i2c_master_init(&local_data.display, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
#endif

#if CONFIG_SSD1306_128x64
    ESP_LOG(INFO, TAG, "Set to 128x64");
    ssd1306_init(&local_data.display, 128, 64);
#endif

    ssd1306_clear_screen(&local_data.display, false);
    ssd1306_contrast(&local_data.display, 0xff);

    ssd1306_display_text_x3(&local_data.display, 0, "Hello", 5, false);

    vTaskDelay(2000/portTICK_PERIOD_MS);
    ssd1306_clear_screen(&local_data.display, false);

    ssd1306_display_text_x3(&local_data.display, 0, "T'MOK", 5, false);


    vTaskDelay(2000/portTICK_PERIOD_MS);
    ssd1306_clear_screen(&local_data.display, false);

    ssd1306_display_text(&local_data.display, 0, "TEST", 4, false);
    ssd1306_display_text(&local_data.display, 1, "IT", 2, false);

    while(1) {
        ESP_LOG(INFO, TAG, "Display service task is running");
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }

}

static bool access_lock(){

    if (!local_data.initialised){
        ESP_LOG(ERROR, TAG, "Display service is not initialised.");
        return false;
    }

    if (xSemaphoreTake(local_data.lock, SECOND_TICK) == pdTRUE){
        return true;
    } 
    ESP_LOG(ERROR, TAG, "Failed to access lock");
    return false;
}

static bool release_lock(){
    
    if (!local_data.initialised){
        ESP_LOG(ERROR, TAG, "Display service is not initialised.");
        return false;
    }

    if (xSemaphoreGive(local_data.lock) == pdTRUE){
        return true;
    }
    ESP_LOG(ERROR, TAG, "Failed to release lock");
    return false;
}