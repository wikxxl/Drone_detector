#include "drone_hunter.h"
#include "core/display.h"
#include "core/wifi/wifi_common.h"
#include "core/mykeyboard.h"
#include <globals.h>

extern uint32_t packet_counter;
extern bool returnToMenu;

std::vector<RemoteID_Drone> droneList;
RemoteID_Drone activeCloneTarget; // Snapshot do klonowania
HunterState currentState = SCAN_LIST;
int selectedIdx = 0;
uint32_t lastHop = 0;
uint32_t lastUI = 0;
uint8_t huntCh = 1;
bool hunterSentry = false;
bool forceScreenRefresh = true;
uint32_t lastDetectionTime = 0;

const DroneOUI drone_database[] = {
    // DJI & Subsidiaries
	{{0x60, 0x60, 0x1F}, "DJI", "Phantom/Mavic/Spark/Tello"},
	{{0x62, 0x60, 0x1F}, "DJI", "Phantom 2 Vision"},
	{{0x04, 0xA8, 0x5A}, "DJI", "Mavic 3/Air 3/Mini 4"},
	{{0x0C, 0x9A, 0xE6}, "DJI", "New models (2025)"},
	{{0x34, 0xD2, 0x62}, "DJI", "Mavic 2/Phantom 4"},
	{{0x48, 0x1C, 0xB9}, "DJI", "Mini 2/Mini 3"},
	{{0x4C, 0x43, 0xF6}, "DJI", "New models (2025)"},
	{{0x58, 0xB8, 0x58}, "DJI", "RC Controllers"},
	{{0x88, 0x29, 0x85}, "DJI", "Enterprise Series"},
	{{0x8C, 0x58, 0x23}, "DJI", "New models (2025)"},
	{{0xE4, 0x7A, 0x2C}, "DJI", "Avata/FPV Series"},
	{{0x44, 0x44, 0xDA}, "DJI BAIWANG", "Mini Series"},

	// Parrot
	{{0x00, 0x12, 0x1C}, "PARROT", "AR.Drone/Bebop/Anafi"},
	{{0x00, 0x26, 0x7E}, "PARROT", "AR.Drone 2.0/Bebop 2"},
	{{0x90, 0x03, 0xB7}, "PARROT", "Disco/Mambo/Swing"},
	{{0x90, 0x3A, 0xE6}, "PARROT", "Anafi/Anafi USA"},
	{{0xA0, 0x14, 0x3D}, "PARROT", "Bebop/Disco FPV"},

	// Autel & Skydio
	{{0xEC, 0x5B, 0xCD}, "AUTEL", "EVO I/II/Max"},
	{{0x8C, 0xDE, 0xF9}, "AUTEL", "EVO Lite/Nano"},
	{{0x38, 0x1D, 0x14}, "SKYDIO", "Skydio 2/X2"},

	// Xiaomi / FIMI
	{{0x64, 0xCC, 0x2E}, "FIMI", "X8 SE/Mi Drone 4K"},
	{{0x78, 0x11, 0xDC}, "FIMI", "A3/FIMI Mini"},
	{{0x7C, 0x49, 0xEB}, "XIAOMI", "Mi Drone 1080P"},
	{{0xF8, 0xA2, 0xD6}, "XIAOMI", "FIMI X8 SE"},
	{{0x50, 0xEC, 0x50}, "FIMI", "Palm/X8 Mini"},

	// Yuneec
	{{0x00, 0xE0, 0x6D}, "YUNEEC", "Typhoon H/H520"},
	{{0x60, 0x01, 0x94}, "YUNEEC", "Mantis Q/Breeze"},

	// Hubsan & Holy Stone
	{{0xE8, 0xAB, 0xFA}, "HUBSAN", "Zino/Pro/Mini"},
	{{0x48, 0x3F, 0xDA}, "HUBSAN", "H501S/H117S"},
	{{0x48, 0x57, 0x02}, "HOLY STONE", "HS100/700/720"},
	{{0xAC, 0x0B, 0xFB}, "HOLY STONE", "HS120D/270"},

	// Budget & Others
	{{0x94, 0xE9, 0x79}, "POTENSIC", "Dreamer/D88/T25"},
	{{0xDC, 0x0D, 0x30}, "RUKO", "F11 Pro/U11 Pro"},
	{{0x34, 0x94, 0x54}, "SNAPTAIN", "SP7100/SP650"},
	{{0x18, 0xFE, 0x34}, "SYMA", "X8 Pro/X500/W1"},
	{{0x24, 0x62, 0xAB}, "EACHINE", "EX5/E520S"},
	{{0xB4, 0xE6, 0x2D}, "EACHINE", "E58/E520"},
	{{0x00, 0x1D, 0xDF}, "WALKERA", "Voyager/Vitus"},
	{{0x44, 0x33, 0x4C}, "WINGSLAND", "S6/K3"},
	{{0x98, 0x66, 0x10}, "ZEROTECH", "Dobby/Hesper"},
	{{0xBC, 0x6A, 0x16}, "EHANG", "Ghostdrone/184"},
	{{0xD4, 0xD9, 0x19}, "GOPRO", "Karma"},
	{{0x00, 0x1A, 0x2C}, "3DR", "Solo/Iris+"},
	{{0x24, 0x0A, 0xC4}, "POWERVISION", "PowerEgg/PowerEye"},
	{{0x00, 0x80, 0x91}, "SENSEFLY", "eBee/Albris"},
	{{0x84, 0xF3, 0xEB}, "BETAFPV", "Cetus/Meteor"},
	{{0xA4, 0xCF, 0x12}, "EMAX", "Tinyhawk/Nanohawk"},

	// DIY & Modules (Espressif)
	{{0x24, 0x0A, 0xC4}, "DIY (ESP)", "ESP8266/32 Drone"},
	{{0x30, 0xAE, 0xA4}, "DIY (ESP)", "ESP32 Drone"},
	{{0xBC, 0xDD, 0xC2}, "DIY (ESP)", "ESP32 Drone"},
	{{0xCC, 0x50, 0xE3}, "DIY (ESP)", "ESP Module Drone"},

	// Remote ID & FPV Goggles
	{{0xC8, 0x8D, 0x83}, "FATSHARK", "HDO/Scout"},
	{{0x00, 0x0E, 0x8F}, "ORQA", "FPV.One Goggles"},
	{{0x00, 0x1B, 0x44}, "DRONETAG", "RemoteID Module"}
};
const int db_size = sizeof(drone_database) / sizeof(drone_database[0]);

