#include "can_buses.h"
#include "can_serde.h"

extern FDCAN_HandleTypeDef hfdcan2, hfdcan3;

CanBus DataCAN;
CanBus MotorCAN;

#ifdef __cplusplus
extern "C" {
#endif

void can_buses_init(void) {
    can_bus_init(&DataCAN, &hfdcan2);
    can_bus_init(&MotorCAN, &hfdcan3);
    if (!can_bus_start(&DataCAN)) {
        Error_Handler();
    }
    if (!can_bus_start(&MotorCAN)) {
        Error_Handler();
    }
}

void can_buses_send_shutdown(void) {
    /* Don't touch the bus if it was never started (Error_Handler can fire
     * during early init, before can_buses_init() has run). */
    if (MotorCAN.hfdcan == NULL || MotorCAN.hfdcan->State != HAL_FDCAN_STATE_BUSY) {
        return;
    }

    /* Shut everything down. */
    uint8_t control_command_message_length = 8;

    CAN_message_t message = {0};
    message.id  = MessageId.ControlCommand;   /* MessageId::ControlCommand: all zeroes = zero torque, inverter off */
    message.len = control_command_message_length;

    can_bus_write(&MotorCAN, &message);
    can_bus_service(&MotorCAN);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
    // If the callback was called for a reason other than a new message, we don't care.
    if (!(RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE)) {
        return;
    }

    // Determine which bus the interrupt was for and call the appropriate handler.
    if (hfdcan == DataCAN.hfdcan) {
        can_bus_on_rx_interrupt(&DataCAN);
    } else if (hfdcan == MotorCAN.hfdcan) {
        can_bus_on_rx_interrupt(&MotorCAN);
    }
}

#ifdef __cplusplus
}
#endif