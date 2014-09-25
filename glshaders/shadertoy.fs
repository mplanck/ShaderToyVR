iChannel0 = noise_rgb_256
iChannel1 = colored_rocks
iChannel2 = bark_and_lichen
ScreenPercentage = .8

::::::

// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

float hash( in vec3 x )
{
	float n = dot( x, vec3(1.0,113.0,257.0) );
    return fract(sin(n)*43758.5453);
}

float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);
	f = f*f*(3.0-2.0*f);
	
	vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;
	vec2 rg = texture2D( iChannel0, (uv+ 0.5)/256.0, -100.0 ).yx;
	return mix( rg.x, rg.y, f.z );
}

float noise( in vec2 x )
{
    vec2 p = floor(x);
    vec2 f = fract(x);
	vec2 uv = p.xy + f.xy*f.xy*(3.0-2.0*f.xy);
	return texture2D( iChannel0, (uv+118.4)/256.0, -100.0 ).x;
}


vec4 texcube( sampler2D sam, in vec3 p, in vec3 n )
{
	vec4 x = texture2D( sam, p.yz );
	vec4 y = texture2D( sam, p.zx );
	vec4 z = texture2D( sam, p.xy );
	return x*abs(n.x) + y*abs(n.y) + z*abs(n.z);
}

//=====================================================================

const mat3 m = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 );


float mapTerrain( vec3 p )
{
	
	p *= 0.1; 
	p.xz *= 0.6;

	
	float time = 0.5 + 0.15*iGlobalTime;
	float ft = fract( time );
	float it = floor( time );
	ft = smoothstep( 0.7, 1.0, ft );
	time = it + ft;
	float spe = 1.4;
	
	float f;
    f  = 0.5000*noise( p*1.00 + vec3(0.0,1.0,0.0)*spe*time );
    f += 0.2500*noise( p*2.02 + vec3(0.0,2.0,0.0)*spe*time );
    f += 0.1250*noise( p*4.01 );
	return 25.0*f-10.0;
}


vec3 gro = vec3(0.0);

float map(in vec3 c) 
{
	vec3 p = c + 0.5;
    
	
	float f = mapTerrain( p ) + 0.25*p.y;

    f = mix( f, 1.0, step( length(gro-p), 5.0 ) );

	return step( f, 0.5 );
}

vec3 lig = normalize( vec3(-0.4,0.3,0.7) );


float castRay( in vec3 ro, in vec3 rd, out vec3 oVos, out vec3 oDir )
{
	vec3 pos = floor(ro);
	vec3 ri = 1.0/rd;
	vec3 rs = sign(rd);
	vec3 dis = (pos-ro + 0.5 + rs*0.5) * ri;
	
	float res = -1.0;
	vec3 mm = vec3(0.0);
	for( int i=0; i<128; i++ ) 
	{
		if( map(pos)>0.5 ) { res=1.0; break; }
		mm = step(dis.xyz, dis.yxy) * step(dis.xyz, dis.zzx);
		dis += mm * rs * ri;
        pos += mm * rs;
	}

	vec3 nor = -mm*rs;
	vec3 vos = pos;
	
    // intersect the cube	
	vec3 mini = (pos-ro + 0.5 - 0.5*vec3(rs))*ri;
	float t = max ( mini.x, max ( mini.y, mini.z ) );
	
	oDir = mm;
	oVos = vos;

	return t*res;

}

float castVRay( in vec3 ro, in vec3 rd, in float maxDist )
{

	vec3 pos = floor(ro);
	vec3 ri = 1.0/rd;
	vec3 rs = sign(rd);
	vec3 dis = (pos-ro + 0.5 + rs*0.5) * ri;
	
	float res = 1.0;
	for( int i=0; i<50; i++ ) 
	{
		if( map(pos)>0.5 ) {res=0.0; break; }
		vec3 mm = step(dis.xyz, dis.yxy) * step(dis.xyz, dis.zzx);
		dis += mm * rs * ri;
        pos += mm * rs;
	}
	
	return res;
}



vec3 path( float t )
{
    vec2 p  = 100.0*sin( 0.02*t*vec2(1.0,1.2) + vec2(0.1,0.9) );
	     p +=  50.0*sin( 0.04*t*vec2(1.3,1.0) + vec2(1.0,4.5) );
	
	return vec3( p.x, 18.0 + 4.0*sin(0.05*t), p.y );
}

