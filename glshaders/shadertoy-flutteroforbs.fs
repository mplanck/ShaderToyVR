ScreenPercentage = .5

:::::::::::::

// **************************************************************************
// DEFINITIONS

#define PI 3.14159
#define TWO_PI 6.28318
#define ONE_OVER_PI 0.3183099
#define ONE_OVER_TWO_PI 0.159154
#define PI_OVER_TWO 1.570796
    
#define EPSILON 0.0001
#define BIG_FLOAT 1000000.0

// **************************************************************************
// OPTIMIZATION DEFS

// Comment in the quality of your choice based on graphics card performance

//#define HIGH_QUALITY
#define MID_QUALITY
//#define LOW_QUALITY

#ifdef HIGH_QUALITY
#define MAX_NUM_TRACE_ITERATIONS 64
#define NUM_POLAR_MARCH_STEPS 128
#define FOG_EXTENT 1400.
#endif

#ifdef MID_QUALITY
#define MAX_NUM_TRACE_ITERATIONS 48
#define NUM_POLAR_MARCH_STEPS 64
#define FOG_EXTENT 1000.
#endif 

#ifdef LOW_QUALITY
#define MAX_NUM_TRACE_ITERATIONS 32
#define NUM_POLAR_MARCH_STEPS 32
#define FOG_EXTENT 800.
#endif 

// always have audio samples as multiple of 3
#define NUM_AUDIO_SAMPLE_TRIPLES 3

// **************************************************************************
// GLOBALS

// the beat electronebulae is about 126 beats per minute, so we some timings
// based events based on multiples of that beat rate.

// - - - - - - - - - - - - -
// Audio based signals
float g_beatRate = 126./60.;
float g_time = iGlobalTime;
float g_audioResponse = 1.;

// audio signals
float g_bassBeat = 0.;
vec3 g_audioFreqs[ NUM_AUDIO_SAMPLE_TRIPLES ];

// - - - - - - - - - - - - -
// Cell walls properties

#define CELL_BOX_HALF_SIZE_DEFAULT 75.
float g_cellBoxHalfSize = CELL_BOX_HALF_SIZE_DEFAULT;

// - - - - - - - - - - - - -
// Fog properties

vec3 g_fogColor = vec3(0.23, 0.12, 0.12);

// - - - - - - - - - - - - -
// Orb properties

//   dimensions of the orb
#define ORB_OUTER_RADIUS_DEFAULT 50.
float g_orbOuterRadius = ORB_OUTER_RADIUS_DEFAULT; // defaults
#define ORB_CENTER_RADIUS_DEFAULT 10.
float g_orbCenterRadius = ORB_CENTER_RADIUS_DEFAULT; // defaults

//   surface properties of the orb
float g_orbSurfaceFacingKr = .1;
float g_orbSurfaceEdgeOnKr = 1.8;
float g_orbIOR = 1.33;

vec2 g_numOrbCells = vec2(24., 128.); // defaults
vec2 g_numOrbsChangeRate = vec2(g_beatRate * 0.05, g_beatRate * 0.0625);

//   properties of the "spark" lines
float g_sparkWidth = 1.2;
float g_sparkColorIntensity = 1.;
vec3 g_sparkCoreColor = vec3(1.1, 0.5, 0.5);
vec3 g_sparkExtentColor = vec3(1.2, 1.2, 2.3);

vec2 g_sparkRotationRate = vec2(.0, g_beatRate * .25);

// - - - - - - - - - - - - -
// Camera properties

vec3 g_camOrigin = vec3(0., 0., -120.);
vec3 g_camPointAt = vec3( 0.0, 0.0, 0.0 );
vec3 g_camUpDir = vec3( 0.0, 1.0, 0.0 );

vec2 g_camRotationRates = vec2(0.125 * g_beatRate, -0.0625 * g_beatRate);

// **************************************************************************
// MATH UTILITIES
// **************************************************************************

// XXX: To get around a case where a number very close to zero can result in 
// erratic behavior with sign, we assume a positive sign when a value is 
// close to 0.
float zeroTolerantSign(float value)
{
    // DEBRANCHED
    // Equivalent to:
    // if (abs(value) > EPSILON) { 
    //    s = sign(value); 
    // }
    return mix(1., sign(value), step(EPSILON, abs(value)));
}

// convert a 3d point to two orb coordinates. First coordinate is latitudinal
// angle (angle from the plane going through x+z) Second coordinate is azimuth
// (rotation around the y axis)

// Range of outputs := ([PI/2, -PI/2], [-PI, PI])
vec2 cartesianToPolar( vec3 p ) 
{    
    return vec2(PI/2. - acos(p.y / length(p)), atan(p.z, p.x));
}

// Convert a polar coordinate (x is latitudinal angle, y is azimuthal angle)
// results in a 3-float vector with y being the up axis.

// Range of outputs := ([-1.,-1.,-1.] -> [1.,1.,1.])
vec3 polarToCartesian( vec2 angles )
{
    float cosLat = cos(angles.x);
    float sinLat = sin(angles.x);
    
    float cosAzimuth = cos(angles.y);
    float sinAzimuth = sin(angles.y);
    
    return vec3(cosAzimuth * cosLat,
                sinLat,
                sinAzimuth * cosLat);
}

// Rotate the input point around the y-axis by the angle given as a 
// cos(angle) and sin(angle) argument.  There are many times where 
// I want to reuse the same angle on different points, so why do the 
// heavy trig twice.
// Range of outputs := ([-1.,-1.,-1.] -> [1.,1.,1.])
vec3 rotateAroundYAxis( vec3 point, float cosangle, float sinangle )
{
    return vec3(point.x * cosangle  + point.z * sinangle,
                point.y,
                point.x * -sinangle + point.z * cosangle);
}

// Rotate the input point around the x-axis by the angle given as a 
// cos(angle) and sin(angle) argument.  There are many times where 
// I want to reuse the same angle on different points, so why do the 
// heavy trig twice.
// Range of outputs := ([-1.,-1.,-1.] -> [1.,1.,1.])
vec3 rotateAroundXAxis( vec3 point, float cosangle, float sinangle )
{
    return vec3(point.x,
                point.y * cosangle - point.z * sinangle,
                point.y * sinangle + point.z * cosangle);
}

// Returns the floor and ceiling of the given float
vec2 floorAndCeil( float x ) 
{
    return vec2(floor(x), ceil(x));
}

// Returns the floor and ceiling of each component in respective
// order (floor(p.x), ceil(p.x), floor(p.y), ceil(p.y)) 
vec4 floorAndCeil2( vec2 p ) 
{
    return vec4(floorAndCeil(p.x), 
                floorAndCeil(p.y));
}

// Returns 2 floats, the first is the rounded value of float.  The second
// is the direction in which the rounding took place.  So if rounding 
// occurred towards a larger number 1 is returned, otherwise -1 is returned. 
vec2 round( float x ) 
{
    return vec2(floor(x+0.5), sign(fract(x)-0.5));
}

