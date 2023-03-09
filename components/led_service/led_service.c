#include <stdio.h>
#include <string.h>

#include "led_service.h"



#define TAG "LED_SERVICE"
#define TASK_NAME "led_service_task"

struct led_color_t colour_red = {
    .red = 10,
    .green = 1,
    .blue = 1
};


struct led_color_t colour_green = {
    .red = 1,
    .green = 10,
    .blue = 1
};


struct led_color_t colour_blue = {
    .red = 1,
    .green = 1,
    .blue = 10
};


struct led_color_t colour_purple = {
    .red = 10,
    .green = 1,
    .blue = 10
};


struct led_color_t colour_yellow = {
    .red = 10,
    .green = 8,
    .blue = 1
};


struct led_color_t colour_white = {
    .red = 10,
    .green = 10,
    .blue = 10
};



#define LED_STRIP_LENGTH ((MATRIX_X * MATRIX_Y) + 1)
#define LED_STRIP_RMT_INTR_NUM 19U // Not sure what this exactly is. It was in the initial config for this library

typedef struct led_strip_t led_strip_data_t;

typedef struct local_data{

    bool initialised;

    TaskHandle_t task_handle;

    SemaphoreHandle_t lock;

    struct led_color_t led_strip_buf_1[LED_STRIP_LENGTH];
    struct led_color_t led_strip_buf_2[LED_STRIP_LENGTH];

    struct led_strip_t led_strip;


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


    struct led_strip_t led_strip = {
        .rgb_led_type = RGB_LED_TYPE_WS2812,
        .rmt_channel = RMT_CHANNEL_1,
        .rmt_interrupt_num = LED_STRIP_RMT_INTR_NUM,
        .gpio = GPIO_NUM_21, // TODO: this works for now, but might need to be moved to another pin.
        .led_strip_buf_1 = local_data.led_strip_buf_1,
        .led_strip_buf_2 = local_data.led_strip_buf_2,
        .led_strip_length = LED_STRIP_LENGTH
    };
    
    memcpy(&local_data.led_strip, &led_strip, sizeof(led_strip_data_t));

    xTaskCreate(led_service_task, TASK_NAME, configMINIMAL_STACK_SIZE*2, &params, tskIDLE_PRIORITY, &local_data.task_handle);

    if (local_data.task_handle == NULL){
        ESP_LOG(ERROR, TAG, "Failed to create task: %s. Aborting.", TASK_NAME);
        goto DELETE_TASK;
    }

    ESP_LOG(ERROR, TAG, "Before assigning %d", local_data.initialised);

    local_data.initialised = true;

    ESP_LOG(ERROR, TAG, "Initialised %d", local_data.initialised);
    return ESP_OK;

DELETE_TASK:
    vTaskDelete(local_data.task_handle);

    return ESP_FAIL;
}

static void led_service_task(void *args){


    vTaskDelay(1000/portTICK_PERIOD_MS); // I don't know why exactly, but it seems this LED Strip library needs some time before initialising 

    
    local_data.led_strip.access_semaphore = xSemaphoreCreateBinary();

    if (!led_strip_init(&local_data.led_strip)){ // TODO: double check if buffers need to be cleared before exiting on fail.
        ESP_LOG(ERROR, TAG, "Failed to initialise LED Strip task (library). Aborting");
        
    } else {
        ESP_LOG(WARN, TAG, "Initialised LED Strip task successfully");
    }


    if (access_lock()){

    ESP_LOG(WARN, TAG, "LED Strip config: GPIO %d strip type %d len %ld", local_data.led_strip.gpio, local_data.led_strip.rgb_led_type, local_data.led_strip.led_strip_length);

    if (!release_lock()){
        ESP_LOG(ERROR, TAG, "Failed to release lock.");
    }
    } else {
        ESP_LOG(ERROR, TAG, "Failed to access lock.");
    }

    while (1){
        ESP_LOG(INFO, TAG, "%s running.", TASK_NAME);
     
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }

}

esp_err_t led_test(){

    led_strip_clear(&local_data.led_strip);
    ESP_LOG(INFO, TAG, "Cleared LED strip buffers");


    /*if (led_strip_set_pixel_rgb(&local_data.led_strip, 3, 7, 1, 1)){
        ESP_LOG(WARN, TAG, "Success");
    } else {
        ESP_LOG(ERROR, TAG, "Fail");
    }*/

    led_strip_set_pixel_color(&local_data.led_strip, 0, &colour_purple);
    led_strip_set_pixel_color(&local_data.led_strip, 1, &colour_purple);
    led_strip_set_pixel_color(&local_data.led_strip, 2, &colour_purple);
    led_strip_set_pixel_color(&local_data.led_strip, 3, &colour_red);
    led_strip_set_pixel_color(&local_data.led_strip, 4, &colour_red);
    led_strip_set_pixel_color(&local_data.led_strip, 5, &colour_green);
    led_strip_set_pixel_color(&local_data.led_strip, 6, &colour_green);
    led_strip_set_pixel_color(&local_data.led_strip, 7, &colour_yellow);
    led_strip_set_pixel_color(&local_data.led_strip, 8, &colour_yellow);
    



    if (!led_strip_show(&local_data.led_strip)) {
        ESP_LOG(ERROR, TAG, "Failed to show");
    } else {
        ESP_LOG(WARN, TAG, "Showed successfully");
    }


    return ESP_OK;
}

