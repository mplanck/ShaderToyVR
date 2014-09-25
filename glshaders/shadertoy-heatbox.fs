iChannel0 = noise_rgb_256
ScreenPercentage = 1.

::::::::::::

// **************************************************************************
// CONFIG OPTIONS

// Had to default to lowest quality since the shader doesn't compile
// on Windows Chrome (most likely due to the heavy nesting in
// spheresdf - thinking up ways to optimize that).  But for mac and
// linux users on chrome, feel free to comment in MEDIUM_QUALITY or
// HIGH_QUALITY to see some diffuse spill on the box walls and some
// subtle reflections.

#define HIGH_QUALITY
//#define MEDIUM_QUALITY
//#define LOW_QUALITY

#ifdef HIGH_QUALITY
#define BOXWALL_DIFFUSE 1
#define BOXWALL_REFLECTIONS 1
#define DISTMARCH_STEPS 100
#endif 

#ifdef MEDIUM_QUALITY
#define BOXWALL_DIFFUSE 1
#define BOXWALL_REFLECTIONS 0
#define DISTMARCH_STEPS 80
#endif 

#ifdef LOW_QUALITY
#define BOXWALL_DIFFUSE 0
#define BOXWALL_REFLECTIONS 0
#define DISTMARCH_STEPS 80
#endif 

#define HEAT_PLUMES_ONLY 0
// set to 1 in order to isolate the cooler of the 4 effects

#define CELLBOX_SIZE 1.5  
// play with this to make the spheres bigger or smaller

#define NUM_CELLBOXES 10. 
// play with this to increase the 3d grid size - make sure it is a
// multiple of 2

#define BOXWALL_EXTENT 1.2 * NUM_CELLBOXES * CELLBOX_SIZE 
// play with this to increase the wall boundaries onto which the
// diffuse and specular shaders fall

#define NUM_AUDIO_SAMPLES 3
// You can have the sampling of the audio be more  granular by
// increasing this number.  Be warned that Windows may not be able to
// handle values greater than 3.

// *******************************************************************
// CONSTANTS

#define PI 3.14159
#define TWO_PI 6.28318
#define PI_OVER_TWO 1.570796

#define EPSILON 0.001
#define BIG_FLOAT 1000000.

#define NOISE_DIMENSION 256.

// *******************************************************************
// GLOBALS

// - - - - - - - - - - - - -
// Time properties

float g_time = iChannelTime[1];
float g_beatRate = 120.;

// - - - - - - - - - - - - -
// Camera properties

vec3 g_camOrigin = vec3(0., 0., 0.);
vec3 g_camPointAt = vec3( 0.0, -1.0, 0.0 );
vec3 g_camUpDir = vec3( 0.0, 1.0, 0.0 );

vec2 g_camRotationRates = vec2(0.001125 * g_beatRate, 
                               -0.00125 * g_beatRate);

// - - - - - - - - - - - - -
// Audio properties

float g_audioFreqs[ NUM_AUDIO_SAMPLES ];

// - - - - - - - - - - - - - 
// Signal properties

vec3 g_plumeOrigin = vec3(0.);
vec3 g_heatWaveDir = vec3(.1, .8, .3);

// *******************************************************************
// UTILITIES

// Assumes the bounds represent a space centered around the origin, so
// if the bounds (2, 2, 2) are provided, then this returns true if p
// is within a box of 2x2x2 centered at the origin.

bool inbounds( vec3 p, 
               vec3 bounds )
{
    return (all(lessThanEqual(p, .5 * bounds)) &&
            all(greaterThanEqual(p, -.5 * bounds)));
}

// Rotate the input point around the y-axis by the angle given as a
// cos(angle) and sin(angle) argument.  There are many times where  I
// want to reuse the same angle on different points, so why do the
// heavy trig twice. Range of outputs := ([-1.,-1.,-1.] -> [1.,1.,1.])

vec3 rotateAroundYAxis( vec3 point, float cosangle, float sinangle )
{
    return vec3(point.x * cosangle  + point.z * sinangle,
                point.y,
                point.x * -sinangle + point.z * cosangle);
}

