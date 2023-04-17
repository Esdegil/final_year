#include <stdio.h>
#include <string.h>
#include "display_service.h"


#define TAG "DISPLAY_SERVICE"
#define TASK_NAME "display_service_task"

typedef struct local_data{

    bool initialised;

    TaskHandle_t task_handle;

    SemaphoreHandle_t lock;

    SSD1306_t display;

    QueueHandle_t queue;

} local_data_t;

typedef struct display_message {
    char *message;
    uint8_t length;

} display_message_t;

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

    local_data.queue = xQueueCreate(3, sizeof(display_message_t));

    local_data.initialised = true;

    ESP_LOG(ERROR, TAG, "Initialised %d", local_data.initialised);
    return ESP_OK;

DELETE_TASK:
    vTaskDelete(local_data.task_handle);

    return ESP_FAIL;

}

static esp_err_t display_data_receiver(display_message_t *data) {

    if (access_lock()) {
        xQueueReceive(local_data.queue, data, 0);

        ESP_LOG(DEBUG, TAG, "Received in display data receiver: message %s mesage len %d", data->message, data->length);

        if (!release_lock()){
            return ESP_FAIL;
        }
    } else {
        return ESP_FAIL;
    }
    return ESP_OK;
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

    display_message_t message_to_display;

    uint8_t rows_required = 0;
    char temp_message[16];
    while(1) {
        

        if (uxQueueMessagesWaiting(local_data.queue) != 0){
            if (display_data_receiver(&message_to_display) == ESP_OK){
            
                ESP_LOG(WARN, TAG, "Received message in task %s its len %d", message_to_display.message, message_to_display.length);
                ssd1306_clear_screen(&local_data.display, false);

                if (message_to_display.length > 16) { // magic number
                    rows_required = (message_to_display.length / 16)+1;
                    if (message_to_display.length % 16 == 0){
                        rows_required--;
                    }
                    for (int i = 0; i < rows_required; i++){
                        for (int j = 0; j < 16; j++){
                            
                            temp_message[j] = message_to_display.message[j + (i * 16)]; 
                        }
                        ssd1306_display_text(&local_data.display, i, temp_message, strlen(temp_message), false);
                        memset(temp_message, 0, 16);    
                    } 
                } else {
                    ssd1306_display_text(&local_data.display, 0, message_to_display.message, message_to_display.length, false);
                }


                
                //ssd1306_display_text(&local_data.display, 1, "IT", 2, false);

            } else {
                ESP_LOG(ERROR, TAG, "Failed to receive data in queue");
            }
        }

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

esp_err_t display_send_message_to_display(char* message) {

    if (!message){
        ESP_LOG(ERROR, TAG, "message is null");
        return ESP_FAIL;
    }

     esp_err_t ret = ESP_FAIL;

    uint8_t message_length = 0;

    message_length = strlen(message);

    if (message_length == 0) {
        ESP_LOG(ERROR, TAG, "Message length is 0. Aborting");
        return ret;
    }
    ESP_LOG(INFO, TAG, "received message len %d and message is %s", message_length, message);

    display_message_t display_data;

    display_data.length = message_length;
    display_data.message = message; // maybe memcpy

   if (access_lock()) {
        
        if (xQueueSend(local_data.queue, (void*)&display_data, (TickType_t)10) != pdTRUE) {
            ESP_LOG(ERROR, TAG, "Failed to post message data to queue. Aborting");
            release_lock();
            return ESP_FAIL;
        }
        
        release_lock();
        ESP_LOG(INFO, TAG, "Posted to queue successfully");
        ret = ESP_OK;
    } else {
        ESP_LOG(ERROR, TAG, "Failed to access lock");
        return ESP_FAIL;
    }

    
    return ret;

}