// Returns 4 floats, 
// the first is the rounded value of p.x
// the second is the direction in which the rounding took place for p.x
// So if rounding occurred towards a greater number 1 is returned, 
// otherwise -1 is returned
// the third is the rounded value of p.y.  
// the fourth is the direction in which the rounding took place for p.y
vec4 round2( vec2 p ) 
{
    return vec4(round(p.x), round(p.y));
}

// **************************************************************************
// NOISE/FRACTAL FUNCTIONS
// **************************************************************************

// Periodic saw tooth function that repeats with a period of 
// 4 and ranges from [-1, 1].  
// The function starts out at 0 for x=0,
//  raises to 1 for x=1,
//  drops to 0 for x=2,
//  continues to -1 for x=3,
//  and then rises back to 0 for x=4
// to complete the period

float sawtooth( float x )
{
    float xmod = mod(x+3.0, 4.);
    return abs(xmod-2.0) - 1.0;
}

// **************************************************************************
// INTERSECT UTILITIES
// **************************************************************************

// intersection for a sphere with a ray. In the case where a ray hits, there 
// is gauranteed to be a tmin and tmax return value even if one of the hits
// is "behind" the ray origin. If both hits are behind the origin, no hit
// takes place.

// Returns a vec3 where:
//  result.x = 1. or 0. to indicate if a hit took place
//  result.y = tmin
//  result.z = tmax

vec3 intersectSphere(vec3 rayOrigin,                 
                     vec3 rayDir, 
                     float radius,
                     vec3 sphereCenter)
{

    // Calculate the ray origin in object space of the sphere
    vec3 ospaceRayOrigin = rayOrigin - sphereCenter;

    // We don't consider an intersection if the ray is inside the sphere

    // DEBRANCH
    // Equivalent to:
    // if (dot(ospaceRayOrigin, ospaceRayOrigin) < radius*radius) {
    //     return vec3(0.);
    // }
    
    float a = dot(rayDir, rayDir);
    float b = 2.0*dot(ospaceRayOrigin, rayDir);
    float c = dot(ospaceRayOrigin, ospaceRayOrigin) - radius*radius;
    float discr = b*b - 4.0*a*c; // discriminant

    float tmin = 0.0;
    float tmax = 0.0;

    // DEBRANCH
    // Equivalent to:
    // if (discr > 0.) {
    //     ...
    // }

    float isdiscrgtZero = step(0., discr);

    // Real root of disc, so intersection
    // avoid taking square root of negative discriminant.
    float sdisc = sqrt(abs(discr));
    tmin = (-b - sdisc)/(2.0 * a);
    tmax = (-b + sdisc)/(2.0 * a); 

    float hit = isdiscrgtZero * max( step(0., tmin), step(0., tmax) );

    return vec3(hit, tmin, tmax);
}

// Reference: http://geomalgorithms.com/a05-_intersect-1.html. Does an
// intersection test against a plane that is assumed to be double sided and
// passes through the origin and has the specified normal.

// Returns a vec2 where:
//   result.x = 1. or 0. if there is a hit
//   result.y = t such that origin + t*dir = hit point
vec2 intersectDSPlane(vec3 origin,
                      vec3 dir,
                      vec3 planeNormal,
                      vec3 planeOffset)
{
    float dirDotN = dot(dir, planeNormal);
    // if the ray direction is parallel to the plane, let's just treat the 
    // ray as intersecting *really* far off, which will get culled as a
    // possible intersection.

    float denom = zeroTolerantSign(dirDotN) * max(abs(dirDotN), EPSILON);
    float t = min(BIG_FLOAT, -dot(planeNormal, (origin - planeOffset)) / denom);    
    return vec2(step(EPSILON, t), t);

}

// Reference: http://geomalgorithms.com/a05-_intersect-1.html Does an
// intersection test against a plane that is assumed to  be single sided and
// passes through a planeOffset and has the specified normal.

// References: 
// http://www.geometrictools.com/Documentation/IntersectionLineCone.pdf
// http://www.geometrictools.com/LibMathematics/Intersection/Intersection.html
//
// Does an intersection with an infinite cone centered at the origin and 
// oriented along the positive y-axis extending to infinite.  Returns the 
// minimum positive value of t if there is an intersection.  

// This function works by taking in the latitudinal parameters from -PI/2 to
// PI/2 which correspond to the latitudinal of the x-z plane.  The reference
// provided assumes the angle of the cone is determined based on the cone angle
// from the y-axis.  So instead of using a cosine of the angle as described in
// the reference, we use a sine of the angle and also consider whether the
// angle is positive or negative to determine which side of the pooled cone
// we are selecting.
//
// If the angle of the z-x plane is near zero, this function just does a an
// intersection against the x-y plane to handle the full range of possible
// input.  This function clamps any angles between -PI/2 and PI/2.

// Trying to DEBRANCH this code results in webgl behaving funny in MacOS
// XXX: Any help fixing this is appreciated

// Returns a vec2 where:
//   result.x = 1. or 0. if there is a hit
//   result.y = t such that origin + t*dir = hit point

vec2 intersectSimpleCone(vec3 origin,
                         vec3 dir,
                         float coneAngle)
{
    
    // for convenience, if coneAngle ~= 0., then intersect
    // with the x-z plane.      
    float axisDir = zeroTolerantSign(coneAngle);
    float clampedConeAngle = clamp(abs(coneAngle), 0., PI/2.);    
    
    float t = 0.;

    if (clampedConeAngle < EPSILON) {
        t = -origin.y / dir.y;
        return vec2(step(0., t), t);
    }
    
    // If coneAngle is 0, assume the cone is infinitely thin and no
    // intersection can take place
    if (clampedConeAngle < EPSILON) {
        return vec2(0.);
    }
    
    float sinAngleSqr = sin(clampedConeAngle);
    sinAngleSqr *= sinAngleSqr;

    // Quadric to solve in order to find values of t such that 
    // origin + direction * t intersects with the pooled cone.
    // 
    // c2 * t^2 + 2*c1*t + c0 = 0
    //
    // This is a little math trick to get rid of the constants in the 
    // classic equation t = ( -b +- sqrt(b^2 - 4ac) ) / 2a
    // by making b = 2*c1, you can see how this helps divide out the constants
    // t = ( -2 * c1 +- sqrt( 4 * c1^2 - 4 * c2c0) / 2 * c2
    // see how the constants drop out so that
    // t = ( -c1 +- sqrt( c1^2 - c2 * c0)) / c2

    // Lots of short cuts in reference intersection code due to the fact that
    // the cone is at the origin and oriented along positive y axis.
    // 
    // A := cone aligned axis = vec3(0., 1., 0.)
    // E := origin - coneOrigin = origin
    // AdD := dot(A, dir) = dir.y
    // AdE := dot(A, E) = origin.y
    // DdE := dot(dir, E) = dot(dir, origin)
    // EdE := dot(E, E) = dot(origin, origin)
    
    float DdO = dot(dir, origin);
    float OdO = dot(origin, origin);
    
    float c2 = dir.y * dir.y - sinAngleSqr;
    float c1 = dir.y * origin.y - sinAngleSqr * DdO;
    float c0 = origin.y * origin.y - sinAngleSqr * OdO;

    float discr = c1*c1 - c2*c0;
    float hit = 0.;

    // if c2 is near zero, then we know the line is tangent to the cone one one
    // side of the pool.  Need to check if the cone is tangent to the negative
    // side of the cone since that could still intersect with quadric in one
    // place.
    if (abs(c2) > EPSILON) {
    
        if (discr < 0.0) {
            
            return vec2(0.);
            
        } else {
    
            // Real root of disc, so intersection may have 2 solutions, fine the 
            // nearest one.
            float sdisc = sqrt(discr);
            float t0 = (-c1 - sdisc)/c2;
            float t1 = (-c1 + sdisc)/c2;
    
            // a simplification we can make since we know the cone is aligned
            // along the y-axis and therefore we only need to see how t affects
            // the y component.
            float intersectPt0y = origin.y + dir.y * t0;
            float intersectPt1y = origin.y + dir.y * t1;    
            
            // If the intersectPts y value is greater than 0. we know we've
            // intersected along the positive y-axis since the cone is aligned
            // along the y-axis
    
            // If the closest intersection point is also a valid intersection
            // have that be the winning value of t.
            
            if ((t0 >= 0.) &&
                (axisDir * intersectPt0y > 0.)) {
                t = t0;
                hit = 1.;
            }
            
            if ((t1 >= 0.) &&
                ((t1 < t0) || (hit < .5)) &&
                (axisDir * intersectPt1y > 0.)) {
                t = t1;     
                hit = 1.;
            }
    
        } 

    } else if (abs(c1) > EPSILON) {
        // This is the code to handle the case where there  is a ray that is on
        // the pooled cone and intersects at the very tip of the cone at the
        // origin.
        float t0 = -0.5 * c0 / c1;
        
        float intersectPty = origin.y + dir.y * t0;
        if (( t0 >= 0.) && (axisDir * intersectPty > 0.)) {
            t = t0;
            hit = 1.;
        }       
    }
    
    return vec2(hit, t);

}

