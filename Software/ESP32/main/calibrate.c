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
 * The designation actual or actual. refers to the actul measured value from the target
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
static void         start_calibration(void);                      // First step in the calibration
static void         perform_calibration();                        // Second step in the calibration
static void         find_coeficients(int calib_shot_n);           // Find the spline coeficients
static void         commit_calibration(void);                     // Commit the calibration to NONVOL
static void         verify_calibration(void);                     // Verify the calibration data by applying it to the shots
static void         void_calibration(void);                       // Cancel the existing calibration
static void         target_offset(int number_of_points);          // Adjust the target data for sensor location
static void         spline_sort(int calib_shot_n);                // Sort the entries in order
static void         solve_matrix(real_t a[][4]);                  // Row reduce a matrix to find the solution
static void         read_test_shots(void);                        // Read in test shots for calibration debugging
static unsigned int read_target_scan(void);                       // Read in a scan of the target for calibration
static bool         build_spline_table(void);                     // Build the spline table from the target scan and the shots
static void         report_mean_std_deviation(const char *label); // Report the mean and standard deviation of the calibration data
static void         show_calibration(void);                       // Show the calibration data for debugging

                                                                  /*
                                                                   *  Definitions
                                                                   */

typedef struct data_point
{
  real_t x;                          // X as recorded on the target
  real_t y;                          // Y as recorded on the target
  real_t rho;                        // Radial distance from target
  real_t angle;                      // Angle to shot from target in radians
} data_point_t;

typedef struct spline_point
{
  data_point_t target;               // Target data
  data_point_t actual;               // Actual data

  real_t scale;                      // Scale factor (actual / target) ~1.0
  real_t s0, s1, s2;                 // Spline coeficients
  real_t offset;                     // Angular offset
  real_t o0, o1, o2;                 // Spline coeficients
} spline_point_t;

typedef struct target_scan_entry
{
  real_t x_mm;                       // X location of the shot in mm
  real_t y_mm;                       // Y location of the shot in mm
  real_t rho;                        // Radial distance from center in mm
  real_t angle;                      // Angle to shot from center in radians
} target_scan_entry_t;

#define MAX_CALIBRATION_SHOTS 10     // Maximum number of calibration shots
#define SPLINE_PADDING        2      // Extra points at start and end
#define SPLINE_VALID          1234.0 // Value to show spline data is valid

#define CAL_START      1
#define CAL_PERFORM    (CAL_START + 1)
#define CAL_COMMIT     (CAL_PERFORM + 1)
#define CAL_SHOW       (CAL_COMMIT + 1)
#define CAL_VOID       (CAL_SHOW + 1)
#define CAL_TEST_SHOTS 8

/*
 *  Variables
 */
static spline_point_t      spline_points[MAX_CALIBRATION_SHOTS + SPLINE_PADDING * 2]; // Spline points for calibration
static target_scan_entry_t target_scan[MAX_CALIBRATION_SHOTS * 3];                    // User input (more than it needs to allow for errors)

static int  calib_shot_n;                                                             // Number of shots gathered
static bool calibration_diag;                                                         // TRUE if in test mode

bool calibration_is_valid = false;                                                    // Calibration valid marker

