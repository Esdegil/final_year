#include "common_components.h"
#include "logger_service.h"

#include "ssd1306.h"


void func(void);

esp_err_t display_service_init();

esp_err_t display_send_message_to_display(char* message);
