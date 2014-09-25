ScreenPercentage = .8

::::::::::::::


// **************************************************************************
// CONSTANTS

#define PI 3.14159
#define TWO_PI 6.28318
#define PI_OVER_TWO 1.570796

#define REALLY_SMALL_NUMBER 0.0001
#define REALLY_BIG_NUMBER 1000000.

#define SPHERE_SURFACE_ID 1.
#define PLANE_SURFACE_ID 2.

// **************************************************************************
// INLINE MACROS

#define MATCHES_SURFACE_ID(id1, id2) (id1 > (id2 - .5)) && (id1 < (id2 + .5))

// **************************************************************************
// DEFINES

// **************************************************************************
// GLOBALS

float g_time        = 0.;
vec4  g_debugcolor  = vec4(0.);

// **************************************************************************
// MATH UTILITIES

// **************************************************************************
// INTERSECTION MATH

// intersection for a sphere with a ray. If the ray origin is inside the
// sphere - no intersection takes place.  So there is gauranteed to be a tmin
// and tmax return value.
// Returns a vec3 where:
//  result.x = 1. or 0. to indicate if a hit took place
//  result.y = tmin such that ray_origin + tmin * ray_direction = 1st intersection point
//  result.z = tmax such that ray_origin + tmax * ray_direction = 2nd intersection point

vec3 intersect_sphere(vec3 ray_origin,
                      vec3 ray_direction,
                      vec3 sphere_center,
                      float sphere_radius)
{

    // Calculate the ray origin in object space of the sphere
    vec3 ray_origin_local_space = ray_origin - sphere_center;

    // If the ray origin is inside the sphere, this is not a hit.
    if (dot(ray_origin_local_space, ray_origin_local_space) < sphere_radius * sphere_radius) {
        return vec3(0.);
    }
    
    // coefficients of quadratic equation.
    float a = dot(ray_direction, ray_direction);
    float b = 2.0*dot(ray_origin_local_space, ray_direction);
    float c = dot(ray_origin_local_space, ray_origin_local_space) - sphere_radius * sphere_radius;

    float discriminant = b*b - 4.0*a*c;

    float tmin = 0.0;
    float tmax = 0.0;
    float intersected = 0.0;

    // Check if discriminant is positive.  If so, we know that there
    // is an intersection in the ray direction from the origin.
    if (discriminant > 0.) {

        float discriminant_root = sqrt(discriminant);
        tmin = (-b - discriminant_root)/(2.0 * a);
        tmax = (-b + discriminant_root)/(2.0 * a); 
        if (tmin > 0.) {
            intersected = 1.;
        }

    }

    return vec3(intersected, tmin, tmax);
}

// Reference: http://geomalgorithms.com/a05-_intersect-1.html. Does an
// intersection test against a plane that is assumed to be double sided and
// passes through the plane_origin and has the specified normal. *Overkill* 
// for intersecting with the x-z plane.

