idf_component_register(
    SRCS "salidas_analogicas.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_adc esp_driver_ledc
)
