#pragma once
#include "Settings.h"

enum class Phase : uint8_t { Start, Follow, Search, ScanLeft, LookLeft, ScanRight,
  LookRight, Recenter, TurnOut, Offset, Parallel, Pass, TurnIn, Return, CenterAxle, Align,
  Finish, Done, Fault };
struct Line {
  uint8_t mask;       // bit 0 is the leftmost detector
  float error;        // positive toward the right
};
struct Scene {
  uint32_t now;
  float travel;       // cumulative average absolute wheel travel, including turns
  Line line;
  uint16_t left, right; // fresh median ranges in mm; no return is represented by 2000
  bool fresh;
};
struct Wheels { int left, right; };
class Race {
public:
  void start(const Scene &s);
  Wheels step(const Scene &s);
  Phase phase() const { return mode; }
private:
  Phase mode = Phase::Fault;
  uint32_t entered = 0, lastTime = 0;
  float origin = 0, lastError = 0, derivative = 0, sweptAt[5] = {};
  uint8_t swept = 0, stable = 0;
  int8_t away = 1; // +1 turns right
  uint16_t leftLook = 0;
  void enter(Phase next, const Scene &s);
  Wheels follow(const Scene &s, bool reduced);
  bool finishStripe(const Scene &s);
  Wheels arcStep(const Scene &s);
};