void main( void )
{
    // inputs	
	vec2 q = gl_FragCoord.xy / iResolution.xy;
    vec2 p = -1.0 + 2.0*q;
    p.x *= iResolution.x/ iResolution.y;	
	
	float time = 2.0*iGlobalTime;
    // camera
    vec3 ro = 20.5*normalize(vec3(cos(time), 0.5, sin(time)));
	vec3 ta = vec3(0.0, 0.0, 0.0);
	float cr = 0.2*cos(0.1*iGlobalTime);
	
	ro = path( time );
	ta = path( time+5.0 ) - vec3(0.0,0.0,0.0);
	gro = ro;
	
	// build ray
    //vec3 ww = normalize( ta - ro);
    //vec3 uu = normalize(cross( vec3(sin(cr),cos(cr),0.0), ww ));
    //vec3 vv = normalize(cross(ww,uu));
	//float r2 = p.x*p.x*0.32 + p.y*p.y;
    //p *= (7.0-sqrt(37.5-11.5*r2))/(r2+1.0);
    //vec3 rd = normalize( p.x*uu + p.y*vv + 2.5*ww );

    vec3 ww = -normalize( ta - ro);
    vec3 uu = normalize(cross( vec3(0.0, 1.0, 0.0), ww ));
    vec3 vv = normalize(cross(ww,uu));
	    
    mat4 camXform =  mat4(vec4(uu, 0.), 
                          vec4(vv, 0.), 
                          vec4(ww, 0.),
                          vec4(0., 0., 0., 1.)) * iCameraTransform; 
    
    ro += 10. * camXform[3].xyz;
    
    vec3 rd = normalize( p.x*camXform[0].xyz + p.y*camXform[1].xyz + 1.4 * iFocalLength * camXform[2].xyz );
    

	vec3 col = vec3(0.0);
	
    // raymarch	
	vec3 vos, dir;
	float t = castRay( ro, rd, vos, dir );
	if( t>0.0 )
	{
        vec3 nor = -dir*sign(rd);
        vec3 pos = ro + rd*t;
        vec3 uvw = pos - vos;
		
		vec3 v1  = vos + nor + dir.yzx;
	    vec3 v2  = vos + nor - dir.yzx;
	    vec3 v3  = vos + nor + dir.zxy;
	    vec3 v4  = vos + nor - dir.zxy;
		vec3 v5  = vos + nor + dir.yzx + dir.zxy;
        vec3 v6  = vos + nor - dir.yzx + dir.zxy;
	    vec3 v7  = vos + nor - dir.yzx - dir.zxy;
	    vec3 v8  = vos + nor + dir.yzx - dir.zxy;
	    vec3 v9  = vos + dir.yzx;
	    vec3 v10 = vos - dir.yzx;
	    vec3 v11 = vos + dir.zxy;
	    vec3 v12 = vos - dir.zxy;
 	    vec3 v13 = vos + dir.yzx + dir.zxy; 
	    vec3 v14 = vos - dir.yzx + dir.zxy ;
	    vec3 v15 = vos - dir.yzx - dir.zxy;
	    vec3 v16 = vos + dir.yzx - dir.zxy;

		vec4 ed = vec4( map(v1),  map(v2),  map(v3),  map(v4)  );
	    vec4 co = vec4( map(v5),  map(v6),  map(v7),  map(v8)  );
	    vec4 ep = vec4( map(v9),  map(v10), map(v11), map(v12) );
	    vec4 cp = vec4( map(v13), map(v14), map(v15), map(v16) );
		
		vec2 uv = vec2( dot(dir.yzx, uvw), dot(dir.zxy, uvw) );
			
        // wireframe
        vec4 ee = 1.0-ep*(1.0-ed);
        float www = 1.0;
        www *= 1.0 - smoothstep( 0.85, 0.99,     uv.x )*ee.x;
        www *= 1.0 - smoothstep( 0.85, 0.99, 1.0-uv.x )*ee.y;
        www *= 1.0 - smoothstep( 0.85, 0.99,     uv.y )*ee.z;
        www *= 1.0 - smoothstep( 0.85, 0.99, 1.0-uv.y )*ee.w;
        www *= 1.0 - smoothstep( 0.85, 0.99,      uv.y*      uv.x )*(1.0-cp.x*(1.0-co.x))*(1.0-ee.x)*(1.0-ee.z);
        www *= 1.0 - smoothstep( 0.85, 0.99,      uv.y* (1.0-uv.x))*(1.0-cp.y*(1.0-co.y))*(1.0-ee.y)*(1.0-ee.z);
        www *= 1.0 - smoothstep( 0.85, 0.99, (1.0-uv.y)*(1.0-uv.x))*(1.0-cp.z*(1.0-co.z))*(1.0-ee.y)*(1.0-ee.w);
        www *= 1.0 - smoothstep( 0.85, 0.99, (1.0-uv.y)*     uv.x )*(1.0-cp.w*(1.0-co.w))*(1.0-ee.x)*(1.0-ee.w);
		
        vec3 wir = smoothstep( 0.4, 0.5, abs(uvw-0.5) );
        float vvv = (1.0-wir.x*wir.y)*(1.0-wir.x*wir.z)*(1.0-wir.y*wir.z);

        col = 2.0*texture2D( iChannel1,0.01*pos.xz ).zyx; 
        col += 0.8*vec3(0.1,0.3,0.4);
        col *= 0.5 + 0.5*texcube( iChannel2, 0.5*pos, nor ).x;
        col *= 1.0 - 0.75*(1.0-vvv)*www;
		
        // lighting
        float dif = clamp( dot( nor, lig ), 0.0, 1.0 );
        float sha = 0.0; if( dif>0.01) sha=castVRay(pos+nor*0.01,lig,32.0);
        float bac = clamp( dot( nor, normalize(lig*vec3(-1.0,0.0,-1.0)) ), 0.0, 1.0 );
        float sky = 0.5 + 0.5*nor.y;
        float amb = clamp(0.75 + pos.y/25.0,0.0,1.0);
        float occ = 1.0;
	
        // ambient occlusion
        occ = 0.0; 
        // (for edges)
        occ += (    uv.x) * ed.x;
        occ += (1.0-uv.x) * ed.y;
        occ += (    uv.y) * ed.z;
        occ += (1.0-uv.y) * ed.w;
        // (for corners)
        occ += (      uv.y *     uv.x ) * co.x*(1.0-ed.x)*(1.0-ed.z);
        occ += (      uv.y *(1.0-uv.x)) * co.y*(1.0-ed.z)*(1.0-ed.y);
        occ += ( (1.0-uv.y)*(1.0-uv.x)) * co.z*(1.0-ed.y)*(1.0-ed.w);
        occ += ( (1.0-uv.y)*     uv.x ) * co.w*(1.0-ed.w)*(1.0-ed.x);
        occ = 1.0 - occ/8.0;
        occ = occ*occ;
        occ = occ*occ;
        occ *= amb;

        // lighting
        vec3 lin = vec3(0.0);
        lin += 6.0*dif*vec3(1.00,0.90,0.70)*sha*(0.5+0.5*occ);
        lin += 0.5*bac*vec3(0.15,0.10,0.10)*occ;
        lin += 2.0*sky*vec3(0.40,0.30,0.15)*occ;

        // line glow	
        float lineglow = 0.0;
        lineglow += smoothstep( 0.4, 1.0,     uv.x )*(1.0-ep.x*(1.0-ed.x));
        lineglow += smoothstep( 0.4, 1.0, 1.0-uv.x )*(1.0-ep.y*(1.0-ed.y));
        lineglow += smoothstep( 0.4, 1.0,     uv.y )*(1.0-ep.z*(1.0-ed.z));
        lineglow += smoothstep( 0.4, 1.0, 1.0-uv.y )*(1.0-ep.w*(1.0-ed.w));
        lineglow += smoothstep( 0.4, 1.0,      uv.y*      uv.x )*(1.0-cp.x*(1.0-co.x));
        lineglow += smoothstep( 0.4, 1.0,      uv.y* (1.0-uv.x))*(1.0-cp.y*(1.0-co.y));
        lineglow += smoothstep( 0.4, 1.0, (1.0-uv.y)*(1.0-uv.x))*(1.0-cp.z*(1.0-co.z));
        lineglow += smoothstep( 0.4, 1.0, (1.0-uv.y)*     uv.x )*(1.0-cp.w*(1.0-co.w));
		
        vec3 linCol = 2.0*vec3(5.0,0.6,0.0);
        linCol *= (0.5+0.5*occ)*(0.25+sha);
        lin += 3.0*lineglow*linCol;
		
        col = col*lin;
        col += 8.0*linCol*vec3(1.0,2.0,3.0)*(1.0-www);//*(0.5+1.0*sha);
        col += 0.1*lineglow*linCol;
        col *= min(0.1,exp( -0.07*t ));
	
        // blend to black & white		
        vec3 col2 = vec3(1.3)*(0.5+0.5*nor.y)*occ*www*(0.9+0.1*vvv)*exp( -0.04*t );;
        float mi = sin(-1.57+0.5*iGlobalTime);
        mi = smoothstep( 0.90, 0.95, mi );
        col = mix( col, col2, mi );
	}

	// gamma	
	col = pow( col, vec3(0.45) );

    col = clamp( col, 0.0, 1.0 );

	// vignetting	
	col *= 0.5 + 0.5*pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.1 );
	
	gl_FragColor = vec4( col, 1.0 );
}