// Returns the vector that is the shortest path from the bounded line segment
// u1->u2 to the line represented by the line passing through v1 and v2.  
//
// result.x is the distance of the shortest path between the line segment and
// the unbounded line
//
// result.y = is the value of t along the line segment of ba [0,1] that 
// represents the 3d point (p) of the shortest vector between the two line 
// segments that rests on the vector between u1 anb u2 such that 
//    p = u1 + (u2-u1) * t
//
// result.z = is the value of t along the line passing through v1 and v2 such
// that q represents the closest point on the line to the line segment u2<-u1:
//    q = v1 + (v2-v1) * t
// t is unbounded in this case but is parameterized such that t=0 at v1 and t=1
// at v2.

vec3 segmentToLineDistance( vec3 u1, 
                            vec3 u2, 
                            vec3 v1, 
                            vec3 v2 )
{
    vec3 u = u2 - u1;
    vec3 v = v2 - v1;
    vec3 w = u1 - v1;
    
    // For the maths:
    // http://geomalgorithms.com/a07-_distance.html#dist3D_Segment_to_Segment
    float a = dot(  u, u );
    float b = dot(  u, v ); 
    float c = dot(  v, v );   
    float d = dot(  u, w );
    float e = dot(  v, w ); 
    
    // just a way of calculating two equations with one operation.
    // th.x is the value of t along ba
    // th.y is the value of t along wv 

    // when a*c - b*b is near 0 (the lines are parallel), we will just assume
    // a close line is the distance between u1 and v1
    float denom = (a * c - b * b);

    // DEBRANCHED
    // Equivalent to:
    // vec2 th = (abs(denom) < EPSILON ? vec2(0.) : 
    //                                  vec2( b*e - c*d, a*e - b*d ) / denom);

    float clampedDenom = sign(denom) * max(EPSILON, abs(denom));
    vec2 th = mix( vec2( b*e - c*d, a*e - b*d ) / clampedDenom, 
                   vec2(0.),
                   step(abs(denom), EPSILON));

    // In the case where the line to line comparison has p be a point that lives
    // off of the bounded segment u2<-u1, just fine the closest path between u1
    // and u2 and pick the shortest

    float ifthxltZero = step(th.x, 0.);
    float ifthxgt1 = step(1., th.x);

    // DEBRANCHED
    // Equivalent to:
    // if (th.x < 0.) {
    //     th.x = 0.;
    //     th.y = dot(v, u1-v1) / c; // v . (u1<-v1) / v . v
    // } else if (th.x > 1.) {
    //     th.x = 1.;
    //     th.y = dot(v, u2-v1) / c; // v . (u2<-v1) / v . v
    // }
    
    th.x = clamp(th.x, 0., 1.);
    th.y = mix(th.y, dot(v, u1-v1) / c, ifthxltZero);
    th.y = mix(th.y, dot(v, u2-v1) / c, ifthxgt1);
    
    // p is the nearest clamped point on the line segment u1->u2
    vec3 p = u1     + u  * th.x;
    // q is the nearest unbounded point on the line segment v1->v2
    vec3 q = v1     + v  * th.y;
    
    return vec3(length(p-q), th.x, th.y);
}


// Returns the vector that is the shortest path from the 3D point to the line
// segment as well as the parameter t that represents the length along the line
// segment that p is closest to.

// Returned result is:

// result.xyz := vector of the path from p to q where q is defined as the point
// on the line segment that is closest to p.
// result.w   := the t parameter such that a + (b-a) * t = q 

vec4 segmentToPointDistance( vec3 a, 
                             vec3 b, 
                             vec3 p)
{
    
    vec3 ba = b - a;    
    float t = dot(ba, (p - a)) / max(EPSILON, dot(ba, ba));
    t = clamp(t, 0., 1.);
    vec4 result = vec4(ba * t + a - p, t);
    return result;
}

// Returns the vector that is the shortest path from the 3D point to the  line
// that passes through a and b as well as the parameter t that represents the
// length along the line that p is closest to.

// Returned result is:

// result.xyz := vector of the path from p to q where q is defined as the point
// on the line segment that is closest to p.
// result.w   := the t parameter such that a + (b-a) * t = q 

vec4 lineToPointDistance( vec3 a, 
                          vec3 b, 
                          vec3 p)
{
    
    vec3 ba = b - a;    
    float t = dot(ba, (p - a)) / dot(ba, ba);
    vec4 result = vec4(ba * t + a - p, t);
    return result;
}

// **************************************************************************
// SHADING UTILITIES
// **************************************************************************

// Approximating a dialectric fresnel effect by using the schlick approximation
// http://en.wikipedia.org/wiki/Schlick's_approximation. Returns a vec3 in case
// I want to approximate a different index of reflection for each channel to
// get a chromatic effect.
vec3 fresnel(vec3 I, vec3 N, float eta)
{
    // assume that the surrounding environment is air on both sides of the 
    // dialectric
    float ro = (1. - eta) / (1. + eta);
    ro *= ro;
    
    float fterm = pow(1. - max(dot(-I, N), 0.), 5.);  
    return vec3(ro + ( 1. - ro ) * fterm); 
}

// **************************************************************************
// TRACE UTILITIES
// **************************************************************************

// This ray structure is used to march the scene and accumulate all final
// results.

