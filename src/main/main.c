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

// Uncomment for initializiation of spinlock
// static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
static const char *TAG = "main";
static TaskHandle_t highPrioTaskHandle = NULL;
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

void uartBufferReaderTask(void *pvParameter)
{
    ESP_LOGI(TAG, "UART buffer reader task started");
    while (1) {
        if (uartBuffer[0] != 0) {
            ESP_LOGI(TAG, "Buffer contains: %s", (char *) uartBuffer);

            // Reset buffer.
            memset(uartBuffer, 0, UART_BUF_SIZE);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

int simulate_serial_read(void)
{
    int r = rand() % 100;
    return r;
}

void high_priority_task(void *pvParameter)
{
    while (1) {
        int r = simulate_serial_read();
        ESP_LOGI(TAG, "Serial: %d", r);
        if (r > 90) {
            ESP_LOGI(TAG, "High value! Write to serial...");
            vTaskDelay(500 / portTICK_PERIOD_MS);
        } else if (r < 10) {
            ESP_LOGI(TAG, "Low value! Write to serial...");
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Do some other stuff here...");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    // start_provisioning(NULL);

    // #if defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE)
    //     /**
    //      * We are treating successful WiFi connection as a checkpoint to cancel rollback
    //      * process and mark newly updated firmware image as active. For production cases,
    //      * please tune the checkpoint behavior per end application requirement.
    //      */
    //     const esp_partition_t *running = esp_ota_get_running_partition();
    //     esp_ota_img_states_t ota_state;
    //     if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
    //         if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
    //             if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
    //                 ESP_LOGI(TAG, "App is valid, rollback cancelled successfully");
    //             } else {
    //                 ESP_LOGE(TAG, "Failed to cancel rollback");
    //             }
    //         }
    //     }
    // #endif

    // // Ensure to disable any WiFi power save mode, this allows best throughput
    // // and hence timings for overall OTA operation.
    // esp_wifi_set_ps(WIFI_PS_NONE);

    // We could pin the high priority task to a specific core and let the other tasks fight for the other core.
    UBaseType_t uxPriorityHighPriorityTask = 15;
    BaseType_t highPriorityTaskStatus = xTaskCreate(
        high_priority_task,
        "high_priority_task",
        1024 * 8,
        NULL,
        uxPriorityHighPriorityTask,
        &highPrioTaskHandle
    );
    // BaseType_t highPriorityTaskStatus = xTaskCreatePinnedToCore(high_priority_task, "high_priority_task", 1024 * 8, NULL, 10, &highPrioTaskHandle, 1);
    if (highPriorityTaskStatus != pdPASS) {
        ESP_LOGE(TAG, "Error creating high priority task! Error code: %d", highPriorityTaskStatus);
    } else
    {
        ESP_LOGI(TAG, "High priority task created successfully!");
    }


    // Create a handle for the OTA task.
    // The handle is used to refer to the task later, e.g. to delete the task.
    BaseType_t otaTaskStatus = xTaskCreate(start_ota, "start_ota", 1024 * 8, NULL, 5, &otaTaskHandle);
    // BaseType_t otaTaskStatus = xTaskCreatePinnedToCore(start_ota, "start_ota", 1024 * 8, NULL, 5, &otaTaskHandle, 1);
    if (otaTaskStatus != pdPASS) {
        ESP_LOGE(TAG, "Error creating OTA task! Error code: %d", otaTaskStatus);
        // TODO: Handle error.
    } else
    {
        ESP_LOGI(TAG, "OTA task created successfully!");
    }

    // TimerHandle_t otaTimerHandle = xTimerCreate("otaTimer", pdMS_TO_TICKS(36*100000), pdTRUE, (void*)1, otaTimerCallback);
    // if (otaTimerHandle == NULL)
    // {
    //     ESP_LOGE(TAG, "Error creating OTA timer!");
    // }
    // else
    // {
    //     ESP_LOGI(TAG, "OTA timer created successfully!");
    //     BaseType_t timerStartState = xTimerStart(otaTimerHandle, 0);
    //     if (timerStartState != pdPASS)
    //     {
    //         ESP_LOGE(TAG, "Error starting OTA timer!");
    //         // TODO: Handle error.
    //     }
    //     else
    //     {
    //         ESP_LOGI(TAG, "OTA timer started successfully!");
    //     }
    // }

    uartBuffer = (uint8_t *) malloc(UART_BUF_SIZE);
    xTaskCreate(start_uart_echo, "uart_echo_task", ECHO_TASK_STACK_SIZE, uartBuffer, 10, NULL);
    xTaskCreate(uartBufferReaderTask, "uartBufferReaderTask", 1024 * 2, NULL, 5, NULL);
}