esp_err_t led_test2(){

    led_strip_clear(&local_data.led_strip);
    ESP_LOG(INFO, TAG, "Cleared LED strip buffers");


    if (led_strip_set_pixel_rgb(&local_data.led_strip, 3, 7, 1, 1)){
        ESP_LOG(WARN, TAG, "Success");
    } else {
        ESP_LOG(ERROR, TAG, "Fail");
    }
    led_strip_set_pixel_rgb(&local_data.led_strip, 4, 7, 1, 1);
    led_strip_set_pixel_rgb(&local_data.led_strip, 5, 1, 1, 7);
    led_strip_set_pixel_rgb(&local_data.led_strip, 6, 1, 1, 7);
    led_strip_set_pixel_rgb(&local_data.led_strip, 7, 7, 7, 7);
    led_strip_set_pixel_rgb(&local_data.led_strip, 8, 7, 7, 7);



    if (!led_strip_show(&local_data.led_strip)) {
        ESP_LOG(ERROR, TAG, "Failed to show");
    } else {
        ESP_LOG(WARN, TAG, "Showed successfully");
    }


    return ESP_OK;
}

esp_err_t led_test3(){

    led_strip_clear(&local_data.led_strip);
    ESP_LOG(INFO, TAG, "Cleared LED strip buffers");


    if (led_strip_set_pixel_rgb(&local_data.led_strip, 3, 7, 3, 1)){
        ESP_LOG(WARN, TAG, "Success");
    } else {
        ESP_LOG(ERROR, TAG, "Fail");
    }
    led_strip_set_pixel_rgb(&local_data.led_strip, 4, 7, 1, 5);
    led_strip_set_pixel_rgb(&local_data.led_strip, 5, 1, 7, 3);
    led_strip_set_pixel_rgb(&local_data.led_strip, 6, 2, 7, 4);
    led_strip_set_pixel_rgb(&local_data.led_strip, 7, 7, 7, 7);
    led_strip_set_pixel_rgb(&local_data.led_strip, 8, 7, 7, 1);

    if (!led_strip_show(&local_data.led_strip)) {
        ESP_LOG(ERROR, TAG, "Failed to show");
    } else {
        ESP_LOG(WARN, TAG, "Showed successfully");
    }


    return ESP_OK;
}

static bool access_lock(){

    ESP_LOG(ERROR, TAG, "Initialised in lock %d", local_data.initialised);

    if (!local_data.initialised){
        ESP_LOG(ERROR, TAG, "led  service is not initialised.");
        return false;
    }

    if (xSemaphoreTake(local_data.lock, SECOND_TICK) == pdTRUE){
        return true;
    } 
    return false;
}

static bool release_lock(){
    
    if (!local_data.initialised){
        ESP_LOG(ERROR, TAG, "led service is not initialised.");
        return false;
    }

    if (xSemaphoreGive(local_data.lock) == pdTRUE){
        return true;
    }
    return false;
}

static esp_err_t movement_forward_for_white(uint8_t empty_cells, uint8_t pos_x, uint8_t pos_y) { 

    
    for (int i = 1; i < empty_cells+1; i++){ // TODO: magic number

        if (!led_strip_set_pixel_color(&local_data.led_strip, (pos_y*MATRIX_Y) + pos_x + (MATRIX_X*i), &colour_purple)) {
           // led_strip_clear(); // TODO: not sure if needed
            return ESP_FAIL;
        } 
    }
    
    return ESP_OK;
    
}

static esp_err_t movement_forward_for_black(uint8_t empty_cells, uint8_t pos_x, uint8_t pos_y) {

    
    for (int i = 1; i < empty_cells+1; i++){ // TODO: magic number

        if (!led_strip_set_pixel_color(&local_data.led_strip, (pos_y*MATRIX_Y) + pos_x - (MATRIX_X*i), &colour_white)) {
           // led_strip_clear(); // TODO: not sure if needed
            return ESP_FAIL;
        } 
    }
    
    return ESP_OK;
    
}

esp_err_t led_op_pawn(bool white, bool special_moves,bool attack_right, bool attack_left, uint8_t empty_cells, uint8_t pos_x, uint8_t pos_y) { // TODO: special moves probably should not be bool

    // TODO: think about this whole concept again while it's not too late

    if (white){
        
        if (access_lock()){

            if (!led_strip_clear(&local_data.led_strip)){
                return ESP_FAIL;
            }

            if (special_moves){
                //TODO: do something
            }

            if (movement_forward_for_white(empty_cells, pos_x, pos_y) != ESP_OK){
                ESP_LOG(ERROR,TAG, "Failed to add forward movement to the buffer. Abortng");
                return ESP_FAIL;
            }
            
          
            
            if (release_lock()){
                return ESP_OK;
            } else {
                return ESP_FAIL;
            }
        } else {
            return ESP_FAIL;
        }
    } else { // if black figure
        if (access_lock()){

            if (!led_strip_clear(&local_data.led_strip)){
                return ESP_FAIL;
            }

            if (special_moves){
                //TODO: do something
            }

            if (movement_forward_for_black(empty_cells, pos_x, pos_y) != ESP_OK){
                ESP_LOG(ERROR, TAG, "Failed to add forward movement to the buffer. Abortng");
                return ESP_FAIL;
            }
            
          
            
            if (release_lock()){
                return ESP_OK;
            } else {
                return ESP_FAIL;
            }
        } else {
            return ESP_FAIL;
        }
    }

    return ESP_FAIL;

}