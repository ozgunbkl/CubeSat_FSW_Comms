#include "unity.h"
#include "../include/comms_frame.h"
#include <stdint.h>

void setUp(void) {
    // This runs before every test
}

void tearDown(void) {
    // This runs after every test
}

/**
 * Test Vector 1: Standard ASCII "123456789"
 * The expected CRC-16/CCITT-FALSE for this string is 0x29B1.
 */
void test_CRC16_StandardString(void) {
    const uint8_t data[] = "123456789";
    uint16_t expected_crc = 0x29B1;
    
    uint16_t actual_crc = COMMS_CalculateCRC16(data, 9);
    
    TEST_ASSERT_EQUAL_HEX16(expected_crc, actual_crc);
}

void test_CRC16_ShortCommand(void) {
    const uint8_t cmd[] = {0x01, 0x02, 0x03};
    // Correct CCITT-FALSE CRC for [0x01, 0x02, 0x03] is 0xADAD
    uint16_t expected_crc = 0xADAD; 
    
    uint16_t actual_crc = COMMS_CalculateCRC16(cmd, 3);
    
    TEST_ASSERT_EQUAL_HEX16(expected_crc, actual_crc);
}

/**
 * Test: Verify that a frame is correctly packaged.
 */
void test_CreateFrame_Basic(void){
    comms_frame_t frame;
    uint8_t test_data[] = {0xDE, 0xAD, 0xBE, 0xEF};  // 4 bytes of data

    //Act: Create the frame
    COMMS_CreateFrame(&frame, test_data, 4);

    //Assert: Check the envelope parts
    TEST_ASSERT_EQUAL_HEX8(FRAME_START_BYTE, frame.start_byte);
    TEST_ASSERT_EQUAL_INT(4, frame.length);
    
    // 0x246B is the correct CCITT-FALSE CRC for: AA 04 DE AD BE EF
    TEST_ASSERT_EQUAL_HEX16(0x246B, frame.crc);
}

void test_Parser_FindsFrameInNoise(void) {
    COMMS_ResetParser();
    
    comms_frame_t tx_frame;
    uint8_t cmd_data[] = {0x01, 0x02, 0x03}; 
    COMMS_CreateFrame(&tx_frame, cmd_data, 3);
    
    uint8_t hi = (uint8_t)(tx_frame.crc >> 8);
    uint8_t lo = (uint8_t)(tx_frame.crc & 0xFF);
    
    // Noise(2) + Start(1) + Len(1) + Payload(3) + CRC(2) + Noise(1) = 10 bytes
    uint8_t stream[] = {0xFF, 0x00, 0xAA, 0x03, 0x01, 0x02, 0x03, hi, lo, 0xEE};
    
    int found = 0;
    for(int i = 0; i < sizeof(stream); i++) {
        if(COMMS_ParseByte(stream[i]) == 1) {
            found = 1;
        }
    }
    TEST_ASSERT_TRUE(found);
}



void test_Mission_ThermalUpdate(void) {
    COMMS_ResetParser();
    
    // 1. Create a "Set Heater to 15C" command
    // Payload[0] = Command ID (0xB2)
    // Payload[1] = Target Temp (15)
    uint8_t thermal_cmd[] = {CMD_THERMAL_CONTROL, 15};
    comms_frame_t tx_frame;
    COMMS_CreateFrame(&tx_frame, thermal_cmd, 2);
    
    // 2. Stream it through the radio
    uint8_t hi = (uint8_t)(tx_frame.crc >> 8);
    uint8_t lo = (uint8_t)(tx_frame.crc & 0xFF);
    uint8_t stream[] = {tx_frame.start_byte, tx_frame.length, 
                        thermal_cmd[0], thermal_cmd[1], 
                        hi, lo};
    
    // 3. Parser hears the bytes
    for(int i = 0; i < (int)sizeof(stream); i++) {
    COMMS_ParseByte(stream[i]);
}
    
    // 4. VERIFY: Did the satellite's target temperature change?
    TEST_ASSERT_EQUAL_INT8(15, g_target_temperature);
}

void test_Mission_OrbitBurn(void) {
    COMMS_ResetParser();
    
    // Initial State Check
    printf("\n[TEST] Starting Altitude: %u meters\n", g_satellite_altitude);
    
    // Create Command: ID 0xA1 (Orbit), Duration 10 seconds
    uint8_t orbit_cmd[] = {CMD_ORBIT_MAINTENANCE, 10}; 
    comms_frame_t tx_frame;
    COMMS_CreateFrame(&tx_frame, orbit_cmd, 2);
    
    // Pack the stream
    uint8_t hi = (uint8_t)(tx_frame.crc >> 8);
    uint8_t lo = (uint8_t)(tx_frame.crc & 0xFF);
    uint8_t stream[] = {0xAA, 0x02, 0xA1, 10, hi, lo};
    
    // Feed the parser
    for(int i = 0; i < (int)sizeof(stream); i++) {
        COMMS_ParseByte(stream[i]);
    }
    
    // VERIFY: 10 seconds of burn * 100m = 1000m gain
    // 500,000 + 1,000 = 501,000
    printf("[TEST] Ending Altitude:   %u meters\n", g_satellite_altitude);
    TEST_ASSERT_EQUAL_UINT32(501000, g_satellite_altitude);
}

void test_Mission_TelemetryDownlink(void) {
    // 1. Setup a specific state
    g_target_temperature = -5;
    g_satellite_altitude = 505000;
    
    comms_frame_t tl_frame;
    COMMS_GenerateTelemetry(&tl_frame);
    
    // 2. Verify the ID and Data
    TEST_ASSERT_EQUAL_INT8(-5, (int8_t)tl_frame.payload[0]);
    
    // 3. Reconstruct the 32-bit altitude from the payload bytes
    uint32_t received_alt = ((uint32_t)tl_frame.payload[1] << 24) |
                            ((uint32_t)tl_frame.payload[2] << 16) |
                            ((uint32_t)tl_frame.payload[3] << 8)  |
                            ((uint32_t)tl_frame.payload[4]);
                            
    TEST_ASSERT_EQUAL_UINT32(505000, received_alt);
    printf("[GROUND] Telemetry Received: Temp=%dC, Alt=%u m, CRC=0x%04X\n", 
            (int8_t)tl_frame.payload[0], received_alt, tl_frame.crc);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_CRC16_StandardString);
    RUN_TEST(test_CRC16_ShortCommand);
    RUN_TEST(test_CreateFrame_Basic);
    RUN_TEST(test_Parser_FindsFrameInNoise);
    RUN_TEST(test_Mission_ThermalUpdate);
    RUN_TEST(test_Mission_OrbitBurn);
    RUN_TEST(test_Mission_TelemetryDownlink);
    return UNITY_END();
}