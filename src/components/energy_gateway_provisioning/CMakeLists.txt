idf_component_register(SRCS "energy_gateway_provisioning.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_event nvs_flash esp_wifi wifi_provisioning)