static real_t shot_distance(int i)                                                    // Distance from target to actual for shot i
{
  return sqrtf(SQ(spline_points[i].target.x - spline_points[i].actual.x) + SQ(spline_points[i].target.y - spline_points[i].actual.y));
}

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
void calibrate(int action)
{
  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "calibrate(%d)", action);))

  switch ( action )
  {
    case CAL_START:
      start_calibration();
      return;

    case CAL_PERFORM:
      perform_calibration();
      return;

    case CAL_COMMIT:
      commit_calibration();
      return;

    case CAL_SHOW:
      show_calibration();
      return;

    case CAL_VOID:
      void_calibration();
      return;

    case CAL_TEST_SHOTS:
      read_test_shots();
      return;

    case 9:
      SEND(ALL, sprintf(_xs, "\r\nExiting calibration. No changes made.");)
      return;

    default:
      SEND(ALL, sprintf(_xs, "\r\nTarget Calibration\r\n");)
      SEND(ALL, sprintf(_xs, "\r\n1 - Gather shot data");)
      SEND(ALL, sprintf(_xs, "\r\n2 - Input actual shots");)
      SEND(ALL, sprintf(_xs, "\r\n3 - Commit calibration data to NONVOL");)
      SEND(ALL, sprintf(_xs, "\r\n4 - Display calibration settings");)
      SEND(ALL, sprintf(_xs, "\r\n5 - Void current calibration");)
      SEND(ALL, sprintf(_xs, "\r\n8 - Read in test shots");)
      SEND(ALL, sprintf(_xs, "\r\n9 - Exit, no action\r\n");)
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
  SEND(ALL, sprintf(_xs, "\r\nAction 1 - Gather Shot Data\r\nCalibration data reset\r\n");)

  SEND(ALL, sprintf(_xs, "\r\nUse a pistol target");)
  SEND(ALL, sprintf(_xs, "\r\nFire at least ten shots at random locations around the target.");)
  SEND(ALL, sprintf(_xs, "\r\nWhen all ten shots are fired, proceed to action 2 to enter the actual locations.");)
  SEND(ALL, sprintf(_xs, "\r\n\r\nStart calibration now...\r\n\r\n");)

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
static void perform_calibration()
{
  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "perform_calibration(%d)", shot_in);))

  SEND(ALL, sprintf(_xs, "\r\n");)

  /*
   *  Validate the shots
   */

  if ( shot_in < MAX_CALIBRATION_SHOTS )
  {
    SEND(ALL, sprintf(_xs, "\r\nNot enough shots fired (%d).", shot_in);)
    return;
  }

  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "\r\nPerforming calibration on %d shots\r\n", shot_in);))

  /*
   * Gather actual shot location from target scan
   */
  read_target_scan(); // Read in the target scan data for calibration

  if ( build_spline_table() == false )
  {
    SEND(ALL, sprintf(_xs, "\r\nCalibration failed. Check the input data and try again.");)
    return;
  }

  report_mean_std_deviation("Calibration before uffset");

  /*
   *  Slide the data to match differences in the circuit
   */
  target_offset(MAX_CALIBRATION_SHOTS); // Slide the target coordinates to match the actual
  report_mean_std_deviation("Calibration after offset");

  /*
   *  Sort the data so that the entries are in order
   */
  spline_sort(MAX_CALIBRATION_SHOTS); // Sort the entries into ascending order by angle

  /*
   *  Perform the calibration calculations
   */
  find_coeficients(MAX_CALIBRATION_SHOTS); // Find the coeficients for the spline fits

  /*
   * Check the calibration data
   */
  verify_calibration(); // Apply the calibration to the shots and see if they match the calibration
  report_mean_std_deviation("Calibration after spline");

  SEND(ALL, sprintf(_xs, "\r\n\r\nCalibration complete. ");)

  SEND(ALL, sprintf(_xs, "\r\nCommit the calibration?");)
  if ( prompt_for_confirm() == true )
  {
    commit_calibration(); // Commit the calibration to NONVOL
    SEND(ALL, sprintf(_xs, "\r\nCalibration committed to NONVOL.\r\n ");)
  }
  else
  {
    SEND(ALL, sprintf(_xs, "\r\nCalibration not committed. No changes made.\r\n");)
  }
 
  return;
}
/*----------------------------------------------------------------
 *
 * @function: read_target_scan()
 *
 * @brief: Read the target scan data for calibration
 *
 * @return: Top MAX_CALIBRATION_SHOTS entries of the target scan
 *
 *----------------------------------------------------------------
 *
 * This function reads the input from the "CAL":2 command. It
 * determines how many target_scan entries there are.
 *
 * The target_scan entries are then sorted largest to smallest
 * and the top MAX_CALIBRATION_SIZE entries are kept.
 *
 *--------------------------------------------------------------*/
