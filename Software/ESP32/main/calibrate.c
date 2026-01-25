/*****************************************************************************
 *
 * Calibrate.c
 *
 * Perform target calibration calculations
 *
 *****************************************************************************/
#include "math.h"
#include "analog_io.h"
#include "stdio.h"
#include "math.h"
#include "stdbool.h"
#include "gpio_types.h"
#include "driver\gpio.h"
#include "nvs.h"
#include "nvs_flash.h"



#include "freETarget.h"
#include "helpers.h"
#include "json.h"
#include "mfs.h"
#include "diag_tools.h"
#include "token.h"
#include "timer.h"
#include "compute_hit.h"
#include "serial_io.h"
#include "gpio.h"
#include "http_client.h"
#include "json.h"
#include "nonvol.h" 

/*
 *  Function Prototypes
 */
static void start_calibration(void);    // First step in the calibration
static void perform_calibration(void);  // Second step in the calibration
static void do_shot_calibratiion(void); // Perform the least squares fit
static void find_coeficients(void);     // Find the spline coeficients
static void commit_calibration(void);   // Commit the calibration to NONVOL

/*
 *  Definitions
 */
typedef struct spline_point
{
  float t_x;        // X as recorded on the target
  float a_x;        // Actual X location
  float t_y;        // Y as recorded on the target
  float a_y;        // Actual Y location
  float t_rho;      // Distance along the spline
  float t_theta;    // Angle along the spline
  float a_rho;      // Distance along the spline
  float a_theta;    // Angle along the spline
  float scale;      // Scale factor (actual / target) ~1.0
  float k0, k1, k2; // Spline coeficients
} spline_point_t;

/*
 *  Variables
 */

static spline_point_t spline_points[10]; // Spline points for calibration
static spline_point_t spline_temp;       // Temporary storage for sorting

static float calib_shot_x[10];           // X locations of calibration shots
static float calib_shot_y[10];           // Y locations of calibration shots
static int   calib_shot_n;               // Number of shots gathered

static float centre_of_mass_t_x;         // Centre of mass of target shots
static float centre_of_mass_t_y;
static float centre_of_mass_a_x;         // Centre of mass of actual shots
static float centre_of_mass_a_y;
static float delta_x, delta_y;           // Delta between centres of mass

/*----------------------------------------------------------------
 *
 * @function: calibrate()
 *
 * @brief: Perform a calibration on the target
 *
 * @return: NONVOL calibration data updated
 *
 *----------------------------------------------------------------
 *
 * Target calibration is performed in two steps.
 *
 * 1 - Gather data from ten shots
 * 2- Enter the actual locations of the shots and perform
 *    a least squares fit to determine the calibration
 *
 *--------------------------------------------------------------*/
void calibrate(void)
{
  double value; // User input

  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "calibration");))

  SEND(ALL, sprintf(_xs, "\r\nTarget Calibration\r\n");)
  SEND(ALL, sprintf(_xs, "\r\n1 - Gather shot data");)
  SEND(ALL, sprintf(_xs, "\r\n2 - Input actual shots");)
  SEND(ALL, sprintf(_xs, "\r\n3 - Commit calibration data to NONVOL");)
  get_number("\r\n\r\nSelect step (1/2/3):", &value);

  switch ( (int)value )
  {
    case 1:
      start_calibration();
      return;

    case 2:
      perform_calibration();
      return;

    case 3:
      commit_calibration();
      return;

    default:
      SEND(ALL, sprintf(_xs, "\r\nUnknown input. Exiting");)
      return;
  }
}

/*----------------------------------------------------------------
 *
 * @function: start_calibration()
 *
 * @brief: Give the user instructions to gather shot data
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * Target calibration is performed in two steps.
 *
 * 1 - Gather data from ten shots
 * 2- Enter the actual locations of the shots and perform
 *    a least squares fit to determine the calibration
 *
 *--------------------------------------------------------------*/

