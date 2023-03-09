#include <stdio.h>
#include "chess_engine.h"


#define TAG "CHESS_ENGINE"
#define TASK_NAME "chess_engine_task"

typedef struct local_data{

    bool initialised;

    TaskHandle_t task_handle;

    SemaphoreHandle_t lock;

    chess_board_t board;

} local_data_t;

static local_data_t local_data;

static void chess_engine_task(void *args);
static bool access_lock();
static bool release_lock();

esp_err_t chess_engine_init(){

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


    xTaskCreate(chess_engine_task, TASK_NAME, configMINIMAL_STACK_SIZE*2, &params, tskIDLE_PRIORITY, &local_data.task_handle);

    if (local_data.task_handle == NULL){
        ESP_LOG(ERROR, TAG, "Failed to create task: %s. Aborting.", TASK_NAME);
        goto DELETE_TASK;
    }

    //local_data.board = {0};

    ESP_LOG(ERROR, TAG, "FULL BOARD? %d", FULL_BOARD);

    if (FULL_BOARD){
        // TODO: later fill this
    } else { // assuming 3x3 prototype

    // White figures init
        local_data.board.board[0][0].figure_type = FIGURE_PAWN;
        local_data.board.board[0][1].figure_type = FIGURE_PAWN;
        local_data.board.board[0][2].figure_type = FIGURE_PAWN;

        local_data.board.board[0][0].white = true;
        local_data.board.board[0][1].white = true;
        local_data.board.board[0][2].white = true;

        local_data.board.board[0][0].pos_x = 0;
        local_data.board.board[0][0].pos_y = 0;

        local_data.board.board[0][1].pos_x = 1;
        local_data.board.board[0][1].pos_y = 0;

        local_data.board.board[0][2].pos_x = 2;
        local_data.board.board[0][2].pos_y = 0;

        // TODO: add led operations

    // Black figures init
        local_data.board.board[0][0].figure_type = FIGURE_PAWN;
        local_data.board.board[0][1].figure_type = FIGURE_PAWN;
        local_data.board.board[0][2].figure_type = FIGURE_PAWN;

        local_data.board.board[0][0].white = true;
        local_data.board.board[0][1].white = true;
        local_data.board.board[0][2].white = true;

        local_data.board.board[0][0].pos_x = 0;
        local_data.board.board[0][0].pos_y = 0;

        local_data.board.board[0][1].pos_x = 1;
        local_data.board.board[0][1].pos_y = 0;

        local_data.board.board[0][2].pos_x = 2;
        local_data.board.board[0][2].pos_y = 0;

        // TODO: add led operations
    }


    local_data.initialised = true;


    return ESP_OK;

DELETE_TASK:
    vTaskDelete(local_data.task_handle);

    return ESP_FAIL;
}

static void chess_engine_task(void *args){

    while(1){

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }

}

static bool access_lock(){

    if (!local_data.initialised){
        ESP_LOG(ERROR, TAG, "Chess Engine service is not initialised.");
        return false;
    }

    if (xSemaphoreTake(local_data.lock, SECOND_TICK) == pdTRUE){
        return true;
    } 
    return false;
}

static bool release_lock(){
    
    if (!local_data.initialised){
        ESP_LOG(ERROR, TAG, "Chess Engine service is not initialised.");
        return false;
    }

    if (xSemaphoreGive(local_data.lock) == pdTRUE){
        return true;
    }
    return false;
}