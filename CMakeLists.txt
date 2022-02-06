if(IDF_VERSION_MAJOR GREATER_EQUAL 4)
    idf_component_register(
        SRC_DIRS src
        INCLUDE_DIRS include
        REQUIRES nvs_flash esp_websocket_client esp_http_client json
    )
else()
    set(COMPONENT_SRCDIRS src)
    set(COMPONENT_ADD_INCLUDEDIRS include)
    set(COMPONENT_REQUIRES nvs_flash esp_websocket_client esp_http_client json)
    register_component()
endif()