static void start_calibration(void)
{
  SEND(ALL, sprintf(_xs, "\r\nStep 1 - Gather Shot Data\r\n");)
  SEND(ALL, sprintf(_xs, "\r\nUse a pistol target");)
  SEND(ALL, sprintf(_xs, "\r\nFire ten shots at random locations around the target.");)
  SEND(ALL, sprintf(_xs, "\r\nWhen all ten shots are fired, proceed to step 2 to enter the actual locations.");)
  SEND(ALL, sprintf(_xs, "\r\n\r\nStart calibration now...\r\n");)
  shot_in  = 0; // Clear out any junk
  shot_out = 0;
  return;
}

/*----------------------------------------------------------------
 *
 * @function: perform_calibration()
 *
 * @brief: Gather the data and do a least squares fit
 *
 * @return: NONVOL updated with new calibration data
 *
 *----------------------------------------------------------------
 *
 * The calibration is
 *
 *--------------------------------------------------------------*/
static void perform_calibration(void)
{
  float  rho, theta;     // Actual shot location in polar coordinates;
  float  a_x_mm, a_y_mm; // Temporary variables Actual
  float  t_x_mm, t_y_mm; // Temporary variables Target
  double value;          // User input
  int    i, j;           // Loop counters

  SEND(ALL, sprintf(_xs, "\r\nPerforming Calibration\r\n");)
  SEND(ALL, sprintf(_xs, "\r\nEnter actual shot locations");)

  /*
   *  Validate the shots
   */
  if ( shot_in < 10 )
  {
    SEND(ALL, sprintf(_xs, "\r\nNot enough shots fired (%d). Exiting.", shot_in);)
    return;
  }

  /*
   * Gather actual shot locations
   */
  calib_shot_n = 0;
  while ( (calib_shot_n < shot_in) && (calib_shot_n < sizeof(calib_shot_x) / sizeof(calib_shot_x[0])) )
  {
    SEND(ALL, sprintf(_xs, "\r\nShot %d X (mm):", calib_shot_n + 1);)
    get_number("", &value);
    spline_points[calib_shot_n].a_x = (float)value;

    SEND(ALL, sprintf(_xs, "Shot %d Y (mm):", calib_shot_n + 1);)
    get_number("", &value);
    spline_points[calib_shot_n].a_y = (float)value;

    calib_shot_n++;
  }

  /*
   *  Find the centre of mass for the shots, Actual and Target
   */
  centre_of_mass_t_x = 0.0f;
  centre_of_mass_t_y = 0.0f;

  for ( i = 0; i < calib_shot_n; i++ )
  {
    centre_of_mass_t_x += spline_points[i].t_x;
    centre_of_mass_t_y += spline_points[i].t_y;
  }
  centre_of_mass_t_x /= calib_shot_n;
  centre_of_mass_t_y /= calib_shot_n;

  centre_of_mass_a_x = 0.0f;
  centre_of_mass_a_y = 0.0f;
  for ( i = 0; i < calib_shot_n; i++ )
  {
    centre_of_mass_a_x += record[i].x_mm;
    centre_of_mass_a_y += record[i].y_mm;
  }
  centre_of_mass_a_x /= calib_shot_n;
  centre_of_mass_a_y /= calib_shot_n;

  /*
   * Convert to polar coordinates and store recorded locations
   */
  for ( i = 0; i < calib_shot_n; i++ )
  {
    a_x_mm = spline_points[i].a_x - centre_of_mass_a_x;
    a_y_mm = spline_points[i].a_y - centre_of_mass_a_y;

    spline_points[i].a_rho   = sqrtf(a_x_mm * a_x_mm + a_y_mm * a_y_mm);
    spline_points[i].a_theta = atan2f(a_y_mm, a_x_mm);
  }

  /*
   *  Bubble sort the list by angle (rho)
   */
  for ( i = 0; i < calib_shot_n - 1; i++ )
  {
    for ( j = 0; j < calib_shot_n - i - 1; j++ )
    {
      if ( spline_points[j].a_theta > spline_points[j + 1].a_theta )
      {
        spline_temp          = spline_points[j];
        spline_points[j]     = spline_points[j + 1]; // Swap the points
        spline_points[j + 1] = spline_temp;
      }
    }
  }

  /*
   *  Insert the target values in the right order with the actuals
   */
  for ( i = 0; i < calib_shot_n; i++ )
  {
    t_x_mm = record[i].x_mm - centre_of_mass_a_x;
    t_y_mm = record[i].y_mm - centre_of_mass_a_y;
    rho    = sqrtf(t_x_mm * t_x_mm + t_y_mm * t_y_mm);
    theta  = atan2f(t_y_mm, t_x_mm);

    for ( j = 0; j < calib_shot_n; j++ )
    {
      if ( j == calib_shot_n - 1 )
      {
        spline_points[j].t_x     = t_x_mm;
        spline_points[j].t_y     = t_y_mm;
        spline_points[j].t_rho   = rho;
        spline_points[j].t_theta = theta;
        spline_points[j].scale   = rho / spline_points[j].a_rho; // Scale factor
        break;
      }
      else if ( spline_points[j].a_theta <= theta && spline_points[j + 1].a_theta >= theta )
      {
        spline_points[j].t_x     = t_x_mm;
        spline_points[j].t_y     = t_y_mm;
        spline_points[j].t_rho   = rho;
        spline_points[j].t_theta = theta;
        spline_points[j].scale   = rho / spline_points[j].a_rho; // Scale factor
        break;
      }
    }
  }

  /*
   *  Perform the calibration calculations
   */
  find_coeficients();
  return;
}

