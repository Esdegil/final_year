/**
* @file display_service.h
* @brief Display Service.
*
* Display Service main purpose is to parse strings passed to it and input them to SSD1306 driver library.
* It receives data from all other services, parses it and passes it further.
*
* @author Vladislavas Putys
*
*/
#include "common_components.h"
#include "logger_service.h"

#include "ssd1306.h"

/**
* @brief Display Service initialisation function.
*
* @returns ESP_OK if successful
* @returns ESP_FAIL if failed
*
*/
esp_err_t display_service_init();

/**
* @brief Function that allows other services to send text information.
*
* This function allosws other services to send a string that should be shown to User via the display. 
*
* @param [in] char array with text that should be displayed. 
*
* @returns ESP_OK if successful
* @returns ESP_FAIL if failed
*
*/
esp_err_t display_send_message_to_display(char* message);
