#ifndef CCSDS_PACKET_H
#define CCSDS_PACKET_H

#include <stdint.h>
#include <stdbool.h>

// CCSDS Primary Header (6 bytes / 48 bits)
typedef struct __attribute__((packed)) {
    uint16_t packet_id;
    uint16_t sequence_ctrl;
    uint16_t packet_length;
} CCSDS_PrimaryHeader_t;

// CCSDS Secondary Header (8 bytes / 64 bits)
typedef struct __attribute__((packed)){
    uint64_t mission_time;     // MET in milliseconds 
} CCSDS_SecondaryHeader_t;

// Standard APID (Application Process Identifiers)
#define APID_ADCS    0x010
#define APID_EPS     0x020
#define APID_FDIR    0x030
#define APID_CDHS    0x040
#define APID_HK      0x050
#define APID_ARCHIVE 0x060
#define APID_PAYLOAD 0x070
#define APID_IDLE    0x7FF    // CCSDS Standard for Idle/Fill packets


// Extract APID from a raw buffer
uint16_t CCSDS_GetAPID(const uint8_t* buffer);


// Check if the Secondary Header Flag is set
bool CCSDS_HasSecondaryHeader(const uint8_t* buffer);

void CCSDS_WrapTelemetry(uint16_t apid, const uint8_t* app_data, uint16_t app_data_len, uint8_t* out_buffer);


#endif
