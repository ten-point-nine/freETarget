/*****************************************************************************
 *
 * Calibrate.c
 *
 * Perform target calibration calculations
 *
 *****************************************************************************
 *
 * In this file.
 *
 * The designation target or t_ indicates a calculation from the target
 * The designation actual or a_ refers to the actul measured value from the target
 *
 *****************************************************************************/
#include "math.h"
#include "stdio.h"
#include "stdbool.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "freETarget.h"
#include "helpers.h"
#include "diag_tools.h"
#include "nonvol.h"
#include "calibrate.h"

/*
 *  Function Prototypes
 */
static void start_calibration(void);   // First step in the calibration
static void perform_calibration(void); // Second step in the calibration
static void find_coeficients(void);    // Find the spline coeficients
static void commit_calibration(void);  // Commit the calibration to NONVOL
static void verify_calibration(void);  // Verify the calibration data by applying it to the shots

/*
 *  Definitions
 */
typedef struct spline_point
{
  double t_x;                    // X as recorded on the target
  double t_y;                    // Y as recorded on the target
  double a_x;                    // Actual X location
  double a_y;                    // Actual Y location
  double t_rho;                  // Distance along the spline
  double t_angle;                // Angle along the spline
  double a_rho;                  // Distance along the spline
  double a_angle;                // Angle along the spline
  double scale;                  // Scale factor (actual / target) ~1.0
  double k0, k1, k2;             // Spline coeficients
} spline_point_t;

#define MAX_CALIBRATION_SHOTS 10 // Maximum number of calibration shots
#define SPLINE_PADDING        2  // Extra points at start and end

/*
 *  Variables
 */
static spline_point_t spline_points[MAX_CALIBRATION_SHOTS + SPLINE_PADDING * 2]; // Spline points for calibration

static int  calib_shot_n;                                                        // Number of shots gathered
static bool calibration_diag;                                                    // TRUE if in test mode

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

  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "calibrate()");))

  SEND(ALL, sprintf(_xs, "\r\nTarget Calibration\r\n");)
  SEND(ALL, sprintf(_xs, "\r\n1 - Gather shot data");)
  SEND(ALL, sprintf(_xs, "\r\n2 - Input actual shots");)
  SEND(ALL, sprintf(_xs, "\r\n3 - Commit calibration data to NONVOL");)
  SEND(ALL, sprintf(_xs, "\r\n9 - Exit, no action");)

  get_number("\r\n\r\nSelect step (1/2/3/9):", &value);

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

    case 9:
      SEND(ALL, sprintf(_xs, "\r\nExiting calibration. No changes made.");)
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
  SEND(ALL, sprintf(_xs, "\r\nWhen all ten shots are fired, proceed to Step 2 to enter the actual locations.");)
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
 * The calibration is performed by the following steps
 *
 * 1 - Gather actual shot locations from the user
 * 2 - Find the centre of mass for actual and target shots
 * 3 - Convert to polar coordinates
 * 4 - Sort by angle
 * 5 - Find the spline coeficients
 *
 *--------------------------------------------------------------*/
