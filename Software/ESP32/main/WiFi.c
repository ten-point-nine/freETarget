/******************************************************************************
 * 
 * WiFi.c 
 * 
 * WiFi Driver for FreeETarget
 * 
 ******************************************************************************
 *
 * The WiFi driver supposts two modes of operation:
 *       1 - Access Point (AP) where the target provides the SSID
 *       2 - Station Mode (STA) where the target talks to a router with an SSID
 * 
 * The two modes are differenciated when the SSID stored in the configuration 
 * is defined (STA) or empty (AP).
 *
 * See: 
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/network/esp_wifi.html
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_netif.html
 * https://medium.com/@fatehsali517/how-to-connect-esp32-to-wifi-using-esp-idf-iot-development-framework-d798dc89f0d6
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/lwip.html
 *
 * *****************************************************************************/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "freETarget.h"
#include "compute_hit.h"
#include "serial_io.h"
#include "json.h"
#include "diag_tools.h"
#include "WiFi.h"
#include "nonvol.h"

#define DEFAULT_IP          192,168,10,9
#define PORT                        1090
#define KEEPALIVE_IDLE              true
#define KEEPALIVE_INTERVAL          100
#define KEEPALIVE_COUNT             50
#define MAX_SOCKETS                 4     // Allow for four sockets
#define AVAILABLE_SOCKET            -1    // The socket is unused  
#define GREETING            "CONNECTED"   // Message to send on connection

/*
 * Macros
 */
#define WIFI_CONNECTED_BIT BIT0           // we are connected to the AP with an IP
#define WIFI_FAIL_BIT      BIT1           // we failed to connect after the maximum amount of retries */
#define WIFI_MAX_RETRY     3              // Try 3x
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN

/*
 * Variables
 */
static wifi_config_t        WiFi_config;
static EventGroupHandle_t s_wifi_event_group;
static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;
static int s_retry_num = 0;
static int socket_list[MAX_SOCKETS];            // Space to remember four sockets
static esp_netif_ip_info_t ipInfo;              // IP Address of the access point
static void WiFi_start_new_connection(int sock);// Socket token to use

/*
 * Private Functions
 */
void WiFi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void tcpip_server_io(void);        // Manage TCPIP traffic

esp_err_t esp_base_mac_addr_get(uint8_t *mac);

/*****************************************************************************
 *
 * @function: WiFi_init()
 *
 * @brief:    Initialize the WiFi Interface
 * 
 * @return:   None
 *
 ******************************************************************************
 *
 * The initialization determines if the target is a station
 * or an access point (AP) that provides the SSID to connect to.
 * 
 * Once that is done the appropriate configuration is made and the target enabled.
 * 
 *******************************************************************************/
void WiFi_init(void)
{
    DLT(DLT_CRITICAL, printf("WiFi_init()\r\n");)

/* 
 * Initialize the WiFI
 */
   if ( json_wifi_ssid[0] == 0 )             // The SSID is undefined
   {
      WiFi_AP_init();
   }
   else
   {
      WiFi_station_init();
   }

/*
 *  All done
 */
   return;
}

/*****************************************************************************
 *
 * @function: WiFi_AP_init()
 *
 * @brief:    Initialize the WiFi Interface as an access point
 * 
 * @return:   None
 *
 ******************************************************************************
 *
 * This function initializes the WiFi to act as an Access Point.
 * 
 * The target broadcasts an SSID and lets clients connect to it.  For example
 * FET-TARGET
 * 
 *******************************************************************************/
