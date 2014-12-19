iChannel0 = stone_tiles
iChannel1 = colored_rocks
iChannel2 = noise_r_256
ScreenPercentage = .8

::::::

// **************************************************************************
// CONSTANTS

#define PI 3.14159
#define TWO_PI 6.28318
#define PI_OVER_TWO 1.570796

#define REALLY_SMALL_NUMBER 0.0001
#define REALLY_BIG_NUMBER 1000000.

// **************************************************************************
// INLINE MACROS

// **************************************************************************
// DEFINES

// **************************************************************************
// GLOBALS

float g_time        = 0.;
vec4  g_debugcolor  = vec4(0.);

float g_cellsize    = 1.;

// **************************************************************************
// MATH UTILITIES

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
    
    float fterm = pow(1. - dot(-I, N), 5.);  
    return vec3(ro + ( 1. - ro ) * fterm); 
}

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

// Step function that starts at y =0 at x=0, then smoothsteps
// up to y=1 at x=0 using a sin.  Then smoothsteps to y=2 at x=2
// and so on.
float sinstep( float x )
{
    return (sin(TWO_PI * x-PI) + (TWO_PI * x - PI) + PI)/TWO_PI;
}

// Step function that starts at y =0 at x=0, then smoothsteps
// up to y=1 at x=0 using a smoothstep.  Then smoothsteps to y=0 at x=2.
// Repeats with a period of 4.
float periodicsmoothstep( float x )
{
    float mx = mod(x, 4.);
    return smoothstep(0., .2, mx) - smoothstep(2., 2.2, mx);
}

// *************************************************************************
// INTERSECTION MATH

// Intersection test against an infinitely tall horizontal box of square
// dimension.  Provide the square dimension as a half length. Tries to 
// anti-alias using a smoothstep across "edge" cases.

// out params are two hit results for the 2 planes of the column

//   plane{1,2}r.x = alpha of hit (used for anti-aliasing)
//   plane{1,2}r.y = t such that ray_origin + t*ray_direction = intersection point
//   plane{1,2}r.zw = x and z coordinates of normal

// In most cases, one of the plane hit results will have an alpha of 0.

void
intersect_tower(vec3 ro, vec3 rd, 
                float hd,
                out vec4 plane1r, 
                out vec4 plane2r)
{    
    // quick way to intersect against 2 planes at once
    // based on the incoming ray direction.  Then decide
    // intersection based on if the intersection is bounded
    // within the half length.
    
    vec2 pn = -sign(rd.xz);
    vec2 po = vec2(hd) * pn;
    vec2 ddn = -rd.xz * pn;
    
    vec2 t = (pn * (ro.xz - po))/ddn;
    vec2 ip = ro.zx + rd.zx * t;
    
    //vec2 fw = clamp(2. * fwidth(abs(ip)).xy, .05, 1.);
    vec2 fw = vec2(.05);
    vec2 amask = smoothstep(vec2(hd) + fw, vec2(hd), abs(ip));    
    
    // Best to recognize "corner" case and treat this as
    // two semi-transparent plane edges overlapping.      
    plane1r = vec4(amask.x, t.x, vec2(pn.x, 0.)); 
    plane2r = vec4(amask.y, t.y, vec2(0., pn.y));    
    
}

// **************************************************************************
// INFORMATION HOLDERS (aka DATA STRUCTURES)

struct CameraInfo
{
    vec3 camera_origin;
    vec3 ray_look_direction;
    vec2 image_plane_uv;
};

// Define a macro for struct initialization so as you add properties, you 
// can update the initializer right here and don't have to find all of your
// references through out your code.
#define INIT_CAMERA_INFO() SurfaceInfo(vec3(0.) /* camera_origin */, vec3(0.) /* ray_look_direction */, vec2(0.) /* image_plane_uv */)

// **************************************************************************
// SETUP WORLD

void setup_globals()
{
    // Way to globally control playback rate.
    g_time = iGlobalTime;
}

