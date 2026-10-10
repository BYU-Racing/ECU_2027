#pragma once
#include "can_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

extern CanBus DataCAN;     /* FDCAN2 */
extern CanBus MotorCAN;    /* FDCAN3 */

void can_buses_init(void);

#ifdef __cplusplus
}
#endif