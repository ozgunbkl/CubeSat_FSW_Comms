# 🛰️ CubeSat Flight Software – Communications & Command Core

A modular, high-reliability **Communication and Command (C&DH / TMTC core)** designed for a simulated CubeSat.  
This project implements a robust **ground-to-space communication link** featuring custom packet framing, CRC-16 error detection, and a command dispatch layer suitable for integration into a larger Flight Software (FSW) architecture.

The system is designed for **software-only development and verification**, reflecting early-phase CubeSat missions where hardware may be unavailable.

---

## 🚀 Key Features

### Robust Framing Protocol
- Custom **Start–Length–Payload–CRC** frame structure  
- Ensures correct synchronization and recovery in noisy serial environments  
- Designed for deterministic, byte-wise parsing  

### CRC-16 Error Detection
- Implements **CRC-16 CCITT (Big-Endian)** checksum  
- Detects bit-level corruption (e.g., simulated radiation-induced errors)  
- Applied consistently on both uplink and downlink paths  

### Non-Blocking State-Machine Parser
- Byte-wise parser implemented as a deterministic **state machine**
- Filters stream noise and extracts valid frames without blocking
- Suitable for RTOS tasks or ISR-driven input

**Parser State Flow:**
```
SEARCHING → READING_LENGTH → READING_PAYLOAD → VERIFYING_CRC → DISPATCH
```

This design ensures robustness against malformed frames and random byte noise on the communication channel.

---

### Command Dispatch Layer
Validated commands are routed to simulated satellite subsystems:

- **Orbit Maintenance (Simulated)**  
  Updates a 32-bit altitude state based on commanded burn durations.  
  Simulates **station-keeping maneuvers** required to counteract atmospheric drag and maintain orbital altitude.

- **Thermal Control**  
  Updates signed temperature thresholds (`int8_t`) for system monitoring in deep-space environments.

---

### Bidirectional Telemetry (Downlink)
- Telemetry generator decomposes 32-bit system states into byte streams  
- Demonstrates end-to-end **ground ↔ satellite** data exchange  

---

## 🏗️ Architecture Overview

The system is structured into three clearly separated layers:

### 1. Transport Layer
- `COMMS_CreateFrame`  
- `COMMS_CalculateCRC16`  

Handles packet framing, serialization, and checksum generation.

---

### 2. Parser Layer
- `COMMS_ParseByte`  

Implements a state-machine parser responsible for synchronization, length validation, and CRC verification.

---

### 3. Application Layer
- `COMMS_DispatchCommand`  

Interprets validated command IDs and updates system-level state variables (e.g., altitude, temperature).

Each layer is independently testable and designed for later integration into an RTOS-based CubeSat Flight Software stack.

---

## 🛠️ Technical Implementation Details

### Bit Manipulation & Serialization
To transmit 32-bit data (such as altitude) over an 8-bit communication link, the system uses explicit bit-shifting and masking.

**Serialization (Downlink):**
```c
uint8_t byte = (uint8_t)((altitude >> 24) & 0xFF);
```

**Deserialization (Uplink):**
```c
uint32_t altitude =
    ((uint32_t)rx[0] << 24) |
    ((uint32_t)rx[1] << 16) |
    ((uint32_t)rx[2] << 8)  |
     (uint32_t)rx[3];
```

This approach ensures portability, deterministic behavior, and precise control over data layout.

### Memory Safety & Determinism
- Uses static internal state to avoid global namespace pollution
- Frames are cleared using memset to prevent stale data propagation
- No dynamic memory allocation
- Designed for predictable execution in constrained embedded environments

---

## 🧪 Verification & Testing
Verification is performed using the Unity Test Framework.
The test suite covers seven mission-relevant scenarios, including:

- CRC validation and rejection of corrupted frames
- Frame extraction from high-noise byte streams
- Signed integer handling for thermal commands
- 32-bit altitude reconstruction and update
- End-to-end bidirectional telemetry validation

### Test Results
```
7 Tests 0 Failures 0 Ignored
OK
[SUB-SYSTEM] ORBIT: Burn for 10 sec. Altitude is now 501000 meters.
[GROUND] Telemetry Received: Temp=-5C, Alt=505000 m
```

---

## 📌 Project Status & Roadmap

### Current Status
- Communications framing: Implemented and verified
- Command dispatch: Implemented (simulated subsystems)
- Unit testing: Complete

### Planned Extensions
- CCSDS-compatible packet structures
- Autonomous safe-mode command handling
- Forward Error Correction (FEC) for enhanced link reliability
- RTOS-based task integration

---

## 📘 Summary
This project demonstrates:

- Embedded communications and framing fundamentals
- CRC-16 data integrity mechanisms
- Deterministic state-machine parser design
- Low-level bit manipulation and memory-safe coding practices
- Verification-driven development suitable for CubeSat Flight Software
