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
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "freETarget.h"
#include "serial_io.h"
#include "json.h"
#include "diag_tools.h"
#include "WiFi.h"

#define PORT                        1090
#define KEEPALIVE_IDLE              true
#define KEEPALIVE_INTERVAL          100
#define KEEPALIVE_COUNT             50
#define MAX_SOCKETS                 4     // Allow for four sockets

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
static int socket_list[MAX_SOCKETS];       // Space to remember four sockets
static esp_netif_ip_info_t ipInfo;         // IP Address of the access point

/*
 * Private Functions
 */
void WiFi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void tcpip_server_io(void);        // Manage TCPIP traffic


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
    DLT(DLT_CRITICAL);
    printf("WiFi_init()\r\n");

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
 * The target broadcasts an SSID and lets clients connect to it.
 * 
 *******************************************************************************/
void WiFi_AP_init(void)
{
    esp_netif_t* wifiAP;
    wifi_init_config_t WiFi_init_config = WIFI_INIT_CONFIG_DEFAULT();

    DLT(DLT_CRITICAL);
    printf("WiFi_AP_init()");
    
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

    strcpy((char*)&WiFi_config.ap.ssid, "FET-");
    strcat((char*)&WiFi_config.ap.ssid, names[json_name_id]);   // SSID Name ->FET-name
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

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &WiFi_config);
    esp_wifi_start();

/*
 * Ready to go
 */
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
 * The target connects to an SSID and lets clients connect to it.
 * 
 * See: https://github.com/espressif/esp-idf/blob/v4.3/examples/wifi/getting_started/station/main/station_example_main.c
 * 
 *******************************************************************************/
void WiFi_station_init(void)
{
   wifi_init_config_t   WiFi_init_config = WIFI_INIT_CONFIG_DEFAULT();

   DLT(DLT_CRITICAL);
   printf("WiFi_station_init()");

   s_wifi_event_group = xEventGroupCreate();
   esp_netif_init();

   esp_event_loop_create_default();
   esp_netif_create_default_wifi_sta();

   esp_wifi_init(&WiFi_init_config);           // Initialize the configuration
   esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi_event_handler, NULL, &instance_any_id);
   esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFi_event_handler, NULL, &instance_got_ip);

   strcpy((char*)&WiFi_config.sta.ssid, json_wifi_ssid);
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
    if ( DLT(DLT_INFO) )
    {
        if (bits & WIFI_CONNECTED_BIT)
        {
            printf( "Connected to ap SSID:%s password:%s",
                     json_wifi_ssid, json_wifi_pwd);
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
/*
 *  All done
 */
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
      if ( event_id == WIFI_EVENT_STA_START)
      {
         esp_wifi_connect();
      }

      if ( event_id == WIFI_EVENT_STA_DISCONNECTED )
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
      }
   }   
   
   if ( event_base == IP_EVENT )
   {
        if ( event_id == IP_EVENT_STA_GOT_IP )
        {
            ipInfo.ip = event->ip_info.ip;
            if ( DLT(DLT_INFO) )
            {
                printf(" Received IP:" IPSTR, IP2STR(&event->ip_info.ip));
            }
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
/*
 * I am an access point
 */
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
      printf("AP Connected");
    } 
   
   if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
   {
      printf("STATION disconnected");
   }

/*
 * All done, return
 */
   return;
}


