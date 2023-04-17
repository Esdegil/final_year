#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "esp_heap_trace.h"
#include "chess_engine.h"


#define TAG "CHESS_ENGINE"
#define TASK_NAME "chess_engine_task"

#define POSITION_CALCULATION_ERROR 255

#define BACKWARD_MOD -1
#define FORWARD_MOD 1
#define LEFT_MOD -1
#define RIGHT_MOD 1

#define HORISONTAL_LOCK 1
#define VERTICAL_LOCK 1

#define NO_LIMIT_FOR_MOVES 0

#define MAX_ATTACK_MOVES_ROOK 4
#define MAX_ATTACK_MOVES_PAWN 2
#define MAX_ATTACK_MOVES_KNIGHT 8
#define MAX_ATTACK_MOVES_QUEEN 8
#define MAX_ATTACK_MOVES_KING 8
#define MAX_ATTACK_MOVES_BISHOP 4 

#define BOARD_PAWN_START_LINE_WHITE 1
#define BOARD_PAWN_START_LINE_BLACK (MATRIX_Y - 2)

#define BOARD_MAIN_FIGURE_START_BLACK (MATRIX_Y - 1)
#define BOARD_MAIN_FIGURE_START_WHITE 0

#define CALCULATIONS_WITHOUT_LEDS false
#define CALCULATIONS_WITH_LEDS true

typedef struct local_data{

    bool initialised;

    TaskHandle_t task_handle;

    SemaphoreHandle_t lock;

    chess_board_t board;

    EventGroupHandle_t event_handle;

    QueueHandle_t queue; // TODO: maybe rename this 

    state_change_data_t last_change_data;

    attackable_figures_t *current_attackable;
    uint8_t counter_attackable;

    figure_position_t possible_king_move;

    uint8_t *check_trajectory;
    uint8_t trajectory_counter;

    bool check;
    bool checkmate;

    uint8_t *temp_led_array;
    uint8_t temp_counter;

} local_data_t;

static local_data_t local_data;

static void chess_engine_task(void *args);
static bool access_lock();
static bool release_lock();

static esp_err_t required_leds_calculation(figure_position_t updated_pos, bool show_leds, bool check_calculations);

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


    xTaskCreate(chess_engine_task, TASK_NAME, configMINIMAL_STACK_SIZE*6, &params, tskIDLE_PRIORITY, &local_data.task_handle);

    if (local_data.task_handle == NULL){
        ESP_LOG(ERROR, TAG, "Failed to create task: %s. Aborting.", TASK_NAME);
        goto DELETE_TASK;
    }

    local_data.event_handle = xEventGroupCreate(); // TODO: add checks and removes here
    local_data.queue = xQueueCreate(3, sizeof(state_change_data_t));// TODO: add checks and removes here

    ESP_LOG(ERROR, TAG, "FULL BOARD? %d", FULL_BOARD);
    local_data.check = false;

    // Init on impossible positions to allow 0:0 to be moved at first turn
    local_data.last_change_data.pos.pos_y = MATRIX_Y+1;
    local_data.last_change_data.pos.pos_x = MATRIX_X+1;

    local_data.checkmate = false;

    if (FULL_BOARD){
        // TODO: later fill this
    } else { // assuming 3x3 prototype

        local_data.board.white_turn = true;

    // White figures init
        local_data.board.board[0][3].figure_type = FIGURE_KING;
        local_data.board.board[0][0].figure_type = FIGURE_END_LIST;
        local_data.board.board[0][1].figure_type = FIGURE_ROOK;
        local_data.board.board[0][2].figure_type = FIGURE_END_LIST;

        local_data.board.board[0][0].white = true;
        local_data.board.board[0][1].white = true;
        local_data.board.board[0][2].white = true;
        local_data.board.board[0][3].white = true;


        local_data.board.board[0][0].led_op = &led_op_pawn;
        local_data.board.board[0][1].led_op = &led_op_pawn;
        local_data.board.board[0][2].led_op = &led_op_pawn;
        local_data.board.board[0][3].led_op = &led_op_pawn;


        

    // Black figures init
        local_data.board.board[3][0].figure_type = FIGURE_KING;
        local_data.board.board[3][1].figure_type = FIGURE_END_LIST;
        local_data.board.board[3][2].figure_type = FIGURE_END_LIST;
        local_data.board.board[3][3].figure_type = FIGURE_END_LIST;

        local_data.board.board[3][0].white = false;
        local_data.board.board[3][1].white = false;
        local_data.board.board[3][2].white = false;
        local_data.board.board[3][3].white = false;


        local_data.board.board[3][0].led_op = &led_op_pawn;
        local_data.board.board[3][1].led_op = &led_op_pawn;
        local_data.board.board[3][2].led_op = &led_op_pawn;
        local_data.board.board[3][3].led_op = &led_op_pawn;

    // Rest of the board

        /*
        local_data.board.board[1][0].figure_type = FIGURE_END_LIST;
        local_data.board.board[1][1].figure_type = FIGURE_END_LIST;
        local_data.board.board[1][2].figure_type = FIGURE_END_LIST;


        local_data.board.board[1][1].white = true;

        local_data.board.board[1][1].led_op = &led_op_pawn;

        local_data.board.board[1][0].pos_x = 0;
        local_data.board.board[1][0].pos_y = 1;

        local_data.board.board[1][1].pos_x = 1;
        local_data.board.board[1][1].pos_y = 1;

        local_data.board.board[1][2].pos_x = 2;
        local_data.board.board[1][2].pos_y = 1;
        */


        for (int i = 1; i < 3; i++){
            for (int j = 0; j < 4; j++){
                local_data.board.board[i][j].figure_type = FIGURE_END_LIST;
                // TODO: maybe expand this
            }
        }

        /*
        local_data.board.board[2][2].figure_type = FIGURE_PAWN;

        local_data.board.board[2][2].white = false;

        local_data.board.board[2][2].led_op = &led_op_pawn;
        */

        /*
        local_data.board.board[1][2].figure_type = FIGURE_ROOK;

        local_data.board.board[1][2].white = true;

        local_data.board.board[1][3].led_op = &led_op_pawn;
        */


        
        local_data.board.board[1][2].figure_type = FIGURE_ROOK;

        local_data.board.board[1][2].white = true;

        local_data.board.board[1][2].led_op = &led_op_pawn;
        

        local_data.board.board[2][0].figure_type = FIGURE_QUEEN;

        local_data.board.board[2][0].white = false;

        local_data.board.board[2][0].led_op = &led_op_pawn;

        /*
        local_data.board.board[2][3].figure_type = FIGURE_BISHOP;

        local_data.board.board[2][3].white = false;

        local_data.board.board[2][3].led_op = &led_op_pawn;
        */

    }


    local_data.initialised = true;


    return ESP_OK;

DELETE_TASK:
    vTaskDelete(local_data.task_handle);

    return ESP_FAIL;
}

static esp_err_t chess_queue_receiver(state_change_data_t* buffer) {

    if (access_lock()) {
        xQueueReceive(local_data.queue, buffer, 0);

        ESP_LOG(ERROR, TAG, "Received in chess_queue_receiver: %d:%d bool %d", (buffer->pos.pos_y), (buffer->pos.pos_x), (buffer->lifted));

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

static esp_err_t pawn_attacks(chess_board_t board, figure_position_t pos, bool realloc_needed, uint8_t modified_y, int8_t hor_mod, uint8_t *led_array_ptr, uint8_t *led_array_counter, attackable_figures_t *figures_ptr, uint8_t *figures_counter) {
    
    uint8_t modified_x = 0;
    uint8_t converted_pos = 0;
    
    ESP_LOG(INFO, TAG, "x mod %d for pos %d:%d", hor_mod, pos.pos_y, pos.pos_x);

    if ((uint8_t)(pos.pos_x + hor_mod) < MATRIX_X){
            modified_x = (uint8_t)(pos.pos_x + hor_mod);
            ESP_LOG(INFO, TAG, "Mody %d modx %d",modified_y, modified_x);
            if (board.board[modified_y][modified_x].figure_type != FIGURE_END_LIST && board.board[modified_y][modified_x].white != board.board[pos.pos_y][pos.pos_x].white) {
                ESP_LOG(INFO, TAG, "Left attack possible on pos %d:%d with counter %d", modified_y, modified_x, *led_array_counter);
                
                //ESP_LOG(WARN, TAG, " no *%p one * %p two * %d addr %p + counter %d", led_array_ptr, *led_array_ptr, **led_array_ptr, &led_array_ptr, **(led_array_counter + *led_array_counter));
                
                /*
                if (realloc_needed){
                    ESP_LOG(WARN, TAG, "Realloc needed");
                    *led_array_ptr = (uint8_t*)realloc(*led_array_ptr, (((*led_array_counter)+2)*sizeof(uint8_t))); // TODO: double check this. Might fail

                    if (!*led_array_ptr || !led_array_ptr) {
                        ESP_LOG(ERROR, TAG, "REALLOC failed");
                        if (*figures_ptr){
                            ESP_LOG(WARN, TAG, "free figures_ptr");
                            free(*figures_ptr);
                            *figures_ptr = NULL;
                        }
                        return ESP_FAIL;
                    } else {
                        ESP_LOG(INFO, TAG, "realloc successful allocated %d + 1", *led_array_counter);
                    }


                }
                */
                figures_ptr->figure = board.board[modified_y][modified_x].figure_type;
                figures_ptr->pos.pos_y = modified_y;
                figures_ptr->pos.pos_x = modified_x;
                (*figures_counter)++;

                converted_pos = MATRIX_TO_ARRAY_CONVERSION((modified_y),(modified_x));
                led_array_ptr[*led_array_counter] = converted_pos;
                ESP_LOG(INFO, TAG, "Added converted %d", led_array_ptr[*led_array_counter]);
                (*led_array_counter)++;

            }
            
        }
    return ESP_OK;
        
}

static void put_converted_into_array(uint8_t *led_array_ptr, uint8_t converted, uint8_t *counter){
    led_array_ptr[*counter] = converted;
    (*counter)++;
}

// TODO: maybe redo this fully
static esp_err_t pawn_led_calculation(figure_position_t pos, uint8_t **led_array_ptr, uint8_t *counter) {

    chess_board_t board;

    if(access_lock()){
        board = local_data.board;
        release_lock();
    } else {
        return ESP_FAIL;
    }

    ESP_LOG(INFO, TAG, "Pawn calculations");

    bool attack_possible = false;
    uint8_t attackable_counter = 0;
    attackable_figures_t *figures_ptr = (attackable_figures_t*)calloc(4, sizeof(attackable_figures_t));
    ESP_LOG(WARN, TAG,"MALLOC");
    if (!figures_ptr) {
        ESP_LOG(ERROR, TAG, "Failed to allocate memory. Pawn.");
        // TODO: return?
    }
    
    uint8_t possible_forward_moves = 1;
    int8_t mod_y = 0;

    if (board.board[pos.pos_y][pos.pos_x].white){

        mod_y = FORWARD_MOD;

        if (pos.pos_y == BOARD_PAWN_START_LINE_WHITE ){
            possible_forward_moves++;
        }

    } else {

        if (pos.pos_y == BOARD_PAWN_START_LINE_BLACK) {
            possible_forward_moves++;
        }

        mod_y = BACKWARD_MOD;

    }

    

    ESP_LOG(INFO, TAG, "Max forward moves %d for this pawn %d:%d", possible_forward_moves, pos.pos_y, pos.pos_x);

    uint8_t modified_y = 0;
    uint8_t converted_pos = 0;

    if (*led_array_ptr){
        ESP_LOG(ERROR, TAG, "led_array_ptr is not null for some reason");
        free(*led_array_ptr);
        *led_array_ptr = NULL;
    }
    if (*counter != 0){
        ESP_LOG(WARN, TAG, "counter is not 0. Setting it to 0");
        *counter = 0;
    }

    ESP_LOG(WARN, TAG, "CALLOC led_array_ptr");
    *led_array_ptr = (uint8_t*)calloc((possible_forward_moves+3), sizeof(uint8_t));

    if (!led_array_ptr){
        ESP_LOG(ERROR, TAG, "led_array_ptr is NULL after calloc");
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "free figures_ptr");
            free(figures_ptr);
            figures_ptr = NULL;
        }
        return ESP_FAIL;
    }


    // Moving forwards for black and white pawns
    for (int i = 1; i < possible_forward_moves + 1; i++){
        modified_y = (uint8_t)(pos.pos_y + (i * mod_y));
        ESP_LOG(DEBUG, TAG, "modified y %d on i %d", modified_y, i); 
        if (modified_y >= MATRIX_Y) {
            ESP_LOG(WARN, TAG, "Pawn reached opposite starting line. Implement turning it into a queen");
            break;
        }
        if (board.board[modified_y][pos.pos_x].figure_type == FIGURE_END_LIST){
            converted_pos = MATRIX_TO_ARRAY_CONVERSION((modified_y), (pos.pos_x));
            //*led_array_ptr[*counter] = converted_pos;
            put_converted_into_array(*led_array_ptr, converted_pos, counter);
            ESP_LOG(INFO, TAG, "pawn move %d:%d is valid", modified_y, pos.pos_x);
            //(*counter)++;
        }
    }

    uint8_t modified_x = 0;
    modified_y = (uint8_t)(pos.pos_y + mod_y);

    if (modified_y < MATRIX_Y){
        bool realloc_needed = *counter >= possible_forward_moves;
        if (pawn_attacks(board, pos, realloc_needed, modified_y, LEFT_MOD, *led_array_ptr, counter, &figures_ptr[attackable_counter], &attackable_counter) != ESP_OK){
            //free stuff
            return ESP_FAIL;
        }

        if (pawn_attacks(board, pos, realloc_needed, modified_y, RIGHT_MOD, *led_array_ptr, counter, &figures_ptr[attackable_counter], &attackable_counter) != ESP_OK){
            //free stuff
            return ESP_FAIL;
        }
    }
    
    if (attackable_counter != 0) {
        if (access_lock()){

            if (local_data.current_attackable){
                ESP_LOG(WARN, TAG, "FREE local_data.current_attackable");
                free(local_data.current_attackable);
                local_data.current_attackable = NULL;
                
            }

            local_data.counter_attackable = *counter;
            local_data.current_attackable = figures_ptr;

            release_lock();
        } else {
            ESP_LOG(ERROR, TAG, "Failed to access lock when saving attackable figures.");
            if (figures_ptr) {
                ESP_LOG(WARN, TAG, "FREE figures_ptr");
                free(figures_ptr);
                figures_ptr = NULL;
            }
        }
    } else {
        if (access_lock()){
            
            ESP_LOG(WARN, TAG, "FREE local_data.current_attackable");
            if (local_data.current_attackable) {
                free(local_data.current_attackable);
                local_data.current_attackable = NULL;
            }
            local_data.counter_attackable = 0;

            release_lock();
        } else {
            ESP_LOG(ERROR, TAG, "Failed to access lock when freeing local_data.current_attackable");
            local_data.counter_attackable = 0;
            // TODO: return
        }
    }


    return ESP_OK;
}

