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

#include "utility.h"

// todo: make these kconfig configurable
#define EIO_VERSION 4
#define SIO_DEFAULT_NAMESPACE "/"
#define SIO_DEFAULT_URL_PATH "/socket.io"
#define SIO_DEFAULT_MAX_CONN_RETRIES 3
#define SIO_DEFAULT_RETRY_INTERVAL_MS 3000

#define SIO_TRANSPORT_POLLING_STRING "polling"
#define SIO_TRANSPORT_WEBSOCKETS_STRING "websockets"
#define MAX_HTTP_RECV_BUFFER 512
#define SIO_SID_SIZE 20
#define SIO_TOKEN_SIZE 7
#define ASCII_RS = 
#define ASCII_RS_INDEX = 30

/**
 * @brief Represents the state of the SocketIO client.
 * 
 */
// todo: change to typedef
const enum sio_client_status {
    SIO_CLIENT_DISCONNECTED,
    SIO_CLIENT_CONNECTING,
    SIO_CLIENT_CONNECTED_HTTP,
    SIO_CLIENT_UPGRADING,
    SIO_CLIENT_CONNECTED_WS
};
// todo: change to typedef
static const enum eio_packet_type {
    EIO_PACKET_OPEN,
    EIO_PACKET_CLOSE,
    EIO_PACKET_PING,
    EIO_PACKET_PONG,
    EIO_PACKET_MESSAGE,
    EIO_PACKET_UPGRADE,
    EIO_PACKET_NOOP
};
// todo: change to typedef
static const enum sio_packet_type {
    SIO_PACKET_CONNECT,
    SIO_PACKET_DISCONNECT,
    SIO_PACKET_EVENT,
    SIO_PACKET_ACK,
    SIO_PACKET_CONNECT_ERROR,
    SIO_PACKET_BINARY_EVENT,
    SIO_PACKET_BINARY_ACK
};

/**
 * @brief SocketIO event base declaration
 * 
 */
ESP_EVENT_DECLARE_BASE(SIO_EVENT);

/**
 * @brief SocketIO event declarations
 * 
 */
typedef enum {
    SIO_EVENT_READY = 0,                    /* SocketIO Client ready */
    SIO_EVENT_CONNECTED_HTTP,               /* SocketIO Client connected over HTTP */
    SIO_EVENT_CONNECTED_WS,                 /* SocketIO Client connected over WebSockets */
    SIO_EVENT_RECEIVED_MESSAGE,             /* SocketIO Client received message */
    SIO_EVENT_CONNECT_ERROR,                /* SocketIO Client failed to connect */
    SIO_EVENT_UPGRADE_TRANSPORT_ERROR,      /* SocketIO Client failed upgrade transport */
    SIO_EVENT_DISCONNECTED                  /* SocketIO Client disconnected */
} sio_event_t;

/**
 * @brief SocketIO available transport types
 * 
 */
typedef enum {
    SIO_TRANSPORT_POLLING,      /* polling */
    SIO_TRANSPORT_WEBSOCKETS    /* websockets */
} sio_transport_t;

/**
 * @brief SocketIO client type
 * 
 */
typedef struct {
    uint8_t eio_version;                          /* EngineIO protocol version */
    uint8_t max_connect_retries; /* Maximum connection retry attempts */
    uint8_t retry_interval_ms;  /* Pause between retry attempts */
    esp_http_client_handle_t *http_client;                      /* ESP HTTP client handle */
    sio_transport_t transport;          /* Preferred SocketIO transport */
    const char *server_address;                                 /* SocketIO server address with port */
    char *token;                                                /* Random token for cache prevention */
    char *session_id;                                           /* SocketIO session ID */
    char *namespace;                    /* SocketIO namespace */
} sio_client_t;

/**
 * @brief Module tag used in log.
 * 
 */
static const char *SIO_TAG = "sio";

static esp_err_t socketio_http_handshake(sio_client_t *sio_client);
static esp_err_t socketio_compile_url(const char *url, const char *server_address);

// static void socketio_polling(void *ignore);

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