/*****************************************************************************
 *
 * @function: tcp_server_task()
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
static char greeting[] = "{\"CONNECTED\"}";

void WiFi_tcp_server_task(void *pvParameters)
{
   DLT(DLT_CRITICAL);
   printf("WiFi_tcp_server_task()");

/*
 *  Move data in and out of the TCP queues
 */
    while (1)
    {
        tcpip_server_io();
/*
 *  Time out till the next time
 */
        vTaskDelay(ONE_SECOND/4);
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
 * Synchronous task called from freeRTOS to interrogate the TCPIP stack and 
 * accept calls from clients
 * 
 *******************************************************************************/
static void tcpip_server_io(void)
{
    int length;
    char rx_buffer[128];
    int  to_write;
    int  i;
    int  buffer_offset;

/*
 * Out to TCPIP
 */      
    to_write = tcpip_queue_2_socket(rx_buffer,  sizeof(rx_buffer));
    if ( to_write > 0 )
    {
        for (i=0; i != MAX_SOCKETS; i++)
        {
            if (socket_list[i] > 0 )
            {      
                buffer_offset = 0;
                while (  buffer_offset < to_write)
                {
                    length = send(socket_list[i], rx_buffer + buffer_offset, to_write-buffer_offset, 0);
                    buffer_offset += length;
                }
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
 * @function: tcpip_socket_poll()
 *
 * @brief:    Tasks to poll the sockets
 * 
 * @return:   None
 *
 ******************************************************************************
 *
 * The recv function is a blocking call that waits for something to 
 * appear in the TCPIP channel.  
 * 
 * These are tasks that pend waiting for something to appear and then
 * put it into the queue.
 *
 *******************************************************************************/
void tcpip_socket_poll_0(void* parameters)
{
    int length;
    char rx_buffer[256];

    DLT(DLT_CRITICAL);
    printf("tcp_socket_poll_0()");

    while (1)
    {
        if (socket_list[0] > 0 )
        {
            length = recv(socket_list[0], rx_buffer, sizeof(rx_buffer), 0 );
            rx_buffer[length] = 0;
            while ( length > 0 )
            {
                length -= tcpip_socket_2_queue(rx_buffer, length);
            }
        }
        vTaskDelay(100);
    }
}

void tcpip_socket_poll_1(void* parameters)
{
    int length;
    char rx_buffer[256];

    DLT(DLT_CRITICAL);
    printf("tcp_socket_poll_1()");

    while (1)
    {
        if (socket_list[1] > 0 )
        {
            length = recv(socket_list[1], rx_buffer, sizeof(rx_buffer), 0 );
            rx_buffer[length] = 0;
            while ( length > 0 )
            {
                length -= tcpip_socket_2_queue(rx_buffer, length);
            }
        }
        vTaskDelay(100);
    }
}

void tcpip_socket_poll_2(void* parameters)
{
    int length;
    char rx_buffer[256];

    DLT(DLT_CRITICAL);
    printf("tcp_socket_poll_2()");

    while (1)
    {
        if (socket_list[2] > 0 )
        {
            length = recv(socket_list[2], rx_buffer, sizeof(rx_buffer), 0 );
            rx_buffer[length] = 0;
            while ( length > 0 )
            {
                length -= tcpip_socket_2_queue(rx_buffer, length);
            }
        }
        vTaskDelay(100);
    }
}

void tcpip_socket_poll_3(void* parameters)
{
    int length;
    char rx_buffer[256];

    DLT(DLT_CRITICAL);
    printf("tcp_socket_poll_3()");

    while (1)
    {
        if (socket_list[3] > 0 )
        {
            length = recv(socket_list[3], rx_buffer, sizeof(rx_buffer), 0 );
            rx_buffer[length] = 0;
            while ( length > 0 )
            {
                length -= tcpip_socket_2_queue(rx_buffer, length);
            }
        }
        vTaskDelay(100);
    }
}

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

   DLT(DLT_CRITICAL);
   printf("tcp_accept_poll()\r\n");
   
/*
 * Start the server
 */
   for (i=0; i != MAX_SOCKETS; i++)
   {
      socket_list[i] = -1;
   }

   struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
   dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
   dest_addr_ip4->sin_family = AF_INET;
   dest_addr_ip4->sin_port = htons(PORT);
   ip_protocol = IPPROTO_IP;

   listen_sock = socket(AF_INET, SOCK_STREAM, ip_protocol);
   if (listen_sock < 0) 
   {
      DLT(DLT_CRITICAL);
      printf("Unable to create socket: errno %d\r\n", errno);
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
                if  (socket_list[i] == -1 )
                {
                    socket_list[i] = sock;
                    send(sock, greeting, sizeof(greeting), 0);
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

            if ( DLT(DLT_CRITICAL) )
            {         
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                printf("Socket accepted ip address: %s\r\n", addr_str);
            }
        }
    }

/*
 *  Never get here
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
void WiFi_my_ip_address
(
    char* s             // Where to return the string
)
{
    sprintf(s, "%d.%d.%d.%d", TO_IP(ipInfo.ip.addr));
    return;
}
