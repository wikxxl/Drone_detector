#pragma once
#include <Arduino.h>
#include <esp_wifi.h>
#include <vector>

enum HunterState { SCAN_LIST, DETAILS, CLONING };

struct DroneOUI {
    uint8_t oui[3];
    const char* brand;
    const char* model;
};

struct RemoteID_Drone {
    uint8_t mac[6];
    String macStr = "";
    String brand = "Unknown";
    String model = "";
    String uavID = "Searching...";
    String opID = "";  // Operator ID (message type 5)
    double lat = 0.0, lon = 0.0;
    double opLat = 0.0, opLon = 0.0;
    float altBaro = 0.0, speedH = 0.0;
    int rssi = -100, channel = 1;
    uint32_t lastSeen = 0;
    uint8_t rawPayload[16];
};

void wifi_drone_hunter();
void drone_hunter_setup();
void drone_hunter_loop();
void drone_hunter_sniffer(void* buf, wifi_promiscuous_pkt_type_t type);
bool drone_hunter_clone_send(const RemoteID_Drone& target);

extern RemoteID_Drone activeCloneTarget; // Globalny cel klonowania
