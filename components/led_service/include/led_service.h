#include "common_components.h"
#include "logger_service.h"


#include "../../../ESP32_LED_STRIP/components/led_strip/inc/led_strip/led_strip.h"
 
esp_err_t led_service_init();

esp_err_t led_op_pawn(uint8_t *arr, uint8_t counter);

esp_err_t led_clear_stripe();
esp_err_t led_no_move_possible(uint8_t position);

esp_err_t led_test();
esp_err_t led_test2();
esp_err_t led_test3();