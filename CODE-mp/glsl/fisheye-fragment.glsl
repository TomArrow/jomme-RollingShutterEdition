#version 400 compatibility

uniform sampler2D text_in;

in vec3 debugColor;
in vec4 vertColor;
in vec3 texUVTransform[2];

in mat4x4 projectionMatrix;

in vec3 normal;

uniform int fishEyeModeUniform; //1= fisheye, 2=equirectangular
uniform float texAverageBrightnessUniform;
uniform float parallaxMapDepthUniform;
uniform int parallaxMapLayersUniform;
uniform float parallaxMapGammaUniform;
uniform float serverTimeUniform;
uniform int isLightmapUniform; 
uniform int isWorldBrushUniform; 
uniform int noiseFuckeryUniform; 

varying vec4 eyeSpaceCoordsGeom;
varying vec4 pureVertexCoordsGeom;


float snoise(vec4 v);

vec2 parallaxMap(){
		vec2 uvCoords;
		//uvCoords.s = dot(eyeSpaceCoordsGeom.xyz,texUVTransform[0]);
		//uvCoords.t = dot(eyeSpaceCoordsGeom.xyz,texUVTransform[1]);
		vec4 color = texture2D(text_in, gl_TexCoord[0].st);
		//vec4 color = texture2D(text_in, uvCoords);
		float offset = 1.0f - max(min((color.x + color.y + color.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f);

		vec3 offset3d =  normalize(eyeSpaceCoordsGeom.xyz)*parallaxMapDepthUniform * offset;
		offset3d -= normal * dot(normal,offset3d); // project onto surface aka get rid of any 3d component that aligns with the normal of the surface

		vec3 transposedCoords = eyeSpaceCoordsGeom.xyz + offset3d;
		
		uvCoords.s = mod(dot(transposedCoords,texUVTransform[0]),1);
		uvCoords.t = mod(dot(transposedCoords,texUVTransform[1]),1);
		return uvCoords;
}
vec2 parallaxMapSteep(){
		int layers = parallaxMapLayersUniform;
		vec2 uvCoords;

		float layerDepth = parallaxMapDepthUniform / float(layers);
		vec3 currentPlace = eyeSpaceCoordsGeom.xyz;
		float gamma = 1.0f/parallaxMapGammaUniform;

		//vec4 color = texture2D(text_in, gl_TexCoord[0].st);

		vec3 viewVecNormalized = normalize(eyeSpaceCoordsGeom.xyz);
		vec3 depthComponent = normal * dot(normal,viewVecNormalized); // Get the depth component that a unity view vector gives us 
		vec3 viewVecFlat = viewVecNormalized - depthComponent;
		float viewVecMultiplier = layerDepth/length(depthComponent); // Calculate how much we have to multiple the unity view vector with to go one layer deeper.
		vec3 oneLayerProgressVec = viewVecFlat*viewVecMultiplier;

		uvCoords.s = mod(dot(currentPlace,texUVTransform[0]),1);
		uvCoords.t = mod(dot(currentPlace,texUVTransform[1]),1);

		for(int i=0; i< layers;i++){
			
			vec4 color = texture2D(text_in, uvCoords);
			float texDepth = parallaxMapDepthUniform*(pow(max(min((color.x + color.y + color.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f),gamma)-1.0f);
			if(texDepth >= -(layerDepth * float(i))){
				break;
			} else {
				currentPlace += oneLayerProgressVec;
				uvCoords.s = mod(dot(currentPlace,texUVTransform[0]),1);
				uvCoords.t = mod(dot(currentPlace,texUVTransform[1]),1);
			}
		}

		//float offset = 1.0f - max(min((color.x + color.y + color.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f);


		//float offset = 1.0f - max(min((color.x + color.y + color.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f);

		//vec3 offset3d =  normalize(eyeSpaceCoordsGeom.xyz)*parallaxMapDepthUniform * offset;
		//float depthHere = dot(normal,offset3d);
		//vec3 depthComponent = normal * depthHere;
		//offset3d -= depthComponent; // project onto surface aka get rid of any 3d component that aligns with the normal of the surface

		//vec3 transposedCoords = eyeSpaceCoordsGeom.xyz + offset3d;
		
		return uvCoords;
}

vec3 perlinNoiseVariation1(){ // Looks a bit like marble?
	vec3 res;
	res.xyz = vec3( snoise(pureVertexCoordsGeom/10.0f));
	res.xyz += vec3( snoise(pureVertexCoordsGeom/20.0f));
	res.xyz += vec3( snoise(pureVertexCoordsGeom/40.0f));
	res.xyz += vec3( snoise(pureVertexCoordsGeom/80.0f));
	res.xyz += vec3( snoise(pureVertexCoordsGeom/160.0f));
	res.xyz += vec3( snoise(pureVertexCoordsGeom/320.0f));
	res.xyz += vec3( snoise(pureVertexCoordsGeom/640.0f));
	res.xyz += vec3( snoise(pureVertexCoordsGeom/1280.0f));
	//gl_FragColor.xyz += 8.0f;
	//gl_FragColor.xyz /= 16.0f;
	res.xyz = abs(res.xyz);
	res.xyz /= 4.0f;
	return res;
}
vec3 perlinNoiseVariation2(){ // Looks a bit like marble?
	vec3 res;
	res.xyz = vec3( snoise(pureVertexCoordsGeom/10.0f))/128.0f;
	res.xyz += vec3( snoise(pureVertexCoordsGeom/20.0f))/64.0f;
	res.xyz += vec3( snoise(pureVertexCoordsGeom/40.0f))/32.0f;
	res.xyz += vec3( snoise(pureVertexCoordsGeom/80.0f))/16.0f;
	res.xyz += vec3( snoise(pureVertexCoordsGeom/160.0f))/8.0f;
	res.xyz += vec3( snoise(pureVertexCoordsGeom/320.0f))/4.0f;
	res.xyz += vec3( snoise(pureVertexCoordsGeom/640.0f))/2.0f;
	res.xyz += vec3( snoise(pureVertexCoordsGeom/1280.0f));
	//gl_FragColor.xyz += 8.0f;
	//gl_FragColor.xyz /= 16.0f;
	//res.xyz = abs(res.xyz);
	res.xyz += 2.0f;
	res.xyz /= 4.0f;
	return res;
}

float perlinNoiseHelper(vec4 coords){ // Looks a bit like marble?
    float val;
    coords.w = serverTimeUniform*100.0;
	//val = ( snoise(pureVertexCoordsGeom/10.0))/128.0;
	//val += ( snoise(pureVertexCoordsGeom/20.0))/64.0;
	//val += ( snoise(pureVertexCoordsGeom/40.0))/32.0;
	//val += ( snoise(pureVertexCoordsGeom/80.0))/16.0;
	val += ( snoise(coords/160.0))/8.0;
	val += ( snoise(coords/320.0))/4.0;
	val += ( snoise(coords/640.0))/2.0;
	val += ( snoise(coords/1280.0));
	//gl_FragColor.xyz += 8.0f;
	//gl_FragColor.xyz /= 16.0f;
	//res.xyz = abs(res.xyz);
	//res.xyz += 1.0;
	//res.xyz *= 0.5;
    //val = abs(val);
    val += 1.0;
    val *= 0.5;
    //val = pow(val ,0.3);
    //val = 1.0 - val;
	//res.xyz = vec3(1.0)-res.xyz;
	return val;
}
float perlinNoiseHelper2(vec4 coords){ // Looks a bit like marble?
    float val;
	float timeVal =  serverTimeUniform*100.0;
    coords.w = timeVal/128.0;
	val = ( snoise(coords/10.0))/128.0;
    coords.w = timeVal/64.0;
	val += ( snoise(coords/20.0))/64.0;
    coords.w = timeVal/32.0;
	val += ( snoise(coords/40.0))/32.0;
    coords.w = timeVal/16.0;
	val += ( snoise(coords/80.0))/16.0;
    coords.w = timeVal/8.0;
	val += ( snoise(coords/160.0))/8.0;
    coords.w = timeVal/4.0;
	val += ( snoise(coords/320.0))/4.0;
    coords.w = timeVal/2.0;
	val += ( snoise(coords/640.0))/2.0;
    coords.w = timeVal;
	val += ( snoise(coords/1280.0));
	//gl_FragColor.xyz += 8.0f;
	//gl_FragColor.xyz /= 16.0f;
	//res.xyz = abs(res.xyz);
	//res.xyz += 1.0;
	//res.xyz *= 0.5;
    val = abs(val);
    //val += 1.0;
    //val *= 0.5;
    //val = pow(val ,0.3);
    //val = 1.0 - val;
	//res.xyz = vec3(1.0)-res.xyz;
	return val;
}


vec3 perlinNoiseVariation3(){ // Looks a bit like marble?
	vec3 res;
    vec3 distort = vec3(perlinNoiseHelper(pureVertexCoordsGeom),perlinNoiseHelper(pureVertexCoordsGeom+vec4(40.3,3.4,100.5,1.0)),perlinNoiseHelper(pureVertexCoordsGeom+vec4(10.1,1.4,101.5,1.0)));
    vec3 distort2 = vec3(perlinNoiseHelper(pureVertexCoordsGeom+30.0*vec4(distort,1.0)),perlinNoiseHelper(pureVertexCoordsGeom+30.0*vec4(distort,1.0)+vec4(15.3,13.4,110.3,1.0)),perlinNoiseHelper(pureVertexCoordsGeom+30.0*vec4(distort,1.0)+vec4(13.1,11.4,151.5,1.0)));
    float finalVal = perlinNoiseHelper(pureVertexCoordsGeom+100.0*vec4(distort2,1.0));
    res =vec3(finalVal);
    if(finalVal > 0.9){
        res = vec3(1.0,0.0,0.0);
    } else if(finalVal > 0.8){
        res = vec3(0.0,0.0,1.0);
    } else if(finalVal > 0.7){
        res = vec3(1.0,1.0,0.0);
    } else if(finalVal > 0.6){
        res = vec3(0.0,1.0,1.0);
    } else if(finalVal > 0.5){
        res = vec3(0.5,1.0,0.0);
    } else if(finalVal > 0.4){
        res = vec3(0.0,1.0,0.5);
    } else if(finalVal > 0.3){
        res = vec3(1.0,0.0,0.5);
    } else if(finalVal > 0.2){
        res = vec3(1.0,0.5,0.7);
    }else if(finalVal > 0.1){
        res = vec3(0.0,0.5,0.3);
    }else{
        
        res = vec3(0.0,1.0,0.0);
    }
	return res;
}
vec3 perlinNoiseVariation4(){ // Looks a bit like marble?
	vec3 res;
	vec4 startCooords = pureVertexCoordsGeom*0.25;
    vec3 distort = vec3(perlinNoiseHelper2(startCooords),perlinNoiseHelper2(startCooords+vec4(40.3,3.4,100.5,1.0)),perlinNoiseHelper2(startCooords+vec4(10.1,1.4,101.5,1.0)));
    vec3 distort2 = vec3(perlinNoiseHelper2(startCooords+30.0*vec4(distort,1.0)),perlinNoiseHelper2(startCooords+30.0*vec4(distort,1.0)+vec4(15.3,13.4,110.3,1.0)),perlinNoiseHelper2(startCooords+30.0*vec4(distort,1.0)+vec4(13.1,11.4,151.5,1.0)));
    float finalVal = perlinNoiseHelper2(startCooords+100.0*vec4(distort2,1.0));
    float finalVal2 = perlinNoiseHelper2(startCooords-33.0*vec4(distort2,1.0));
    float finalVal3 = perlinNoiseHelper2(startCooords-72.456*vec4(distort2,1.0));
    res =vec3(pow(finalVal,2.4),pow(finalVal2,2.4),pow(finalVal3,2.4));
	return res;
}
vec3 perlinNoiseVariation5(vec4 coords){ // Looks a bit like marble?
	vec3 res;
	float val,val2;
	float timeVal =  serverTimeUniform*2.5;
    coords.w = timeVal;
	val = ( snoise(coords/0.078125))/16384.0;
	val += ( snoise(coords/0.15625))/8192.0;
	val += ( snoise(coords/0.3125))/4096.0;
	val += ( snoise(coords/0.625))/2048.0;
	val += ( snoise(coords/1.25))/1024.0;
	val += ( snoise(coords/2.5))/512.0;
	val += ( snoise(coords/5.0))/256.0;
	val += ( snoise(coords/10.0))/128.0;
	val += ( snoise(coords/20.0))/64.0;
	val += ( snoise(coords/40.0))/32.0;
	val += ( snoise(coords/80.0))/16.0;
	val += ( snoise(coords/160.0))/8.0;
	val += ( snoise(coords/320.0))/4.0;
	val += ( snoise(coords/640.0))/2.0;
	val += ( snoise(coords/1280.0));
	//gl_FragColor.xyz += 8.0f;
	//gl_FragColor.xyz /= 16.0f;
	//res.xyz = abs(res.xyz);
	//res.xyz += 1.0;
	//res.xyz *= 0.5;
    val = abs(val);
    //return vec3(val < 0.002);
    //val += 1.0;
    val = 1.0 - val;
    val2=val;
	float dist = length(eyeSpaceCoordsGeom.xyz);
	dist = max(0.0,1000.0-dist);
    val = pow(val ,1000.0+dist*2.0);
    val2 = pow(val2 ,40.0);
	//res.xyz = vec3(1.0)-res.xyz;
    res.xyz = vec3(val*10.0)+vec3(val2*0.5)*vec3(val2*0.5,val2*0.7,1.0);
	res.x = max(0.0,pow(res.x,2.4));
	res.y = max(0.0,pow(res.y,2.4));
	res.z = max(0.0,pow(res.z,2.4));
	return res;
}

void main(void)
{
    //const float depth = 5.0f;

	int perlinFuckery = noiseFuckeryUniform;

    if(fishEyeModeUniform == 0){
	
		vec2 uvCoords;
		if(isLightmapUniform == 0 && perlinFuckery == 0){
			uvCoords = parallaxMapLayersUniform < 2 ? parallaxMap():parallaxMapSteep();
		} else {
			uvCoords = gl_TexCoord[0].st; // Don't parallax lightmaps
		}
		vec4 color = texture2D(text_in, uvCoords);

		gl_FragColor = color*vertColor; 
		gl_FragColor.xyz+=debugColor;
		vec4 startCooords = pureVertexCoordsGeom*0.25;
		if(isWorldBrushUniform > 0 && isLightmapUniform == 0 && perlinFuckery > 0){
			//gl_FragColor.xyz+=pureVertexCoordsGeom.xyz/1000.0f; 
			switch(perlinFuckery){
				case 1:
			gl_FragColor.xyz = perlinNoiseVariation1();
				break;
				case 2:
			gl_FragColor.xyz = perlinNoiseVariation2();
				break;
				case 3:
			gl_FragColor.xyz = perlinNoiseVariation3();
				break;
				case 4:
			gl_FragColor.xyz = perlinNoiseVariation4();
				break;
				case 5:
			gl_FragColor.xyz = perlinNoiseVariation4()*0.25+perlinNoiseVariation5(startCooords);
				break;
			}
		}
		if(isLightmapUniform > 0 && isWorldBrushUniform > 0 && perlinFuckery > 0){
			gl_FragColor.xyz = vec3(1.0,1.0,1.0);
		}
		//gl_FragColor.xyz+=eyeSpaceCoordsGeom.xyz/1000.0f; // cool effect lol
	} else {
		
		vec4 color = texture2D(text_in, gl_TexCoord[0].st);
		gl_FragColor = color*vertColor; 
		gl_FragColor.xyz+=debugColor;
	}
}






//
// Description : Array and textureless GLSL 2D/3D/4D simplex
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0; }

float mod289(float x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0; }

vec4 permute(vec4 x) {
     return mod289(((x*34.0)+1.0)*x);
}

float permute(float x) {
     return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

float taylorInvSqrt(float r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

vec4 grad4(float j, vec4 ip)
  {
  const vec4 ones = vec4(1.0, 1.0, 1.0, -1.0);
  vec4 p,s;

  p.xyz = floor( fract (vec3(j) * ip.xyz) * 7.0) * ip.z - 1.0;
  p.w = 1.5 - dot(abs(p.xyz), ones.xyz);
  s = vec4(lessThan(p, vec4(0.0)));
  p.xyz = p.xyz + (s.xyz*2.0 - 1.0) * s.www;

  return p;
  }

// (sqrt(5) - 1)/4 = F4, used once below
#define F4 0.309016994374947451

float snoise(vec4 v)
  {
  const vec4  C = vec4( 0.138196601125011,  // (5 - sqrt(5))/20  G4
                        0.276393202250021,  // 2 * G4
                        0.414589803375032,  // 3 * G4
                       -0.447213595499958); // -1 + 4 * G4

// First corner
  vec4 i  = floor(v + dot(v, vec4(F4)) );
  vec4 x0 = v -   i + dot(i, C.xxxx);

// Other corners

// Rank sorting originally contributed by Bill Licea-Kane, AMD (formerly ATI)
  vec4 i0;
  vec3 isX = step( x0.yzw, x0.xxx );
  vec3 isYZ = step( x0.zww, x0.yyz );
//  i0.x = dot( isX, vec3( 1.0 ) );
  i0.x = isX.x + isX.y + isX.z;
  i0.yzw = 1.0 - isX;
//  i0.y += dot( isYZ.xy, vec2( 1.0 ) );
  i0.y += isYZ.x + isYZ.y;
  i0.zw += 1.0 - isYZ.xy;
  i0.z += isYZ.z;
  i0.w += 1.0 - isYZ.z;

  // i0 now contains the unique values 0,1,2,3 in each channel
  vec4 i3 = clamp( i0, 0.0, 1.0 );
  vec4 i2 = clamp( i0-1.0, 0.0, 1.0 );
  vec4 i1 = clamp( i0-2.0, 0.0, 1.0 );

  //  x0 = x0 - 0.0 + 0.0 * C.xxxx
  //  x1 = x0 - i1  + 1.0 * C.xxxx
  //  x2 = x0 - i2  + 2.0 * C.xxxx
  //  x3 = x0 - i3  + 3.0 * C.xxxx
  //  x4 = x0 - 1.0 + 4.0 * C.xxxx
  vec4 x1 = x0 - i1 + C.xxxx;
  vec4 x2 = x0 - i2 + C.yyyy;
  vec4 x3 = x0 - i3 + C.zzzz;
  vec4 x4 = x0 + C.wwww;

// Permutations
  i = mod289(i);
  float j0 = permute( permute( permute( permute(i.w) + i.z) + i.y) + i.x);
  vec4 j1 = permute( permute( permute( permute (
             i.w + vec4(i1.w, i2.w, i3.w, 1.0 ))
           + i.z + vec4(i1.z, i2.z, i3.z, 1.0 ))
           + i.y + vec4(i1.y, i2.y, i3.y, 1.0 ))
           + i.x + vec4(i1.x, i2.x, i3.x, 1.0 ));

// Gradients: 7x7x6 points over a cube, mapped onto a 4-cross polytope
// 7*7*6 = 294, which is close to the ring size 17*17 = 289.
  vec4 ip = vec4(1.0/294.0, 1.0/49.0, 1.0/7.0, 0.0) ;

  vec4 p0 = grad4(j0,   ip);
  vec4 p1 = grad4(j1.x, ip);
  vec4 p2 = grad4(j1.y, ip);
  vec4 p3 = grad4(j1.z, ip);
  vec4 p4 = grad4(j1.w, ip);

// Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;
  p4 *= taylorInvSqrt(dot(p4,p4));

// Mix contributions from the five corners
  vec3 m0 = max(0.6 - vec3(dot(x0,x0), dot(x1,x1), dot(x2,x2)), 0.0);
  vec2 m1 = max(0.6 - vec2(dot(x3,x3), dot(x4,x4)            ), 0.0);
  m0 = m0 * m0;
  m1 = m1 * m1;
  return 49.0 * ( dot(m0*m0, vec3( dot( p0, x0 ), dot( p1, x1 ), dot( p2, x2 )))
               + dot(m1*m1, vec2( dot( p3, x3 ), dot( p4, x4 ) ) ) ) ;

  }