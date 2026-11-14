#include "Hardware.h"
#include <EEPROM.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <stddef.h>

namespace {
struct Calibration {
  uint16_t magic, white[5], black[5], checksum;
} cal;
bool haveWhite, haveBlack, valid;
uint16_t checksum() {
  const uint8_t *bytes = reinterpret_cast<const uint8_t*>(&cal);
  uint16_t crc=0xffff;
  for (size_t n=0;n<offsetof(Calibration,checksum);++n) {
    crc ^= bytes[n];
    for (uint8_t b=0;b<8;++b) crc = (crc & 1) ? (crc>>1)^0xa001 : crc>>1;
  }
  return crc;
}
bool contrastOK() {
  for (uint8_t i=0;i<5;++i)
    if (cal.black[i]>1023 || cal.white[i]>1023 ||
        abs(int(cal.black[i])-int(cal.white[i])) < cfg::minContrast) return false;
  return true;
}
VL53L0X tof[2];
struct Ranges {
  uint16_t window[3], filtered;
  uint8_t index, samples;
  uint32_t last;
  bool ok;
} ranges[2];
bool initialized;
uint32_t lastPoll;
uint16_t median(uint16_t a,uint16_t b,uint16_t c) {
  if (a>b) { uint16_t t=a; a=b; b=t; }
  return max(a,min(b,c));
}
bool registers(uint8_t addr, uint8_t reg, uint8_t *buf,uint8_t len) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) || Wire.requestFrom(addr,len)!=len) return false;
  for (uint8_t i=0;i<len;++i) buf[i]=Wire.read();
  return !Wire.getWireTimeoutFlag();
}
void updateRange(uint8_t side,uint32_t now) {
  Ranges &r=ranges[side];
  uint8_t status, payload[12];
  const uint8_t addr=pins::address[side];
  Wire.clearWireTimeoutFlag();
  if (!registers(addr,VL53L0X::RESULT_INTERRUPT_STATUS,&status,1)) { r.ok=false; return; }
  if (!(status & 7)) return;
  if (!registers(addr,VL53L0X::RESULT_RANGE_STATUS,payload,sizeof(payload))) { r.ok=false; return; }
  tof[side].writeReg(VL53L0X::SYSTEM_INTERRUPT_CLEAR,1);
  if (tof[side].last_status || Wire.getWireTimeoutFlag()) { r.ok=false; return; }
  uint16_t dist = (uint16_t(payload[10]) << 8) | payload[11];
  const uint8_t deviceStatus = (payload[0] >> 3) & 15;
  // A completed measurement with the usual no-target sentinel has no near echo.
  // A bus failure, hardware error or other rejected measurement is never clearance.
  if (deviceStatus == 4 && (dist == 8190 || dist == 8191)) dist=2000;
  else if (deviceStatus != 11 || dist == 0 || dist >= 8190) { r.ok=false; return; }
  r.window[r.index] = dist;
  r.index = (r.index+1)%3;
  if (r.samples<3) ++r.samples;
  if (r.samples==3) r.filtered=median(r.window[0],r.window[1],r.window[2]);
  r.last=now;
  r.ok=true;
}
}
void sensorsBegin() {
  EEPROM.get(0,cal);
  valid=cal.magic==0x5352 && cal.checksum==checksum() && contrastOK();
  haveWhite=haveBlack=valid;
  for (uint8_t p : pins::shutdown) { digitalWrite(p,LOW); pinMode(p,OUTPUT); }
  Wire.begin();
  Wire.setWireTimeout(3000,true);
  delay(3);
  for (uint8_t i=0;i<2;++i) {
    pinMode(pins::shutdown[i],INPUT); // board's 2.8 V pull-up; never drive high
    delay(5);
    tof[i].setTimeout(100);
    if (!tof[i].init()) return;
    tof[i].setAddress(pins::address[i]);
    uint8_t id;
    if (!registers(pins::address[i],0xc0,&id,1) || id!=0xee) return;
    if (!tof[i].setMeasurementTimingBudget(33000)) return;
    tof[i].startContinuous(cfg::rangePeriod);
    if (tof[i].last_status) return;
    delay(22); // stagger the two optical integrations
  }
  initialized=true;
}
bool calibrated() { return valid; }
void captureSurface(bool black) {
  // Motors are disabled during these serial bench commands.
  uint16_t *sample=black ? cal.black : cal.white;
  for (uint8_t i=0;i<5;++i) {
    uint32_t sum=0;
    for (uint8_t n=0;n<32;++n) sum+=analogRead(pins::line[i]);
    sample[i]=sum/32;
  }
  if (black) haveBlack=true; else haveWhite=true;
  valid=haveBlack && haveWhite && contrastOK();
  if (valid) { cal.magic=0x5352; cal.checksum=checksum(); EEPROM.put(0,cal); }
}
Line readLine() {
  int32_t moment=0;
  uint16_t total=0;
  uint8_t mask=0;
  if (!valid) return {0,0};
  for (uint8_t i=0;i<5;++i) {
    const int raw=analogRead(pins::line[i]);
    long strength=1000L*(raw-int(cal.white[i]))/(int(cal.black[i])-int(cal.white[i]));
    strength=constrain(strength,0L,1000L);
    if (strength>=cfg::blackLevel) mask|=1u<<i;
    const uint16_t weight=strength>180 ? strength-180 : 0;
    total+=weight;
    moment+=(int(i)-2)*int32_t(weight);
  }
  return {mask,total ? moment*cfg::sensorPitch/total : 0};
}
void rawLine() {
  for (uint8_t p:pins::line) { Serial.print(analogRead(p)); Serial.print(' '); }
}
void pollRange(uint32_t now) {
  if (!initialized || now-lastPoll<10) return;
  lastPoll=now;
  updateRange(0,now);
  updateRange(1,now);
}
bool rangesFresh(uint32_t now) {
  for (const Ranges &r:ranges)
    if (!r.ok || r.samples<3 || now-r.last>cfg::rangeMaxAge) return false;
  return initialized;
}
uint16_t distanceMm(uint8_t side) { return ranges[side].filtered; }
