//This file contains common stuff that will be used across other components and main file
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"

#define SECOND_TICK 1000/portTICK_PERIOD_MS

#define TEST_VALUE 15