void WiFi_AP_init(void)
{
    esp_netif_t* wifiAP;
    wifi_init_config_t WiFi_init_config = WIFI_INIT_CONFIG_DEFAULT();

    DLT(DLT_CRITICAL, printf("WiFi_AP_init()\r\n");)
    
/*
 * Create the network interface
 */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

/*
 * Setup the WiFi IP address before staring
 */   
    wifiAP = esp_netif_create_default_wifi_ap();
    IP4_ADDR(&ipInfo.ip, 192,168,10,9);                 // Setup the base IP address
	IP4_ADDR(&ipInfo.gw, 192,168,10,9);                 // Setup the gateway (not used but needed)
	IP4_ADDR(&ipInfo.netmask, 255,255,255,0);           // Setup the subnet mask
	esp_netif_dhcps_stop(wifiAP);                       // Remove the old value
	esp_netif_set_ip_info(wifiAP, &ipInfo);             // Put in the one
	esp_netif_dhcps_start(wifiAP);                      // and start the DHCP

/*
 *  Initialize the WiFi Access Point
 */
    esp_wifi_init(&WiFi_init_config);                   

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi_event_handler, NULL, NULL);

    sprintf((char*)&WiFi_config.ap.ssid, "FET-%s", names[json_name_id]);   // SSID Name ->FET-name
    WiFi_config.ap.ssid_len = strlen(json_wifi_ssid);
    WiFi_config.ap.channel  = json_wifi_channel;
    strcpy((char*)&WiFi_config.ap.password, json_wifi_pwd);
    WiFi_config.ap.max_connection = 4;
    if ( json_wifi_pwd[0] == 0 )
    {
        WiFi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    else
    {
        WiFi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }
    WiFi_config.ap.ssid_hidden = json_wifi_hidden;
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &WiFi_config);
    esp_wifi_start();

/*
 * Ready to go
 */
   set_status_LED(LED_ACCESS);                 // I am an access point
   return;
}


/*****************************************************************************
 *
 * @function: WiFi_station_init()
 *
 * @brief:    Initialize the WiFi Interface as a station
 * 
 * @return:   None
 *
 ******************************************************************************
 *
 * This function initializes the WiFi to act as an station
 * 
 * The target connects to an SSID and lets clients connect to it. For example
 * SSID = myHomeInternet
 * 
 * See: https://github.com/espressif/esp-idf/blob/v4.3/examples/wifi/getting_started/station/main/station_example_main.c
 * 
 *******************************************************************************/
void WiFi_station_init(void)
{
   char str_c[256];

   wifi_init_config_t   WiFi_init_config = WIFI_INIT_CONFIG_DEFAULT();

   DLT(DLT_CRITICAL, printf("WiFi_station_init()");)

   s_wifi_event_group = xEventGroupCreate();
   esp_netif_init();

   esp_event_loop_create_default();
   esp_netif_create_default_wifi_sta();

   esp_wifi_init(&WiFi_init_config);           // Initialize the configuration
   esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi_event_handler, NULL, &instance_any_id);
   esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFi_event_handler, NULL, &instance_got_ip);

   DLT(DLT_CRITICAL, printf("WiFI SSID:%s", json_wifi_ssid);)
   strcpy((char*)&WiFi_config.sta.ssid, json_wifi_ssid);
   DLT(DLT_CRITICAL, printf("WiFI password:%s", json_wifi_pwd);)
   strcpy((char*)&WiFi_config.sta.password, json_wifi_pwd);
   if ( json_wifi_pwd[0] == 0)
   {
       WiFi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
   }
   else
   {
       WiFi_config.sta.threshold.authmode = WIFI_AUTH_WEP;
   }
   WiFi_config.sta.pmf_cfg.capable = true;
   WiFi_config.sta.pmf_cfg.required = false;
   esp_wifi_set_mode(WIFI_MODE_STA);
   esp_wifi_set_config(WIFI_IF_STA, &WiFi_config);
   esp_wifi_start();                      // Start the WiFi

/*
 * Wait here for an event to occur
 */
   EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
/*
 *  The target has connected to an access point
 */
    DLT(DLT_CRITICAL, 
    {
        if (bits & WIFI_CONNECTED_BIT)
        {
            WiFi_my_IP_address(str_c);
            printf( "Connected to ap SSID: %s\r\nWiFi_IP_ADDRESS: \"%s\",", json_wifi_ssid, str_c);
        } 
        else if (bits & WIFI_FAIL_BIT) 
            {
                printf("Failed to connect to SSID:%s, password:%s",
                     json_wifi_ssid, json_wifi_pwd);
            }
            else
            {
                printf("UNEXPECTED EVENT");
            }
    }
    )
/*
 *  All done
 */
   set_status_LED(LED_STATION);
   return;
}

/*****************************************************************************
 *
 * @function:WiFi_event_handler
 *
 * @brief:   Manage events coming from the FreeRTOS event handler
 * 
 * @return:   None
 *
 ******************************************************************************
 *
 * The initialization determines if the target is a station
 * or an access point (AP) that provides the SSID to connect to.
 * 
 * Once that is done the appropriate configuration is made and the target enabled.
 * 
 * IMPORTANT
 * 
 * This function only relates to the WiFi connecting to the SSID.
 * 
 *******************************************************************************/
