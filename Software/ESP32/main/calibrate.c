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
 * The designation actual or actual_ refers to the actul measured value from the target
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
#include "json.h"
#include "nonvol.h"
#include "calibrate.h"

/*
 *  Function Prototypes
 */
static void start_calibration(void);             // First step in the calibration
static void perform_calibration(void);           // Second step in the calibration
static void find_coeficients(void);              // Find the spline coeficients
static void commit_calibration(void);            // Commit the calibration to NONVOL
static void verify_calibration(void);            // Verify the calibration data by applying it to the shots
static void void_calibration(void);              // Cancel the existing calibration
static void target_offset(int number_of_points); // Adjust the target data for sensor location
static void spline_sort(int calib_shot_n);       // Sort the entries in order

/*
 *  Definitions
 */
typedef struct spline_point
{
  double target_x;                   // X as recorded on the target
  double target_y;                   // Y as recorded on the target
  double actual_x;                   // Actual X location
  double actual_y;                   // Actual Y location
  double target_rho;                 // Radial distance from target
  double target_angle;               // Angle to shot from target in radians
  double actual_rho;                 // Radial distance as actual
  double actual_angle;               // Angle to shot as actual in radians
  double scale;                      // Scale factor (actual / target) ~1.0
  double k0, k1, k2;                 // Spline coeficients
} spline_point_t;

#define MAX_CALIBRATION_SHOTS 10     // Maximum number of calibration shots
#define SPLINE_PADDING        2      // Extra points at start and end
#define SPLINE_VALID          1234.0 // Value to show spline data is valid

/*
 *  Variables
 */
static spline_point_t spline_points[MAX_CALIBRATION_SHOTS + SPLINE_PADDING * 2]; // Spline points for calibration

static int  calib_shot_n;                                                        // Number of shots gathered
static bool calibration_diag;                                                    // TRUE if in test mode

static bool calibration_is_valid = false;                                        // Calibration valid marker

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
  double action; // User input

  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "calibrate()");))

  SEND(ALL, sprintf(_xs, "\r\nTarget Calibration\r\n");)
  SEND(ALL, sprintf(_xs, "\r\n1 - Gather shot data");)
  SEND(ALL, sprintf(_xs, "\r\n2 - Input actual shots");)
  SEND(ALL, sprintf(_xs, "\r\n3 - Commit calibration data to NONVOL");)
  SEND(ALL, sprintf(_xs, "\r\n4 - Void current calibration");)
  SEND(ALL, sprintf(_xs, "\r\n9 - Exit, no action");)

  get_number("\r\n\r\nSelect action: ", &action);

  switch ( (int)action )
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

    case 4:
      void_calibration();
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
  SEND(ALL, sprintf(_xs, "\r\nAction 1 - Gather Shot Data\r\n");)
  SEND(ALL, sprintf(_xs, "\r\nUse a pistol target");)
  SEND(ALL, sprintf(_xs, "\r\nFire at least ten shots at random locations around the target.");)
  SEND(ALL, sprintf(_xs, "\r\nWhen all ten shots are fired, proceed to action 2 to enter the actual locations.");)
  SEND(ALL, sprintf(_xs, "\r\n\r\nStart calibration now...\r\n");)

  shot_in                  = 0;     // Clear out any junk
  shot_out                 = 0;
  json_x_offset            = 0;     // Reset any offsets
  json_y_offset            = 0;
  json_sensor_angle_offset = 0;
  calibration_is_valid     = false; // Turn off the calibration

  /*
   *  Wait for the user to enter the data
   */
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
 * When entering the data, the user has the option to
 *
 * 1 - Use the data as entered
 * 2 - Skip this entry and try the next
 * 3 - Reenter the data
 *
 *--------------------------------------------------------------*/
