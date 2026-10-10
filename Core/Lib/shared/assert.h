/*
 * Assertion and panic handling for the ECU.
 *
 * Assertions call the registered failure handler when their condition is false.
 * The handler decides how to respond to soft and safety-critical failures.
 */
#ifndef PROJECT_ASSERT_H
#define PROJECT_ASSERT_H

#include <stdint.h>

/* Assertion and panic handling code. This allows any part of the code to use assertions.
 * What's an assertion? Let's look at an example.
 * Say there's a temperature sensor that should never exceed 90°C.
 * You could write code like
 * 
 *   uint16_t temperature = read_temperature();
 *   SAFETY_ASSERT(temperature <= 90);
 * 
 * If the temperature ever reaches 91°C, it will call `assert_failed`, which runs
 * whatever code the user has for handling failed asserts. This makes it really
 * easy to check if values are out of range, or certain preconditions must always
 * be met.
 * 
 * There's one other important thing to know: there are multiple different levels
 * of asserts, each of which the user can choose what to do if it fails. Here are
 * some of them:
 *  - `AssertLevel::Soft` is for minor state checks. If a soft assert fails, it means
 *    that nothing dangerous has happened, just some minor check failed. This will do
 *    a soft reset of the system, where it shuts off the engine for a few seconds
 *    before resuming as normal.
 *  - `AssertLevel::Safety` is for safety-critical checks. If a safety assert fails
 *    the car goes into a unrecoverable shutdown, and the whole thing will need to
 *    be reset.
 *  */

/* Error codes for safety assertions.
 * Zero is intentionally unused because it represents "no error" on the screen.
 */
typedef enum {
    ASSERT_CODE_UNKNOWN = 1,
    ASSERT_CODE_BAD_MESSAGE = 2,
    ASSERT_CODE_INTEGER_OVERFLOW = 3,
    /* Codes 3-20 are reserved for future general messages. */
    ASSERT_CODE_BRAKE_AND_THROTTLE_PRESSED = 20,
    ASSERT_CODE_THROTTLE_OUT_OF_RANGE = 21,
    ASSERT_CODE_THROTTLE_SENSORS_DIVERGED = 22,
    /* T.4.2.11(b), 2026 rules. */
    ASSERT_CODE_PEDAL_TIMEOUT = 23
} AssertCode;

/* Captures the filename and line number of the assertion call. */
typedef struct {
    const char *filename;
    int line_no;
} LineInfo;

#define CAPTURE_LINE_INFO() ((LineInfo){__FILE__, __LINE__})

/* Soft assertions are for minor state checks.
 * Production builds may elevate these to safety assertions.
 */
typedef enum {
    ASSERT_LEVEL_SOFT = 0,
    ASSERT_LEVEL_SAFETY = 1
} AssertLevel;

/* Function type for the assertion failure handler. */
typedef void (*assert_failed_handler_t)(
    AssertLevel level,
    LineInfo line_info,
    AssertCode error_code
);

/* Register the function called when an assertion fails. */
void register_assert_failed_handler(assert_failed_handler_t handler);

/* Call the registered handler, if one has been registered. */
void assert_failed(
    AssertLevel level,
    LineInfo line_info,
    AssertCode error_code
);

#define GENERIC_ASSERT(level, condition, code) \
    do { \
        if (!(condition)) { \
            assert_failed((level), CAPTURE_LINE_INFO(), (code)); \
        } \
    } while (0)

#define SOFT_ASSERT(condition, code) \
    GENERIC_ASSERT(ASSERT_LEVEL_SOFT, (condition), (code))

#define SAFETY_ASSERT(condition, code) \
    GENERIC_ASSERT(ASSERT_LEVEL_SAFETY, (condition), (code))

#endif /* PROJECT_ASSERT_H */
