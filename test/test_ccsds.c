#include <unity.h>
#include <string.h>
#include "ccsds_packet.h"
#include "time_service.h"

void setUp(void) {
    TIME_Init(); // Reset mission time to 0
}

void tearDown(void) {}

void test_CCSDS_Wrap_Header_Logic(void) {
    uint8_t buffer[128];
    uint8_t dummy_payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint16_t apid = 0x010; // ADCS

    // Action: Wrap the packet
    CCSDS_WrapTelemetry(apid, dummy_payload, 4, buffer);

    // 1. Verify Packet ID (Byte 0 and 1)
    // Expected binary: 000 (Ver) 1 (Type) 1 (SecHdr) 00000010000 (APID 0x10)
    // Hex: 0x1810 -> Big Endian: [0x18] [0x10]
    TEST_ASSERT_EQUAL_HEX8(0x18, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x10, buffer[1]);

    // 2. Verify Sequence Flags (Byte 2 and 3)
    // Expected: 0xC000 (Standalone) -> Big Endian: [0xC0] [0x00]
    TEST_ASSERT_EQUAL_HEX8(0xC0, buffer[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[3]);

    // 3. Verify Length (Byte 4 and 5)
    // Formula: (SecHdr(8) + Payload(4)) - 1 = 11
    // Hex: 0x000B -> Big Endian: [0x00] [0x0B]
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[4]);
    TEST_ASSERT_EQUAL_HEX8(0x0B, buffer[5]);
}

void test_CCSDS_Secondary_Header_Time(void) {
    uint8_t buffer[128];
    uint8_t dummy_payload[] = {0x01};
    
    // Simulate time passing: 0x0102030405060708 ms
    // We manually tick the time service
    for(int i = 0; i < 100; i++) TIME_Tick1ms();

    CCSDS_WrapTelemetry(0x010, dummy_payload, 1, buffer);

    // The Secondary Header starts at index 6. 
    // Since we ticked 100 times (0x64), we expect Big Endian 64 at the end.
    TEST_ASSERT_EQUAL_HEX8(0x64, buffer[13]); 
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[12]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_CCSDS_Wrap_Header_Logic);
    RUN_TEST(test_CCSDS_Secondary_Header_Time);
    return UNITY_END();
}