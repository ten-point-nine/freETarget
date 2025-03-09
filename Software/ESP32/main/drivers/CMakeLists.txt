idf_component_register(SRCS  
                    "analog_io.c"
                    "gpio.c"
                    "gpio_define.c"
                    "i2c.c"
                    "pwm.c"
                    "pcnt.c"
                    "serial_io.c"
                    "token.c"
                    "bluetooth.c"
                    "dac.c"

                    INCLUDE_DIRS 
                    "." 
                    "./tcpip" 
                    "./drivers"
                    "./html"
                    "C:/Users/allan/esp/esp-idf/esp-idf/components/freertos/FreeRTOS-Kernel/include/freertos"
                    "C:/Users/allan/esp/esp-idf/esp-idf/components/hal/include/hal"
                    "C:/Users/allan/esp/esp-idf/esp-idf/components/esp_adc/include/esp_adc"
                    "C:/Users/allan/esp/esp-idf/esp-idf/components/esp_http_server/include"
                    
                    )