/*----------------------------------------------------------------
 *
 * @function: find_coeficients()
 *
 * @brief: Find the coeficients for the spline fits
 *
 * @return: Coeficients stored in spline_points
 *
 *----------------------------------------------------------------
 *
 * The spline operates on three points at a time
 *
 * before, point, after
 *
 * This function finds the coeficients k0, k1, k2 for each point
 * in the spline based on the scale factors at each point.
 *
 * See Numerical Recipes for details.
 *
 * https://en.wikipedia.org/wiki/Spline_interpolation
 *
 *--------------------------------------------------------------*/
static void find_coeficients(void)
{
  float a[3][4];    // Augmented matrix for solving
  int   i, j, k;    // Loop counter
  float x0, x1, x2; // X variable, (rho  in this case)
  float y0, y1, y2; // Y variable (scale in this case

  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "\r\nCalculating calibration coeficients...\r\n");))

  /*
   *  Construct the linear system of equations for each segment
   */
  for ( i = 0; i < calib_shot_n - 1; i++ )
  {
    x0 = spline_points[i - 1].t_rho;
    x1 = spline_points[i].t_rho;
    x2 = spline_points[i + 1].t_rho;

    y0 = spline_points[i - 1].scale;
    y1 = spline_points[i].scale;
    y2 = spline_points[i + 1].scale;

    a[0][0] = 2.0 / (x1 - x0);
    a[0][1] = 1.0 / (x1 - x0);
    a[0][2] = 0.0f;

    a[1][0] = 1.0 / (x1 - x0);
    a[1][1] = 2.0 * ((1.0 / (x1 - x0)) + (1.0 / (x2 - x1)));
    a[1][2] = 1.0 / (x2 - x1);

    a[2][0] = 0.0f;
    a[2][1] = 1.0 / (x2 - x1);
    a[2][2] = 2.0 / (x2 - x1);

    a[0][3] = 3.0 * ((y1 - y0) / ((x1 - x0) * (x1 - x0)));
    a[1][3] = 3.0 * ((y2 - y1) / ((x2 - x1) * (x2 - x1)) - (y1 - y0) / ((x1 - x0) * (x1 - x0)));
    a[2][3] = 3.0 * ((y2 - y1) / ((x2 - x1) * (x2 - x1)));
  }

  /*
   * Solve the linear equations to find k
   */
  for ( i = 0; i < 3; i++ )
  {
    float diag = a[i][i];     // Make the diagonal 1
    for ( j = 0; j < 4; j++ )
    {
      a[i][j] /= diag;
    }

    for ( k = 0; k < 3; k++ ) // Eliminate the other rows
    {
      if ( k != i )
      {
        float factor = a[k][i];
        for ( j = 0; j < 4; j++ )
        {
          a[k][j] -= factor * a[i][j];
        }
      }
    }
  }

  /*
   *  Store the coefficients for each point
   */
  spline_points[i].k0 = a[0][3];
  spline_points[i].k1 = a[1][3];
  spline_points[i].k2 = a[2][3];

  /*
   * All done, return
   */
  SEND(ALL, sprintf(_xs, "\r\nCalibration calculations complete.\r\n");)
  return;
}