static void perform_calibration(void)
{
  double x, y, action; // User input
  int    use_shot_n;   // Shot used for calibration

  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "perform_calibration()");))

  /*
   *  Validate the shots
   */
  if ( shot_in < MAX_CALIBRATION_SHOTS )
  {
    SEND(ALL, sprintf(_xs, "\r\nNot enough shots fired (%d). Exiting.", shot_in);)
    return;
  }

  SEND(ALL, sprintf(_xs, "\r\nChoose ten shots furthest away from the centre\r\n");)

  /*
   * Gather actual shot locations
   */
  calib_shot_n = 0; // Shot to use for calibration
  use_shot_n   = 0; // Shot used for data point

  if ( calibration_diag == false )
  {
    while ( (calib_shot_n < shot_in) && (calib_shot_n < MAX_CALIBRATION_SHOTS) )
    {
      while ( 1 )   //  Get input
      {
        SEND(ALL, sprintf(_xs, "Enter shot (%4.2f, %4.2f)", record[calib_shot_n].x_mm, record[calib_shot_n].y_mm);)
        get_number("\r\nActual X: ", &x);
        get_number("\r\nActual Y: ", &y);

        SEND(ALL, sprintf(_xs, "Target (%4.2f, %4.2f)   Actual (%4.2f, %4.2f)", record[calib_shot_n].x_mm, record[calib_shot_n].y_mm,
                          spline_points[calib_shot_n].target_x, spline_points[calib_shot_n].target_y);)

        get_number("\r\nAction (1-use, 2-skip, 3-reenter, 4-abort):", &action);

        switch ( (int)action )
        {
          case 1:                                                             // Use
            spline_points[calib_shot_n].actual_x   = (double)x;               // Actual
            spline_points[calib_shot_n].actual_y   = (double)y;
            spline_points[calib_shot_n].actual_rho = sqrtf(SQ(spline_points[use_shot_n].actual_x) + SQ(spline_points[use_shot_n].actual_y));
            spline_points[calib_shot_n].actual_angle = atan2_2PI(spline_points[use_shot_n].actual_y, spline_points[use_shot_n].actual_x);

            spline_points[calib_shot_n].target_x   = record[use_shot_n].x_mm; // Target
            spline_points[calib_shot_n].target_y   = record[use_shot_n].y_mm;
            spline_points[calib_shot_n].target_rho = sqrtf(SQ(spline_points[use_shot_n].target_x) + SQ(spline_points[use_shot_n].target_y));
            spline_points[calib_shot_n].target_angle = atan2_2PI(spline_points[use_shot_n].target_y, spline_points[use_shot_n].target_x);

            spline_points[calib_shot_n].scale = spline_points[calib_shot_n].target_rho / record[calib_shot_n].radius;
            calib_shot_n++;
            break;

          case 2: // Skip this shot
            use_shot_n++;
            break;

          case 3: // Enter data again
            break;

          case 4: // Bail out
            return;
        }
      }
    }
  }
  else
  {
    calib_shot_n = MAX_CALIBRATION_SHOTS;
    shot_in      = MAX_CALIBRATION_SHOTS;
  }

  /*
   *  Slide the data to match differences in the circuit
   */
  target_offset(calib_shot_n);

  /*
   *  Sort the data so that the entries are in order
   */
  spline_sort(calib_shot_n);

  /*
   *  Perform the calibration calculations
   */
  find_coeficients();

  /*
   * Check the calibration data
   */
  verify_calibration(); // Apply the calibration to the shots and see if they match the calibration

  SEND(ALL, sprintf(_xs, "\r\n\r\nCalibration complete. ");)
  SEND(ALL, sprintf(_xs, "\r\nRemember to commit the calibration");)
  return;
}
/*----------------------------------------------------------------
 *
 * @function: target_offset()
 *
 * @brief: Slide the target coordinates to match the actual
 *
 * @return:Target cooordinates adjusted
 *
 *----------------------------------------------------------------
 *
 * There is an offset between what the circtuit things and the
 * actual target postion.  This is most often due to the sensors
 * being slightly out of alighment.
 *
 * This function works out the average postion of the target
 * and the actual and slides the target data to meet the actual.
 *
 *--------------------------------------------------------------*/
