#include "Race.h"
#include <math.h>
#include <stdlib.h>

namespace {
Wheels stop() { return {0, 0}; }
Wheels pivot(int direction) { return {direction * cfg::crawl, -direction * cfg::crawl}; }
Wheels forward(int speed) { return {speed, speed}; }
bool narrow(uint8_t mask) {
  return mask && !(mask & (mask >> 1) & (mask >> 2) & (mask >> 3))
              && !(mask & 1 && mask & 16);
}
bool intrudes(uint16_t range) {
  // Parallel optical axes. Reject a return whose beam cannot overlap the car corridor.
  const float radians = cfg::sensorAngleDeg * 0.017453293f;
  return range * cosf(radians) <= cfg::detectDistance &&
         cfg::sensorOffset + range * sinf(radians) <= cfg::bodyWidth / 2 + cfg::clearance;
}
}

void Race::enter(Phase next, const Scene &s) {
  mode = next;
  origin = s.travel;
  entered = s.now;
  stable = 0;
}
void Race::start(const Scene &s) {
  swept = 0;
  lastError = derivative = 0;
  lastTime = s.now;
  enter(Phase::Start, s);
}
Wheels Race::follow(const Scene &s, bool reduced) {
  const float dt = (s.now - lastTime) * 0.001f;
  if (dt > 0 && dt < 0.1f)
    derivative += 0.25f * ((s.line.error - lastError) / dt - derivative);
  else derivative = 0;
  lastTime = s.now;
  lastError = s.line.error;
  const float steer = fmaxf(-90, fminf(90, cfg::kp * lastError + cfg::kd * derivative));
  const int speed = reduced || fabsf(lastError) > 8 ? cfg::corner : cfg::cruise;
  return {int(speed + steer), int(speed - steer)};
}
bool Race::finishStripe(const Scene &s) {
  // Require a broad simultaneous signature as well as both outer probes in a
  // short distance window. A narrow line weaving sideways must not trigger it.
  for (uint8_t i = 0; i < 5; ++i) {
    const uint8_t bit = 1u << i;
    if (s.line.mask & bit) { swept |= bit; sweptAt[i] = s.travel; }
    else if (s.travel - sweptAt[i] > 20) swept &= ~bit;
  }
  const uint8_t m = s.line.mask;
  const bool broad = (m & 15) == 15 || (m & 30) == 30 || ((m & 17) == 17 && (m & 14));
  if (broad) stable = 5;
  else if (stable) --stable;
  return swept == 31 && stable;
}
Wheels Race::arcStep(const Scene &s) {
  const float d = s.travel - origin;
  if (s.now - entered > 6500) { enter(Phase::Fault,s); return stop(); }
  // Stop on an unexpected close return during translational motion.
  if ((mode == Phase::Offset || mode == Phase::Pass || mode == Phase::Return || mode == Phase::CenterAxle) &&
      (s.left <= cfg::hazardStop || s.right <= cfg::hazardStop)) {
    enter(Phase::Fault,s); return stop();
  }
  switch (mode) {
    case Phase::ScanLeft:
      if (d < cfg::turnTravel(18)) return pivot(-1);
      enter(Phase::LookLeft,s); break;
    case Phase::LookLeft:
      if (s.now - entered >= 220) { leftLook = s.left; enter(Phase::ScanRight,s); }
      break;
    case Phase::ScanRight:
      if (d < cfg::turnTravel(36)) return pivot(1);
      enter(Phase::LookRight,s); break;
    case Phase::LookRight:
      if (s.now - entered >= 220) {
        if (leftLook < cfg::clearSideMm && s.right < cfg::clearSideMm) { enter(Phase::Fault,s); break; }
        away = s.right >= leftLook ? 1 : -1;
        enter(Phase::Recenter,s);
      }
      break;
    case Phase::Recenter:
      if (d < cfg::turnTravel(18)) return pivot(-1);
      enter(Phase::TurnOut,s); break;
    case Phase::TurnOut:
      if (d < cfg::turnTravel(45)) return pivot(away);
      enter(Phase::Offset,s); break;
    case Phase::Offset:
      if (d < cfg::lateralLeg) return forward(cfg::bypass);
      enter(Phase::Parallel,s); break;
    case Phase::Parallel:
      if (d < cfg::turnTravel(45)) return pivot(-away);
      enter(Phase::Pass,s); break;
    case Phase::Pass:
      if (d < cfg::straightPass) return forward(cfg::bypass);
      enter(Phase::TurnIn,s); break;
    case Phase::TurnIn:
      if (d < cfg::turnTravel(45)) return pivot(-away);
      enter(Phase::Return,s); break;
    case Phase::Return:
      if ((s.line.mask & 4) && narrow(s.line.mask)) ++stable; else stable = 0;
      if (stable >= 3) { enter(Phase::CenterAxle,s); break; }
      if (d >= cfg::lateralLeg + 140) { enter(Phase::Fault,s); break; }
      return forward(cfg::crawl);
    case Phase::CenterAxle:
      // The axle must reach the point where the central probe found the line
      // before yawing back toward the original course tangent.
      if (d < cfg::axleToLine) return forward(cfg::crawl);
      enter(Phase::Align,s); break;
    case Phase::Align:
      if (d < cfg::turnTravel(30)) return pivot(away);
      if (narrow(s.line.mask) && fabsf(s.line.error) <= 12) ++stable; else stable = 0;
      if (stable >= 3) { swept = 0; enter(Phase::Follow,s); break; }
      if (d > cfg::turnTravel(70)) { enter(Phase::Fault,s); break; }
      if (stable) return stop();
      return pivot(away);
    default: break;
  }
  return stop();
}
Wheels Race::step(const Scene &s) {
  if (mode == Phase::Done || mode == Phase::Fault) return stop();
  if (!s.fresh) { enter(Phase::Fault,s); return stop(); }
  const float d = s.travel - origin;
  switch (mode) {
    case Phase::Start:
      if (s.left < cfg::hazardStop || s.right < cfg::hazardStop || d > 225 || s.now-entered > 3500) {
        enter(Phase::Fault,s); return stop();
      }
      if (narrow(s.line.mask)) ++stable; else stable = 0;
      if (d >= 75 && stable >= 4) { swept = 0; enter(Phase::Follow,s); }
      return narrow(s.line.mask) ? follow(s,true) : forward(cfg::crawl);
    case Phase::Follow: {
      if (finishStripe(s)) { enter(Phase::Finish,s); return forward(cfg::crawl); }
      bool l = intrudes(s.left), r = intrudes(s.right);
      if (l || r) {
        swept = 0;
        if ((l && s.right < cfg::clearSideMm) || (r && s.left < cfg::clearSideMm))
          enter(Phase::ScanLeft,s);
        else { away = l ? 1 : -1; enter(Phase::TurnOut,s); }
        return stop();
      }
      if (!s.line.mask) { enter(Phase::Search,s); return stop(); }
      return follow(s,false);
    }
    case Phase::Search:
      if (s.left < cfg::hazardStop || s.right < cfg::hazardStop || d > 58 || s.now-entered > 900) {
        enter(Phase::Fault,s); return stop();
      }
      if (narrow(s.line.mask)) { swept = 0; enter(Phase::Follow,s); return follow(s,true); }
      return pivot(lastError >= 0 ? 1 : -1);
    case Phase::Finish:
      if (d >= fmaxf(0, cfg::finishNoseOver - cfg::noseAheadOfLine - cfg::brakeTravel))
        enter(Phase::Done,s);
      else if (s.now-entered > 1200) enter(Phase::Fault,s);
      else return forward(cfg::crawl);
      return stop();
    default: return arcStep(s);
  }
}
