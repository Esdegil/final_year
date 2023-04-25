/**
* @file device.h
* @brief Device Service header file.
*
* Contains functions allowed to be used by other services.
*
* @author Vladislavas Putys
*
*/

#pragma once

#include "common_components.h"
#include "logger_service.h"
#include "chess_engine.h"
#include "display_service.h"

#include "esp_err.h"
#include "driver/gpio.h"

/**
* @brief Getter function for needed pin.
*
* Getter function returning the level of chosen pin.
*
* @param [in] int corresponding to pin.
* @param [out] uint8_t pointer to variable that will be updated with read state.
*
* @returns ESP_OK if successful
* @returns ESP_FAIL if failed
*
*/
esp_err_t device_get_pin_level(int pin, uint8_t *level);

/**
* @brief Setter function for needed pin.
*
* Sets required pin to required state.
*
* @param [in] int corresponding to pin.
* @param [in] uint8_t variable pin will be set to. Can be 1 or 0.
*
* @returns ESP_OK if successful
* @returns ESP_FAIL if failed
*
*/
esp_err_t device_set_pin_level(int pin, uint8_t level);

/**
* @brief Updates Device Services with required board state.
*
* Updates Device Services with required board state acquired from Chess Engine Service.
*
* @param [in] chess_board_t a copy of current board state stored in Chess Engine Service. 
*
* @returns ESP_OK if successful
* @returns ESP_FAIL if failed
*
*/
esp_err_t device_receive_required_positions(chess_board_t board);

/**
* @brief Device Service initialisation function.
*
* @returns ESP_OK if successful
* @returns ESP_FAIL if failed
*
*/
esp_err_t device_init();
