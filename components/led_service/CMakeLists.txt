idf_component_register(SRCS "led_service.c" "../../ESP32_LED_STRIP/components/led_strip/led_strip.c"
                    INCLUDE_DIRS "include" "../../ESP32_LED_STRIP/components/led_strip/inc/led_strip" REQUIRES common_components freertos logger_service driver)
