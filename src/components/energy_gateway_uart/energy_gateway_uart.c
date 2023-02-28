/* Energy Gateway UART

   This example code is in the Public Domain (or CC0 licensed, at 
   your option) and is heavily based on the example code from espressif.

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

/**
 * This is an example which echos any data it receives on configured UART back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: configured UART
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below (See Kconfig)
 */

#define ECHO_TEST_TXD (CONFIG_EXAMPLE_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_EXAMPLE_UART_RXD)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM      (CONFIG_EXAMPLE_UART_PORT_NUM)
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define ECHO_TASK_STACK_SIZE    (CONFIG_EXAMPLE_TASK_STACK_SIZE)

#define UART_BUF_SIZE (1024)

static const char *TAG  = "Energy Gateway UART";

void start_uart_echo(void *uartData)
{
    ESP_LOGI(TAG, "UART echo task started");
    ESP_LOGI(TAG, "Buffer contains: %s", (char *) uartData);
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));
    
    // uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        uart_read_bytes(ECHO_UART_PORT_NUM, (char *) uartData, (UART_BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        // If a zero is received, nothing happens.
        if (((char *) uartData)[0] != 0) {
            uint8_t receivedValue = *(uint8_t *) uartData;
            ESP_LOGI(TAG, "Buffer contains: %d", receivedValue);
            if (receivedValue > 250) {
                ESP_LOGI(TAG, "High value! Write to serial...");
                *(uint8_t *) uartData = 1;
                uart_write_bytes(ECHO_UART_PORT_NUM, (const char *) uartData, 1);
                vTaskDelay(2000 / portTICK_PERIOD_MS);
            } else if (receivedValue < 5) {
                ESP_LOGI(TAG, "Low value! Write to serial...");
                *(uint8_t *) uartData = 0;
                uart_write_bytes(ECHO_UART_PORT_NUM, (const char *) uartData, 1);
                vTaskDelay(2000 / portTICK_PERIOD_MS);
            }

            // Reset buffer.
            memset(uartData, 0, UART_BUF_SIZE);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
