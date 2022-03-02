/**
 * @file socketio.c
 * 
 * @author Kristijan Grozdanovski (kgrozdanovski7@gmail.com)
 * 
 * @brief SocketIO client library for ESP-IDF framework.
 * 
 * @version 1
 * 
 * @date 2022-01-10
 * 
 * @copyright Copyright (c) 2022, Kristijan Grozdanovski
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. 
 */

#include "socketio.h"
#include "socketio_types.h"

esp_err_t _http_event_handler(esp_http_client_event_t *evt);

ESP_EVENT_DEFINE_BASE(SIO_EVENT);

void socketio_client_init(sio_client_t *sio_client, const char *server_address)
{
    sio_client->server_address = server_address;
    sio_client->max_connect_retries = SIO_DEFAULT_MAX_CONN_RETRIES;
    sio_client->retry_interval_ms = SIO_DEFAULT_RETRY_INTERVAL_MS;
    sio_client->eio_version = EIO_VERSION;
    sio_client->transport = SIO_TRANSPORT_POLLING;
    sio_client->namespace = SIO_DEFAULT_NAMESPACE;
    sio_client->token = util_mkrndstr(SIO_TOKEN_SIZE);
}

esp_err_t socketio_begin(sio_client_t *sio_client, uint8_t connect_retries)
{
    esp_err_t handshake;

    connect_retries = connect_retries ?: 0;

    // Protocol relies on primarily establishing a HTTP handshake
    handshake = socketio_http_handshake(sio_client);
    if (handshake != ESP_OK) {
        if (connect_retries < sio_client->max_connect_retries) {
            connect_retries++;
            vTaskDelay(pdMS_TO_TICKS(sio_client->retry_interval_ms));

            return socketio_begin(sio_client, connect_retries);
        }

        return handshake;
    }

    // If handshake was successful and upgrade to WS is possible (HTTP 101)
    return ESP_OK;
}

