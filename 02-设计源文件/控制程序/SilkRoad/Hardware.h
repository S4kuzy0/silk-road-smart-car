#pragma once
#include <Arduino.h>
#include "Race.h"
namespace pins {
constexpr uint8_t line[] = {A0,A1,A2,A3,A6};
constexpr uint8_t encoder[] = {2,3}, motors[][2] = {{5,6},{9,10}};
constexpr uint8_t shutdown[] = {7,8}, start = 4;
constexpr uint8_t address[] = {0x30,0x31};
}
void driveBegin();
void driveUpdate(Wheels target, uint32_t now);
float travelMm();
uint32_t wheelEdges(uint8_t side);
bool wheelsStalled();
void sensorsBegin();
bool calibrated();
void captureSurface(bool black);
Line readLine();
void rawLine();
void pollRange(uint32_t now);
bool rangesFresh(uint32_t now);
uint16_t distanceMm(uint8_t side);
