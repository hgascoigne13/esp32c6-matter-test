set(PRIV_REQUIRES_LIST device esp_matter openthread app_reset nvs_flash)

idf_component_register(SRC_DIRS          "." "i2cdev" "bh1750" "scd30"
                      PRIV_INCLUDE_DIRS  "." "esp_idf_lib_helpers" "i2cdev" "bh1750" "scd30"
                      PRIV_REQUIRES      ${PRIV_REQUIRES_LIST})

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
