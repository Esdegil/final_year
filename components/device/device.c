#include <stdio.h>
#include "device.h"

#define TAG "DEVICE"
#define TASK_NAME "device_task"



typedef struct local_data{

    bool initialised;

    TaskHandle_t task_handle;

    SemaphoreHandle_t lock;

    bool switch_matrix[MATRIX_X][MATRIX_Y];

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

#define OUT_PIN_1 32
#define OUT_PIN_2 33
#define OUT_PIN_3 25

#define IN_PIN_1 34
#define IN_PIN_2 36
#define IN_PIN_3 39


int pin_amount = 2;

int out_pins[3] = {32, 33, 25};
int in_pins[3] = {36, 39, 34};

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

    vTaskDelay(2000/portTICK_PERIOD_MS);

    gpio_reset_pin(IN_PIN_1);
    gpio_reset_pin(IN_PIN_2);
    gpio_reset_pin(IN_PIN_3);

    gpio_reset_pin(OUT_PIN_1);
    gpio_reset_pin(OUT_PIN_2);
    gpio_reset_pin(OUT_PIN_3);

    gpio_set_pull_mode(IN_PIN_1, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IN_PIN_2, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IN_PIN_3, GPIO_PULLDOWN_ONLY);
    //gpio_set_pull_mode(OUT_PIN_1, GPIO_PULLDOWN_ONLY);
    //gpio_set_pull_mode(OUT_PIN_2, GPIO_PULLDOWN_ONLY);
    //gpio_set_pull_mode(OUT_PIN_3, GPIO_PULLDOWN_ONLY);

    

    gpio_set_direction(IN_PIN_1, GPIO_MODE_INPUT);
    gpio_set_direction(IN_PIN_2, GPIO_MODE_INPUT);
    gpio_set_direction(IN_PIN_3, GPIO_MODE_INPUT);
    gpio_set_direction(OUT_PIN_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(OUT_PIN_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(OUT_PIN_3, GPIO_MODE_OUTPUT);

    

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

    for(int i = 0; i < MATRIX_X; i++){
            for (int j = 0; j < MATRIX_Y; j++){

                device_set_pin_level(out_pins[j], 0);
            }
            device_set_pin_level(out_pins[i], 1);
            for (int j = 0; j < MATRIX_Y; j++){
                device_get_pin_level(in_pins[j], &level); 
                // TODO: double check about this reversed order
                if ((bool)level != local_data.switch_matrix[j][i]){
                    ESP_LOG(WARN, TAG, "Change detected at pin %d  with pin %d set. at level %d array id %d:%d", in_pins[j], out_pins[i], level, j, i);
                    local_data.switch_matrix[j][i] = level ? true : false;
                    print_array();    
                }
                //ESP_LOG(WARN, TAG, "Pin %d  pos %d level %d", in_pins[j], j, level);
                
            }
        }


    
    state_change_data_t changed_state_figure;
    while(1){

        for(int i = 0; i < MATRIX_X; i++){
            for (int j = 0; j < MATRIX_Y; j++){

                device_set_pin_level(out_pins[j], 0);
            }
            vTaskDelay(50/portTICK_PERIOD_MS);
            device_set_pin_level(out_pins[i], 1);
            for (int j = 0; j < MATRIX_Y; j++){
                device_get_pin_level(in_pins[j], &level); 
                // TODO: double check about this reversed order
                if ((bool)level != local_data.switch_matrix[j][i]){
                    ESP_LOG(WARN, TAG, "Change detected at pin %d  with pin %d set. at level %d array id %d:%d", in_pins[j], out_pins[i], level, j, i);
                    local_data.switch_matrix[j][i] = level ? true : false;
                    changed_state_figure.lifted = !level; // TODO: double check this
                    changed_state_figure.pos.pos_x = i;
                    changed_state_figure.pos.pos_y = j;
                    print_array();
                    if (update_board_on_lift(changed_state_figure) != ESP_OK){
                        ESP_LOG(ERROR, TAG, "Failed to update chess engine from device service");
                    }
                }
                ESP_LOG(WARN, TAG, "Pin %d  pos %d level %d", in_pins[i], j, level);
                
            }
        }
        
    vTaskDelay(5000/portTICK_PERIOD_MS);
    }
    

}



esp_err_t device_get_pin_level(int pin, uint8_t *level){
    *level = gpio_get_level(pin);
    
    //ESP_LOG(INFO, TAG, "reading pin %d level %d", pin, *level);
    return ESP_OK;
}

esp_err_t device_set_pin_level(int pin, uint8_t level) {
    //ESP_LOG(INFO, TAG,"setting pin %d to level %d", pin, level);
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
