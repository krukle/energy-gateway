#include <stdio.h>
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

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif

static const char *TAG = "main";
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t otaTaskHandle = NULL;

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

void app_main(void)
{
    energy_gateway_start_provisioning();

    #if defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE)
        /**
         * We are treating successful WiFi connection as a checkpoint to cancel rollback
         * process and mark newly updated firmware image as active. For production cases,
         * please tune the checkpoint behavior per end application requirement.
         */
        const esp_partition_t *running = esp_ota_get_running_partition();
        esp_ota_img_states_t ota_state;
        if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
            if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
                if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
                    ESP_LOGI(TAG, "App is valid, rollback cancelled successfully");
                } else {
                    ESP_LOGE(TAG, "Failed to cancel rollback");
                }
            }
        }
    #endif

    // Ensure to disable any WiFi power save mode, this allows best throughput
    // and hence timings for overall OTA operation.
    esp_wifi_set_ps(WIFI_PS_NONE);


    // Create a handle for the OTA task.
    // The handle is used to refer to the task later, e.g. to delete the task.
    BaseType_t otaTaskStatus = xTaskCreate(&start_ota, "start_ota", 1024 * 8, &spinlock, 5, &otaTaskHandle);
    if (otaTaskStatus != pdPASS) {
        ESP_LOGE(TAG, "Error creating OTA task! Error code: %d", otaTaskStatus);
        // TODO: Handle error.
    } else
    {
        ESP_LOGI(TAG, "OTA task created successfully!");
    }


    TimerHandle_t otaTimerHandle = xTimerCreate("otaTimer", pdMS_TO_TICKS(1000*20), pdTRUE, (void*)1, otaTimerCallback);
    if (otaTimerHandle == NULL)
    {
        ESP_LOGE(TAG, "Error creating OTA timer!");
    }
    else
    {
        ESP_LOGI(TAG, "OTA timer created successfully!");
        BaseType_t timerStartState = xTimerStart(otaTimerHandle, 0);
        if (timerStartState != pdPASS)
        {
            ESP_LOGE(TAG, "Error starting OTA timer!");
            // TODO: Handle error.
        }
        else
        {
            ESP_LOGI(TAG, "OTA timer started successfully!");
        }
    }
}
