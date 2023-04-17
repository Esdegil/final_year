idf_component_register(SRCS "device.c"
                    INCLUDE_DIRS "include" REQUIRES common_components driver logger_service freertos chess_engine display_service)
