
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
#define DAC_ADDR  0x60 // DAC I2C address
#define DAC_WRITE 0x58 // Single write
#define VREF_LO   0
#define VREF_HI   1
#define VREF_2    2
#define VREF_3    3

/*
 *  Functions
 */
void DAC_write(float value[4]);
void DAC_test(void); // Ramp the DACs