static void perform_calibration(void)
{
  double         value;       // User input
  int            i, j;        // Loop counters
  spline_point_t spline_temp; // Temporary

  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "perform_calibration");))

  /*
   *  Validate the shots
   */
  if ( shot_in < MAX_CALIBRATION_SHOTS )
  {
    SEND(ALL, sprintf(_xs, "\r\nNot enough shots fired (%d). Exiting.", shot_in);)
    return;
  }

  /*
   * Gather actual shot locations
   */
  calib_shot_n = 0;
  if ( calibration_diag == false )
  {
    while ( (calib_shot_n < shot_in) && (calib_shot_n < MAX_CALIBRATION_SHOTS) )
    {
      SEND(ALL, sprintf(_xs, "\r\nShot %d X (mm):", calib_shot_n + 1);)
      get_number("", &value);
      spline_points[calib_shot_n].a_x = (double)value;
      spline_points[calib_shot_n].t_x = record[calib_shot_n].x_mm;

      SEND(ALL, sprintf(_xs, "Shot %d Y (mm):", calib_shot_n + 1);)
      get_number("", &value);
      spline_points[calib_shot_n].a_y = (double)value;
      spline_points[calib_shot_n].t_y = record[calib_shot_n].y_mm;

      spline_points[calib_shot_n].t_rho   = sqrtf(SQ(spline_points[calib_shot_n].t_x) + SQ(spline_points[calib_shot_n].t_y));
      spline_points[calib_shot_n].t_angle = atan2_2PI(spline_points[calib_shot_n].t_y, spline_points[calib_shot_n].t_x);

      calib_shot_n++;
    }
  }
  else
  {
    calib_shot_n = MAX_CALIBRATION_SHOTS;
    shot_in      = MAX_CALIBRATION_SHOTS;
  }

  /*
   *  Bubble sort the list by angle (rho)
   */
  for ( i = 0; i < calib_shot_n - 1; i++ )
  {
    for ( j = 0; j < calib_shot_n - i - 1; j++ )
    {
      if ( spline_points[j].a_angle > spline_points[j + 1].a_angle )
      {
        spline_temp          = spline_points[j];
        spline_points[j]     = spline_points[j + 1]; // Swap the points
        spline_points[j + 1] = spline_temp;
      }
    }
  }

  for ( i = 0; i < calib_shot_n; i++ )
  {
    printf("\r\nIndex: %d  Angle: %.6f", i, spline_points[i].a_angle);
  }

  /*
   *  Pad the beginning and end of the list with extra points to prevent singularities
   *  0->2   1->3   ... n-2->n   n-1->n+1
   *
   *  then 0 and 1 are copies of n-2 and n-1 with angle adjusted
   *  and n+1 and n+2 are copies of 2 and 3 with angle adjusted
   *
   */
  for ( i = calib_shot_n - 1; i >= 0; i-- )
  {
    spline_points[i + SPLINE_PADDING] = spline_points[i];              // Shift up
  }

  spline_points[0] = spline_points[calib_shot_n - 2 + SPLINE_PADDING]; // Put the last two points at the start
  spline_points[0].a_angle -= TWO_PI;
  spline_points[1] = spline_points[calib_shot_n - 1 + SPLINE_PADDING];
  spline_points[1].a_angle -= TWO_PI;
  spline_points[calib_shot_n + 0 + SPLINE_PADDING] = spline_points[3]; // Put the first two points at the end
  spline_points[calib_shot_n + 0 + SPLINE_PADDING].a_angle += TWO_PI;
  spline_points[calib_shot_n + 1 + SPLINE_PADDING] = spline_points[2];
  spline_points[calib_shot_n + 1 + SPLINE_PADDING].a_angle += TWO_PI;

  printf("\r\nPadded");
  for ( i = 0; i < MAX_CALIBRATION_SHOTS + SPLINE_PADDING * 2; i++ )
  {
    printf("\r\nIndex: %d  Angle: %.6f", i, spline_points[i].a_angle);
  }
  // Correct to this point

  /*
   *  Perform the calibration calculations
   */
  find_coeficients();

  /*
   * Check the calibration data
   */
  verify_calibration(); // Apply the calibration to the shots and see if they match the calibration

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
#define NX 4         // Number of points in the spline segment
#define NY 3         // Number of equations +1 for augmented matrix

