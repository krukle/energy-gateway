idf_component_register(SRCS "energy_gateway_ota.c"
                    # Embed the server root certificate into the final binary
                    EMBED_TXTFILES server_certs/ca_cert.pem
                    INCLUDE_DIRS "include"
                    REQUIRES esp_event app_update esp_http_client esp_https_ota nvs_flash esp_wifi protocol_examples_common)
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
