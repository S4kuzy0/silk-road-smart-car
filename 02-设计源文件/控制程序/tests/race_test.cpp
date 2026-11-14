#include "../SilkRoad/Race.h"
#include <cassert>
#include <cstdio>
#include <cmath>

struct Rig {
  Race car;
  Scene s{0,0,{31,0},800,800,true};
  Wheels cmd{0,0};
  Rig() { car.start(s); }
  void tick(float mm=1) { s.now+=10; s.travel+=mm; cmd=car.step(s); }
  void steps(int n,float mm=1) { while(n--) tick(mm); }
  void armed() {
    steps(15); assert(car.phase()==Phase::Start);
    s.line={4,0}; steps(70);
    assert(car.phase()==Phase::Follow);
  }
  void advance(Phase from,Phase next,float mm=1) {
    assert(car.phase()==from);
    for(int i=0;i<700 && car.phase()==from;++i) tick(mm);
    assert(car.phase()==next);
  }
};

void bypass(bool leftObstacle) {
  Rig r; r.armed();
  r.s.left=leftObstacle?130:800; r.s.right=leftObstacle?800:130; r.tick();
  assert(r.car.phase()==Phase::TurnOut && r.cmd.left==0);
  r.tick(); assert((r.cmd.left>0)==leftObstacle && r.cmd.left==-r.cmd.right);
  r.s.left=r.s.right=800; r.s.line={0,0};
  r.advance(Phase::TurnOut,Phase::Offset);
  r.advance(Phase::Offset,Phase::Parallel);
  r.advance(Phase::Parallel,Phase::Pass);
  r.advance(Phase::Pass,Phase::TurnIn);
  r.advance(Phase::TurnIn,Phase::Return);
  r.steps(100); r.s.line={4,0}; r.steps(3);
  r.advance(Phase::CenterAxle,Phase::Align);
  r.advance(Phase::Align,Phase::Follow);
}
int main() {
  { Rig r; r.armed(); assert(r.cmd.left>0 && r.cmd.right>0); puts("PASS start stripe lockout"); }
  bypass(true); puts("PASS left obstacle / right detour and same-heading reacquisition");
  bypass(false); puts("PASS right obstacle / left detour and same-heading reacquisition");
  { Rig r; r.armed(); r.s.line={16,20}; r.tick(); assert(r.cmd.left>r.cmd.right);
    r.s.line={0,0}; r.tick(); r.tick(); assert(r.cmd.left>0 && r.cmd.right<0);
    r.s.line={8,10}; r.tick(); assert(r.car.phase()==Phase::Follow); puts("PASS line loss and recovery"); }
  { Rig r; r.armed(); r.s.line={0,0}; r.steps(80); assert(r.car.phase()==Phase::Fault); puts("PASS bounded lost-line search"); }
  { Rig r; r.armed(); r.s.fresh=false; r.tick(); assert(r.car.phase()==Phase::Fault && !r.cmd.left && !r.cmd.right);
    r.s.fresh=true; r.tick(); assert(r.car.phase()==Phase::Fault); puts("PASS stale range locks brakes"); }
  { Rig r; r.armed(); r.s.line={1,-20}; r.tick(); r.s.line={3,-15}; r.tick();
    r.s.line={6,-5}; r.tick(); r.s.line={12,5}; r.tick(); r.s.line={24,15}; r.tick();
    assert(r.car.phase()==Phase::Follow); puts("PASS sweeping narrow line is not a finish"); }
  { Rig r; r.armed(); r.s.line={7,-10}; r.tick(); r.s.line={15,-5}; r.tick();
    r.s.line={30,5}; r.tick(); assert(r.car.phase()==Phase::Finish);
    r.advance(Phase::Finish,Phase::Done); r.steps(20); assert(!r.cmd.left && !r.cmd.right);
    puts("PASS oblique finish and latched brake with zero obstacles counted"); }
  { Rig r; r.armed(); r.s.line={31,0}; r.tick(); assert(r.car.phase()==Phase::Finish);
    r.steps(130,0); assert(r.car.phase()==Phase::Fault); puts("PASS finish travel timeout"); }
  { Rig r; r.armed(); r.s.left=r.s.right=145; r.tick(); r.advance(Phase::ScanLeft,Phase::LookLeft);
    r.advance(Phase::LookLeft,Phase::ScanRight,0); r.advance(Phase::ScanRight,Phase::LookRight);
    r.advance(Phase::LookRight,Phase::Fault,0); puts("PASS ambiguous blocked scan stops"); }
  { Rig r; r.armed(); r.s.left=r.s.right=145; r.tick(); r.advance(Phase::ScanLeft,Phase::LookLeft);
    r.s.left=600; r.advance(Phase::LookLeft,Phase::ScanRight,0);
    r.advance(Phase::ScanRight,Phase::LookRight); r.advance(Phase::LookRight,Phase::Recenter,0);
    r.advance(Phase::Recenter,Phase::TurnOut); r.tick(); assert(r.cmd.left<0); puts("PASS scan chooses confirmed free side"); }
  { Rig r; r.armed(); r.s.left=120; r.tick(); r.advance(Phase::TurnOut,Phase::Offset);
    r.s.right=50; r.tick(); assert(r.car.phase()==Phase::Fault); puts("PASS close hazard during detour"); }
  { Rig r; r.steps(250); assert(r.car.phase()==Phase::Fault); puts("PASS start without acquired route stops"); }
}