struct SceneRayStruct {
    vec3 origin;        // origin of the ray - updates at refraction, 
                        // reflection boundaries
    vec3 dir;           // direction of the ray - updates at refraction, 
                        // reflection boundaries

    float marchDist;    // current march distance of the ray - updated during
                        // tracing
    float marchMaxDist; // max march distance of the ray - does not update

    vec3 cellCoords; // current box being marched through - updated during
                        // marching

    vec4 orbHitPoint;   // xyz is the point on the orb that hit point took
                        // place.  The w value is 0 or 1 depending on if there
                        // was in fact a hit on an orb.

    vec4 color;         // accumulated color and opaqueness of the ray - updated
                        // during tracing

    vec4 debugColor;    // an alternative color that is updated during
                        // traversal if it has a non-zero opaqueness, then it
                        // will be returned to the frame buffer as the current
                        // frag color. Alpha is ignored when displaying to the 
                        // frag color.
};

// This ray structure is used to march through a space that represents the
// interior of the orbs and is chopped up into cells whose boundaries are
// aligned with the azimuthal and latitudinal angles of the sphere.

// Looking at a cross section of the orb from the azimuthal direction (from
// top down). You can see how in this case of a sphere divided into 8
// subdomains, the cell coordinates wrap at the back of the sphere.

//
//        . * ' * .
//      *   8 | 1   *
//    *  \    |    /  *
//   /  7  \  |  /  2  \
//  *________\|/________*
//  *        /|\        *
//   \  6  /  |  \  3  /
//    *  /    |    \  *
//      *   5 | 4   *
//        ' * . * ' 
//
//        V Front V


// Looking at a cross section of the orb from the side split down the center.
// You can see in this example of the latitudinal cells having 4 subdomains
// (or cells), the latitudinal cells are divided by cones originating from the
// sphere center.

//
//        . * ' * .
//      * \   4   / *
//    *    \     /    *
//   /  3   \   /   3  \
//  *________\ /________*
//  *        / \        *
//   \  2   /   \   2  /
//    *    /     \    *
//      * /   1   \ *
//        ' * . * '
//
//        V Bottom V

struct OrbRayStruct {
    vec3 origin;                  // origin of the ray to march through the
                                  // orb space - does not update
    
    vec3 dir;                     // direction of the ray to march through the
                                  // orb space - does not update

    float marchDist;              // current march distance of the ray through
                                  // the sphere - updated during tracing

    float marchNextDist;          // represents the next extent of the current
                                  // cell so we can create a line segment on
                                  // the ray that represents the part of the
                                  // line that is clipped by the orb cell
                                  // boundaries.  We then use this line
                                  // segment to perform a distance test  of
                                  // the spark

    float marchMaxDist;           // max march distance of the ray - does not 
                                  // update represents the other side the sphere
 
    int azimuthalOrbCellMarchDir; // when marching through the orb cells, we
                                  // keep track of which way the ray is going
                                  // in the azimuthal direction, so we only
                                  // need to test against one side of the
                                  // orb cell - a plane that lies on the
                                  // y-axis and is rotated  based on the
                                  // current cell.  This avoids precision
                                  // issues calculated at beginning of
                                  // traversal and used through out the
                                  // iterations.

    vec2 orbCellCoords;           // Keep track of the current orb cell
                                  // we're in: x is the latitudinal cell number
                                  // y is the azimuthal cell number

    float orbHash;                // Hash to this particular orb so we can use
                                  // it as the unique identifier to key into 
                                  // variation based signals.

    vec4 color;                   // accumulated color and opaqueness of the ray
                                  // marching the orbs.

    vec4 debugColor;              // an alternative color that is updated during
                                  // traversal if it has a non-zero opaqueness,
                                  // then it will be returned to the frame
                                  // buffer as the current frag color

};

// Returns the distance field to the cell center corresponding to the 
// cell coordinate plus the neighbor offset, which should only have 
// integer values in the range ([-1,-1,-1], [1,1,1])
//
// result.xyz = p relative to the neighbor orb center
// result.w   = distance to orb exterior 
vec4 currCellOrbDF( vec3 modp, 
                    vec3 cellCoords, 
                    vec3 neighborOffset)
{

    float maxOffset = g_cellBoxHalfSize - ORB_OUTER_RADIUS_DEFAULT;
    vec3 currCellCoords = cellCoords + neighborOffset;  

    // Add a subtle sinuisoidal wave to sorb center.
    vec3 noiseSeed = vec3(dot(vec3(5.3, 5.0, 6.5), currCellCoords),
                          dot(vec3(6.5, 5.3, 5.0), currCellCoords),
                          dot(vec3(5.0, 6.5, 5.3), currCellCoords));
    
    noiseSeed += 0.5 * vec3(0.5, 0.9, 1.2) * g_time;
                          
    vec3 currOffset = vec3(maxOffset) * cos(noiseSeed);     

    vec3 currSphCenter = 2. * g_cellBoxHalfSize * neighborOffset + currOffset;
    vec3 sphereRelativeP = modp - currSphCenter;

    float dist = length(sphereRelativeP) - g_orbOuterRadius;    

    return vec4(sphereRelativeP, dist);   
}

// Returns the distance field to the nearest orb shell plus the position of
// p relative to the center of the orb.
//
// result.xyz = p relative to orb center
// result.w   = distance to orb exterior 
vec4 orbDF( vec3 p, vec3 rayDir, vec3 cellCoords )
{

    // Look into this cell to check distance to that sphere and then check the
    // distance to the spheres for the next cells to see if they are closer,
    // we can guess the "next" cells by just looking at the sign of the ray
    // dir (so we don't check all 27 possible cells - 9x9x9 grid).  This
    // limits it down just to checking 8 possible cells.
    vec3 modp = mod(p, 2. * g_cellBoxHalfSize) - g_cellBoxHalfSize;

    vec4 nextCellOffset = vec4(sign(rayDir), 0.);

    // 1. current cell
    vec4 result = currCellOrbDF(modp, cellCoords, vec3(0.));
        
    // 2. neighbor in the x direction of the ray
    vec4 neighborResult = currCellOrbDF(modp, cellCoords, nextCellOffset.xww);    
    if (neighborResult.w < result.w) { result = neighborResult; }

    // 3. neighbor in the y direction of the ray
    neighborResult = currCellOrbDF(modp, cellCoords, nextCellOffset.wyw);
    if (neighborResult.w < result.w) { result = neighborResult; }

    // 4. neighbor in the z direction of the ray
    neighborResult = currCellOrbDF(modp, cellCoords, nextCellOffset.wwz);
    if (neighborResult.w < result.w) { result = neighborResult; }
    
    // 5. neighbor in the x-y direction of the ray
    neighborResult = currCellOrbDF(modp, cellCoords, nextCellOffset.xyw);
    if (neighborResult.w < result.w) { result = neighborResult; }

    // 6. neighbor in the y-z direction of the ray
    neighborResult = currCellOrbDF(modp, cellCoords, nextCellOffset.wyz);
    if (neighborResult.w < result.w) { result = neighborResult; }

    // 7. neighbor in the x-z direction of the ray
    neighborResult = currCellOrbDF(modp, cellCoords, nextCellOffset.xwz);
    if (neighborResult.w < result.w) { result = neighborResult; }
        
    // 8. neighbor in the x-y-z direction of the ray
    neighborResult = currCellOrbDF(modp, cellCoords, nextCellOffset.xyz);
    if (neighborResult.w < result.w) { result = neighborResult; }

    return result;
}

