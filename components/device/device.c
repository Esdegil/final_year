#include <stdio.h>
#include <string.h>
#include "device.h"

#define TAG "DEVICE"
#define TASK_NAME "device_task"



typedef struct local_data{

    bool initialised;

    TaskHandle_t task_handle;

    SemaphoreHandle_t lock;

    bool switch_matrix[MATRIX_Y][MATRIX_X];

    bool received_matrix[MATRIX_Y+1][MATRIX_X+1];

    chess_board_t board;

} local_data_t;

static local_data_t local_data;


static void device_task(void *args);
static bool access_lock();
static bool release_lock();


static const char* const figure_names[] = {
    [FIGURE_PAWN] = "Pawn",
    [FIGURE_ROOK] = "Rook",
    [FIGURE_KNIGHT] = "Knight",
    [FIGURE_BISHOP] = "Bishop",
    [FIGURE_QUEEN] = "Queen",
    [FIGURE_KING] = "King"
};

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

#define OUT_PIN_1 23
#define OUT_PIN_2 18
#define OUT_PIN_3 5
#define OUT_PIN_4 17
#define OUT_PIN_5 16
#define OUT_PIN_6 4
#define OUT_PIN_7 2
#define OUT_PIN_8 26

#define IN_PIN_1 36
#define IN_PIN_2 39
#define IN_PIN_3 34
#define IN_PIN_4 35
#define IN_PIN_5 32
#define IN_PIN_6 33
#define IN_PIN_7 25
#define IN_PIN_8 14

#define START_BUTTON_OUT_PIN 27
#define START_BUTTON_IN_PIN 13

int pin_amount = 2;