void WiFi_event_handler
(
   void* arg, 
   esp_event_base_t event_base,
   int32_t event_id, 
   void* event_data
)
{
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

/*
 * I am a station
 */
   if ( event_base == WIFI_EVENT )
   {
      if ( event_id == WIFI_EVENT_STA_START)                // Begin a connection to the SSID
      {
         esp_wifi_connect();
      }

      if ( event_id == WIFI_EVENT_STA_DISCONNECTED )        // End a connection to the SSID
      {
         if (s_retry_num < WIFI_MAX_RETRY)
         {
            esp_wifi_connect();
            s_retry_num++;
         }
         else
         {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
         }
         set_status_LED(LED_STATION);
        }
    }   
   
   if ( event_base == IP_EVENT )
   {
        if ( event_id == IP_EVENT_STA_GOT_IP )
        {
            ipInfo.ip = event->ip_info.ip;
            DLT(DLT_INFO, printf(" Received IP:" IPSTR, IP2STR(&event->ip_info.ip));)
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
/*
 * I am an access point
 */
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
      DLT(DLT_CRITICAL, printf("AP connected");)
    } 
   
   if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
   {
      DLT(DLT_CRITICAL, printf("AP disconnected");)
   }

/*
 * All done, return
 */
   return;
}


/*****************************************************************************
 *
 * @function: WiFi_tcp_server_task()
 *
 * @brief: Synchorous task to manage the TCPIP Stack
 * 
 * @return: Never
 *
 ******************************************************************************
 *
 * Synchronous task called from freeRTOS to interrogate the TCPIP stack and 
 * accept calls from clients.
 * 
 * Once a socket has been connected, the input and outut queues are managed
 * to send and receive data
 * 
 *******************************************************************************/

void WiFi_tcp_server_task(void *pvParameters)
{
   DLT(DLT_CRITICAL, printf("WiFi_tcp_server_task()");)

/*
 *  Move data in and out of the TCP queues
 */
    while (1)
    {
        tcpip_server_io();
/*
 *  Time out till the next time
 */
        vTaskDelay(ONE_SECOND/2);
    }
}

/*****************************************************************************
 *
 * @function: tcpip_server_io()
 *
 * @brief: Transmit data in and out of the target
 * 
 * @return: None
 *
 ******************************************************************************
 *
 * If there is any data in the tcpip_queue_2_socket queue, this function 
 * extracts the data and then loops through the active sockets to put the data
 * out to the client.
 * 
 * When trying to send to a previously active socket which is now closed, the
 * send() function will return a -1 to indicate that no information was sent.
 * This is the signal that the connection has been dropped.  At the end of the
 * loop, if all of the sockets have been closed the connection indication is
 * updated.
 * 
 *******************************************************************************/
static void tcpip_server_io(void)
{
    int  length;
    char rx_buffer[128];
    int  to_send;
    int  i;
    int  buffer_offset;
    bool new_socket_closed;

    new_socket_closed = false;              // Was a socket closed this cycle?
/*
 * Out to TCPIP
 */      
    to_send = tcpip_queue_2_socket(rx_buffer,  sizeof(rx_buffer));
    if ( to_send > 0 )
    {
        for (i=0; i != MAX_SOCKETS; i++)
        {
            if (socket_list[i] > 0 )
            {      
                buffer_offset = 0;
                while (  buffer_offset < to_send)
                {
                    length = send(socket_list[i], rx_buffer + buffer_offset, to_send-buffer_offset, 0);
                    if ( length <= 0 )
                    {
                        close(socket_list[i]);
                        socket_list[i] = AVAILABLE_SOCKET;
                        new_socket_closed = true;
                        break;
                    }
                    buffer_offset += length;
                }
            }
        }
    }
/*
 *  See if all of the sockets are closed
 */
    if ( new_socket_closed )
    {
        for (i=0; i != MAX_SOCKETS; i++)
        {
            if (socket_list[i] > 0 )
            {
                break;
            }
        }

        if ( i == MAX_SOCKETS )                 // All of them are closed?
        {
            if ( json_wifi_ssid[0] != 0 )       //  I'm a station
            {
                set_status_LED(LED_STATION);
            }
            else                                // I'm an access point
            {
                set_status_LED(LED_ACCESS);
            }       
        }
    }
/*
 *  All done
 */
    return;
}

/*****************************************************************************
 *
 * @function: tcpip_socket_poll()  0-3
 *
 * @brief:    Tasks to poll the sockets
 * 
 * @return:   None
 *
 ******************************************************************************
 *
 * There are four identical functions that wait for a TCPIP message to arrive.
 * On receipt of the message, the bytes are copied from TCPIP into a circular
 * queue that holds everything until needed.
 * 
 * IMPORTANT
 * 
 * The recv function is a blocking call that waits for something to 
 * appear in the TCPIP channel. Meaning that these functions will be suspended
 * indefinitly until something arrives and then be woken up until the next
 * recv() call is made.  For this reason, the functions are separated out, one
 * for each possible socket.
 *
 *******************************************************************************/
void tcpip_socket_poll_0(void* parameters)
{
    int length;
    char rx_buffer[256];

    DLT(DLT_CRITICAL, printf("tcp_socket_poll_0()");)

    while (1)
    {
        if (socket_list[0] > 0 )
        {
            length = recv(socket_list[0], rx_buffer, sizeof(rx_buffer), 0 );
            if ( length > 0 )
            {
                tcpip_socket_2_queue(rx_buffer, length);
            }
        }
        vTaskDelay(10);
    }
}

void tcpip_socket_poll_1(void* parameters)
{
    int length;
    char rx_buffer[256];

    DLT(DLT_CRITICAL, printf("tcp_socket_poll_1()");)

    while (1)
    {
        if (socket_list[1] > 0 )
        {
            length = recv(socket_list[1], rx_buffer, sizeof(rx_buffer), 0 );
            if ( length > 0 )
            {
                tcpip_socket_2_queue(rx_buffer, length);
            }
        }
        vTaskDelay(10);
    }
}

void tcpip_socket_poll_2(void* parameters)
{
    int length;
    char rx_buffer[256];

    DLT(DLT_CRITICAL, printf("tcp_socket_poll_2()");)

    while (1)
    {
        if (socket_list[2] > 0 )
        {
            length = recv(socket_list[2], rx_buffer, sizeof(rx_buffer), 0 );
            if ( length > 0 )
            {
                tcpip_socket_2_queue(rx_buffer, length);
            }
        }
        vTaskDelay(10);
    }
}

void tcpip_socket_poll_3(void* parameters)
{
    int length;
    char rx_buffer[256];

    DLT(DLT_CRITICAL, printf("tcp_socket_poll_3()");)

    while (1)
    {
        if (socket_list[3] > 0 )
        {
            length = recv(socket_list[3], rx_buffer, sizeof(rx_buffer), 0 );
            if ( length > 0 )
            {
                tcpip_socket_2_queue(rx_buffer, length);
            }
        }
        vTaskDelay(10);
    }
}

/*****************************************************************************
 *
 * @function: tcpip_accept_poll()
 *
 * @brief:    Tasks to poll waiting for an incoming connection
 * 
 * @return:   None
 *
 ******************************************************************************
 *
 * Once the WiFi has been set up, the target waits here for an incoming
 * connection.
 * 
 * Once the connection has been made, the function determines the socket address
 * and then looks for an empty entry in the socket list. The new socket is now
 * added to the socket list and polled via the functions above.
 *
 *******************************************************************************/
void tcpip_accept_poll(void* parameters)
{
   char addr_str[128];
   int ip_protocol = 0;
   int keepAlive = 1;
   int keepIdle = KEEPALIVE_IDLE;
   int keepInterval = KEEPALIVE_INTERVAL;
   int keepCount = KEEPALIVE_COUNT;
   struct sockaddr_storage dest_addr;
   int listen_sock;
   int option = 1;
   struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
   socklen_t addr_len = sizeof(source_addr);
   int sock;
   int i;

   DLT(DLT_CRITICAL, printf("tcp_accept_poll()");)
   
/*
 * Start the server
 */
   for (i=0; i != MAX_SOCKETS; i++)
   {
      socket_list[i] = AVAILABLE_SOCKET;
   }

   struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
   dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
   dest_addr_ip4->sin_family = AF_INET;
   dest_addr_ip4->sin_port = htons(PORT);
   ip_protocol = IPPROTO_IP;

   listen_sock = socket(AF_INET, SOCK_STREAM, ip_protocol);
   if (listen_sock < 0) 
   {
      DLT(DLT_CRITICAL, printf("Unable to create socket: errno %d\r\n", errno);)
      vTaskDelete(NULL);
      return;
   }

   option = 1;
   setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
   bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
   listen(listen_sock, 1);

    while (1)
    {
        sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock > 0)
        {
            for (i= 0; i != MAX_SOCKETS; i++ )
            {
                if  (socket_list[i] == AVAILABLE_SOCKET )
                {
                    socket_list[i] = sock;
                    WiFi_start_new_connection(sock);
                    break;
                }
            }

/*
 * Set tcp keepalive option
 */
            setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

            DLT(DLT_INFO, 
            {         
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                printf("Socket accepted ip address: %s\r\n", addr_str);
            }
            )
            set_status_LED(LED_STATION_CN);
        }
    }

/*
 *  Never get here
 */
    return;
}

