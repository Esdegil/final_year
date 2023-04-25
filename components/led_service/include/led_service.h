/**
* @file led_service.h
* @brief LED Service header file.
*
* Contains functions allowed to be used by other services.
*
* @author Vladislavas Putys
*
*/
#include "common_components.h"
#include "logger_service.h"


#include "../../../ESP32_LED_STRIP/components/led_strip/inc/led_strip/led_strip.h"
 
 /**
* @brief LED Service initialisation function.
*
* @returns ESP_OK if successful
* @returns ESP_FAIL if failed
*
*/
esp_err_t led_service_init();

/**
* @brief General LED operation function.
*
* See @ref led_operation_t "led_operation_t" and @ref figure_data_t "figure_data_t" for more information.
*
* @param [in] uint8_t pointer to array of uint8_t type.
* @param [in] uint8_t variable containing data with number of elements in the array.
* @param [in] bool variable with colour data. 
*
* @returns ESP_OK if successful
* @returns ESP_FAIL if failed
*
*/
esp_err_t led_op_general(uint8_t *arr, uint8_t counter, bool white);

/**
* @brief Clearing current LED Strip.
*
* Function to clear(turn off) whole LED Strip.
*
* @returns ESP_OK if successful
* @returns ESP_FAIL if failed
*
*/
esp_err_t led_clear_stripe();

/**
* @brief Function to use when something incorrect happens.
*
* Lights up with red colour for chosen position. 
*
* Can be used when calculations failed or show that no moves are possible for lifted figure. Also used as general visual error representation.
*
* @param [in] uint8_t variable corresponding to position that should be lit up.
*
* @returns ESP_OK if successful
* @returns ESP_FAIL if failed
*
*/
esp_err_t led_no_move_possible(uint8_t position);

esp_err_t led_test();
esp_err_t led_test2();
esp_err_t led_test3();