#include "assert.h"
#include <stddef.h>

/* Keep the current handler private to this source file. */
static assert_failed_handler_t current_handler = NULL;

void register_assert_failed_handler(assert_failed_handler_t handler)
{
    current_handler = handler;
}

void assert_failed(
    AssertLevel level,
    LineInfo line_info,
    AssertCode error_code
)
{
    if (current_handler != NULL) {
        current_handler(level, line_info, error_code);
    }
}