/*****************************************************************************
 *
 * @function: WiFi_start_new_connection
 *
 * @brief:    Prepare a new connection
 * 
 * @return:   Nothing
 *
 ******************************************************************************
 *
 * A new socket connection has been made.
 * 
 * This function checks to see if it is the first connection, and if so
 * reset everything 
 * 
 * Once that has been done, then update the PC client with all of  the pending
 * scores.
 * 
 *******************************************************************************/
static void WiFi_start_new_connection
(
    int sock            // Socket token to use
)
{
    int i, j;

/*
 *  See if this is the first connection on a WiFi socket
 */
    if ( json_wifi_reset_first != 0 )               // Reset on first connection?
    {
        j = 0;
        for (i=0; i != MAX_SOCKETS; i++)            // How many connections do we have?
        {
            if (socket_list[i] == AVAILABLE_SOCKET)
            {
                j++;
            }
        }

        if ( j == 1)                                // This is the first, start new
        {
            start_new_session();
        }
    }

/*
 *  Inform the PC what is going on
 */
    sprintf(_xs, "{\"%s\":%10.6f}", GREETING, esp_timer_get_time()/100000.0/60.0);
    send(sock, _xs, strlen(_xs), 0);                    // Only send to the most recent connection

    for (i=0; i != SHOT_SPACE; i++)
    {
        if ( record[i].is_valid == true )
        {
            send_replay(&record[i], i);
            send(sock, _xs, strlen(_xs), 0);
        }
    }

/*
 *  All done, return
 */
    return;
}
/*****************************************************************************
 *
 * @function: WiFi_loopback_test
 *
 * @brief:    Echo the input to the output
 * 
 * @return:   Never
 *
 ******************************************************************************
 *
 * A waiting task is started.
 * 
 * The waiting task copies the input to the output of the synchronous IO 
 * 
 *******************************************************************************/
