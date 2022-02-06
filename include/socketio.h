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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include "esp_websocket_client.h"
#include "esp_http_client.h"
#include "cJSON.h"

#include "utility.h"

/**
 * @attention use envs instead!
 * 
 */
#define SOCKETIO_EIO_VERSION 4
#define SOCKETIO_SERVER_PORT 3000

#define MAX_HTTP_RECV_BUFFER 512
#define SIO_SID_LENGTH 20

static const enum EIO_PACKET_TYPE {
    EIO_PACKET_OPEN,
    EIO_PACKET_CLOSE,
    EIO_PACKET_PING,
    EIO_PACKET_PONG,
    EIO_PACKET_MESSAGE,
    EIO_PACKET_UPGRADE,
    EIO_PACKET_NOOP
};

static const enum SIO_PACKET_TYPE {
    SIO_PACKET_CONNECT,
    SIO_PACKET_DISCONNECT,
    SIO_PACKET_EVENT,
    SIO_PACKET_ACK,
    SIO_PACKET_CONNECT_ERROR,
    SIO_PACKET_BINARY_EVENT,
    SIO_PACKET_BINARY_ACK
};

/**
 * @brief Module tag used in log.
 * 
 */
static const char *SIO_TAG = "sio";
static const char *socketio_url_namespace = "/socket.io/";

static esp_err_t socketio_http_handshake(const char* server_address);
static esp_err_t socketio_compile_url(const char *url, const char *server_address);

/**
 * @brief 
 * 
 * @param url 
 * @return esp_err_t 
 */
esp_err_t socketio_connect(const char* server_url);

#ifdef __cplusplus
}
#endif