static uint8_t required_cells_calculation_vertical(chess_board_t board, figure_position_t pos, int8_t mod_y, uint8_t limit, bool *attack_possible, attackable_figures_t *figure){
    ESP_LOG(DEBUG, TAG, "Mods passed y:%d", mod_y)

    if (mod_y != FORWARD_MOD && mod_y != BACKWARD_MOD){
        ESP_LOG(ERROR, TAG, "Incorrect mod_y modificator passed. returning 255");
        return POSITION_CALCULATION_ERROR;
    }

    uint8_t updated_limit = NO_LIMIT_FOR_MOVES;
    if (limit == 0) {
        updated_limit = MATRIX_Y + 1;
    } else {
        updated_limit = limit;
    }

    ESP_LOG(DEBUG, TAG, "Updated limit: %d", updated_limit);

    uint8_t required_cells = 0; 
    bool loop_broken = false;


    for (int i = 1; !(board.board[pos.pos_y + (i * mod_y)][pos.pos_x].figure_type != FIGURE_END_LIST) && i <= updated_limit; i++){
        ESP_LOG(DEBUG, TAG, "Current i: %d limit %d", i, updated_limit);
        required_cells++;
        if((uint8_t)(pos.pos_y + (i*mod_y)) >= MATRIX_Y){ // TODO: double check this as well
            required_cells--;
            loop_broken = true;
            ESP_LOG(ERROR, TAG, "Left Loop broken");
            break;
        }
        ESP_LOG(WARN, TAG, "Checks passed on %d:%d", pos.pos_y + (i * mod_y), pos.pos_x);

    }

    if (!loop_broken && required_cells != 0){

        uint8_t adj_modified_y = pos.pos_y + ((required_cells + 1) * mod_y);

        ESP_LOG(DEBUG, TAG, "Modified uint8 y %d", adj_modified_y);

        if (board.board[pos.pos_y + (required_cells * mod_y)][pos.pos_x].figure_type != FIGURE_END_LIST && board.board[pos.pos_y + (required_cells * mod_y)][pos.pos_x].white == board.board[pos.pos_y][pos.pos_x].white){
            ESP_LOG(WARN, TAG, "same colour figure");
            required_cells--;
        } else if (adj_modified_y < MATRIX_Y){
            ESP_LOG(WARN, TAG, "Not out of bounds vertical");
            if (board.board[pos.pos_y + ((required_cells + 1) * mod_y)][pos.pos_x].figure_type != FIGURE_END_LIST && updated_limit != 1) { 
                if (board.board[pos.pos_y + ((required_cells + 1) * mod_y)][pos.pos_x].white != board.board[pos.pos_y][pos.pos_x].white) {
                    ESP_LOG(INFO, TAG, "//////////////////// Enemy figure on %d:%d", pos.pos_y + ((required_cells + 1) * mod_y), pos.pos_x );
                    required_cells++;
                    *attack_possible = true;
                    figure->pos.pos_y = pos.pos_y + (required_cells * mod_y);
                    figure->pos.pos_x = pos.pos_x;
                    figure->figure = board.board[figure->pos.pos_y][figure->pos.pos_x].figure_type;
                    
                    
                }
            }
        }
    } else if (!loop_broken && required_cells == 0) {

        uint8_t adj_modified_y = pos.pos_y + mod_y;

        ESP_LOG(DEBUG, TAG, "Modified uint8 y %d", adj_modified_y);

        if (adj_modified_y >= MATRIX_X){
            ESP_LOG(INFO, TAG, "out of x bounds");
        } else if (board.board[adj_modified_y][pos.pos_x].figure_type != FIGURE_END_LIST){

            ESP_LOG(DEBUG, TAG, "adj mod y:%d. Y position after mod applied %d",mod_y, adj_modified_y);

            

            if(board.board[adj_modified_y][pos.pos_x].white != board.board[pos.pos_y][pos.pos_x].white){ // TODO: made changes in adj_modified_y was posy + adj_modified_y
                ESP_LOG(WARN, TAG, "Enemy figure colour %s adjacent on pos %d:%d", board.board[adj_modified_y][pos.pos_x].white ? "white" : "black", adj_modified_y, pos.pos_x);
                required_cells++;

                *attack_possible = true;
                figure->pos.pos_y = adj_modified_y; // TODO: REALLY CHECK THIS
                figure->pos.pos_x = pos.pos_x;
                figure->figure = board.board[figure->pos.pos_y][figure->pos.pos_x].figure_type;
            }

        }
        
    }

    return required_cells;


}

static uint8_t required_cells_calculation_horisontal(chess_board_t board, figure_position_t pos, int8_t mod_x, uint8_t limit, bool *attack_possible, attackable_figures_t *figure) {

    ESP_LOG(DEBUG, TAG, "Mods passed x:%d", mod_x)

    if (mod_x != RIGHT_MOD && mod_x != LEFT_MOD){
        ESP_LOG(ERROR, TAG, "Incorrect mod_x modificator passed. returning 255");
        return POSITION_CALCULATION_ERROR;
    }

    uint8_t updated_limit = NO_LIMIT_FOR_MOVES;
    if (limit == 0) {
        updated_limit = MATRIX_Y + 1;
    } else {
        updated_limit = limit;
    }

    uint8_t required_cells = 0; 
    bool loop_broken = false;


    for (int i = 1; !(board.board[pos.pos_y][pos.pos_x + (i * mod_x)].figure_type != FIGURE_END_LIST) && i <= updated_limit; i++){

        required_cells++;
        if((uint8_t)(pos.pos_x + (i*mod_x)) >= MATRIX_X){ // TODO: double check this as well
            required_cells--;
            loop_broken = true;
            ESP_LOG(ERROR, TAG, "Left Loop broken");
            break;
        }
        ESP_LOG(WARN, TAG, "Checks passed on %d:%d", pos.pos_y, pos.pos_x+ (i * mod_x));

    }

    if (!loop_broken && required_cells != 0){

        uint8_t adj_modified_x = pos.pos_x + ((required_cells + 1) * mod_x);

        ESP_LOG(DEBUG, TAG, "Modified uint8 x %d",adj_modified_x);

        if (board.board[pos.pos_y][pos.pos_x + (required_cells * mod_x)].figure_type != FIGURE_END_LIST && board.board[pos.pos_y][pos.pos_x + (required_cells * mod_x)].white == board.board[pos.pos_y][pos.pos_x].white){
            ESP_LOG(WARN, TAG, "same colour figure");
            required_cells--;
        } else if (adj_modified_x < MATRIX_X) { 
            ESP_LOG(WARN, TAG, "Not out of bounds horisontal");
            if (board.board[pos.pos_y][pos.pos_x + ((required_cells + 1) * mod_x)].figure_type != FIGURE_END_LIST && updated_limit != 1) { 
                if (board.board[pos.pos_y][pos.pos_x + ((required_cells + 1) * mod_x)].white != board.board[pos.pos_y][pos.pos_x].white) {
                    ESP_LOG(INFO, TAG, "//////////////////// Enemy figure on %d:%d", pos.pos_y, pos.pos_x + ((required_cells + 1) * mod_x ));
                    required_cells++;
                    *attack_possible = true;
                    figure->pos.pos_y = pos.pos_y;
                    figure->pos.pos_x = pos.pos_x + ((required_cells) * mod_x );
                    figure->figure = board.board[figure->pos.pos_y][figure->pos.pos_x].figure_type;
                }
            }
        }
    } else if (!loop_broken && required_cells == 0) {

        uint8_t adj_modified_x = pos.pos_x + mod_x;

        ESP_LOG(DEBUG, TAG, "Modified uint8 x %d",adj_modified_x);

        if (adj_modified_x >= MATRIX_X){
            ESP_LOG(INFO, TAG, "out of x bounds");
        } else if (board.board[pos.pos_y][adj_modified_x].figure_type != FIGURE_END_LIST){

            ESP_LOG(DEBUG, TAG, "mod x:%d. X position after mod applied %d",mod_x, adj_modified_x);

            

            if(board.board[pos.pos_y][adj_modified_x].white != board.board[pos.pos_y][pos.pos_x].white){
                ESP_LOG(WARN, TAG, "Enemy figure adjacent");
                required_cells++;
                *attack_possible = true;
                figure->pos.pos_y = pos.pos_y;
                figure->pos.pos_x = adj_modified_x;
                figure->figure = board.board[figure->pos.pos_y][figure->pos.pos_x].figure_type;
            }

        }
        
    }

    return required_cells;


}

static esp_err_t required_cells_calculation_diagonal(chess_board_t board, figure_position_t pos, int mod_x, int mod_y, uint8_t limit,  bool *attack_possible, attackable_figures_t *figure) {
    
    ESP_LOG(DEBUG, TAG, "Mods passed y:%d x:%d", mod_y, mod_x)

    if (mod_x != RIGHT_MOD && mod_x != LEFT_MOD){
        ESP_LOG(ERROR, TAG, "Incorrect mod_x modificator passed :%d. returning 255", mod_x);
        return POSITION_CALCULATION_ERROR;
    }

    if (mod_y != FORWARD_MOD && mod_y != BACKWARD_MOD){
        ESP_LOG(ERROR, TAG, "Incorrect mod_y modificator passed %d. returning 255", mod_y);
        return POSITION_CALCULATION_ERROR;
    }

    uint8_t updated_limit = NO_LIMIT_FOR_MOVES;
    if (limit == 0) {
        updated_limit = MATRIX_Y + 1;
    } else {
        updated_limit = limit;
    }
    
    uint8_t required_cells = 0; 
    bool loop_broken = false;


    for (int i = 1; !(board.board[pos.pos_y + (i * mod_y)][pos.pos_x + (i * mod_x)].figure_type != FIGURE_END_LIST) && i <= updated_limit; i++){

        required_cells++;
        if((uint8_t)(pos.pos_x + (i*mod_x)) >= MATRIX_X || ((uint8_t)(pos.pos_y + (i * mod_y)) >= MATRIX_Y)){ // TODO: double check this as well
            required_cells--;
            loop_broken = true;
            ESP_LOG(ERROR, TAG, "Left Loop broken");
            break;
        }
        ESP_LOG(WARN, TAG, "Checks passed on %d:%d", pos.pos_y + (i * mod_y), pos.pos_x+ (i * mod_x));

    }

    

    

    if (!loop_broken && required_cells != 0){

        uint8_t adj_modified_y = pos.pos_y + ((required_cells + 1) * mod_y);
        uint8_t adj_modified_x = pos.pos_x + ((required_cells + 1) * mod_x);

        ESP_LOG(DEBUG, TAG, "Modified uint8s %d:%d", adj_modified_y,adj_modified_x);

        if (board.board[pos.pos_y + (required_cells * mod_y)][pos.pos_x + (required_cells * mod_x)].figure_type != FIGURE_END_LIST && board.board[pos.pos_y + (required_cells * mod_y)][pos.pos_x + (required_cells * mod_x)].white == board.board[pos.pos_y][pos.pos_x].white){
            ESP_LOG(WARN, TAG, "same colour figure");
            required_cells--;
        } else if (adj_modified_y < MATRIX_Y && adj_modified_x < MATRIX_X){
            ESP_LOG(WARN, TAG, "Not out of bounds diagonal");
            if (board.board[pos.pos_y + ((required_cells + 1) * mod_y)][pos.pos_x + ((required_cells + 1) * mod_x)].figure_type != FIGURE_END_LIST && updated_limit != 1){
                if (board.board[pos.pos_y + ((required_cells + 1) * mod_y)][pos.pos_x + ((required_cells + 1) * mod_x)].white != board.board[pos.pos_y][pos.pos_x].white) {
                    ESP_LOG(INFO, TAG, "//////////////////// Enemy figure on %d:%d", pos.pos_y + ((required_cells + 1) * mod_y), pos.pos_x + ((required_cells + 1) * mod_x ));
                    required_cells++;
                    *attack_possible = true;
                    figure->pos.pos_y = pos.pos_y + (required_cells * mod_y);
                    figure->pos.pos_x = pos.pos_x + (required_cells * mod_x);
                    figure->figure = board.board[figure->pos.pos_y][figure->pos.pos_x].figure_type;
                }
            }
        } 
    } else if (!loop_broken && required_cells == 0) {

        uint8_t adj_modified_y = pos.pos_y + mod_y;
        uint8_t adj_modified_x = pos.pos_x + mod_x;

        ESP_LOG(DEBUG, TAG, "Modified uint8s %d:%d", adj_modified_y,adj_modified_x);

        if (adj_modified_y >= MATRIX_Y || adj_modified_x >= MATRIX_X){
            ESP_LOG(INFO, TAG, "out of bounds");
        } else if (board.board[pos.pos_y + (1 * mod_y)][pos.pos_x + (1 * mod_x)].figure_type != FIGURE_END_LIST){

            ESP_LOG(DEBUG, TAG, "mods y:%d x:%d positions after mods applied %d:%d",mod_y, mod_x,pos.pos_y + (1*mod_y), pos.pos_x + (1*mod_x));

            if(board.board[pos.pos_y + (1 * mod_y)][pos.pos_x + (1 * mod_x)].white != board.board[pos.pos_y][pos.pos_x].white){
                ESP_LOG(WARN, TAG, "Enemy figure adjacent");
                required_cells++;
                *attack_possible = true;
                figure->pos.pos_y = pos.pos_y + mod_y;
                figure->pos.pos_x = pos.pos_x + mod_x;
                figure->figure = board.board[figure->pos.pos_y][figure->pos.pos_x].figure_type;
            }

        }
        
    }

    return required_cells;
}

static esp_err_t populate_led_array_direct(uint8_t *array, uint8_t fw, uint8_t bc, uint8_t r, uint8_t l, figure_position_t pos, uint8_t *counter){

    if (!array || !counter){
        ESP_LOG(ERROR, TAG, "Null array or counter ptr. Aborting");
        return ESP_FAIL;
    }

    ESP_LOG(INFO, TAG, "Filling horisontal and vertical. Base pos %d:%d", pos.pos_y, pos.pos_x);

    
        for (int i = 1; i <= fw; i++){
            //array[*counter] = (((pos.pos_y+i)*MATRIX_Y) + pos.pos_x);
            
            array[*counter] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y+i), pos.pos_x);
            ESP_LOG(WARN, TAG, "FW  i %d filled %d", i, array[*counter]);
            (*counter)++;
        }
    
    
        for (int i = 1; i <= bc; i++){
            //array[*counter] = (((pos.pos_y-i)*MATRIX_Y) + pos.pos_x);
            //ESP_LOG(WARN, TAG, "moded %d:%d. Test: %d", pos.pos_y-i, pos.pos_x, ((pos.pos_y-1) * MATRIX_Y) + pos.pos_x);
            array[*counter] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y-i), pos.pos_x);
            ESP_LOG(WARN, TAG, "BC i %d filled %d", i, array[*counter]);
            (*counter)++;
        }
    

    
        for (int i = 1; i <= r; i++){
            //array[*counter] = ((pos.pos_y*MATRIX_Y) + (pos.pos_x+i));
            array[*counter] = MATRIX_TO_ARRAY_CONVERSION(pos.pos_y, (pos.pos_x+i));
            ESP_LOG(WARN, TAG, "R i %d filled %d", i, array[*counter]);
            (*counter)++;
        }
    

    
        for (int i = 1; i <= l; i++){
            //array[*counter] = ((pos.pos_y*MATRIX_Y) + (pos.pos_x-i));
            array[*counter] = MATRIX_TO_ARRAY_CONVERSION(pos.pos_y, (pos.pos_x-i));
            ESP_LOG(WARN, TAG, "L i %d filled %d", i, array[*counter]);
            (*counter)++;
        }
    
    ESP_LOG(INFO, TAG, "%d position filled", *counter);

    for (uint8_t i = 0; i < *counter; i++){
        ESP_LOG(INFO, TAG, "populated with: %d", array[i]);
    }

    return ESP_OK;
}

