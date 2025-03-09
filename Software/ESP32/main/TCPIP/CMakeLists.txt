idf_component_register(
    SRCS  

                    "wifi.c"
                    "http_client.c"
                    "http_test.c"
                    "http_server.c"
                    "http_services.c"


                    INCLUDE_DIRS 
                    "." 
                    "./tcpip" 
                    "./drivers"
                    "./html"
                    "C:/Users/allan/esp/esp-idf/esp-idf/components/freertos/FreeRTOS-Kernel/include/freertos"
                    "C:/Users/allan/esp/esp-idf/esp-idf/components/hal/include/hal"
                    "C:/Users/allan/esp/esp-idf/esp-idf/components/esp_adc/include/esp_adc"
                    "C:/Users/allan/esp/esp-idf/esp-idf/components/esp_http_server/include"
                    
                    EMBED_FILES 
                    "html/index.html"
                    "html/issf.png"
                    "html/post_test.html"
                
                    )