static unsigned int read_target_scan(void)
{
  int                 target_sample, i, m; // Loop indexes
  target_scan_entry_t temp;

  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "read_target_scan()");))

                                           /*
                                            * Read in the data and save it
                                            */
  target_sample = 0;
  json_find_first();
  while ( 1 )
  {
    if ( json_get_array_next(IS_FLOAT, (void *)&target_scan[target_sample].x_mm) == false )
    {
      break;
    }
    if ( json_get_array_next(IS_FLOAT, (void *)&target_scan[target_sample].y_mm) == false )
    {
      break;
    }
    target_scan[target_sample].rho   = sqrtf(SQ(target_scan[target_sample].x_mm) + SQ(target_scan[target_sample].y_mm));
    target_scan[target_sample].angle = atan2_2PI(target_scan[target_sample].y_mm, target_scan[target_sample].x_mm);

    DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "Read Target Scan %d : X: %4.2f  Y: %4.2f  Rho: %4.2f  Angle: %4.2f", target_sample,
                                           target_scan[target_sample].x_mm, target_scan[target_sample].y_mm, target_scan[target_sample].rho,
                                           target_scan[target_sample].angle);))
    target_sample++;
    if ( target_sample >= sizeof(target_scan) / sizeof(target_scan_entry_t) ) // Check for overflow
    {
      break;
    }
  }

  /*
   *  Sort the data so that the entries are in order of distance from the centre
   */

  /*
   * sort the values of target_sample in ascending order by rho
   */
  for ( i = 0; i < target_sample - 1; i++ )
  {
    for ( m = i + 1; m < target_sample; m++ )
    {
      if ( target_scan[i].rho > target_scan[m].rho )
      {
        temp           = target_scan[i];
        target_scan[i] = target_scan[m];
        target_scan[m] = temp;
      }
    }
  }

  for ( i = 0; i < target_sample / 2; i++ )
  {
    temp                               = target_scan[i];
    target_scan[i]                     = target_scan[target_sample - 1 - i];
    target_scan[target_sample - 1 - i] = temp;
  }

  DLT(DLT_CALIBRATION, {
    printf("\r\nSorted Target Scan by distance from centre\r\n");
    for ( i = 0; i != MAX_CALIBRATION_SHOTS; i++ )
    {
      printf("Target Scan %d : X: %4.2f  Y: %4.2f  Rho: %4.2f  Angle: %4.2f\r\n", i, target_scan[i].x_mm, target_scan[i].y_mm,
             target_scan[i].rho, target_scan[i].angle);
    }
  })

  /*
   * All done, return
   */
  return target_sample;
}

/*----------------------------------------------------------------
 *
 * @function: build_spline_table()
 *
 * @brief: Build the spline table for calibration
 *
 * @return: Spline table built from target scan and shots
 *
 *----------------------------------------------------------------
 *
 * The input data from target Scan is compared against the
 * computed values from the target
 *
 * target_scan Vs. record
 *
 *
 *
 *
 *--------------------------------------------------------------*/
