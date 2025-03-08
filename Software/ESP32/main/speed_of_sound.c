/*-------------------------------------------------------
 *
 * speed_of_sound.c
 *
 * Compute the speed of sound based on Temp & Humidity
 *
 * Contributed by Steve Carrington
 *
 * ----------------------------------------------------*/

#include "math.h"
#include "stdio.h"

#include "freETarget.h"
#include "gpio.h"
#include "diag_tools.h"
#include "serial_io.h"
#include "compute_hit.h"

#define TO_MM       1000.0d           // Convert Metres to mm
#define TO_US       1000000.0d        // Convert seconds to microseconds
#define R           8314.46261815324d // Universal gas constant kJ/K*mol, 2019.
#define M           28.966d           // Mean Molar Mass of Dry Air with 0.0440% CO2. Projected for 2030.
#define speed_MPSTo 331.38d           // Speed in Metres per second of Sound in Dry Air at 0 degrees C.

/*----------------------------------------------------------------
 *
 * @function: speed_of_sound
 *
 * @brief: Return the speed of sound (mm / us)
 *
 *----------------------------------------------------------------
 *
 * The speed of sound is computed based on the board temperature
 * The Relative Humitity is fixed at 50% pending the addtion of
 * a humidity sensor
 *
 * For Theory see the following:
 * Enviromental Effects on the Speed of Sound by Dennis A. Bohn,
 * Journal of Audio Engineering Society, Vol. 36, No.4, 1988 April.
 *
 * For Vapor Pressure algorithm see the following:
 * Vapor Pressure Equation for Water in the Range 0 to 100 C, by Arnold Wexler and Lewis Greenspan,
 * Journal of Research of the National Bureau of Standards.
 * Section A, Physics and Chemistry.
 * National Institute of Standards and Technology.
 *
 * For Speific Heats Cp & Cv to generate algorithm for y = Ratio of Specific Heats see the following:
 * Engineering ToolBox,(2005), Dry Air - Thermodynamic and Physical Properties.
 *
 * For Molar Mass of Dry Air see the following:
 * "A twenty-first centry molar mass for dry air." The Free Libary. 2008
 *
 * NOTE: The following assumes that Air is a homogeneous mixture.
 *
 * Speed of sound in an ideal Gas is given by the equation:
 * C = SQRT(yRT/M)
 * C = Speed of Sound.....(speed_MPS)
 * y Ratio of Specific Heats of Gas (Cp/Cv), varies with Temperature for Air @ Standard Atmospheic Pressure. y = 1.4 for an ideal diatomic
 *gas. R Universal Gas Constant. Revised 2019. Avogaro's number and the Boltzmann constant were given exact numerical values as was the gas
 *constant. SI Units. T Temperature in degrees Kelvin. (Use TK below.) M Mean Molar Mass of Gas @ sea level, varies for Air due mainly to
 *Humidity and CO2 content.
 *
 * Valid for temperature range:   -20 C  < T <  40 C
 *
 *--------------------------------------------------------------*/

#define TO_MM 1000.0d                          // Convert Meters to MM
#define TO_US 1000000.0d                       // Convert seconds to microseconds

double speed_of_sound(double temperature,      // Current temperature in degrees C
                      double relative_humidity // RH, 0-100%
)
{
  double speed_MPS;                            // Speed Metres per second
  double speed_mmPuS;                          // Speed in mm per microsecond

  double y;                                    // Ratio of Specific Heats
  double TK;                                   // Temperature in degrees Kelvin
  double vapor_pressure;                       //
  double mole_fraction;                        //
  double specific_heat_ratio;                  // Specific heat ratio of air @ relative_humidity
  double mean_molar_weight;                    // Mean Molar Weight of Air @ relative_humidity
  double change_in_speed;                      // % Change in speed due to humidity, relative to Speed of Sound in Dry Air at 0 degrees C.

  //********************************************************************************************************************************************************************************
  // The following can be implemented to calculate a more accurate value for M defined above as 28.9660 if the YEAR is known.
  // double MMM                  // Mean Molar Mass of Dry Air.
  // int CO2_percent             // Calculated % CO2 (PPM) based on NOAA Data. Estimated valid to 2036. Accuracy < 1% error 1945~2036. Used
  // to calculate Mean Molar Mass of Dry Air.
  // Calculate Mean Molar Mass of Dry Air for previous year CO2 average content.
  // present_YEAR = year()
  // CO2_percent = int(57697.26 - 59.19204286*(present_YEAR - 1) + 0.015264286*sq(present_YEAR - 1)  // % CO2 in PPM as a function of YEAR
  // MMM = 28.96074487d + ((double)CO2_percent)*12.0107d/1000000.0d  // Mean Molecular Mass of Dry Air for CO2_percent.
  //*********************************************************************************************************************************************************************************

  // Calculate Coefficients to determine % change in speed of sound due to Humidity only, relative to Speed of Sound in Dry Air at 0 degrees
  // C.

  if ( temperature < 0.0d )
  {
    relative_humidity = 0; // Vapor Pressure below 0 degrees C has little effect on Speed of Sound, due to aerosols it tends to crystalize.
  }

  TK = 273.15d + temperature; // Temperature converted to degrees Kelvin.

  vapor_pressure = exp(((-7511.52d / TK) + 96.5389644d + (0.02399897d * (TK)) + (-0.000011654551d * sq(TK)) +
                        (-0.000000012810336d * (TK * TK * TK)) + (0.000000000020998405d * (TK * TK * TK * TK))) -
                       12.150799d * log(TK));
  // Calculate saturation vapor pressure at temperature TK

  mole_fraction = 0.01d * ((double)relative_humidity) * vapor_pressure / 101325.0d; // Calculate Mole Fraction of Water Vapor.

  specific_heat_ratio = (7.0d + mole_fraction) / (5.0d + mole_fraction); // Calculate Specific Heat Ratio of air @ relative_humidity

  mean_molar_weight = (M - (M - 18.01528d) * mole_fraction);             // Mean Molar Weight of Air @ relative_humidity

  change_in_speed = (1.0d / sqrt(1.4d / M) * 100.0d) * sqrt(specific_heat_ratio / mean_molar_weight) - 100.0d;
  // % Change in speed due to humidity, relative to Speed of Sound in Dry Air at 0 degrees C.

  y = 1.40092d - (0.0000196395d * temperature) - (0.000000162593d * sq(temperature));
  // Algorithm for Ratio of Speific Heats of Air (Cp/Cv) relative to temperature in degrees C.

  speed_MPS = sqrt((y * R * TK) / M)       // Speed, in Metres per second, of Sound in Dry Air at (temperature) degrees C.

              + ((speed_MPSTo / 100.0d) *
                 change_in_speed);         //% Change in speed due to humidity, relative to Speed of Sound in Dry Air at 0 degrees C.

  speed_mmPuS = speed_MPS * TO_MM / TO_US; // Convert down to mm/us

  DLT(DLT_DIAG, SEND(ALL, sprintf(_xs, "Temperature: %4.2fC Humidity: %4.2f%% Speed of Sound: %4.2fmm/us", temperature, relative_humidity,
                                  speed_MPS);))

  /*
   * @return the speed of sound
   */
  return speed_mmPuS;
}