void target_offset(int number_of_points)
{
  double avg_ax, avg_ay, avg_tx, avg_ty; // Averages
  double avg_aangle, avg_tangle;         // Average angles
  int    i;

  /*
   * Work out the offset between the targets
   */
  avg_ax     = 0;
  avg_ay     = 0;
  avg_tx     = 0;
  avg_ty     = 0;
  avg_tangle = 0;
  avg_aangle = 0;

  for ( i = 0; i < number_of_points; i++ )
  {
    avg_ax     = spline_points[i].actual_x;
    avg_ay     = spline_points[i].actual_y;
    avg_tx     = spline_points[i].target_x;
    avg_ty     = spline_points[i].target_y;
    avg_aangle = spline_points[i].actual_angle;
    avg_tangle = spline_points[i].target_angle;
  }

  json_x_offset            = (avg_ax - avg_tx) / calib_shot_n;
  json_y_offset            = (avg_ay - avg_ty) / calib_shot_n;
  json_sensor_angle_offset = (avg_aangle - avg_tangle) / calib_shot_n; // Offset in radians

  SEND(ALL,
       sprintf(_xs, "\r\nX Offset: %4.2f  Y Offset: %4.2f  Sensor: %4.2f\r\n", json_x_offset, json_y_offset, json_sensor_angle_offset);)

  /*
   *  Slide the target loctions slightly to match the actuals
   */
  for ( i = 0; i < number_of_points; i++ )
  {
    spline_points[i].target_x += json_x_offset;
    spline_points[i].target_y += json_y_offset;
    spline_points[i].target_angle += json_sensor_angle_offset; // in Radians
  }

  /*
   *  All done, return
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: spline_sort()
 *
 * @brief: Sort the entries into ascending order by angle
 *
 * @return:The spline list sorted in angle order
 *
 *----------------------------------------------------------------
 *
 * The data points are collected in random order.
 *
 * This function sorts the data into ascending order by angle. This
 * means that the first entries are lowest, and last entry is highest
 *
 * In order to avoid discontinuites at the beginning and end, the
 * highest angles are adjusted by 2-PI and are copied to the
 * beginning so that the lowest entries are correct.  Similarly the
 * lowest entries are copied to the end and adjusted so that the
 * large entries are coneinious.
 *
 *--------------------------------------------------------------*/
void spline_sort(int calib_shot_n)
{
  int            i, j;
  spline_point_t spline_temp;

  /*
   *  Bubble sort the list by angle
   */
  for ( i = 0; i < calib_shot_n - 1; i++ )
  {
    for ( j = 0; j < calib_shot_n - i - 1; j++ )
    {
      if ( spline_points[j].actual_angle > spline_points[j + 1].actual_angle )
      {
        spline_temp          = spline_points[j];
        spline_points[j]     = spline_points[j + 1]; // Swap the points
        spline_points[j + 1] = spline_temp;
      }
    }
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
  spline_points[0].actual_angle -= TWO_PI;
  spline_points[1] = spline_points[calib_shot_n - 1 + SPLINE_PADDING];
  spline_points[1].actual_angle -= TWO_PI;

  spline_points[calib_shot_n + 0 + SPLINE_PADDING] = spline_points[3]; // Put the first two points at the end
  spline_points[calib_shot_n + 0 + SPLINE_PADDING].actual_angle += TWO_PI;
  spline_points[calib_shot_n + 1 + SPLINE_PADDING] = spline_points[2];
  spline_points[calib_shot_n + 1 + SPLINE_PADDING].actual_angle += TWO_PI;

  /*
   Sorted. return
   */
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
#define NX 4           // Number of points in the spline segment
#define NY 3           // Number of equations +1 for augmented matrix

static void find_coeficients(void)
{
  double a[NY][NX];    // Augmented matrix for solving
  double diag, factor; // Diagonal element, Normalization factor
  int    i, j, k;      // Loop counter
  int    spline_i;     // Spline point index
  double x0, x1, x2;   // X variable, (rho  in this case)
  double y0, y1, y2;   // Y variable (scale in this case

  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "find_coefficient");))

  /*
   *  Construct the linear system of equations for each segment
   */
  for ( spline_i = 1; spline_i < (calib_shot_n + SPLINE_PADDING * 2); spline_i++ )
  {
    x0 = spline_points[spline_i - 1].actual_rho;
    x1 = spline_points[spline_i].actual_rho;
    x2 = spline_points[spline_i + 1].actual_rho;

    y0 = spline_points[spline_i - 1].scale;
    y1 = spline_points[spline_i].scale;
    y2 = spline_points[spline_i + 1].scale;

    no_singularity(&x0, &x1, &x2);
    no_singularity(&y0, &y1, &y2);

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
      diag = a[i][i];            // Make the diagonal 1
      for ( j = 0; j < NX; j++ )
      {
        a[i][j] /= diag;
      }

      for ( k = 0; k < NY; k++ ) // Eliminate the other rows
      {
        if ( k != i )
        {
          factor = a[k][i];
          for ( j = 0; j < NX; j++ )
          {
            a[k][j] -= (factor * a[i][j]);
          }
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
 * This generates a circle and computes a spline scale for each
 * angle.
 *
 * In the output list, each entry should show a scale of 1.0
 *
 *--------------------------------------------------------------*/
#define VERIFY_SIZE 40
#define VERIFY_STEP (TWO_PI / (float)VERIFY_SIZE)
static void verify_calibration(void)
{
  int    i;
  double scale; //  Answer we are looking for
  double theta; // Angle we are trying

  SEND(ALL, sprintf(_xs, "\r\nVerify_calibration()\r\n");)

  for ( i = 0; i < VERIFY_SIZE; i++ )
  {
    theta = VERIFY_STEP * (float)i;
    scale = solve_spline(theta, true); // Override calibration
    SEND(ALL, sprintf(_xs, "\r\nIndex:%d  Angle:%4.4f  Calibrated Scale: %4.6f", i, theta / PI, scale);)
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
double solve_spline(double angle, // Angle to compute scaling factor
                    bool   valid  // Override if needed
)
{
  int    s;                       // segment index
  double scale;                   // Scale factor
  double x0, x1;                  // X boundaries
  double y0;                      // Value of y at x0
  double t;                       // Interval fraction

  /*
   *  Return 1.0 if the calibration is not present
   */
  if ( (valid == false) || (calibration_is_valid == false) )
  {
    return 1.0f;
  }

  /*
   *  Find the right segment
   */
  for ( s = SPLINE_PADDING; s < MAX_CALIBRATION_SHOTS + SPLINE_PADDING; s++ )
  {
    if ( (angle >= spline_points[s].actual_angle) && (angle < spline_points[s + 1].actual_angle) )
    {
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
  x0 = spline_points[s].actual_angle;
  x1 = spline_points[s + 1].actual_angle;
  y0 = spline_points[s].scale;

  t     = (angle - x0) / (x1 - x0); // Interval fraction
  scale = y0 + (spline_points[s].k0 * t) + (spline_points[s].k1 * t * t) + (spline_points[s].k2 * t * t * t);

  /*
   * All done, return the scale factor
   */
  return scale;
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
  int     i;
  size_t  size;               // How many bytes written
  double *blob;

  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "commit_calibration()");))

  size      = 0;
  blob      = (double *)&_xs; // Point to a chunk of memory
  *(blob++) = SPLINE_VALID;   // Put in a marker
  size += sizeof(double);
  /*
   *  Put the calibration data into _xs
   */

  for ( i = 0; i != calib_shot_n; i++ )
  {
    *(blob++) = spline_points[i].scale;
    *(blob++) = spline_points[i].k0;
    *(blob++) = spline_points[i].k1;
    *(blob++) = spline_points[i].k2;
    *(blob++) = spline_points[i].actual_angle;
    size += sizeof(double) * 5;
  }

  size = (size_t)blob - (size_t)&_xs;
  nvs_set_blob(my_handle, NONVOL_CALIBRATION_DATA, &_xs, size);
  nvs_set_i32(my_handle, NONVOL_X_OFFSET, (int)(json_x_offset * 1000));
  nvs_set_i32(my_handle, NONVOL_Y_OFFSET, (int)(json_y_offset * 1000));
  nvs_set_i32(my_handle, NONVOL_SENSOR_ANGLE_OFFSET, (int)(json_sensor_angle_offset * 1000));
  nvs_commit(my_handle);

  /*
   * All done, return
   */
  calibration_is_valid = true;
  return;
}

/*----------------------------------------------------------------
 *
 * @function: get_calibration()
 *
 * @brief: Retrieve the calibration data from NONVOL
 *
 * @return: TRUE if settings were retreived
 *
 *----------------------------------------------------------------
 *
 * Pull the calibration data from NONVOL into the spline_points array
 * for use during target processing.
 *
 *--------------------------------------------------------------*/
bool get_calibration(void)
{
  int    i;
  size_t size;
  float *blob;
  float  marker;

  //  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "get_calibration()");))

  calibration_is_valid = false; // Assume false until proven otherwise

  /*
   *  Put the calibration data into _xs
   */
  blob = (double *)&_xs; // Point to a chunk of memory
  nvs_get_blob(my_handle, NONVOL_CALIBRATION_DATA, &blob, &size);
  if ( size != 0 )
  {
    marker = *(blob++);
    for ( i = 0; i != calib_shot_n; i++ )
    {
      spline_points[i].scale        = *(blob++);
      spline_points[i].k0           = *(blob++);
      spline_points[i].k1           = *(blob++);
      spline_points[i].k2           = *(blob++);
      spline_points[i].actual_angle = *(blob++);
    }
    calibration_is_valid = (marker == SPLINE_VALID);
  }

  /*
   * If we get here then there is no calibration data
   */
  return calibration_is_valid;
}

/*----------------------------------------------------------------
 *
 * @function: reset_calibration()
 *
 * @brief: Clear out all of the calibration data
 *
 * @return: Nothing
 *
 *----------------------------------------------------------------
 *
 * Invalidate the existing calibration settings.
 *
 *--------------------------------------------------------------*/
static void void_calibration(void)
{
  float blob;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "clear_calibration()");))

  calibration_is_valid = false; // Assume false until proven otherwise

  blob = 0;
  nvs_set_blob(my_handle, NONVOL_CALIBRATION_DATA, &blob, sizeof(float));
  return;
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
 * Generate a ten point circle with radius of 75mm
 *
 * This should generate a spline fit that is unifor in all
 * directions.
 *
 *--------------------------------------------------------------*/
#define TEST_STEP (TWO_PI / (float)MAX_CALIBRATION_SHOTS) // Increment between steps

void calibration_test(void)
{
  unsigned int i;
  float        theta;

  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "calibration_test()");))

  for ( i = 0; i != MAX_CALIBRATION_SHOTS; i++ )
  {
    theta = TEST_STEP * (float)i;

    record[i].radius = 75.0f;
    record[i].angle  = theta;
    record[i].x_mm   = record[i].radius * cosf(theta);
    record[i].y_mm   = record[i].radius * sinf(theta);

    spline_points[i].actual_x = record[i].x_mm;
    spline_points[i].actual_y = record[i].y_mm;
    spline_points[i].target_x = record[i].x_mm;
    spline_points[i].target_y = record[i].y_mm;

    spline_points[i].actual_rho   = record[i].radius;
    spline_points[i].actual_angle = record[i].angle;

    spline_points[i].target_rho   = record[i].radius;
    spline_points[i].target_angle = record[i].angle;
    spline_points[i].scale        = 1.0f;
    SEND(ALL, sprintf(_xs, "\r\nIndex: %d Angle: %.6f X: %.6f  Y: %6f  Scale: %.6f  ", i, spline_points[i].actual_angle,
                      spline_points[i].target_x, spline_points[i].target_y, spline_points[i].scale);)
  }

  calibration_diag = true;
  shot_in          = MAX_CALIBRATION_SHOTS; // Fake it

  calibrate();

  return;
}