// Returns a vec2 where:
//   result.x = 1. or 0. if there is a hit
//   result.y = t such that ray_origin + t*ray_direction = intersection point
vec2 intersect_plane(vec3 ray_origin,
                     vec3 ray_direction,
                     vec3 plane_normal,
                     vec3 plane_origin)
{
    float ray_direction_dot_normal = dot(ray_direction, plane_normal);

    float denominator = ray_direction_dot_normal;    
    float intersected = 0.;

    float t = REALLY_BIG_NUMBER;
    // If the denominator is not a really small number (positive or negative)
    // then an intersection took place.  If it is really small, then the ray
    // is close to parallel to the given plane.
    if (abs(denominator) > REALLY_SMALL_NUMBER) {
        t = -dot(plane_normal, (ray_origin - plane_origin)) / denominator;    
        if (t > REALLY_SMALL_NUMBER) {
            intersected = 1.;
        }
    }
    return vec2(intersected, t);

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

struct SurfaceInfo
{
    float surface_id;
    vec3 surface_point;
    vec3 surface_normal;
};
#define INIT_SURFACE_INFO(view_dir) SurfaceInfo(-1. /* surface_id */, vec3(0.) /* surface_point */, vec3(0.) /* surface_normal */)

// **************************************************************************
// SETUP WORLD

void setup_globals()
{
    // Way to globally control playback rate.
    g_time = 1. * iGlobalTime;
}

CameraInfo setup_camera()
{
    // camera position and where it's looking.  These 2 properties define
    // where our camera is located and oriented in the scene.
    vec3 camera_origin = vec3(0.0, .8, 2.0);    
    vec3 camera_points_at = vec3(0., .8, 0.);

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
    vec3 iu = vec3(0., 1., 0.);

    // Find the orthonormal basis of our camera.  iz is the normalized eye
    // direction along the z axis (where we're looking).  ix is the direction to
    // the right of  the camera.  iy is the upward facing direction of our
    // camera.  Note that iy is not necessrily (0, 1., 0.) since the camera 
    // could be tilted based on our look_at variable.
    vec3 iz = -normalize( camera_points_at - camera_origin );
    vec3 ix = normalize( cross(iz, iu) );
    vec3 iy = cross(ix, iz);
    
    mat4 cam_xf =  mat4(vec4(ix, 0.), 
                        vec4(iy, 0.), 
                        vec4(iz, 0.),
                        vec4(0., 0., 0., 1.));

    cam_xf = cam_xf * iCameraTransform;
    
    camera_origin += 1. * cam_xf[3].xyz;

    // project the camera ray through the current pixel
    vec3 ray_look_direction = normalize( image_plane_uv.x * cam_xf[0].xyz + image_plane_uv.y * cam_xf[1].xyz + cam_xf[2].xyz );

    return CameraInfo(camera_origin, ray_look_direction, image_plane_uv);

}

// **************************************************************************
// INTERSECT WORLD

SurfaceInfo intersect_scene(vec3 ray_origin,
                            vec3 ray_direction)
{
    // Loops over all objects in the world and finds the object that is the
    // closest to the given ray origin when looking in the given ray direction.
    // The resulting SurfaceInfo represents all of the surface data for that 
    // closest object so we can shade the this fragment to represent it.

    SurfaceInfo surface = INIT_SURFACE_INFO(ray_direction);
    
    // Initialize the closest intersection as REALLY far away since we will find
    // the closest intersecting surface by comparing against this value.  As we
    // find closer surfaces of intersection, we will  update this value.
    float closest_intersection = REALLY_BIG_NUMBER;

    // Intersect a sphere.

    // Have the sphere bob up and down through the plane 
    vec3 sphere_position = vec3(0., .75 + .3 * sin(g_time), 0.);
    vec3 sphere_result = intersect_sphere(ray_origin, 
                                          ray_direction, 
                                          sphere_position,
                                          .5);

    float sphere_intersected = sphere_result.x;
    float sphere_intersection_dist = sphere_result.y;

    if (sphere_intersected > .5 && sphere_intersection_dist < closest_intersection) {                               

        // If there was a sphere intersection, update the SurfaceInfo ...

        // To indicate a sphere needs to be shaded
        surface.surface_id = SPHERE_SURFACE_ID;
        
        vec3 intersection_point = ray_origin + ray_direction * sphere_intersection_dist;
        vec3 intersection_point_in_sphere_space = intersection_point - sphere_position;

        // The normal is just the direction to the center of the sphere in
        // sphere space

        surface.surface_normal = normalize(intersection_point_in_sphere_space);
        surface.surface_point = intersection_point;
        
        // ... and then update the closest intersection.
        closest_intersection = sphere_intersection_dist;

        // Comment this out to see the power of the g_debugcolor trick.  You 
        // can use this to debug any signal in your shader as long as you set
        // g_debugcolor.a > 0.
        
        // g_debugcolor = vec4(pow(.3 * sphere_intersection_dist, 3.));

    }

    // Now intersect the x-z plane of our scene.  Using a generic
    // intersect_plane is *overkill* for this particular case.
    vec2 plane_result = intersect_plane(ray_origin, 
                                        ray_direction, 
                                        vec3(0., 1., 0.), 
                                        vec3(0., 0., 0.));

    float plane_intersected = plane_result.x;
    float plane_intersection_dist = plane_result.y;
    
    if (plane_intersected > .5 && plane_intersection_dist < closest_intersection)
    {
        // If there was a sphere intersection, update the SurfaceInfo ...

        surface.surface_point = ray_origin + ray_direction * plane_intersection_dist;

        // Normal of the plane is constant over the surface
        surface.surface_normal = vec3(0., 1., 0.);  

        // To indicate a sphere needs to be shaded
        surface.surface_id = PLANE_SURFACE_ID;

        // ... and then update the closest intersection. 
        closest_intersection = plane_intersection_dist;
    }

    return surface;
}

// **************************************************************************
// SHADE WORLD

vec3 shade_world(SurfaceInfo surface)
{
    // Do the work of taking the intersected surface data and then determine the 
    // material properties of the currently shaded fragment.  Once material 
    // properties are determined, we can run the surface and material data 
    // through our lighting model.

    // Initialize scene color
    vec3 scene_color = vec3(0.);

    float pulse = .5 * sin(g_time + .2 * PI) + .5;
    if (MATCHES_SURFACE_ID(surface.surface_id, SPHERE_SURFACE_ID))
    {
        // This is where we can define material properties for this surface
        // based on SurfaceInfo.  Right now, we don't have any valuable data
        // in SurfaceInfo other than ID.  We'll expand this on this in the 
        // next step.
        float ndd = dot(vec3(0., -1., 0.), surface.surface_normal);
        scene_color = .08 * mix(vec3(.5, 0.2, 0.2), vec3(1., .6, .5), ndd);
        float remapndd = smoothstep(-pulse, .5, ndd);
        
        vec3 illum = 1.1 * vec3(1., .6, .5) * pow(remapndd, 1.2) * pulse;
        scene_color += illum;
    } 
    
    else if (MATCHES_SURFACE_ID(surface.surface_id, PLANE_SURFACE_ID))
    {
        float l = length(surface.surface_point);
        float r = smoothstep(10. * pulse, 0., l);
        r *= .4 + .6 * smoothstep(0., 5. * pulse, l);
        scene_color = r * 1.8 * vec3(1., .6, .5);        
    }


    return scene_color;
}

// **************************************************************************
// MAIN

void main()
{   
    // Our final scene color
    vec3 scene_color = vec3(0.1);
        
    // ----------------------------------
    // SETUP GLOBALS

    setup_globals();

    // ----------------------------------
    // SETUP CAMERA

    CameraInfo camera = setup_camera();

    // ----------------------------------
    // SCENE MARCHING

    SurfaceInfo surface = intersect_scene( camera.camera_origin, 
                                           camera.ray_look_direction );
    
    // ----------------------------------
    // SHADING 

    scene_color += shade_world( surface ) ;

    // Debug color - great debugging tool.  
    if (g_debugcolor.a > 0.) 
    {
        gl_FragColor.rgb = g_debugcolor.rgb;
    } else {
        gl_FragColor.rgb = scene_color;
    }

    gl_FragColor.a = 1.;
}