static bool build_spline_table(void)
{
  int    i, n;        // Loop indexes
  int    min_n;       // Shot number with the closest shot to the target scan entry
  real_t distance, d; // Distance from input to shot

  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "build_spline_table()");))

  /*
   * Match up the closest shots to the calibration shots
   */
  for ( i = 0; i != MAX_CALIBRATION_SHOTS + SPLINE_PADDING * 2; i++ ) // Loop on target scan entries and find the closest shot
  {
    distance = 1E6;
    min_n    = 0;
    n        = 0;

    /*
     * Compare shot location against target scan entry and find the closest one
     */
    for ( n = 0; n != shot_in; n++ )
    {
      d = sqrt(SQ(target_scan[i].x_mm - record[n].x_mm) + SQ(target_scan[i].y_mm - record[n].y_mm)); // Distance from target scan to shot

      if ( d < distance )
      {
        distance = d;
        min_n    = n;
      }
    }

    /*
     *  Save the target_scan entries along with the shot record in the spline table for later analysis
     */
    spline_points[i].target.x     = record[min_n].x_mm;  // Target
    spline_points[i].target.y     = record[min_n].y_mm;
    spline_points[i].target.rho   = sqrtf(SQ(record[min_n].x_mm) + SQ(record[min_n].y_mm));
    spline_points[i].target.angle = atan2_2PI(record[min_n].y_mm, record[min_n].x_mm);

    spline_points[i].actual.x     = target_scan[i].x_mm; // Actual from Target Scan
    spline_points[i].actual.y     = target_scan[i].y_mm;
    spline_points[i].actual.rho   = target_scan[i].rho;
    spline_points[i].actual.angle = target_scan[i].angle;

    spline_points[i].scale  = spline_points[i].target.rho / spline_points[i].actual.rho;
    spline_points[i].offset = spline_points[i].target.rho - spline_points[i].actual.rho;
  }

  /*
   * All done, return
   */
  return true;
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
  real_t avg_actual_x, avg_actual_y, avg_target_x, avg_target_y; // Averages
  real_t avg_actual_angle, avg_target_angle;                     // Average angles
  int    i;

  /*
   * Work out the offset between the targets
   */
  avg_actual_x     = 0;
  avg_actual_y     = 0;
  avg_target_x     = 0;
  avg_target_y     = 0;
  avg_target_angle = 0;
  avg_actual_angle = 0;

  for ( i = 0; i < number_of_points; i++ )
  {
    avg_actual_x += spline_points[i].actual.x;
    avg_actual_y += spline_points[i].actual.y;
    avg_actual_angle += spline_points[i].actual.angle;

    avg_target_x += spline_points[i].target.x;
    avg_target_y += spline_points[i].target.y;
    avg_target_angle += spline_points[i].target.angle;
  }

  json_x_offset            = (avg_actual_x - avg_target_x) / number_of_points;
  json_y_offset            = (avg_actual_y - avg_target_y) / number_of_points;
  json_sensor_angle_offset = (avg_actual_angle - avg_target_angle) / number_of_points; // Offset in radians

  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "X Offset: %4.2f  Y Offset: %4.2f  Sensor: %4.2f\r\n", json_x_offset, json_y_offset,
                                         json_sensor_angle_offset);))

  /*
   *  Slide the target loctions slightly to match the actuals
   */
  for ( i = 0; i < number_of_points; i++ )
  {
    spline_points[i].target.x += json_x_offset;
    spline_points[i].target.y += json_y_offset;
    spline_points[i].target.angle += json_sensor_angle_offset; // in Radians
    spline_points[i].offset = spline_points[i].actual.angle - spline_points[i].target.angle;
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
      if ( spline_points[j].actual.angle > spline_points[j + 1].actual.angle )
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
  spline_points[0].actual.angle -= TWO_PI;
  spline_points[1] = spline_points[calib_shot_n - 1 + SPLINE_PADDING];
  spline_points[1].actual.angle -= TWO_PI;

  spline_points[calib_shot_n + 0 + SPLINE_PADDING] = spline_points[3]; // Put the first two points at the end
  spline_points[calib_shot_n + 0 + SPLINE_PADDING].actual.angle += TWO_PI;
  spline_points[calib_shot_n + 1 + SPLINE_PADDING] = spline_points[2];
  spline_points[calib_shot_n + 1 + SPLINE_PADDING].actual.angle += TWO_PI;

  DLT(DLT_CALIBRATION, {
    for ( i = 0; i < calib_shot_n + SPLINE_PADDING * 2; i++ )
    {
      printf("Spline Point %d : Target (%4.2f, %4.2f)  Actual(%4.2f, %4.2f)  Scale: %4.2f  Offset: %4.2f\r\n", i, spline_points[i].target.x,
             spline_points[i].target.y, spline_points[i].actual.x, spline_points[i].actual.y, spline_points[i].scale,
             spline_points[i].offset);
    }
  })

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
#define NX 4         // Number of points in the spline segment
#define NY 3         // Number of equations +1 for augmented matrix

static void find_coeficients(int calib_shot_n)
{
  real_t a[NY][NX];  // Augmented matrix for solving
  int    spline_i;   // Spline point index
  real_t x0, x1, x2; // X variable, (rho  in this case)
  real_t y0, y1, y2; // Y variable (scale in this case

  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "find_coefficients()");))

  /*
   *  Construct the linear system of equations for each segment
   */
  for ( spline_i = 1; spline_i < (calib_shot_n + SPLINE_PADDING * 2); spline_i++ )
  {
    x0 = spline_points[spline_i - 1].actual.rho;
    x1 = spline_points[spline_i].actual.rho;
    x2 = spline_points[spline_i + 1].actual.rho;

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

    solve_matrix(a);
  }

  /*
   *  Store the coefficients for each point
   */
  spline_points[spline_i].s0 = a[0][3];
  spline_points[spline_i].s1 = a[1][3];
  spline_points[spline_i].s2 = a[2][3];

  /*
   *  Repeat for angular iterpolation
   */
  for ( spline_i = 1; spline_i < (calib_shot_n + SPLINE_PADDING * 2); spline_i++ )
  {
    x0 = spline_points[spline_i - 1].actual.rho;
    x1 = spline_points[spline_i].actual.rho;
    x2 = spline_points[spline_i + 1].actual.rho;

    y0 = spline_points[spline_i - 1].offset;
    y1 = spline_points[spline_i].offset;
    y2 = spline_points[spline_i + 1].offset;

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

    solve_matrix(a);
  }

  /*
   *  Store the coefficients for each point
   */
  spline_points[spline_i].o0 = a[0][3];
  spline_points[spline_i].o1 = a[1][3];
  spline_points[spline_i].o2 = a[2][3];

  /*
   * All done, return
   */

  return;
}