static void find_coeficients(void)
{
  double a[NY][NX];  // Augmented matrix for solving
  int    i, j, k;    // Loop counter
  int    spline_i;   // Spline point index
  double x0, x1, x2; // X variable, (rho  in this case)
  double y0, y1, y2; // Y variable (scale in this case

  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "find_coefficient");))

  printf("\r\nfind_coefficeints()\r\n");
  /*
   *  Construct the linear system of equations for each segment
   */
  for ( spline_i = 1; spline_i < (calib_shot_n + SPLINE_PADDING * 2); spline_i++ )
  {
    x0 = spline_points[spline_i - 1].t_rho;
    x1 = spline_points[spline_i].t_rho;
    x2 = spline_points[spline_i + 1].t_rho;

    y0 = spline_points[spline_i - 1].scale;
    y1 = spline_points[spline_i].scale;
    y2 = spline_points[spline_i + 1].scale;

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

    /*
     * Solve the linear equations to find k
     */
    for ( i = 0; i < NY; i++ )
    {
      double diag = a[i][i];     // Make the diagonal 1
      for ( j = 0; j < NX; j++ )
      {
        a[i][j] /= diag;
      }

      for ( k = 0; k < NY; k++ ) // Eliminate the other rows
      {
        if ( k != i )
        {
          double factor = a[k][i];
          for ( j = 0; j < NX; j++ )
          {
            a[k][j] -= factor * a[i][j];
          }
        }
      }
    }

    /*
     *  Store the coefficients for each point
     */
    spline_points[spline_i].k0 = a[0][3];
    spline_points[spline_i].k1 = a[1][3];
    spline_points[spline_i].k2 = a[2][3];

    printf("\r\nIndex %d k0: %.6f  k1: %.6f  k2: %.6f  ", spline_i, spline_points[spline_i].k0, spline_points[spline_i].k1,
           spline_points[spline_i].k2);
  }

  /*
   * All done, return
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
void commit_calibration(void)
{
  int  i;
  char name[32];
  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "commit_calibration()");))

  /*
   *  Save the spline coefficients for each point to NONVOL
   */
  for ( i = 0; i != sizeof(spline_points) / sizeof(spline_points[0]); i++ )
  {
    sprintf(name, "%s_%d%d", NONVOL_CALIBRATION_DATA, i, 0);
    nvs_set_i32(my_handle, name, (uint32_t)(spline_points[i].k0 * 10000)); // Conver to integer to save
    sprintf(name, "%s_%d%d", NONVOL_CALIBRATION_DATA, i, 1);
    nvs_set_i32(my_handle, name, (uint32_t)(spline_points[i].k1 * 10000));
    sprintf(name, "%s_%d%d", NONVOL_CALIBRATION_DATA, i, 2);
    nvs_set_i32(my_handle, name, (uint32_t)(spline_points[i].k2 * 10000));
  }

  nvs_commit(my_handle);

  /*
   * All done, return
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: get_calibration()
 *
 * @brief: Retrieve the calibration data from NONVOL
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * Pull the calibration data from NONVOL into the spline_points array
 * for use during target processing.
 *--------------------------------------------------------------*/
void get_calibration(void)
{
  int  i;
  char name[32];
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "get_calibration()");))

  for ( i = 0; i != sizeof(spline_points) / sizeof(spline_points[0]); i++ )
  {
    sprintf(name, "%s_%d%d", NONVOL_CALIBRATION_DATA, i, 0);
    nvs_get_i32(my_handle, name, &spline_points[i].k0);
    sprintf(name, "%s_%d%d", NONVOL_CALIBRATION_DATA, i, 1);
    nvs_get_i32(my_handle, name, &spline_points[i].k1);
    sprintf(name, "%s_%d%d", NONVOL_CALIBRATION_DATA, i, 2);
    nvs_get_i32(my_handle, name, &spline_points[i].k2);
  }

  /*
   * All done, return
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: verify_calibration()
 *
 * @brief: Apply the calibration to the shots and see if they match
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * The calibration is verified by applying the spline to the
 * recorded shots and comparing the scaled rho to the
 * calibrated rho.
 *
 *--------------------------------------------------------------*/