// Rotate the input point around the x-axis by the angle given as a
// cos(angle) and sin(angle) argument.  There are many times where  I
// want to reuse the same angle on different points, so why do the
// heavy trig twice. Range of outputs := ([-1.,-1.,-1.] -> [1.,1.,1.])

vec3 rotateAroundXAxis( vec3 point, float cosangle, float sinangle )
{
    return vec3(point.x,
                point.y * cosangle - point.z * sinangle,
                point.y * sinangle + point.z * cosangle);
}

// Returns a smoothed vec3 noise sample based on input n.  Noise
// samples are within the range ([0., 0., 0.] -> [1., 1., 1.])

vec3 noise1v( float n )
{   
    
    vec2 coords = vec2(mod(n,NOISE_DIMENSION)/NOISE_DIMENSION, 
                       floor(n/NOISE_DIMENSION)/NOISE_DIMENSION);
    
    return texture2D(iChannel0, coords).rgb;
} 

// Lerps between the discrete audio samples gathered during
// animateGlobals.  The parameter t is within the range [0, 1]

float audioAttenuate(float t)
{    
    float lookup = t * 0.95 * float(NUM_AUDIO_SAMPLES-1);

    float signal = 0.;
    for (int i = 0; i < NUM_AUDIO_SAMPLES-1; i += 1) {
        if ( int(lookup) == i )
        {
            signal = mix(g_audioFreqs[i], 
                         g_audioFreqs[i+1], 
                         fract(lookup));
        }
    }
   
    return signal;
}

// *******************************************************************
// DISTANCE FUNCTIONS

// Returns a vec3 that represents the termination of the distance
// field  marching.

//  x := the distance to the nearest object
//  y := the material id of the nearest object (0 if non found)
//  z := the heat of the nearest object (if applicable)

vec3 spheredf( vec3 p, 
               vec3 cellcoords )
{


    // This default result has no material or heat, but does return a
    // default distance of half a cell size.  The is the "skipping"
    // distance since it will most likely jump to the next cell.  We
    // could optimize this "skip" "distance" by actually doing the
    // math to find the minimum amount that jumps to the next cell,
    // but this in practice gets the job done.
    vec3 result = vec3(.5 * CELLBOX_SIZE, 0., 0.);

    // Don't process the current cell if we're not in the box bound.
    // We use this check since we use distance lookups to find the
    // outer edge of the box of spheres with a 1 cell buffer, so the
    // "outer" cells can have "over extended" distance samples even
    // though those buffer cells do not contain any spheres.
    if ( inbounds( p, vec3( NUM_CELLBOXES * CELLBOX_SIZE ) ) ) 
    {

        float signal = 0.;

#if !HEAT_PLUMES_ONLY
        int selector = int(mod(g_time/10.,5.));
#endif 

#if !HEAT_PLUMES_ONLY
        // I like the heat plumes so I give them a little extra time
        // when cycling.
        if (selector == 0 || selector == 1)  // heat plumes
        {
#endif

            float t = -g_beatRate * .04125 * g_time;    
            float basst = .5 + .5 * smoothstep(.6, 
                                               .98, 
                                               g_audioFreqs[0]);
            t = mod(t + .5 * length(cellcoords + g_plumeOrigin), 15.);
            signal = basst * (smoothstep(0., 3., t) - 
                              smoothstep(3., 5., t));

#if !HEAT_PLUMES_ONLY
        }
#endif

#if !HEAT_PLUMES_ONLY

        float audiot = smoothstep(NUM_CELLBOXES/2., 
                                  -NUM_CELLBOXES/2.,
                                  cellcoords.y);
        audiot = audioAttenuate(1. - audiot);

        if (selector == 2) // audio vertical heat waves
        {

            signal = audiot * (.5 * sin(g_beatRate * 0.03125 * g_time + 
                dot(g_heatWaveDir, cellcoords) ) + .5);        

        }
        if (selector == 3) // audio heat pulses
        {            

            audiot = .1 + .9 * audiot;
            signal = audiot * pow(.5 * sin(-g_beatRate * 0.03925 * g_time + 
                .3 * length(cellcoords + 
                    vec3(0., NUM_CELLBOXES/3., 0.))) + .5, 2.);
        }
        if (selector == 4) // audio eminating heat boxes
        {

            audiot = .1 + .9 * audiot;
            signal =  audiot * (.5 * sin(-g_beatRate * 0.03925 * g_time + 
                .8 * max(abs(cellcoords.x), 
                     max(abs(cellcoords.y), 
                         abs(cellcoords.z)))) + .5);
        }
#endif

        float heat = 10000. * signal; 
        // Don't consider this a valid cell if it isn't hot enough.  
        if (heat > 100.) {

            // Bloat the radius based on the radius scale signal.
            // There is always a little bit of sphere if there is
            // enough heat.
            float radius = .8 * (.9 * signal + .1) * 
                            .5 * CELLBOX_SIZE;

            // Push the sphere away from the origin based on the
            // displacement signal
            vec3 sphereCenter = cellcoords * CELLBOX_SIZE - 
                            vec3(CELLBOX_SIZE * .5);
            vec3 towardsCenter = -normalize(sphereCenter);                
            sphereCenter -= (2. * signal - 1.) * 
                            (.5 * CELLBOX_SIZE - radius) * 
                            towardsCenter;
            

            result = vec3(length( p - sphereCenter ) - radius, 
                          1., 
                          heat);
        }
    }
    
    return result;
}


