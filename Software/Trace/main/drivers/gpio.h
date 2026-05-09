/*----------------------------------------------------------------
 *
 * gpio.h
 *
 * Header file for GPIO functions
 *
 *---------------------------------------------------------------*/
#ifndef _GPIO_H_
#define _GPIO_H_

/*
 * Global functions
 */
void init_gpio(void);                             // Initialize the GPIO ports

void         digital_test(void);                  // Execute the digital test
void         set_status_LED(unsigned int status); // Set the status LED
void         status_LED_timer(void);              // Timer to drive the status LED
void         timer_run_all(void);                 // Run all fo the timers at once
void         timer_cycle_oscillator(void);        // Turn the oscillator on and off
unsigned int board_version(void);                 // Read the revision jumpers

/*
 *  Global Variables
 */
extern volatile unsigned int step_count; // Number of steps before stopping

/*
 *  Port Definitions
 */

#define SPI_SDI          GPIO_NUM_0 // SPI Master In Slave Out
#define SPI_SDO          GPIO_NUM_1 // SPI Master Out Slave In
#define STATUS_LED       GPIO_NUM_2 // Status LED
#define TP1              GPIO_NUM_3 // Test point 1
#define BMI270_CS        GPIO_NUM_4 // SPI Chip Select
#define BMI270_INTERRUPT GPIO_NUM_5 // Input attached to FIFO interupt
#define BD_REV         GPIO_NUM_6 // MSBLSB of board revision
#define SWITCH_GPIO      GPIO_NUM_7 // Control switch input

#define ROM_MESSAGE GPIO_NUM_8      // GPIO used for testing boolean output
#define BOOT_GPIO   GPIO_NUM_9      // GPIO used for testing boolean output

#define SPI_SCLK GPIO_NUM_10        // SPI clock

#define TP2 GPIO_NUM_18             // Test point 2
#define TP3 GPIO_NUM_19             // Test point 3

#endif
