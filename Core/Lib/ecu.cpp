#include <cmath>
#include <optional>
#include <setjmp.h>

#include "assert.hpp"
#include "util.hpp"
#include "util.h"
#include "can_serde.hpp"
#include "ecu_logic.hpp"
#include "can_buses.h"
#include "main.h"

#include "stm32g4xx_hal_gpio.h"
#include "stm32g4xx_hal_adc.h"
#include "stm32g4xx_hal_tim.h"
#include "stm32g4xx_hal_fdcan.h"

using namespace std;

Ecu ECU = {};

Timer broadcast_build_info_timer(0, BROADCAST_INFO_INTERVAL_MS);
Timer debug_pacing(0, 500);

/* All these timers control how often messages are emitted for each of these sensors. */
Timer throttle1_timer(0, 30);
Timer throttle2_timer(0, 30);
Timer brake_timer(0, 30);
Timer start_switch_timer(0, 30);

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
        /* Check if any sensors need to send values down the CAN bus, and
        * if so, send them. */
        if (throttle1_timer.shouldFire(current_time_ms)) {
            uint16_t reading = HAL_ADC_GetValue(&hadc1);
        }

        if (throttle2_timer.shouldFire(current_time_ms)) {
            uint16_t reading = read_brake();
        }

        // TO DO:Not sure where we are getting brake values, figure that out
        // if (brake_timer.shouldFire(current_time_ms)) {
        //     uint16_t reading = HAL_GPIO_ReadPin();
        // }

        if (start_switch_timer.shouldFire(current_time_ms)) {
            bool reading = HAL_GPIO_ReadPin(SW_1_GPIO_Port, SW_1_Pin) != 0;
        }
        std::optional<CAN_message_t> to_send = ECU.pollCan(current_time_ms);
        if (to_send.has_value()) {
            if (!can_bus_write(&MotorCAN, &(*to_send))){
                // If doesn't send, queue is full
                assert_failed_handler(AssertLevel::Safety, LineInfo{__FILE__, __LINE__}, AssertCode::BadMessage);
            }
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