int out_pins[MATRIX_Y] = {OUT_PIN_1, OUT_PIN_2, OUT_PIN_3, OUT_PIN_4, OUT_PIN_5, OUT_PIN_6, OUT_PIN_7, OUT_PIN_8};
int in_pins[MATRIX_X] = {IN_PIN_1, IN_PIN_2, IN_PIN_3, IN_PIN_4, IN_PIN_5, IN_PIN_6, IN_PIN_7, IN_PIN_8};

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
    gpio_reset_pin(IN_PIN_4);
    gpio_reset_pin(IN_PIN_5);
    gpio_reset_pin(IN_PIN_6);
    gpio_reset_pin(IN_PIN_7);
    gpio_reset_pin(IN_PIN_8);

    gpio_reset_pin(OUT_PIN_1);
    gpio_reset_pin(OUT_PIN_2);
    gpio_reset_pin(OUT_PIN_3);
    gpio_reset_pin(OUT_PIN_4);
    gpio_reset_pin(OUT_PIN_5);
    gpio_reset_pin(OUT_PIN_6);
    gpio_reset_pin(OUT_PIN_7);
    gpio_reset_pin(OUT_PIN_8);

    

    gpio_set_pull_mode(IN_PIN_1, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IN_PIN_2, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IN_PIN_3, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IN_PIN_4, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IN_PIN_5, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IN_PIN_6, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IN_PIN_7, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(IN_PIN_8, GPIO_PULLDOWN_ONLY);

    gpio_set_pull_mode(OUT_PIN_1, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(OUT_PIN_2, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(OUT_PIN_3, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(OUT_PIN_4, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(OUT_PIN_5, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(OUT_PIN_6, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(OUT_PIN_7, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(OUT_PIN_8, GPIO_PULLDOWN_ONLY);
    //gpio_set_pull_mode(OUT_PIN_1, GPIO_PULLDOWN_ONLY);
    //gpio_set_pull_mode(OUT_PIN_2, GPIO_PULLDOWN_ONLY);
    //gpio_set_pull_mode(OUT_PIN_3, GPIO_PULLDOWN_ONLY);

    

    gpio_set_direction(IN_PIN_1, GPIO_MODE_INPUT);
    gpio_set_direction(IN_PIN_2, GPIO_MODE_INPUT);
    gpio_set_direction(IN_PIN_3, GPIO_MODE_INPUT);
    gpio_set_direction(IN_PIN_4, GPIO_MODE_INPUT);
    gpio_set_direction(IN_PIN_5, GPIO_MODE_INPUT);
    gpio_set_direction(IN_PIN_6, GPIO_MODE_INPUT);
    gpio_set_direction(IN_PIN_7, GPIO_MODE_INPUT);
    gpio_set_direction(IN_PIN_8, GPIO_MODE_INPUT);

    gpio_set_direction(OUT_PIN_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(OUT_PIN_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(OUT_PIN_3, GPIO_MODE_OUTPUT);
    gpio_set_direction(OUT_PIN_4, GPIO_MODE_OUTPUT);
    gpio_set_direction(OUT_PIN_5, GPIO_MODE_OUTPUT);
    gpio_set_direction(OUT_PIN_6, GPIO_MODE_OUTPUT);
    gpio_set_direction(OUT_PIN_7, GPIO_MODE_OUTPUT);
    gpio_set_direction(OUT_PIN_8, GPIO_MODE_OUTPUT);

    gpio_config_t out14_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL<<GPIO_NUM_14,
        .pull_down_en = GPIO_PULLDOWN_ONLY,
        .pull_up_en =  GPIO_PULLUP_DISABLE
    };

    gpio_config(&out14_conf);

    gpio_reset_pin(GPIO_NUM_27);
    gpio_reset_pin(GPIO_NUM_13);

    gpio_set_pull_mode(START_BUTTON_OUT_PIN, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(START_BUTTON_IN_PIN, GPIO_PULLDOWN_ONLY);
    
    gpio_set_direction(START_BUTTON_OUT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(START_BUTTON_IN_PIN, GPIO_MODE_INPUT);

    gpio_config_t out13_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL<<GPIO_NUM_13,
        .pull_down_en = GPIO_PULLDOWN_ONLY,
        .pull_up_en =  GPIO_PULLUP_DISABLE
    };

    gpio_config(&out13_conf);

    /*
    gpio_config_t out8_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL<<GPIO_N,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en =  GPIO_PULLUP_DISABLE
    };

    gpio_config(&out8_conf);
    */
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

    bool button_ready_to_start = false;
    bool matrices_align = false;
    uint8_t start_level = 0;

    if (device_set_pin_level(START_BUTTON_OUT_PIN, 1) != ESP_OK){
        ESP_LOG(ERROR, TAG, "Failed to set start button out to 1");
    } else {
        ESP_LOG(INFO, TAG, "Set successfully");
    }

    uint8_t missing_figures_counter = 0;
    uint8_t led_array[32];
    uint8_t converted_pos = 0;
    // Board initialisation and alignement loop

    uint8_t init_level = 0;
    char *colour;
    bool at_least_one_missing = true;
    bool white_figure;
    for (int i = 0; i < MATRIX_Y; i++){
        for (int j = 0; j < MATRIX_X; j++){
            if (local_data.board.board[i][j].figure_type != FIGURE_END_LIST){
                ESP_LOG(WARN, TAG, "Checking figure type %d", local_data.board.board[i][j].figure_type);
                device_set_pin_level(out_pins[i], 1);
                ESP_LOG(ERROR, TAG, "Seeting %d", out_pins[i]);
                vTaskDelay(500/portTICK_PERIOD_MS);
                device_get_pin_level(in_pins[j], &level);
                ESP_LOG(WARN, TAG, "level after read %d on pin %d", level, in_pins[j]);
                if ((bool)level != true){
                    white_figure = local_data.board.board[i][j].white;
                    if (white_figure){
                        colour = "white ";
                    } else {
                        colour = "black ";
                    }
                    uint8_t colour_len = strlen(colour);
                    char *figure_name = figure_names[local_data.board.board[i][j].figure_type];
                    char rest[] = " on highlighted square";
                    uint8_t figure_len = strlen(figure_name);
                    uint8_t rest_len = strlen(rest);

                    ESP_LOG(WARN, TAG, "Sizes %d col %d fig %d rest", colour_len, figure_len, rest_len);

                    char full_message[(colour_len + figure_len + rest_len + 1)];
                    memset(full_message, 0, rest_len+colour_len+figure_len);
          
                    strcat(full_message, colour);
                    strcat(full_message, figure_name);
                    strcat(full_message, rest);
                    
                    ESP_LOG(INFO, TAG, "Message %s", full_message);

                    display_send_message_to_display(full_message);

                    converted_pos = MATRIX_TO_ARRAY_CONVERSION((i), (j));
                    led_array[0] = converted_pos;
                    if (led_op_general(led_array, 1, white_figure) != ESP_OK){
                        ESP_LOG(ERROR, TAG, "Failed to highlight required square");
                    }

                    level = 0;
                    while(!(bool)level){
                        device_get_pin_level(in_pins[j], &level);
                        vTaskDelay(300/portTICK_PERIOD_MS);
                    }
                    level = 0;
                    vTaskDelay(500/portTICK_PERIOD_MS);
                    while(!(bool)level){
                        device_get_pin_level(in_pins[j], &level);
                        vTaskDelay(300/portTICK_PERIOD_MS);
                    }
                    device_set_pin_level(out_pins[i], 0);
                }
            }
        }
    }

    while(!button_ready_to_start || !matrices_align){
        device_get_pin_level(START_BUTTON_IN_PIN, &start_level);
        button_ready_to_start = start_level ? true : false;
    
        ESP_LOG(INFO, TAG, "level %d", start_level);

        for(int i = 0; i < MATRIX_X; i++){
                for (int j = 0; j < MATRIX_Y; j++){

                    device_set_pin_level(out_pins[j], 0);
                }
                device_set_pin_level(out_pins[i], 1);
                vTaskDelay(80/portTICK_PERIOD_MS);
                for (int j = 0; j < MATRIX_Y; j++){
                    device_get_pin_level(in_pins[j], &level); 
                    
                    if ((bool)level != local_data.switch_matrix[i][j]){
                        ESP_LOG(WARN, TAG, "Change detected at pin %d  with pin %d set. at level %d array id %d:%d", in_pins[j], out_pins[i], level, i, j);
                        vTaskDelay(500/portTICK_PERIOD_MS);
                        device_get_pin_level(in_pins[j], &level);

                        if ((bool)level != local_data.switch_matrix[i][j]){
                            ESP_LOG(WARN, TAG, "Change detected again at pin %d  with pin %d set. at level %d array id %d:%d", in_pins[j], out_pins[i], level, i, j);
                        
                            local_data.switch_matrix[i][j] = level ? true : false;
                            print_array();  
                        }  
                    }
                    
                }
            }
            matrices_align = true;
            
            for (int i = 0; i < MATRIX_Y; i++){
                for (int j = 0; j < MATRIX_X; j++) {
                    if (local_data.switch_matrix[i][j] != local_data.received_matrix[i][j]) {
                        matrices_align = false;
                    }
                }
            }
            if (matrices_align){
                ESP_LOG(INFO, TAG, "Matrices aligned sucessfully");
            }

    }
    device_set_pin_level(START_BUTTON_OUT_PIN, 0);

    chess_engine_device_service_ready();

    ESP_LOG(INFO, TAG, "Matrices aligned sucessfully and Start button was pressed");
    ESP_LOG(INFO, TAG, "STARTING GAME");
    state_change_data_t changed_state_figure;
    while(1){

        for(int i = 0; i < MATRIX_X; i++){
            for (int j = 0; j < MATRIX_Y; j++){

                device_set_pin_level(out_pins[j], 0);
            }
            vTaskDelay(80/portTICK_PERIOD_MS);
            device_set_pin_level(out_pins[i], 1);
            for (int j = 0; j < MATRIX_Y; j++){
                device_get_pin_level(in_pins[j], &level); 
                // TODO: double check about this reversed order
                if ((bool)level != local_data.switch_matrix[i][j]){
                    ESP_LOG(WARN, TAG, "Change detected at pin %d  with pin %d set. at level %d array id %d:%d", in_pins[j], out_pins[i], level, i, j);
                    vTaskDelay(500/portTICK_PERIOD_MS);
                    device_get_pin_level(in_pins[j], &level);

                    if ((bool)level != local_data.switch_matrix[i][j]){
                        ESP_LOG(WARN, TAG, "Change detected again at pin %d  with pin %d set. at level %d array id %d:%d", in_pins[j], out_pins[i], level, i, j);
                        local_data.switch_matrix[i][j] = level ? true : false;
                        changed_state_figure.lifted = !level; // TODO: double check this
                        changed_state_figure.pos.pos_x = j;
                        changed_state_figure.pos.pos_y = i;
                        print_array();
                        if (update_board_on_lift(changed_state_figure) != ESP_OK){
                            ESP_LOG(ERROR, TAG, "Failed to update chess engine from device service");
                        }
                    }
                }
                //ESP_LOG(WARN, TAG, "Pin %d  pos %d level %d", in_pins[i], j, level);
                
            }
        }
        
    vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    

}

esp_err_t device_receive_required_positions(chess_board_t board){

    local_data.board = board;

     for (int i = 0; i < MATRIX_Y; i++){
        for (int j = 0; j < MATRIX_X; j++){
            local_data.received_matrix[i][j] = false;
            if (board.board[i][j].figure_type != FIGURE_END_LIST){
                ESP_LOG(INFO, TAG, "Figure should be on %d:%d", i, j);
                local_data.received_matrix[i][j] = true;
            }
        }
    }


    return ESP_OK;

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