void WiFi_loopback_task(void* parameters);

void WiFi_loopback_test(void)
{
    printf("WiFi_loopback\r\n");
    xTaskCreate(WiFi_loopback_task,    "WiFi_loopback_task",          4096, NULL, 5, NULL);
    return;
}

void WiFi_loopback_task(void* parameters)
{
    int length;
    char buffer[1024];
    int  i;

    tcpip_app_2_queue("Hello", 5);

    while (1)
    {
        length = tcpip_queue_2_app(buffer, sizeof(buffer));
        if ( length != 0 )
        {
            for (i=0; i != length; i++)
            {
                buffer[i]++;                        // Add 1 to the input
            }
            tcpip_app_2_queue(buffer, length);
        }
    vTaskDelay(ONE_SECOND);
    }
/*
 *  Never get here
 */
}

/*****************************************************************************
 *
 * @function: WiFi_my_IP_address()
 *
 * @brief:    Return the IP address as a string
 * 
 * @return:   None
 *
 ****************************************************************************/
#define TO_IP(x) ((int)x) & 0xff, ((int)x >> 8) & 0xff, ((int)x >> 16) & 0xff, ((int)x >> 24) & 0xff
void WiFi_my_IP_address
(
    char* s             // Where to return the string
)
{
    sprintf(s, "%d.%d.%d.%d", TO_IP(ipInfo.ip.addr));
    return;
}


/*****************************************************************************
 *
 * @function: WiFi_MAC_address()
 *
 * @brief:    Return the MAC address as an array of bytes
 * 
 * @return:   None
 *
 ****************************************************************************/