/*----------------------------------------------------------------
 *
 * @function: commit_calibration()
 *
 * @brief: Commit the calibration data to NONVOL
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * Save the calibration constants to NONVOL
 *
 * Save them as discrete values for each point.
 *--------------------------------------------------------------*/
void commit_calibration(void)
{
  int  i;
  char name[32];
  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "\r\nCommitting calibration data to NONVOL...\r\n");))

  for ( i = 0; i != sizeof(spline_points) / sizeof(spline_points[0]); i++ )
  {
    sprintf(name, "%s_%d%d", NONVOL_CALIBRATION_DATA, i, 0);
    nvs_set_f32(my_handle, name, &spline_points[i].k0);
    sprintf(name, "%s_%d%d", NONVOL_CALIBRATION_DATA, i, 1);
    nvs_set_f32(my_handle, name, &spline_points[i].k1);
    sprintf(name, "%s_%d%d", NONVOL_CALIBRATION_DATA, i, 2);
    nvs_set_f32(my_handle, name, &spline_points[i].k2);
  }

  nvs_commit(my_handle);
  SEND(ALL, sprintf(_xs, "\r\nCalibration data committed to NONVOL.\r\n");)
  /*
   * Free the allocated memory
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: commit_calibration()
 *
 * @brief: Commit the calibration data to NONVOL
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * Save the calibration constants to NONVOL
 *
 * Save them as discrete values for each point.
 *--------------------------------------------------------------*/
void get_calibration(void)
{
  int  i;
  char name[32];
  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "\r\nRetrieving calibration data from NONVOL...\r\n");))

  for ( i = 0; i != sizeof(spline_points) / sizeof(spline_points[0]); i++ )
  {
    sprintf(name, "%s_%d%d", NONVOL_CALIBRATION_DATA, i, 0);
    nvs_get_f32(my_handle, name, &spline_points[i].k0);
    sprintf(name, "%s_%d%d", NONVOL_CALIBRATION_DATA, i, 1);
    nvs_get_f32(my_handle, name, &spline_points[i].k1);
    sprintf(name, "%s_%d%d", NONVOL_CALIBRATION_DATA, i, 2);
    nvs_get_f32(my_handle, name, &spline_points[i].k2);
  }

  /*
   * Free the allocated memory
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: solve_spline()
 *
 * @brief: Use the spline coeficients to find the scale factor
 *
 * @return: new scaled value
 *
 *----------------------------------------------------------------
 *
 *
 *
 *
 *--------------------------------------------------------------*/
float solve_spline(float theta)
{
  int   i;       // Loop counter
  float scale;   // Scale factor
  float delta_r; // Delta rho

  /*
   *  Find the right segment
   */
  for ( i = 0; i < calib_shot_n - 1; i++ )
  {
    if ( theta >= spline_points[i].a_theta && theta <= spline_points[i + 1].a_theta )
    {
      /*
       *  Solve the spline equation
       */
      delta_r = theta - spline_points[i].a_theta;
      scale   = spline_points[i].k0 + (spline_points[i].k1) * delta_r + (spline_points[i].k2 * delta_r * delta_r);
      return scale;
    }
  }

  /*
   *  Out of range, return 1.0
   */
  return 1.0f;
}