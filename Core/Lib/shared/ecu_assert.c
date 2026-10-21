#include "ecu_assert.h"
#include <stddef.h>

static void ecu_default_handler(AssertLevel level, LineInfo line_info, AssertCode error_code){
    (void)level;
    (void)line_info;
    (void)error_code;
}

static ecu_assert_failed_handler_t ecu_current_handler = ecu_default_handler;

void ecu_register_assert_failed_handler(ecu_assert_failed_handler_t handler)
{
    if (handler != NULL) {
        ecu_current_handler = handler;
    }
}

void ecu_assert_failed( AssertLevel level, LineInfo line_info, AssertCode error_code){
    ecu_current_handler(level, line_info, error_code);

    if (level == ASSERT_LEVEL_SAFETY) {
        for (;;) {
            // TO DO: currently a latched assert handler, find a better way
        }
    }
}
