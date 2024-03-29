#include <stdio.h>
#include <string.h>

#include "led_service.h"



#define TAG "LED_SERVICE"
#define TASK_NAME "led_service_task"

#define LED_OP_READY_BIT_0 (1<<0)

#define LED_CLEAR_BIT_0 (1<<1)

struct led_color_t colour_red = {
    .red = 100  ,
    .green = 1,
    .blue = 1
};


struct led_color_t colour_green = {
    .red = 1,
    .green = 100,
    .blue = 1
};


struct led_color_t colour_blue = {
    .red = 1,
    .green = 1,
    .blue = 10
};


struct led_color_t colour_purple = {
    .red = 100,
    .green = 1,
    .blue = 100
};


struct led_color_t colour_yellow = {
    .red = 10,
    .green = 8,
    .blue = 1
};


struct led_color_t colour_white = {
    .red = 200,
    .green = 200,
    .blue = 200
};



//#define LED_STRIP_LENGTH ((MATRIX_X * MATRIX_Y))
#define LED_STRIP_LENGTH 64
#define LED_STRIP_RMT_INTR_NUM 19U // Not sure what this exactly is. It was in the initial config for this library

typedef struct led_strip_t led_strip_data_t;

typedef struct local_data{

    bool initialised;
    bool led_strip_lib_initialised;

    TaskHandle_t task_handle;

    SemaphoreHandle_t lock;

    EventGroupHandle_t event_handle;

    struct led_color_t led_strip_buf_1[LED_STRIP_LENGTH];
    struct led_color_t led_strip_buf_2[LED_STRIP_LENGTH];

    struct led_strip_t led_strip;

    uint8_t led_array[MATRIX_Y*MATRIX_X];


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

    local_data.event_handle = xEventGroupCreate(); 

    struct led_strip_t led_strip = {
        .rgb_led_type = RGB_LED_TYPE_WS2812,
        .rmt_channel = RMT_CHANNEL_1,
        .rmt_interrupt_num = LED_STRIP_RMT_INTR_NUM,
        .gpio = GPIO_NUM_19, // TODO: this works for now, but might need to be moved to another pin.
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
    local_data.led_strip_lib_initialised = false;

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
        local_data.led_strip_lib_initialised = true;
        for (int i = 1; i < MATRIX_X * MATRIX_Y; i+=2){
            led_strip_set_pixel_color(&local_data.led_strip, i, &colour_white); 
        }
    }


    if (access_lock()){

    ESP_LOG(WARN, TAG, "LED Strip config: GPIO %d strip type %d len %ld", local_data.led_strip.gpio, local_data.led_strip.rgb_led_type, local_data.led_strip.led_strip_length);

    if (!release_lock()){
        ESP_LOG(ERROR, TAG, "Failed to release lock.");
    }
    } else {
        ESP_LOG(ERROR, TAG, "Failed to access lock.");
    }


    EventBits_t bits;
    while(1) {
        
        bits = xEventGroupWaitBits(local_data.event_handle, LED_OP_READY_BIT_0, pdTRUE, pdTRUE, SECOND_TICK);
        if ((bits & LED_OP_READY_BIT_0) != 0){
            ESP_LOG(INFO, TAG, "Received New LED op///////////////////////////////////////////////////////////////////////");
            if (!led_strip_show(&local_data.led_strip)){
                ESP_LOG(ERROR, TAG, "Failed to show led strip in task");
            }
            for (int i = 1; i < MATRIX_X * MATRIX_Y; i+=2){
                led_strip_set_pixel_color(&local_data.led_strip, i, &colour_white); 
            }
        }
       

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }

}

