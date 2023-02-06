idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES energy_gateway_ota energy_gateway_provisioning)