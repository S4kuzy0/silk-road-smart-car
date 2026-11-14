#include "Hardware.h"

Race race;
bool running=false, locked=false, released=false;
uint32_t changedAt=0, lastTick=0;
bool lastButton=false;

void setup() {
  driveBegin();
  pinMode(pins::start,INPUT_PULLUP);
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.begin(115200);
  sensorsBegin();
  Serial.println(F("Bench: W=background B=black ?=readings. Start with D4 button."));
}
void loop() {
  uint32_t now=millis();
  pollRange(now);
  if (!running && !locked && Serial.available()) {
    char cmd=Serial.read();
    if (cmd=='W' || cmd=='B') {
      captureSurface(cmd=='B');
      Serial.println(calibrated()?F("Calibration ready"):F("Need both contrasting surfaces"));
    }
    if (cmd=='?') {
      rawLine();
      Serial.print(F("| L/R mm ")); Serial.print(distanceMm(0)); Serial.print(' ');
      Serial.print(distanceMm(1)); Serial.print(F(" fresh=")); Serial.print(rangesFresh(now));
      Serial.print(F(" calibrated=")); Serial.print(calibrated());
      Serial.print(F(" edges L/R=")); Serial.print(wheelEdges(0)); Serial.print(' '); Serial.print(wheelEdges(1));
      Serial.print(F(" wheel mm=")); Serial.println(travelMm());
    }
  }
  if (now-lastTick<cfg::controlMs) return;
  lastTick=now;
  Scene s={now,travelMm(),readLine(),distanceMm(0),distanceMm(1),rangesFresh(now)};
  const bool pressed=digitalRead(pins::start)==LOW;
  if (lastButton!=pressed) { lastButton=pressed; changedAt=now; }
  if (!pressed && now-changedAt>=40) released=true;
  if (!running && !locked && released && pressed && now-changedAt>=40) {
    released=false;
    if (calibrated() && s.fresh) { race.start(s); running=true; }
  }
  if (running) {
    driveUpdate(race.step(s),now);
    if (race.phase()==Phase::Done || race.phase()==Phase::Fault || wheelsStalled()) {
      driveUpdate({0,0},now);
      locked=true;
      running=false;
      Serial.println(race.phase()==Phase::Done && !wheelsStalled()?F("FINISH"):F("FAULT"));
    }
  }
  digitalWrite(LED_BUILTIN,locked ? ((race.phase()==Phase::Done && !wheelsStalled()) || (now/150)%2) :
               (running || (calibrated() && s.fresh && (now/600)%2)));
}
