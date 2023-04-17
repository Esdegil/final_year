#include "common_components.h"
#include "logger_service.h"
#include "led_service.h"
#include "display_service.h"
#include "device.h"

esp_err_t chess_engine_init();

esp_err_t update_board_on_lift(state_change_data_t data);

esp_err_t chess_engine_device_service_ready();