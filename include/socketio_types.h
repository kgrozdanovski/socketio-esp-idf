/**
 * @file socketio_types.h
 * 
 * @author Kristijan Grozdanovski (kgrozdanovski7@gmail.com)
 * 
 * @brief SocketIO client library for ESP-IDF framework.
 * 
 * @version 1
 * 
 * @date 2022-02-28
 * 
 * @copyright Copyright (c) 2022, Kristijan Grozdanovski
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. 
 */

#ifndef __SOCKETIO_TYPES_H
#define __SOCKETIO_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include "esp_system.h"

/**
 * @brief EngineIO packet type
 * 
 */
typedef enum {
    EIO_PACKET_OPEN,
    EIO_PACKET_CLOSE,
    EIO_PACKET_PING,
    EIO_PACKET_PONG,
    EIO_PACKET_MESSAGE,
    EIO_PACKET_UPGRADE,
    EIO_PACKET_NOOP
} eio_packet_t;

/**
 * @brief SocketIO event declarations
 * 
 */
typedef enum {
    SIO_EVENT_READY = 0,                /* SocketIO Client ready */
    SIO_EVENT_CONNECTED_HTTP,           /* SocketIO Client connected over HTTP */
    SIO_EVENT_CONNECTED_WS,             /* SocketIO Client connected over WebSockets */
    SIO_EVENT_RECEIVED_MESSAGE,         /* SocketIO Client received message */
    SIO_EVENT_CONNECT_ERROR,            /* SocketIO Client failed to connect */
    SIO_EVENT_UPGRADE_TRANSPORT_ERROR,  /* SocketIO Client failed upgrade transport */
    SIO_EVENT_DISCONNECTED              /* SocketIO Client disconnected */
} sio_event_t;

/**
 * @brief Represents the state of the SocketIO client
 * 
 */
typedef enum {
    SIO_CLIENT_DISCONNECTED,
    SIO_CLIENT_CONNECTING,
    SIO_CLIENT_CONNECTED_HTTP,
    SIO_CLIENT_UPGRADING,
    SIO_CLIENT_CONNECTED_WS
} sio_client_status_t;

/**
 * @brief Represents a SocketIO packet type
 * 
 */
typedef enum {
    SIO_PACKET_CONNECT,
    SIO_PACKET_DISCONNECT,
    SIO_PACKET_EVENT,
    SIO_PACKET_ACK,
    SIO_PACKET_CONNECT_ERROR,
    SIO_PACKET_BINARY_EVENT,
    SIO_PACKET_BINARY_ACK
} sio_packet_t;

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
    uint8_t eio_version;            /* EngineIO protocol version */
    uint8_t max_connect_retries;    /* Maximum connection retry attempts */
    uint8_t retry_interval_ms;      /* Pause between retry attempts */
    uint8_t ping_interval_ms;       /* Server-configured ping interval */
    uint8_t ping_timeout_ms;        /* Server-configured ping wait-timeout */
    sio_transport_t transport;      /* Preferred SocketIO transport */
    const char *server_address;     /* SocketIO server address with port */
    char *token;                    /* Random token for cache prevention */
    char *session_id;               /* SocketIO session ID */
    char *namespace;                /* SocketIO namespace */
} sio_client_t;

/*
 * -----------------------------------------------------------------------
 *
 *      Prototypes for functions concerning type-related operations.
 *
 * -----------------------------------------------------------------------
 */

/**
 * @brief Get the packet type as a string for simpler comparisons.
 * 
 * @param eio_packet_type 
 * @return char* 
 */
char* socketio_get_eio_packet_type(eio_packet_t eio_packet_type);

/**
 * @brief Get the SocketIO transport type string value.
 * 
 * @param transport 
 * @return char* 
 */
char* socketio_get_sio_transport(sio_transport_t transport);

#ifdef __cplusplus
}
#endif

#endif