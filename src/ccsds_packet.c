#include "ccsds_packet.h"
#include "time_service.h"
#include <string.h>
#include <arpa/inet.h>   // For htons/ntohs (Big-Endian conversion)

uint16_t CCSDS_GetAPID(const uint8_t* buffer){
    if(!buffer) return 0;

    // Map the buffer to the struct
    CCSDS_PrimaryHeader_t* hdr = (CCSDS_PrimaryHeader_t*)buffer;
    
    // 1. Convert from Big-Endian (Network) to Little-Endian (Host)
    uint16_t id = ntohs(hdr->packet_id);

    // 2. Mask the last 11 bits (0x07FF = 0000 0111 1111 1111)
    return (id & 0x07FF);
}

bool CCSDS_HasSecondaryHeader(const uint8_t* buffer) {
    if (!buffer) return false;
    
    CCSDS_PrimaryHeader_t* hdr = (CCSDS_PrimaryHeader_t*)buffer;
    uint16_t id = ntohs(hdr->packet_id);
    
    // Secondary Header Flag is bit 11 (counting from right, 0-indexed)
    // Mask: 0x0800 (0000 1000 0000 0000)
    return (id & 0x0800) != 0;
}


void CCSDS_WrapTelemetry(uint16_t apid, const uint8_t* app_data, uint16_t app_data_len, uint8_t* out_buffer){
    CCSDS_PrimaryHeader_t* pri_hdr = (CCSDS_PrimaryHeader_t*)out_buffer;
    CCSDS_SecondaryHeader_t* sec_hdr = (CCSDS_SecondaryHeader_t*)(out_buffer + sizeof(CCSDS_PrimaryHeader_t));

    // 1. Build Packet ID (Version 0, Type 1 (TM), Sec Hdr 1, APID)
    // 0x1800 sets the Type bit and the Secondary Header flag bit
    uint16_t id = 0x1800 | (apid & 0x07FF);
    pri_hdr ->packet_id = htons(id);  // Flip to big-endian
    
    // 2. Sequence Control (For now, let's just set "Unsegmented" flags 0xC000)
    pri_hdr->sequence_ctrl = htons(0xC000);

    // 3. Length: (Sec Hdr size + App Data size) - 1
    uint16_t total_len = sizeof(CCSDS_SecondaryHeader_t) + app_data_len - 1;
    pri_hdr->packet_length = htons(total_len);

    // 4. Set the Time in Secondary Header
    uint64_t now = TIME_GetMilliseconds();
    sec_hdr->mission_time = __builtin_bswap64(now);

    // 5. Copy the actual data (ADCS, EPS, etc.) after the headers
    memcpy(out_buffer + sizeof(CCSDS_PrimaryHeader_t) + sizeof(CCSDS_SecondaryHeader_t), app_data, app_data_len);
}