/*----------------------------------------------------------------
 *
 * @function: solve_matrix()
 *
 * @brief: Solve the matrix to find the coeffients
 *
 * @return: Coeficients stored fourth column
 *
 *----------------------------------------------------------------
 *
 * Classical row reduction
 *
 *--------------------------------------------------------------*/
static void solve_matrix(real_t a[][NX])
{
  int    i, j, k;
  real_t diag, factor;

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
 * Take the target score we recorded and apply the solution
 * and we should get the actual score seen by the paper.
 *
 *--------------------------------------------------------------*/
static void verify_calibration(void)
{
  int    i;
  real_t angle_offset, scale_factor;

  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "\r\nVerify_calibration()\r\n");))

  for ( i = SPLINE_PADDING; i < MAX_CALIBRATION_SHOTS + SPLINE_PADDING * 2; i++ )
  {
    angle_offset                  = solve_spline_for_angle(spline_points[i].actual.angle);
    spline_points[i].target.angle = spline_points[i].actual.angle + angle_offset;

    scale_factor                = solve_spline_for_scale(spline_points[i].actual.angle);
    spline_points[i].target.rho = spline_points[i].actual.rho * scale_factor;

    spline_points[i].target.x = spline_points[i].target.rho * cosf(spline_points[i].target.angle);
    spline_points[i].target.y = spline_points[i].target.rho * sinf(spline_points[i].target.angle);

    DLT(DLT_CALIBRATION,
        SEND(ALL, sprintf(_xs, "Shot %d : Target (%4.2f, %4.2f)  Actual(%4.2f, %4.2f) Error: %4.2f  Scale: %4.2f  Offset: %4.2f", i,
                          spline_points[i].target.x, spline_points[i].target.y, spline_points[i].actual.x, spline_points[i].actual.y,
                          shot_distance(i), scale_factor, angle_offset);))
  }

  /*
   *  All done, return
   */

  return;
}

