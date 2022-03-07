if(IDF_VERSION_MAJOR GREATER_EQUAL 4)
    idf_component_register(
        SRC_DIRS src components
        INCLUDE_DIRS include components
        REQUIRES nvs_flash esp_websocket_client esp_http_client esp_http_server json
    )
else()
    set(COMPONENT_SRCDIRS src components)
    set(COMPONENT_ADD_INCLUDEDIRS include components)
    set(COMPONENT_REQUIRES nvs_flash esp_websocket_client esp_http_client esp_http_server json)
    register_component()
endif()