CameraInfo setup_camera()
{
    
    vec3 camera_origin    = vec3(0.0, 0., 0.0);
        
    // aspect_ratio := View Width / View Height
    // inv_aspect_ratio := View Height / View Width
    float inv_aspect_ratio = iResolution.y / iResolution.x;
    vec2 image_plane_uv = gl_FragCoord.xy / iResolution.xy - .5;

    // multiply by the inverse aspect ratio so that we don't have squashing
    // along the vertical axis.  This means that the image_plane_uv will be 
    // in the range ([-1,1], [-1/AspectRatio, 1/AspectRatio])
    image_plane_uv.y *= inv_aspect_ratio;

    // calculate the ray origin and ray direction that represents mapping the
    // image plane towards the scene through a pin-hole camera.  Assume the 
    // camera is trying to orient its roll as close to true up 
    // (along the positive y-axis) as possible.  This math breaks down if the 
    // camera is looking along the absolute y-axis.

    vec3 ww = vec3(0., 0., -1.);
    vec3 uu = normalize(cross( vec3(0.0,1.0,0.0), ww ));
    vec3 vv = normalize(cross(ww,uu));
    
    mat4 camXform =  mat4(vec4(uu, 0.), 
                          vec4(vv, 0.), 
                          vec4(ww, 0.),
                          vec4(0., 0., 0., 1.)) * iCameraTransform; 
                                            // ^-- push the camera transform of
                                            // the Rift onto the camXform
                                              
    camera_origin += camXform[3].xyz; // <-- to capture stereo viewing, you'll need 
                         			  // account for the eye offset

    //float fl = iFocalLength; // <-- often need to adjust the focal length until it feels
    //                         // right - use the sphere grid to calibrate
                                
    float fl = .8;
    vec3 ray_look_direction = normalize( image_plane_uv.x * camXform[0].xyz + image_plane_uv.y * camXform[1].xyz + fl * camXform[2].xyz );

    return CameraInfo(camera_origin, ray_look_direction, image_plane_uv);

}

// **************************************************************************
// SHADE WORLD

vec3 tower_lightcolor(vec2 coords)
{
    return mix( texture2D(iChannel0, (10./iChannelResolution[0].xy) * coords).rgb,
                texture2D(iChannel1, (10./iChannelResolution[1].xy) * coords).rgb,
                periodicsmoothstep( .05 * g_time - .005 * (coords.x + coords.y) - 1.));
}

float tower_mask(vec2 cell_coords)
{
    return step(4., dot(abs(cell_coords), vec2(1.)));
}

vec3 tower_lightpos(vec2 cell_coords, vec3 tower_color)
{
    float light_value = tower_mask(cell_coords) * tower_color.r;
    return vec3(0., mod(5. * g_time + 200. * light_value, 110.) - 100., 0.);

    // return vec3(0., mod(5. * g_time + 10. * (cell_coords.x + 4. * cell_coords.y), 110.) - 100., 0.);
    // return vec3(0., mod(10. * g_time + 2. * cell_coords.x, 200.) - 100., 0.);
}

float tower_rotoffset(vec2 coords)
{
    return texture2D(iChannel2, (10./iChannelResolution[2].xy) * coords).r;
}

vec3 
neighbor_light( vec3 hp, vec3 n, vec3 neighbor_cell_coord)
{
   vec3 neighbor_color = tower_lightcolor(neighbor_cell_coord.xz);
   vec3 light_pos = tower_lightpos(neighbor_cell_coord.xz, neighbor_color);
   light_pos += g_cellsize * vec3(neighbor_cell_coord.x + .5, 0., neighbor_cell_coord.z + .5);
   vec3 l = hp - light_pos;        
   float llen = length(l);
   return neighbor_color * max(0., dot(-normalize(l), n)) * pow(1./llen, .3);
}