esp_err_t led_test(){

    led_strip_clear(&local_data.led_strip);
    ESP_LOG(INFO, TAG, "Cleared LED strip buffers");

    esp_err_t ret = ESP_OK;

    for (int i = 0; i < LED_STRIP_LENGTH+1; i++){
        
        for (int j = 0; j < i; j++){
            if (!led_strip_set_pixel_color(&local_data.led_strip, j, &colour_green)){
                ret = ESP_FAIL;
            }
        }
        ESP_LOG(WARN, TAG, "SHOWWWW2-------------------------------------------------------------------");
        if (!led_strip_show(&local_data.led_strip)){
            ret = ESP_FAIL;
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    led_clear_stripe();
    return ret;
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


    ESP_LOG(WARN, TAG, "SHOWWWW3-------------------------------------------------------------------");
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

    for (int i = 0; i < LED_STRIP_LENGTH; i++){
        led_strip_set_pixel_color(&local_data.led_strip, i, &colour_green);
    }

    ESP_LOG(WARN, TAG, "SHOWWWW4-------------------------------------------------------------------");
    if (!led_strip_show(&local_data.led_strip)) {
        ESP_LOG(ERROR, TAG, "Failed to show");
    } else {
        ESP_LOG(WARN, TAG, "Showed successfully");
    }


    return ESP_OK;
}

static bool access_lock(){

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


esp_err_t led_op_general(uint8_t *arr, uint8_t counter, bool white){

    if (!arr || counter == 0){
        ESP_LOG(ERROR, TAG, "args are null. Aborting");
        return ESP_FAIL;
    }

    struct led_color_t *colour = &colour_red;

    if (white){
        colour = &colour_green;
    } else {
        colour = &colour_purple;
    }

    
    if (local_data.led_strip_lib_initialised) {
    if (access_lock()){
        /*if (!led_strip_clear(&local_data.led_strip)) {
            ESP_LOG(ERROR, TAG, "Failed to clear LED strip buffers. Aborting");
            release_lock();
            return ESP_FAIL;
        }*/
        for (int i = 1; i < MATRIX_X * MATRIX_Y; i+=2){
            led_strip_set_pixel_color(&local_data.led_strip, i, &colour_white); 
        }
        for (int i = 0; i < counter; i++){
            ESP_LOG(INFO, TAG, "Pixel %d", arr[i]);
            if (!led_strip_set_pixel_color(&local_data.led_strip, arr[i], colour)){
                ESP_LOG(ERROR, TAG, "Failed to set colour for pixel %d", arr[i]);
                release_lock();
                return ESP_FAIL; 
            }
        }
        xEventGroupSetBits(local_data.event_handle, LED_OP_READY_BIT_0);
        release_lock();
    } else {
        return ESP_FAIL;
    }
    } else return ESP_FAIL;
    return ESP_OK;



}

esp_err_t led_clear_stripe(){

    esp_err_t ret = ESP_FAIL;

    if (local_data.led_strip_lib_initialised){
    if (access_lock()){

        if (!led_strip_clear(&local_data.led_strip)){
            ESP_LOG(ERROR, TAG, "Failed to clear the stripe");
        } else {
            ESP_LOG(INFO, TAG, "Cleared strip successfully");
        }
        
        for (int i = 1; i < MATRIX_X * MATRIX_Y; i+=2){
            led_strip_set_pixel_color(&local_data.led_strip, i, &colour_white); 
        }
        ESP_LOG(WARN, TAG, "SHOWWWW-------------------------------------------------------------------");
        led_strip_show(&local_data.led_strip);
        
        xEventGroupSetBits(local_data.event_handle, LED_CLEAR_BIT_0);

        if (!release_lock()){
            
            return ESP_FAIL;
        } else {
            ret = ESP_OK;
        }

    } else {
        return ESP_FAIL;
    }
    } else return ESP_FAIL;
    return ret;
}

esp_err_t led_no_move_possible(uint8_t position) {

    esp_err_t ret = ESP_OK;
    if (local_data.led_strip_lib_initialised) {
        if (access_lock()){

            if (!led_strip_clear(&local_data.led_strip)){
                ESP_LOG(ERROR, TAG, "Failed to clear the stripe");
                ret = ESP_FAIL;
            }

            if (!led_strip_set_pixel_color(&local_data.led_strip, position, &colour_red)){
                ESP_LOG(ERROR, TAG, "Failed to set pixel %d to red", position);
                ret = ESP_FAIL;
            }
            ESP_LOG(WARN, TAG, "SHOWWWW-------------------------------------------------------------------");
            if (!led_strip_show(&local_data.led_strip)) {
                ESP_LOG(ERROR, TAG, "Failed to show set buffer");
                ret = ESP_FAIL;
            }

            if (!release_lock()) {
                return ESP_FAIL;
            }
        }
    } else {
        ESP_LOG(ERROR, TAG, "Led strip library not initialised yet");
        ret = ESP_FAIL;
    }

    return ret;
}