/*----------------------------------------------------------------
 *
 * @function: solve_spline_for_angle()
 *
 * @brief: Use the spline coeficients to find the scale factor
 *
 * @return: new angular offset
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
real_t solve_spline_for_angle(real_t angle) // Angle to compute scaling factor
{
  int    s;                                 // segment index
  real_t offset;                            // Angular correction
  real_t x0, x1;                            // X boundaries
  real_t y0;                                // Value of y at x0
  real_t t;                                 // Interval fraction

  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "solve_spline_for_angle(%4.2f)", angle);))

  /*
   *  Return 1.0 if the calibration is not present
   */
  if ( calibration_is_valid == false )
  {
    return 0.0f;
  }

  /*
   *  Find the right segment
   */
  for ( s = SPLINE_PADDING; s < MAX_CALIBRATION_SHOTS + SPLINE_PADDING * 2; s++ )
  {
    if ( (angle >= spline_points[s].actual.angle) && (angle < spline_points[s + 1].actual.angle) )
    {
      break;
    }
  }

  if ( s == MAX_CALIBRATION_SHOTS + SPLINE_PADDING )
  {
    DLT(DLT_CALIBRATION, {
      SEND(ALL, sprintf(_xs, "Spline angle not found %4.2f radians", angle);)
      for ( s = 0; s != MAX_CALIBRATION_SHOTS + SPLINE_PADDING * 2; s++ )
      {
        SEND(ALL, sprintf(_xs, "\r\nSpline Point: %d  Angle: %4.2f", s, spline_points[s].actual.angle);)
      }
    })

    return 0.0f;                          // Not found, return 0
  }

  x0 = spline_points[s].actual.angle;     // Previous point
  x1 = spline_points[s + 1].actual.angle; // next point
  y0 = spline_points[s].offset;

  t      = (angle - x0) / (x1 - x0);      // Interval fraction
  offset = y0 + (spline_points[s].o0 * t) + (spline_points[s].o1 * t * t) + (spline_points[s].o2 * t * t * t);

  /*
   * All done, return the angle offset
   */
  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "Spline angle offset: %4.2f", offset);))
  return offset;
}

