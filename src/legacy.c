/* WiFi Network and WebSocket Protocol demo code

   Created by: Kristijan Grozdanovski.
   Date created: 5 January 2020

   References:
        https://github.com/espressif/esp-idf/tree/30372f5a4ff2c0dfdaad95f544dc36bcdda30b75/examples/wifi/getting_started/station ,
        https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/network/esp_wifi.html ,
        https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/protocols/esp_websocket_client.html#application-example ,
        https://github.com/espressif/esp-idf/blob/30372f5a4ff2c0dfdaad95f544dc36bcdda30b75/examples/protocols/websocket/main/websocket_example.c

*/
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_websocket_client.h"
#include "esp_http_client.h"


#ifdef CONFIG_IDF_TARGET_ESP32
    #define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
    #define CHIP_NAME "ESP32-S2 Beta"
#endif

#define MAX_HTTP_RECV_BUFFER 512
static const char *TAG = "demo-app";

/* The examples use WiFi configuration that you can set via project configuration menu
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define ESP_WIFI_SSID      ""
#define ESP_WIFI_PASS      ""
#define ESP_MAXIMUM_RETRY  5

/* FreeRTOS event groups to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
static EventGroupHandle_t s_websocket_event_group;

/* The event group allows multiple bits for each event, but we only care about one event 
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;
const int WS_CONNECTED_BIT = BIT1;

static int s_retry_num = 0;

/* Prototype */
void websocket_connect(void);
void socketio_http_handshake(void);

static void wifi_event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        // Event-based function call flow should be kept in mind. 
        // We cannot attempt to connect to a WebSocket server without an Internet connection first.
        //printf("Establishing WebSocket link...\n");
        //websocket_connect();
        printf("Attempting to perform HTTP SocketIO handshake...\n");
        socketio_http_handshake();
    }
}

static void wifi_connect(void){
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             ESP_WIFI_SSID, ESP_WIFI_PASS);
}

// Resembling a callback for every event fired over the "WebSocket"
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
            xEventGroupSetBits(s_websocket_event_group, WS_CONNECTED_BIT);
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
            xEventGroupClearBits(s_websocket_event_group, WS_CONNECTED_BIT);
            break;
        case WEBSOCKET_EVENT_DATA:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
            ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
            ESP_LOGW(TAG, "Received=%.*s\r\n", data->data_len, (char*)data->data_ptr);
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
            break;
    }
}

/*  Initialize, establish a connection and register the event handler
 * Call this after IP_EVENT_STA_GOT_IP (successfull wifi connection).
 */
void websocket_connect(void){
    s_websocket_event_group = xEventGroupCreate();

    const char *data = "Message in a bottle.";

    const TickType_t xTicksToWait = 10000 / portTICK_PERIOD_MS;

    const esp_websocket_client_config_t ws_cfg = {
        .uri = "ws://echo.websocket.org",
        .port = 80
    };

    printf("Initializing WebSocket connection to %s\n", ws_cfg.uri);
    esp_websocket_client_handle_t ws_client = esp_websocket_client_init(&ws_cfg);

    // This registers an event handler function for all websocket events
    esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)ws_client);

    // If an internet connection has been established then attempt WebSocket connection
    EventBits_t wifiBits, wsBits;
    wsBits = xEventGroupWaitBits(s_websocket_event_group, WS_CONNECTED_BIT, pdTRUE, pdTRUE, xTicksToWait);

    printf("Opening the WebSocket connection...\n");
    esp_websocket_client_start(ws_client);

    if(wsBits && WS_CONNECTED_BIT){     // We can now try to send events of WebSockets
        printf("Sending demo WebSocket event data, expecting echo...\n");
        esp_websocket_client_send(ws_client, data, 21, 500 / portTICK_PERIOD_MS);

        vTaskDelay(4000 / portTICK_PERIOD_MS);

        printf("Closing the websocket connection...\n");
        esp_websocket_client_stop(ws_client);
        ESP_LOGW(TAG, "WebSocket stopped.\n");
        esp_websocket_client_destroy(ws_client);
    }
}

void socketio_http_handshake(){
    printf("Handshake begin...\n");

    const char *path = "/socket.io/";
    const char *query = "?EIO=3&transport=polling&t=M-NR8iO";

    esp_err_t err;
    esp_http_client_config_t config = {
        // .url = "http://httpbin.org/get",
        .url = "http://192.168.100.11:8070/socket.io/?EIO=3&transport=polling&t=M-NR8iO",
        .port = 8070,
        .path = path,
        .query = query,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    printf("Performing first request...\n");
    // first request
    err = esp_http_client_perform(client);
    ESP_ERROR_CHECK(err);   // terminates program if error

    ESP_LOGI(TAG, "Status = %d, content_length = %d",
           esp_http_client_get_status_code(client),
           esp_http_client_get_content_length(client));
    
    // second request
    // esp_http_client_set_url(client, "http://httpbin.org/anything");
    // esp_http_client_set_method(client, HTTP_METHOD_DELETE);
    // esp_http_client_set_header(client, "HeaderKey", "HeaderValue");

    // printf("Performing second request...\n");
    // err = esp_http_client_perform(client);
    // ESP_ERROR_CHECK(err);   // terminates program if error

    // ESP_LOGI(TAG, "Status = %d, content_length = %d",
    //        esp_http_client_get_status_code(client),
    //        esp_http_client_get_content_length(client));

    printf("Performing client cleanup...\n");
    esp_http_client_cleanup(client);
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    printf("Main process called...\n");

    printf("Connecting to wifi...\n");
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    wifi_connect();

    printf("End of main process.\n");

    // printf("Restarting in 5 seconds...\n");
    // vTaskDelay(5000 / portTICK_PERIOD_MS);
    // esp_restart();
}