void WiFi_MAC_address
(
    char* mac             // Where to return the string
)
{
    esp_base_mac_addr_get((uint8_t*)mac);
        return;
}

/*****************************************************************************
 *
 * @function: WiFi_config()
 *
 * @brief:    Configure the WiFi
 * 
 * @return:   None
 *
 ****************************************************************************
 *
 * This function walks the user throught the operation of the WiFi to tailor
 * it for thier needs.
 * 
 ****************************************************************************/

void WiFi_configuration(void)
{
    char ch;

    printf("\r\nWiFi Configuration");
    printf("\r\n");

/*
 *  Show the current settings
 */
    printf("\r\nWiFi Mode:");
    if ( json_wifi_ssid[0] == 0 )
    {
        printf("\r\nTarget creates it's own SSID: FET-%s",names[json_name_id] );
    }
    else
    {
        printf("\r\nTarget uses local SSID: %s", json_wifi_ssid);
        printf(" with password: %s", json_wifi_pwd);
    }

    WiFi_my_IP_address(_xs);
    printf("\r\nIP address: %s", _xs);

    printf("\r\nWiFi SSID hidden %d", json_wifi_hidden);

#if ( BUIILD_HTTP || BUILD_HTTPS || BUILD_SIMPLE)
    if ( json_remote_url[0] != 0 )
    {
        printf("\r\nTarget connects to remote server: %s", json_remote_url);
    }
#endif

/*
 * Enter the new settings
 */
    printf("\r\n! - Exit");
    printf("\r\n1 - SSID:     %s", json_wifi_ssid);
    printf("\r\n2 - password: %s", json_wifi_pwd);
    printf("\r\n3 - channel:  %d", json_wifi_channel);
    printf("\r\n4 - Hide access point SSID: %d", json_wifi_hidden);
    printf("\r\n>");

    while (1)
    {
        if ( serial_available(ALL) != 0 )
        {
            ch = serial_getch(ALL);
            printf("%c", ch); 
            switch(ch)
            {
                case '!':           // Exit
                    printf("\r\nDone\r\n");
                    return;

                case '1':           // SSID
                    printf("\r\nEnter SSID:");
                    if ( get_string(_xs, SSID_SIZE) )
                    {
                        strcpy(json_wifi_ssid, _xs);
                        nvs_set_str(my_handle, NONVOL_WIFI_SSID, json_wifi_ssid);
                    }
                    break;

                case '2':           // PWD
                    printf("\r\nEnter Password:");
                    if ( get_string(_xs, PWD_SIZE) )
                    {
                        strcpy(json_wifi_pwd, _xs);
                        nvs_set_str(my_handle, NONVOL_WIFI_PWD, json_wifi_pwd);
                    }
                    break;

                case '3':           // WiFi Channel
                    printf("\r\nWiFi channel (1-11):");
                    if ( get_string(_xs, 2) )
                    {
                        json_wifi_channel = atoi(_xs);
                        nvs_set_i32(my_handle, NONVOL_WIFI_CHANNEL, json_wifi_channel);
                    }
                    break;

                case '4':           // Hide Accesss point SSID
                    printf("\r\nWiFi hide SSID (0/1):");
                    if ( get_string(_xs, 2) )
                    {
                        json_wifi_hidden = atoi(_xs);
                        nvs_set_i32(my_handle, NONVOL_WIFI_HIDDEN, json_wifi_hidden);
                    }
                    break;

#if ( BUIILD_HTTP || BUILD_HTTPS || BUILD_SIMPLE)
                case '4':           // Enable remote URL
                    printf("\r\nEnable remote URL :");
                    if ( get_string(_xs, 2) )
                    {
                        json_remote_active = atoi(_xs);
                        nvs_set_i32(my_handle, NONVOL_REMOTE_ACTIVE, json_remote_active);
                    }
                    break;

                case '5':           // Remote Server URL
                    printf("\r\nEnter remote server URL:");
                    if ( get_string(_xs, URL_SIZE) )
                    {
                        strcpy(json_remote_url, _xs);
                        nvs_set_str(my_handle, NONVOL_REMOTE_URL, json_remote_url);
                    }
                    break;
#endif
                default:
                    break;
            }
            printf("\r\n");
        }
        vTaskDelay(ONE_SECOND/4);
    }
}