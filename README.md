# 🛸 Bruce Drone Detection Suite

Zestaw modułów do wykrywania i analizy dronów dla firmware [Bruce](https://github.com/pr3y/Bruce) na ESP32.

> ⚠️ **UWAGA PRAWNA**: Moduły przeznaczone wyłącznie do celów edukacyjnych i badań bezpieczeństwa. Używanie funkcji zakłócających (jamming) oraz klonowania Remote ID jest **nielegalne** w większości krajów. Użytkownik ponosi pełną odpowiedzialność za zgodność z lokalnymi przepisami.

---

## 📦 Zawartość

| Moduł | Plik | Protokół | Cel |
|-------|------|----------|-----|
| **NRF Drone Radar** | `nrf_radar.cpp/.h` | 2.4GHz NRF24 | Drony RC (zabawkowe) |
| **WiFi Drone Hunter** | `drone_hunter.cpp/.h` | WiFi 802.11 | Drony komercyjne (Remote ID) |

---

## 🎯 NRF Drone Radar v3.1

Moduł wykrywający drony zabawkowe/RC pracujące na protokołach NRF24L01+ (2.4GHz).

### Wykrywane protokoły

| Dron | Bind ID | Kanały | Data Rate |
|------|---------|--------|-----------|
| SYMA X5/X8 | `0xA202020202` | 10, 34, 58, 72 | 250kbps |
| SYMA X5C-1 | `0x53594D415831` | 10, 30, 55, 75 | 250kbps |
| BAYANG/E010 | `0x1223344556` | 0, 20, 40, 60 | 1Mbps |
| EACHINE H8 | `0xBA11BA11BA` | 5, 25, 45, 65 | 1Mbps |
| MJX BUGS 3/5W | `0x6D6D6D6D6D` | 30, 50, 70, 80 | 1Mbps |
| WLTOYS V202 | `0x6688686868` | 14, 30, 46, 62 | 250kbps |
| HUBSAN H107 | `0xE7E7E7E7E7` | 5, 20, 45, 70 | 250kbps |

### Funkcje

- **RPD Scanning** – Skanowanie pasma 2400-2485 MHz
- **Protocol Handover** – Automatyczna identyfikacja protokołu drona
- **Hop Pattern Verification** – Weryfikacja sekwencji frequency hopping
- **Locked Tracking** – Śledzenie zablokowanego celu
- **Fox Hunt Mode** – Dekoder DNA z paskiem siły sygnału
- **Sentry Mode** – Automatyczne wykrywanie + reakcja co 8s
- **SD Logging** – Zapis logów do `/radar_dron_nrf.txt`
- **Reactive Jamming** – Zakłócanie na kanałach hop drona

### Schemat działania

```
┌─────────────────────────────────────────────────────────────┐
│                      NRF DRONE RADAR                        │
├─────────────────────────────────────────────────────────────┤
│  1. SCAN       Skanowanie kanałów 0-85 (2400-2485 MHz)     │
│       ↓                                                     │
│  2. DETECT     Wykrycie aktywności RF (RPD)                │
│       ↓                                                     │
│  3. IDENTIFY   Dopasowanie do bazy protokołów              │
│       ↓                                                     │
│  4. VERIFY     Weryfikacja hop pattern (2+ kanały)         │
│       ↓                                                     │
│  5. LOCK       Zablokowanie na celu + tracking             │
│       ↓                                                     │
│  6. ACTION     Fox Hunt / Jamming / Logging                │
└─────────────────────────────────────────────────────────────┘
```

### Interfejs użytkownika

| Przycisk | Lista | Fox Hunt |
|----------|-------|----------|
| **NEXT/PREV** | Nawigacja po liście | — |
| **SELECT** | Sentry ON/OFF lub wejście do detali | Atak manualny |
| **ESC** | Wyjście z programu | Powrót do listy |

### Wyświetlane informacje

**Lista (SCAN):**
- Aktualny kanał skanowania (MHz)
- Status: `SCAN` / `TARGET LOCKED!` / `SENTRY MODE ACTIVE`
- Lista wykrytych dronów (max 4) z kanałem

**Fox Hunt (DETAILS):**
- Nazwa protokołu drona
- Adres bind (hex)
- Pasek siły sygnału (0-100%)
- Surowe dane payload (16 bajtów hex)

---

## 📡 WiFi Drone Hunter

Moduł wykrywający drony komercyjne nadające sygnał **Remote ID** (ASTM F3411 / ASD-STAN prEN 4709-002).

### Wykrywane marki (baza OUI)

| Producent | Modele |
|-----------|--------|
| **DJI** | Phantom, Mavic, Mini, Air, Avata, FPV, Enterprise |
| **Parrot** | AR.Drone, Bebop, Anafi, Disco, Mambo |
| **Autel** | EVO I/II/Max/Lite/Nano |
| **Skydio** | Skydio 2, X2 |
| **FIMI/Xiaomi** | X8 SE, Mi Drone, A3, Mini |
| **Yuneec** | Typhoon H, H520, Mantis Q |
| **Hubsan** | Zino, H501S, H117S |
| **Holy Stone** | HS100, HS700, HS720 |
| **Inne** | Potensic, Ruko, Snaptain, SYMA, Eachine, Walkera, 3DR, GoPro Karma |
| **DIY** | ESP8266/ESP32 custom drones |
| **FPV** | FatShark, Orqa goggles |

### Dekodowane dane Remote ID

| Typ wiadomości | Dane |
|----------------|------|
| **Type 0** | UAV Serial Number (20 znaków) |
| **Type 1** | Pozycja drona (lat/lon), wysokość, prędkość |
| **Type 4** | Pozycja operatora (lat/lon) |
| **Type 5** | Operator ID (20 znaków) |

### Funkcje

- **Passive Sniffing** – Tryb promiscuous WiFi, brak transmisji
- **Remote ID Parsing** – Pełny dekoder ASTM F3411
- **Brand Detection** – Identyfikacja na podstawie MAC OUI
- **Multi-target Tracking** – Do 6 dronów jednocześnie
- **Channel Hopping** – Automatyczne przeskakiwanie kanałów WiFi
- **iClone Mode** – Klonowanie Remote ID drona (retransmisja)
- **Sentry Mode** – Auto-clone przy wykryciu

### Schemat działania

```
┌─────────────────────────────────────────────────────────────┐
│                    WIFI DRONE HUNTER                        │
├─────────────────────────────────────────────────────────────┤
│  1. PROMISCUOUS  ESP32 WiFi w trybie nasłuchu              │
│       ↓                                                     │
│  2. BEACON       Wykrycie ramki z sygnaturą FA:0B:BC       │
│       ↓                                                     │
│  3. PARSE        Dekodowanie Remote ID (Type 0-5)          │
│       ↓                                                     │
│  4. IDENTIFY     Dopasowanie OUI do bazy producentów       │
│       ↓                                                     │
│  5. TRACK        Aktualizacja pozycji, RSSI, kanału        │
│       ↓                                                     │
│  6. ACTION       Clone / Display / Log                     │
└─────────────────────────────────────────────────────────────┘
```

### Interfejs użytkownika

| Przycisk | Lista | Detale | Klonowanie |
|----------|-------|--------|------------|
| **NEXT/PREV** | Nawigacja | — | — |
| **SELECT** | Sentry / Wejście do detali | Start iClone | — |
| **PREV** | — | Powrót do listy | Stop klonowania |
| **ESC** | Wyjście do menu Bruce | Wyjście | Wyjście |

### Wyświetlane informacje

**Lista (SCAN_LIST):**
- Licznik pakietów
- Status Sentry Mode
- Lista dronów: MAC, marka, RSSI

**Szczegóły (DETAILS):**
- Marka + model
- Serial Number (UAV ID)
- MAC address
- Pozycja drona (lat/lon)
- Wysokość + prędkość
- Pozycja operatora
- Surowe dane payload (16 bajtów)

**Klonowanie (CLONING):**
- Czerwony ekran "TRANSMITTING DNA"
- Dane klonowanego drona
- Sekwencja burst: Type 0 → 1 → 4 → 5

---

## 🔧 Wymagania sprzętowe

### NRF Drone Radar

| Komponent | Wymagania |
|-----------|-----------|
| **MCU** | ESP32 (obsługiwany przez Bruce) |
| **Radio** | NRF24L01+ lub NRF24L01+PA+LNA |
| **Antena** | Zalecana zewnętrzna 2.4GHz |
| **SD Card** | Opcjonalnie (do logowania) |

**Podłączenie NRF24L01:**

```
NRF24L01    ESP32
────────    ─────
VCC    →    3.3V
GND    →    GND
CE     →    GPIO (konfiguracja Bruce)
CSN    →    GPIO (konfiguracja Bruce)
SCK    →    GPIO18 (VSPI_CLK)
MOSI   →    GPIO23 (VSPI_MOSI)
MISO   →    GPIO19 (VSPI_MISO)
IRQ    →    (opcjonalnie)
```

### WiFi Drone Hunter

| Komponent | Wymagania |
|-----------|-----------|
| **MCU** | ESP32 z WiFi |
| **Antena** | Wbudowana lub zewnętrzna 2.4GHz |

---

## 📥 Instalacja

### 1. Pobierz pliki

```bash
git clone https://github.com/YOUR_REPO/bruce-drone-modules.git
```

### 2. Skopiuj do projektu Bruce

```bash
cp nrf_radar.cpp nrf_radar.h /path/to/Bruce/src/modules/rf/
cp drone_hunter.cpp drone_hunter.h /path/to/Bruce/src/modules/wifi/
```

### 3. Zarejestruj moduły

Dodaj do odpowiedniego pliku menu Bruce:

```cpp
// W menu RF:
#include "modules/rf/nrf_radar.h"
// ...
{"NRF Drone Radar", nrf_drone_radar},

// W menu WiFi:
#include "modules/wifi/drone_hunter.h"
// ...
{"Drone Hunter", wifi_drone_hunter},
```

### 4. Kompilacja

```bash
pio run -e YOUR_BOARD
pio run -e YOUR_BOARD --target upload
```

---

## 🏗️ Architektura kodu

### NRF Radar

```
nrf_radar.cpp
├── DRONE_DB[]              // Baza protokołów dronów RC
├── DetectedDrone           // Struktura wykrytego drona
├── reset_radio()           // Reset NRF do trybu skanera
├── radar_scan_step()       // Krok skanowania (1 kanał)
├── radar_check_protocol()  // Identyfikacja protokołu
├── verify_pattern()        // Weryfikacja hop pattern
├── radar_track_target()    // Śledzenie zablokowanego celu
├── fire_jammer()           // Transmisja zakłócająca
├── logToSD()               // Zapis do karty SD
└── nrf_drone_radar()       // Główna pętla + UI
```

### WiFi Hunter

```
drone_hunter.cpp
├── drone_database[]        // Baza OUI producentów
├── RemoteID_Drone          // Struktura drona Remote ID
├── drone_hunter_sniffer()  // Callback promiscuous
├── drone_hunter_setup()    // Inicjalizacja WiFi
├── drone_hunter_loop()     // Główna pętla + UI
├── sendCloneBeacon()       // Transmisja pojedynczej ramki
└── drone_hunter_clone_send() // Sekwencja burst klonowania
```

---

## 📋 Format logów (NRF Radar)

Plik: `/radar_dron_nrf.txt` na karcie SD

```
[123s] SYMA X5/X8 | Ch:34
[156s] BAYANG/E010 | Ch:40
[189s] MJX BUGS 3 | Ch:50
```

---

## 🔬 Protokół Remote ID (WiFi)

### Sygnatura beacona

```
Offset  Wartość   Opis
0x00    0xFA      Vendor OUI byte 1
0x01    0x0B      Vendor OUI byte 2  
0x02    0xBC      Vendor OUI byte 3
0x03    0x0D      Protocol version
0x04    Counter   Message counter
0x05+   Messages  25-byte message blocks
```

### Struktura wiadomości (25 bajtów)

```
Byte 0: [Type:4][SubType:4]
Byte 1: Flags/Length
Byte 2-24: Payload (zależy od typu)
```

---

## ⚠️ Ostrzeżenia

1. **Jamming jest nielegalny** – Funkcja zakłócania (fire_jammer) narusza przepisy telekomunikacyjne w UE, USA i większości krajów.

2. **Klonowanie Remote ID jest nielegalne** – Podszywanie się pod identyfikator drona może być karane jako przestępstwo.

3. **Tylko do badań** – Moduły przeznaczone do testów w kontrolowanym środowisku laboratoryjnym.

4. **Brak gwarancji** – Kod dostarczony "as is", bez żadnych gwarancji.

---

## 📄 Licencja

MIT License – Zobacz plik LICENSE

---

## 🤝 Współtworzenie

1. Fork repozytorium
2. Utwórz branch: `git checkout -b feature/nowa-funkcja`
3. Commit: `git commit -m 'Dodano nową funkcję'`
4. Push: `git push origin feature/nowa-funkcja`
5. Otwórz Pull Request

---

## 📚 Powiązane projekty

- [Bruce Firmware](https://github.com/pr3y/Bruce) – Główny projekt
- [RF24 Library](https://github.com/nRF24/RF24) – Biblioteka NRF24L01
- [ASTM F3411](https://www.astm.org/f3411-22a.html) – Standard Remote ID

---

*Ostatnia aktualizacja: Styczeń 2026*
