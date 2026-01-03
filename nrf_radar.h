#ifndef __NRF_RADAR_H
#define __NRF_RADAR_H
#include "nrf_common.h"
#include <RF24.h>

/*
 * NRF Drone Radar v3.1 (Ported for Bruce)
 * Detects RC drones (Syma, Bayang, MJX, etc.) using a single NRF24 module.
 * Features: RPD Scanning, Protocol Handover, Locked Tracking, Reactive Jamming.
 */

void nrf_drone_radar();

#endif
