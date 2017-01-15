
size = 100; // of main disk
$fn  = 64;  // for the arcs

// background disk
translate([0,0,-5])color("white") circle(size);

// kisrhombille3_7 is drawn in a unit disk
scale(size) kisrhombille3_7(3);

module kisrhombille3_7(n=1,t=0.015) {
    r = 0.497; // this is a magic number :)
    q = [for(i=[0:6]) r*[cos(i*360/7),sin(i*360/7)] ];
    color("magenta")
    for(i=[0:6]) {
        hyperSegment([0,0],q[i],t=t);
        hyperSegment(q[i],q[(i+1)%7],t=t);
        hyperSegment(q[i],q[(i+2)%7],t=t);
    }
    for(i=[0:6]) {
        recur_kisrhombille3_7(q[i], q[(i+1)%7], n, t);
    }
}

module recur_kisrhombille3_7(q1, q2, n, t) {
    p  = hyperRotation(q1,q2,360/7);
    p1 = hyperRotation(q1,q2,2*360/7);
    p2 = hyperRotation(q1,q2,3*360/7);
    q  = hyperRotation(q1,p ,360/7);
    color("red") {
        hyperSegment(p ,q1,t*0.7);
        hyperSegment(p ,q2,t*0.7);
        hyperSegment(p ,p1,t*0.7);
        hyperSegment(p2,p1,t*0.7);
        hyperSegment(p1,q2,t*0.7);
        hyperSegment(p2,q2,t*0.7);
        hyperSegment(p ,q ,t*0.7);
    }
    color("blue") {
        hyperSegment(p , hyperReflection(p,q1,q2),t*0.7);
        hyperSegment(p1, hyperReflection(p1,q2,p2),t*0.7);
    }
    color("green") {
        hyperSegment(p , p2,t*0.7);
        hyperSegment(q2, q ,t*0.7);
        hyperSegment(q1, p1,t*0.7);
    }
    if(n>1) {
        recur_kisrhombille3_7(p, p1 , n-1, t*0.7);
        recur_kisrhombille3_7(p1, p2, n-1, t*0.7);
        recur_kisrhombille3_7(q , p , n-1, t*0.7);
    }
}

//
//  Hyperbolic geometry functions and modules
//  All points are supposed to be in a unit disk 

// Hyperbolic segment joining p and q
module hyperSegment(p,q, t=0.05) {
    c = center(p,q);
    if(c!=undef) {
        R = radius(p,q);
        a01 = atan2((p-c)[1], (p-c)[0]);
        a02 = atan2((q-c)[1], (q-c)[0]);
        a1 = min(a01,a02); a2=max(a01,a02);
        a3 = a2-a1>180 ? a1+360: a1;
        a4 = a2;
        polyline([for(a=[a3:(a4-a3)/$fn:a4]) R*[cos(a), sin(a)]+c ],t=t); 
    } else polyline([p,q],t=t);
}

module hyperLine(p,q, t=0.05) { 
    s = idealPoints(p,q);
    if(s!=undef) hyperSegment(s[0],s[1],t);
    else if( norm(p-q)>1e-6 )
        polyline([unit(p-q),unit(q-p)],t);
}
         
// Hyperbolic lines in the unit disk -> Euclidean circles
function center(p,q) = 
    abs(p*[(p-q).y, -(p-q).x]) < 1e-6 ? undef : // p, q and [0,0] are colinear
        let( u = (p*q -1)/( 2*p*[(p-q).y, -(p-q).x] ) )
        (p+q)/2 - u*[(p-q).y, -(p-q).x];

function radius(p,q) = norm(center(p, q)-p);

// return the ideal points of a line passing thru p with direction d
function hyperLineByDirection(p,d) =
    norm(p) < 1e-6 ? norm(d)>1e-6 ? [unit(d),-unit(d)] : undef :
        let( dc = [-d.y, d.x] )
        abs(p*dc) < 1e-6 ? [unit(d),-unit(d)] :
            let( c  = p + dc*(1 - p*p)/(2*p*dc) )
            unitCircleInters(c,norm(c-p));

function hyperLineDirectionAt(p,q) =
    let( c = center(p,q) )
    c==undef ? norm(p-q)>1e-6 ? p-q : undef :
        [ (p-c).y, -(p-c).x ];

// the extreme points of a hyperline thru p,q
// the result [a,b] is such that the hyperline thru p,q pass by the points
// in the order a,p,q,b
function idealPoints(p,q) =
    let( c = center(p,q) )
    c == undef ? 
        norm(p-q) > 1e-6 ? 
            [ unit(p-q), -unit(p-q)] :
            undef :
        let( ip = unitCircleInters(c,norm(c-p)) )
        (ip[1]-ip[0])*(p-q) < 0 ?
            ip :
            [ip[1], ip[0]] ;
    
// Hyperbolic metric in the unit disk
// It may return an infinity value
function hyperDistance(p,q) =
    let( id = idealPoints(p,q) )
    norm(p-id[0])<1e-12 || norm(id[1]-q)<1e-12 ? 1/0 : 
        ln(norm(q-id[0])*norm(id[1]-p)/norm(p-id[0])/norm(id[1]-q));

// Hyperbolic transforms

// translation of x by dx
function hyperTranslation(x, dx) =
    let( d = 1 + 2*dx*x + dx*dx*x*x )
    abs(d) < 1e-6 ? undef :
        ((1 + dx*x + x*x)*dx + (1 - dx*dx)*x)/d;

function hyperReflection(x, p, q) =
    let( c  = center(p,q)) // Euclidean circle of 
                           // hyperline thru p,q
    c == undef ? undef :
        let( r  = radius(p,q),
             cx = x-c,     // dir of the reflected point
             s  = pow(r/norm(cx),2) )
        (c + s*cx);

function hyperRotation(x,c,a) =
    norm(x-c) < 1e-6 ? x :
        let( cc = center(x,c),
             d0 = hyperLineDirectionAt(c,x),
             cs = cos(a/2),
             sn = sin(a/2),
             rt = [[cs,sn],[-sn,cs]],
             d1 = d0*rt,
             rl = hyperLineByDirection(c,d1))
        hyperReflection(x,rl[0],rl[1]);

//    
// Euclidean geometry functions
//

// intersection points between unit circle 
// and the circle [c(enter),r(adius)]
function unitCircleInters(c,r) =
    norm(c)<1e-6 || norm(c)-abs(r) > 1 ? [] :
        let( x  = -(r*r - c*c -1)/norm(c)/2,
             y  = sqrt(1 - x*x) )
        [ [x,y], [x,-y] ]*[[c.x, c.y],[-c.y,c.x]]/norm(c);

function unit(v) = v/norm(v);

// Drawing module

module polyline(p, t=1) {
    for(i=[0:len(p)-2]) {
        v = p[i+1]-p[i];
        h = norm(v);
        if( h > 1e-12)
            translate(p[i])
                rotate(atan2(v[1],v[0])) 
                    translate([-t/2,-t/2]) square([h+t,t]);
    }
}