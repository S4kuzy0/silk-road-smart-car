#include "Hardware.h"
#include <util/atomic.h>
#include <math.h>

namespace {
volatile uint32_t edges[2];
void countLeft() { ++edges[0]; }
void countRight() { ++edges[1]; }
struct WheelLoop {
  uint32_t seen = 0, lastEdge = 0, sample = 0, sampleCount = 0;
  int8_t sign = 0;
  float rate = 0, integral = 0;
} wheel[2];
bool jammed = false;
uint32_t count(uint8_t i) {
  uint32_t result;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { result = edges[i]; }
  return result;
}
void brake(uint8_t i) {
  digitalWrite(pins::motors[i][0], HIGH);
  digitalWrite(pins::motors[i][1], HIGH);
}
void updateWheel(uint8_t i, int request, uint32_t now) {
  WheelLoop &w = wheel[i];
  const uint32_t n = count(i);
  const int8_t sign = (request > 0) - (request < 0);
  if (w.sign != sign) {
    w.sign = sign;
    w.integral = w.rate = 0;
    w.lastEdge = w.sample = now;
    w.sampleCount = n;
  }
  if (n != w.seen) { w.seen = n; w.lastEdge = now; }
  if (!sign) { brake(i); return; }
  if (now-w.lastEdge > 500) { jammed = true; brake(i); return; }
  const float speed = abs(request);
  const uint32_t elapsed = now-w.sample;
  if (elapsed >= 120) {
    w.rate = (n-w.sampleCount) * cfg::mmPerEdge * 1000 / elapsed;
    w.integral = constrain(w.integral + (speed-w.rate)*elapsed*0.00055f, -55.0f, 55.0f);
    w.sample = now;
    w.sampleCount = n;
  }
  int pwm = int(40 + 0.50f*speed + 0.45f*(speed-w.rate) + w.integral);
  digitalWrite(pins::motors[i][sign > 0 ? 1 : 0],LOW);
  analogWrite(pins::motors[i][sign > 0 ? 0 : 1],constrain(pwm,0,235));
}
}
void driveBegin() {
  for (uint8_t i=0; i<2; ++i) {
    pinMode(pins::encoder[i],INPUT_PULLUP);
    for (uint8_t j=0;j<2;++j) pinMode(pins::motors[i][j],OUTPUT);
    brake(i);
  }
  attachInterrupt(digitalPinToInterrupt(pins::encoder[0]),countLeft,CHANGE);
  attachInterrupt(digitalPinToInterrupt(pins::encoder[1]),countRight,CHANGE);
}
void driveUpdate(Wheels target,uint32_t now) {
  updateWheel(0,target.left,now);
  updateWheel(1,target.right,now);
  if (jammed) { brake(0); brake(1); }
}
float travelMm() { return (count(0)+count(1))*0.5f*cfg::mmPerEdge; }
uint32_t wheelEdges(uint8_t side) { return count(side); }
bool wheelsStalled() { return jammed; }
