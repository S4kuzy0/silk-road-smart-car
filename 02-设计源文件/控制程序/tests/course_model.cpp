// Straight and gently curved planar fixtures; ideal traction, two ray sensors.
// Coordinates: x right, y forward, heading positive clockwise. Units mm and s.
#include "../SilkRoad/Race.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
struct Pose { double x=0,y=-94,h=0; };
struct Obstacle { double x,y,w,l; };
struct Point { double x,y; };
Point local(const Pose &p,double x,double y) {
  return {p.x+x*cos(p.h)+y*sin(p.h),p.y-x*sin(p.h)+y*cos(p.h)};
}
double rayHit(Point o,double dx,double dy,const Obstacle &b) {
  double near=0,far=2000;
  for(int axis=0;axis<2;++axis) {
    const double pos=axis?o.y:o.x, dir=axis?dy:dx;
    const double lo=axis?b.y:b.x,hi=lo+(axis?b.l:b.w);
    if(fabs(dir)<1e-8) { if(pos<lo || pos>hi) return 2000; continue; }
    double a=(lo-pos)/dir,c=(hi-pos)/dir;
    if(a>c) std::swap(a,c);
    near=std::max(near,a); far=std::min(far,c);
  }
  return near<=far ? near : 2000;
}
bool collision(const Pose &p,const Obstacle &b) {
  // Conservative full body rectangle measured from the axle.
  const Point centre=local(p,0,12);
  const double dx=b.x+b.w/2-centre.x,dy=b.y+b.l/2-centre.y;
  const double c=cos(p.h),s=sin(p.h),a=fabs(c),d=fabs(s);
  const double hx=cfg::bodyWidth/2,hy=82;
  return fabs(dx)<=b.w/2+a*hx+d*hy && fabs(dy)<=b.l/2+d*hx+a*hy &&
         fabs(dx*c-dy*s)<=hx+a*b.w/2+d*b.l/2 &&
         fabs(dx*s+dy*c)<=hy+d*b.w/2+a*b.l/2;
}
double lineX(double y,bool curved) { return curved ? 45*sin(y/600.0) : 0; }
Line track(const Pose &p,double finish,bool curved) {
  uint8_t mask=0;
  double sum=0; int count=0;
  for (int i=0;i<5;++i) {
    const double x=(i-2)*cfg::sensorPitch;
    Point hit=local(p,x,84);
    const bool centre=fabs(hit.x-lineX(hit.y,curved))<=10 && hit.y>=0 && hit.y<=finish;
    const bool bar=(fabs(hit.y)<10 || fabs(hit.y-finish)<10) && fabs(hit.x)<250;
    if(centre || bar) { mask|=1<<i; sum+=x; ++count; }
  }
  return {mask,count?float(sum/count):0};
}
bool run(const Obstacle *objects,int count,double finish,bool curved,const char *label) {
  Race car; Pose p; Scene s={0,0,{0,0},2000,2000,true};
  car.start(s);
  for(int k=0;k<15000;++k) {
    s.now=k*10;
    s.line=track(p,finish,curved);
    s.left=s.right=2000;
    for (int n=0;n<count;++n) {
      s.left=std::min(s.left,(uint16_t)rayHit(local(p,-35,72),sin(p.h),cos(p.h),objects[n]));
      s.right=std::min(s.right,(uint16_t)rayHit(local(p,35,72),sin(p.h),cos(p.h),objects[n]));
    }
    Wheels m=car.step(s);
    double ds=(m.left+m.right)*0.005;
    double dh=(m.left-m.right)*0.01/cfg::track;
    p.x+=ds*sin(p.h+dh/2); p.y+=ds*cos(p.h+dh/2); p.h+=dh;
    s.travel+=(abs(m.left)+abs(m.right))*0.005f;
    bool touched=false;
    for(int n=0;n<count;++n) touched=touched || collision(p,objects[n]);
    if(touched || car.phase()==Phase::Fault || car.phase()==Phase::Done) {
      bool ok=!touched && car.phase()==Phase::Done &&
              local(p,0,94).y>finish && local(p,0,-70).y<finish;
      printf("%s %s | axle=(%.1f,%.1f) yaw=%.1f state=%d contact=%d t=%.2f\n",
             ok?"PASS":"FAIL",label,p.x,p.y,p.h*57.296,
             int(car.phase()),int(touched),s.now*0.001);
      return ok;
    }
  }
  return false;
}
int main() {
  bool ok=true;
  const int dims[][2]={{100,150},{60,60},{70,120},{100,110}};
  for(const auto &pair:dims) for(int side=0;side<2;++side) {
    Obstacle b={side==0?-double(pair[0]):0,760,double(pair[0]),double(pair[1])};
    char label[48];
    snprintf(label,sizeof(label),"%dx%d %s",pair[0],pair[1],side==0?"left":"right");
    ok=run(&b,1,1750,false,label) && ok;
  }
  for(int mirror=0;mirror<2;++mirror) {
    Obstacle objects[4];
    for(int i=0;i<4;++i) {
      double y=780+1390*i;
      double x=lineX(y,true)-((i+mirror)%2 ? 0 : dims[i][0]);
      objects[i]={x,y,double(dims[i][0]),double(dims[i][1])};
    }
    ok=run(objects,4,6200,true,mirror?"curve R-L-R-L":"curve L-R-L-R") && ok;
  }
  return ok?0:1;
}