// Query 4 neighbors for their diffuse irradiance selected based on facing normal
vec3 
neighbors_diffuse( vec3 hp, vec3 n, vec3 cell_coord )
{

    vec3 rs = sign(n);
    vec3 bounce = vec3(0.);
    bounce += neighbor_light(hp, n, cell_coord + vec3(0.,   0., rs.z));
    bounce += neighbor_light(hp, n, cell_coord + vec3(rs.x, 0., rs.z));
    bounce += neighbor_light(hp, n, cell_coord + vec3(rs.x, 0., 0.));

    // TODO: debranch
    if (abs(n.z) > abs(n.x)) {
        bounce += neighbor_light(hp, n, cell_coord + vec3(-rs.x, 0., rs.z));
    } 
    else
    {
        bounce += neighbor_light(hp, n, cell_coord + vec3(rs.x, 0., -rs.z));
    }
    
    return bounce;
}

void shade_tower_side(vec3 rro, 
                      vec3 rrd, 
                      mat3 rrt, 
                      vec3 lcol,
                      float hd,
                      vec3 cell_coords,
                      vec4 pr, 
                      inout vec4 scene_col)
{

    if (pr.x > .05)
    {
    vec3 hp = rro + rrd * pr.y;
    hp.y = -abs(hp.y);

    vec3 lpos = tower_lightpos(cell_coords.xz, lcol);
    float fo = max(0., 1. - .08 * length(lpos - hp)); fo *= fo;
    vec3 n = vec3(pr.z, 0., pr.w); 
    vec3 l = normalize(hp - lpos);
    vec3 r = refract(rrd, n, .5);
    vec3 fr = fresnel(rrd, n, .3);

    // glow
    float glow = 15. * (1. - fr.r) * smoothstep(.1, .0, abs(hp.y - lpos.y)) + .08 * fo;        

    // ambient
    float amb = .05 + .1 * pow(smoothstep(.0, .1, min(abs(hp.x),abs(hp.z))), 4.);        

    // transmissive
    float trans = 20. * fo * (1. - fr.r) * pow(max(0., dot(l, -r)), 2.);

    // diffuse 
    float diff = 10.8 * fo * max(0., abs(dot(l, n)));

    // depth
    vec2 pn = -sign(rrd.xz);
    vec2 ddn = rrd.xz * pn;
    vec2 po = hd * -pn;
    vec2 t = -(pn * (hp.xz - po))/ddn;
    float depth = max(0., min(t.x, t.y));

    // depth glow
    float depth_glow = .5 * min(3., .5 * depth);

    // neighbors bounce 
    vec3 ghp = g_cellsize * vec3(cell_coords.x + .5, 0., cell_coords.z + .5) + rrt * hp;
    vec3 gn  = rrt * n;
    vec3 neighbors = 1.5 * neighbors_diffuse(ghp, gn, cell_coords);

    // darken with depth
    float darken = exp(-.2 * pr.y);
    float alpha = pr.x * mix(.2, 1., .2 * depth);

    // add it all up
    scene_col.rgb += (1. - scene_col.a) * alpha * darken * (neighbors + lcol * (depth_glow + trans + diff + glow + amb));

    // depth attenuated opacity
    scene_col.a += (1. - scene_col.a) * alpha;
    }

}

vec4 shade_cell(vec3 ray_origin,
                vec3 ray_direction,
                vec3 cell_coords)
{
    vec4 result = vec4(0.);
   
    if (tower_mask(cell_coords.xz) > .5) 
    {
        vec3 lcol = tower_lightcolor(cell_coords.xz);
        float ang = .5 * g_time + 180. * tower_rotoffset(cell_coords.xz);

        float ca = cos(ang); float sa = sin(ang);
        mat3 rt = mat3(ca, 0., sa, 0., 1., 0.,-sa, 0., ca);
        mat3 rrt = mat3(ca, 0., -sa, 0., 1., 0.,sa, 0., ca);    
    
        vec3 rro = rt * ray_origin;
        vec3 rrd = rt * ray_direction;
    
        float hd = .1;
        vec4 p1r = vec4(0.), p2r = vec4(0.);
        intersect_tower(rro, rrd, hd, p1r, p2r);
    
        shade_tower_side(rro, rrd, rrt, lcol, hd, cell_coords, p1r, result);
        shade_tower_side(rro, rrd, rrt, lcol, hd, cell_coords, p2r, result);
    }
    return result;
}

