#include <unity.h>
#include <string.h>
#include <stdio.h>
#include "comms_frame.h"
#include "ccsds_packet.h"
#include "cdhs_router.h"
#include "time_service.h"
#include "tm_manager.h"

uint8_t shared_downlink_bus[150];
uint16_t last_packet_len = 0;

// Helper function to build the radio frame for testing
void COMMS_CreateFrame_IntegrationHelper(uint8_t* payload, uint8_t len, uint8_t* out) {
    out[0] = 0xAA; // Start Byte
    out[1] = len;  // Length
    memcpy(&out[2], payload, len);
    
    // Calculate CRC over Start + Len + Payload
    uint16_t crc = COMMS_CalculateCRC16(out, len + 2);
    out[len + 2] = (uint8_t)(crc >> 8);   // CRC High
    out[len + 3] = (uint8_t)(crc & 0xFF); // CRC Low
}

void setUp(void) {
    TIME_Init();
    COMMS_ResetParser();
}

void tearDown(void) {}

void test_FullChain_RadioToRouter(void) {
    uint8_t ccsds_buf[64];
    uint8_t radio_frame[128];
    uint8_t my_cmd_data[] = {0x01, 0x02, 0x03}; // Fake ADCS command data
    
    // 1. Create a CCSDS Packet for ADCS (APID 0x010)
    // This adds the 6-byte Primary and 8-byte Secondary Header
    CCSDS_WrapTelemetry(0x010, my_cmd_data, 3, ccsds_buf);
    uint8_t ccsds_packet_len = 6 + 8 + 3; // 17 bytes

    // 2. Wrap that CCSDS Packet into a COMMS Frame (Start Byte 0x5A + Length + CRC)
    COMMS_CreateFrame_IntegrationHelper(ccsds_buf, ccsds_packet_len, radio_frame);
    
    // 3. Feed the resulting bytes into the Parser one by one
    // This simulates the radio receiving data bit-by-bit
    int result = 0;
    uint8_t total_frame_size = ccsds_packet_len + 4; // CCSDS + Start + Len + 2 bytes CRC
    
    printf("\n[TEST] Feeding %d bytes into Parser...\n", total_frame_size);
    for(int i = 0; i < total_frame_size; i++) {
        result = COMMS_ParseByte(radio_frame[i]);
    }

    // 4. Verification
    TEST_ASSERT_EQUAL_INT(1, result); // Parser should return 1 (Frame Valid)
    // You should also see "CDHS: Routing packet to ADCS..." in the console!
}

void Simulate_Ground_Station(uint8_t* rx, uint16_t len) {
    printf("\n[EARTH TERMINAL] Incoming Data Detected...\n");

    // Check CRC
    uint16_t rx_crc = (rx[len-2] << 8) | rx[len-1];
    if (COMMS_CalculateCRC16(rx, len-2) == rx_crc) {
        printf("EARTH: CRC Valid! ✅\n");
        
        // Extract Time from CCSDS (offset 2 for frame, offset 6 for CCSDS Sec Header)
        uint64_t* raw_time = (uint64_t*)&rx[2 + 6];
        uint64_t sat_time = __builtin_bswap64(*raw_time);
        
        // Extract APID
        uint16_t apid = CCSDS_GetAPID(&rx[2]);

        printf("EARTH: Satellite Time: %llu ms\n", sat_time);
        printf("EARTH: Subsystem ID: 0x%03X\n", apid);
    } else {
        printf("EARTH: CRC Error! Packet discarded. ❌\n");
    }
}


void test_Full_Telemetry_Cycle(void) {
    uint8_t mock_data[] = {0xDE, 0xAD, 0xBE};
    
    printf("[TEST] Executing real TM_SendReport...\n");

    // 1. Satellite sends it
    TM_SendReport(0x020, mock_data, 3);

    // 2. Ground Station receives the exact same bytes
    Simulate_Ground_Station(shared_downlink_bus, last_packet_len); // 17 (CCSDS) + 4 (Frame)
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_FullChain_RadioToRouter);
    RUN_TEST(test_Full_Telemetry_Cycle);
    return UNITY_END();
}