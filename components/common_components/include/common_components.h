//This file contains common stuff that will be used across other components and main file
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_event.h"

#pragma once

#define SECOND_TICK 1000/portTICK_PERIOD_MS

#define TEST_VALUE 15

#define FIGURE_LIFTED_BIT_0 (1 << 0)

ESP_EVENT_DECLARE_BASE(TEST_EVENTS);

#define MATRIX_X 3
#define MATRIX_Y 3

#define FULL_BOARD (MATRIX_X == 8 && MATRIX_Y == 8) // for developing 

#define MAX_PAWN_MOVES 4 // TODO: rethink or recount this
#define MAX_KNIGHT_MOVES 8

#define PAWN_MAX_FORWARD_MOVEMENT 2

#define MATRIX_TO_ARRAY_CONVERSION(pos_y, pos_x) (pos_y * MATRIX_Y + pos_x)

typedef struct figure_position {
    uint8_t pos_x;
    uint8_t pos_y;
} figure_position_t;

typedef struct state_change_data {
    figure_position_t pos;
    bool lifted;
} state_change_data_t;

typedef void(*led_operation_t)(uint8_t*, uint8_t);// last () are arguments.

typedef enum chess_figures {
    FIGURE_PAWN = 0,
    FIGURE_ROOK = 1,
    FIGURE_KNIGHT = 2,
    FIGURE_BISHOP = 3,
    FIGURE_QUEEN = 4,
    FIGURE_KING = 5,
    FIGURE_END_LIST
} chess_figures_t;
// TODO: pawns require a bool of first move
typedef struct figure_data {
    chess_figures_t figure_type;
    bool white;
    uint8_t pos_x;
    uint8_t pos_y;
    led_operation_t led_op;
}figure_data_t;

typedef struct chess_board { // TODO: this maybe needs rethinking
    figure_data_t board[MATRIX_Y+1][MATRIX_X+1];
    bool white_turn;
} chess_board_t;

typedef enum events {

    EVENT_MATRIX_SWITCH_OPEN = 0,
    EVENT_MATRIX_SWITCH_CLOSED = 1

}events_t;
