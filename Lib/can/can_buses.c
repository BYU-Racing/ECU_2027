#include "can_buses.h"
<<<<<<< HEAD
#include "stm32g4xx_hal_fdcan.h"
=======
>>>>>>> e2076087a2a2da198c37c586714f03828df2b6d3

extern FDCAN_HandleTypeDef hfdcan2, hfdcan3;

CanBus DataCAN;
CanBus MotorCAN;

void can_buses_init(void) {
    can_bus_init(&DataCAN, &hfdcan2);
    can_bus_init(&MotorCAN, &hfdcan3);
<<<<<<< HEAD
    if (can_bus_start(&DataCAN) != HAL_OK) {
        Error_Handler();
    }
    if (can_bus_start(&MotorCAN) != HAL_OK) {
=======
    if (!can_bus_start(&DataCAN)) {
        Error_Handler();
    }
    if (!can_bus_start(&MotorCAN)) {
>>>>>>> e2076087a2a2da198c37c586714f03828df2b6d3
        Error_Handler();
    }
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