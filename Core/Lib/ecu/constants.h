#pragma once

#include <stdint.h>


/* ASSERTS */
#define SOFT_RESET_LENGTH_MS 2000

/* COMMUNICATIONS */
// TO DO: Not sure about how to implement serial stuff yet
//#define SERIAL_BAUD_RATE 115200
#define CAN_BAUD_RATE 250000

/* Interval between broadcasting the system build info. */
#define BROADCAST_INFO_INTERVAL_MS 1000


/* THROTTLE CONSTANTS */

/* How long we tolerate between receiving pedal values until we panic. */
#define PEDAL_TIMEOUT_MS 200

//TO DO: double check this, 60ms should be fine but without the DCF it could be longer
/* How long an implausibility is allowed to last before panicking.
 * Why 60 ms, and not 100 ms, like the rules say? Because the DCF
 * only generates messages every 30 ms, so we could receive data
 * that's 30 ms old. 30 ms + 60 ms  90 ms, which gives us another
 * 10 ms of wiggle room for the code to run. See the 2026 rules,
 * section T.4.2.5 */
#define ALLOWED_IMPLAUSILIBTY_LENGTH_MS 60


#define THROTTLE1_LOW 660
#define THROTTLE1_HIGH 950
/* If the throttle 1 values is lower or higher than this range, we panic.
 * Used to comply with T.4.2.10, 2026 rules. */
#define THROTTLE1_MIN_OUT_OF_RANGE 620
#define THROTTLE1_MAX_OUT_OF_RANGE 970

/* If outside this range, the throttle will clamp to 0%-100%. */
#define THROTTLE2_LOW 430
#define THROTTLE2_HIGH 650
/* If the throttle 2 value is lower or higher than this range, we panic.
 * Used to comply with T.4.2.10, 2026 rules. */
#define THROTTLE2_MIN_OUT_OF_RANGE 350
#define THROTTLE2_MAX_OUT_OF_RANGE 725

/* Maximum allowed difference between the two throttles' values,
 * after those values have been mapped to [THROTTLE[X]_MIN, THROTTLE[X]_MAX]. */
#define THROTTLE_DISAGREE 10

#define MIN_THROTTLE 0 /* In CASCADIA format, 1  0.1Nm */
#define MAX_THROTTLE 100 /* In CASCADIA format, 1  0.1Nm */
#define SMOOTH_PERIOD_MS  30 /* How often the input is smoothed */
#define PID_PERIOD_MS  10    /* How often the PID is updated */


/* BRAKES */
/* The reported brake level should never be lower than this, or it is considered
 * an unrecoverable fault. */
#define BRAKE_PRESSURE_MIN 10

/* The amount that the brake needs to be pressed down before it's considered
 * pressed by the system. */
#define BRAKE_CONSIDERED_PRESSED 20

/* This delay is where the horn is played */
#define STARTUP_DELAY_MS 2000  /* 2 seconds */

#define TORQUE_FLOOR 10

/* MAX TORQUE in CASCADIA format, 1  0.1Nm */
#define MAX_TORQUE 1000
 
/* Drive Modes */
#define DRIVE_MODE 0
