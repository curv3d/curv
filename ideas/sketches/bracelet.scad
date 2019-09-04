num=20;
for (i=[0:num-1])
  rotate([0,0,i*360/num])
    translate([40,0,0])
      rotate([0,i*90,0])
        scale([0.7,1,1])
          rotate_extrude() translate([10,0,0]) circle(r=2);
