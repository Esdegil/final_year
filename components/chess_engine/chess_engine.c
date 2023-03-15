#include <stdio.h>
#include "chess_engine.h"


#define TAG "CHESS_ENGINE"
#define TASK_NAME "chess_engine_task"

typedef struct local_data{

    bool initialised;

    TaskHandle_t task_handle;

    SemaphoreHandle_t lock;

    chess_board_t board;

    EventGroupHandle_t event_handle;

    QueueHandle_t queue; // TODO: maybe rename this 

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

    local_data.event_handle = xEventGroupCreate(); // TODO: add checks and removes here
    local_data.queue = xQueueCreate(1, sizeof(figure_position_t));// TODO: add checks and removes here

    ESP_LOG(ERROR, TAG, "FULL BOARD? %d", FULL_BOARD);

    if (FULL_BOARD){
        // TODO: later fill this
    } else { // assuming 3x3 prototype

    // White figures init
        local_data.board.board[0][0].figure_type = FIGURE_KNIGHT;
        local_data.board.board[0][1].figure_type = FIGURE_KNIGHT;
        local_data.board.board[0][2].figure_type = FIGURE_END_LIST;

        local_data.board.board[0][0].white = true;
        local_data.board.board[0][1].white = true;
        local_data.board.board[0][2].white = true;

        local_data.board.board[0][0].pos_x = 0;
        local_data.board.board[0][0].pos_y = 0;

        local_data.board.board[0][1].pos_x = 1;
        local_data.board.board[0][1].pos_y = 0;

        local_data.board.board[0][2].pos_x = 2;
        local_data.board.board[0][2].pos_y = 0;

        local_data.board.board[0][0].led_op = &led_op_pawn;
        local_data.board.board[0][1].led_op = &led_op_pawn;
        local_data.board.board[0][2].led_op = &led_op_pawn;


        

    // Black figures init
        local_data.board.board[2][0].figure_type = FIGURE_END_LIST;
        local_data.board.board[2][1].figure_type = FIGURE_END_LIST;
        local_data.board.board[2][2].figure_type = FIGURE_END_LIST;

        local_data.board.board[2][0].white = false;
        local_data.board.board[2][1].white = false;
        local_data.board.board[2][2].white = false;

        local_data.board.board[2][0].pos_x = 0;
        local_data.board.board[2][0].pos_y = 2;

        local_data.board.board[2][1].pos_x = 1;
        local_data.board.board[2][1].pos_y = 2;

        local_data.board.board[2][2].pos_x = 2;
        local_data.board.board[2][2].pos_y = 2;

        local_data.board.board[2][0].led_op = &led_op_pawn;
        local_data.board.board[2][1].led_op = &led_op_pawn;
        local_data.board.board[2][2].led_op = &led_op_pawn;

    // Rest of the board

        local_data.board.board[1][0].figure_type = FIGURE_END_LIST;
        local_data.board.board[1][1].figure_type = FIGURE_END_LIST;
        local_data.board.board[1][2].figure_type = FIGURE_END_LIST;


        //local_data.board.board[1][1].white = true;

        //local_data.board.board[1][1].led_op = &led_op_pawn;

        local_data.board.board[1][0].pos_x = 0;
        local_data.board.board[1][0].pos_y = 1;

        local_data.board.board[1][1].pos_x = 1;
        local_data.board.board[1][1].pos_y = 1;

        local_data.board.board[1][2].pos_x = 2;
        local_data.board.board[1][2].pos_y = 1;
    }


    local_data.initialised = true;


    return ESP_OK;

DELETE_TASK:
    vTaskDelete(local_data.task_handle);

    return ESP_FAIL;
}

