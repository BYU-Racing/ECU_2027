#include <cmath>
#include <optional>
#include <setjmp.h>

#include "assert.hpp"
#include "util.hpp"
#include "can_serde.hpp"
#include "ecu_logic.hpp"
#include "can_buses.h"

#include "stm32g4xx_hal_gpio.h"
#include "stm32g4xx_hal_adc.h"
#include "stm32g4xx_hal_tim.h"
#include "stm32g4xx_hal_fdcan.h"

using namespace std;

Ecu ECU = {};

Timer broadcast_build_info_timer(0, BROADCAST_INFO_INTERVAL_MS);
Timer debug_pacing(0, 500);

void assert_failed_handler(AssertLevel level, LineInfo info, AssertCode error_code) {
    /* Shut everything down. */
    CAN_message_t shutdown_message = empty_can_message(MessageId::ControlCommand, 8);
    /* `empty_can_message` is guaranteed to generate a message full of zeroes,
    * and since this is panic wind down code, it's best to keep it as simple as
    * possible, so we don't use the normal message creation function. */
    can_bus_write(&MotorCAN, &shutdown_message);

    /* Loop forever so we never do anything after the panic. */
    while (true) {
        // TO DO: Fix serial connection so we can print out the error code and file name. Currently, this doesn't work.
        // Serial.printf("Safety assertion failed! In file %s:%d with error code %d\n", info.filename, info.line_no, error_code);
        // Serial.printf("File hash %lu\n", (unsigned long) str_hash(info.filename));

        CriticalFault fault_msg;
        fault_msg.error_code = error_code;
        fault_msg.assert_failure_line = info.line_no;
        fault_msg.file_name_hash = str_hash(info.filename);

        can_bus_write(&MotorCAN, &create_critical_fault_command(fault_msg));

        // Blinks the onboard LED, might not work with ECU board.
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(500);
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(500);
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(500);
    }
}

void ecu_setup() {
  // First things first, register the panic handler. If something goes
    // wrong during setup, we'll wind everything down.
    register_assert_failed_handler(assert_failed_handler);

    // TO DO: Get serial connection working for debugging.
    // Serial.begin(SERIAL_BAUD_RATE);

    // Serial.println("============================================");
    // Serial.println("==========Motor CAN initialized=============");
    // Serial.println("============================================");
}

void ecu_loop() {
    /* The ECU does not keep track of what time it is, nor does it use `millis`,
     * so we always have to tell it what time it is. */
    uint32_t current_time_ms = HAL_GetTick();

    /* Receive a message and have the ECU process it. */
    CAN_message_t rmsg;
    if (can_bus_read(&MotorCAN, &rmsg)) {
        ECU.processMessage(current_time_ms, rmsg);
    }

    /* Generate all outgoing messages and send each. */
    while (true) {
        std::optional<CAN_message_t> to_send = ECU.pollCan(current_time_ms);
        if (to_send.has_value()) {
            can_bus_write(&MotorCAN, &(*to_send));
        } else {
            break;
        }
    }


    auto state = ECU.pollGpioState(current_time_ms);
    HAL_GPIO_WritePin(GPIOA, Horn_IO_Pin, state.horn_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, Brake_Light_IO_Pin, state.brake_light_on ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // TO DO: Added to broadcast who flashed code to the ECU. Currently doens't work.
    // if (broadcast_build_info_timer.shouldFire(current_time_ms)) {
    //     can_bus_write(&MotorCAN, &create_code_hash_message(GIT_COMMIT_HASH_U64));
    //     can_bus_write(&MotorCAN, &create_commit_author_message(GIT_COMMIT_AUTHOR));
    //     can_bus_write(&MotorCAN, &create_uploader_message(GIT_UPLOADER));
    // }

#ifdef ENABLE_DEBUGGING
    if (debug_pacing.shouldFire(current_time_ms)) {
        ECU.printState();
    }
#endif
}