static esp_err_t populate_led_array_diagonal(uint8_t* array, uint8_t lf, uint8_t lb, uint8_t rf, uint8_t rb, figure_position_t pos, uint8_t *counter) {

    if (!array || !counter){
        ESP_LOG(ERROR, TAG, "Null array or counter ptr. Aborting");
        return ESP_FAIL;
    }

    ESP_LOG(WARN, TAG, "Current pos %d:%d", pos.pos_y, pos.pos_x);

    ESP_LOG(INFO, TAG, "Filling diagonal");
    
        for (int i = 1; i <= lf; i++){
            array[*counter] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y + i), (pos.pos_x - i));
            ESP_LOG(DEBUG, TAG, "ids %d:%d Calculated lf pos: %d", pos.pos_y + i, pos.pos_x - i, array[*counter]);
            (*counter)++;
        }
    
    
        for (int i = 1; i <= lb; i++){
            array[*counter] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y - i), (pos.pos_x - i));
            ESP_LOG(DEBUG, TAG, "ids %d:%d Calculated lb pos: %d", pos.pos_y - i, pos.pos_x - i, array[*counter]);
            (*counter)++;
        }
    

    
        for (int i = 1; i <= rf; i++){
            array[*counter] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y + i), (pos.pos_x + i));
            ESP_LOG(DEBUG, TAG, "ids %d:%d Calculated rf pos: %d", pos.pos_y + i, pos.pos_x + i, array[*counter]);
            (*counter)++;
        }
    

    
        for (int i = 1; i <= rb; i++){
            array[*counter] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y - i), (pos.pos_x + i));
            ESP_LOG(DEBUG, TAG, "ids %d:%d Calculated rb pos: %d", pos.pos_y - i, pos.pos_x + i, array[*counter]);
            (*counter)++;
        }
    
    ESP_LOG(INFO, TAG, "%d position filled", *counter);

    for (uint8_t i = 0; i < *counter; i++){
        ESP_LOG(INFO, TAG, "populated with: %d", array[i]);
    }

    return ESP_OK;


}


