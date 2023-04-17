#pragma once

#include "common_components.h"
#include "logger_service.h"
#include "chess_engine.h"

#include "esp_err.h"
#include "driver/gpio.h"


esp_err_t device_get_pin_level(int pin, uint8_t *level);

esp_err_t device_set_pin_level(int pin, uint8_t level);

esp_err_t device_receive_required_positions(bool matrix[][MATRIX_X]);

esp_err_t device_init();
