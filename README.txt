================================================================================
ShaderToyVR User Guide

ShaderToy VR converts toys that use a ray marching/ray casting approach which
calculate a camera origin (aka ro) and a camera ray direction (aka rd) per
pixel into an Oculus Rift DK2 compatible experiences.  

Most toy's construct ro and rd using  a pin-hole camera model.  They do so by
calculating an orthonormal basis for the camera, and then use that to map pixel
coordinates to a ray direction.   The code often looks like this:

...

    vec2 q = gl_FragCoord.xy / iResolution.xy;
    vec2 p = -1.0 + 2.0*q;
    p.x *= iResolution.x/ iResolution.y;

    vec3 ro = vec3(0.0, 0.0, 0.0);
    vec3 ta = vec3(0.0, 1.0, 0.0);
    
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize(cross( vec3(0.0,1.0,0.0), ww ));
    vec3 vv = normalize(cross(ww,uu));
    
    float fl = 1.;
    vec3 rd = normalize( p.x*uu + p.y*vv + fl * ww );

 ... 

    
It's important to note that p.x is in the range [ -1/ASPECTRATIO, 1/ASPECTRATIO]
and p.y is in the  range [-1, 1]

To convert to ShaderToyVR, you'll need to begin with an empty text file in the
"glshaders" directory called "shadertoy.fs".  The top of the text file declares
parameters to the shadertoy, including what goes in the channels. Right now,
only 2D and CubeMap textures are supported.  You can also specify the default
screen percentage of the textures to be displayed in the eyes.  The default is
1., meaning 100%.  You'll need to tweak this value (often lower) to get a good
frame rate experience in the Rift.  Note you can adjust the Screen Percentage in
experience by using the <MINUS> and <EQUALS> keys (see below).

Here's an example header section:

"
iChannel0 = noise_rgb_256
ScreenPercentage = .7

"

Acceptable values for iChannel# are:

== 2D TEXTURES ==
noise_rgb_256
noise_r_256
noise_rgb_64
noise_r_64
noise_r_8
stone_tiles
old_birch
rusted_metal
deepsky_pattern
london_street
finished_wood
bark_and_lichen
colored_rocks
cloth_weave
animal_print
nyan_cat

== CUBEMAPS ==
uffizi_gallery_512
uffizi_gallery_64
st_peters_256
st_peters_64
grove_512
grove_64

Once you have your header arguments, make a line that begins with a "colon".
This tells ShaderToyVR to expect the next set of lines to be the fragment
shader.  You can then paste the ShaderToy code from shadertoy.com into the
remainder of the file.  You'll need to make edits like the following to respect
the Rift position:

...
    vec2 q = gl_FragCoord.xy / iResolution.xy;
    vec2 p = -1.0 + 2.0*q;
    p.x *= iResolution.x/ iResolution.y;

    vec3 ro = vec3(0.0, 0.0, 0.0);
    vec3 ta = vec3(0.0, 1.0, 0.0);

    vec3 ww = -normalize( ta - ro ); // <-- often need to negate the 
                                     // "look direction" since the Rift likes
                                     // looking down the negative z direction

    vec3 uu = normalize(cross( vec3(0.0,1.0,0.0), ww ));
    vec3 vv = normalize(cross(ww,uu));
    
    mat4 camXform =  mat4(vec4(uu, 0.), 
                          vec4(vv, 0.), 
                          vec4(ww, 0.),
                          vec4(0., 0., 0., 1.)) * iCameraTransform; 
                                            // ^-- push the camera transform of
                                            // the Rift onto the camXform
                                              
    ro += camXform[3].xyz; // <-- to capture stereo viewing, you'll need 
                           // account for the eye offset

    float fl = iFocalLength; // <-- often need to adjust the focal length until it feels
                             // right - use the sphere grid to calibrate

    vec3 rd = normalize( p.x*camXform[0].xyz + p.y*camXform[1].xyz + fl * camXform[2].xyz );
...

Take a look at some of the already converted shadertoy examples that come with
the project to see how it works.

================================================================================
Some gotchas

Each toy experience seems to need different tweaks to the scalar on the
z-direction vector.   I'm trying to find a universal fix, but for now, I find a
good value by bringing up the experience and then displaying the "sphere grid"
with the 'g' key.  I try to find a value so that the toy experience feels as
grounded as the sphere when moving my head.  Right now, you can tweak the
uniform variable iFocalLength that the can be changed with the <COMMA> and
<PERIOD> keys.  I'm soon going to have a proper print out so you can record the
final iFocalLength that works.

Toys that modify the camera model by warping p or vignetting the focal length
will still work in VR but can be nauseating.  Also, very bright experiences tend
to get blown out due to the low resolution and bright display.  I tend to bring
down the brightness and up the contrast to compensate.

================================================================================
Key Commands:

'r'         Reset the default position of the Rift.  Recommended to do this when
            looking at a toy            
            (TODO: only start a shader toy when a user is ready and hits the
             spacebar)

'g'         Display a debug representation of a transparent sphere grid around
            the user's default position (rendered in GL)

'c'         Display a debug representation of the positional camera

<MINUS>     Decrement the Screen Percentage of the rendered eye textures by 10%.  
<EQUALS>    Increment the Screen Percentage of the rendered eye textures by 10%.  
            Screen Percentage clamps at a minimum of 10% and a maximum of 200%

<COMMA>     Decrease iFocalLength by .1 (to be multiplied into the camera ray
            <calculation).
<PERIOD>    Increase iFocalLength by .1 (to be multiplied into the camera ray
            <calculation).
            iFocalLength clamps at a minimum of .1 and a maximum of 4.

't'         Display FPS in the console window that launches the app 
            (TODO: have a simple text display in the GL view)

'q'         Quit the Experience

Coming Soon:
'o'         Refresh the shader from the text file (allowing on the fly edits)

<SPACE>     Start and Pause the experience

================================================================================
Supported ShaderToy Variables

iResolution             Works!
iGlobalTime             Works!
iChannelTime[4]         Same values as iGlobalTime
iChannelResolution[4]   Works!
iMouse                  Just Zeros
iChannel0..3            Works! (see above)
iDate                   Works!
iSampleRate             Just Zero
