//This file contains common stuff that will be used across other components and main file
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"

#pragma once

#define SECOND_TICK 1000/portTICK_PERIOD_MS

#define TEST_VALUE 15


#define MATRIX_X 3
#define MATRIX_Y 3

typedef void(*led_operation_t)(uint8_t, uint8_t);// last () is arguments. can be (int, int)

typedef enum chess_figures {
    FIGURE_PAWN = 0,
    FIGURE_ROOK = 1,
    FIGURE_KNIGHT = 2,
    FIGURE_BISHOP = 3,
    FIGURE_QUEEN = 4,
    FIGURE_KING = 5,
    FIGURE_END_LIST
} chess_figures_t;

typedef struct figure_data {
    chess_figures_t figure_type;
    bool white;
    uint8_t pos_x;
    uint8_t pos_y;
    led_operation_t led_op;
}figure_data_t;

typedef struct chess_board {
    figure_data_t board[MATRIX_Y+1][MATRIX_X+1];
    bool white_turn;
} chess_board_t;