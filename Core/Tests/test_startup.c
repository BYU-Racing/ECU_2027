/* tests/test_startup.c: link ecu_logic.c, can_serde.c, ecu_assert.c, util (no ADC) */
#include <assert.h>
#include <stdlib.h>
#include "ecu_logic.h"
#include "can_serde.h"
#include "constants.h"
#include "util.h"

static void on_fail(AssertLevel l, LineInfo li, AssertCode c) {
    printf("ASSERT %d at %s:%d\n", c, li.filename, li.line_no);
    exit(1);
}

int main(void) {
    ecu_register_assert_failed_handler(on_fail);

    CAN_message_t vsm = { .id = 170, .len = 8 };
    vsm.buf[0] = 3;                         /* PRE_CHARGE_COMPLETE */
    ecu_set_start_switch();

    CAN_message_t out;
    bool enabled = false;
    for (uint32_t t = 1000; t < 4000; t += 10) {
        ecu_process_message(t, vsm);
        pedals_input_throttle_one_position(t, THROTTLE1_LOW);   /* 0% */
        pedals_input_throttle_two_position(t, THROTTLE2_LOW);
        pedals_input_brake_position(t, 200);                    /* brake down */
        ecu_update(t);
        while (ecu_poll_can(t, &out)) enabled = (out.buf[5] & 1);
    }
    assert(enabled);        /* inverter enable bit set after the 2 s countdown */
    return 0;
}