// --- SNIFFER ---
void drone_hunter_sniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
    packet_counter++;
    if (type != WIFI_PKT_MGMT || currentState == CLONING) return;
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t* payload = pkt->payload;
    int len = pkt->rx_ctrl.sig_len;

    for (int i = 0; i < len - 25; i++) {
        if (payload[i] == 0xFA && payload[i+1] == 0x0B && payload[i+2] == 0xBC) {
            uint8_t* mac = &payload[10];
            RemoteID_Drone* target = nullptr;

            for (auto &d : droneList) {
                if (memcmp(d.mac, mac, 6) == 0) { target = &d; break; }
            }

            if (!target && droneList.size() < 6) {
                RemoteID_Drone newDrone;
                memcpy(newDrone.mac, mac, 6);
                char mBuf[20];
                sprintf(mBuf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                newDrone.macStr = String(mBuf);
                newDrone.brand = "Unknown";
                droneList.push_back(newDrone);
                target = &droneList.back();
            }

            if (target) {
                target->rssi = pkt->rx_ctrl.rssi;
                target->lastSeen = millis();
                target->channel = pkt->rx_ctrl.channel;
                memcpy(target->rawPayload, &payload[i+5], 16);

                for (int d=0; d<db_size; d++) {
                    if (mac[0] == drone_database[d].oui[0] && mac[1] == drone_database[d].oui[1]) {
                        target->brand = String(drone_database[d].brand);
                        target->model = String(drone_database[d].model);
                        break;
                    }
                }

                int ptr = i + 5;
                while (ptr + 25 <= len) {
                    uint8_t mType = payload[ptr] >> 4;
                    if (mType == 0x0) { char id[21]={0}; memcpy(id, &payload[ptr+2], 20); target->uavID = String(id); }
                    else if (mType == 1) {
                        int32_t la = payload[ptr+5]|(payload[ptr+6]<<8)|(payload[ptr+7]<<16)|(payload[ptr+8]<<24);
                        int32_t lo = payload[ptr+9]|(payload[ptr+10]<<8)|(payload[ptr+11]<<16)|(payload[ptr+12]<<24);
                        target->lat = la / 1e7; target->lon = lo / 1e7;
                        target->altBaro = ((payload[ptr+15]|(payload[ptr+16]<<8)) * 0.5f) - 1000.0f;
                        target->speedH = payload[ptr+3] * 0.25f * 3.6f;
                    }
                    else if (mType == 4) {
                        int32_t pla = payload[ptr+2]|(payload[ptr+3]<<8)|(payload[ptr+4]<<16)|(payload[ptr+5]<<24);
                        int32_t plo = payload[ptr+6]|(payload[ptr+7]<<8)|(payload[ptr+8]<<16)|(payload[ptr+9]<<24);
                        target->opLat = pla / 1e7; target->opLon = plo / 1e7;
                    }
                    else if (mType == 5) { // Operator ID
                        char opid[21]={0}; memcpy(opid, &payload[ptr+2], 20); target->opID = String(opid);
                    }
                    ptr += 25;
                }
                if (hunterSentry) drone_hunter_clone_send(*target);
            }
        }
    }
}

