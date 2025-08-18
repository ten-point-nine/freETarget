
/******************************************************************************
 *
 * @file: dac.h
 *
 * Common interface to the DAC
 *
 *****************************************************************************
 *
 * See: https://ww1.microchip.com/downloads/en/DeviceDoc/22187E.pdf
 *
 *****************************************************************************/

/*
 *  Definitions
 */
#define DAC_MCP4728ADDR  0x60 // DAC I2C address
#define DAC_MCP4725_ADDR 0x60 // DAC I2C address for MCP4725
// #define DAC_WRITE 0x58 // Single write
#define VREF_LO 0
#define VREF_HI 1
#define VREF_2  2
#define VREF_3  3

/*
 *  Functions
 */
void DAC_write(double volts[]);
void DAC_test(void);      // Ramp the DACs
void DAC_read(void);      // Read the DAC registers
void DAC_calibrate(void); // Adjust the DAC output