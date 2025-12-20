#include <stdio.h>
#include "unity.h"
#include "../../include/comms_frame.h"
#include <string.h>

#define CRC16_POLY 0x1021

// Global or static variables to track the parser's progress
static parser_state_t current_state = STATE_SEARCHING_FOR_START;
static comms_frame_t rx_frame;
static uint8_t payload_index = 0;
static uint8_t crc_index = 0;
static uint16_t received_crc = 0;

// Global variables to represent the satellite's physical state
uint8_t g_thruster_duration = 0;
int8_t g_target_temperature = 20; // Default 20 Celsius
uint8_t g_heater_status = 0;

uint32_t g_satellite_altitude = 500000; // 500,000 meters (500km)

void COMMS_DispatchCommand(comms_frame_t *frame){
    if(frame->length < 1) return; // Need at least a Command ID

    //Byte 0 of the payload is the Command ID
    uint8_t command_id = frame->payload[0];

    switch (command_id){
        case CMD_ORBIT_MAINTENANCE:
            //Byte 1 of the payload is the thruster duration
            g_thruster_duration = frame->payload[1];
            // Simulate: Each second of burn adds 100 meters of altitude
            uint32_t altitude_gain = g_thruster_duration * 100;
            g_satellite_altitude += altitude_gain;
            
            printf("[SUB-SYSTEM] ORBIT: Burn for %d sec. Altitude is now %u meters.\n", g_thruster_duration, g_satellite_altitude);
            break;
        
        case CMD_THERMAL_CONTROL:
            //Byte 1 of the payload is the target temperature
            g_target_temperature = (int8_t)frame->payload[1];
            printf("[SUB-SYSTEM] THERMAL: Target temperature set to %d Celsius.\n", g_target_temperature);
            break;
        
        default:
            printf("[SUB-SYSTEM] ERROR: Unknown Command ID 0x%02X .\n", command_id);
            break;
    }
    
}

/**
 * @brief Processes a single byte received from the radio.
 * @return 1 if a full, valid frame was found, 0 otherwise.
 */
int COMMS_ParseByte(uint8_t byte) {
    switch (current_state) {
        case STATE_SEARCHING_FOR_START:
            if (byte == FRAME_START_BYTE) {
                memset(&rx_frame, 0, sizeof(comms_frame_t));
                rx_frame.start_byte = byte;
                current_state = STATE_READING_LENGTH;
            }
            break;

        case STATE_READING_LENGTH:
            if (byte > 0 && byte <= MAX_PAYLOAD_SIZE) {
                rx_frame.length = byte;
                payload_index = 0;
                current_state = STATE_READING_PAYLOAD;
            } else {
                current_state = STATE_SEARCHING_FOR_START;
            }
            break;

        case STATE_READING_PAYLOAD:
            rx_frame.payload[payload_index++] = byte;
            if (payload_index >= rx_frame.length) {
                crc_index = 0;
                received_crc = 0;
                current_state = STATE_VERIFYING_CRC;
            }
            break;

        case STATE_VERIFYING_CRC:
            if (crc_index == 0) {
                received_crc = (uint16_t)byte << 8; // High Byte
                crc_index++;
            } else {
                received_crc |= (uint16_t)byte;    // Low Byte
                
                // Reconstruct validation buffer
                uint8_t v_buf[MAX_PAYLOAD_SIZE + 2];
                v_buf[0] = FRAME_START_BYTE;
                v_buf[1] = rx_frame.length;
                for(int i = 0; i < rx_frame.length; i++) {
                    v_buf[i+2] = rx_frame.payload[i];
                }

                uint16_t calc_crc = COMMS_CalculateCRC16(v_buf, rx_frame.length + 2);
                
                // Debugging (Keep this until you see the Green Pass!)
                printf("DEBUG SAT: Calc: 0x%04X, Recv: 0x%04X\n", calc_crc, received_crc);

                current_state = STATE_SEARCHING_FOR_START;
                if (calc_crc == received_crc) {
                    COMMS_DispatchCommand(&rx_frame); // Execute immediately!
                    return 1;
                }
            }
            break;
    }
    return 0;
}

void COMMS_ResetParser(void) {
    current_state = STATE_SEARCHING_FOR_START;
    payload_index = 0;
    crc_index = 0;
    received_crc = 0;
}


uint16_t COMMS_CalculateCRC16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF; // Initial value for CCITT-FALSE

    for (size_t i = 0; i < length; i++) {
        // Step 1: XOR the byte into the MSB of the CRC
        crc ^= (uint16_t)data[i] << 8;

        for (uint8_t j = 0; j < 8; j++) {
            // Step 2: If MSB is 1, shift and XOR with polynomial
            if (crc & 0x8000) {
                crc = (crc << 1) ^ CRC16_POLY;
            } else {
                // Step 3: Otherwise, just shift
                crc <<= 1;
            }
        }
    }
    return crc;
}

void COMMS_CreateFrame(comms_frame_t *frame, const uint8_t *payload, uint8_t length) {
    if(frame == NULL || payload == NULL || length > MAX_PAYLOAD_SIZE){
        return;  // Basic safety check
    }

    // 1. Set the Header
    frame->start_byte = FRAME_START_BYTE;
    frame->length = length;

    // 2. Copy the Data
    memcpy(frame->payload, payload, length);

    // 3. Calculate the CRC for the entire packet (except the CRC field itself)
    uint8_t temp_buffer[MAX_PAYLOAD_SIZE + 2]; 
    temp_buffer[0] = frame->start_byte;
    temp_buffer[1] = frame->length;
    memcpy(&temp_buffer[2], payload, length);

    // Calculate CRC over (Start Byte + Length + Payload)
    frame->crc = COMMS_CalculateCRC16(temp_buffer, length + 2);
}

void COMMS_GenerateTelemetry(comms_frame_t *out_frame){
    uint8_t status_data[5];

    // Byte 0: Current Temperature
    status_data[0] = (uint8_t)g_target_temperature;

    // Byte 1-4: Current Altitude (Breaking 32-bit into 4 bytes)
    status_data[1] = (uint8_t)((g_satellite_altitude >> 24) & 0xFF);
    status_data[2] = (uint8_t)((g_satellite_altitude >> 16) & 0xFF);
    status_data[3] = (uint8_t)((g_satellite_altitude >> 8) & 0xFF);
    status_data[4] = (uint8_t)(g_satellite_altitude & 0xFF);

    // Using CreateFrame to seal it with a CRC 
    COMMS_CreateFrame(out_frame, status_data, 5);
}