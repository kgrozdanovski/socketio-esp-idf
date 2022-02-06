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

esp_err_t _http_event_handler(esp_http_client_event_t *evt);

esp_err_t socketio_connect(const char* server_url)
{
    esp_err_t handshake;

    // Protocol relies on primarily establishing a HTTP handshake
    handshake = socketio_http_handshake(server_url);
    if (handshake != ESP_OK) {
        return handshake;
    }

    // If handshake was successful and upgrade to WS is possible (HTTP 101)
    return ESP_OK;
}

esp_err_t socketio_http_handshake(const char *server_address)
{
    size_t server_address_len = strlen(server_address);
    char response_buffer[MAX_HTTP_RECV_BUFFER] = {0};

    esp_http_client_config_t config = {
        .url = server_address,
        .event_handler = _http_event_handler,
        .user_data = response_buffer,        // Pass address of local buffer to get response
        .disable_auto_redirect = true
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(SIO_TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        return err;
    }

    int http_response_status_code = esp_http_client_get_status_code(client);
    int http_response_content_length = esp_http_client_get_content_length(client);
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

    /*
     * The next step is to perform a Namespace Connection request 
     * 
     */

    // The new URL must contain the previously received SID
    char sid_query_param[SIO_SID_LENGTH + 5] = "&sid=";
    strcat(sid_query_param, sid);

    ESP_LOGV(SIO_TAG, "sid_query_param: %s", sid_query_param);

    size_t url_length = server_address_len + SIO_SID_LENGTH + 5;
    char nc_request_url[url_length];
    memset(nc_request_url, 0, url_length * sizeof(char));

    strcpy(nc_request_url, server_address);
    strcat(nc_request_url, sid_query_param);

    ESP_LOGV(SIO_TAG, "nc_request_url: %s", nc_request_url);

    uint8_t post_data_numeric = EIO_PACKET_MESSAGE * 10 + SIO_PACKET_CONNECT;
    char* post_data = malloc(3);
    snprintf(post_data, 3, "%d", post_data_numeric);

    esp_http_client_set_url(client, nc_request_url); 
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "text/plain");
    esp_http_client_set_header(client, "Accept", "text/html");
    esp_http_client_set_post_field(client, post_data, 2);

    ESP_LOGI(SIO_TAG, "Performing SocketIO Namespace Connection Request...");

    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(SIO_TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(SIO_TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
   }

    err = esp_http_client_close(client);
    if (err != ESP_OK) {
        ESP_LOGE(SIO_TAG, "Failed to close HTTP client: %s", esp_err_to_name(err));
    }

    ESP_LOGV(SIO_TAG, "Performing client cleanup...\n");
    return esp_http_client_cleanup(client);
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
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
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