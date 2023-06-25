idf_component_register(
    SRC_DIRS src "src"
    INCLUDE_DIRS include "include"
    REQUIRES nvs_flash esp_websocket_client esp_http_client esp_http_server json
)