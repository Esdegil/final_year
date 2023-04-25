/**
* @file chess_engine.h
* @brief Chess Engine Service.
*
* Chess Engine Service essentially is the main body of this software.
* It receives information from Device Service, carries out necessary calculations and passes required data further to Display Service and/or LED Service.
*
* @author Vladislavas Putys
*
*/

#include "common_components.h"
#include "logger_service.h"
#include "led_service.h"
#include "display_service.h"
#include "device.h"



/**
* @brief Chess Engine initialisation function.
*
* @returns ESP_OK if successful
* @returns ESP_FAIL if failed
*
*/
esp_err_t chess_engine_init();

/**
* @brief Function that updates chess engine with new state of the board.
*
* @param [in] state_change_data_t data structure with information on happened change 
*
* @returns ESP_OK if successful
* @returns ESP_FAIL if failed
*
*/
esp_err_t update_board_on_lift(state_change_data_t data);

/**
* @brief Function to notify Chess Engine the initial device setup is finished. 
*
* @returns ESP_OK if successful
* @returns ESP_FAIL if failed
*
*/
esp_err_t chess_engine_device_service_ready();