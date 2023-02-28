#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "protocol_examples_common.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"

#include "energy_gateway_ota.h"
#include "energy_gateway_provisioning.h"
#include "energy_gateway_uart.h"

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif

#define ESP_TASK_CUSTOM_PRIO_MAX  (ESP_TASK_PRIO_MAX - 6) // 19 : Over networks tasks but under interrupts and events.

static const char *TAG = "main";
static TaskHandle_t uartTaskHandle = NULL;
static TaskHandle_t otaTaskHandle = NULL;
static uint8_t *uartBuffer = NULL;

void otaTimerCallback( TimerHandle_t pxTimer )
{
    if (otaTaskHandle != NULL) {
        ESP_LOGI("otaTimerCallback", "Resuming OTA task");
        vTaskResume( otaTaskHandle );
    } else {
        ESP_LOGE("otaTimerCallback", "OTA task handle is NULL. Cannot resume OTA task.");
        // TODO: Handle error.
    }
}

void reboot_on_error(BaseType_t to_check, const char *msg)
{
    if (to_check != pdPASS) {
        ESP_LOGE(TAG, "Error: %s. Code: %d", msg, to_check);
        esp_restart();
    }
}

void app_main(void)
{
    start_provisioning(NULL);
    
    esp_wifi_set_ps(WIFI_PS_NONE); // Ensure to disable any WiFi power save mode, this allows best throughput

    uartBuffer = (uint8_t *) malloc(UART_BUF_SIZE);
    BaseType_t highPriorityTaskStatus = xTaskCreate(
        start_uart_echo,
        "uart_task",
        ECHO_TASK_STACK_SIZE,
        uartBuffer,
        ESP_TASK_CUSTOM_PRIO_MAX,
        &uartTaskHandle
    );
    reboot_on_error(highPriorityTaskStatus, "Error creating high priority task!");
    ESP_LOGI(TAG, "High priority task created successfully!");

    BaseType_t otaTaskStatus = xTaskCreate(start_ota, "start_ota", 1024 * 8, NULL, ESP_TASK_MAIN_PRIO + 1, &otaTaskHandle);
    reboot_on_error(otaTaskStatus, "Error creating OTA task!");
    ESP_LOGI(TAG, "OTA task created successfully!");

    // Test with lower value to see if uart task is not interrupted.
    // TimerHandle_t otaTimerHandle = xTimerCreate("otaTimer", pdMS_TO_TICKS(36*100000), pdTRUE, (void*)1, otaTimerCallback);
    TimerHandle_t otaTimerHandle = xTimerCreate("otaTimer", pdMS_TO_TICKS(5000), pdTRUE, (void*)1, otaTimerCallback);
    if (otaTimerHandle == NULL)
    {
        // TODO: Handle error. Should we send it somewhere?
        ESP_LOGE(TAG, "Error creating OTA timer!");
        esp_restart();
    }
    ESP_LOGI(TAG, "OTA timer created successfully!");

    BaseType_t timerStartState = xTimerStart(otaTimerHandle, 0);
    reboot_on_error(timerStartState, "Error starting OTA timer!");
    ESP_LOGI(TAG, "OTA timer started successfully!");
}
