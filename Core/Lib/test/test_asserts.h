#ifndef TEST_ASSERTS_H
#define TEST_ASSERTS_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static int test_asserts_run_count = 0;
static int test_asserts_fail_count = 0;
static int current_test_fail = 0;

#define ASSERT_TRUE(cond)                                                                                               \
    do {                                                                                                                \
        if (!(cond)) {                                                                                                  \
            printf("ASSERTION FAILED: %s:%d - Condition is false\n", __FILE__, __LINE__);                               \
            current_test_fail = 1;                                                                                      \
            return;                                                                                                     \
        }                                                                                                               \
    } while (0)

#define ASSERT_FALSE(cond)                                                                                              \
    do {                                                                                                                \
        if (cond) {                                                                                                     \
            printf("ASSERTION FAILED: %s:%d - Condition is true\n", __FILE__, __LINE__);                                \
            current_test_fail = 1;                                                                                      \
            return;                                                                                                     \
        }                                                                                                               \
    } while (0)

#define ASSERT_COND(cond)                                                                                               \
    do {                                                                                                                \
        if (!(cond)) {                                                                                                  \
            printf("ASSERTION FAILED: %s:%d\n", __FILE__, __LINE__);                                                    \
            current_test_fail = 1;                                                                                      \
            return;                                                                                                     \
        }                                                                                                               \
    } while (0)

#define ASSERT_EQUAL_INT(expected, actual)                                                                              \
    do {                                                                                                                \
        if ((expected) != (actual)) {                                                                                   \
            printf("ASSERTION FAILED: %s:%d - Expected %d, got %d\n", __FILE__, __LINE__, (expected), (actual));        \
            current_test_fail = 1;                                                                                      \
            return;                                                                                                     \
        }                                                                                                               \
    } while (0)

#define ASSERT_EQUAL_STR(expected, actual)                                                                              \
    do {                                                                                                                \
        if (strcmp((expected), (actual)) != 0) {                                                                        \
            printf("ASSERTION FAILED: %s:%d - Expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, (expected), (actual));\
            current_test_fail = 1;                                                                                      \
            return;                                                                                                     \
        }                                                                                                               \
    } while (0)

#define ASSERT_EQUAL_PTR(expected, actual)                                                                              \
    do {                                                                                                                \
        if ((expected) != (actual)) {                                                                                   \
            printf("ASSERTION FAILED: %s:%d - Expected pointer %p, got %p\n", __FILE__, __LINE__, (expected), (actual));\
            current_test_fail = 1;                                                                                      \
            return;                                                                                                     \
        }                                                                                                               \
    } while (0)

#define ASSERT_EQUAL_ARRAY(expected, actual, size)                                                                      \
    do {                                                                                                                \
        if (memcmp((expected), (actual), (size)) != 0) {                                                                \
            printf("ASSERTION FAILED: %s:%d - Arrays differ\n", __FILE__, __LINE__);                                    \
            current_test_fail = 1;                                                                                      \
            return;                                                                                                     \
        }                                                                                                               \
    } while (0)

#define RUN_TEST(test_func)                                                                                             \
    do {                                                                                                                \
        current_test_fail = 0;                                                                                          \
        test_asserts_run_count++;                                                                                       \
        test_func();                                                                                                    \
        if (current_test_fail) {                                                                                        \
            printf("[FAILED] %s\n", #test_func);                                                                        \
            test_asserts_fail_count++;                                                                                  \
        } else {                                                                                                        \
            printf("[PASSED] %s\n", #test_func);                                                                        \
        }                                                                                                               \
    } while (0)

#define TEST_SUMMARY()                                                                                                  \
    do {                                                                                                                \
        if (test_asserts_run_count > 0) {                                                                               \
            printf("%.2f%% Passed, Tests run: %d, Failed tests: %d\n",                                                  \
                        (double)(test_asserts_run_count - test_asserts_fail_count) / test_asserts_run_count * 100.0,    \
                        test_asserts_run_count,                                                                         \
                        test_asserts_fail_count);                                                                       \
        } else {                                                                                                        \
            printf("No tests run.\n");                                                                                  \
        }                                                                                                               \
    } while (0)

#endif // TEST_ASSERTS_H