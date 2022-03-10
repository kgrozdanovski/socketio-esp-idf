![Project banner](assets/socketio-esp-idf-banner.png)

## Overview

This project aims to provide a standard interface for connecting to and coomunicating with SocketIO servers.

Features not supported include secure connections and encryption (WSS, SSL), as well as SocketIO P2P (peer-to-peer)
over WebRTC.

## How to use example

### Hardware Required

To use this driver and run the example all you need is a board featuring a ESP32 chip and 

This client and the provided example should be able to run on any commonly available ESP32 development board, however 
as of the time of writing it has only been tested on an unofficial copy of the LilyGO TTGO T-Display board.

If you have successfully tested this driver on another board please let me know and I will maintain a list of support
boards in this section.

### Configure the project

Currently the project does not rely on nor does it support any sdkconfig parameters however some configurability is planned for the future
through sdkconfig or Kconfig.

### Build and Flash

Build the project and flash it to the board as you would any other project, then run monitor tool to view the serial output:

```
idf.py -p PORT flash monitor
```

_(Replace PORT with the name of the serial port to use.)_

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/v4.3.1/esp32/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

If you have successfully built and flashed the example to your board then you should expect to see serial output simillar to this:

```

```

## Troubleshooting

Make sure that you have a working connection to the internet or to the LAN where the local host hosting the SocketIO server resides.

Make sure you are connecting to a SocketIO (non-WSS) server and not a standard WebSockets server.

Make sure the server is listening to the port 80 and that the same is not behind a firewall.

Make sure the server permits the 'origin' from which you are sending packets.

## Disclaimer

This project was started with the purpose of learning more about C, ESP-IDF, protocol implementation and reverse-engineering. It was never
intended to be fully-functional or regularly maintained therefore parts of the codebase may be sub-optimal, disorganized or poorly implemented.

Please do not use this in a production environment and let it serve only as a reference-implementation of an otherwise poorly-documented protocol.

In case there is sufficient interest for both using and contributing to this project I would be willing to help clean-up the code and organize the effort.