// Returns a vec3 that represents the termination of the distance
// field  marching.

//  x := the distance to the nearest object
//  y := the material id of the nearest object (0 if non found)
//  z := the heat of the nearest object (if applicable)

vec3 spheresdf( vec3 p, 
                vec3 rdir ) 
{
    float dist = 10.;

    vec3 cellCoords = ceil(p / CELLBOX_SIZE);
    vec4 nextCellOffset = vec4(sign(rdir), 0.);
            
    // Look into this cell to check distance to that sphere and then
    // check the distance to the spheres for the next cells to see if
    // they are closer, we can guess the "next" cells by just looking
    // at the sign of the ray dir (so we don't check all 27 possible
    // cells - 9x9x9 grid).  This limits it down just to checking 8
    // possible cells.

    // XXX: we could limit this to 4 cell checks if we trace the 
    // ray through the 8 potential cells and only test the cells 
    // through which the ray passes.

    // 1. current cell
    vec3 result = spheredf( p, cellCoords );

    // 2. neighbor in the x direction of the ray
    vec3 neighborResult = spheredf(p, cellCoords + nextCellOffset.xww);    
    // DEBRANCHED - equivalent to
    // if (neighborResult.x < result.x) { result = neighborResult; }
    result = mix(result, neighborResult, 
                 step(neighborResult.x, result.x));

    // 3. neighbor in the y direction of the ray
    neighborResult = spheredf(p, cellCoords + nextCellOffset.wyw);
    result = mix(result, neighborResult, 
                 step(neighborResult.x, result.x));

    // 4. neighbor in the z direction of the ray
    neighborResult = spheredf(p, cellCoords + nextCellOffset.wwz);
    result = mix(result, neighborResult, 
                 step(neighborResult.x, result.x));
    
    // 5. neighbor in the x-y direction of the ray
    neighborResult = spheredf(p, cellCoords + nextCellOffset.xyw);
    result = mix(result, neighborResult, 
                 step(neighborResult.x, result.x));

    // 6. neighbor in the y-z direction of the ray
    neighborResult = spheredf(p, cellCoords + nextCellOffset.wyz);
    result = mix(result, neighborResult, 
                 step(neighborResult.x, result.x));

    // 7. neighbor in the x-z direction of the ray
    neighborResult = spheredf(p, cellCoords + nextCellOffset.xwz);
    result = mix(result, neighborResult, 
                 step(neighborResult.x, result.x));
        
    // 8. neighbor in the x-y-z direction of the ray
    neighborResult = spheredf(p, cellCoords + nextCellOffset.xyz);    
    result = mix(result, neighborResult, 
                 step(neighborResult.x, result.x));

    return result;
}

