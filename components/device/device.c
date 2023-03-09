#include <stdio.h>
#include "device.h"

#define TAG "DEVICE"
#define TASK_NAME "device_task"



typedef struct local_data{

    bool initialised;

    TaskHandle_t task_handle;

    SemaphoreHandle_t lock;

    bool switch_matrix[MATRIX_X+1][MATRIX_Y+1];

} local_data_t;

static local_data_t local_data;


static void device_task(void *args);
static bool access_lock();
static bool release_lock();



esp_err_t device_init(){

    if (local_data.initialised){
        ESP_LOG(ERROR, TAG, "%s already initialised. Aborting.", TASK_NAME);
        return ESP_FAIL;
    }


    ESP_LOG(INFO, TAG, "Initialising LED service.");

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
        goto DELETE_TASK;
    }

    for(int i = 0; i < MATRIX_X; i++){
        for(int j = 0; j < MATRIX_Y; j++){
            local_data.switch_matrix[i][j] = false;
        }
    }
    ESP_LOG(ERROR, TAG, "Before assigning %d", local_data.initialised);

    local_data.initialised = true;

    ESP_LOG(ERROR, TAG, "After assigning %d", local_data.initialised);

    return ESP_OK;

DELETE_TASK:
    vTaskDelete(local_data.task_handle);
    // TODO: double check if memset 0 on local_data needed

    return ESP_FAIL;

}

static void print_array(){
    if(access_lock()){
        
        for (int i = 0; i < MATRIX_X; i++){
            printf("{ ");
            for (int j = 0; j < MATRIX_Y; j++){
                printf("%d ", local_data.switch_matrix[i][j] ? 1 : 0);
            }
            printf("}\n");
        }

        release_lock();
    } else {
        ESP_LOG(ERROR, TAG, "Failed to access lock.");
    }
}

#define IN_PIN_1 32
#define IN_PIN_2 33
#define IN_PIN_3 25

#define OUT_PIN_1 26
#define OUT_PIN_2 27
#define OUT_PIN_3 23


int pin_amount = 2;

int pins[4] = {32, 33, 25};
int out_pins[4] = {26, 27, 23};

static void device_task(){

    uint8_t level = 0;

    //gpio_set_direction(27, GPIO_MODE_OUTPUT);

    //gpio_set_level(27, 1);

    print_array();
    

    /*if (gpio_set_pull_mode(IN_PIN_1, GPIO_PULLDOWN_ONLY) != ESP_OK) {
        
    }
    if (gpio_set_pull_mode(IN_PIN_2, GPIO_PULLDOWN_ONLY) != ESP_OK) {
     ESP_LOG(ERROR, TAG, "Failed to set pull mode");}
    if (gpio_set_pull_mode(OUT_PIN_1, GPIO_PULLDOWN_ONLY) != ESP_OK)  {ESP_LOG(ERROR, TAG, "Failed to set pull mode");   }
    if (gpio_set_pull_mode(OUT_PIN_2, GPIO_PULLDOWN_ONLY) != ESP_OK) {ESP_LOG(ERROR, TAG, "Failed to set pull mode");
     
    }*/

    gpio_set_pull_mode(IN_PIN_1, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IN_PIN_2, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IN_PIN_3, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(OUT_PIN_1, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(OUT_PIN_2, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(OUT_PIN_3, GPIO_PULLDOWN_ONLY);


    gpio_set_direction(IN_PIN_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN_PIN_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN_PIN_3, GPIO_MODE_OUTPUT);
    gpio_set_direction(OUT_PIN_1, GPIO_MODE_INPUT);
    gpio_set_direction(OUT_PIN_2, GPIO_MODE_INPUT);
    gpio_set_direction(OUT_PIN_3, GPIO_MODE_INPUT);


#ifdef TEST
    gpio_set_pull_mode(22, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(19, GPIO_PULLDOWN_ONLY);

    gpio_set_direction(22, GPIO_MODE_OUTPUT);
    gpio_set_direction(19, GPIO_MODE_OUTPUT);


    gpio_set_pull_mode(21, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(18, GPIO_PULLDOWN_ONLY);

    gpio_set_direction(21, GPIO_MODE_INPUT);
    gpio_set_direction(18, GPIO_MODE_INPUT);

    device_set_pin_level(22, 1);
    device_set_pin_level(19, 1);


    while(1){

        device_get_pin_level(21, &level);
        device_get_pin_level(18, &level);
        

        vTaskDelay(1000/portTICK_PERIOD_MS);

    }
#endif


    while(1){

        for(int i = 0; i < MATRIX_X; i++){
            for (int j = 0; j < MATRIX_Y; j++){

                device_set_pin_level(pins[j], 0);
            }
            device_set_pin_level(pins[i], 1);
            for (int j = 0; j < MATRIX_Y; j++){
                device_get_pin_level(out_pins[j], &level);
                if ((bool)level != local_data.switch_matrix[i][j]){
                    ESP_LOG(WARN, TAG, "Change detected at pin %d at level %d", out_pins[i], level);
                    local_data.switch_matrix[i][j] = level ? true : false;
                }
                ESP_LOG(WARN, TAG, "Pin %d  pos %d level %d", out_pins[i], j, level);
                print_array();
            }
        }
        
        /*ESP_LOG(INFO, TAG, "Testing task.");
        vTaskDelay(1000/portTICK_PERIOD_MS);*/
    vTaskDelay(5000/portTICK_PERIOD_MS);
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
