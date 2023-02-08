#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
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

void app_main(void)
{
    energy_gateway_start_provisioning();
//   ESP_LOGI(TAG, "OTA example app_main start");
//     // Initialize NVS.
//     esp_err_t err = nvs_flash_init();
//     if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
//         // partition table. This size mismatch may cause NVS initialization to fail.
//         // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
//         // If this happens, we erase NVS partition and initialize NVS again.
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         err = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK( err );

//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());

//     /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
//      * Read "Establishing Wi-Fi or Ethernet Connection" section in
//      * examples/protocols/README.md for more information about this function.
//     */
//     ESP_ERROR_CHECK(example_connect());

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
    TaskHandle_t otaTaskHandle = NULL;
    BaseType_t otaTaskStatus = xTaskCreate(&advanced_ota_example_task, "advanced_ota_example_task", 1024 * 8, NULL, 5, &otaTaskHandle);
    if (otaTaskStatus != pdPASS) {
        ESP_LOGE(TAG, "Error creating OTA task! Error code: %d", otaTaskStatus);
        return 1;
    }
    {
        ESP_LOGI(TAG, "OTA task created successfully!");
    }

}