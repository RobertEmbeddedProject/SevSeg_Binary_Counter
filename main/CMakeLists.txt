idf_component_register(
  SRCS
    "main.c"
  INCLUDE_DIRS
    "."
  REQUIRES
    driver log esp_netif lwip esp_event esp_driver_pcnt esp_wifi esp_timer nvs_flash bt 
)