// Given a scene ray generated from a camera, trace the scene by first
// distance field marching.

float traceScene(inout SceneRayStruct sceneRay)
{

    // --------------------------------------------------------------------
    // How deep into the "infinite" cooridor of orbs do we want to go.  Each
    // iteration does distance field marching which will converge on the 
    // nearest orb or continue to the next.

    vec3 marchPoint = sceneRay.origin;
    float dist = BIG_FLOAT;
    
    for (int iterations = 0; 
         iterations < MAX_NUM_TRACE_ITERATIONS; 
         iterations += 1) {
        
        if ((sceneRay.marchMaxDist < sceneRay.marchDist) ||
            (dist < .01)) {
            continue;
        }  
                
        // used as a way to hash the current cell
        sceneRay.cellCoords = floor(marchPoint /(2. * g_cellBoxHalfSize));

        // scene map
        vec4 orbDFresult = orbDF( marchPoint, 
                                  sceneRay.dir,
                                  sceneRay.cellCoords );
        dist = orbDFresult.w;

        if (dist < .01) {

            sceneRay.orbHitPoint.xyz = orbDFresult.xyz;
            sceneRay.orbHitPoint.w = 1.;
        }
        
        sceneRay.marchDist += dist;
        marchPoint += dist * sceneRay.dir; 
        
    }            

    return sceneRay.orbHitPoint.w;
}


