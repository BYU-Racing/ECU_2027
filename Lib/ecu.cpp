#include <cmath>
#include <optional>
#include <setjmp.h>

#include "assert.hpp"
#include "util.hpp"
#include "can_serde.hpp"
#include "ecu_logic.hpp"
<<<<<<< HEAD
=======
#include "can_buses.h"
#include "main.h"
>>>>>>> e2076087a2a2da198c37c586714f03828df2b6d3

#include "stm32g4xx_hal_gpio.h"
#include "stm32g4xx_hal_adc.h"
#include "stm32g4xx_hal_tim.h"
#include "stm32g4xx_hal_fdcan.h"
<<<<<<< HEAD
#include "stm32g4xx_hal.c"

using namespace std;

FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> DataCAN;
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> MotorCAN;

=======

using namespace std;

>>>>>>> e2076087a2a2da198c37c586714f03828df2b6d3
Ecu ECU = {};

Timer broadcast_build_info_timer(0, BROADCAST_INFO_INTERVAL_MS);
Timer debug_pacing(0, 500);

<<<<<<< HEAD
=======
/* All these timers control how often messages are emitted for each of these sensors. */
Timer throttle1_timer(0, 30);
Timer throttle2_timer(0, 30);
Timer brake_timer(0, 30);
Timer start_switch_timer(0, 30);

>>>>>>> e2076087a2a2da198c37c586714f03828df2b6d3
void assert_failed_handler(AssertLevel level, LineInfo info, AssertCode error_code) {
    /* Shut everything down. */
    CAN_message_t shutdown_message = empty_can_message(MessageId::ControlCommand, 8);
    /* `empty_can_message` is guaranteed to generate a message full of zeroes,
    * and since this is panic wind down code, it's best to keep it as simple as
    * possible, so we don't use the normal message creation function. */
<<<<<<< HEAD
    MotorCAN.write(shutdown_message);

    /* Loop forever so we never do anything after the panic. */
    while (true) {
        Serial.printf("Safety assertion failed! In file %s:%d with error code %d\n", info.filename, info.line_no, error_code);
        Serial.printf("File hash %lu\n", (unsigned long) str_hash(info.filename));
=======
    can_bus_write(&MotorCAN, &shutdown_message);

    /* Loop forever so we never do anything after the panic. */
    while (true) {
        // TO DO: Fix serial connection so we can print out the error code and file name. Currently, this doesn't work.
        // Serial.printf("Safety assertion failed! In file %s:%d with error code %d\n", info.filename, info.line_no, error_code);
        // Serial.printf("File hash %lu\n", (unsigned long) str_hash(info.filename));
>>>>>>> e2076087a2a2da198c37c586714f03828df2b6d3

        CriticalFault fault_msg;
        fault_msg.error_code = error_code;
        fault_msg.assert_failure_line = info.line_no;
        fault_msg.file_name_hash = str_hash(info.filename);

<<<<<<< HEAD
        MotorCAN.write(create_critical_fault_command(fault_msg));

        pinMode(13, OUTPUT);
        digitalWrite(13, HIGH);
        delay(500);
        digitalWrite(13, LOW);
        delay(500);
    }
}

void setup() {
=======
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
>>>>>>> e2076087a2a2da198c37c586714f03828df2b6d3
  // First things first, register the panic handler. If something goes
    // wrong during setup, we'll wind everything down.
    register_assert_failed_handler(assert_failed_handler);

<<<<<<< HEAD
    Serial.begin(SERIAL_BAUD_RATE);

    MotorCAN.begin();
    MotorCAN.setBaudRate(CAN_BAUD_RATE);
    DataCAN.begin();
    DataCAN.setBaudRate(CAN_BAUD_RATE);

    pinMode(HORN_PIN, OUTPUT);
    pinMode(BRAKE_LIGHT_PIN, OUTPUT);

    Serial.println("============================================");
    Serial.println("==========Motor CAN initialized=============");
    Serial.println("============================================");
}

void loop() {
=======
    // TO DO: Get serial connection working for debugging.
    // Serial.begin(SERIAL_BAUD_RATE);

    // Serial.println("============================================");
    // Serial.println("==========Motor CAN initialized=============");
    // Serial.println("============================================");
}

void ecu_loop() {
>>>>>>> e2076087a2a2da198c37c586714f03828df2b6d3
    /* The ECU does not keep track of what time it is, nor does it use `millis`,
     * so we always have to tell it what time it is. */
    uint32_t current_time_ms = HAL_GetTick();

    /* Receive a message and have the ECU process it. */
    CAN_message_t rmsg;
<<<<<<< HEAD
    if (MotorCAN.read(rmsg)) {
=======
    if (can_bus_read(&MotorCAN, &rmsg)) {
>>>>>>> e2076087a2a2da198c37c586714f03828df2b6d3
        ECU.processMessage(current_time_ms, rmsg);
    }

    /* Generate all outgoing messages and send each. */
    while (true) {
<<<<<<< HEAD
        std::optional<CAN_message_t> to_send = ECU.pollCan(current_time_ms);
        if (to_send.has_value()) {
            MotorCAN.write(*to_send);
=======
        /* Check if any sensors need to send values down the CAN bus, and
        * if so, send them. */
        if (throttle1_timer.shouldFire(current_time_ms)) {
            uint16_t reading = HAL_ADC_GetValue(&hadc1);
        }

        if (throttle2_timer.shouldFire(current_time_ms)) {
            uint16_t reading = HAL_ADC_GetValue();
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
>>>>>>> e2076087a2a2da198c37c586714f03828df2b6d3
        } else {
            break;
        }
    }

<<<<<<< HEAD
    auto state = ECU.pollGpioState(current_time_ms);
    digitalWrite(HORN_PIN, state.horn_on);
    digitalWrite(BRAKE_LIGHT_PIN, state.brake_light_on);

    if (broadcast_build_info_timer.shouldFire(current_time_ms)) {
        MotorCAN.write(create_code_hash_message(GIT_COMMIT_HASH_U64));
        MotorCAN.write(create_commit_author_message(GIT_COMMIT_AUTHOR));
        MotorCAN.write(create_uploader_message(GIT_UPLOADER));
    }
=======

    auto state = ECU.pollGpioState(current_time_ms);
    HAL_GPIO_WritePin(GPIOA, Horn_IO_Pin, state.horn_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, Brake_Light_IO_Pin, state.brake_light_on ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // TO DO: Added to broadcast who flashed code to the ECU. Currently doens't work.
    // if (broadcast_build_info_timer.shouldFire(current_time_ms)) {
    //     can_bus_write(&MotorCAN, &create_code_hash_message(GIT_COMMIT_HASH_U64));
    //     can_bus_write(&MotorCAN, &create_commit_author_message(GIT_COMMIT_AUTHOR));
    //     can_bus_write(&MotorCAN, &create_uploader_message(GIT_UPLOADER));
    // }
>>>>>>> e2076087a2a2da198c37c586714f03828df2b6d3

#ifdef ENABLE_DEBUGGING
    if (debug_pacing.shouldFire(current_time_ms)) {
        ECU.printState();
    }
#endif
}