static esp_err_t chess_queue_receiver(figure_position_t* buffer) {

    if (access_lock()) {
        xQueueReceive(local_data.queue, buffer, 0);
        if (!release_lock()){
            return ESP_FAIL;
        }
    } else {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t movement_forward_calculation(uint8_t empty_cells, figure_position_t pos, bool white, uint8_t *available_moves, uint8_t *counter) {

    if (white){
        for (int i = 1; i < empty_cells+1; i++){ // TODO: magic number

            available_moves[*counter]=(MATRIX_Y*(pos.pos_y+i)) + pos.pos_x;
            ESP_LOG(INFO, TAG, "Calculated forward move: %d:%d", pos.pos_y+i, pos.pos_x);
            (*counter)++;
        }
    } else {
        for (int i = 1; i < empty_cells+1; i++){ // TODO: magic number

            available_moves[*counter]=(MATRIX_Y*(pos.pos_y-i)) + pos.pos_x;
            ESP_LOG(INFO, TAG, "Calculated forward move: %d:%d", pos.pos_y-i, pos.pos_x);
            (*counter)++;
        }
    }
    
    return ESP_OK;
    
}

static esp_err_t pawn_led_calculation(figure_position_t pos) {
    
    chess_board_t board;

    if(access_lock()){
        board = local_data.board;
        release_lock();
    } else {
        return ESP_FAIL;
    }

    uint8_t available_moves[MAX_PAWN_MOVES];
    uint8_t counter = 0;
    uint8_t empty_cells = 0;

    if (board.board[pos.pos_y][pos.pos_x].white){
        ESP_LOG(ERROR, TAG, "Calculating for white");
        // TODO: add checkmate checks etc

        for (int i = 1; i < PAWN_MAX_FORWARD_MOVEMENT+1; i++){
            if (pos.pos_y+1 >= MATRIX_Y){
                ESP_LOG(WARN, TAG, "Breaking loop.White");
                break;
            }
            if (board.board[pos.pos_y+i][pos.pos_x].figure_type == FIGURE_END_LIST){
                empty_cells++;
            }
        }
        ESP_LOG(WARN, TAG, "Empty cells calculated: %d", empty_cells);
        movement_forward_calculation(empty_cells, pos, board.board[pos.pos_y][pos.pos_x].white, available_moves, &counter);

        if (pos.pos_x > 0) {
            if (board.board[pos.pos_y+1][pos.pos_x-1].figure_type != FIGURE_END_LIST){ // attacking movement
                ESP_LOG(WARN, TAG, "Calculating left attacking move. White");
                available_moves[counter]=(MATRIX_X*(pos.pos_y+1)) + pos.pos_x-1;
                counter++;
            }
        } else {
            ESP_LOG(INFO, TAG, "No left attack possible. White");
        }

        if (pos.pos_x < MATRIX_X-1){
            if (board.board[pos.pos_y+1][pos.pos_x+1].figure_type != FIGURE_END_LIST){ // attacking movement
            ESP_LOG(WARN, TAG, "Calculating right attacking move. White");
                available_moves[counter]=(MATRIX_X*(pos.pos_y+1)) + pos.pos_x+1;
                counter++;
            }
        } else {
            ESP_LOG(INFO, TAG, "No right attack possible. White");
        }

    } else {
        ESP_LOG(ERROR, TAG, "Calculating for black");
        // TODO: add checkmate checks etc

        for (int i = PAWN_MAX_FORWARD_MOVEMENT; i > 0; i--){
            if (pos.pos_y-1 > pos.pos_y){ // TODO: really double check this
                ESP_LOG(WARN, TAG, "Breaking loop. Black");
                break;
            }
            if (board.board[pos.pos_y-i][pos.pos_x].figure_type == FIGURE_END_LIST){
                empty_cells++;
            }
        }

        movement_forward_calculation(empty_cells, pos, board.board[pos.pos_y][pos.pos_x].white, available_moves, &counter);

        if (pos.pos_x != 0){
            if (board.board[pos.pos_y-1][pos.pos_x-1].figure_type != FIGURE_END_LIST){ // attacking movement
                ESP_LOG(WARN, TAG, "Calculating left attacking move. Black");
                available_moves[counter]=(MATRIX_X*(pos.pos_y-1)) + pos.pos_x-1;
                counter++;
            }
        } else {
            ESP_LOG(INFO, TAG, "No left attack possible. Black");
        }

        if (pos.pos_x < MATRIX_X - 1) {
            if (board.board[pos.pos_y-1][pos.pos_x+1].figure_type != FIGURE_END_LIST){ // attacking movement
                ESP_LOG(WARN, TAG, "Calculating right attacking move. Black");
                available_moves[counter]=(MATRIX_X*(pos.pos_y-1)) + pos.pos_x+1;
                counter++;
            }
        } else {
            ESP_LOG(INFO, TAG, "No right attack possible. Black");
        }

    }

    ESP_LOG(WARN, TAG, "Calculated available moves for %d:%d", pos.pos_y, pos.pos_x);
    for (int i = 0; i < counter; i++){
        ESP_LOG(WARN, TAG, "Move: %d", available_moves[i]);
    }

    local_data.board.board[pos.pos_y][pos.pos_x].led_op(available_moves, counter);

    /*if (local_data.board.board[pos.pos_y][pos.pos_x].led_op(available_moves, counter) != ESP_OK){
        ESP_LOG(ERROR, TAG, "Failed to execute led operation on pawn movement");
        return ESP_FAIL;
    }*/

    return ESP_OK;
}

static uint8_t required_cells_calculation_forward(chess_board_t board, figure_position_t pos){
    uint8_t required_cells = 0;
    bool loop_broken = false;
    
    for (int i = 1; !(board.board[pos.pos_y+i][pos.pos_x].figure_type != FIGURE_END_LIST); i++){
        required_cells++;
        if(pos.pos_y+i >= MATRIX_Y){
            required_cells--;
            loop_broken = true;
            ESP_LOG(ERROR, TAG, "Forward Loop broken");
            break;
        }
        ESP_LOG(WARN, TAG, "Checks passed on %d:%d", pos.pos_y+i, pos.pos_x);   
    }

    if (!loop_broken && required_cells != 0){
        if (board.board[pos.pos_y+required_cells][pos.pos_x].figure_type != FIGURE_END_LIST && board.board[pos.pos_y+required_cells][pos.pos_x].white == board.board[pos.pos_y][pos.pos_x].white){
            ESP_LOG(WARN, TAG, "Forward same colour figure");
            required_cells--;
        }
    }
    return required_cells;
}

static uint8_t required_cells_calculation_backward(chess_board_t board, figure_position_t pos){
    uint8_t required_cells = 0;
    bool loop_broken = false;
    
    for (int i = 1; !(board.board[pos.pos_y-i][pos.pos_x].figure_type != FIGURE_END_LIST); i++){
        required_cells++;
        if(pos.pos_y-i >= MATRIX_Y){ // TODO: double check this as well
            required_cells--;
            loop_broken = true;
            ESP_LOG(ERROR, TAG, "Backward Loop broken");
            break;
        }
        ESP_LOG(WARN, TAG, "Checks passed on %d:%d", pos.pos_y-i, pos.pos_x);   
    }

    if (!loop_broken && required_cells != 0){
        if (board.board[pos.pos_y-required_cells][pos.pos_x].figure_type != FIGURE_END_LIST && board.board[pos.pos_y-required_cells][pos.pos_x].white == board.board[pos.pos_y][pos.pos_x].white){
            ESP_LOG(WARN, TAG, "Backward same colour figure");
            required_cells--;
        }
    }
    return required_cells;
}

static uint8_t required_cells_calculation_right(chess_board_t board, figure_position_t pos){
    uint8_t required_cells = 0;
    bool loop_broken = false;
    
    for (int i = 1; !(board.board[pos.pos_y][pos.pos_x+i].figure_type != FIGURE_END_LIST); i++){
        required_cells++;
        if(pos.pos_x+i >= MATRIX_X){ // TODO: double check this as well
            required_cells--;
            loop_broken = true;
            ESP_LOG(ERROR, TAG, "Right Loop broken");
            break;
        }
        ESP_LOG(WARN, TAG, "Checks passed on %d:%d", pos.pos_y, pos.pos_x+i);   
    }

    if (!loop_broken && required_cells != 0){
        if (board.board[pos.pos_y][pos.pos_x+required_cells].figure_type != FIGURE_END_LIST && board.board[pos.pos_y][pos.pos_x+required_cells].white == board.board[pos.pos_y][pos.pos_x].white){
            ESP_LOG(WARN, TAG, "Right same colour figure");
            required_cells--;
        }
    }
    return required_cells;
}

static uint8_t required_cells_calculation_left(chess_board_t board, figure_position_t pos){
    uint8_t required_cells = 0;
    bool loop_broken = false;
    
    for (int i = 1; !(board.board[pos.pos_y][pos.pos_x-i].figure_type != FIGURE_END_LIST); i++){
        required_cells++;
        if(pos.pos_x-i >= MATRIX_X){ // TODO: double check this as well
            required_cells--;
            loop_broken = true;
            ESP_LOG(ERROR, TAG, "Left Loop broken");
            break;
        }
        ESP_LOG(WARN, TAG, "Checks passed on %d:%d", pos.pos_y, pos.pos_x+i);   
    }

    if (!loop_broken && required_cells != 0){
        if (board.board[pos.pos_y][pos.pos_x-required_cells].figure_type != FIGURE_END_LIST && board.board[pos.pos_y][pos.pos_x+required_cells].white == board.board[pos.pos_y][pos.pos_x].white){
            ESP_LOG(WARN, TAG, "Left same colour figure");
            required_cells--;
        }
    }
    return required_cells;
}

static esp_err_t populate_led_array(uint8_t *array, uint8_t fw, uint8_t bc, uint8_t r, uint8_t l, figure_position_t pos){

    if (!array){
        ESP_LOG(ERROR, TAG, "Null array ptr. Aborting");
        return ESP_FAIL;
    }

    uint8_t counter = 0;

    
        for (int i = 1; i <= fw; i++){
            array[counter] = (((pos.pos_y+i)*MATRIX_Y) + pos.pos_x);
            ESP_LOG(INFO, TAG, "y %d x %d calc %d", pos.pos_y, pos.pos_x, (((pos.pos_y+i)*MATRIX_Y) + pos.pos_x));
            counter++;
        }
    
    
        for (int i = 1; i <= bc; i++){
            array[counter] = (((pos.pos_y-i)*MATRIX_Y) + pos.pos_x);
            counter++;
        }
    

    
        for (int i = 1; i <= r; i++){
            array[counter] = ((pos.pos_y*MATRIX_Y) + (pos.pos_x+i));
            counter++;
        }
    

    
        for (int i = 1; i <= l; i++){
            array[counter] = ((pos.pos_y*MATRIX_Y) + (pos.pos_x-i));
            counter++;
        }
    
    ESP_LOG(INFO, TAG, "%d position filled", counter);

    for (uint8_t i = 0; i < counter; i++){
        ESP_LOG(INFO, TAG, "populated with: %d", array[i]);
    }

    return ESP_OK;
}


static esp_err_t rook_led_calculation(figure_position_t pos){
    
    chess_board_t board;

    if(access_lock()){
        board = local_data.board;
        release_lock();
    } else {
        return ESP_FAIL;
    }

    ESP_LOG(INFO, TAG, "Calculating Rook");

    uint8_t forward_cells = 0;
    uint8_t backward_cells = 0;
    uint8_t right_cells = 0;
    uint8_t left_cells = 0;

    forward_cells = required_cells_calculation_forward(board, pos);
    backward_cells = required_cells_calculation_backward(board, pos);
    right_cells = required_cells_calculation_right(board, pos);
    left_cells = required_cells_calculation_left(board, pos);

    uint8_t total_cells = forward_cells+backward_cells+right_cells+left_cells;

    ESP_LOG(INFO, TAG, "Calculated: fw %d bc %d r %d l %d. Sum: %d", forward_cells, backward_cells, right_cells, left_cells, total_cells);

    ESP_LOG(WARN, TAG, "MALLOC");
    
    uint8_t *arr_ptr = {0};

    arr_ptr = calloc(total_cells, sizeof(uint8_t));

    if (!arr_ptr){
        ESP_LOG(ERROR, TAG, "Failed to malloc. Aborting");
        arr_ptr = NULL;
        return ESP_FAIL;
    }

    populate_led_array(arr_ptr, forward_cells, backward_cells, right_cells, left_cells, pos);

    local_data.board.board[pos.pos_y][pos.pos_x].led_op(arr_ptr, total_cells);

    
    if (arr_ptr){
        ESP_LOG(WARN, TAG, "FREE");
        free(arr_ptr);
    }
    return ESP_OK;
}

static esp_err_t knight_led_calculation(figure_position_t pos) {
    chess_board_t board;

    if(access_lock()){
        board = local_data.board;
        release_lock();
    } else {
        return ESP_FAIL;
    }

    ESP_LOG(INFO, TAG, "Calculating Knight");

    uint8_t array[MAX_KNIGHT_MOVES];
    uint8_t empty_cells = 0;

    uint8_t one = 1;
    uint8_t two = 2;

    
    if ((uint8_t)(pos.pos_y + 2) < MATRIX_Y){
        
        if ((uint8_t)(pos.pos_x + 1) < MATRIX_X){
            
            if (board.board[pos.pos_y+2][pos.pos_x+1].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y+2][pos.pos_x+1].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y+2 x +1");
                    ESP_LOG(WARN, TAG, "Y + 2 x + 1");
                    array[empty_cells] = (((pos.pos_y+2)*MATRIX_Y) + pos.pos_x+1);
                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y + 2 x + 1");
                array[empty_cells] = (((pos.pos_y+2)*MATRIX_Y) + pos.pos_x+1);
                empty_cells++;
            }
        }   
        if ((uint8_t)(pos.pos_x - 1) < MATRIX_X) {
            if (board.board[pos.pos_y+2][pos.pos_x-1].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y+2][pos.pos_x-1].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y+2 x-1");
                    ESP_LOG(WARN, TAG, "Y + 2 x - 1");
                    array[empty_cells] = (((pos.pos_y+2)*MATRIX_Y) + pos.pos_x-1);
                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y + 2 x - 1");
                array[empty_cells] = (((pos.pos_y+2)*MATRIX_Y) + pos.pos_x-1);
                empty_cells++;
            }
        }
    }

    
    if ((uint8_t)(pos.pos_y - 2) < MATRIX_Y){
        
        if ((uint8_t)(pos.pos_x + 1) < MATRIX_X){
            if (board.board[pos.pos_y-2][pos.pos_x+1].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y-2][pos.pos_x+1].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y-2 x +1");
                    array[empty_cells] = (((pos.pos_y-2)*MATRIX_Y) + pos.pos_x+1);
                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y - 2 x + 1");
                array[empty_cells] = (((pos.pos_y-2)*MATRIX_Y) + pos.pos_x+1);
                empty_cells++;
            }
        }
        if ((uint8_t)(pos.pos_x - 1) < MATRIX_X) {
            if (board.board[pos.pos_y-2][pos.pos_x-1].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y-2][pos.pos_x-1].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y-2 x-1");
                    array[empty_cells] = (((pos.pos_y-2)*MATRIX_Y) + pos.pos_x-1);
                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y - 2 x - 1");
                array[empty_cells] = (((pos.pos_y-2)*MATRIX_Y) + pos.pos_x-1);
                empty_cells++;
            }
        }
    }

    
    if ((uint8_t)(pos.pos_x + 2) < MATRIX_X){
        if ((uint8_t)(pos.pos_y + 1) < MATRIX_Y){
            if (board.board[pos.pos_y+1][pos.pos_x+2].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y+1][pos.pos_x+2].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y+1 x+2");
                    array[empty_cells] = (((pos.pos_y+1)*MATRIX_Y) + pos.pos_x+2);
                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y + 1 x + 2");
                array[empty_cells] = (((pos.pos_y+1)*MATRIX_Y) + pos.pos_x+2);
                empty_cells++;
            }
        }
        if ((uint8_t)(pos.pos_y - 1) < MATRIX_Y){
            if (board.board[pos.pos_y-1][pos.pos_x+2].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y-1][pos.pos_x+2].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y-1 x+2");
                    array[empty_cells] = (((pos.pos_y-1)*MATRIX_Y) + pos.pos_x+2);
                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y - 1 x + 2");
                array[empty_cells] = (((pos.pos_y-1)*MATRIX_Y) + pos.pos_x+2);
                empty_cells++;
            }
        }
    }

    
    if ((uint8_t)(pos.pos_x - 2) < MATRIX_X){
        if ((uint8_t)(pos.pos_y + 1) < MATRIX_Y){
            if (board.board[pos.pos_y+1][pos.pos_x-2].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y+1][pos.pos_x-2].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y+1 x-2");
                    array[empty_cells] = (((pos.pos_y+1)*MATRIX_Y) + pos.pos_x-2);
                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y + 1 x - 2");
                array[empty_cells] = (((pos.pos_y+1)*MATRIX_Y) + pos.pos_x-2);
                empty_cells++;
            }
        }
        if ((uint8_t)(pos.pos_y - 1) < MATRIX_Y){
            if (board.board[pos.pos_y-1][pos.pos_x-2].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y-1][pos.pos_x-2].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y-1 x-2");
                    array[empty_cells] = (((pos.pos_y-1)*MATRIX_Y) + pos.pos_x-2);
                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y - 1 x - 2");
                array[empty_cells] = (((pos.pos_y-1)*MATRIX_Y) + pos.pos_x-2);
                empty_cells++;
            }
        }
    }

    ESP_LOG(INFO, TAG, "Calculated: %d", empty_cells);

    for (uint8_t i = 0; i < empty_cells; i++){
        ESP_LOG(INFO, TAG, "Found position: %d", array[i]);
    }

    if (empty_cells > 0) {
        local_data.board.board[pos.pos_y][pos.pos_x].led_op(array, empty_cells);
    }

    

    return ESP_OK;
}

