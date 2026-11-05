#include "can/can_bus.h"
#include "can/can_buses.h"
#include "can/CAN_message_t.h"
#include "can/can_serde.h"

CanBus test_bus;
FDCAN_HandleTypeDef test_handle;

void test_can_bus_init(void) {
    can_bus_init(&test_bus, &test_handle);

    ASSERT_EQUAL_PTR(test_bus.hfdcan, &test_handle);
    ASSERT_EQUAL_INT(test_bus.rx_head, 0);
    ASSERT_EQUAL_INT(test_bus.rx_tail, 0);
    ASSERT_EQUAL_INT(test_bus.tx_head, 0);
    ASSERT_EQUAL_INT(test_bus.tx_tail, 0);
    ASSERT_EQUAL_INT(test_bus.rx_overflow, 0);
}


void test_can_bus_start(void) {
    ASSERT_TRUE(can_bus_start(&test_bus));
}

void test_can_bus_write_and_read(void) {
    CAN_message_t msg_to_send = empty_can_message(0x123, 8);

    ASSERT_TRUE(can_bus_write(&test_bus, &msg_to_send));
    ASSERT_EQUAL_INT(test_bus.tx_head, 1);
    ASSERT_EQUAL_INT(test_bus.tx_tail, 0);
    ASSERT_EQUAL_INT(test_bus.tx_buf[0].id, 0x123);

    CAN_message_t msg_received;
    ASSERT_TRUE(can_bus_read(&test_bus, &msg_received));
}


int main(void) {
    RUN_TEST(test_can_bus_init);
    RUN_TEST(test_can_bus_start);

    TEST_SUMMARY();

    return 0;
}