float boxdf( vec3 p, vec3 bounds )
{
    vec3 boxd = abs(p) - .5 * bounds;
    return length(max(boxd, 0.));
}

float boxinteriortrace( vec3 origin, 
                        vec3 rayDir, 
                        float halfCellSize )
{
    
    vec3 pn = -sign(rayDir);
    vec3 ddn = rayDir * pn;
    vec3 po = halfCellSize * -pn;
    vec3 t = -(pn * (origin - po))/ddn;
    float dist = min(t.x, min(t.y, t.z));    

    return dist;
}

// Returns a vec3 that represents the termination of the distance
// field  marching.

//  x := the distance to the nearest object
//  y := the material id of the nearest object (0 if non found)
//  z := the heat of the nearest object (if applicable)
vec3 scenedf( vec3 p, 
              vec3 rdir )
{
    vec3 result = vec3(1., 10., 0.);

    if (inbounds( p, vec3((NUM_CELLBOXES + 2.) * CELLBOX_SIZE ))) {
       vec3 dfresult = spheresdf( p, rdir );
       result = dfresult;
    } else {
       result = vec3(boxdf( p, vec3(NUM_CELLBOXES * CELLBOX_SIZE)), 
                     -1., 
                     0.);
    }

    float boxdist = boxinteriortrace(p, rdir, BOXWALL_EXTENT);

    // DEBRANCHED
    // equivalent to if (boxdist < dist) {
    result = mix(result, 
                 vec3(boxdist, 2., 0.), 
                 step(boxdist, result.x));

    return result;
}

// *******************************************************************
// DISTANCE MARCHING

// Returns a vec3 that represents the termination of the distance
// field  marching.

//  x := the distance to the nearest object
//  y := the material id of the nearest object (0 if non found)
//  z := the heat of the nearest object (if applicable)
vec3 distmarch( vec3 ro, vec3 rd, float maxd )
{
    
    float dist = 6.;
    float t = 0.;
    float material = 0.;
    float heat = 0.;
    for (int i=0; i < DISTMARCH_STEPS; i++) 
    {
        if (( abs(dist) < EPSILON || t > maxd) && material >= 0. ) 
            continue;

        t += dist;
        vec3 dfresult = scenedf( ro + t * rd, rd );
        dist = dfresult.x;
        material = dfresult.y;
        heat = dfresult.z;
    }

    if( t > maxd ) material = -1.0; 
    return vec3( t, material, heat );
}

// *******************************************************************
// SHADE FUNCTIONS

vec3 calcWallNormal( vec3 p,
                     vec3 rdir )
{
    vec3 epsilon = vec3( EPSILON, 0.0, 0.0 );
    vec3 n = vec3(
        boxinteriortrace(p + epsilon.xyy, rdir, BOXWALL_EXTENT) - 
            boxinteriortrace(p - epsilon.xyy, rdir, BOXWALL_EXTENT),
        boxinteriortrace(p + epsilon.yxy, rdir, BOXWALL_EXTENT) - 
            boxinteriortrace(p - epsilon.yxy, rdir, BOXWALL_EXTENT),
        boxinteriortrace(p + epsilon.yyx, rdir, BOXWALL_EXTENT) - 
            boxinteriortrace(p - epsilon.yyx, rdir, BOXWALL_EXTENT) );
    return normalize( n );
}

// Eye-balled values trying to represent black body temperature
// variation.   Expecting temperature to be in the range 0 to 10000.
vec3 sphereColor(float T)
{
    vec3 whiteHotColor = vec3(.88, .98, .97);
    vec3 hotColor  = vec3(.95, .78, .23); 
    vec3 coolColor = vec3(0.43, .01, .003);
    vec3 noheatColor = vec3(.0);
    return mix(mix(noheatColor, coolColor,
                   smoothstep(200., 1000., T)),
               mix(hotColor, whiteHotColor, 
                    smoothstep(8000., 10000., T)),
                smoothstep(1000., 8000., T));

}

