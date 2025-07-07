file(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)
idf_component_register(SRCS ${app_sources} 
                    INCLUDE_DIRS "."
                    "." 
                    "./tcpip" 
                    "./drivers"
                    "./html"
                    "C:/Users/allan/esp/esp-idf/esp-idf/components/freertos/FreeRTOS-Kernel/include/freertos"
                    "C:/Users/allan/esp/esp-idf/esp-idf/components/hal/include/hal"
                    "C:/Users/allan/esp/esp-idf/esp-idf/components/esp_adc/include/esp_adc"
                    "C:/Users/allan/esp/esp-idf/esp-idf/components/esp_http_server/include"
                    
                    EMBED_FILES 
                    "html/FreeEtarget.html"
                    "html/FreeEtarget.png"
                    "html/help.html"
                    "html/menu.html"
                    "tcpip/ca_cert.pem"
                    )