/*----------------------------------------------------------------
 *
 * @function: solve_spline_for_scale()
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
real_t solve_spline_for_scale(real_t angle) // Angle to compute scaling factor
{
  int    s;                                 // segment index
  real_t scale;                             // Scale factor
  real_t x0, x1;                            // X boundaries
  real_t y0;                                // Value of y at x0
  real_t t;                                 // Interval fraction

  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "solve_spline_for_scale(%4.2f)", angle);))
  /*
   *  Return 1.0 if the calibration is not present
   */
  if ( calibration_is_valid == false )
  {
    return 1.0f;
  }

  /*
   *  Find the right segment
   */
  for ( s = SPLINE_PADDING; s != MAX_CALIBRATION_SHOTS + SPLINE_PADDING * 2; s++ )
  {
    if ( (angle >= spline_points[s].actual.angle) && (angle < spline_points[s + 1].actual.angle) )
    {
      break;
    }
  }

  if ( s == MAX_CALIBRATION_SHOTS + SPLINE_PADDING * 2 )
  {
    DLT(DLT_CALIBRATION, {
      SEND(ALL, sprintf(_xs, "Spline angle not found %4.2f radians", angle);)
      for ( s = 0; s < MAX_CALIBRATION_SHOTS + SPLINE_PADDING * 2; s++ )
      {
        SEND(ALL, sprintf(_xs, "\r\nSpline Point: %d  Angle: %4.2f", s, spline_points[s].actual.angle);)
      }
    })
    return 1.0f; // Not found, return 1.0
  }

  /*
   * Calculate the scale factor in this segment
   */
  x0 = spline_points[s].actual.angle;     // Previous point
  x1 = spline_points[s + 1].actual.angle; // next point
  y0 = spline_points[s].scale;
  if ( y0 == 0 )
  {
    y0 = 1.0f;
  }

  t     = (angle - x0) / (x1 - x0); // Interval fraction
  scale = y0 + (spline_points[s].s0 * t) + (spline_points[s].s1 * t * t) + (spline_points[s].s2 * t * t * t);

                                    /*
                                     * All done, return the scale factor
                                     */
  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "Spline scale: %4.2f", scale);))
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
  size_t  size; // How many bytes written
  real_t *blob;
  real_t *begin;

  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "commit_calibration()");))

  size      = 0;
  blob      = (real_t *)&_xs; // Point to a chunk of memory
  begin     = blob;
  *(blob++) = SPLINE_VALID;   // Put in a marker

  /*
   *  Put the calibration data into _xs
   */

  for ( i = 0; i != MAX_CALIBRATION_SHOTS + SPLINE_PADDING * 2; i++ )
  {
    *(blob++) = spline_points[i].actual.angle; // 9
    *(blob++) = spline_points[i].scale;        // 1
    *(blob++) = spline_points[i].s0;           // 2
    *(blob++) = spline_points[i].s1;           // 3
    *(blob++) = spline_points[i].s2;           // 4
    *(blob++) = spline_points[i].offset;       // 5
    *(blob++) = spline_points[i].o0;           // 6
    *(blob++) = spline_points[i].o1;           // 7
    *(blob++) = spline_points[i].o2;           // 8
  }

  size = (size_t)blob - (size_t)begin;
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
 * @function: get_target_calibration()
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
bool get_target_calibration(void)
{
  int     i;
  size_t  size;
  real_t *blob;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "get_target_calibration()");))

  calibration_is_valid = false; // Assume false until proven otherwise

  /*
   *  Put the calibration data into _xs
   */
  blob = (real_t *)_xs; // Point to a chunk of memory

  if ( nvs_get_blob(my_handle, NONVOL_CALIBRATION_DATA, blob, &size) != ESP_OK )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Target calibration not retreived");))
    return false;
  }

  if ( size != 0 )
  {
    if ( *(blob) != SPLINE_VALID )
    {
      DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Target calibration not retreived");))
      return false;
    }
    blob++;
    for ( i = 0; i != MAX_CALIBRATION_SHOTS + (SPLINE_PADDING * 2); i++ )
    {
      spline_points[i].actual.angle = *(blob++);
      spline_points[i].scale        = *(blob++);
      spline_points[i].s0           = *(blob++);
      spline_points[i].s1           = *(blob++);
      spline_points[i].s2           = *(blob++);
      spline_points[i].offset       = *(blob++);
      spline_points[i].o0           = *(blob++);
      spline_points[i].o1           = *(blob++);
      spline_points[i].o2           = *(blob++);
    }
  }

  /*
   * Calibration retrieved
   */
  calibration_is_valid = true;
  SEND(ALL, sprintf(_xs, "Target calibration enabled");)
  return true;
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
  real_t blob;

  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "clear_calibration()");))

  SEND(ALL, sprintf(_xs, "Do you want to void the calibration?");)

  if ( prompt_for_confirm() == false )
  {
    return;
  }

  calibration_is_valid = false; // Assume false until proven otherwise

  blob = 0;
  nvs_set_blob(my_handle, NONVOL_CALIBRATION_DATA, &blob, sizeof(real_t));
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
#define TEST_STEP (TWO_PI / (real_t)MAX_CALIBRATION_SHOTS) // Increment between steps

void calibration_test(void)
{
  unsigned int i;
  real_t       theta;

  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "calibration_test()");))

  for ( i = 0; i != MAX_CALIBRATION_SHOTS; i++ )
  {
    theta = TEST_STEP * (real_t)i;

    record[i].radius = 75.0f;
    record[i].angle  = theta;
    record[i].x_mm   = record[i].radius * cosf(theta);
    record[i].y_mm   = record[i].radius * sinf(theta);

    spline_points[i].actual.x = record[i].x_mm;
    spline_points[i].actual.y = record[i].y_mm;
    spline_points[i].target.x = record[i].x_mm;
    spline_points[i].target.y = record[i].y_mm;

    spline_points[i].actual.rho   = record[i].radius;
    spline_points[i].actual.angle = record[i].angle;

    spline_points[i].target.rho   = record[i].radius;
    spline_points[i].target.angle = record[i].angle;
    spline_points[i].scale        = 1.0f;
    DLT(DLT_CALIBRATION,
        SEND(ALL, sprintf(_xs, "\r\nIndex: %d Angle: %.6f X: %.6f  Y: %6f  Scale: %.6f  ", i, spline_points[i].actual.angle,
                          spline_points[i].target.x, spline_points[i].target.y, spline_points[i].scale);))
  }

  calibration_diag = true;

  calibrate(CAL_PERFORM);

  return;
}