static esp_err_t rook_led_calculation(figure_position_t pos, uint8_t **led_array_ptr, uint8_t *counter){
    
    if (*counter != 0) {
        ESP_LOG(ERROR, TAG, "Counter is %d at the start. Aborting", *counter);
    }

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

    bool attack_possible = false;
    attackable_figures_t *figures_ptr = (attackable_figures_t*)calloc(4, sizeof(attackable_figures_t));
    ESP_LOG(WARN, TAG,"MALLOC");
    if (!figures_ptr) {
        ESP_LOG(ERROR, TAG, "Failed to allocate memory. Rook.");
        // TODO: return?
    }
    

    forward_cells = required_cells_calculation_vertical(board, pos, FORWARD_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    
    if (forward_cells == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }
    

    backward_cells = required_cells_calculation_vertical(board, pos, BACKWARD_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    
    if (backward_cells == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }
    

    right_cells = required_cells_calculation_horisontal(board, pos, RIGHT_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    
    if (right_cells == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }
    
    left_cells = required_cells_calculation_horisontal(board, pos, LEFT_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);

    if (right_cells == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }
    if (*counter != 0) {
        if (access_lock()){

            if (local_data.current_attackable){
                ESP_LOG(WARN, TAG, "FREE local_data.current_attackable");
                free(local_data.current_attackable);
                local_data.current_attackable = NULL;
                
            }

            local_data.counter_attackable = *counter;
            local_data.current_attackable = figures_ptr;

            release_lock();
        } else {
            ESP_LOG(ERROR, TAG, "Failed to access lock when saving attackable figures.");
            if (figures_ptr) {
                ESP_LOG(WARN, TAG, "FREE figures_ptr");
                free(figures_ptr);
                figures_ptr = NULL;
            }
        }
    } else {
        if (access_lock()){
            
            ESP_LOG(WARN, TAG, "FREE local_data.current_attackable");
            if (local_data.current_attackable) {
                free(local_data.current_attackable);
                local_data.current_attackable = NULL;
            }
            local_data.counter_attackable = 0;

            release_lock();
        } else {
            ESP_LOG(ERROR, TAG, "Failed to access lock when freeing local_data.current_attackable");
            local_data.counter_attackable = 0;
            // TODO: return
        }
    }

    uint8_t total_cells = forward_cells+backward_cells+right_cells+left_cells;

    ESP_LOG(INFO, TAG, "Calculated: fw %d bc %d r %d l %d. Sum: %d", forward_cells, backward_cells, right_cells, left_cells, total_cells);

    ESP_LOG(WARN, TAG, "MALLOC");
    
    *led_array_ptr = NULL;

    *led_array_ptr = (uint8_t*)calloc(total_cells, sizeof(uint8_t));

    if (!led_array_ptr){
        ESP_LOG(ERROR, TAG, "Failed to malloc. Aborting");
        led_array_ptr = NULL;
        return ESP_FAIL;
    }

    *counter = 0;

    populate_led_array_direct(*led_array_ptr, forward_cells, backward_cells, right_cells, left_cells, pos, counter);

    ESP_LOG(WARN, TAG, "Array 0 after populating %d", *led_array_ptr[0]);

    *counter = total_cells;

   
    return ESP_OK;
}

static esp_err_t test_function(uint8_t *static_array, uint8_t *dynamic_array, uint8_t counter) {

    for (int i = 0; i < counter; i++){
        ESP_LOG(INFO, TAG, "I %d counter %d", i, counter);
        dynamic_array[i] = static_array[i];
        ESP_LOG(WARN, TAG, "Values dyn %d sta %d", dynamic_array[i], static_array[i]);
    }

    return ESP_OK;
}

static esp_err_t knight_led_calculation(figure_position_t pos, uint8_t **led_array_ptr, uint8_t *counter) {
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

    
    attackable_figures_t *figures_ptr = calloc(MAX_ATTACK_MOVES_KNIGHT, sizeof(attackable_figures_t));
    uint8_t attackable_counter = 0;
    ESP_LOG(WARN, TAG,"MALLOC");
    if (!figures_ptr) {
        ESP_LOG(ERROR, TAG, "Failed to allocate memory. Bishop.");
        // TODO: return?
    }


    if ((uint8_t)(pos.pos_y + 2) < MATRIX_Y){
        
        if ((uint8_t)(pos.pos_x + 1) < MATRIX_X){
            
            if (board.board[pos.pos_y+2][pos.pos_x+1].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y+2][pos.pos_x+1].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y+2 x +1");
                    ESP_LOG(WARN, TAG, "Y + 2 x + 1");
                    //array[empty_cells] = (((pos.pos_y+2)*MATRIX_Y) + pos.pos_x+1);
                    array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y+2), (pos.pos_x+1));
                    figures_ptr[attackable_counter].pos.pos_y = pos.pos_y + 2;
                    figures_ptr[attackable_counter].pos.pos_x = pos.pos_x + 1;
                    figures_ptr[attackable_counter].figure = board.board[pos.pos_y+2][pos.pos_x+1].figure_type;
                    attackable_counter++;

                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y + 2 x + 1");
                //array[empty_cells] = (((pos.pos_y+2)*MATRIX_Y) + pos.pos_x+1);
                array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y+2), (pos.pos_x+1));

                empty_cells++;
            }
        }   
        if ((uint8_t)(pos.pos_x - 1) < MATRIX_X) {
            if (board.board[pos.pos_y+2][pos.pos_x-1].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y+2][pos.pos_x-1].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y+2 x-1");
                    ESP_LOG(WARN, TAG, "Y + 2 x - 1");
                   // array[empty_cells] = (((pos.pos_y+2)*MATRIX_Y) + pos.pos_x-1);
                    array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y+2), (pos.pos_x-1));

                    figures_ptr[attackable_counter].pos.pos_y = pos.pos_y + 2;
                    figures_ptr[attackable_counter].pos.pos_x = pos.pos_x - 1;
                    figures_ptr[attackable_counter].figure = board.board[pos.pos_y+2][pos.pos_x-1].figure_type;
                    attackable_counter++;
                    
                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y + 2 x - 1");
                //array[empty_cells] = (((pos.pos_y+2)*MATRIX_Y) + pos.pos_x-1);
                array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y+2), (pos.pos_x-1));
                    
                empty_cells++;
            }
        }
    }

    
    if ((uint8_t)(pos.pos_y - 2) < MATRIX_Y){
        
        if ((uint8_t)(pos.pos_x + 1) < MATRIX_X){
            if (board.board[pos.pos_y-2][pos.pos_x+1].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y-2][pos.pos_x+1].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y-2 x +1");
                    //array[empty_cells] = (((pos.pos_y-2)*MATRIX_Y) + pos.pos_x+1);
                    array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y-2), (pos.pos_x+1));
                    
                    figures_ptr[attackable_counter].pos.pos_y = pos.pos_y - 2;
                    figures_ptr[attackable_counter].pos.pos_x = pos.pos_x + 1;
                    figures_ptr[attackable_counter].figure = board.board[pos.pos_y-2][pos.pos_x+1].figure_type;
                    attackable_counter++;
                    

                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y - 2 x + 1");
                //array[empty_cells] = (((pos.pos_y-2)*MATRIX_Y) + pos.pos_x+1);
                array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y-2), (pos.pos_x+1));
                    
                empty_cells++;
            }
        }
        if ((uint8_t)(pos.pos_x - 1) < MATRIX_X) {
            if (board.board[pos.pos_y-2][pos.pos_x-1].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y-2][pos.pos_x-1].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y-2 x-1");
                    //array[empty_cells] = (((pos.pos_y-2)*MATRIX_Y) + pos.pos_x-1);
                    array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y-2), (pos.pos_x-1));

                    figures_ptr[attackable_counter].pos.pos_y = pos.pos_y - 2;
                    figures_ptr[attackable_counter].pos.pos_x = pos.pos_x - 1;
                    figures_ptr[attackable_counter].figure = board.board[pos.pos_y-2][pos.pos_x-1].figure_type;
                    attackable_counter++;
                    
                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y - 2 x - 1");
                //array[empty_cells] = (((pos.pos_y-2)*MATRIX_Y) + pos.pos_x-1);
                array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y-2), (pos.pos_x-1));
                    
                empty_cells++;
            }
        }
    }

    
    if ((uint8_t)(pos.pos_x + 2) < MATRIX_X){
        if ((uint8_t)(pos.pos_y + 1) < MATRIX_Y){
            if (board.board[pos.pos_y+1][pos.pos_x+2].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y+1][pos.pos_x+2].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y+1 x+2");
                    //array[empty_cells] = (((pos.pos_y+1)*MATRIX_Y) + pos.pos_x+2);
                    array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y+1), (pos.pos_x+2));

                    figures_ptr[attackable_counter].pos.pos_y = pos.pos_y + 1;
                    figures_ptr[attackable_counter].pos.pos_x = pos.pos_x + 2;
                    figures_ptr[attackable_counter].figure = board.board[pos.pos_y+1][pos.pos_x+2].figure_type;
                    attackable_counter++;
                    
                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y + 1 x + 2");
                //array[empty_cells] = (((pos.pos_y+1)*MATRIX_Y) + pos.pos_x+2);
                array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y+1), (pos.pos_x+2));
                empty_cells++;
            }
        }
        if ((uint8_t)(pos.pos_y - 1) < MATRIX_Y){
            if (board.board[pos.pos_y-1][pos.pos_x+2].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y-1][pos.pos_x+2].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y-1 x+2");
                    //array[empty_cells] = (((pos.pos_y-1)*MATRIX_Y) + pos.pos_x+2);
                    array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y-1), (pos.pos_x+2));

                    figures_ptr[attackable_counter].pos.pos_y = pos.pos_y - 1;
                    figures_ptr[attackable_counter].pos.pos_x = pos.pos_x + 2;
                    figures_ptr[attackable_counter].figure = board.board[pos.pos_y-1][pos.pos_x+2].figure_type;
                    attackable_counter++;
                    
                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y - 1 x + 2");
                //array[empty_cells] = (((pos.pos_y-1)*MATRIX_Y) + pos.pos_x+2);
                array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y-1), (pos.pos_x+2));
                empty_cells++;
            }
        }
    }

    
    if ((uint8_t)(pos.pos_x - 2) < MATRIX_X){
        if ((uint8_t)(pos.pos_y + 1) < MATRIX_Y){
            if (board.board[pos.pos_y+1][pos.pos_x-2].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y+1][pos.pos_x-2].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y+1 x-2");
                    //array[empty_cells] = (((pos.pos_y+1)*MATRIX_Y) + pos.pos_x-2);
                    array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y+1), (pos.pos_x-2));

                    figures_ptr[attackable_counter].pos.pos_y = pos.pos_y + 1;
                    figures_ptr[attackable_counter].pos.pos_x = pos.pos_x - 2;
                    figures_ptr[attackable_counter].figure = board.board[pos.pos_y+1][pos.pos_x-2].figure_type;
                    attackable_counter++;
                    
                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y + 1 x - 2");
               // array[empty_cells] = (((pos.pos_y+1)*MATRIX_Y) + pos.pos_x-2);
                array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y+1), (pos.pos_x-2));
                empty_cells++;
            }
        }
        if ((uint8_t)(pos.pos_y - 1) < MATRIX_Y){
            if (board.board[pos.pos_y-1][pos.pos_x-2].figure_type != FIGURE_END_LIST){
                if (board.board[pos.pos_y-1][pos.pos_x-2].white != board.board[pos.pos_y][pos.pos_x].white){
                    ESP_LOG(WARN, TAG, "Enemy figure y-1 x-2");
                    //array[empty_cells] = (((pos.pos_y-1)*MATRIX_Y) + pos.pos_x-2);
                    array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y-1), (pos.pos_x-2));

                    figures_ptr[attackable_counter].pos.pos_y = pos.pos_y - 1;
                    figures_ptr[attackable_counter].pos.pos_x = pos.pos_x - 2;
                    figures_ptr[attackable_counter].figure = board.board[pos.pos_y-1][pos.pos_x-2].figure_type;
                    attackable_counter++;
                    

                    empty_cells++;
                }
            } else {
                ESP_LOG(WARN, TAG, "Y - 1 x - 2");
                //array[empty_cells] = (((pos.pos_y-1)*MATRIX_Y) + pos.pos_x-2);
                array[empty_cells] = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y-1), (pos.pos_x-2));
                empty_cells++;
            }
        }
    }

     if (attackable_counter != 0) {
        if (access_lock()){

            if (local_data.current_attackable){
                ESP_LOG(WARN, TAG, "FREE local_data.current_attackable");
                free(local_data.current_attackable);
                local_data.current_attackable = NULL;
                
            }

            local_data.counter_attackable = attackable_counter;
            local_data.current_attackable = figures_ptr;

            release_lock();
        } else {
            ESP_LOG(ERROR, TAG, "Failed to access lock when saving attackable figures.");
            if (figures_ptr) {
                ESP_LOG(WARN, TAG, "FREE figures_ptr");
                free(figures_ptr);
                figures_ptr = NULL;
            }
        }
    } else {
        if (access_lock()){
            
            ESP_LOG(WARN, TAG, "FREE local_data.current_attackable");
            if (local_data.current_attackable) {
                free(local_data.current_attackable);
                local_data.current_attackable = NULL;
            }
            local_data.counter_attackable = 0;

            release_lock();
        } else {
            ESP_LOG(ERROR, TAG, "Failed to access lock when freeing local_data.current_attackable");
            local_data.counter_attackable = 0;
            // TODO: return
        }
    }

    ESP_LOG(INFO, TAG, "Calculated: %d", empty_cells);

    printf("Minimum free heap size: %" PRIu32 " bytes largest block %d \n", esp_get_minimum_free_heap_size(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    ESP_LOG(INFO, TAG, " size %d", (sizeof(uint8_t) * empty_cells));


    *led_array_ptr = NULL;

    *led_array_ptr = (uint8_t*)malloc((empty_cells+3) * sizeof(uint8_t));

   
    if (!(*led_array_ptr) || !led_array_ptr){
        ESP_LOG(ERROR, TAG, "Failed to malloc. Aborting");
        led_array_ptr = NULL;
       
        return ESP_FAIL;
    }

    if (heap_caps_check_integrity(MALLOC_CAP_8BIT, true)) {

    } else {
        ESP_LOG(ERROR, TAG, "Heap is corrupted");
    }

    if (heap_caps_check_integrity_addr(led_array_ptr, true)){

    } else {
        ESP_LOG(ERROR, TAG, "Heap is corrupted around led_array_ptr addr");
    }

    printf("Minimum free heap size: %" PRIu32 " bytes largest block %d \n", esp_get_minimum_free_heap_size(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

    test_function(array, *led_array_ptr, empty_cells);


    /*
    for (int i = 0; i < empty_cells; i++){
        


        ESP_LOG(INFO, TAG, "Current i %d emptycells %d", i, empty_cells);
        ESP_LOG(WARN, TAG, "REALLOC");
        *led_array_ptr = (uint8_t*)realloc(*led_array_ptr, ((i + 1) * sizeof(uint8_t)));
        
        
        if (!(*led_array_ptr) || !led_array_ptr){
            ESP_LOG(WARN, TAG, "NULL PTR");
            *led_array_ptr = (uint8_t*)realloc(*led_array_ptr, ((i + 1) * sizeof(uint8_t)));
        }
        *led_array_ptr[i] = array[i];
        ESP_LOG(INFO, TAG, "Found position: %d led_array_ptr %d", array[i], *led_array_ptr[i]);

        
    }*/

    *counter = empty_cells;

    /*if (empty_cells > 0) {
        local_data.board.board[pos.pos_y][pos.pos_x].led_op(*led_array_ptr, empty_cells);
    }*/

    

    return ESP_OK;
}

static esp_err_t bishop_led_calculation(figure_position_t pos, uint8_t **led_array_ptr, uint8_t *counter) {

    chess_board_t board;

    if(access_lock()){
        board = local_data.board;
        release_lock();
    } else {
        return ESP_FAIL;
    }

    uint8_t left_forward = 0;
    uint8_t left_backward = 0;
    uint8_t right_forward = 0;
    uint8_t right_backward = 0;

     bool attack_possible = false;
    attackable_figures_t *figures_ptr = calloc(MAX_ATTACK_MOVES_BISHOP, sizeof(attackable_figures_t));
    ESP_LOG(WARN, TAG,"MALLOC");
    if (!figures_ptr) {
        ESP_LOG(ERROR, TAG, "Failed to allocate memory. Bishop.");
        // TODO: return?
    }

    ESP_LOG(INFO, TAG, "Calculating diagonal moves for Bishop");

    left_forward = required_cells_calculation_diagonal(board, pos, LEFT_MOD, FORWARD_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    
    if (left_forward == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    left_backward = required_cells_calculation_diagonal(board, pos, LEFT_MOD, BACKWARD_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    if (left_backward == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }
    
    right_forward = required_cells_calculation_diagonal(board, pos, RIGHT_MOD, FORWARD_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    if (right_forward == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    right_backward = required_cells_calculation_diagonal(board, pos, RIGHT_MOD, BACKWARD_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    if (right_backward == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    if (*counter != 0) {
        if (access_lock()){

            if (local_data.current_attackable){
                ESP_LOG(WARN, TAG, "FREE local_data.current_attackable");
                free(local_data.current_attackable);
                local_data.current_attackable = NULL;
                
            }

            local_data.counter_attackable = *counter;
            local_data.current_attackable = figures_ptr;

            release_lock();
        } else {
            ESP_LOG(ERROR, TAG, "Failed to access lock when saving attackable figures.");
            if (figures_ptr) {
                ESP_LOG(WARN, TAG, "FREE figures_ptr");
                free(figures_ptr);
                figures_ptr = NULL;
            }
        }
    } else {
        if (access_lock()){
            
            ESP_LOG(WARN, TAG, "FREE local_data.current_attackable");
            if (local_data.current_attackable) {
                free(local_data.current_attackable);
                local_data.current_attackable = NULL;
            }
            local_data.counter_attackable = 0;

            release_lock();
        } else {
            ESP_LOG(ERROR, TAG, "Failed to access lock when freeing local_data.current_attackable");
            local_data.counter_attackable = 0;
            // TODO: return
        }
    }

    uint8_t total = left_forward + left_backward + right_forward + right_backward;

    if (total == 0) {
        ESP_LOG(WARN, TAG, "No possible moves for bishop or calculations are wrong");
        return ESP_FAIL;
    }

    *led_array_ptr = NULL;

    *led_array_ptr = (uint8_t*)calloc(total, sizeof(uint8_t));

    if (!led_array_ptr){
        ESP_LOG(ERROR, TAG, "Failed to malloc. Aborting");
        led_array_ptr = NULL;
       
        return ESP_FAIL;
    }

    *counter = 0;

    if (populate_led_array_diagonal(*led_array_ptr, left_forward, left_backward, right_forward, right_backward, pos, counter) != ESP_OK){
        ESP_LOG(ERROR, TAG, "Failed to populate diagonal array. Bishop");
        
        return ESP_FAIL;
    }

    //local_data.board.board[pos.pos_y][pos.pos_x].led_op(arr_ptr, total);

    
    return ESP_OK;
}

static esp_err_t queen_led_calculation(figure_position_t pos, uint8_t **led_array_ptr, uint8_t *counter) {

    chess_board_t board;

    if(access_lock()){
        board = local_data.board;
        release_lock();
    } else {
        return ESP_FAIL;
    }

    uint8_t left_forward = 0;
    uint8_t left_backward = 0;
    uint8_t right_forward = 0;
    uint8_t right_backward = 0;

    bool attack_possible = false;
    attackable_figures_t *figures_ptr = calloc(MAX_ATTACK_MOVES_QUEEN, sizeof(attackable_figures_t));
    ESP_LOG(WARN, TAG,"MALLOC");
    if (!figures_ptr) {
        ESP_LOG(ERROR, TAG, "Failed to allocate memory. Queen.");
        // TODO: return?
    }
    
    (*counter)++;

    ESP_LOG(INFO, TAG, "Calculating diagonal moves for queen");

    left_forward = required_cells_calculation_diagonal(board, pos, LEFT_MOD, FORWARD_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    
    if (left_forward == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    left_backward = required_cells_calculation_diagonal(board, pos, LEFT_MOD, BACKWARD_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    if (left_backward == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }
    
    right_forward = required_cells_calculation_diagonal(board, pos, RIGHT_MOD, FORWARD_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    if (right_forward == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    right_backward = required_cells_calculation_diagonal(board, pos, RIGHT_MOD, BACKWARD_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    if (right_backward == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    uint8_t total_diagonal = left_forward + left_backward + right_forward + right_backward;

    uint8_t forward_cells = 0;
    uint8_t backward_cells = 0;
    uint8_t right_cells = 0;
    uint8_t left_cells = 0;

    ESP_LOG(INFO, TAG, "Calculating horisontal and vertical moves for queen");

   

    forward_cells = required_cells_calculation_vertical(board, pos, FORWARD_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    
    if (forward_cells == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    backward_cells = required_cells_calculation_vertical(board, pos, BACKWARD_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    
    if (backward_cells == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }


    right_cells = required_cells_calculation_horisontal(board, pos, RIGHT_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);
    
     if (right_cells == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    left_cells = required_cells_calculation_horisontal(board, pos, LEFT_MOD, NO_LIMIT_FOR_MOVES, &attack_possible, &figures_ptr[*counter]);

    if (left_cells == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    if (*counter != 0) {
        if (access_lock()){

            if (local_data.current_attackable){
                ESP_LOG(WARN, TAG, "FREE local_data.current_attackable");
                free(local_data.current_attackable);
                local_data.current_attackable = NULL;
                
            }
            ESP_LOG(WARN, TAG, "Rewriting local_data.counter_attackable and local_data.current_attackable");
            local_data.counter_attackable = *counter;
            local_data.current_attackable = figures_ptr;

            ESP_LOG(INFO, TAG, "counter %d pos 0 type %d", local_data.counter_attackable, local_data.current_attackable[0].figure);

            release_lock();
        } else {
            ESP_LOG(ERROR, TAG, "Failed to access lock when saving attackable figures.");
            if (figures_ptr) {
                ESP_LOG(WARN, TAG, "FREE figures_ptr");
                free(figures_ptr);
                figures_ptr = NULL;
            }
        }
    } else {
        if (access_lock()){
            
            ESP_LOG(WARN, TAG, "FREE local_data.current_attackable");
            if (local_data.current_attackable) {
                free(local_data.current_attackable);
                local_data.current_attackable = NULL;
            }
            local_data.counter_attackable = 0;

            release_lock();
        } else {
            ESP_LOG(ERROR, TAG, "Failed to access lock when freeing local_data.current_attackable");
            local_data.counter_attackable = 0;
            // TODO: return
        }
    }

    uint8_t total_direct = forward_cells + backward_cells + right_cells + left_cells;

    uint8_t total_combined = total_diagonal + total_direct;

    if (total_combined == 0) {
        ESP_LOG(WARN, TAG, "No possible moves for queen or calculations are wrong");
        return ESP_FAIL; // TODO: probably not needed. Same thing in bishop
    }

    *led_array_ptr = NULL;

    *led_array_ptr = (uint8_t*)calloc(total_combined, sizeof(uint8_t));

    if (!led_array_ptr){
        ESP_LOG(ERROR, TAG, "Failed to malloc. Aborting");
        led_array_ptr = NULL;
       
        return ESP_FAIL;
    }

    *counter = 0;

    if (populate_led_array_diagonal(*led_array_ptr, left_forward, left_backward, right_forward, right_backward, pos, counter) != ESP_OK){
        ESP_LOG(ERROR, TAG, "Failed to populate diagonal array Queen");
    
        return ESP_FAIL;
    }

    if (populate_led_array_direct(*led_array_ptr, forward_cells, backward_cells, right_cells, left_cells, pos, counter) != ESP_OK) {
        ESP_LOG(ERROR, TAG, "Failed to populate direct array Queen");
    
        return ESP_FAIL;
    }

    //local_data.board.board[pos.pos_y][pos.pos_x].led_op(, total_combined);

    return ESP_OK;


}

static uint8_t count_figures_on_board_by_colour(chess_board_t board, bool white) {

    uint8_t counter = 0;

    for (uint8_t i = 0; i < MATRIX_Y; i++){

        for (uint8_t j = 0; j < MATRIX_X; j++){

            if (board.board[i][j].figure_type != FIGURE_END_LIST && board.board[i][j].white == white){
                counter++;
            }

        }

    }

    return counter;

}


static esp_err_t check_all_enemy_moves_for_pos(chess_board_t board, figure_position_t pos, bool king_calculations){
  

    // TODO: assuming .white_turn is correct. Double check that later.
    bool white = false;
    white = !board.white_turn;

    bool in_check = false;
    uint8_t check_figure = 0;
   
    if (access_lock()){
        in_check = local_data.check;
        if (in_check){
            if (local_data.check_trajectory){
                ESP_LOG(WARN, TAG, "traj counter %d", local_data.trajectory_counter);
                check_figure = local_data.check_trajectory[local_data.trajectory_counter-1];
            } else {
                ESP_LOG(ERROR, TAG, "check_trajectory is NULL in check_all_enemy_moves_for_pos in check conditions");
                release_lock();
                return ESP_FAIL;
            }
        }
        release_lock();
    } else {
        return ESP_FAIL;
    }

    //uint8_t total_figures = count_figures_on_board_by_colour(board, white);

    figure_position_t temp_pos;
    esp_err_t ret = ESP_FAIL;

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
    ESP_LOG(INFO, TAG, "CHECKING POS %d:%d_____________________________________", pos.pos_y, pos.pos_x);
    local_data.possible_king_move = pos;


    for (uint8_t i = 0; i < MATRIX_Y; i++){

        for (uint8_t j = 0; j < MATRIX_X; j++){

            if (board.board[i][j].figure_type != FIGURE_END_LIST && board.board[i][j].white == white){ // TODO: maybe do this easy to read
                
                if (board.board[i][j].figure_type == FIGURE_KING){
                    ESP_LOG(ERROR, TAG, "NOT CALCULATING FOR KING. DOUBLE CHECK IT");
                    continue;
                }
                
                if (in_check && !king_calculations) {
                    if (check_figure == MATRIX_TO_ARRAY_CONVERSION(i, j)){
                        ESP_LOG(INFO, TAG, "Found attacking figure at %d:%d array %d", i, j, MATRIX_TO_ARRAY_CONVERSION(i, j));
                        ESP_LOG(INFO, TAG, "Skipping this figure");
                        continue;
                    }
                }

                temp_pos.pos_y = i;
                temp_pos.pos_x = j;
                ESP_LOG(WARN, TAG, "Checking for pos %d:%d", temp_pos.pos_y, temp_pos.pos_x);
                ret = required_leds_calculation(temp_pos, CALCULATIONS_WITHOUT_LEDS, true);
                if (ret == ESP_ERR_NO_MEM){
                    uint8_t converted_pos = MATRIX_TO_ARRAY_CONVERSION((pos.pos_y), (pos.pos_x));
                    for (int z = 0; z < local_data.temp_counter; z++) {
                        if (local_data.temp_led_array[z] == converted_pos){
                            ESP_LOG(ERROR, TAG, "At least 1 figure is blocking this move: %d:%d", pos.pos_y, pos.pos_x);
                            return ret;
                        }
                    }
                    ESP_LOG(ERROR, TAG, "Looks like it's okay to move here %d:%d", pos.pos_y, pos.pos_x);
                    return ESP_OK;
                }
            }

        }

    }

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    return ESP_OK;

}


uint8_t calculate_diagonal_with_mods(chess_board_t board, figure_position_t pos, int8_t hor_mod, int8_t ver_mod, uint8_t limit, bool *attack_possible, attackable_figures_t **figure, uint8_t *counter) {
    uint8_t result = 0;
    bool in_check = false;
    figure_position_t temp_pos;

    if (access_lock()){
        in_check = local_data.check;
        release_lock();
    } else {
        ESP_LOG(ERROR, TAG, "Can't access lock to get the check status");
    }

    chess_figures_t figure_type = FIGURE_END_LIST;
    figure_type = board.board[pos.pos_y][pos.pos_x].figure_type;

    result = required_cells_calculation_diagonal(board, pos, hor_mod, ver_mod, limit, attack_possible, figure[*counter]); // TODO: not sure of this
    if (figure_type == FIGURE_KING && result != 0 && limit == 1){
        temp_pos.pos_y = pos.pos_y + ver_mod;
        temp_pos.pos_x = pos.pos_x + hor_mod;
        ESP_LOG(INFO, TAG, "MODS: hor %d ver %d. Original: %d:%d temp modified: %d:%d", hor_mod, ver_mod, pos.pos_y, pos.pos_x, temp_pos.pos_y, temp_pos.pos_x);
        if (check_all_enemy_moves_for_pos(board, temp_pos, true) == ESP_ERR_NO_MEM){
            ESP_LOG(INFO, TAG, "No possible attacks for pos %d:%d", temp_pos.pos_y, temp_pos.pos_x);
            result = 0;
        }
        if (in_check) {
            ESP_LOG(WARN, TAG, "PLACEHOLDER")
        }
    }
    return result;
}

static uint8_t calculate_horisontal_with_mods(chess_board_t board, figure_position_t pos, uint8_t hor_mod, uint8_t limit, bool *attack_possible, attackable_figures_t **figure, uint8_t *counter){

    uint8_t result = 0;
    bool in_check = false;
    figure_position_t temp_pos;

    if (access_lock()){
        in_check = local_data.check;
        release_lock();
    } else {
        ESP_LOG(ERROR, TAG, "Can't access lock to get the check status");
    }

    chess_figures_t figure_type = FIGURE_END_LIST;
    figure_type = board.board[pos.pos_y][pos.pos_x].figure_type;

    // TODO: not sure of this
    result = required_cells_calculation_horisontal(board, pos, hor_mod, limit, attack_possible, figure[*counter]);
    if (figure_type == FIGURE_KING  && result != 0 && limit == 1){
        temp_pos.pos_y = pos.pos_y;
        temp_pos.pos_x = pos.pos_x + hor_mod;
        ESP_LOG(INFO, TAG, "MODS: hor %d ver %d. Original: %d:%d temp modified: %d:%d", hor_mod, 1, pos.pos_y, pos.pos_x, temp_pos.pos_y, temp_pos.pos_x);
        if (check_all_enemy_moves_for_pos(board, temp_pos, true) == ESP_ERR_NO_MEM){
            ESP_LOG(INFO, TAG, "No possible attacks for pos %d:%d", temp_pos.pos_y, temp_pos.pos_x);
            result = 0;
        }
        if (in_check) {
            ESP_LOG(WARN, TAG, "PLACEHOLDER")
        }
    }
    return result;

}

static uint8_t calculate_vertical_with_mods(chess_board_t board, figure_position_t pos, uint8_t ver_mod, uint8_t limit, bool *attack_possible, attackable_figures_t **figure, uint8_t *counter){

    uint8_t result = 0;
    bool in_check = false;
    figure_position_t temp_pos;

    if (access_lock()){
        in_check = local_data.check;
        release_lock();
    } else {
        ESP_LOG(ERROR, TAG, "Can't access lock to get the check status");
    }

    chess_figures_t figure_type = FIGURE_END_LIST;
    figure_type = board.board[pos.pos_y][pos.pos_x].figure_type;

    // TODO: not sure of this
    result = required_cells_calculation_vertical(board, pos, ver_mod, limit, attack_possible, figure[*counter]);
    if (figure_type == FIGURE_KING && result != 0 && limit == 1){
        temp_pos.pos_y = pos.pos_y + ver_mod;
        temp_pos.pos_x = pos.pos_x;
        ESP_LOG(INFO, TAG, "MODS: hor %d ver %d. Original: %d:%d temp modified: %d:%d", 1, ver_mod, pos.pos_y, pos.pos_x, temp_pos.pos_y, temp_pos.pos_x);
        if (check_all_enemy_moves_for_pos(board, temp_pos, true) == ESP_ERR_NO_MEM){
            ESP_LOG(INFO, TAG, "No possible attacks for pos %d:%d", temp_pos.pos_y, temp_pos.pos_x);
            result = 0;
        }
        if (in_check) {
            ESP_LOG(WARN, TAG, "PLACEHOLDER")
        }
    }
    return result;

}

esp_err_t king_led_calculations(figure_position_t pos, uint8_t **led_array_ptr, uint8_t *counter) {

    chess_board_t board;
    bool in_check = false;
    figure_position_t temp_pos;

    if(access_lock()){
        board = local_data.board;
        in_check = local_data.check;
        release_lock();
    } else {
        return ESP_FAIL;
    }

    uint8_t left_forward = 0;
    uint8_t left_backward = 0;
    uint8_t right_forward = 0;
    uint8_t right_backward = 0;

    // TODO: needs a free at some point.
    bool attack_possible = false;
    attackable_figures_t *figures_ptr = calloc(MAX_ATTACK_MOVES_KING, sizeof(attackable_figures_t));
    ESP_LOG(WARN, TAG,"MALLOC");
    if (!figures_ptr) {
        ESP_LOG(ERROR, TAG, "Failed to allocate memory. KING.");
        // TODO: return?
    }
    

    ESP_LOG(INFO, TAG, "Calculating diagonal moves for king");

    left_forward = calculate_diagonal_with_mods(board, pos, LEFT_MOD, FORWARD_MOD, 1, &attack_possible, &figures_ptr, counter);

    //left_forward = required_cells_calculation_diagonal(board, pos, LEFT_MOD, FORWARD_MOD, 1, &attack_possible, &figures_ptr[*counter]);
    
    if (left_forward == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    left_backward = calculate_diagonal_with_mods(board, pos, LEFT_MOD, BACKWARD_MOD, 1, &attack_possible, &figures_ptr, counter);

    //left_backward = required_cells_calculation_diagonal(board, pos, LEFT_MOD, BACKWARD_MOD, 1, &attack_possible, &figures_ptr[*counter]);
    if (left_backward == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }
    
    right_forward = calculate_diagonal_with_mods(board, pos, RIGHT_MOD, FORWARD_MOD, 1, &attack_possible, &figures_ptr, counter);

    //right_forward = required_cells_calculation_diagonal(board, pos, RIGHT_MOD, FORWARD_MOD, 1, &attack_possible, &figures_ptr[*counter]);
    if (right_forward == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    right_backward = calculate_diagonal_with_mods(board, pos, RIGHT_MOD, BACKWARD_MOD, 1, &attack_possible, &figures_ptr, counter);

    //right_backward = required_cells_calculation_diagonal(board, pos, RIGHT_MOD, BACKWARD_MOD, 1,  &attack_possible, &figures_ptr[*counter]);
    if (right_backward == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }


    uint8_t total_diagonal = left_forward + left_backward + right_forward + right_backward;

    if (total_diagonal == 0){
        ESP_LOG(WARN, TAG, "Total diagonal 0");
    } else {
        ESP_LOG(WARN, TAG, "Total diagonal %d", total_diagonal);
    }

    uint8_t forward_cells = 0;
    uint8_t backward_cells = 0;
    uint8_t right_cells = 0;
    uint8_t left_cells = 0;

    ESP_LOG(INFO, TAG, "Calculating horisontal and vertical moves for king");

    

    //forward_cells = required_cells_calculation_vertical(board, pos, FORWARD_MOD, 1, &attack_possible, &figures_ptr[*counter]);
    
    forward_cells = calculate_vertical_with_mods(board, pos, FORWARD_MOD, 1, &attack_possible, &figures_ptr, counter);

    if (forward_cells == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    backward_cells = calculate_vertical_with_mods(board, pos, BACKWARD_MOD, 1, &attack_possible, &figures_ptr, counter);

    //backward_cells = required_cells_calculation_vertical(board, pos, BACKWARD_MOD, 1, &attack_possible, &figures_ptr[*counter]);
    
    if (backward_cells == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    //right_cells = required_cells_calculation_horisontal(board, pos, RIGHT_MOD, 1, &attack_possible, &figures_ptr[*counter]);
    
    right_cells = calculate_horisontal_with_mods(board, pos, RIGHT_MOD, 1, &attack_possible, &figures_ptr, counter);

    if (right_cells == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    //left_cells = required_cells_calculation_horisontal(board, pos, LEFT_MOD, 1, &attack_possible, &figures_ptr[*counter]);#
    left_cells = calculate_horisontal_with_mods(board, pos, LEFT_MOD, 1, &attack_possible, &figures_ptr, counter);

    if (left_cells == POSITION_CALCULATION_ERROR) {
        if (figures_ptr){
            ESP_LOG(WARN, TAG, "FREE");
            free(figures_ptr);
        }
        return ESP_FAIL;
    }

    if (attack_possible){
        (*counter)++;
        attack_possible = false;
    }

    if (counter != 0) {
        if (access_lock()){

            if (local_data.current_attackable){
                ESP_LOG(WARN, TAG, "FREE local_data.current_attackable");
                free(local_data.current_attackable);
                local_data.current_attackable = NULL;
                
            }

            local_data.counter_attackable = *counter;
            local_data.current_attackable = figures_ptr;

            release_lock();
        } else {
            ESP_LOG(ERROR, TAG, "Failed to access lock when saving attackable figures.");
            if (figures_ptr) {
                ESP_LOG(WARN, TAG, "FREE figures_ptr");
                free(figures_ptr);
                figures_ptr = NULL;
            }
        }
    } else {
        if (access_lock()){
            
            ESP_LOG(WARN, TAG, "FREE local_data.current_attackable");
            if (local_data.current_attackable) {
                free(local_data.current_attackable);
                local_data.current_attackable = NULL;
            }
            local_data.counter_attackable = 0;

            release_lock();
        } else {
            ESP_LOG(ERROR, TAG, "Failed to access lock when freeing local_data.current_attackable");
            local_data.counter_attackable = 0;
            // TODO: return
        }
    }

    uint8_t total_direct = forward_cells + backward_cells + right_cells + left_cells;

    uint8_t total_combined = total_diagonal + total_direct;

    if (total_combined == 0) {
        ESP_LOG(WARN, TAG, "No possible moves for king or calculations are wrong");
        // TODO: free figures_ptr?
        return ESP_ERR_INVALID_STATE;
    }

    *led_array_ptr = NULL;

    *led_array_ptr = (uint8_t*)calloc(total_combined, sizeof(uint8_t));

    if (!led_array_ptr){
        ESP_LOG(ERROR, TAG, "Failed to malloc. Aborting");
        led_array_ptr = NULL;
       
        return ESP_FAIL;
    }

    *counter = 0;

    if (populate_led_array_diagonal(*led_array_ptr, left_forward, left_backward, right_forward, right_backward, pos, counter) != ESP_OK){
        ESP_LOG(ERROR, TAG, "Failed to populate diagonal array");
     
        return ESP_FAIL;
    }

    if (populate_led_array_direct(*led_array_ptr, forward_cells, backward_cells, right_cells, left_cells, pos, counter) != ESP_OK) {
        ESP_LOG(ERROR, TAG, "Failed to populate direct array");
      
        return ESP_FAIL;
    }

   // local_data.board.board[pos.pos_y][pos.pos_x].led_op(arr_ptr, total_combined);

    
  

    return ESP_OK;
}


static esp_err_t checkmate_check(figure_position_t pos) {

    chess_board_t board;
    if (access_lock()) {
        board = local_data.board;
        release_lock();
    } else {
        ESP_LOG(ERROR, TAG, "Checkmate check failed because couldn't access lock");
        return ESP_FAIL;
    }

    figure_position_t temp_pos;
    esp_err_t ret = 0;

    bool white = false;
    bool at_least_one_possible_move = false;

    white = !board.board[pos.pos_y][pos.pos_x].white; // opposite because we need opposite colour of figure that checked the king

    for (int i = 0; i < MATRIX_Y; i++){
        temp_pos.pos_y = i;
        for (int j = 0; j < MATRIX_X; j++){
            temp_pos.pos_x = j;
            if (board.board[i][j].figure_type != FIGURE_END_LIST && board.board[i][j].white == white) {

                
                ret = required_leds_calculation(temp_pos, CALCULATIONS_WITHOUT_LEDS, false);// TODO: check this
                if (ret != ESP_OK){

                    ESP_LOG(WARN, TAG, "returned ret: %d", ret);

                } else {
                    at_least_one_possible_move = true;
                }
                if (at_least_one_possible_move) {
                    ESP_LOG(INFO, TAG, "There is still at least 1 possible move to avoid checkmate");
                    return ESP_OK;
                }
            }
        }
    }
    if (!at_least_one_possible_move){
        ESP_LOG(WARN, TAG, "Looks like there are no possible moves left to avoid checkmate");
        return ESP_FAIL;
    }


    return ESP_OK;
}

static esp_err_t required_leds_calculation(figure_position_t updated_pos, bool show_leds, bool check_calculations){
    
    chess_figures_t current_figure;
    bool check = false;

    esp_err_t ret = ESP_OK;

    if (access_lock()){
        current_figure = local_data.board.board[updated_pos.pos_y][updated_pos.pos_x].figure_type;
        check = local_data.check;

        if (local_data.temp_led_array){
            ESP_LOG(WARN, TAG, "FREE local_data.temp_led_array");
            free(local_data.temp_led_array);
            local_data.temp_led_array = NULL;
        }
        local_data.temp_counter = 0;

        release_lock();
    } else {
        return ESP_FAIL;
    }

    uint8_t *led_array_ptr = NULL;
    uint8_t counter = 0;

    bool check_happened_this_turn = false;

    /*
    heap_trace_record_t trace_record[100];

    heap_trace_init_standalone(trace_record, 100);

    heap_trace_start(HEAP_TRACE_ALL);
    */

    uint8_t traj_counter = 0;


    switch (current_figure){ 
        case FIGURE_PAWN:
            if (pawn_led_calculation(updated_pos, &led_array_ptr, &counter) != ESP_OK){

            }
            for (int i = 0; i < counter; i++){
                ESP_LOG(WARN, TAG, "returned led_array_ptr i %d val %d", i, led_array_ptr[i]);
            }

            if (local_data.counter_attackable != 0){
                ESP_LOG(WARN, TAG, "attackable counter %d", local_data.counter_attackable);
                for (int i = 0; i < local_data.counter_attackable; i++) {
                    ESP_LOG(WARN, TAG, "Checking type %d on pos %d:%d", local_data.current_attackable[i].figure, local_data.current_attackable[i].pos.pos_y,local_data.current_attackable[i].pos.pos_x);
                    if (local_data.current_attackable[i].figure == FIGURE_KING){
                        if (check_calculations){
                            ESP_LOG(WARN, TAG, "King under possible attack from knight");

                             if (led_array_ptr){
                                ESP_LOG(WARN, TAG, "FREE led_array_ptr");
                                free(led_array_ptr);
                                led_array_ptr = NULL;
                            }
                            return ESP_ERR_NO_MEM;

                        }
                        ESP_LOG(WARN, TAG, "King under attack");
                        local_data.check = true;
                        check_happened_this_turn = true;

                        
                        figure_position_t king_pos = local_data.current_attackable[i].pos;

                        
                        uint8_t *modified_led_array_ptr = NULL;

                        modified_led_array_ptr = (uint8_t*)calloc(1, sizeof(uint8_t));

                        if (!modified_led_array_ptr){
                            ESP_LOG(ERROR, TAG, "Failed to malloc modified_led_array_ptr");
                            modified_led_array_ptr = NULL;
                            goto FUNCTION_FAIL;
                        }

                        modified_led_array_ptr[0] = MATRIX_TO_ARRAY_CONVERSION((updated_pos.pos_y), (updated_pos.pos_x));
                        
                        ESP_LOG(WARN, TAG, "Added %d or %d:%d to led_array_ptr", MATRIX_TO_ARRAY_CONVERSION((updated_pos.pos_y), (updated_pos.pos_x)), updated_pos.pos_y, updated_pos.pos_x);

                        traj_counter = 1;

                        if (local_data.check_trajectory){
                            ESP_LOG(ERROR, TAG, "local_data.check_trajectory is not null.");
                            free(local_data.check_trajectory);
                            local_data.check_trajectory = NULL;
                        }

                        for (int j = 0; j < traj_counter; j++){
                            ESP_LOG(INFO, TAG, "Mod led array ptr %d", modified_led_array_ptr[j]);
                        }

                        local_data.check_trajectory = modified_led_array_ptr;

                        if (local_data.check_trajectory) {
                            ESP_LOG(ERROR, TAG, "trajectory 0 %d", local_data.check_trajectory[0]);
                        }

                        
                        local_data.trajectory_counter = traj_counter;

                        break;
                    }
                }
            }

            break;
        case FIGURE_ROOK:
            if (rook_led_calculation(updated_pos, &led_array_ptr, &counter) != ESP_OK) {

            }
            if (local_data.counter_attackable != 0){
                ESP_LOG(WARN, TAG, "attackable counter %d", local_data.counter_attackable);
                for (int i = 0; i < local_data.counter_attackable; i++) {
                    ESP_LOG(WARN, TAG, "Checking type %d on pos %d:%d", local_data.current_attackable[i].figure, local_data.current_attackable[i].pos.pos_y,local_data.current_attackable[i].pos.pos_x);
                    if (local_data.current_attackable[i].figure == FIGURE_KING ){
                        if (check_calculations){
                            ESP_LOG(WARN, TAG, "King under possible attack from rook");

                             if (led_array_ptr){
                                ESP_LOG(WARN, TAG, "FREE led_array_ptr");
                                free(led_array_ptr);
                                led_array_ptr = NULL;
                            }
                            return ESP_ERR_NO_MEM;

                            
                        }
                        ESP_LOG(WARN, TAG, "King under attack");
                        local_data.check = true;
                        check_happened_this_turn = true;

                        
                        int8_t mod_x = 0;
                        int8_t mod_y = 0;
                        
                        figure_position_t king_pos = local_data.current_attackable[i].pos;

                        if (king_pos.pos_y > updated_pos.pos_y){
                            mod_y = FORWARD_MOD;
                        } else if (king_pos.pos_y < updated_pos.pos_y) {
                            mod_y = BACKWARD_MOD;
                        }

                        if (king_pos.pos_x > updated_pos.pos_x){
                            mod_x = RIGHT_MOD;
                        } else if (king_pos.pos_x < updated_pos.pos_x) {
                            mod_x = LEFT_MOD;
                        }

                        uint8_t *modified_led_array_ptr = NULL;

                        if (mod_y != 0){
                            for (int j = updated_pos.pos_y + mod_y; j != king_pos.pos_y; j += mod_y){
                                ESP_LOG(WARN, TAG, "REALLOC");
                                modified_led_array_ptr = (uint8_t*)realloc(modified_led_array_ptr, ((traj_counter+1)*(sizeof(uint8_t))));
                                
                                if (!modified_led_array_ptr) {
                                    ESP_LOG(ERROR, TAG, "Failed to reallocate more memory. Aborting.");
                                    goto FUNCTION_FAIL;
                                }
                                
                                modified_led_array_ptr[traj_counter] = MATRIX_TO_ARRAY_CONVERSION(j, (updated_pos.pos_x));

                                traj_counter++;
                            }
                        } else if (mod_x != 0){
                            for (int j = updated_pos.pos_x + mod_x; j != king_pos.pos_x; j += mod_x){
                                
                                ESP_LOG(WARN, TAG, "REALLOC");
                                modified_led_array_ptr = (uint8_t*)realloc(modified_led_array_ptr, ((traj_counter+1)*(sizeof(uint8_t))));
                                
                                if (!modified_led_array_ptr) {
                                    ESP_LOG(ERROR, TAG, "Failed to reallocate more memory. Aborting.");
                                    goto FUNCTION_FAIL;
                                }

                                modified_led_array_ptr[traj_counter] = MATRIX_TO_ARRAY_CONVERSION((updated_pos.pos_y), j);
                                traj_counter++;
                            }
                        }

                        if (local_data.check_trajectory){
                            ESP_LOG(ERROR, TAG, "local_data.check_trajectory is not null.");
                            free(local_data.check_trajectory);
                            local_data.check_trajectory = NULL;
                        }

                        ESP_LOG(WARN, TAG, "REALLOC for figure pos");
                        modified_led_array_ptr = (uint8_t *)realloc(modified_led_array_ptr, ((traj_counter+1) * sizeof(uint8_t))); 
                        

                        if (!modified_led_array_ptr){
                            ESP_LOG(ERROR, TAG, "Failed to reallocate memory on check");
                            return ESP_FAIL; // TODO: dobule check
                        }

                        for (int j = 0; j < traj_counter; j++){
                            ESP_LOG(INFO, TAG, "Mod led array ptr %d", modified_led_array_ptr[j]);
                        }

                        
                        modified_led_array_ptr[traj_counter] = MATRIX_TO_ARRAY_CONVERSION((updated_pos.pos_y), (updated_pos.pos_x));
                        traj_counter++;

                        ESP_LOG(WARN, TAG, "Added %d or %d:%d to led_array_ptr", MATRIX_TO_ARRAY_CONVERSION((updated_pos.pos_y), (updated_pos.pos_x)), updated_pos.pos_y, updated_pos.pos_x);

                        local_data.check_trajectory = modified_led_array_ptr;

                         if (local_data.check_trajectory) {
                            ESP_LOG(ERROR, TAG, "trajectory 0 %d", local_data.check_trajectory[0]);
                        }

                        ESP_LOG(DEBUG, TAG, "Past memcpy");
                        local_data.trajectory_counter = traj_counter;

                        break;

                    }

                    

                }
            }

            break;
        case FIGURE_KNIGHT:
            if (knight_led_calculation(updated_pos, &led_array_ptr, &counter) != ESP_OK){

            }

            for (int i = 0; i < counter; i++){
                ESP_LOG(WARN, TAG, "returned led_array_ptr i %d val %d", i, led_array_ptr[i]);
            }

            if (local_data.counter_attackable != 0){
                ESP_LOG(WARN, TAG, "attackable counter %d", local_data.counter_attackable);
                for (int i = 0; i < local_data.counter_attackable; i++) {
                    ESP_LOG(WARN, TAG, "Checking type %d on pos %d:%d", local_data.current_attackable[i].figure, local_data.current_attackable[i].pos.pos_y,local_data.current_attackable[i].pos.pos_x);
                    if (local_data.current_attackable[i].figure == FIGURE_KING){
                        if (check_calculations){
                            ESP_LOG(WARN, TAG, "King under possible attack from knight");

                             if (led_array_ptr){
                                ESP_LOG(WARN, TAG, "FREE led_array_ptr");
                                free(led_array_ptr);
                                led_array_ptr = NULL;
                            }
                            return ESP_ERR_NO_MEM;

                        }
                        ESP_LOG(WARN, TAG, "King under attack");
                        local_data.check = true;
                        check_happened_this_turn = true;

                        
                        figure_position_t king_pos = local_data.current_attackable[i].pos;

                        
                        uint8_t *modified_led_array_ptr = NULL;

                        modified_led_array_ptr = (uint8_t*)calloc(1, sizeof(uint8_t));

                        if (!modified_led_array_ptr){
                            ESP_LOG(ERROR, TAG, "Failed to malloc modified_led_array_ptr");
                            modified_led_array_ptr = NULL;
                            goto FUNCTION_FAIL;
                        }

                        modified_led_array_ptr[0] = MATRIX_TO_ARRAY_CONVERSION((updated_pos.pos_y), (updated_pos.pos_x));
                        
                        ESP_LOG(WARN, TAG, "Added %d or %d:%d to led_array_ptr", MATRIX_TO_ARRAY_CONVERSION((updated_pos.pos_y), (updated_pos.pos_x)), updated_pos.pos_y, updated_pos.pos_x);

                        traj_counter = 1;

                        if (local_data.check_trajectory){
                            ESP_LOG(ERROR, TAG, "local_data.check_trajectory is not null.");
                            free(local_data.check_trajectory);
                            local_data.check_trajectory = NULL;
                        }

                        for (int j = 0; j < traj_counter; j++){
                            ESP_LOG(INFO, TAG, "Mod led array ptr %d", modified_led_array_ptr[j]);
                        }


                        local_data.check_trajectory = modified_led_array_ptr;

                        ESP_LOG(DEBUG, TAG, "Past memcpy");

                        if (local_data.check_trajectory) {
                            ESP_LOG(ERROR, TAG, "trajectory 0 %d", local_data.check_trajectory[0]);
                        }

                        
                        local_data.trajectory_counter = traj_counter;

                        break;
                    }
                }
            }

            break;
        case FIGURE_BISHOP:
            if (bishop_led_calculation(updated_pos, &led_array_ptr, &counter) != ESP_OK){

            }
            if (local_data.counter_attackable != 0){
                ESP_LOG(WARN, TAG, "attackable counter %d", local_data.counter_attackable);
                for (int i = 0; i < local_data.counter_attackable; i++) {
                    ESP_LOG(WARN, TAG, "Checking type %d on pos %d:%d", local_data.current_attackable[i].figure, local_data.current_attackable[i].pos.pos_y,local_data.current_attackable[i].pos.pos_x);
                    if (local_data.current_attackable[i].figure == FIGURE_KING){
                        if (check_calculations){
                            ESP_LOG(WARN, TAG, "King under possible attack from bishop");

                             if (led_array_ptr){
                                ESP_LOG(WARN, TAG, "FREE led_array_ptr");
                                free(led_array_ptr);
                                led_array_ptr = NULL;
                            }
                            return ESP_ERR_NO_MEM;

                        }
                        ESP_LOG(WARN, TAG, "King under attack");
                        local_data.check = true;
                        check_happened_this_turn = true;

                        
                        int8_t mod_x = 0;
                        int8_t mod_y = 0;
                        
                        figure_position_t king_pos = local_data.current_attackable[i].pos;

                        if (king_pos.pos_y > updated_pos.pos_y){
                            mod_y = FORWARD_MOD;
                        } else if (king_pos.pos_y < updated_pos.pos_y) {
                            mod_y = BACKWARD_MOD;
                        }

                        if (king_pos.pos_x > updated_pos.pos_x){
                            mod_x = RIGHT_MOD;
                        } else if (king_pos.pos_x < updated_pos.pos_x) {
                            mod_x = LEFT_MOD;
                        }

                        uint8_t *modified_led_array_ptr = NULL;

                        if (mod_y != 0 && mod_x != 0){
                            uint8_t modified_x = updated_pos.pos_x;
                            for (int j = updated_pos.pos_y + mod_y; j != king_pos.pos_y; j += mod_y){
                                ESP_LOG(WARN, TAG, "REALLOC mods y %d x %d", mod_y, mod_x);
                                modified_led_array_ptr = (uint8_t*)realloc(modified_led_array_ptr, ((traj_counter+1)*(sizeof(uint8_t))));
                                
                                if (!modified_led_array_ptr) {
                                    ESP_LOG(ERROR, TAG, "Failed to reallocate more memory. Aborting.");
                                    goto FUNCTION_FAIL;
                                }
                                modified_x += mod_x;
                                modified_led_array_ptr[traj_counter] = MATRIX_TO_ARRAY_CONVERSION(j, modified_x);

                                traj_counter++;
                            }
                        }

                        if (local_data.check_trajectory){
                            ESP_LOG(ERROR, TAG, "local_data.check_trajectory is not null.");
                            free(local_data.check_trajectory);
                            local_data.check_trajectory = NULL;
                        }

                        ESP_LOG(WARN, TAG, "REALLOC for figure pos");
                        modified_led_array_ptr = (uint8_t *)realloc(modified_led_array_ptr, ((traj_counter+1) * sizeof(uint8_t))); 
                        

                        if (!modified_led_array_ptr){
                            ESP_LOG(ERROR, TAG, "Failed to reallocate memory on check");
                            return ESP_FAIL; // TODO: dobule check
                        }

                        for (int j = 0; j < traj_counter; j++){
                            ESP_LOG(INFO, TAG, "Mod led array ptr %d", modified_led_array_ptr[j]);
                        }

                        uint8_t converted_base_position = MATRIX_TO_ARRAY_CONVERSION((updated_pos.pos_y), (updated_pos.pos_x));
                        modified_led_array_ptr[traj_counter] = converted_base_position;
                        traj_counter++;
                        ESP_LOG(WARN, TAG, "Added %d or %d (predefined) %d:%d to led_array_ptr", MATRIX_TO_ARRAY_CONVERSION((updated_pos.pos_y), (updated_pos.pos_x)), converted_base_position, updated_pos.pos_y, updated_pos.pos_x);


                        local_data.check_trajectory = modified_led_array_ptr;

                         if (local_data.check_trajectory) {
                            ESP_LOG(ERROR, TAG, "trajectory 0 %d", local_data.check_trajectory[0]);
                        }

                        local_data.trajectory_counter = traj_counter;

                        break;

                    }
                }
            }
            break;
        case FIGURE_QUEEN:
            if (queen_led_calculation(updated_pos, &led_array_ptr, &counter) != ESP_OK){

            }
            if (local_data.counter_attackable != 0){
                ESP_LOG(WARN, TAG, "attackable counter %d", local_data.counter_attackable);
                for (int i = 0; i < local_data.counter_attackable; i++) {
                    ESP_LOG(WARN, TAG, "Checking type %d on pos %d:%d", local_data.current_attackable[i].figure, local_data.current_attackable[i].pos.pos_y,local_data.current_attackable[i].pos.pos_x);
                    if (local_data.current_attackable[i].figure == FIGURE_KING){
                        if (check_calculations){
                            ESP_LOG(WARN, TAG, "King under possible attack from queen");

                            local_data.temp_led_array = led_array_ptr;
                            local_data.temp_counter = counter;
                            /*
                            if (led_array_ptr){
                                ESP_LOG(WARN, TAG, "FREE led_array_ptr");
                                free(led_array_ptr);
                                led_array_ptr = NULL;
                            } 
                            */
                            return ESP_ERR_NO_MEM;

                        }
                        ESP_LOG(WARN, TAG, "King under attack");
                        local_data.check = true;
                        check_happened_this_turn = true;

                        
                        int8_t mod_x = 0;
                        int8_t mod_y = 0;
                        
                        figure_position_t king_pos = local_data.current_attackable[i].pos;

                        if (king_pos.pos_y > updated_pos.pos_y){
                            mod_y = FORWARD_MOD;
                        } else if (king_pos.pos_y < updated_pos.pos_y) {
                            mod_y = BACKWARD_MOD;
                        }

                        if (king_pos.pos_x > updated_pos.pos_x){
                            mod_x = RIGHT_MOD;
                        } else if (king_pos.pos_x < updated_pos.pos_x) {
                            mod_x = LEFT_MOD;
                        }

                        uint8_t *modified_led_array_ptr = NULL;

                        if (mod_y != 0 || mod_x != 0){
                            uint8_t modified_x = updated_pos.pos_x;
                            uint8_t modified_y = updated_pos.pos_y;

                            ESP_LOG(INFO, TAG, "mods in trajectory filling y %d x %d", mod_y, mod_x);

                            if (mod_y != 0) { 
                                for (int j = updated_pos.pos_y + mod_y; j != king_pos.pos_y; j += mod_y){
                                    ESP_LOG(WARN, TAG, "REALLOC mods y %d x %d", mod_y, mod_x);
                                    modified_led_array_ptr = (uint8_t*)realloc(modified_led_array_ptr, ((traj_counter+1)*(sizeof(uint8_t))));
                                    
                                    if (!modified_led_array_ptr) {
                                        ESP_LOG(ERROR, TAG, "Failed to reallocate more memory. Aborting.");
                                        goto FUNCTION_FAIL;
                                    }
                                    modified_x += mod_x;
                                    modified_led_array_ptr[traj_counter] = MATRIX_TO_ARRAY_CONVERSION(j, modified_x);

                                    traj_counter++;
                                }
                            } else if (mod_x != 0) {
                                for (int j = updated_pos.pos_x + mod_x; j != king_pos.pos_x; j += mod_x){
                                    ESP_LOG(WARN, TAG, "REALLOC mods y %d x %d", mod_y, mod_x);
                                    modified_led_array_ptr = (uint8_t*)realloc(modified_led_array_ptr, ((traj_counter+1)*(sizeof(uint8_t))));
                                    
                                    if (!modified_led_array_ptr) {
                                        ESP_LOG(ERROR, TAG, "Failed to reallocate more memory. Aborting.");
                                        goto FUNCTION_FAIL;
                                    }
                                    modified_y += mod_y;
                                    modified_led_array_ptr[traj_counter] = MATRIX_TO_ARRAY_CONVERSION(modified_y, j);

                                    traj_counter++;
                                }
                            }
                        } else if (mod_y == 0 && mod_x == 0){
                            ESP_LOG(ERROR, TAG, "King happens to be at the same cell as the queen. WRONG. Aborting");
                            goto FUNCTION_FAIL;
                        }

                        if (local_data.check_trajectory){
                            ESP_LOG(ERROR, TAG, "local_data.check_trajectory is not null.");
                            free(local_data.check_trajectory);
                            local_data.check_trajectory = NULL;
                        }

                        ESP_LOG(WARN, TAG, "REALLOC for figure pos");
                        modified_led_array_ptr = (uint8_t *)realloc(modified_led_array_ptr, ((traj_counter+1) * sizeof(uint8_t))); 
                        

                        if (!modified_led_array_ptr){
                            ESP_LOG(ERROR, TAG, "Failed to reallocate memory on check");
                            return ESP_FAIL; // TODO: dobule check
                        }

                        for (int j = 0; j < traj_counter; j++){
                            ESP_LOG(INFO, TAG, "Mod led array ptr %d", modified_led_array_ptr[j]);
                        }

                        uint8_t converted_base_position = MATRIX_TO_ARRAY_CONVERSION((updated_pos.pos_y), (updated_pos.pos_x));
                        modified_led_array_ptr[traj_counter] = converted_base_position;
                        traj_counter++;
                        ESP_LOG(WARN, TAG, "Added %d or %d (predefined) %d:%d to led_array_ptr", MATRIX_TO_ARRAY_CONVERSION((updated_pos.pos_y), (updated_pos.pos_x)), converted_base_position, updated_pos.pos_y, updated_pos.pos_x);


                        local_data.check_trajectory = modified_led_array_ptr;

                         if (local_data.check_trajectory) {
                            ESP_LOG(ERROR, TAG, "trajectory 0 %d", local_data.check_trajectory[0]);
                        }

                        local_data.trajectory_counter = traj_counter;

                        break;
                    }
                }
            }
            break;
        case FIGURE_KING:
            ret = king_led_calculations(updated_pos, &led_array_ptr, &counter);
            if (ret != ESP_OK){
                if (ret == ESP_ERR_INVALID_STATE){
                    ESP_LOG(WARN, TAG, "Either error in calculations or no possible moves for king");
                    if (show_leds) {
                        led_no_move_possible(MATRIX_TO_ARRAY_CONVERSION(updated_pos.pos_y, updated_pos.pos_x));
                    }
                }
            }
            if (local_data.counter_attackable != 0){
                ESP_LOG(WARN, TAG, "attackable counter %d", local_data.counter_attackable);
                for (int i = 0; i < local_data.counter_attackable; i++) {
                    ESP_LOG(WARN, TAG, "Checking type %d on pos %d:%d", local_data.current_attackable->figure, local_data.current_attackable->pos.pos_y,local_data.current_attackable->pos.pos_x);
                    
                    // TODO: this is probably unneeded for king.
                    if (local_data.current_attackable->figure == FIGURE_KING){
                        if (check_calculations){
                            ESP_LOG(WARN, TAG, "King under possible attack from king? Check TODO");

                             if (led_array_ptr){
                                ESP_LOG(WARN, TAG, "FREE led_array_ptr");
                                free(led_array_ptr);
                                led_array_ptr = NULL;
                            }
                            return ESP_ERR_NO_MEM;

                        }
                        ESP_LOG(WARN, TAG, "King under attack");
                        local_data.check = true;
                    }
                }
            }
            break;

        default:
            ESP_LOG(ERROR, TAG, "Incorrect figure type. Aborting");
        }


    if (check_calculations) {
        uint8_t possible_move_converted = MATRIX_TO_ARRAY_CONVERSION(local_data.possible_king_move.pos_y, local_data.possible_king_move.pos_x);
        ESP_LOG(WARN, TAG, "converted %d", possible_move_converted);
        for (int i = 0; i < counter; i++){
            if (led_array_ptr[i] == possible_move_converted) {
                ESP_LOG(ERROR, TAG, "Gentelmen, we found him");
                ESP_LOG(INFO, TAG, "Led array i %d", led_array_ptr[i]);
                ESP_LOG(WARN, TAG, "King would be under possible attack at %d:%d converted %d", local_data.possible_king_move.pos_y,local_data.possible_king_move.pos_x, MATRIX_TO_ARRAY_CONVERSION(local_data.possible_king_move.pos_y, local_data.possible_king_move.pos_x));

                if (led_array_ptr){
                    ESP_LOG(WARN, TAG, "FREE led_array_ptr");
                    free(led_array_ptr);
                    led_array_ptr = NULL;
                }
                return ESP_ERR_NO_MEM;
            }
        }
    }


    
        
        

        if (!led_array_ptr){
            ESP_LOG(ERROR, TAG, "NULL ptr");
        } else if (check){

            uint8_t traj_counter_passed = local_data.trajectory_counter;

            uint8_t valid_moves_counter = 0;

            ESP_LOG(INFO, TAG, "Check happened not this turn");

            ESP_LOG(INFO, TAG, "Passed traj counter %d", traj_counter_passed);

            if (current_figure != FIGURE_KING || (current_figure == FIGURE_KING && traj_counter_passed == 1)) {

                for (int i = 0; i < counter; i++){
                    for (int j = 0; j < traj_counter_passed; j++){

                        if (local_data.check_trajectory[j] == led_array_ptr[i]){
                            ESP_LOG(INFO, TAG, "This move %d will protect king from check", led_array_ptr[i]);
                            led_array_ptr[valid_moves_counter] = led_array_ptr[i];
                            valid_moves_counter++;
                                
                        }

                    }
                    
                }
            } else if (current_figure == FIGURE_KING && traj_counter_passed > 1){
               
                bool did_not_match = true;

                for (int i = 0; i < counter; i++){
                    did_not_match = true;
                    for (int j = 0; j < traj_counter_passed; j++){

                        if (local_data.check_trajectory[j] == led_array_ptr[i]){
                            did_not_match = false;
                            ESP_LOG(WARN, TAG, "This move %d will not protect king from check", led_array_ptr[i]);

                            for (int z = i; z < counter-1; z++){
                                led_array_ptr[i] = led_array_ptr[i+1];
                            }
                            led_array_ptr[counter-1] = 0;
                            counter--;
                            i--;
                            break;
                                
                        }

                    }
                    if (did_not_match){
                        valid_moves_counter++;
                    }
                    
                }
            }
            if (valid_moves_counter > 0){
                
                ESP_LOG(INFO, TAG, "Valid moves num %d", valid_moves_counter);
                if (show_leds){
                    ESP_LOG(INFO, TAG, "Showing LEDS");
                    local_data.board.board[updated_pos.pos_y][updated_pos.pos_x].led_op(led_array_ptr, valid_moves_counter);
                }
            } else {
                ESP_LOG(INFO, TAG, "No moves for this figure will protect king from check");
                if (show_leds){
                    ESP_LOG(INFO, TAG, "Showing LEDS");
                    led_no_move_possible(MATRIX_TO_ARRAY_CONVERSION((updated_pos.pos_y), (updated_pos.pos_x)));
                }
                return ESP_ERR_INVALID_STATE;
            }
        } else if (!check){
        
            //ESP_LOG(INFO, TAG, "Array 0 after returning from function %d", led_array_ptr[0]);

            for (int i = 0; i < counter; i++) {
                    ESP_LOG(INFO, TAG, "Array %d after returning from function %d", i, led_array_ptr[i]);
            }
            if (show_leds){
                ESP_LOG(INFO, TAG, "Showing LEDS");
                local_data.board.board[updated_pos.pos_y][updated_pos.pos_x].led_op(led_array_ptr, counter);
            }

            
        }
        
        
    

    // uncomment if problems with heap appear
    /*
    heap_trace_stop();
    heap_trace_dump();
    */

    if (!check_happened_this_turn){
        if (led_array_ptr != NULL){
            ESP_LOG(WARN, TAG, "FREE led_array_ptr main");
            free(led_array_ptr);
            led_array_ptr = NULL;
        }
    } else {
        ESP_LOG(WARN, TAG, "Check happened this turn. need to check if any enemy figures or moves can protect king from check");
        display_send_message_to_display("Check!");
        if (checkmate_check(updated_pos) != ESP_OK){
            ESP_LOG(ERROR, TAG, "No possible moves left to do");
            local_data.checkmate = true;
        }
    }
    
    if (local_data.current_attackable) {
        ESP_LOG(WARN, TAG, "FREE local_data.current_attackable");
        free(local_data.current_attackable);
        local_data.current_attackable = NULL;
    }

    return ret;
FUNCTION_FAIL:
    ret = ESP_FAIL;

    // TODO: actually do this

    return ret;
}

static figure_position_t find_king_by_colour(chess_board_t board, bool white){
    figure_position_t king_pos;

    king_pos.pos_y = MATRIX_Y;
    king_pos.pos_x = MATRIX_X;

    for (int i = 0; i < MATRIX_Y; i++){
        for (int j = 0; j < MATRIX_X; j++){
            if (board.board[i][j].figure_type == FIGURE_KING && board.board[i][j].white == white){
                ESP_LOG(INFO, TAG, "Found %s king at pos %d:%d", white ? "white" : "black", i, j);
                king_pos.pos_y = i;
                king_pos.pos_x = j;
            }
        }
    }

    return king_pos;

}

static void send_checking_matrix_to_device_service() {
    bool b_matrix[MATRIX_Y][MATRIX_X];

    for (int i = 0; i < MATRIX_Y; i++){
        for (int j = 0; j < MATRIX_X; j++){
            b_matrix[i][j] = false;
            if (local_data.board.board[i][j].figure_type != FIGURE_END_LIST){
                ESP_LOG(INFO, TAG, "Figure should be on %d:%d", i, j);
                b_matrix[i][j] = true;
            }
        }
    }

    device_receive_required_positions(b_matrix);

}

static void chess_engine_task(void *args){

    
    state_change_data_t change_data;
    ESP_LOG(ERROR, TAG, "Sizeof board struct: %d", sizeof(chess_board_t));
    ESP_LOG(ERROR, TAG, "Sizeof local data struct: %d", sizeof(local_data_t));

    bool is_white_turn = true;

    chess_board_t board;

    bool allowed_to_do_move = true;

    figure_position_t attacking_figure_initial_position;
    bool attacking = false;
    bool last_figure_lifted = false;
    bool allow_lifting_enemy_figure = false;
    bool different_colours = false;
    bool same_position = false;
    bool moving_to_empty = false;
    bool target_empty = false;
    bool valid_colour_for_turn = false;
    bool last_figure_valid_colour = false;

    bool check = false;

    figure_position_t temp_pos;
    temp_pos.pos_y = MATRIX_Y;
    temp_pos.pos_x = MATRIX_X;

    bool checkmate = false;

    send_checking_matrix_to_device_service();

    if (display_send_message_to_display("Waiting for game to be setup and started") != ESP_OK){
            ESP_LOG(ERROR, TAG, "Failed to post turn data to display");
        }

    EventBits_t bits;
    while(1) {
        bits = xEventGroupWaitBits(local_data.event_handle, BOARD_READY_BIT_0, pdTRUE, pdTRUE, SECOND_TICK);
        if ((bits & BOARD_READY_BIT_0) != 0){
            ESP_LOG(INFO, TAG, "Board is set! Starting game");
            break;
        }
    }

    if (local_data.board.white_turn){
        if (display_send_message_to_display("white turn now!") != ESP_OK){
            ESP_LOG(ERROR, TAG, "Failed to post turn data to display");
        }
    }
    while(1){

        if (access_lock()){
            board = local_data.board;
            check = local_data.check;
            checkmate = local_data.checkmate;
            release_lock();
        } else {
            
        }

        if (checkmate){
            ESP_LOG(WARN, TAG, "CHECKMATE. Game is over");
            if (display_send_message_to_display("CHECKMATE") != ESP_OK){
                ESP_LOG(ERROR, TAG, "Failed to post chechmate message to display queue");
            }
            break;
        }

        if (uxQueueMessagesWaiting(local_data.queue) != 0){
                if (chess_queue_receiver(&change_data) != ESP_OK){

                } else {
                    allowed_to_do_move = true; // TODO: probably no longer needed
                    last_figure_lifted = local_data.last_change_data.lifted;
                    different_colours = (board.board[change_data.pos.pos_y][change_data.pos.pos_x].white != board.board[local_data.last_change_data.pos.pos_y][local_data.last_change_data.pos.pos_x].white);
                    same_position = ((change_data.pos.pos_y == local_data.last_change_data.pos.pos_y) && (change_data.pos.pos_x == local_data.last_change_data.pos.pos_x));
                    target_empty = (board.board[change_data.pos.pos_y][change_data.pos.pos_x].figure_type == FIGURE_END_LIST);
                    moving_to_empty = (target_empty && ((local_data.board.board[local_data.last_change_data.pos.pos_y][local_data.last_change_data.pos.pos_x].white == local_data.board.white_turn)));
                    valid_colour_for_turn = (board.board[change_data.pos.pos_y][change_data.pos.pos_x].white == board.white_turn);
                    last_figure_valid_colour = (board.board[local_data.last_change_data.pos.pos_y][local_data.last_change_data.pos.pos_x].white == board.white_turn);

                    ESP_LOG(WARN, TAG, "Different colours: %d same position %d target empty %d moving_to_empty %d valid_colour_for_turn %d", different_colours, same_position, target_empty, moving_to_empty, valid_colour_for_turn);

                    //allow_lifting_enemy_figure = local_data.last_change_data.lifted

                    ESP_LOG(WARN, TAG, "Received positions: %d:%d. Figure was %s:%d", change_data.pos.pos_y, change_data.pos.pos_x, change_data.lifted ? "lifted" : "put down", change_data.lifted);
                    ESP_LOG(WARN, TAG, "Last saved state change positions: %d:%d. figure was %s:%d",local_data.last_change_data.pos.pos_y, local_data.last_change_data.pos.pos_x, local_data.last_change_data.lifted ? "lifted" : "put down", local_data.last_change_data.lifted);
                    // Checking if the figure that changed state is not the same that done it previously
                    

                    if (change_data.lifted){
                        
                        if (last_figure_lifted){

                            if (different_colours && !target_empty) {

                                attacking_figure_initial_position = local_data.last_change_data.pos;
                                ESP_LOG(WARN, TAG, "Attacking figure position %d:%d", attacking_figure_initial_position.pos_y, attacking_figure_initial_position.pos_x);
                                attacking = true;

                                //local_data.board.white_turn = !local_data.board.white_turn; // Changing turns
                                //ESP_LOG(INFO, TAG, "%s turn now!", local_data.board.white_turn ? "white" : "black");

                            } else {
                                ESP_LOG(ERROR, TAG, "Something's wrong. I can feel it. Same colours lifted");
                            }

                        } else {

                            if (valid_colour_for_turn) {

                                local_data.board.board[temp_pos.pos_y][temp_pos.pos_x] = local_data.board.board[change_data.pos.pos_y][change_data.pos.pos_x];
                                local_data.board.board[change_data.pos.pos_y][change_data.pos.pos_x].led_op = NULL;
                                local_data.board.board[change_data.pos.pos_y][change_data.pos.pos_x].figure_type = FIGURE_END_LIST;
                                
                                
                                if (check_all_enemy_moves_for_pos(board, find_king_by_colour(board, local_data.board.white_turn), false) == ESP_ERR_NO_MEM){
                                    ESP_LOG(WARN, TAG, "Can't move this figure because it'll cause a checkmate");
                                    local_data.board.board[change_data.pos.pos_y][change_data.pos.pos_x] = local_data.board.board[temp_pos.pos_y][temp_pos.pos_x];
                                    local_data.board.board[temp_pos.pos_y][temp_pos.pos_x].led_op = NULL;
                                    local_data.board.board[temp_pos.pos_y][temp_pos.pos_x].figure_type = FIGURE_END_LIST;
                                    led_no_move_possible(MATRIX_TO_ARRAY_CONVERSION(change_data.pos.pos_y, change_data.pos.pos_x));
                                    
                                } else {
                                    ESP_LOG(INFO, TAG, "Moving this figure %d:%d won't cause immediate checkmate", change_data.pos.pos_y, change_data.pos.pos_x);
                                    local_data.board.board[change_data.pos.pos_y][change_data.pos.pos_x] = local_data.board.board[temp_pos.pos_y][temp_pos.pos_x];
                                    local_data.board.board[temp_pos.pos_y][temp_pos.pos_x].led_op = NULL;
                                    local_data.board.board[temp_pos.pos_y][temp_pos.pos_x].figure_type = FIGURE_END_LIST;
                                    required_leds_calculation(change_data.pos, CALCULATIONS_WITH_LEDS, false);

                                }

                                
                                
                            } else {
                                ESP_LOG(WARN, TAG, "Here1");
                                ESP_LOG(ERROR, TAG, "Not %s turn", board.board[change_data.pos.pos_y][change_data.pos.pos_x].white ? "white" : "black");
                                led_no_move_possible(MATRIX_TO_ARRAY_CONVERSION(change_data.pos.pos_y, change_data.pos.pos_x));
                            }
                        }

                    } else {

                        if (moving_to_empty && last_figure_lifted){

                            if (last_figure_valid_colour){

                                local_data.board.board[change_data.pos.pos_y][change_data.pos.pos_x] = local_data.board.board[local_data.last_change_data.pos.pos_y][local_data.last_change_data.pos.pos_x];
                                local_data.board.board[local_data.last_change_data.pos.pos_y][local_data.last_change_data.pos.pos_x].led_op = NULL;
                                local_data.board.board[local_data.last_change_data.pos.pos_y][local_data.last_change_data.pos.pos_x].figure_type = FIGURE_END_LIST;
                                        // TODO: really double check above

                                        
                                        
                                ESP_LOG(WARN, TAG, "Figure was moved from %d:%d to %d:%d", local_data.last_change_data.pos.pos_y, local_data.last_change_data.pos.pos_x, change_data.pos.pos_y, change_data.pos.pos_x);

                                if (check){
                                    if (!local_data.check_trajectory || local_data.trajectory_counter == 0){
                                        ESP_LOG(ERROR, TAG, "Something went really wrong"); 
                                        continue; // TODO: not sure what to do in this situation
                                    }
                                    uint8_t new_pos = 0;
                                    new_pos = MATRIX_TO_ARRAY_CONVERSION((change_data.pos.pos_y), (change_data.pos.pos_x)); 

                                    bool not_on_trajectory = true;

                                    for (int i = 0; i < local_data.trajectory_counter; i++){
                                        if (local_data.check_trajectory[i] == new_pos){
                                            not_on_trajectory = false;
                                            ESP_LOG(WARN, TAG, "Check eliminated by moving here %d:%d converted %d", change_data.pos.pos_y, change_data.pos.pos_x, MATRIX_TO_ARRAY_CONVERSION((change_data.pos.pos_y), (change_data.pos.pos_x)));
                                            local_data.check = false;
                                            local_data.trajectory_counter = 0;
                                            ESP_LOG(WARN, TAG, "FREE local_data.check_trajectory");
                                            free(local_data.check_trajectory);
                                            local_data.check_trajectory = NULL;
                                            break;
                                        }
                                    }
                                    if (not_on_trajectory) {
                                        ESP_LOG(WARN, TAG, "Figure moved out of the check trajectory");
                                        local_data.check = false;
                                        
                                        if (local_data.check_trajectory) {
                                            ESP_LOG(WARN, TAG, "FREE local_data.check_trajectory");
                                            local_data.trajectory_counter = 0;
                                            free(local_data.check_trajectory);
                                            local_data.check_trajectory = NULL;
                                        }
                                    }

                                }
                                required_leds_calculation(change_data.pos, CALCULATIONS_WITHOUT_LEDS, false); // TODO: why this is here?

                                local_data.board.white_turn = !local_data.board.white_turn; // Changing turns
                                ESP_LOG(INFO, TAG, "%s turn now!", local_data.board.white_turn ? "white" : "black");

                                char *colour;
                                if (local_data.board.white_turn){
                                    colour = "white";
                                } else {
                                    colour = "black";
                                }
                                //char colour[5] = local_data.board.white_turn ? "white" : "black";
                                char rest[] = " turn now!";

                                uint8_t colour_length = strlen(colour);
                                uint8_t rest_length = strlen(rest);

                                char full_message[(colour_length + rest_length + 1)]; 

                                strcat(full_message, colour);
                                strcat(full_message, rest);

                                ESP_LOG(WARN, TAG, "Message %s", full_message);

                                if (display_send_message_to_display(full_message) != ESP_OK){
                                    ESP_LOG(ERROR, TAG, "Failure on posting message to device service");
                                }

                                led_clear_stripe();

                            } else {
                                ESP_LOG(WARN, TAG, "Here2");
                                ESP_LOG(ERROR, TAG, "Not %s turn", board.board[change_data.pos.pos_y][change_data.pos.pos_x].white ? "white" : "black");
                                led_no_move_possible(MATRIX_TO_ARRAY_CONVERSION(change_data.pos.pos_y, change_data.pos.pos_x));
                            }

                        } else if (attacking && last_figure_lifted) {
                            
                            ESP_LOG(WARN, TAG, "Figure %d on %d:%d was attacked by %d from %d:%d", local_data.board.board[change_data.pos.pos_y][change_data.pos.pos_x].figure_type, change_data.pos.pos_y, change_data.pos.pos_x, local_data.board.board[attacking_figure_initial_position.pos_y][attacking_figure_initial_position.pos_x].figure_type, attacking_figure_initial_position.pos_y, attacking_figure_initial_position.pos_x);
                                
                            local_data.board.board[change_data.pos.pos_y][change_data.pos.pos_x] = local_data.board.board[attacking_figure_initial_position.pos_y][attacking_figure_initial_position.pos_x];
                            local_data.board.board[attacking_figure_initial_position.pos_y][attacking_figure_initial_position.pos_x].figure_type = FIGURE_END_LIST;
                            local_data.board.board[attacking_figure_initial_position.pos_y][attacking_figure_initial_position.pos_x].led_op = NULL;
                            

                            if (check){
                                    if (!local_data.check_trajectory || local_data.trajectory_counter == 0){
                                        ESP_LOG(ERROR, TAG, "Something went really wrong"); 
                                        continue; // TODO: not sure what to do in this situation
                                    }
                                    uint8_t new_pos = 0;
                                    new_pos = MATRIX_TO_ARRAY_CONVERSION((change_data.pos.pos_y), (change_data.pos.pos_x));    
                                    for (int i = 0; i < local_data.trajectory_counter; i++){
                                        if (local_data.check_trajectory[i] == new_pos){
                                            ESP_LOG(WARN, TAG, "Check eliminated by attacking here %d:%d converted %d", change_data.pos.pos_y, change_data.pos.pos_x, MATRIX_TO_ARRAY_CONVERSION((change_data.pos.pos_y), (change_data.pos.pos_x)));
                                            local_data.check = false;
                                            local_data.trajectory_counter = 0;
                                            ESP_LOG(WARN, TAG, "FREE local_data.check_trajectory");
                                            free(local_data.check_trajectory);
                                            local_data.check_trajectory = NULL;
                                            break;
                                        }
                                    }
                                }

                            required_leds_calculation(change_data.pos, CALCULATIONS_WITHOUT_LEDS, false);

                            local_data.board.white_turn = !local_data.board.white_turn; // Changing turns
                            ESP_LOG(INFO, TAG, "%s turn now!", local_data.board.white_turn ? "white" : "black");
                            char *colour;
                            if (local_data.board.white_turn){
                                colour = "white";
                            } else {
                                colour = "black";
                            }
                            //char colour[5] = local_data.board.white_turn ? "white" : "black";
                            char rest[] = " turn now!";

                            uint8_t colour_length = strlen(colour);
                            uint8_t rest_length = strlen(rest);

                            char full_message[(colour_length + rest_length + 1)]; 

                            strcat(full_message, colour);
                            strcat(full_message, rest);

                            ESP_LOG(WARN, TAG, "Message %s", full_message);

                            if (display_send_message_to_display(full_message) != ESP_OK){
                                ESP_LOG(ERROR, TAG, "Failure on posting message to device service");
                            }

                            attacking = false;

                            led_clear_stripe();

                        } else if (last_figure_lifted) { // not attacking but new put down and old lifted
                            
                            
                            if (same_position) {
                                ESP_LOG(INFO, TAG, "Same figure put down");
                                if (led_clear_stripe() != ESP_OK){
                                    ESP_LOG(ERROR, TAG, "Failed to clear led stripe");
                                }
                            } else {
                                ESP_LOG(ERROR, TAG, "Something's wrong. I can feel it");
                            }
                            
                        }

                    }
                    
                    local_data.last_change_data = change_data;
                    
            }

            
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

esp_err_t chess_engine_device_service_ready(){
    xEventGroupSetBits(local_data.event_handle, BOARD_READY_BIT_0);
    return ESP_OK;
}

esp_err_t update_board_on_lift(state_change_data_t data) {

    ESP_LOG(INFO, TAG, "%d:%d %s %d", data.pos.pos_y, data.pos.pos_x, data.lifted ? "lifted" : "put down", data.lifted);

    if (access_lock()) {
        
        if (xQueueSend(local_data.queue, (void*)&data, (TickType_t)10) != pdTRUE) {
            ESP_LOG(ERROR, TAG, "Failed to post pos x to queue. Aborting");
            release_lock();
            return ESP_FAIL;
        }
        
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