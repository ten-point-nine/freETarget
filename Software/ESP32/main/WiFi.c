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
#include "lwip/dns.h"
#include "lwip/ip4_addr.h"

#include "freETarget.h"
#include "serial_io.h"
#include "json.h"
#include "diag_tools.h"
#include "WiFi.h"
#include "nonvol.h"

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
static int socket_list[MAX_SOCKETS];        // Space to remember four sockets
static esp_netif_ip_info_t ipInfo;          // IP Address of the access point
static int dns_valid;                       // We have a valid IP address for the URL
static ip_addr_t url_ip_address;            // Address of the server

/*
 * Private Functions
 */
void WiFi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void tcpip_server_io(void);        // Manage TCPIP traffic
static void dns_found_cb ( const char* name, const ip_addr_t* ip_addr, void* callback_arg);
esp_err_t esp_base_mac_addr_get(uint8_t *mac);

/*
 * Definitions 
 */
#define TO_IP(x) ((int)x) & 0xff, ((int)x >> 8) & 0xff, ((int)x >> 16) & 0xff, ((int)x >> 24) & 0xff

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
 * The target broadcasts an SSID and lets clients connect to it.
 * 
 *******************************************************************************/
void WiFi_AP_init(void)
{
    esp_netif_t* wifiAP;
    wifi_init_config_t WiFi_init_config = WIFI_INIT_CONFIG_DEFAULT();

    DLT(DLT_CRITICAL, printf("WiFi_AP_init()");)
    
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
 * The target connects to an SSID and lets clients connect to it.
 * 
 * See: https://github.com/espressif/esp-idf/blob/v4.3/examples/wifi/getting_started/station/main/station_example_main.c
 * 
 *******************************************************************************/
void WiFi_station_init(void)
{
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
    DLT(DLT_INFO, 
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
    )
/*
 *  All done
 */
   set_status_LED(LED_STATION);
   return;
}

/*****************************************************************************
 *
 * @function: WiFi_get_remote_IP()
 *
 * @brief:    Find the address of the remote IP
 * 
 * @return:   TRUE if the dns has been found
 *
 ****************************************************************************
 *
 * This function calls the DNS server to obtain the IP address of a URL.
 * 
 * Example "google.com" is 142.250.190.14
 * 
 * See https://gist.github.com/MakerAsia/37d2659310484bdbba9d38558e2c3cdb
 * for programming example
 * 
 * See https://www.nongnu.org/lwip/2_0_x/group__infrastructure__errors.html
 * for LWIP errors
 * 
 ****************************************************************************/
bool WiFi_get_remote_IP
(
    char* remote_url             // Text string of the remote URL 
)
{
    int i;
/*
 * Prepare the callback for the result 
 */
    if ( dns_gethostbyname(remote_url, &url_ip_address, dns_found_cb, NULL) == 0 )
    {
        dns_valid = 1;              // IP was cached and available
    }
    else
    {
        dns_valid = 0;              // IP is not currently valid
    }

/*
 * Wait here for the DNS to come back
 */
    i = 10;
    while ( (dns_valid == 0) || ( i != 0) )
    {
        vTaskDelay(ONE_SECOND);
    }

/*
 *  Return if the DNS is valid
 */
    return dns_valid;
}

static void dns_found_cb
(
    const char* name,           // Name of dns search
    const ip_addr_t* ip_addr,   // IP address of the URL
    void* callback_arg          // Not used 
)
{
    url_ip_address = *ip_addr;
    dns_valid = true;

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
         set_status_LED(LED_STATION_CN);
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
      DLT(DLT_CRITICAL, printf("AP Connected\r\n");)
      set_status_LED(LED_ACCESS_CN);
    } 
   
   if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
   {
      DLT(DLT_CRITICAL, printf("STATION disconnected");)
      set_status_LED(LED_ACCESS);
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
static char greeting[] = "{\"CONNECTED\"}";

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

    DLT(DLT_CRITICAL, printf("tcp_socket_poll_0()");)

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

            DLT(DLT_CRITICAL, 
            {         
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                printf("Socket accepted ip address: %s\r\n", addr_str);
            }
            )
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

#if ( BUILD_HTTP || BUILD_HTTPS || BUILD_SIMPLE )
/*****************************************************************************
 *
 * @function: WiFi_DNS_test
 *
 * @brief:    Use the DNS sofware to find an IP address
 * 
 * @return:   Nothing
 *
 ******************************************************************************
 *
 * A waiting task is started.
 * 
 * The waiting task copies the input to the output of the synchronous IO 
 * 
 *******************************************************************************/
static char test_URL[] = "google.com";

void WiFi_DNS_test(void)
{
    int i;
    char str_c[16];

    DLT(DLT_CRITICAL, printf("WiFi_DNS_test()\r\n");)

/*
 * Make sure we ares setup correctly
 */
    if ( json_wifi_ssid[0] == 0 )
    {
        DLT(DLT_CRITICAL, printf("\r\nWiFi must be attached to gateway");)
        return;
    }

/*
 *  Go look for the remote address
 */
    WiFi_get_remote_IP(test_URL);

    i = 0;
    while ( (dns_valid == 0) && ( i != 10) )
    {
        printf("%d ", i);
        vTaskDelay(ONE_SECOND);
        i++;
    }

/*
 *  Got it
 */
    if ( i == 10 )
    {
        DLT(DLT_CRITICAL, printf("DNS lookup failed");)
    }
    else
    {
        WiFi_remote_IP_address(str_c);
        printf("\r\nThe IP address of %s is %s\r\n", test_URL, str_c);
    }
    
    return;
}
#endif 

/*****************************************************************************
 *
 * @function: WiFi_my_IP_address()
 *
 * @brief:    Return the IP address as a string
 * 
 * @return:   None
 *
 ****************************************************************************/
void WiFi_my_IP_address
(
    char* s             // Where to return the string
)
{
    sprintf(s, "%d.%d.%d.%d", TO_IP(ipInfo.ip.addr));
    return;
}

#if ( BUILD_HTTP || BUILD_HTTPS || BUILD_SIMPLE )
void WiFi_remote_IP_address
(
    char* s             // Where to return the string
)
{
    sprintf(s, "%d.%d.%d.%d", TO_IP(url_ip_address.u_addr.ip4.addr));
    return;
}
#endif 

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
bool get_string
(
    char  destination[],
    int   size
)
{
    int ch;             // Input character
    int i;              // Input index

    i = 0;
    destination[0] = 0;
    while (1)
    {
        if ( serial_available(ALL) != 0 )
        {
            ch = serial_getch(ALL);
            printf("%c", ch); 

            switch (ch)
            {
                case 8:                 // Backspace
                    i--;
                    if ( i < 0 )
                    {
                        i = 0;
                    }
                    destination[i] = 0;
                    break;

                case '\r':                // Enter
                case '\n':                // newline
                    return 1;

                default:
                    destination[i] = ch;
                    if ( i < size )
                    {
                        i++;
                    }
                    destination[i] = 0;
                    break;
            }
        }
        vTaskDelay(10);
    }
}
void WiFi_setup(void)
{
    char ch;
    int i;

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
    printf("\r\n1 - SSID");
    printf("\r\n2 - password");
    printf("\r\n3 - channel");
#if ( BUIILD_HTTP || BUILD_HTTPS || BUILD_SIMPLE)
    printf("\r\n4 - enable remote server");
    printf("\r\n5 - remote server URL");
#endif
    printf("\r\n:");

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
                        i=0;
                        json_wifi_channel = 0;
                        while (_xs[i] != 0)
                        {
                           json_wifi_channel *= 10;
                           json_wifi_channel += (_xs[i] - '0');
                           i++;
                        }
                        nvs_set_i32(my_handle, NONVOL_WIFI_CHANNEL, json_wifi_channel);
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