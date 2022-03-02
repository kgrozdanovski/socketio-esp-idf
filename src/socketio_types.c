#include "socketio_types.h"

char* socketio_get_eio_packet_type(eio_packet_t eio_packet_type)
{
    switch (eio_packet_type) {
        case EIO_PACKET_OPEN:
            return "0";
            break;
        case EIO_PACKET_CLOSE:
            return "1";
            break;
        case EIO_PACKET_PING:
            return "2";
            break;
        case EIO_PACKET_PONG:
            return "3";
            break;
        case EIO_PACKET_MESSAGE:
            return "4";
            break;
        case EIO_PACKET_UPGRADE:
            return "5";
            break;
        case EIO_PACKET_NOOP:
            return "6";
            break;
        default:
            return NULL;
            break;
    }
}

char* socketio_get_sio_transport(sio_transport_t transport)
{
    switch (transport) {
        case SIO_TRANSPORT_POLLING:
            return "polling";
            break;
        case SIO_TRANSPORT_WEBSOCKETS:
            return "websockets";
            break;
        default:
            return NULL;
            break;
    }
}