/*----------------------------------------------------------------
 *
 * @function: read_shots_for_calibration()
 *
 * @brief: Create a test calibration dataset
 *   * @return: None
 *
 *----------------------------------------------------------------
 *
 * Generate a ten point circle with radius of 75mm
 *
 * This should generate a spline fit that is unifor in all
 * directions.
 *
 *--------------------------------------------------------------*/
void read_test_shots(void)
{
  real_t actual_x, actual_y; // User input

  json_find_first();         // Find the start of the input list

  for ( calib_shot_n = 0; calib_shot_n != SHOT_SPACE; calib_shot_n++ )
  {
    if ( (json_get_array_next(IS_FLOAT, (void *)&actual_x) && json_get_array_next(IS_FLOAT, &actual_y)) == false )
    {
      break;
    }

    record[calib_shot_n].shot   = calib_shot_n; // Fill in the shot record with the actual values
    record[calib_shot_n].x_mm   = actual_x;
    record[calib_shot_n].y_mm   = actual_y;
    record[calib_shot_n].radius = sqrt(SQ(actual_x) + SQ(actual_y));
    record[calib_shot_n].angle  = atan2_2PI(actual_y, actual_x);
  }

  /*
   *  Finished loading data
   */
  SEND(ALL, sprintf(_xs, "\r\nRead %d shots for calibration\r\n", calib_shot_n + 1);)
  shot_in  = calib_shot_n + 1;
  shot_out = shot_in;
  return;
}

/*----------------------------------------------------------------
 *
 * @function: report_mean_std_deviation()
 *
 * @brief: Report the mean and standard deviation of the calibration data
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
static void report_mean_std_deviation(const char *label)
{
  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "report_mean_std_deviation()");))

  real_t sum_error, mean_error;       // Mean error
  real_t sum_deviation, stddev_error; // Standard deviation of error
  int    i;

  /*
   *  Calculate the mean
   */
  sum_error = 0.0f;
  for ( i = 0; i != MAX_CALIBRATION_SHOTS; i++ )
  {
    sum_error += shot_distance(i + SPLINE_PADDING);
  }
  mean_error = sum_error / (real_t)(MAX_CALIBRATION_SHOTS);

  /*
   *  Calculate the standard deviation
   */
  sum_deviation = 0.0f;
  for ( i = 0; i != MAX_CALIBRATION_SHOTS; i++ )
  {
    sum_deviation += SQ(shot_distance(i + SPLINE_PADDING) - mean_error); // Square of the error
  }
  stddev_error = sqrt(sum_deviation / (real_t)(MAX_CALIBRATION_SHOTS));

  SEND(ALL, sprintf(_xs, "\r\n%s: Mean Error: %4.2fmm   Std Dev: %4.2fmm\r\n", label, mean_error, stddev_error);)

  /*
   *  All done, return
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: show_calibration()
 *
 * @brief: Display the calibration settings from NONVOL
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * Display the settings so they can be verifieed.
 *
 *--------------------------------------------------------------*/
static void show_calibration(void)
{
  int i;

  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "show_calibration()");))

  for ( i = 0; i != MAX_CALIBRATION_SHOTS + SPLINE_PADDING * 2; i++ )
  {
    SEND(ALL, sprintf(_xs, "\r\n\r\nSpline: %d  Angle: %4.2f", i, spline_points[i].actual.angle);)
    SEND(ALL, sprintf(_xs, "\r\nScale: %4.2f  s0:%6.4f  s1:%6.4f  s2:%6.4f", spline_points[i].scale, spline_points[i].s0,
                      spline_points[i].s1, spline_points[i].s2);)
    SEND(ALL, sprintf(_xs, "\r\nOffset: %4.2f o0:%4.2f  o1:%4.2f  o2:%4.2f", spline_points[i].offset, spline_points[i].o0,
                      spline_points[i].o1, spline_points[i].o2);)
  }

  /*
   * All done, return
   */
  return;
}