// Function used for ray marching the interior of the orb.  The sceneRay is 
// also passed in since it may have some information we want to key off of
// like the current orb cell coordinate.
//
// Returns the OrbRayStruct that was passed in with certain parameters
// updated.
void shadeOrbInterior(inout OrbRayStruct orbRay, 
                      in SceneRayStruct sceneRay)
{    
    
    // Before marching into the orb coordinates, first determine if the orbRay
    // is casting in the direction of the sphere where the "angle" is increasing
    // along the azimuthal direction, in which case we assume we are marching
    // along the azimuthal cells in a positive direction.

    vec3 orbRayEnd = orbRay.origin + orbRay.dir * orbRay.marchMaxDist;
    vec4 ltpresult = lineToPointDistance(orbRay.origin, orbRayEnd, vec3(0.));
    vec3 ltp = ltpresult.xyz;
    vec3 rdircrossltp = normalize(cross(orbRay.dir, ltp));
        
    orbRay.azimuthalOrbCellMarchDir = (dot(rdircrossltp, 
                                      vec3(0., 1., 0.)) > 0.) ? 1 : -1;  


    // ------------------------------------------------------------
    // Convert the cartesian point to orb space within the range
    // Latitudinal    := [-PI/2, PI/2]
    // Azimuth        := [-PI, PI]
    vec2 orbRayOriginPt = cartesianToPolar(orbRay.origin);                
    
    //
    // convert orb coordinates into number of spark domains       
    // Latitudinal    := [-PI/2, PI/2]
    // Azimuth        := [-PI, PI]
    // LatdDomains    := [-numLatdDomains/2., numLatdDomains/2.] 
    // AzimuthDomains := [-numAzimDomains/2., numAzimDomains/2.]
    // In order to avoid confusion        

    orbRayOriginPt -= g_sparkRotationRate * g_time;
    orbRayOriginPt *= g_numOrbCells * vec2(1./PI, 1./TWO_PI);

    vec4 rOrbRayOriginPtResults = round2(orbRayOriginPt);
    vec2 cellCoords = vec2(rOrbRayOriginPtResults.xz);                        
    cellCoords -= vec2(.5,.5) * rOrbRayOriginPtResults.yw;

    orbRay.orbCellCoords = cellCoords;

    // ------------------------------------------------------------
    // March through the orb cells a fixed number of steps.

    for ( int i = 0; i < NUM_POLAR_MARCH_STEPS; i++)
    {
        // If orbRay color has near opaque opacity or we've reached the extent of 
        // the orb, no need to continue marching.  Use continue instead of break
        // since that appears to be more optimal.
        if (max(step(orbRay.marchMaxDist, orbRay.marchDist),
                step(0.95, orbRay.color.a)) > .5) {
            continue;
        }
        
        vec3 marchPoint = orbRay.origin + orbRay.dir * orbRay.marchDist;
        
        // Convert the cartesian point to orb space within the range
        // Latitudinal    := [-PI/2, PI/2]
        // Azimuth        := [-PI, PI]
        vec2 orbMarchPt = cartesianToPolar(marchPoint);                
        
        // convert orb coordinates into number of cell domains       
        // From:
        // Latitudinal    := [-PI/2, PI/2]
        // Azimuth        := [-PI, PI]
        // To:
        // LatdDomains    := [-numLatdDomains/2., numLatdDomains/2.]        
        // AzimuthDomains := [-numAzimDomains/2., numAzimDomains/2.]

        orbMarchPt -= g_sparkRotationRate * g_time;
        orbMarchPt *= g_numOrbCells * vec2(ONE_OVER_PI, ONE_OVER_TWO_PI);

        vec2 orbSparkStart = orbRay.orbCellCoords;     
        
        // convert back into orb coordinate range 
        // Latitudinal      := [-PI/2, PI/2]
        // Azimuth        := [-PI, PI] 
        vec2 remapOrbSparkStart = orbSparkStart * vec2(PI, TWO_PI) / 
                                            g_numOrbCells;
                
        remapOrbSparkStart += g_sparkRotationRate * g_time;     
                
        // ------------------------------------------------------------------
        // Spark color contribution

        // create the properties of this spark
        vec3 sparkDir = polarToCartesian(remapOrbSparkStart);
        vec3 sparkOrigin = g_orbCenterRadius * sparkDir;
        vec3 sparkEnd = g_orbOuterRadius * sparkDir;
                
        // test this marches closeness to the spark and use that to
        // drive spark presence.
        vec3 sparkResult = segmentToLineDistance(sparkOrigin,
                                                 sparkEnd,
                                                 orbRay.origin,
                                                 orbRayEnd);

        vec3 sparkColor = vec3(0.);
        float sparkOpacity = 0.;        
        
        // As the orbs are traced farther into the distance, increase the glow extent
        // and alpha extent to account for filtering.

        float distanceLerp = smoothstep(100., 600., sceneRay.marchDist);
        float sparkGlowExtent = max(g_sparkWidth, mix(.5, 3.0, distanceLerp));
        float sparkAlphaExtent = 1.2 * sparkGlowExtent;
        float sparkAttenuate = abs(sparkResult.x);

        // Use triples to compress the expensive operations by using their vec3
        // versions.
        vec3 audioAttenuateTriples = vec3(0.);
        
        // Determine how the audio signals drive this spark.  Each audio sample is
        // "traveling" through the spark of the orbs from top down with different offsets.
        for (int i = 0; i < NUM_AUDIO_SAMPLE_TRIPLES; i += 1) {
            vec3 indexes = 3. * vec3(float(i) + 0., 
                                     float(i) + 1.,
                                     float(i) + 2.);

            vec3 sparkPopTravelSeed = 10. * indexes + orbRay.orbHash + orbRay.orbCellCoords.x + g_beatRate * 3. * g_time;                                  
            sparkPopTravelSeed = smoothstep(vec3(15.), 
                                            vec3(0.), 
                                            mod(sparkPopTravelSeed, 
                                                5. * 3. * float(NUM_AUDIO_SAMPLE_TRIPLES)));

            sparkPopTravelSeed = smoothstep(0., 
                                            0.75,  
                                            pow(sparkPopTravelSeed, vec3(1.5)) );

            audioAttenuateTriples += g_audioFreqs[i] * sparkPopTravelSeed;
        }
        
        // Take the sum of all of the audio attnuate triples for this
        // spark.
        float audioAttenuate = dot(audioAttenuateTriples, vec3(1.));

        audioAttenuate = mix(1., min(1., audioAttenuate), g_audioResponse);
    
        sparkAttenuate = mix(1.2 * sparkAlphaExtent, sparkAttenuate, audioAttenuate);

        // The core presence has a longer throw then the spark itself, which helps it
        // pop against sparks in the bg.
        float sparkCorePresence = smoothstep(sparkGlowExtent, 0., sparkAttenuate);
        float sparkAmt = smoothstep(sparkAlphaExtent, 0., sparkAttenuate);        

        sparkColor = g_sparkColorIntensity * mix(g_sparkCoreColor, 
                                                 g_sparkExtentColor, 
                                                 sparkResult.y);

        // Pump this spark based on the amount of audio passing through 
        // the spark.
        sparkColor = mix(sparkColor, 
                         1.25 * sparkColor, 
                         g_audioResponse * audioAttenuate);

        sparkColor = (1. - orbRay.color.a) * sparkColor * sparkCorePresence;
        sparkOpacity = (1. - orbRay.color.a) * sparkAmt;        

        // ------------------------------------------------------------------
        // Compute the cell bound marching
        
        // Test the boundary walls of a polar cell represented by the floor or
        // ceil of the polarized march pt.  The boundary walls to test are
        // determined based on how we're marching through the orb cells.

        // Remember that x is the latitudinal angle so to find it's boundary,
        // we intersect with a cone.  y is the azimuth angle so we can more
        // simply intersect the plane that is perpendicular with the x-z plane
        // and rotated around the y-axis by the value of the angle.

        // Remember to remap the floors and ceils to   
        // Latitudinal      := [-PI/2, PI/2]
        // Azimuth          := [-PI, PI]
        vec4 orbCellBounds = floorAndCeil2(orbRay.orbCellCoords) *
            vec2(PI, TWO_PI).xxyy / g_numOrbCells.xxyy;
        
        orbCellBounds += g_sparkRotationRate.xxyy * g_time;
        
        float nextRelativeDist = orbRay.marchMaxDist - orbRay.marchDist;
        float t = BIG_FLOAT;        
        vec2 cellCoordIncr = vec2(0.);

        // Intersect with the planes passing through the origin and aligned
        // with the y-axis.  The plane is a rotation around the y-axis based
        // on the  azimuthal boundaries.  Remember we know which direction the
        // ray is traveling so we only need to test one side of the cell.        
        float orbNextCellAzimAngle = (orbRay.azimuthalOrbCellMarchDir < 0 ? 
                                        orbCellBounds.z : orbCellBounds.w);

        vec3 orbNextCellAzimBound = vec3(-sin(orbNextCellAzimAngle), 
                                          0., 
                                          cos(orbNextCellAzimAngle));

        vec2 intersectResult = intersectDSPlane(marchPoint, orbRay.dir, 
                                                orbNextCellAzimBound, vec3(0.) );

        // DEBRANCHED
        // Equivalent to:
        //
        // if ((intersectResult.x > 0.5) && (nextRelativeDist > intersectResult.y)) {
        //    nextRelativeDist = intersectResult.y;
        //    cellCoordIncr = vec2(0., orbRay.azimuthalOrbCellMarchDir);            
        // }

        float isAzimPlaneHit = intersectResult.x * step(intersectResult.y, nextRelativeDist);
        nextRelativeDist = mix(nextRelativeDist, intersectResult.y, isAzimPlaneHit);
        cellCoordIncr = mix(cellCoordIncr, vec2(0., orbRay.azimuthalOrbCellMarchDir), isAzimPlaneHit);
        
        // XXX Future work: It would be nice if we only test one side of the
        // cell wall in the latitudinal direction based on the direction of the
        // orb ray as it marches through the cells, like what I'm doing with the
        // azimuthal direction. But due to some shader issues and the extra
        // complexity this adds to the  code, this seems like a more stable
        // approach.  If we do go down this road, you could determine which
        // direction the ray is travelling but you'll need to test if that ray
        // crosses the "dividing plane" since the "negative" direction will
        // become the "positive" direction at  that point.  Perhaps this
        // indicates I need to rethink how the orb cell coordinates are
        // defined.  I think solving this problem will get to the bottom of the
        // black speckling that happens.

        // Test the top of the current orb cell
        intersectResult = intersectSimpleCone(marchPoint, orbRay.dir, orbCellBounds.x);

        // DEBRANCHED
        // Equivalent to:
        //
        // if ((intersectResult.x > 0.5) && (nextRelativeDist > intersectResult.y)) {
        //    nextRelativeDist = intersectResult.y;
        //    cellCoordIncr = vec2(-1., 0.);            
        // }
        
        float isTopConeHit = intersectResult.x * step(intersectResult.y, nextRelativeDist);
        nextRelativeDist = mix(nextRelativeDist, intersectResult.y, isTopConeHit);
        cellCoordIncr = mix(cellCoordIncr, vec2(-1, 0.), isTopConeHit);
    

        // Test the bottom of the current orb cell
        intersectResult = intersectSimpleCone(marchPoint, orbRay.dir, orbCellBounds.y);

        // DEBRANCHED
        // Equivalent to:
        //
        // if ((intersectResult.x > 0.5) && (nextRelativeDist > intersectResult.y)) {
        //    nextRelativeDist = intersectResult.y;
        //    cellCoordIncr = vec2(-1., 0.);            
        // }

        float isBottomConeHit = intersectResult.x * step(intersectResult.y, nextRelativeDist);
        nextRelativeDist = mix(nextRelativeDist, intersectResult.y, isBottomConeHit);
        cellCoordIncr = mix(cellCoordIncr, vec2(1, 0.), isBottomConeHit);
        

        // ------------------------------------------------------------------
        // Update orbRay for next march step

        // We now know what cell we're going to march into next
        // XXX: There is a fudge factor on the EPSILON here to get around some
        // precision issues we're seeing with intersecting the simple cones. 
        // This probably indicates there is something flawed in the cell logic
        // traversal.
        orbRay.marchNextDist = min(orbRay.marchMaxDist, orbRay.marchDist + nextRelativeDist + .01);
        orbRay.orbCellCoords += cellCoordIncr;

        // Make sure that y wraps (the azimuthal dimension) when you've reached
        // the extent of the number of orb cells
        orbRay.orbCellCoords.y = mod(orbRay.orbCellCoords.y + g_numOrbCells.y/2., 
                                     g_numOrbCells.y) - g_numOrbCells.y/2.;
        
        orbRay.marchDist = orbRay.marchNextDist;        
        
        // ------------------------------------------------------------------
        // Shade the center of the orb interior itself (only if this cell's
        // march has intersected with that center interior).  Avoid an if 
        // statement here by having the signal that drives the presence of 
        // this color come on only when the ray is intersecting that orb
        // interior.

        vec3 marchExit = orbRay.origin + orbRay.dir * orbRay.marchNextDist;  

        vec4 segToPtResult = segmentToPointDistance(marchPoint,
                                                    marchExit,
                                                    vec3(0.));
        
        float distToCenter = length(segToPtResult.xyz);

        float centerOrbProximityMask = smoothstep(g_orbCenterRadius + 1., 
                                                  g_orbCenterRadius, 
                                                  distToCenter);

        float sparkOriginGlowMaxDist = mix(3.0, 0.5, 
                                           smoothstep(8., 
                                                      128., 
                                                      min(g_numOrbCells.x, 
                                                          g_numOrbCells.y)));
        
        float sparkOriginGlow = min(sparkOriginGlowMaxDist, abs(sparkOriginGlowMaxDist - 
                                      distance(segToPtResult.xyz, sparkOrigin)));
        
        sparkOriginGlow = audioAttenuate * pow(sparkOriginGlow, 3.5);

        // Spark origin contribution, color a signal based on this particular marches
        // closest point to the orb origin and that distance to the actual spark 
        // origin.
        sparkColor += .06 * centerOrbProximityMask * sparkOriginGlow * g_sparkCoreColor;
        sparkOpacity += .06 * centerOrbProximityMask * sparkOriginGlow;

        float isMarchIntersectingCenterOrb = smoothstep(g_orbCenterRadius + .1, 
                                                        g_orbCenterRadius, distToCenter);

        float centerOrbGlow = .4 * g_sparkColorIntensity * max(0., 
                    1. - dot(-normalize(segToPtResult.xyz), orbRay.dir));

        centerOrbGlow = pow(centerOrbGlow, .6);

        // Center orb glow (diffuse falloff) - independent of sparks
        sparkColor += max(0., (1. - sparkOpacity)) * g_sparkCoreColor * centerOrbGlow * isMarchIntersectingCenterOrb;
                
        orbRay.color.rgb += (1. - orbRay.color.a) * sparkColor;
        orbRay.color.a += (1. - orbRay.color.a) * sparkOpacity;
                                
        // If we want to terminate march (because we intersected the center orb), 
        // then make sure to make the alpha for this orb ray 1. 
        orbRay.color.a += (1. - orbRay.color.a) * isMarchIntersectingCenterOrb;        

    }
    
}