// **************************************************************************
// MARCHING

// simplified dda marching along the x and z direction.  Ignore crossing the 
// y plane

// References for DDA marching:
// original tutorial:  http://lodev.org/cgtutor/raycasting.html
// initial shadertoy reference by fb39ca4: https://www.shadertoy.com/view/4dX3zl
// optimization by iq: https://www.shadertoy.com/view/4dfGzs
float dda_march( vec3 ro, vec3 rd,
                 float maxdist,
                 out vec4 scene_rgba )
{
 
    vec3 cell_coord = floor(ro/g_cellsize); cell_coord.y = 0.;
    vec3 rs = sign(rd);
    
    vec2 deltaDist = g_cellsize/rd.xz;
    vec2 sideDist = ((cell_coord.xz - ro.xz)/g_cellsize + 0.5 + rs.xz*0.5) * deltaDist;    

    float res = 0.0;
    vec3 mm = vec3(0.0);
    
    scene_rgba = vec4(0.);
    
    float t = 0.;
    
    vec3 pos = ro;
    vec3 cell_pos = mod(ro, g_cellsize) - .5 * g_cellsize;
    
    for( int i=0; i<16; i++ ) 
    {
        if (scene_rgba.a > .95 || t >= maxdist) { continue; }

        // DDA march along the xz boundaries, ignoring the y plane boundaries
        mm.xz = step(sideDist.xy, sideDist.yx);

        vec3 normal = vec3(0.); 
        normal.xz = mm.xz * rs.xz;
        cell_coord += mm * rs * vec3(1., 0., 1.);
        
        vec3 ddn = rd * -rs;
        vec3 po = .5 * g_cellsize * rs;
        vec3 plane_t = (rs * (cell_pos - po))/ddn;
        float cell_extent = min(plane_t.x, plane_t.z);        
        pos += cell_extent * rd;
        
        cell_pos = pos - g_cellsize * cell_coord - .5 * g_cellsize;

        vec4 cell_res = shade_cell(cell_pos, rd, cell_coord);

        t = length(pos - ro);

        // composite
        scene_rgba.rgb += cell_res.rgb * cell_res.a * exp(-.05 * t + 1.);
        scene_rgba.a += (1. - scene_rgba.a) * cell_res.a;

        sideDist += mm.xz * rs.xz * deltaDist;  
    }    
    
    return t;
}


// **************************************************************************
// MAIN

void main()
{   
    // ----------------------------------
    // SETUP GLOBALS

    setup_globals();

    // ----------------------------------
    // SETUP CAMERA

    CameraInfo camera = setup_camera();

    // ----------------------------------
    // SHADING 

    vec4 scene_rgba = vec4(0.);

    dda_march(camera.camera_origin, camera.ray_look_direction, 100., scene_rgba);

    // ----------------------------------
    // POST
    
    // gamma correct
    scene_rgba.rgb = pow(scene_rgba.rgb, vec3(.5));
    
    // saturate
    scene_rgba.rgb = clamp(mix( vec3(dot(vec3(.2125, .7154, .0721), scene_rgba.rgb)), 
                                scene_rgba.rgb, 
                                1.5), 0.0, 1.0);
    
    // Debug color - great debugging tool.      
    if (g_debugcolor.a > 0.) 
    {
        gl_FragColor.rgb = g_debugcolor.rgb;
    } else {
        gl_FragColor.rgb = scene_rgba.rgb;
    }


    gl_FragColor.a = 1.;
}
