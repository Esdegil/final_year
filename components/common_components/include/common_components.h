/**
* @file common_components.h
* @brief File contating common data structures, definitions and macros.
*
* @author Vladislavas Putys
*
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_event.h"

#pragma once

/** 
* @def SECOND_TICK 
* @brief Macro to represent conversion of 1s length.
*
*/
#define SECOND_TICK 1000/portTICK_PERIOD_MS

/** 
* @def BOARD_READY_BIT_0 
* @brief bit to indicate that board is ready.
*
*/
#define BOARD_READY_BIT_0 (1 << 0)

/** 
* @def ESP_EVENT_DECLARE_BASE 
* @brief UNUSED.
*
*/
ESP_EVENT_DECLARE_BASE(TEST_EVENTS);

/** 
* @def MATRIX_X 
* @brief number of cells on X coordinate.
*
*/
#define MATRIX_X 8

/** 
* @def MATRIX_Y
* @brief number of cells on Y coordinate.
*
*/
#define MATRIX_Y 8

/** 
* @def FULL_BOARD
* @brief results to 1 or 0 depending on @ref MATRIX_X "MATRIX_X" and @ref MATRIX_Y "MATRIX_Y"
*
*/
#define FULL_BOARD (MATRIX_X == 8 && MATRIX_Y == 8) // for developing 

/** 
* @def MAX_KNIGHT_MOVES
* @brief defines maximum possible positions number for knight figure
*
*/
#define MAX_KNIGHT_MOVES 8

/** 
* @def MATRIX_TO_ARRAY_CONVERSION
* @brief Matrix to array conversion macro function.
*
* Depends on the bool set in the global config
*
*/
#ifdef CONFIG_BREADBOARD_LED_SETUP
#define MATRIX_TO_ARRAY_CONVERSION(pos_y, pos_x) (pos_y * MATRIX_Y + pos_x)
#else
#define MATRIX_TO_ARRAY_CONVERSION(pos_y, pos_x) (pos_y % 2 == 0) ? ((pos_y * MATRIX_Y) + pos_x) : (pos_y * MATRIX_Y + (MATRIX_X - pos_x - 1))
#endif


/** 
 * @typedef typedef struct figure_position figure_position_t
* @brief Data structure holding 2 uint8_t's corresponding to X and Y coordinates.
*
*   
*/
typedef struct figure_position {
    uint8_t pos_x;
    uint8_t pos_y;
} figure_position_t;

/** 
 * @typedef typedef struct state_change_data state_change_data_t
* @brief Data structure with coordinates and lifted state.
*
*  contains a bool for lifted status indication and @ref figure_position_t "figure_position_t structure"
*/
typedef struct state_change_data {
    figure_position_t pos;
    bool lifted;
} state_change_data_t;

/** 
* @var typedef void(*led_operation_t)(uint8_t*, uint8_t, bool) led_operation_t
* @brief A pointer to function.
*
*/
typedef void(*led_operation_t)(uint8_t*, uint8_t, bool); // last () are arguments.

/** 
* @var  typedef enum chess_figures chess_figures_t
* @brief Represents chess figures.
*
*/
typedef enum chess_figures {
    FIGURE_PAWN = 0,
    FIGURE_ROOK = 1,
    FIGURE_KNIGHT = 2,
    FIGURE_BISHOP = 3,
    FIGURE_QUEEN = 4,
    FIGURE_KING = 5,
    FIGURE_END_LIST
} chess_figures_t;

/** 
* @typedef typedef struct figure_data figure_data_t
* @brief Data structure with data of one figure.
*
*  contains:
*
*  a bool for colour indication,
*
*  @ref chess_figures_t "chess_figures_t" enumerator
*
*  @ref led_operation_t "led_operation_t" pointer to function
*/
typedef struct figure_data {
    chess_figures_t figure_type;
    bool white;
    led_operation_t led_op;
}figure_data_t;

/** 
* @typedef typedef struct chess_board chess_board_t
* @brief Data structure containing information on the chessboard.
*
*  contains:
*
*  @ref figure_data_t "figure_data_t" type matrix of @ref MATRIX_Y "MATRIX_Y + 1" * @ref MATRIX_X "MATRIX_X + 1" size.
*
*  bool to keep track of current turn.
*/
typedef struct chess_board { 
    figure_data_t board[MATRIX_Y+1][MATRIX_X+1];
    bool white_turn;
} chess_board_t;

/** 
* @typedef typedef struct attackable_figures attackable_figures_t
* @brief data structure used in check/checkmate calculations.
*
*  contains:
*
*  @ref chess_figures_t "chess_figures_t" type of figure.
*
*  @ref figure_position_t "figure_position_t" coordinates of that figure.
*/
typedef struct attackable_figures {
    chess_figures_t figure;
    figure_position_t pos;
} attackable_figures_t;

/** 
* @var typedef enum events events_t
* @brief UNUSED.
*
*/
typedef enum events {

    EVENT_MATRIX_SWITCH_OPEN = 0,
    EVENT_MATRIX_SWITCH_CLOSED = 1

}events_t;