void drone_hunter_setup() {
    returnToMenu = false;
    droneList.clear();
    currentState = SCAN_LIST;
    selectedIdx = 0;
    forceScreenRefresh = true;
    WiFi.mode(WIFI_STA); WiFi.disconnect();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(drone_hunter_sniffer);
}

// Flaga pomocnicza - czy ekran CLONING został narysowany
static bool cloneScreenDrawn = false;

void drone_hunter_loop() {
    while (!returnToMenu) {
        uint32_t now = millis();

        // CLEANUP - tylko gdy NIE klonujemy (podczas klonowania sniffer jest wyłączony)
        if (currentState != CLONING) {
            for (int i = droneList.size() - 1; i >= 0; i--) {
                if (now - droneList[i].lastSeen > 10000) {
                    droneList.erase(droneList.begin() + i);
                    if (selectedIdx >= (int)droneList.size() + 1) selectedIdx = 0;
                    forceScreenRefresh = true;
                }
            }
        }

        // ESC = WYJŚCIE DO MENU BRUCE
        if (check(EscPress)) { returnToMenu = true; break; }

        // PREV = POWRÓT O JEDNO OKNO / W GÓRĘ LISTY (nie dla CLONING - obsługiwane osobno)
        if (check(PrevPress) && currentState != CLONING) {
            if (currentState == SCAN_LIST) { if(selectedIdx > 0) selectedIdx--; lastUI = 0; }
            else if (currentState == DETAILS) { currentState = SCAN_LIST; forceScreenRefresh = true; }
        }

        if (check(NextPress)) {
            if (currentState == SCAN_LIST) { selectedIdx++; if (selectedIdx > (int)droneList.size()) selectedIdx = 0; lastUI = 0; }
        }

        if (check(SelPress)) {
            if (currentState == SCAN_LIST) {
                if (selectedIdx == 0) { hunterSentry = !hunterSentry; lastUI = 0; }
                else if (selectedIdx <= (int)droneList.size()) { currentState = DETAILS; forceScreenRefresh = true; }
            } else if (currentState == DETAILS) {
                activeCloneTarget = droneList[selectedIdx-1]; // RÓB SNAPSHOT DANYCH
                currentState = CLONING;
                cloneScreenDrawn = false;
                forceScreenRefresh = false; // Wyłącz wymuszone odświeżanie
                esp_wifi_set_promiscuous(false);
                esp_wifi_set_channel(activeCloneTarget.channel, WIFI_SECOND_CHAN_NONE);
            }
        }

        // === SCAN_LIST ===
        if (currentState == SCAN_LIST) {
            if (forceScreenRefresh) {
                tft.fillScreen(bruceConfig.bgColor);
                drawMainBorderWithTitle("drone radar", false);
                forceScreenRefresh = false;
                lastUI = 0;
            }
            if (now - lastHop > 250) { huntCh++; if (huntCh > 13) huntCh = 1; esp_wifi_set_channel(huntCh, WIFI_SECOND_CHAN_NONE); lastHop = now; }
            if (now - lastUI > 300) {
                tft.setCursor(50, 46); tft.setTextSize(FM);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.printf("SCAN CH: %d ", huntCh);

                int yStart = 70; int itemH = 24;
                if (selectedIdx == 0) { tft.fillRoundRect(5, yStart, tftWidth-10, itemH, 4, TFT_ORANGE); tft.setTextColor(TFT_BLACK); }
                else { tft.drawRoundRect(5, yStart, tftWidth-10, itemH, 4, TFT_ORANGE); tft.fillRect(6, yStart+1, tftWidth-12, itemH-2, bruceConfig.bgColor); tft.setTextColor(TFT_ORANGE); }
                tft.setCursor(15, yStart + 5); tft.setTextSize(1);
                tft.printf("AUTO-CLONE (SENTRY): %s", hunterSentry ? "ON" : "OFF");

                tft.fillRect(5, yStart + 28, tftWidth-10, 135, bruceConfig.bgColor);
                for (int i=0; i<(int)droneList.size(); i++) {
                    int y = yStart + ((i + 1) * (itemH + 4));
                    if ((i + 1) == selectedIdx) { tft.fillRoundRect(5, y, tftWidth-10, itemH, 4, bruceConfig.priColor); tft.setTextColor(bruceConfig.bgColor); }
                    else { tft.drawRoundRect(5, y, tftWidth-10, itemH, 4, bruceConfig.priColor); tft.setTextColor(TFT_WHITE); }
                    tft.setCursor(12, y + 5);
                    tft.print(droneList[i].brand + " " + droneList[i].uavID.substring(0, 10));
                    tft.setCursor(tftWidth - 55, y + 5);
                    tft.printf("%d", droneList[i].rssi);
                }
                lastUI = now;
            }
        }
        // === DETAILS ===
        else if (currentState == DETAILS) {
            if (forceScreenRefresh) {
                tft.fillScreen(bruceConfig.bgColor);
                drawMainBorderWithTitle("dna details", false);
                forceScreenRefresh = false;
                lastUI = 0;
            }
            if (now - lastUI > 400) {
                // Sprawdzamy czy indeks jest nadal ważny
                if (selectedIdx - 1 < (int)droneList.size() && selectedIdx > 0) {
                    RemoteID_Drone& d = droneList[selectedIdx - 1];
                    esp_wifi_set_channel(d.channel, WIFI_SECOND_CHAN_NONE);

                    tft.drawRect(10, 42, 140, 10, TFT_WHITE);
                    int barW = map(constrain(d.rssi, -100, -30), -100, -30, 0, 138);
                    tft.fillRect(11, 43, barW, 8, (d.rssi > -70) ? TFT_GREEN : (d.rssi > -85) ? TFT_YELLOW : TFT_RED);
                    tft.fillRect(11+barW, 43, 138-barW, 8, bruceConfig.bgColor);
                    tft.setCursor(160, 43); tft.setTextColor(TFT_RED, bruceConfig.bgColor); tft.printf("LOCKED CH:%d (%ddBm)", d.channel, d.rssi);

                    tft.setCursor(10, 58); tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor); tft.print("DNA PAYLOAD (HEX):");
                    for (int i = 0; i < 16; i++) {
                        int xP = 10 + ((i % 8) * 25);
                        int yP = 70 + ((i / 8) * 10);
                        tft.fillRect(xP, yP, 20, 9, bruceConfig.bgColor);
                        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
                        tft.setCursor(xP, yP); tft.printf("%02X", d.rawPayload[i]);
                    }

                    tft.fillRect(10, 100, tftWidth-20, 95, bruceConfig.bgColor);
                    tft.setCursor(10, 100); tft.setTextColor(TFT_GREEN); tft.print("NAME: " + d.brand + " " + d.model);
                    tft.setCursor(10, 113); tft.setTextColor(TFT_YELLOW); tft.print("SN:   " + d.uavID);
                    tft.setCursor(10, 126); tft.setTextColor(TFT_ORANGE); tft.print("MAC:  " + d.macStr);
                    tft.setCursor(10, 142); tft.setTextColor(TFT_CYAN); tft.printf("DRN: %.5f, %.5f", d.lat, d.lon);
                    tft.setCursor(10, 155); tft.setTextColor(TFT_WHITE); tft.printf("ALT: %.0fm | SPD: %.0fkm/h", d.altBaro, d.speedH);
                    tft.setCursor(10, 168); tft.setTextColor(TFT_MAGENTA);
                    if (d.opLat != 0) tft.printf("PLT: %.5f, %.5f", d.opLat, d.opLon); else tft.print("PLT: NO FIX");

                    tft.setCursor(10, 195); tft.setTextColor(bruceConfig.priColor); tft.print("[SEL] CLONE | [PREV] RADAR");
                } else {
                    // Dron zniknął z listy, wracamy do skanera
                    currentState = SCAN_LIST;
                    forceScreenRefresh = true;
                }
                lastUI = now;
            }
        }
        // === CLONING ===
        else if (currentState == CLONING) {
            // Rysuj ekran tylko raz
            if (!cloneScreenDrawn) {
                tft.fillScreen(TFT_RED);
                drawMainBorderWithTitle("iclone active", false);

                int y = 45;
                tft.setTextColor(TFT_WHITE, TFT_RED);
                tft.drawCentreString("TRANSMITTING DNA", tftWidth/2, y, 2);

                y += 28;
                tft.setTextColor(TFT_YELLOW, TFT_RED);
                tft.setCursor(10, y); tft.print("MAC: " + activeCloneTarget.macStr);

                y += 16;
                tft.setTextColor(TFT_CYAN, TFT_RED);
                tft.setCursor(10, y); tft.print("ID:  " + activeCloneTarget.uavID);

                y += 16;
                tft.setTextColor(TFT_GREEN, TFT_RED);
                tft.setCursor(10, y); tft.print("OP:  " + activeCloneTarget.opID);

                y += 16;
                tft.setTextColor(TFT_WHITE, TFT_RED);
                tft.setCursor(10, y); tft.printf("DRN: %.5f, %.5f", activeCloneTarget.lat, activeCloneTarget.lon);

                y += 16;
                tft.setCursor(10, y); tft.printf("PLT: %.5f, %.5f", activeCloneTarget.opLat, activeCloneTarget.opLon);

                y += 16;
                tft.setCursor(10, y); tft.printf("ALT: %.0fm  SPD: %.0fkm/h  CH: %d", activeCloneTarget.altBaro, activeCloneTarget.speedH, activeCloneTarget.channel);

                tft.setTextColor(TFT_YELLOW, TFT_RED);
                tft.setCursor(10, 195); tft.print("[PREV] STOP CLONE");

                cloneScreenDrawn = true;
            }

            // Klonowanie - zwraca false jeśli przerwano
            if (!drone_hunter_clone_send(activeCloneTarget)) {
                currentState = DETAILS;
                esp_wifi_set_promiscuous(true);
                forceScreenRefresh = true;
            }
            continue;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    esp_wifi_set_promiscuous(false);
}

// Pomocnicze funkcje do kodowania
static void putLatLon(uint8_t* buf, double val) {
    int32_t enc = (int32_t)(val * 10000000.0);
    memcpy(buf, &enc, 4);
}

static void putAlt(uint8_t* buf, float val) {
    uint16_t enc = (uint16_t)((val + 1000.0) * 2.0);
    memcpy(buf, &enc, 2);
}

static uint8_t cloneCounter = 0;

// Wysyła pojedynczy beacon Remote ID danego typu
static void sendCloneBeacon(const RemoteID_Drone& target, uint8_t msgType) {
    uint8_t packet[256];
    memset(packet, 0, sizeof(packet));

    uint8_t header[] = {
        0x80, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x64, 0x00,
        0x01, 0x00
    };

    memcpy(&header[10], target.mac, 6);
    memcpy(&header[16], target.mac, 6);
    memcpy(packet, header, sizeof(header));
    int ptr = sizeof(header);

    packet[ptr++] = 0;
    packet[ptr++] = 5;
    memcpy(&packet[ptr], "CLONE", 5);
    ptr += 5;

    packet[ptr++] = 221;
    packet[ptr++] = 30;
    packet[ptr++] = 0xFA;
    packet[ptr++] = 0x0B;
    packet[ptr++] = 0xBC;
    packet[ptr++] = 0x0D;
    packet[ptr++] = cloneCounter++;

    uint8_t* msg = &packet[ptr];
    memset(msg, 0, 25);

    switch(msgType) {
        case 0:
            if (target.uavID.length() > 0) {
                msg[0] = 0x00;
                msg[1] = 0x12;
                int snLen = target.uavID.length();
                if (snLen > 20) snLen = 20;
                memcpy(&msg[2], target.uavID.c_str(), snLen);
            }
            break;

        case 1:
            if (target.lat != 0.0 || target.lon != 0.0) {
                msg[0] = 0x10;
                msg[1] = 0x20;
                msg[3] = 20;
                putLatLon(&msg[5], target.lat);
                putLatLon(&msg[9], target.lon);
                putAlt(&msg[15], target.altBaro);
                putAlt(&msg[17], -1000.0f);
                msg[23] = (9 << 4) | 3;
            }
            break;

        case 4:
            if (target.opLat != 0.0 || target.opLon != 0.0) {
                msg[0] = 0x40;
                msg[1] = 0x01;
                putLatLon(&msg[2], target.opLat);
                putLatLon(&msg[6], target.opLon);
                putAlt(&msg[12], 0.0f);
            }
            break;

        case 5:
            msg[0] = 0x50;
            msg[1] = 0x00;
            if (target.opID.length() > 0) {
                int opLen = target.opID.length();
                if (opLen > 20) opLen = 20;
                memcpy(&msg[2], target.opID.c_str(), opLen);
            } else if (target.uavID.length() > 0) {
                int opLen = target.uavID.length();
                if (opLen > 20) opLen = 20;
                memcpy(&msg[2], target.uavID.c_str(), opLen);
            }
            break;
    }

    ptr += 25;
    esp_wifi_80211_tx(WIFI_IF_STA, packet, ptr, true);
}

// Główna funkcja klonowania - sekwencja BURST
bool drone_hunter_clone_send(const RemoteID_Drone& target) {
    sendCloneBeacon(target, 0);
    vTaskDelay(8 / portTICK_PERIOD_MS);
   // if (check(PrevPress)) return false;

    sendCloneBeacon(target, 1);
    vTaskDelay(8 / portTICK_PERIOD_MS);
    //if (check(PrevPress)) return false;

    sendCloneBeacon(target, 4);
    vTaskDelay(8 / portTICK_PERIOD_MS);
    //if (check(PrevPress)) return false;

    sendCloneBeacon(target, 5);
    vTaskDelay(80 / portTICK_PERIOD_MS);
    if (check(PrevPress)) return false;

    return true;
}

void wifi_drone_hunter() {
    drone_hunter_setup();
    drone_hunter_loop();
}