vec3 sampleEnvironment(vec3 normalizedDir)
{

    vec3 skyColor     = vec3(0.55, 0.45, 0.58);
    vec3 horizonColor = vec3(0.50, 0.35, 0.55);

    float envAmp = 1.0 * (1. + .1 * g_bassBeat);
    return envAmp * mix(horizonColor, skyColor, smoothstep(-.5, 1.0, normalizedDir.y)); 
}

void simpleShadeOrb(inout SceneRayStruct sceneRay)
{

    vec3 hitPoint = sceneRay.orbHitPoint.xyz;
    vec3 rayDir   = sceneRay.dir;

    // We march the interior assuming the sphere is at the origin.        
    vec3 hitNormal = normalize(hitPoint);

    float falloff = dot(hitNormal, -sceneRay.dir);
    sceneRay.color.rgb = vec3(1. - falloff);
    sceneRay.color.a = 1.;
    
}

void shadeOrb(inout SceneRayStruct sceneRay)
{

    // Create a unique hash for this particular cell. We're assuming
    // that the number of visible cells won't exceed 100x100x100.
    float orbHash = (sceneRay.cellCoords.x + 
                     100. * sceneRay.cellCoords.y + 
                     10000. * sceneRay.cellCoords.z);

    // --------------------------------------------------------------------
    // by rotating the hitPoint and the ray direction around the 
    // origin of the orb, we can add rotational variety from orb
    // to orb. 
    vec3 hitPoint = sceneRay.orbHitPoint.xyz;
    vec3 rayDir   = sceneRay.dir;

    float rotateXAngle = 0.125 * g_time + orbHash;
    float cosRotateXAxis = cos(rotateXAngle);
    float sinRotateXAxis = sin(rotateXAngle);    
    
    hitPoint = rotateAroundXAxis(hitPoint, cosRotateXAxis, sinRotateXAxis);
    rayDir   = rotateAroundXAxis(rayDir, cosRotateXAxis, sinRotateXAxis);

    float rotateYAngle = 0.125 * g_time + orbHash;
    float cosRotateYAxis = cos(rotateYAngle);
    float sinRotateYAxis = sin(rotateYAngle);

    hitPoint = rotateAroundYAxis(hitPoint, cosRotateYAxis, sinRotateYAxis);
    rayDir   = rotateAroundYAxis(rayDir, cosRotateYAxis, sinRotateYAxis);
    
    // We march the interior assuming the sphere is at the origin.        
    vec3 hitNormal = normalize(hitPoint);
    
    // --------------------------------------------------------------------
    // Environment map reflection on orb surface

    vec3 reflectDir = reflect(rayDir, hitNormal);
    vec3 reflColor = sampleEnvironment(reflectDir);
    
    float reflectRatio = fresnel(rayDir, hitNormal, 1. / g_orbIOR).x;
    float kr = mix(g_orbSurfaceFacingKr, g_orbSurfaceEdgeOnKr, reflectRatio);
    sceneRay.color.rgb += kr * reflColor;
    
    sceneRay.color.a += (1. - sceneRay.color.a) * min(1., kr);//reflectRatio;

    // --------------------------------------------------------------------
    // Calculate the interior distance to march
     
    vec3 refractDir = refract(rayDir, hitNormal, 1. / g_orbIOR);    
        
    // Consider the interior sphere (the emitter) when ray marching
    vec3 innerIntersectResult = intersectSphere(hitPoint, 
                                                refractDir, 
                                                g_orbCenterRadius,
                                                vec3(0.));

    float interiorSphereHitDist = mix(BIG_FLOAT, 
                                      innerIntersectResult.y, 
                                      innerIntersectResult.x);

    // --------------------------------------------------------------------
    // Orb Interior Shading     

    vec3 orbIntersectResult = intersectSphere(hitPoint, 
                                              refractDir,
                                              g_orbOuterRadius,
                                              vec3(0.));
    
    float sphereDepth = abs(orbIntersectResult.z - orbIntersectResult.y);    
    float rayExtent = min(interiorSphereHitDist, sphereDepth);        

    OrbRayStruct orbRay = OrbRayStruct(hitPoint, // origin
                                       refractDir, // dir
                                       0., // ray parameterized start
                                       0., // the next ray march step
                                       rayExtent, // ray parameterized end
                                       0, //  azimuthalOrbCellMarchDir
                                       vec2(0.), // orbCellCoords
                                       orbHash, // unique float for this orb
                                       vec4(0.), // color
                                       vec4(0.));  // debugColor 

    shadeOrbInterior(orbRay, sceneRay); 

    // --------------------------------------------------------------------
    // Transfer the orb ray march results to the scene ray trace.

    sceneRay.color.rgb += (1. - sceneRay.color.a) * orbRay.color.rgb;
    sceneRay.color.a += (1. - sceneRay.color.a) * orbRay.color.a;

    sceneRay.debugColor += orbRay.debugColor;

}

