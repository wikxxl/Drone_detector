# 🛸 Bruce Drone Detection Suite

A set of drone detection and analysis modules for [Bruce](https://github.com/pr3y/Bruce) firmware on ESP32.

> ⚠️ **LEGAL DISCLAIMER**: These modules are intended solely for educational purposes and security research. Using jamming functions and Remote ID cloning is **illegal** in most countries. Users bear full responsibility for compliance with local regulations.

---

## 📦 Contents

| Module | File | Protocol | Target |
|--------|------|----------|--------|
| **NRF Drone Radar** | `nrf_radar.cpp/.h` | 2.4GHz NRF24 | RC drones (toys) |
| **WiFi Drone Hunter** | `drone_hunter.cpp/.h` | WiFi 802.11 | Commercial drones (Remote ID) |

---

## 🎯 NRF Drone Radar v3.1

Module for detecting toy/RC drones operating on NRF24L01+ protocols (2.4GHz).

### Supported Protocols

| Drone | Bind ID | Channels | Data Rate |
|-------|---------|----------|-----------|
| SYMA X5/X8 | `0xA202020202` | 10, 34, 58, 72 | 250kbps |
| SYMA X5C-1 | `0x53594D415831` | 10, 30, 55, 75 | 250kbps |
| BAYANG/E010 | `0x1223344556` | 0, 20, 40, 60 | 1Mbps |
| EACHINE H8 | `0xBA11BA11BA` | 5, 25, 45, 65 | 1Mbps |
| MJX BUGS 3/5W | `0x6D6D6D6D6D` | 30, 50, 70, 80 | 1Mbps |
| WLTOYS V202 | `0x6688686868` | 14, 30, 46, 62 | 250kbps |
| HUBSAN H107 | `0xE7E7E7E7E7` | 5, 20, 45, 70 | 250kbps |

### Features

- **RPD Scanning** – Scans the 2400-2485 MHz band
- **Protocol Handover** – Automatic drone protocol identification
- **Hop Pattern Verification** – Verifies frequency hopping sequence
- **Locked Tracking** – Tracks locked target
- **Fox Hunt Mode** – DNA decoder with signal strength bar
- **Sentry Mode** – Automatic detection + response every 8s
- **SD Logging** – Saves logs to `/radar_dron_nrf.txt`
- **Reactive Jamming** – Jamming on drone's hop channels

### Operation Flow

```
┌─────────────────────────────────────────────────────────────┐
│                      NRF DRONE RADAR                        │
├─────────────────────────────────────────────────────────────┤
│  1. SCAN       Scanning channels 0-85 (2400-2485 MHz)      │
│       ↓                                                     │
│  2. DETECT     RF activity detection (RPD)                 │
│       ↓                                                     │
│  3. IDENTIFY   Match against protocol database             │
│       ↓                                                     │
│  4. VERIFY     Hop pattern verification (2+ channels)      │
│       ↓                                                     │
│  5. LOCK       Lock on target + tracking                   │
│       ↓                                                     │
│  6. ACTION     Fox Hunt / Jamming / Logging                │
└─────────────────────────────────────────────────────────────┘
```

### User Interface

| Button | List View | Fox Hunt |
|--------|-----------|----------|
| **NEXT/PREV** | Navigate list | — |
| **SELECT** | Sentry ON/OFF or enter details | Manual attack |
| **ESC** | Exit program | Return to list |

### Displayed Information

**List (SCAN):**
- Current scanning channel (MHz)
- Status: `SCAN` / `TARGET LOCKED!` / `SENTRY MODE ACTIVE`
- List of detected drones (max 4) with channel

**Fox Hunt (DETAILS):**
- Drone protocol name
- Bind address (hex)
- Signal strength bar (0-100%)
- Raw payload data (16 bytes hex)

---

## 📡 WiFi Drone Hunter

Module for detecting commercial drones broadcasting **Remote ID** signals (ASTM F3411 / ASD-STAN prEN 4709-002).

### Supported Brands (OUI Database)

| Manufacturer | Models |
|--------------|--------|
| **DJI** | Phantom, Mavic, Mini, Air, Avata, FPV, Enterprise |
| **Parrot** | AR.Drone, Bebop, Anafi, Disco, Mambo |
| **Autel** | EVO I/II/Max/Lite/Nano |
| **Skydio** | Skydio 2, X2 |
| **FIMI/Xiaomi** | X8 SE, Mi Drone, A3, Mini |
| **Yuneec** | Typhoon H, H520, Mantis Q |
| **Hubsan** | Zino, H501S, H117S |
| **Holy Stone** | HS100, HS700, HS720 |
| **Others** | Potensic, Ruko, Snaptain, SYMA, Eachine, Walkera, 3DR, GoPro Karma |
| **DIY** | ESP8266/ESP32 custom drones |
| **FPV** | FatShark, Orqa goggles |

### Decoded Remote ID Data

| Message Type | Data |
|--------------|------|
| **Type 0** | UAV Serial Number (20 characters) |
| **Type 1** | Drone position (lat/lon), altitude, speed |
| **Type 4** | Operator position (lat/lon) |
| **Type 5** | Operator ID (20 characters) |

### Features

- **Passive Sniffing** – WiFi promiscuous mode, no transmission
- **Remote ID Parsing** – Full ASTM F3411 decoder
- **Brand Detection** – Identification based on MAC OUI
- **Multi-target Tracking** – Up to 6 drones simultaneously
- **Channel Hopping** – Automatic WiFi channel switching
- **iClone Mode** – Remote ID cloning (retransmission)
- **Sentry Mode** – Auto-clone on detection

### Operation Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    WIFI DRONE HUNTER                        │
├─────────────────────────────────────────────────────────────┤
│  1. PROMISCUOUS  ESP32 WiFi in monitor mode                │
│       ↓                                                     │
│  2. BEACON       Detect frame with FA:0B:BC signature      │
│       ↓                                                     │
│  3. PARSE        Decode Remote ID (Type 0-5)               │
│       ↓                                                     │
│  4. IDENTIFY     Match OUI against manufacturer database   │
│       ↓                                                     │
│  5. TRACK        Update position, RSSI, channel            │
│       ↓                                                     │
│  6. ACTION       Clone / Display / Log                     │
└─────────────────────────────────────────────────────────────┘
```

### User Interface

| Button | List | Details | Cloning |
|--------|------|---------|---------|
| **NEXT/PREV** | Navigate | — | — |
| **SELECT** | Sentry / Enter details | Start iClone | — |
| **PREV** | — | Return to list | Stop cloning |
| **ESC** | Exit to Bruce menu | Exit | Exit |

### Displayed Information

**List (SCAN_LIST):**
- Packet counter
- Sentry Mode status
- Drone list: MAC, brand, RSSI

**Details (DETAILS):**
- Brand + model
- Serial Number (UAV ID)
- MAC address
- Drone position (lat/lon)
- Altitude + speed
- Operator position
- Raw payload data (16 bytes)

**Cloning (CLONING):**
- Red screen "TRANSMITTING DNA"
- Cloned drone data
- Burst sequence: Type 0 → 1 → 4 → 5

---

## 🔧 Hardware Requirements

### NRF Drone Radar

| Component | Requirements |
|-----------|--------------|
| **MCU** | ESP32 (supported by Bruce) |
| **Radio** | NRF24L01+ or NRF24L01+PA+LNA |
| **Antenna** | External 2.4GHz recommended |
| **SD Card** | Optional (for logging) |

**NRF24L01 Wiring:**

```
NRF24L01    ESP32
────────    ─────
VCC    →    3.3V
GND    →    GND
CE     →    GPIO (Bruce config)
CSN    →    GPIO (Bruce config)
SCK    →    GPIO18 (VSPI_CLK)
MOSI   →    GPIO23 (VSPI_MOSI)
MISO   →    GPIO19 (VSPI_MISO)
IRQ    →    (optional)
```

### WiFi Drone Hunter

| Component | Requirements |
|-----------|--------------|
| **MCU** | ESP32 with WiFi |
| **Antenna** | Built-in or external 2.4GHz |

---

## 📥 Installation

### 1. Download Files

```bash
git clone https://github.com/YOUR_REPO/bruce-drone-modules.git
```

### 2. Copy to Bruce Project

```bash
cp nrf_radar.cpp nrf_radar.h /path/to/Bruce/src/modules/nrf24/
cp drone_hunter.cpp drone_hunter.h /path/to/Bruce/src/modules/wifi/
```

### 3. Register Modules

Add to the appropriate Bruce menu file:

```cpp
// In RF menu:
#include "modules/nrf24/nrf_radar.h"
// ...
{"NRF Drone Radar", nrf_drone_radar},

// In WiFi menu:
#include "modules/wifi/drone_hunter.h"
// ...
{"Drone Hunter", wifi_drone_hunter},
```

### 4. Compile

```bash
pio run -e YOUR_BOARD
pio run -e YOUR_BOARD --target upload
```

---

## 🏗️ Code Architecture

### NRF Radar

```
nrf_radar.cpp
├── DRONE_DB[]              // RC drone protocol database
├── DetectedDrone           // Detected drone structure
├── reset_radio()           // Reset NRF to scanner mode
├── radar_scan_step()       // Scanning step (1 channel)
├── radar_check_protocol()  // Protocol identification
├── verify_pattern()        // Hop pattern verification
├── radar_track_target()    // Locked target tracking
├── fire_jammer()           // Jamming transmission
├── logToSD()               // SD card logging
└── nrf_drone_radar()       // Main loop + UI
```

### WiFi Hunter

```
drone_hunter.cpp
├── drone_database[]        // Manufacturer OUI database
├── RemoteID_Drone          // Remote ID drone structure
├── drone_hunter_sniffer()  // Promiscuous callback
├── drone_hunter_setup()    // WiFi initialization
├── drone_hunter_loop()     // Main loop + UI
├── sendCloneBeacon()       // Single frame transmission
└── drone_hunter_clone_send() // Cloning burst sequence
```

---

## 📋 Log Format (NRF Radar)

File: `/radar_dron_nrf.txt` on SD card

```
[123s] SYMA X5/X8 | Ch:34
[156s] BAYANG/E010 | Ch:40
[189s] MJX BUGS 3 | Ch:50
```

---

## 🔬 Remote ID Protocol (WiFi)

### Beacon Signature

```
Offset  Value     Description
0x00    0xFA      Vendor OUI byte 1
0x01    0x0B      Vendor OUI byte 2  
0x02    0xBC      Vendor OUI byte 3
0x03    0x0D      Protocol version
0x04    Counter   Message counter
0x05+   Messages  25-byte message blocks
```

### Message Structure (25 bytes)

```
Byte 0: [Type:4][SubType:4]
Byte 1: Flags/Length
Byte 2-24: Payload (type-dependent)
```

---

## ⚠️ Warnings

1. **Jamming is illegal** – The jamming function (fire_jammer) violates telecommunications regulations in the EU, USA, and most countries.

2. **Remote ID cloning is illegal** – Spoofing a drone identifier may be prosecuted as a criminal offense.

3. **Research only** – These modules are intended for testing in controlled laboratory environments.

4. **No warranty** – Code provided "as is" without any warranties.

---

## 📄 License

MIT License – See LICENSE file

---

## 🤝 Contributing

1. Fork the repository
2. Create a branch: `git checkout -b feature/new-feature`
3. Commit: `git commit -m 'Add new feature'`
4. Push: `git push origin feature/new-feature`
5. Open a Pull Request

---

## 📚 Related Projects

- [Bruce Firmware](https://github.com/pr3y/Bruce) – Main project
- [RF24 Library](https://github.com/nRF24/RF24) – NRF24L01 library
- [ASTM F3411](https://www.astm.org/f3411-22a.html) – Remote ID standard

---

*Last updated: January 2026*