esp_err_t socketio_http_handshake(sio_client_t* sio_client)
{
    char response_buffer[MAX_HTTP_RECV_BUFFER] = {0};

    // Post event 
    esp_event_post(SIO_EVENT, SIO_EVENT_READY, sio_client->token, SIO_TOKEN_SIZE, pdMS_TO_TICKS(50));

    // Form the request URL
    char *url = malloc(SIO_MAX_URL_LENGTH);
    sprintf(
        url,
        "%s%s/?EIO=%d&transport=%s&t=%s",
        sio_client->server_address,
        SIO_DEFAULT_URL_PATH,
        EIO_VERSION,
        SIO_TRANSPORT_POLLING_STRING,
        sio_client->token
    );

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
        .user_data = response_buffer,        // Pass address of local buffer to get response
        .disable_auto_redirect = true
    };
    esp_http_client_handle_t http_client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(http_client);
    if (err != ESP_OK) {
        ESP_LOGE(SIO_TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        return err;
    }

    int http_response_status_code = esp_http_client_get_status_code(http_client);
    int http_response_content_length = esp_http_client_get_content_length(http_client);
    ESP_LOGI(
        SIO_TAG, "HTTP GET Status = %d, content_length = %d",
        http_response_status_code,
        http_response_content_length
    );

    if (http_response_status_code != 200) {
        ESP_LOGW(SIO_TAG, "Initial HTTP request failed with status code %d", http_response_status_code);
        return ESP_FAIL;
    }
    if (http_response_content_length <= 0) {
        ESP_LOGW(SIO_TAG, "Initial HTTP request failed: No content returned.");
        return ESP_FAIL;
    }
    size_t http_response_content_length_u = (uint) http_response_content_length;

    /*
     * We are expecting an EngineIO encoded response body 
     * 
     */
    util_extract_json(&response_buffer);
    ESP_LOGD(SIO_TAG, "Cleaned response buffer: %s", response_buffer);
    
    char eio_packet_type, eio_response_object[http_response_content_length_u - 1];

    // The first byte represents the EIO packet type
    eio_packet_type = response_buffer[0];
    int eio_packet_type_num = eio_packet_type - '0';    // Convert ASCII index to digit

    // The packet type is superseeded by a JSON object
    util_substr(
        &eio_response_object,
        &response_buffer,
        &http_response_content_length_u,
        1,
        http_response_content_length
    );

    ESP_LOGD(SIO_TAG, "Server responded with EIO packet type %d", eio_packet_type_num);
    ESP_LOGD(SIO_TAG, "Server responded with EIO body %s", eio_response_object);

    // Packet type should be opening packet
    if (eio_packet_type_num != EIO_PACKET_OPEN) {
        ESP_LOGW(SIO_TAG, "SocketIO server returned inapropriate packet type: %d", eio_packet_type_num);
        return ESP_FAIL;
    }

    ESP_LOGD(SIO_TAG, "Parsing JSON response...\n");

    cJSON *body = cJSON_Parse(&eio_response_object);
    char *sid = cJSON_GetObjectItem(body, "sid")->valuestring;

    ESP_LOGD(SIO_TAG, "Server responded with SID: %s", sid);
    sio_client->session_id = sid;

    sio_client->ping_interval_ms = cJSON_GetObjectItem(body, "pingInterval")->valueint;
    sio_client->ping_timeout_ms = cJSON_GetObjectItem(body, "pingTimeout")->valueint;
// todo: remember to use cJson_Delete()
    /*
     * The next step is to perform a Namespace Connection request 
     * 
     */

    // The new URL must contain the previously received SID
    char *nc_request_url = malloc(SIO_MAX_URL_LENGTH);
    sprintf(
        nc_request_url,
        "%s%s/?EIO=%d&transport=%s&t=%s&sid=%s",
        sio_client->server_address,
        SIO_DEFAULT_URL_PATH,
        EIO_VERSION,
        SIO_TRANSPORT_POLLING_STRING,
        sio_client->token,
        sid
    );

    ESP_LOGV(SIO_TAG, "nc_request_url: %s", nc_request_url);

    uint8_t post_data_numeric = EIO_PACKET_MESSAGE * 10 + SIO_PACKET_CONNECT;
    char* post_data = malloc(3);
    snprintf(post_data, 3, "%d", post_data_numeric);

    esp_http_client_set_url(http_client, nc_request_url); 
    esp_http_client_set_method(http_client, HTTP_METHOD_POST);
    esp_http_client_set_header(http_client, "Content-Type", "text/plain");
    esp_http_client_set_header(http_client, "Accept", "text/html");
    esp_http_client_set_post_field(http_client, post_data, 2);

    ESP_LOGI(SIO_TAG, "Performing SocketIO Namespace Connection Request...");

    err = esp_http_client_perform(http_client);
    if (err == ESP_OK) {
        esp_event_post(SIO_EVENT, SIO_EVENT_CONNECTED_HTTP, sid, SIO_SID_SIZE, pdMS_TO_TICKS(50));

        // Begin long-polling
        xTaskCreate(&socketio_polling, "socketio_polling",  4096, (void *) sio_client, 6, NULL);

        ESP_LOGI(SIO_TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(http_client),
                esp_http_client_get_content_length(http_client));
    } else {
        esp_event_post(SIO_EVENT, SIO_EVENT_CONNECT_ERROR, sid, SIO_SID_SIZE, pdMS_TO_TICKS(50));

        ESP_LOGE(SIO_TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_close(http_client);
    esp_http_client_cleanup(http_client);

   // util_extract_json(&response_buffer);
    // ESP_LOGI(SIO_TAG, "Cleaned response buffer: %s", response_buffer);

    // esp_websocket_client_config_t config = {
    //     .uri = sio_client->server_address
    // };
    // esp_websocket_client_handle_t client = esp_websocket_client_init(&config);

    // bool conn = esp_websocket_client_is_connected(client);
    // ESP_LOGI(SIO_TAG, "ESP Websocket client connected: %d", conn);    

    // err = esp_http_client_close(client);
    // if (err != ESP_OK) {
    //     ESP_LOGE(SIO_TAG, "Failed to close HTTP client: %s", esp_err_to_name(err));
    // }

    // esp_event_post(SIO_EVENT, SIO_EVENT_DISCONNECTED, sid, SIO_SID_SIZE, pdMS_TO_TICKS(50));

    // ESP_LOGV(SIO_TAG, "Performing client cleanup...\n");
    // return esp_http_client_cleanup(client);

    return ESP_OK;
}

void socketio_polling(void* pvParameters)
{
    sio_client_t* sio_client = (sio_client_t *) pvParameters;
    char response_buffer[MAX_HTTP_RECV_BUFFER] = {0};

    // Form the request URL
    char *url = malloc(SIO_MAX_URL_LENGTH);
    sprintf(
        url,
        "%s%s/?EIO=%d&transport=%s&t=%s&sid=%s",
        sio_client->server_address,
        SIO_DEFAULT_URL_PATH,
        EIO_VERSION,
        SIO_TRANSPORT_POLLING_STRING, 
        sio_client->token,
        sio_client->session_id
    );

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
        .user_data = response_buffer,
        .disable_auto_redirect = true
    };
    esp_http_client_handle_t http_client = esp_http_client_init(&config);

    while (true) {
        esp_err_t err = esp_http_client_perform(http_client);
        if (err != ESP_OK) {
            // todo: emit DISCONNECTED event on any fail
            ESP_LOGE(SIO_TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
            vTaskDelete(NULL);
        }

        int http_response_status_code = esp_http_client_get_status_code(http_client);
        int http_response_content_length = esp_http_client_get_content_length(http_client);
        ESP_LOGI(
            SIO_TAG, "HTTP GET Status = %d, content_length = %d",
            http_response_status_code,
            http_response_content_length
        );

        if (http_response_status_code != 200) {
            ESP_LOGW(SIO_TAG, "Polling HTTP request failed with status code %d", http_response_status_code);
            vTaskDelete(NULL);
        }
        if (http_response_content_length <= 0) {
            ESP_LOGW(SIO_TAG, "Polling HTTP request failed: No content returned.");
            vTaskDelete(NULL);
        }
        size_t http_response_content_length_u = (uint) http_response_content_length;

        socketio_parse_message_queue(sio_client, &response_buffer);

        // Clear the response buffer
        memset(response_buffer, 0, MAX_HTTP_RECV_BUFFER);

        vTaskDelay(pdMS_TO_TICKS(sio_client->ping_interval_ms));
    }

    // /*
    //  * We are expecting an EngineIO encoded response body 
    //  * 
    //  */
    // util_extract_json(&response_buffer);
    // ESP_LOGD(SIO_TAG, "Cleaned response buffer: %s", response_buffer);

	vTaskDelete(NULL);
}

esp_err_t socketio_parse_message_queue(sio_client_t* socketio_client, char* content)
{
    // Extract the first token
    char* token = strtok(content, ASCII_RS);

    // loop through the string to extract all other tokens
    while(token != NULL) {
        ESP_LOGI(SIO_TAG, "%s\n", token);

        if (strcmp(token, socketio_get_eio_packet_type(EIO_PACKET_PING)) == 0) {
            // Server sent a PING packet, expects PONG
            ESP_LOGI(SIO_TAG, "Received PING packet from server.");
            socketio_send_pong(socketio_client);
        } else if (strcmp(token, socketio_get_eio_packet_type(EIO_PACKET_CLOSE)) == 0) {
            // Server sent a CLOSE packet
            ESP_LOGI(SIO_TAG, "Connection to SocketIO server closed.");
            esp_event_post(SIO_EVENT, SIO_EVENT_DISCONNECTED, socketio_client->session_id, SIO_SID_SIZE, pdMS_TO_TICKS(50));
        } else {
            ESP_LOGI(SIO_TAG, "token: %s", token);
        }

        token = strtok(NULL, ASCII_RS);
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    return ESP_OK;
}

esp_err_t socketio_send_pong(sio_client_t* socketio_client)
{
    if (socketio_client->transport == SIO_TRANSPORT_POLLING) {
        ESP_LOGI(SIO_TAG, "Responding with PONG packet...");
        return socketio_send_pong_http(socketio_client);
    }
    else if (socketio_client->transport == SIO_TRANSPORT_WEBSOCKETS) {
        // todo...
    }

    return ESP_OK;
}

esp_err_t socketio_send_pong_http(sio_client_t* socketio_client)
{
    esp_err_t err;

    // The new URL must contain the previously received SID
    char *url = malloc(SIO_MAX_URL_LENGTH);
    sprintf(
        url,
        "%s%s/?EIO=%d&transport=%s&t=%s&sid=%s",
        socketio_client->server_address,
        SIO_DEFAULT_URL_PATH,
        EIO_VERSION,
        SIO_TRANSPORT_POLLING_STRING,
        socketio_client->token,
        socketio_client->session_id
    );

    char response_buffer[MAX_HTTP_RECV_BUFFER] = {0};
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
        .user_data = response_buffer,        // Pass address of local buffer to get response
        .disable_auto_redirect = true
    };
    esp_http_client_handle_t http_client = esp_http_client_init(&config);

    char* post_data = malloc(2);    // always allocate +1 for the '\0' char
    sprintf(post_data, "%d", EIO_PACKET_PONG);

    esp_http_client_set_method(http_client, HTTP_METHOD_POST);
    esp_http_client_set_header(http_client, "Content-Type", "text/plain");
    esp_http_client_set_header(http_client, "Accept", "text/html");
    esp_http_client_set_post_field(http_client, post_data, 2);

    err = esp_http_client_perform(http_client);
    if (err == ESP_OK) {
        // Reset the SocketIO token to avoid caching
        socketio_client->token = util_mkrndstr(SIO_TOKEN_SIZE);

        ESP_LOGI(SIO_TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(http_client),
                esp_http_client_get_content_length(http_client));
    } else {
        esp_event_post(SIO_EVENT, SIO_EVENT_DISCONNECTED, socketio_client->session_id, SIO_SID_SIZE, pdMS_TO_TICKS(50));

        ESP_LOGE(SIO_TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        return err;
    }

    esp_http_client_close(http_client);
    esp_http_client_cleanup(http_client);

    return ESP_OK;
}

// todo: move to user-space
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(SIO_TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(SIO_TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(SIO_TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(SIO_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(SIO_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(SIO_TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(SIO_TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(SIO_TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(SIO_TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                if (output_buffer != NULL) {
                    free(output_buffer);
                    output_buffer = NULL;
                }
                output_len = 0;
                ESP_LOGI(SIO_TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(SIO_TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
        // case HTTP_EVENT_REDIRECT:
        //     ESP_LOGD(SIO_TAG, "HTTP_EVENT_REDIRECT");
        //     esp_http_client_set_header(evt->client, "From", "user@example.com");
        //     esp_http_client_set_header(evt->client, "Accept", "text/html");
        //     break;
    }
    return ESP_OK;
}