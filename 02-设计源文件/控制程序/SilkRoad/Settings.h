#pragma once
#include <stdint.h>

// Geometry in mm; rates in mm/s. Update from the loaded car on the actual mat.
namespace cfg {
constexpr float wheelDiameter = 65.0f, track = 130.0f;
constexpr uint8_t slots = 20;
constexpr float mmPerEdge = 3.141593f * wheelDiameter / (2 * slots); // CHANGE interrupts
constexpr float bodyWidth = 157, sensorOffset = 35, sensorAngleDeg = 0;
constexpr float noseAheadOfLine = 10, brakeTravel = 12, finishNoseOver = 45;
constexpr float axleToLine = 84;
constexpr int cruise = 185, corner = 105, crawl = 78, bypass = 100;
constexpr float kp = 3.1f, kd = 0.045f; // error in mm, derivative in mm/s
constexpr float sensorPitch = 10.16f; // four perfboard pitches
constexpr uint16_t blackLevel = 570, minContrast = 100;
constexpr float detectDistance = 180, clearance = 18;
constexpr uint16_t rangeMaxAge = 180, rangePeriod = 55;
constexpr float hazardStop = 65, straightPass = 370;
constexpr uint16_t clearSideMm = 240;
constexpr float turnTravel(float degrees) { return track * 3.141593f * degrees / 360.0f; }
constexpr float lateralLeg = (bodyWidth / 2 + 75 + clearance) * 1.414214f;
constexpr uint16_t controlMs = 10;
}
