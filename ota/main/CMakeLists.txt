idf_component_register(SRCS "advanced_https_ota_example.c"
                    # Embed the server root certificate into the final binary
                    EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem)
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