void animateGlobals()
{

    // Change the number of polar cells along the azimuthal and latitudinal axis
    // over time using a sawtooth signal. 
    //g_numOrbCells.x = 16. + 4. * (2. * (sawtooth( g_numOrbsChangeRate.x * g_time + 1.)) +
    //                                2.);
    
    g_numOrbCells.y = 8. + 8. * pow(2., floor(2. * sawtooth( g_numOrbsChangeRate.y * g_time + 4.) + 
                                       2.));
    
    // inflate and deflate the cell box size over time using a sine function. 
    g_cellBoxHalfSize = CELL_BOX_HALF_SIZE_DEFAULT * (1. + 0.5 * (0.5 * sin(.25 * (g_beatRate * g_time / TWO_PI)) + .5)); 

    
    // Ramp into the audio driving the spark signals.
    g_audioResponse = mix(0.4, 0.9, smoothstep(0., 5., g_time));
    
    // Would be nice if I could find a way to ramp off of the beat better, but that
    // would require some history knowledge.
    g_bassBeat = g_audioResponse * smoothstep(0.93, 1.01, sin(iGlobalTime));

    // For each audio sample, sample the audio channel along the x-axis (increasing frequency
    // with each sample).  
    for (int i = 0; i < NUM_AUDIO_SAMPLE_TRIPLES; i += 1) {
        vec3 indexes = vec3(3. * float(i) + 0., 
                            3. * float(i) + 1.,
                            3. * float(i) + 2.);

        vec3 audioOffsets = indexes * (0.95/float(NUM_AUDIO_SAMPLE_TRIPLES*3));
        g_audioFreqs[i] = smoothstep(-0.4, 0.9, vec3(sin(i*iGlobalTime + PI * audioOffsets.x),
                                                     sin(i*iGlobalTime + PI * audioOffsets.y),
                                                     sin(i*iGlobalTime + PI * audioOffsets.z)));
    }
    
    // In thumbnail mode, show at least some of the audio sparks
    if (iResolution.y < 250.) 
        g_audioFreqs[0] = vec3(1.);
    
    // Thump the color intensity, and fog color based on the bass beat.
    g_sparkColorIntensity = mix(1., 1.3, g_bassBeat);
    g_fogColor *= mix(1., 1.4, g_bassBeat);

    // remap the mouse click ([-1, 1], [-1/ar, 1/ar])
    vec2 click = iMouse.xy / iResolution.xx;    
    click = 2.0 * click - 1.0;   
    
    // camera position
    vec3 lookAtOffset = vec3(0., 0., -20.);
    
    // Then add an offset on top of the lookAtOffset so that we "crane"
    // the camera 200 units away from the origin.  We can then slowly
    // rotate the crane around it's azimuth to explore the scene.
    g_camOrigin = vec3(0., 0.0 , -200.);
    g_camPointAt = g_camOrigin + lookAtOffset;


}

// **************************************************************************
// MAIN

void main(void)
{

    // ----------------------------------
    // Animate globals

    animateGlobals();

    // ----------------------------------
    // Setup Camera

    // Shift p so it's in the range -1 to 1 in the x-axis and 1./aspectRatio
    // to 1./aspectRatio in the y-axis (a reminder aspectRatio := width /
    // height of screen)

    // I could simplify this to:
    // vec2 p = gl_FragCoord.xy / iResolution.xx; <- but that's a bit obtuse
    // to read.
    
    vec2 p = gl_FragCoord.xy / iResolution.xy;      
    float aspectRatio = iResolution.x / iResolution.y;
    p = 2.0 * p - 1.0;
    p.y *= 1./aspectRatio;
    
    // calculate the rayDirection that represents mapping the  image plane
    // towards the scene
    vec3 cameraZDir = -normalize( g_camPointAt - g_camOrigin );
    vec3 cameraXDir = normalize( cross(cameraZDir, g_camUpDir) );

    // no need to normalize since we know cameraXDir and cameraZDir are orthogonal
    vec3 cameraYDir = cross(cameraXDir, cameraZDir);
    
    
    mat4 camXform =  mat4(vec4(cameraXDir, 0.), 
                          vec4(cameraYDir, 0.), 
                          vec4(cameraZDir, 0.),
                          vec4(0., 0., 0., 1.)) * iCameraTransform; 
    
    g_camOrigin += 40. * iCameraTransform[3].xyz;

    vec3 rayDir = normalize( p.x*camXform[0].xyz + p.y*camXform[1].xyz + 1.8 * camXform[2].xyz );    
        
    // Add a clipping plane bias so orbs don't get too close to camera
    g_camOrigin += 5. * rayDir;
    SceneRayStruct ray = SceneRayStruct(g_camOrigin, // origin
                                        rayDir, // direction
                                        0., // current march depth
                                        FOG_EXTENT, // max march depth
                                        vec3(0.), // cellCoords
                                        vec4(0.), // orbHitPoint
                                        vec4(0.), // color
                                        vec4(0.)); // debug color
    
    // ----------------------------------
    // Trace Scene

    traceScene(ray);

    // ----------------------------------
    // Shade Scene

    if ((ray.marchDist < FOG_EXTENT) && (ray.orbHitPoint.w > .5)) {
        //simpleShadeOrb(sceneRay);
        shadeOrb(ray);        
    }

    // ----------------------------------
    // Harvest final color results

    vec4 tracedColor = ray.color;
    
    // multiply the alpha onto the color.  If you want info on why I do this,
    // consult the source paper:
    // http://graphics.pixar.com/library/Compositing/paper.pdf
    tracedColor.rgb *= tracedColor.a;

    // ----------------------------------
    // Fog

    tracedColor.rgb = mix(tracedColor.rgb, g_fogColor, 
                          smoothstep(50., FOG_EXTENT, ray.marchDist));

    // ----------------------------------
    // Color grading

    // Increase contrast
    tracedColor.rgb = pow(tracedColor.rgb, vec3(1.7));
    
    // vigneting
    vec2 uv = p*0.5+0.5;
    float vignet = pow(10.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y),0.4);        
    tracedColor *= mix(0.3, 1., vignet);

    gl_FragColor = tracedColor;
    
    // Debug color's alpha is not multiplied during final output.  If it is 
    // non-zero, then the debugcolor is respected fully 
    gl_FragColor.rgb = mix(gl_FragColor.rgb, ray.debugColor.rgb, 
                           step(EPSILON, ray.debugColor.a));
}