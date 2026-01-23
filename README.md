# 🛰️ CubeSat Flight Software – Communications Subsystem (Comms)

This repository implements a high-reliability **Link and Network Layer** synchronization core for a 1U/3U CubeSat. It serves as the **"Ears"** of the satellite, transforming raw, noisy radio byte streams into validated **CCSDS (Consultative Committee for Space Data Systems)** packets.

This project has evolved from a simple command simulator into a standardized communication stack featuring a robust state-machine parser, CRC-16 error detection, and a post-office routing architecture.

---

## 🚀 Key Features

### Standardized CCSDS Space Packet Protocol

* Implements **CCSDS Primary Headers** (6 bytes) for Application Process Identifier (APID) routing.
* Integrated with a **Secondary Header** (8 bytes) containing Mission Elapsed Time (MET) for precise telemetry timestamping.
* Supports multi-subsystem addressing (ADCS, EPS, CDHS, etc.).

### Non-Blocking State-Machine Parser

* Byte-wise parser designed for interrupt-driven or DMA-based radio inputs.
* Resists stream noise by implementing a deterministic **Search → Length → Payload → CRC** flow.
* Ensures the CPU only processes data once a full, valid frame is synchronized.

### CRC-16 (CCITT-FALSE) Data Integrity

* Implements the industry-standard **CRC-16 CCITT-FALSE** (Polynomial: `0x1021`, Initial: `0xFFFF`).
* Every packet is mathematically verified before being passed to the Command & Data Handling (CDH) brain.
* Applied consistently across both **Uplink (Commands)** and **Downlink (Telemetry)**.

---

## 🏗️ System Architecture & Data Flow

The system follows the **OSI Model** principles, separating the "how it moves" (Link Layer) from the "what it says" (Application Layer).

### 1. The Link Layer (The Delivery Truck)

Responsible for **framing**. It wraps the data in a "Comms Frame":

```
[Start Byte: 0xAA] [Length] [--- CCSDS PACKET ---] [CRC-16 High] [CRC-16 Low]
```

### 2. The Network Layer (The Post Office)

Once the frame is validated, the **CCSDS Packet** is handed to the **CDHS Router**. The router extracts the 11-bit APID to determine the destination:

* **APID 0x010**: Attitude Determination & Control (ADCS)
* **APID 0x020**: Electrical Power System (EPS)
* **APID 0x040**: Internal CDH Tasks
* **APID 0x7FF**: Idle/Fill Packets (Discarded)

---

## 📂 Project Structure & Dependencies

This subsystem is part of a **modular Flight Software Stack**. It is designed to be built alongside its sibling repositories:

```
CubeSat_FSW_Comms/
├── lib/
│   └── comms_frame/          # Core COMMS_ParseByte state machine and CRC math
├── src/
│   └── ccsds_packet.c        # Packet wrapping, APID bit-masking, endianness
├── include/                  # Standardized headers for cross-repo communication
└── test/
    ├── unity/                # Unity Test Framework
    └── test_integration.c    # Grand integration test
```

### External Dependencies:

* **[Cubesat_CDH_FSW](https://github.com/ozgunbkl/Cubesat_CDH_FSW)**: Provides the routing logic (`cdhs_router.h`)
* **CubeSat_Time_Service**: Provides the 64-bit MET for packet headers

---

## 🛠️ Technical Implementation Details

### Bit-Manipulation & Endianness

Space standards require **Big-Endian** network byte order. The system ensures compatibility between different MCU architectures using explicit bit-shifting:

```c
// Extracting 11-bit APID from a 16-bit header field
uint16_t raw_header = (buffer[0] << 8) | buffer[1];
uint16_t apid = raw_header & 0x07FF; 
```

### Memory Safety

* **No Dynamic Allocation**: Zero use of `malloc`, preventing heap fragmentation and "Out of Memory" crashes in deep space.
* **Defensive Programming**: Explicit state resets (`received_crc = 0`) at the end of every parsing cycle to prevent stale data pollution.

---

## 🧪 Verification & Grand Integration

Verification is performed using the **Unity Test Framework**. In addition to unit tests, this project features a **Grand Integration Test** that simulates the entire ground-to-space-to-subsystem chain.

### The Integration Loop:

1. **Wrap**: Build a CCSDS packet with a timestamp and ADCS command.
2. **Frame**: Encapsulate it in a Comms frame with CRC.
3. **Inject**: Feed the bytes one-by-one into the parser.
4. **Route**: Verify the CDH Router identifies the APID and "delivers" the command.

### To run the integration test:

```bash
gcc -o test_integration test/test_integration.c \
    lib/comms_frame/comms_frame.c src/ccsds_packet.c \
    ../Cubesat_CDH_FSW/src/cdhs_router.c \
    ../CubeSat_Time_Service/src/time_service.c \
    test/unity/unity.c \
    -I include -I ../Cubesat_CDH_FSW/include -I ../CubeSat_Time_Service/include -I test/unity
./test_integration
```

---

## 📘 Summary

This project demonstrates:

* ✅ Implementation of **CCSDS Standard 133.0-B-2** (Space Packet Protocol)
* ✅ **Multi-Subsystem Routing** via APID identification
* ✅ **Robust State-Machine Parser** design for noisy RF links
* ✅ **Modular Flight Software** architecture across multiple repositories

---


## 🏆 Features Demonstrated

| Feature | Status | Description |
|---------|--------|-------------|
| CCSDS Primary Header | ✅ Complete | 6-byte header with APID routing |
| CCSDS Secondary Header | ✅ Complete | 8-byte header with MET timestamp |
| State Machine Parser | ✅ Complete | Noise-resistant byte-wise parsing |
| CRC-16 Validation | ✅ Complete | End-to-end packet integrity |
| Multi-Subsystem Routing | ✅ Complete | APID-based command distribution |
| Zero Dynamic Allocation | ✅ Complete | No malloc/free for reliability |
| Integration Testing | ✅ Complete | Full ground-to-subsystem simulation |

---

## ⚙️ Building the Project

This project uses **PlatformIO** with the **ESP-IDF framework**:

1. **Install PlatformIO**: Follow the [official guide](https://platformio.org/install)

2. **Clone the repository**:
   ```bash
   git clone https://github.com/ozgunbkl/CubeSat_FSW_Comms.git
   cd CubeSat_FSW_Comms
   ```

3. **Build & Upload**:
   ```bash
   platformio run --target upload -e esp32dev
   ```

4. **Monitor Serial Output**:
   ```bash
   platformio device monitor
   ```


---

**Built with ❤️ for the CubeSat community**
