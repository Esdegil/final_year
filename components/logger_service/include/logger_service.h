/**
* @file logger_service.h
* @brief File contating all-in-one macro for logging.
*
* @author Vladislavas Putys
*
*/

#include "common_components.h"

#include "esp_log.h"

#define ERROR ESP_LOG_ERROR
#define WARN ESP_LOG_WARN
#define INFO ESP_LOG_INFO
#define DEBUG ESP_LOG_DEBUG

#define ESP_LOG(level, tag, format, args...) \
if (level == ESP_LOG_ERROR) ESP_LOGE(tag, format, ##args); \
else if (level == ESP_LOG_WARN) ESP_LOGW(tag, format, ##args); \
else if (level == ESP_LOG_INFO) ESP_LOGI(tag, format, ##args); \
else if (level == ESP_LOG_DEBUG) ESP_LOGD(tag, format, ##args);