// Return a vec3 representing the color contribution of the shaded
// wall.
vec3 shadeWall( vec3 p, 
                vec3 n,
                vec3 rdir )
{

    float kd = .1;
    float ks = .9;

    vec3 diffcol = vec3(0.0);
    vec3 speccol = vec3(0.0);
    vec3 ambcol = vec3(0.018, 0.013, 0.013);

    // DIFFUSE

    // jittered samples of the 8 corners of the heat box to get a
    // diffuse response.  Consider a box of dimension 2x2x2 centered
    // at the origin.  In the corner  around the normalized
    // coordinates (-.75, -.75, -.75), I jitter within  the range
    // [(-1, -1., -1.), (-.5, -.5, -.5)].  This is a hack that  gives
    // us bias coverage towards the outside of the heatbox, but since
    // we distance attenuate, one can argue the outer portions of the
    // box are more important in a hackish importance sampled scheme.

    // Here's a cross section of one side of the heat box.  The small
    // dots  in each corner represent the area in which the jittering
    // can happen and the x represents an example spot in which there
    // could be a color  lookup to drive the diffuse response on the
    // wall.

    // * --------------- *
    // | x  .   |   .x . |
    // | .  .   |   .  . |
    // |        |        |
    // |-----------------|
    // |        |        |
    // | . x.   |   .  . |
    // | .  .   |   .x . |
    // * --------------- *
    
#if BOXWALL_DIFFUSE
    
    vec3 lightp = vec3(0.);
    float halfBoxSize = .5 * NUM_CELLBOXES * CELLBOX_SIZE;
    for (float x = 0.; x < 2.; x++ ) {
        lightp.x = (-.75 + (1.5 * x)) * halfBoxSize;
    for (float y = 0.; y < 2.; y++ ) {
        lightp.y = (-.75 + (1.5 * y)) * halfBoxSize;
    for (float z = 0.; z < 2.; z++ ) {
        lightp.z = (-.75 + (1.5 * z)) * halfBoxSize;

        float noiseseed = x + 2. * y + 4. * z + 10. * g_time;
        vec3 noiseoffset = halfBoxSize * (noise1v( noiseseed ) - .5);

        vec3 ldir = (lightp + noiseoffset) - p;
        vec3 nldir = normalize(ldir);
        
        vec3 cellCoords = ceil(lightp / CELLBOX_SIZE);
        vec3 dfresult = spheredf( lightp, cellCoords );
                
        if (dfresult.y < 1.5)
        {
            float lenldir = length(ldir);
            float diffuse = clamp( dot( n, nldir ), 0., 1.);
            // attenuate falloff by a distance metric.
            float distatten = max(0., 1. - (lenldir/80.));
            // have the light falloff by a square factor
            distatten *= distatten;
            diffcol += diffuse * distatten * sphereColor(dfresult.z);
        }

    } } }

    diffcol /= 8.;
    diffcol *= 3. * vec3(0.4, .4, .5);
    
#endif

    // SPECULAR

    // Reflect the scene but have it fall off dramatically to give
    // a kind of deep onyx look.

#if BOXWALL_REFLECTIONS
    vec3 reflmarch = distmarch( p, rdir, 21. );

    if (reflmarch.y > 0.5 && reflmarch.y < 1.5) 
    {
        float distatten = max(0., 1. - reflmarch.x/30.);
        distatten *= distatten;
        speccol = distatten * sphereColor(reflmarch.z);
    }
#endif
    return ambcol + kd * diffcol + ks * speccol;
}

// *******************************************************************
// ANIMATE GLOBALS