static void verify_calibration(void)
{
  int    i;
  double scale; //

  SEND(ALL, sprintf(_xs, "\r\nverify_calibration()\r\n");)

  for ( i = SPLINE_PADDING; i < MAX_CALIBRATION_SHOTS + SPLINE_PADDING; i++ )
  {
    scale = solve_spline(spline_points[i].a_angle);
    SEND(ALL, sprintf(_xs, "\r\nIndex:%d  Angle:%4.6f  Actual Scale:%4.6f  Calibrated Scale: %4.6f", i, spline_points[i].a_angle,
                      spline_points[i].scale, scale);)
  }

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
 * This implements the spline interpolation given in the Wikipedia
 * article:
 *
 * https://en.wikipedia.org/wiki/Spline_interpolation
 *
 *
 *--------------------------------------------------------------*/
double solve_spline(double angle)
{
  int    s;      // segment index
  double scale;  // Scale factor
  double x0, x1; // X boundaries
  double y0;     // Value of y at x0
  double t;      // Interval fraction

  /*
   *  Find the right segment
   */
  for ( s = SPLINE_PADDING; s < MAX_CALIBRATION_SHOTS + SPLINE_PADDING; s++ )
  {
    if ( (angle >= spline_points[s].a_angle) && (angle <= spline_points[s + 1].a_angle) )
    {
      printf("\r\nIndex: %d angle: %.6f  spline:%.6f", s, angle, spline_points[s].a_angle);
      break;
    }
  }

  if ( s == MAX_CALIBRATION_SHOTS + SPLINE_PADDING )
  {
    return 1.0f; // Not found, return 1.0
  }

  /*
   * Calculate the scale factor in this segment
   */
  x0 = spline_points[s].a_angle;
  x1 = spline_points[s + 1].a_angle;
  y0 = spline_points[s].scale;

  t = (angle - x0) / (x1 - x0); // Interval fraction
  printf("\r\nIndex: %d x0: %.6f   x1: %.6f  t: %6f ", s, x0, x1, t);
  scale = y0 + (spline_points[s].k0 * t) + (spline_points[s].k1 * t * t) + (spline_points[s].k2 * t * t * t);

  /*
   * All done, return the scale factor
   */
  return scale;
}

/*----------------------------------------------------------------
 *
 * @function: calibration_test
 *
 * @brief: Create a test calibration dataset
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * Create a ransom sample of calibration data for testing
 *
 *--------------------------------------------------------------*/
void calibration_test(void)
{
  unsigned int i;

  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "calibration_test()");))

  for ( i = 0; i != MAX_CALIBRATION_SHOTS; i++ )
  {
    record[i].x_mm   = (double)(rand() % 400) - 200.0f;
    record[i].y_mm   = (double)(rand() % 400) - 200.0f;
    record[i].angle  = atan2_2PI(record[i].y_mm, record[i].x_mm);
    record[i].radius = sqrtf(SQ(record[i].x_mm) + SQ(record[i].y_mm));

    spline_points[i].a_x = record[i].x_mm + ((double)(rand() % 5000) / 1000.0f) - 0.5f;
    spline_points[i].a_y = record[i].y_mm + ((double)(rand() % 5000) / 1000.0f) - 0.5f;
    spline_points[i].t_x = record[i].x_mm;
    spline_points[i].t_y = record[i].y_mm;

    spline_points[i].a_rho   = sqrtf(SQ(spline_points[i].a_x) + SQ(spline_points[i].a_y));
    spline_points[i].a_angle = atan2_2PI(spline_points[i].a_y, spline_points[i].a_x);
    spline_points[i].scale   = spline_points[i].a_rho / record[i].radius;

    spline_points[i].t_rho   = sqrtf(SQ(spline_points[i].t_x) + SQ(spline_points[i].t_y));
    spline_points[i].t_angle = atan2_2PI(spline_points[i].t_y, spline_points[i].t_x);
    SEND(ALL, sprintf(_xs, "\r\nIndex: %d  Angle: %.6f  Scale: %.6f", i, spline_points[i].a_angle, spline_points[i].scale);)
  }

  calibration_diag = true;
  shot_in          = MAX_CALIBRATION_SHOTS; // Fake it

  calibrate();

  return;
}
