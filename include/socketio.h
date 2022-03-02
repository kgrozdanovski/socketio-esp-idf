/**
 * @file socketio.h
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

#ifndef __SOCKETIO_H
#define __SOCKETIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_websocket_client.h"
#include "cJSON.h"

#include "socketio_types.h"
#include "utility.h"

/*
 * ----------------------------------------------------------
 *
 *              Defines and component constants.
 *
 * ----------------------------------------------------------
 */

// todo: make these kconfig configurable
#define EIO_VERSION 4
#define SIO_DEFAULT_NAMESPACE "/"
#define SIO_DEFAULT_URL_PATH "/socket.io"
#define SIO_DEFAULT_MAX_CONN_RETRIES 3
#define SIO_DEFAULT_RETRY_INTERVAL_MS 3000

#define SIO_MAX_URL_LENGTH 512
#define SIO_TRANSPORT_POLLING_STRING "polling"
#define SIO_TRANSPORT_WEBSOCKETS_STRING "websockets"
#define MAX_HTTP_RECV_BUFFER 512
#define SIO_SID_SIZE 20
#define SIO_TOKEN_SIZE 7
#define ASCII_RS ""
#define ASCII_RS_INDEX = 30

/**
 * @brief Module tag used in log.
 * 
 */
static const char *SIO_TAG = "sio";

/**
 * @brief SocketIO event base declaration
 * 
 */
ESP_EVENT_DECLARE_BASE(SIO_EVENT);

/*
 * ----------------------------------------------------------
 *
 *              Internal function prototypes.
 *
 * ----------------------------------------------------------
 */

static esp_err_t socketio_http_handshake(sio_client_t *sio_client);
static esp_err_t socketio_compile_url(const char *url, const char *server_address);
static void socketio_polling(void* pvParameters);
static esp_err_t socketio_parse_message_queue(sio_client_t* socketio_client, char* content);
static esp_err_t socketio_send_pong(sio_client_t* socketio_client);
static esp_err_t socketio_send_pong_http(sio_client_t* socketio_client);

/*
 * ----------------------------------------------------------
 *
 *      Prototypes for functions of the public API.
 *
 * ----------------------------------------------------------
 */

/**
 * @brief Initialize the SocketIO client with default values
 * 
 * @param sio_client 
 * @param server_address 
 */
void socketio_client_init(sio_client_t *sio_client, const char *server_address);

/**
 * @brief 
 * 
 * @param sio_client 
 * @param connect_retries 
 * @return esp_err_t 
 */
esp_err_t socketio_begin(sio_client_t *sio_client, uint8_t connect_retries);

#ifdef __cplusplus
}
#endif

#endif