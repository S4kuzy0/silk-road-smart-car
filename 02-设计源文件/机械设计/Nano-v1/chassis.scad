// Silk Road review v1. All dimensions in mm. +Y is forward; Z=0 is the mat.
// Select part and output from Customizer or openscad -D 'part="base"'.
part = "assembly"; // [assembly,base,tray,encoder,deck,drop,probe,hood,tof,caster,coupon]
output = "solid"; // [solid,cut]
$fn = 48;
stock = 3;
wheelD = 65;
tread = 27;
track = 130;
motorX = 39;
axleY = -12;
baseZ = 48;
lineY = 72;
noseY = 82;
probeFaceZ = 3.2;
module hole(x,y,d=3.3) { translate([x,y]) circle(d=d); }
module capsule(x,y,length,width,angle=0) {
    translate([x,y]) rotate(angle) hull()
      for (a=[-1,1]) translate([a*(length-width)/2,0]) circle(d=width);
}
module rounded(w,h,r=3) {
    hull() for (i=[-1,1],j=[-1,1]) translate([i*(w/2-r),j*(h/2-r)]) circle(r);
}
module clamp_edges(x,y) { // open outer edges for two reusable cable ties
    for (dy=[-30,6]) {
        capsule(x-14,y+dy,6,3,90);
        translate([x+10,y+dy-2]) square([8,4]);
    }
}
module base() {
    difference() {
        rounded(100,164,9);
        for (s=[-1,1]) scale([s,1]) {
            clamp_edges(motorX,axleY);
            // Clear rotating inner encoder disc and attach sliding pickup plate.
            capsule(21,axleY,42,5,90);
            for (y=[axleY-12,axleY+12]) capsule(12,y,10,3.4);
            for (y=[-31,29]) hole(41,y); // upper deck spacers
            for (y=[-21,17]) capsule(32,y,10,3.6,90); // battery straps
            hole(35,58); // ToF pivot
            hole(29,72); // lower sensor bracket foot
        }
        for (x=[-12,12]) capsule(x,-65,12,3.4,90);
        for (x=[-14,14]) capsule(x,38,10,4.5);
    }
}
module tray() {
    difference() {
        rounded(24,64,2);
        for (y=[-18,18],s=[-1,1]) translate([s*12-2,y-2]) square([4,4]);
    }
}
module encoder() { // hangs from base on spacers; long slots for module/zip ties
    difference() {
        rounded(17,37,2);
        for(y=[-12,12]) capsule(0,y,10,3.4,90);
        for(y=[-4,4]) capsule(0,y,12,2.8);
    }
}
module deck() {
    difference() {
        translate([0,-1]) rounded(90,70,5);
        for(s=[-1,1], y=[-31,29]) hole(41*s,y);
        for(x=[-17,17],y=[-11,6]) capsule(x,y,8,3.2);
        for(x=[-21,0,21],y=[15,23]) capsule(x,y,12,3.3);
        for(x=[-21,0,21]) capsule(x,-23,12,3.3);
    }
}
module drop() { // flat hanger, local Y is vertical from the bottom
    difference() {
        translate([0,21]) rounded(14,42,2);
        hole(0,37);
        capsule(0,9,14,3.4,90);
    }
}
module probe() { // horizontal carrier for perfboard; two hangers via angle brackets
    difference() {
        rounded(70,17,2);
        for(s=[-1,1]) {
            hole(s*29,0);
            capsule(s*19,0,7,3.2,90);
        }
        capsule(0,0,26,5);
    }
}
module hood() { // identical front/back shades; leave detector optical path open
    difference() {
        rounded(54,9,1.6);
        for(x=[-19,19]) hole(x,1,2.3);
    }
}
module tof() { // pivot + tie-mounted board on small right-angle bracket
    difference() {
        rounded(22,22,3);
        hole(0,0);
        for(x=[-7,7]) capsule(x,0,12,2.9,90);
    }
}
module caster() {
    difference() {
        rounded(52,30,3);
        for (s=[-1,1]) { hole(12*s,0); capsule(19.5*s,0,5.2,3.4); }
    }
}
module coupon() { // right side of the real chassis for one wheel + pickup
    intersection() { base(); translate([4,-53]) square([47,59]); }
}
module outline(name) {
    if (name=="base") base();
    else if (name=="tray") tray();
    else if (name=="encoder") encoder();
    else if (name=="deck") deck();
    else if (name=="drop") drop();
    else if (name=="probe") probe();
    else if (name=="hood") hood();
    else if (name=="tof") tof();
    else if (name=="caster") caster();
    else if (name=="coupon") coupon();
    else assert(false,"Unknown part");
}
module solid(name,printed=true) {
    linear_extrude(stock) outline(name);
    if (printed && name=="base") // ribs on upper surface, away from holders and bolt heads
        for(s=[-1,1]) translate([s*40-1,37,stock-0.1]) cube([2,15,3.1]);
}
module boxAt(c,size,tint) {
    color(tint) translate(c) cube(size,center=true);
}
module build() {
    color("BurlyWood") translate([0,0,baseZ]) solid("base");
    color("Wheat") translate([0,0,75]) solid("deck");
    for(s=[-1,1]) {
        color("DimGray") translate([s*track/2,axleY,wheelD/2])
            rotate([0,90,0]) cylinder(h=tread,d=wheelD,center=true);
        boxAt([s*motorX,axleY-12,baseZ-13],[20,60,26],"Gold");
        color("Tan") translate([s*(motorX-1),axleY-12,baseZ-29]) solid("tray");
        color("SlateGray") translate([s*21,axleY,wheelD/2])
            rotate([0,90,0]) cylinder(h=2,d=32,center=true);
        color("Tan") translate([s*12,axleY,29.5]) rotate([90,0,90]) solid("encoder");
        for(y=[-31,29]) color("Silver") translate([s*41,y,51]) cylinder(d=5,h=24);
        color("Sienna") translate([s*29,lineY-7,6]) rotate([90,0,0]) solid("drop");
        color("Tan") translate([s*35,58,baseZ+stock]) solid("tof");
        boxAt([s*35,60,61],[14,3,15],"SeaGreen");
    }
    color("Sienna") translate([0,lineY,9.5]) solid("probe");
    boxAt([0,lineY,7.6],[49,12,1.6],"DarkSeaGreen");
    for(i=[-2:2]) boxAt([i*10.16,lineY,5],[5.8,7,3.6],"MidnightBlue");
    for (y=[lineY-8,lineY+8]) color("Black")
        translate([0,y,7.5]) rotate([90,0,0]) solid("hood");
    color("Tan") translate([0,-65,19]) solid("caster");
    for(x=[-12,12]) color("Silver") translate([x,-65,22]) cylinder(d=5,h=26);
    boxAt([0,-65,17],[48,32,4],"Gray");
    color("Silver") translate([0,-65,7.9]) sphere(r=7.9);
    boxAt([0,-13,61],[60,60,20],"DarkSlateGray");
    boxAt([20,-2,81],[45,18,6],"Teal"); // USB faces the tray edge
    boxAt([-17,19,81],[19,15,6],"SteelBlue");
    boxAt([16,19,81],[22,15,6],"SeaGreen");
}
if (part=="assembly") build();
else if (output=="cut") outline(part);
else solid(part);
