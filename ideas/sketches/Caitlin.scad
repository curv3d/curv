ball_height = 56;
top_radius = 20;
head_minkow = 3;
threw_hole = 11;

minkowski(){
    difference(){
        sphere(r=top_radius);
        cylinder(r=9, h=top_radius);
        for(i=[0:72:360]){
            rotate([0,60,i])
            cylinder(r=9, h=top_radius);
            // rotate([0,120,i])
            // cylinder(r=6, h=ball_height/2);
        }
    }
    sphere(r=head_minkow);
}