void animateGlobals()
{

    // remap the mouse click ([-1, 1], [-1/ar, 1/ar])
    vec2 click = iMouse.xy / iResolution.xx;    
    click = 2.0 * click - 1.0;   
    
    // camera position
    g_camOrigin = vec3(0., -15.0 , 15.);

    // Rotate the camera around the origin

    g_camPointAt   = vec3(0., -15., 0.);
        
    // For each audio sample, sample the audio channel along the
    // x-axis (increasing frequency with each sample).
    for (int i = 0; i < NUM_AUDIO_SAMPLES; i += 1) {
        float offsets = float(i) * (0.95/float(NUM_AUDIO_SAMPLES));
        g_audioFreqs[i] = sin(g_time + i);
    }

#if !HEAT_PLUMES_ONLY
    // slightly modify the direction of the wave based on noise to add 
    // effect for the vertical heat wave event.
    g_heatWaveDir = vec3(0., 1., 0.);
    g_heatWaveDir += .3 * vec3(cos(.8 * g_time), 0., sin(.8 * g_time));
                         
#endif

    // calculate the plume origin that changes over time to add
    // interesting detail.
    float cosElev = cos(g_time * .2);
    float sinElev = sin(g_time * .2);

    float cosAzim = cos(g_time * .5);
    float sinAzim = sin(g_time * .5);
    
    g_plumeOrigin = NUM_CELLBOXES/2. * 
                       vec3(cosAzim * cosElev,
                            sinElev,
                            sinAzim * cosElev);
}

// *******************************************************************
// MAIN

void main(void)
{   
    
    // ----------------------------------
    // Animate globals

    animateGlobals();

    // ----------------------------------
    // Setup Camera

    // Shift p so it's in the range -1 to 1 in the x-axis and
    // 1./aspectRatio to 1./aspectRatio in the y-axis (a reminder
    // aspectRatio := width / height of screen)

    // I could simplify this to:
    // vec2 p = gl_FragCoord.xy / iResolution.xx; <- but that's a bit
    // obtuse to read.
    
    vec2 p = gl_FragCoord.xy / iResolution.xy;      
    float aspectRatio = iResolution.x / iResolution.y;
    p = 2.0 * p - 1.0;
    p.y *= 1./aspectRatio;
    
    // calculate the rdirection that represents mapping the  image
    // plane towards the scene
    vec3 cameraZDir = -normalize( g_camPointAt - g_camOrigin );
    vec3 cameraXDir = normalize( cross(cameraZDir, g_camUpDir) );

    // no need to normalize since we know cameraXDir and cameraZDir
    // are orthogonal
    vec3 cameraYDir = cross(cameraXDir, cameraZDir);
    
    mat4 camXform =  mat4(vec4(cameraXDir, 0.), 
                          vec4(cameraYDir, 0.), 
                          vec4(cameraZDir, 0.),
                          vec4(0., 0., 0., 1.)) * iCameraTransform; 
    
    g_camOrigin += camXform[3].xyz;
    
    vec3 rdir = normalize( p.x*camXform[0].xyz + p.y*camXform[1].xyz + 
                           2. * camXform[2].xyz );  

    // ----------------------------------
    // Scene Marching

    vec3 scenemarch = distmarch( g_camOrigin, rdir, 100. );

    vec4 scenecol = vec4( vec3(0.01, 0.01, 0.016), 1.);
    
    if (scenemarch.y > 0.5 && scenemarch.y < 1.5) 
    {
        scenecol.rgb = sphereColor(scenemarch.z);
    }
    
    else if (scenemarch.y > 1.5 && scenemarch.y < 2.5)
    {
        vec3 wp = g_camOrigin + scenemarch.x * rdir;
        vec3 wn = calcWallNormal( wp, rdir );
        vec3 wrefldir = normalize(reflect(wp - g_camOrigin, wn));

        scenecol.rgb += shadeWall(wp, wn, wrefldir);
        
    }
    
    // ----------------------------------
    // Grading

    // fall off exponentially into the distance (as if there is a spot
    // light on the point of interest).
    scenecol *= exp( -0.0005*scenemarch.x*scenemarch.x );

    // gamma correct
    scenecol.rgb = pow(scenecol.rgb, vec3(.6));

    gl_FragColor.rgb = scenecol.rgb;
    gl_FragColor.a = 1.;
    
}