static esp_err_t required_leds_calculation(figure_position_t updated_pos){
    
    chess_figures_t current_figure;

    if (access_lock()){
        current_figure = local_data.board.board[updated_pos.pos_y][updated_pos.pos_x].figure_type;
        release_lock();
    } else {
        return ESP_FAIL;
    }
    switch (current_figure){
        case FIGURE_PAWN:
            if (pawn_led_calculation(updated_pos) != ESP_OK){

            }
            break;
        case FIGURE_ROOK:
            if (rook_led_calculation(updated_pos) != ESP_OK) {

            }
            break;
        case FIGURE_KNIGHT:
            if (knight_led_calculation(updated_pos) != ESP_OK){

            }
            break;

        default:
            ESP_LOG(ERROR, TAG, "Incorrect figure type. Aborting");
        }


    return ESP_OK;
}

static void chess_engine_task(void *args){

    

    figure_position_t updated_pos;

    ESP_LOG(ERROR, TAG, "Sizeof board struct: %d", sizeof(chess_board_t));

    while(1){

        if (uxQueueMessagesWaiting(local_data.queue) != 0){
            if (chess_queue_receiver(&updated_pos) != ESP_OK){

            } else {
                //updated_pos = (figure_position_t*)queue_buffer;
                ESP_LOG(WARN, TAG, "Received positions: %d:%d", updated_pos.pos_y, updated_pos.pos_x);

                
                required_leds_calculation(updated_pos);
            }
        }

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }

}


esp_err_t update_board_on_lift(figure_position_t pos) {

    if (access_lock()) {
        
        if (xQueueSend(local_data.queue, (void*)&pos, (TickType_t)10) != pdTRUE) {
            ESP_LOG(ERROR, TAG, "Failed to post pos x to queue. Aborting");
            return ESP_FAIL;
        }
        xEventGroupSetBits(local_data.event_handle, FIGURE_LIFTED_BIT_0); // TODO: think if it's needed here at all
        release_lock();
        ESP_LOG(INFO, TAG, "Posted to queue successfully");
    } else {
        ESP_LOG(ERROR, TAG, "Failed to access lock");
        return ESP_FAIL;
    }

    return ESP_OK;

}


static bool access_lock(){

    if (!local_data.initialised){
        ESP_LOG(ERROR, TAG, "Chess Engine service is not initialised.");
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
        ESP_LOG(ERROR, TAG, "Chess Engine service is not initialised.");
        return false;
    }

    if (xSemaphoreGive(local_data.lock) == pdTRUE){
        return true;
    }
    ESP_LOG(ERROR, TAG, "Failed to release lock");
    return false;
}