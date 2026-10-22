#version 420 compatibility
#extension GL_ARB_shader_storage_buffer_object : enable
#extension GL_ARB_shader_group_vote : enable

#define TEXTURE_COUNT 14

#define MAX_DLIGHTS 32

#define TEXTUREGRAD 1

#define PERLINFVCKERY 1

#define	LIGHTMAP_ARRAY 1

#if LIGHTMAP_ARRAY
#define LIGHTMAPNUM(a) lightmapNumsUniform[(a)]
#define DELUXELIGHTMAPNUM(a,b) (lightmapNumsUniform[(a)]+1)
#else
#define LIGHTMAPNUM(a) (a)
#define DELUXELIGHTMAPNUM(a,b) (b)
#endif


#define	CGEN_BAD 0
#define	CGEN_IDENTITY_LIGHTING 1	// tr.identityLight
#define	CGEN_IDENTITY 2		// always (1 11 11 11)
#define	CGEN_ENTITY 3			// grabbed from entity's modulate field
#define	CGEN_ONE_MINUS_ENTITY 4	// grabbed from 1 - entity.modulate
#define	CGEN_EXACT_VERTEX 5		// tess.vertexColors
#define	CGEN_VERTEX 6			// tess.vertexColors * tr.identityLight
#define	CGEN_ONE_MINUS_VERTEX 7
#define	CGEN_WAVEFORM 8			// programmatically generated
#define	CGEN_LIGHTING_DIFFUSE 9
#define	CGEN_FOG 10				// standard fog
#define	CGEN_CONST 11				// fixed color
#define	CGEN_LIGHTMAP0 12
#define	CGEN_LIGHTMAP1 13
#define	CGEN_LIGHTMAP2 14
#define	CGEN_LIGHTMAP3 15

#define TCGEN_ENVIRONMENT_MAPPED 17

#define SF_FACE 2
#define SF_GRID 3

#define GLS_SRCBLEND_ZERO						0x00000001
#define GLS_SRCBLEND_ONE						0x00000002
#define GLS_SRCBLEND_DST_COLOR					0x00000003
#define GLS_SRCBLEND_ONE_MINUS_DST_COLOR		0x00000004
#define GLS_SRCBLEND_SRC_ALPHA					0x00000005
#define GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA		0x00000006
#define GLS_SRCBLEND_DST_ALPHA					0x00000007
#define GLS_SRCBLEND_ONE_MINUS_DST_ALPHA		0x00000008
#define GLS_SRCBLEND_ALPHA_SATURATE				0x00000009
#define		GLS_SRCBLEND_BITS					0x0000000f

#define GLS_DSTBLEND_ZERO						0x00000010
#define GLS_DSTBLEND_ONE						0x00000020
#define GLS_DSTBLEND_SRC_COLOR					0x00000030
#define GLS_DSTBLEND_ONE_MINUS_SRC_COLOR		0x00000040
#define GLS_DSTBLEND_SRC_ALPHA					0x00000050
#define GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA		0x00000060
#define GLS_DSTBLEND_DST_ALPHA					0x00000070
#define GLS_DSTBLEND_ONE_MINUS_DST_ALPHA		0x00000080
#define		GLS_DSTBLEND_BITS					0x000000f0

#define GLS_DEPTHMASK_TRUE						0x00000100

#define GLS_POLYMODE_LINE						0x00001000

#define GLS_DEPTHTEST_DISABLE					0x00010000
#define GLS_DEPTHFUNC_EQUAL						0x00020000

#define GLS_ATEST_GT_0							0x10000000
#define GLS_ATEST_LT_80							0x20000000
#define GLS_ATEST_GE_80							0x40000000
#define GLS_ATEST_GE_C0							0x80000000
#define		GLS_ATEST_BITS						0xF0000000

#define GLS_DEFAULT			GLS_DEPTHMASK_TRUE
#define GLS_ALPHA			(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)




const float samplebias = 0.5f; // sample bias for parallax mapping and texture normal calc. TODO: Make it dynamic. if capturing, we can do more.

float biaslod(float baselod){
	return baselod-samplebias*baselod;
}




uniform int jitterIndexUniform; 
uniform int jitterTotalFramesUniform;
uniform int serverTimeStartUniform;
uniform int serverTimeUniform;
uniform float serverTimeFractionUniform;
#define FLOATSERVERTIME_PERLIN ((float(serverTimeUniform)+serverTimeFractionUniform)*0.001f)



const float PI = 3.1415926535;

const float ALPHA = 0.14;
const float INV_ALPHA = 1.0 / ALPHA;
const float K = 2.0 / (PI * ALPHA);
float nrand( vec2 n )
{
	return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}
float inv_error_function(float x)
{
	float y = log(1.0 - x*x);
	float z = K + 0.5 * y;
	return sqrt(sqrt(max(0.0001f,z*z - y * INV_ALPHA)) - z) * sign(x);
}

float gaussian_rand( vec2 n )
{
    int a = serverTimeUniform & 65535;
	float t = fract(fract(float(a) *13.4326426f) + fract( serverTimeFractionUniform*13.4326426f )); // MEH
	float x = nrand( n + 0.07*t );

    float mult= 0.20f;
    
	float tmp = inv_error_function(x*2.0-1.0)*mult;
    if(isinf(tmp) || isnan(tmp)){
        return 0.5;
    }
    if(jitterTotalFramesUniform > 1 /*&& blurEarlyStageUniform == 0*/){
       //tmp *= max(1.0f,0.33f*sqrt(float(jitterTotalFramesUniform)));
       //tmp *= pow(float(jitterTotalFramesUniform),0.125f);
       tmp *= pow(float(jitterTotalFramesUniform),0.5f);
    }
    if(isinf(tmp) || isnan(tmp)){
        return 0.5;
    }
    return tmp + 0.5;
}



uniform float modelBumpProximityFadeUniform;
uniform float modelBumpProximityFadeTargetUniform;
uniform float modelBumpProximitySkinFadeUniform;
uniform float modelBumpProximitySkinFadeTargetUniform;
uniform float modelBumpIntensityUniform;
uniform float modelBumpIntensitySkinUniform;
uniform float modelBumpIntensityFaceTexUniform;
uniform float modelBumpIntensityFaceTexSkinUniform;
uniform int textureIsFaceUniform; // texture name contains "head" or "face"
uniform int haveLightmapDistDataUniform;

/*
float colorSkinProbability_Reference(vec3 rgb){
	const vec3 minsrgb = vec3(0.372549,0.156862,0.078431);
	// first rgb check
	if( any(lessThan(rgb,minsrgb))
		|| rgb.r < max(0.058823+rgb.g,rgb.b)
	){
		return 0.0;
	}
	
	// convert to hsv
	// we skip the Cmax branch, we already know R is the highest component
	const vec2 hsmax = vec2(50.0,0.45);
	float delta = rgb.r-min(rgb.g,rgb.b);
	vec2 hs = vec2(
		60.0*mod((rgb.g-rgb.b)/delta,6.0),
		delta/rgb.r - 0.23
	);
	if(all(lessThanEqual(hs,hsmax))){
		return 1.0; // yup nice
	}
	
	const vec3 ydot = vec3(0.299,0.587,0.114);
	// y = r, Cb = g, Cr = b
	float y = dot(rgb,ydot);
	vec3 YCbCr = vec3(
		y,
		0.564*(rgb.b-y)+0.5,
		0.713*(rgb.r-y)+0.5
	);

	const vec3 YCbCrMin = vec3(0.313725,0.333333,0.529411);
	if( all(greaterThan(YCbCr,YCbCrMin))
	 && YCbCr.b <= (1.5862*YCbCr.g)+0.078431
	 && YCbCr.b >= (0.3448*YCbCr.g)+0.298850
	 && YCbCr.b >= (-4.5652*YCbCr.g)+0.919863
	 && YCbCr.b <= (-1.15*YCbCr.g)+1.183333
	 && YCbCr.b <= (-2.2857*YCbCr.g)+1.697450
	){
		return 1.0;
	}
	
	return 0.0;
}
float colorSkinProbability_Optimized(vec3 rgb){
	const vec3 minsrgb = vec3(0.372549,0.156862,0.078431);
	const vec3 hsmax = vec3(50.0,0.45,0.0);
	const vec3 ydot = vec3(0.299,0.587,0.114);
	const vec2 crMaxDot1 = vec2(0.3448,0.298850);
	const vec2 crMaxDot2 = vec2(-4.5652,0.919863);
	const vec2 crMinDot1 = vec2(1.5862,0.078431);
	const vec2 crMinDot2 = vec2(-1.15,1.183333);
	const vec2 crMinDot3 = vec2(-2.2857,1.697450);
	
	// first rgb check
	if( any(lessThan(rgb,minsrgb))
		|| rgb.r < max(0.058823+rgb.g,rgb.b)
	){
		return 0.0;
	}
	
	// convert to hsv
	// we skip the Cmax branch, we already know R is the highest component
	float delta = rgb.r-min(rgb.g,rgb.b);
	float s = delta/rgb.r - 0.23;
	vec3 hs = vec3(60.0*mod((rgb.g-rgb.b)/delta,6.0),s,-s);
	if(all(lessThanEqual(hs,hsmax))){
		return 1.0;
	}
	
	// y = r, Cb = g, Cr = b, 1 = a/w (just for the crMin/crMax dots)
	float y = dot(rgb,ydot);
	vec4 YCbCr = vec4(y,0.564*(rgb.b-y)+0.5,0.713*(rgb.r-y)+0.5,1.0);
	
	float crMin = max(dot(YCbCr.ga,crMaxDot1),dot(YCbCr.ga,crMaxDot2));
	float crMax = min(dot(YCbCr.ga,crMinDot1),min(dot(YCbCr.ga,crMinDot2),dot(YCbCr.ga,crMinDot3)));

	const vec3 YCbCrMin = vec3(0.313725,0.333333,0.529411);
	if( all(greaterThan(YCbCr.xyz,YCbCrMin))
	 && YCbCr.b <= crMax
	 && YCbCr.b >= crMin
	){
		return 1.0;
	}
	
	return 0.0;
}


*/

// return 0-1 likelihood of a color being a skin color. 
// based on
// "Human Skin Detection Using RGB, HSV and YCbCr Color Models"
// by S. Kolkur1, D. Kalbande2, P. Shimpi2, C. Bapat2, and J. Jatakia2
//
// with some totally non-scientific guesswork calculation to "feather" around the cutoff values
// 
// see above commented-out versions for something closer to the reference without the "feather"
// (readable and "optimized" versions)
//
// TODO do fancy optimizations?
float colorSkinProbability(vec3 rgb){
	rgb = pow(rgb,vec3(0.45));
	const vec3 minsrgb = vec3(0.372549,0.156862,0.078431);
	const vec3 hsmax = vec3(50.0,0.45,0.0);
	const vec3 ydot = vec3(0.299,0.587,0.114);
	const vec2 crMaxDot1 = vec2(0.3448,0.298850);
	const vec2 crMaxDot2 = vec2(-4.5652,0.919863);
	const vec2 crMinDot1 = vec2(1.5862,0.078431);
	const vec2 crMinDot2 = vec2(-1.15,1.183333);
	const vec2 crMinDot3 = vec2(-2.2857,1.697450);
	
	// first rgb check
	float rMin = max(0.058823+rgb.g,rgb.b);
	if( any(lessThan(rgb,minsrgb))
		|| rgb.r < rMin
	){
		return 0.0;
	}
	
	float minDist = min(rgb.r-max(minsrgb.r,rMin),min(rgb.g-minsrgb.g,rgb.b-minsrgb.b));
	float rMult = sqrt(minDist/1.0*8.0);
	float mult = 0.0;
	
	// convert to hsv
	// we skip the Cmax branch, we already know R is the highest component
	float delta = rgb.r-min(rgb.g,rgb.b);
	float s = delta/rgb.r - 0.23;
	vec3 hs = vec3(60.0*mod((rgb.g-rgb.b)/delta,6.0),s,-s);
	if(all(lessThanEqual(hs,hsmax))){
		
		float sDist = min(hsmax.g-hs.g,hs.g)/(hsmax.g)*4.0;
		float hDist = (hsmax.r-hs.r)*4.0;
		mult = max(mult,rMult*sqrt(min(sDist,hDist)));
	}
	
	// y = r, Cb = g, Cr = b, 1 = a/w (just for the crMin/crMax dots)
	float y = dot(rgb,ydot);
	vec4 YCbCr = vec4(y,0.564*(rgb.b-y)+0.5,0.713*(rgb.r-y)+0.5,1.0);
	
	float crMin = max(dot(YCbCr.ga,crMaxDot1),dot(YCbCr.ga,crMaxDot2));
	float crMax = min(dot(YCbCr.ga,crMinDot1),min(dot(YCbCr.ga,crMinDot2),dot(YCbCr.ga,crMinDot3)));

	const vec3 YCbCrMin = vec3(0.313725,0.333333,0.529411);
	if( all(greaterThan(YCbCr.xyz,YCbCrMin))
	 && YCbCr.b <= crMax
	 && YCbCr.b >= crMin
	){
		float crDist = min(crMax-YCbCr.b,YCbCr.b-crMin)/(crMax-crMin)*4.0;
		
		mult = max(mult,rMult*sqrt(crDist));
	}
	
	return clamp(mult,0.0,1.0);
}

float getModelBumpIntensity(vec3 color, float dist){
	float value = 1.0f;
	float skinValue = 0.0f;
	bool modelBumpProximitySkinFade = modelBumpProximitySkinFadeUniform > 0 && dist < modelBumpProximitySkinFadeUniform;
	bool needSkinValue = textureIsFaceUniform > 0 && modelBumpIntensityFaceTexSkinUniform != 1.0f || modelBumpIntensitySkinUniform != 1.0f || modelBumpProximitySkinFade;
	if(needSkinValue){
		skinValue = colorSkinProbability(color);
	}

	value *= modelBumpIntensityUniform;
	if(modelBumpProximityFadeUniform > 0 && dist < modelBumpProximityFadeUniform){
		value *= mix(modelBumpProximityFadeTargetUniform,1.0,dist/modelBumpProximityFadeUniform);
	}
	if(textureIsFaceUniform > 0){
		value *= modelBumpIntensityFaceTexUniform;
		value *= (1.0f-skinValue) + skinValue * modelBumpIntensityFaceTexSkinUniform; // skin color sensitive
	}

	// skin color sensitive:
	value *= (1.0f-skinValue) + skinValue * modelBumpIntensitySkinUniform;
	if(modelBumpProximitySkinFade){
		value *= (1.0f-skinValue) + skinValue * mix( modelBumpProximitySkinFadeTargetUniform, 1.0, dist/modelBumpProximitySkinFadeUniform ); 
	}
	return value;
}

uniform uint bindingRectImageBitmaskUniform;





//need 420 if we wanna try
//layout(early_fragment_tests) in;

/* AlphaFunction */
#define ALPHA_NEVER                          0x0200
#define ALPHA_LESS                           0x0201
#define ALPHA_EQUAL                          0x0202
#define ALPHA_LEQUAL                         0x0203
#define ALPHA_GREATER                        0x0204
#define ALPHA_NOTEQUAL                       0x0205
#define ALPHA_GEQUAL                         0x0206
#define ALPHA_ALWAYS                         0x0207



#define NUM_GLSL_EXTRA_LIGHTMAPS_MAX 14

uniform sampler2D text_in[31];
uniform sampler2DArray text_inArray31;


vec4 sampleTextureSafe(int sampler,vec2 uvCoords,float thelod,vec4 thegrad){
#if TEXTUREGRAD
	return textureGrad(text_in[sampler],uvCoords,thegrad.xy,thegrad.zw);
#else
	return textureLod(text_in[sampler],uvCoords,thelod);
#endif
}

in vec3 debugColor;
varying vec4 vertColor;
varying vec3 lightDir;
varying vec3 ambientLight;
varying vec3 vertexNormal;
in vec3 texUVTransform[2];

varying vec2 my_TexCoord[TEXTURE_COUNT];

//flat in uint shadowLineLightBitmasks[1152];
#define MULTDIVIDE255 0.0039215686274509803921568627451f

uniform mat4x4 worldModelViewMatrixUniform;
in mat4x4 worldModelViewMatrixReverseGeom;


uniform mat4x4 projectorModelViewMatrixUniform;
uniform mat4x4 projectorProjectionMatrixUniform;
uniform vec3 projectorPosUniform;
uniform int projectorActiveUniform;

const mat4 lightdirtransform = mat4(
	2.0f,0.0f,0.0f,0.0f,
	0.0f,2.0f,0.0f,0.0f,
	0.0f,0.0f,2.0f,0.0f,
	-1.0f,-1.0f,-1.0f,1.0f
);

in vec3 normal;
in vec3 worldNormal;

uniform int fishEyeModeUniform; //1= fisheye, 2=equirectangular
uniform float texAverageBrightnessUniform;
uniform float parallaxMapDepthUniform;
uniform float dLightFastSkipThresholdUniform;
uniform float dLightDeluxeShadowIntensityUniform;
uniform float dLightIntensityUniform;
uniform float dLightSpecIntensityUniform;
uniform float dLightSpecGammaUniform;
uniform float dLightSpecBaseReflectivityUniform; // for schlick
uniform float dLightSpecDistanceDecayUniform; // distance between light source and reflecting pixel
uniform float dLightSpecDistanceMinUniform; // no decay up to this distance
uniform float dLightAddPowUniform;
uniform float dLightAddPostPowMultUniform;


uniform int parallaxMapLayersUniform;
uniform float parallaxMapGammaUniform;

uniform float worldReflectNormalMixUniform;
uniform float worldReflectGradMultUniform;
uniform float worldReflectPuddleThreshUniform;
uniform int worldReflectMultiSampleUniform;

uniform int isLightmapUniform; 
uniform int isWorldBrushUniform; 
uniform int isSaberUniform; 
uniform int dLightFastUniform; 
uniform vec3 dLightJitterUniform; 
uniform int dLightVoxelShadowsUniform; 
uniform vec3 dLightVoxelShadowJitterUniform;
uniform int dLightVoxelShadowJitterMethodUniform;
uniform int noiseFuckeryUniform; 
uniform int noiseFuckeryLightmapUniform; 
uniform float noiseFuckeryHDRIntensityUniform; 
uniform float noiseFuckeryLightmapIntensityUniform; 
uniform vec3 viewOriginUniform; 

uniform float myFogUniform; 
uniform vec3 myFogColorUniform; 

varying vec4 eyeSpaceCoordsGeom;
varying vec4 pureVertexCoordsGeom;

#define RENDERFLAG_SIMPLELIGHTING 1
#define RENDERFLAG_NOLIGHTING 2 // skyboxes and such
#define RENDERFLAG_TWOSIDED 4 // grass and such
#define RENDERFLAG_SCENEVIEW 8 // for reflection view renders, simplified lighting and such
#define RENDERFLAG_SCENEVIEWBOUND 16 // for reflection view renders and such. have a rendered scene view bound.
#define RENDERFLAG_ISGORE 32 // is gore
#define RENDERFLAG_FASTPREVIEW 64
#define RENDERFLAG_SCENEVIEWWORLDREFLECTBOUND 128
#define RENDERFLAG_RENDERINGWORLDREFLECT 256

uniform int alphaFuncUniform; 
uniform float alphaFuncValueUniform;
uniform int renderFlagsUniform;

uniform int zPrepassUniform;

uniform int deluxeMappingUniform;
uniform int deluxeMappingFakeUniform;
uniform int deluxeMapsRGBUniform;

uniform int haveVertexLightDirectionUniform;
uniform int isModelUniform;
uniform int surfaceTypeUniform;
uniform int stageColorGenUniform;
uniform int stageTCGenUniform;
uniform uint rawStateBitsUniform;
uniform uint appliedStateBitsUniform;
uniform int stageForceNormalUniform;
uniform int stageHasTCModUniform;
uniform int gigaTCGenUniform;

uniform int thermalVisionUniform;
uniform int shaderDebugUniform;


uniform float	cloudScaleUniform;
uniform float	cloudTimeScaleUniform;
uniform float	cloudPowerUniform;
uniform float	cloudIntensityCompensateUniform;


float angleOnPlane(vec3 point, vec3 axis1, vec3 axis2)
{

	vec2 planePosition = vec2(dot(point, axis1), dot(point, axis2));
	return acos(dot(vec2(1, 0), normalize(planePosition)));
}
vec3 getPerpendicularAxis(vec3 point, vec3 mainAxis)
{
	return normalize(point - dot(point, mainAxis) *mainAxis);
}



vec2 get360UVFromVector(vec3 outVec){
	const vec3 axis[3] = {
		 vec3(0.0, 0.0, -1.0),
		 vec3(-1.0, 0.0, 0.0),
		 vec3(0.0, 1.0, 0.0)
	};
	//axis[0] = vec3(0.0, 0.0, -1.0);
	//axis[1] = vec3(-1.0, 0.0, 0.0);
	//axis[2] = vec3(0.0, 1.0, 0.0);
	const float pi = radians(180);
	float xAngle = angleOnPlane(outVec, -axis[1].xyz, axis[0].xyz) / pi;
	vec3 perpendicularZAxisToPoint = getPerpendicularAxis(outVec, axis[2].xyz);
	float yAngle = angleOnPlane(outVec, axis[2].xyz, perpendicularZAxisToPoint) / pi;
		
	float depth = dot(axis[0].xyz, outVec);
	xAngle -= 0.5;
	xAngle *= 2;
	float widthSign = sign(xAngle);
	xAngle = depth <= 0 ? xAngle : widthSign *(1.0 + (1.0 - abs(xAngle)));
	xAngle *= 0.5;

	yAngle -= 0.5;
	yAngle *= 2;
	float heightSign = sign(yAngle);
	//outFragColor.x = -xAngle;
	//outFragColor.y = 0;//yAngle;
	//outFragColor.z = 0;
	//vec2 uvRefl = vec2((xAngle+1.0)/2.0f,(yAngle+1.0)/2.0f);
	return vec2((xAngle+1.0)/2.0f,(yAngle+1.0)/2.0f);
}







const vec3 rgbToGray = vec3( 0.2989f,0.5870f, 0.1140f);

// thermal vision
//
// intensity table:
// where,L,a,b
// 0,6,24,-30
// 0.15,30,63,-108
// 0.4,63,-10,-49
// 0.65,41,-42,41
// 0.85,93,-20,89
// 1.0,54,73,66
//
// distance table:
// 0,76,-12,-32
// 0.07,65,-11,-47
// fade alpha to 0 towards 1.0
//
const float Epsilon = 0.008856f; // Intent is 216/24389
const float Kappa = 903.3f; // Intent is 24389/27
const float KappaInv = 1.0f/Kappa; // Intent is 24389/27
const vec3 white = vec3(0.95047f,1.000f,1.08883f);
const float something = 16.0f / 116.0f;
const float something2 = 1.0f / 7.787f;
//const mat3 xyztorgb = mat3(3.2406f, -0.9689f, 0.0557f,-1.5372f, 1.8758f, -0.2040f, -0.4986f, 0.0415f, 1.0570f);
const mat3 xyztorgb = mat3(3.2406f,-1.5372f,-0.4986f,-0.9689f,1.8758f,0.0415f,0.0557f,-0.2040f,1.0570f);
vec3 lab2rgb( vec3 c ) {
    float y = ( c.x + 16.0f ) / 116.0f;
    float x = c.y / 500.0f + y;
    float z = y - c.z / 200.0f;
	float x3 = x * x * x;
	float z3 = z * z * z;
    vec3 outVal = vec3(
        //white.x * (x3 > Epsilon ? x3 : (116.0f*x-16.0f)/Kappa),
        white.x * (x3 > Epsilon ? x3 : (x - something)*something2),
        white.y * (c.x > (Kappa * Epsilon) ? y*y*y : c.x *KappaInv),
        white.z * (z3 > Epsilon ? z3 : (z - something) *something2));
	return outVal*xyztorgb ;
};
const vec3 heatLUT[21] = {
	// 0,6,24,-30
	vec3(6.0f,24.0f,-30.0f),vec3(13.92f,36.87f,-55.74),vec3(21.84f,49.74f,-81.48f),
	// 0.15,30,63,-108
	vec3(30.0f,63.0f,-108.0f),vec3(36.6f,48.4f,-96.2f),vec3(43.2f,33.8f,-84.4f),vec3(49.8f,19.2f,-72.6f),vec3(56.4f,4.6f,-60.8f),
	// 0.4,63,-10,-49
	vec3(63.0f,-10.0f,-49.0f),	vec3(58.6f,-16.4f,-31.0f),	vec3(54.2f,-22.8f,-13.0f),	vec3(49.8f,-29.2f,5.0f),vec3(45.4f,-35.6f,23.0f),
	// 0.65,41,-42,41
	vec3(41.0f,-42.0f,41.0f),	vec3(54.0f,-36.5f,53.0f),vec3(67.0f,-31.0f,65.0f),vec3(80.0f,-25.5f,77.0f),
	// 0.85,93,-20,89
	vec3(93.0f,-20.0f,89.0f),	vec3(80.13f,10.69f,81.41f),vec3(67.26f,41.38f,73.82f),
	// 1.0,54,73,66
	vec3(54.0f,73.0f,66.0f),
};

const vec3 veryFarColor = vec3(76,-12,-32);
const vec3 farColor = vec3(65,-11,-47);
void heatVision(inout vec4 colorInOut, vec3 lightmapIn, vec3 mynormal){
	bool additive = (rawStateBitsUniform & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_ONE && (rawStateBitsUniform & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_ONE;
	bool mult = (rawStateBitsUniform & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_DST_COLOR || (rawStateBitsUniform & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_SRC_COLOR;
    bool additiveToAlpha = thermalVisionUniform == 3 && additive;
	bool legacy = thermalVisionUniform == 2;
	vec3 colorIn = colorInOut.xyz;
	float rawIntensity = clamp(dot(rgbToGray,colorInOut.xyz),0.0f,1.0f);
	float powfactor = 0.2f;
	float normalmult = 1.0f;
	float distanceFactor =  0.25f;
	//if(/* additive || mult || isSaberUniform > 0 ||*/(rawStateBitsUniform & (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS)) == 0){
		//colorInOut = vec4(0.0f,0.0f,0.0f,0.0f);
		//return;
	//}
	//if((rawStateBitsUniform & (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS)) > 0 ){
	//	colorInOut = vec4(0.0f,0.0f,0.0f,0.0f);
	//	return;
	//}
	if((renderFlagsUniform & RENDERFLAG_NOLIGHTING) == 0){
		// dont do for sky cuz it spazzes out
		distanceFactor =  length(eyeSpaceCoordsGeom);
		distanceFactor = 1.0f/(distanceFactor*0.001f+1.0f);
	}
	if(isModelUniform > 0 || /*stageColorGenUniform == CGEN_LIGHTING_DIFFUSE ||*/ stageForceNormalUniform > 0){
		powfactor = 0.45f;
		colorIn /= texAverageBrightnessUniform;
		normalmult = clamp(dot(mynormal,normalize(-eyeSpaceCoordsGeom.xyz)),0.0f,1.0f);
	}
	if(isSaberUniform > 0){
		powfactor = 1.0f;
		//colorIn -= 1.0f/2550.0f;
		if(legacy){
			colorIn*= 400.0f;
		} else{
			colorIn*= 200.0f;
		}
	}
	colorIn.x = pow(colorIn.x,powfactor);
	colorIn.y = pow(colorIn.y,powfactor);
	colorIn.z = pow(colorIn.z,powfactor);
	if((isModelUniform > 0 || /*stageColorGenUniform == CGEN_LIGHTING_DIFFUSE ||*/ stageForceNormalUniform > 0) && !additive){
		colorIn *= 0.5f;
		colorIn += vec3(0.5f);
		colorIn *= 40.0f;
	}
	float lightmapMult = additiveToAlpha ? rawIntensity : 1.0f;
	float intensity =  dot(rgbToGray,colorIn+lightmapMult*lightmapIn*1.0f)*0.03f*normalmult;
	float multiplier = 1.0f;
	if(intensity < 0.0f){
		intensity = 0.0f;
	}  
	if(thermalVisionUniform == 2 || thermalVisionUniform == 3){
		if(additiveToAlpha){
			float alpha = rawIntensity;//clamp(intensity,0.0f,1.0f);
			//alpha *= alpha;
			if(intensity > 1.0f){
				intensity=1.0f + (1.0f-1.0f/intensity);
			}
			if(alpha > 0.0f){
				//intensity /= alpha; // premultiply it so we keep the correct amount
			}
			colorInOut = vec4(0.0f,intensity,distanceFactor,alpha);
		} else {
			if(intensity > 1.0f){
				intensity=1.0f;
			}
			float alpha = !additive ? 1.0f : rawIntensity;//clamp(intensity,0.0f,1.0f);
			colorInOut.xyz = vec3(0.0f,intensity*alpha,distanceFactor*alpha);
		}
		return;
	}
	if(intensity > 1.0f){
		multiplier = intensity;
		intensity=1.0f;
	}

	intensity *= 20.0f;
	int index = clamp(int(intensity),0,19);
	float lerp = intensity - float(index);
	vec3 result = mix(heatLUT[index],heatLUT[index+1],lerp);
	vec3 distcolor = mix(veryFarColor,farColor,clamp(distanceFactor*14.28f,0.0f,1.0f));
	result = mix(distcolor,result,min(1.0f,distanceFactor*1.1f));
	result = lab2rgb(result);
	result *= multiplier;
	result.x = max(0.0f,result.x);
	result.y = max(0.0f,result.y);
	result.z = max(0.0f,result.z);
	//return result*0.25f;
	colorInOut.xyz = result*0.25f;
	//return (result*0.5f+100.0f)*0.01f;
}


// multipass stuff
#define MYGL_MODULATE                       0x2100
#define MYGL_DECAL                          0x2101
#define MYGL_ADD							0x0104
#define MYGL_REPLACE                        0x1E01
uniform int stageImageBitmaskUniform;
uniform int stageLightmapBitmaskUniform;
uniform int multiTexModeUniform;

float angleAttenuate(float lightnormalDot){
	float fd90 = 0.5;
	float inv = 1.0f - lightnormalDot;
    float burleysimple = 1.0f + (fd90 - 1.0f) * inv*inv*inv*inv*inv;
	return lightnormalDot*burleysimple;
}


float snoise(vec4 v);


struct dlight_t {
	vec3			origin;
	vec3			color;				// range from 0.0 to 1.0, should be color normalized
	float			radius;
	float			mindist;
};

struct dlightCheap_t {
	vec4			origin;
	vec4			color;
	float			radius;
	float			mindist;
	int				flags;
	int				filler2;
};

uniform int dLightsCountUniform;
uniform dlight_t dLightsUniform[32]; 

uniform int shaderStylesUniform[14];
uniform int lightmapNumsUniform[32];

#define SHADOWLINE_PARENTSKIPS 1

struct shadowline_t {
	vec4			point1;
	vec4			point2;
	float			width;
	float			a;
	float			b;
	float			c;
	float			d;
	float			e;
	int				flags; // 1 = use point1 for feet shadow, 2 = only ambient occlusion, 4 = pvsVisible (dynamically calculated)
	// automatically calculated:
	float			halfLineLength;
	vec4			middle;
	vec4			lightdir; // for foot shadows
	vec4			parentPos;
	vec4			lightDots[MAX_DLIGHTS];
	vec4			lightExtraInfo[MAX_DLIGHTS]; // 0 = fast skip possible  3 = plane dist
	int				parentIndex;
	int				fastSkipPossible;
	int				h;
	int				i;
};

uniform int shadowLinesCountUniform;
uniform int cheapLightsCountUniform;
//uniform shadowline_t shadowLinesUniform[64*18]; 

struct variousSSBOData_t { // various static ssbo stuff.
	vec4	styleSundirections[64];
};

layout(std430, binding = 8) buffer variousDataLayout
{
    variousSSBOData_t variousData;
};
layout(std430, binding = 6) buffer lightStyleIntensitiesLayout
{
    vec4 lightStyles[64];
};

layout(std430, binding = 3) buffer shadowLinesLayout
{
    shadowline_t shadowLines[64*18];
};

layout(std430, binding = 7) buffer cheaplightsLayout
{
    dlightCheap_t cheaplights[1024];
};


float above1To2SoftApproach(float value) // the result of this will approach 2 but never reach it. perfection
{
    return (1.0f - 1.0f / (1.0f + pow(value, 0.833333333333333333f))) * 2.0f; // dont ask me why exactly 0.83333. i just aligned the two derivatives in le graphing calculator :)
}

float getNormalizedColorIntensity(vec3 color){
	// old method that just cuts off
	//return max(min((color.x + color.y + color.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f);
	float normalized = max((color.x + color.y + color.z)*0.33333333f/texAverageBrightnessUniform,0.0f);



	return 0.5f*above1To2SoftApproach(normalized*2.0f);
}






// direction mustt be in eye space and normalized
vec4 getVertexLightIntensity(vec4 color, vec3 direction, vec3 referenceNormal, vec3 lightNormal, vec3 viewerVectorNorm, float specIntensitySchlickMult,float viewerDistance, bool twoSided, bool allowDarken){
	
	//return vec4(1.0f);
	vec3 maybeMirroredLightNormal =  twoSided && dot(referenceNormal,direction) < 0 ? -lightNormal : lightNormal;
	// REMEMBER: IF WE SEE SHIMMER ON DARK SIDES OF OBJECTS, PUT A VERSION OF THIS BACK.
	float intensity = 1.0f;
	float referenceAlignment = dot(direction,referenceNormal);
	vec4 oldColor = color;
	if(referenceAlignment<0 && !twoSided){
		if(referenceAlignment<-0.5){
			return allowDarken ?vec4(0.0f):color;
		}
		// fade out towards back
		// cutting off hard leads to ugly seam due to calculateTextureNormal basically adding "noise" to the normal
		// and thus no longer allowing us to have such a nice cutoff point.
		// on the other hand, just allowing anything leads to shimmer on backsides of objects
		intensity = (referenceAlignment+0.5f) * 2.0f; 
	}
	//if((stageLightmapBitmaskUniform & (1<<2))>0)
	{
		
		//if(true){
			
			float alignment = max(0.0f,dot(maybeMirroredLightNormal,(direction).xyz));
			//float alignment = dot(worldLightNormal,direction.xyz);

			float ambientFactor = twoSided ? 0.25f : 0.0f; // twosided is stuff like leafs etc. dont let them get totally black

			//do some specular
			vec3 lightVector1Norm = -normalize(direction.xyz);
			vec3 mirroredVec = lightVector1Norm - 2.0*maybeMirroredLightNormal*dot(lightVector1Norm,maybeMirroredLightNormal);
			vec3 mirroredVecNorm = normalize(mirroredVec);

			float specIntensity = pow(max(0.0,dot(mirroredVecNorm,viewerVectorNorm)),dLightSpecGammaUniform);
			
			// do schlick's approximation of fresnel. steep angles looking onto surface: more reflective
			specIntensity *= specIntensitySchlickMult;
			
			float totalDist = viewerDistance; // + dist // dont know distance to light
			//vec3 addVal = color.xyz*specIntensity*dLightSpecIntensityUniform/totalDist;
			float specIntensityTotal = 300.0f*specIntensity*dLightSpecIntensityUniform/max(0.02f,totalDist);

			//ambientFactor = 1.0f - (   (1.0f-ambientFactor) * modelBumpIntensity );
			//color.xyz *= (1.0f-ambientFactor)*alignment*alignment*alignment+modelBumpIntensity*specIntensityTotal + ambientFactor;
			color.xyz *= (1.0f-ambientFactor)*alignment*alignment*alignment+specIntensityTotal + ambientFactor;
			//color.xyz = vec3(totalDist);
			//color *= alignment*alignment*alignment+specIntensityTotal;
			//color *= 100.0f;
			color.xyz *= intensity;

		//}
	}
	if(!allowDarken){
		color = max(color,oldColor);
	}
	return color;
}

vec3 powVec(vec3 invec, float power){
	return vec3(
		pow(invec.x,power),
		pow(invec.y,power),
		pow(invec.z,power)
	);
}

vec3 srgbtolinear(vec3 inColor){

	return mix(pow((inColor+0.055f)*(1.0f/1.055f),vec3(2.4f)),inColor*(1.0f/12.92f),lessThanEqual(inColor, vec3(0.04045f)));
}
vec3 lineartosrgb(vec3 inColor)
{
    return mix(vec3(1.055)*pow(inColor, vec3(1.0/2.4)) - vec3(0.055), inColor * vec3(12.92), lessThan(inColor, vec3(0.0031308)));
}


vec4 getLightmapIntensity(bool haveVertLightDir, int sampler, int deluxeSampler, vec2 lmtexcoord, vec3 eyespacelightdir, bool havedeluxe, vec3 lightNormal, vec3 lightReferenceNormal, mat4 dirmat, vec3 viewerVectorNorm, float specIntensitySchlickMult,float viewerDistance, bool twoSided, int style, vec3 worldPixel){
	vec4 color;
	vec4 direction = vec4(1.0f);
	bool haveDir = false;
	bool doDist = true;
	bool haveDist = haveLightmapDistDataUniform > 0; // todo have a uniform inform us.
	//if((stageLightmapBitmaskUniform & (1<<2))>0)
	{
		//return vec4(-vertexNormal,1.0f)*0.1f;
		//vec2 thelod = textureQueryLod(sampler,lmtexcoord);
		float directionality = 1.0f;
#if LIGHTMAP_ARRAY
		if((stageLightmapBitmaskUniform & (1<<sampler))>0){
			color = texture(text_inArray31, vec3(lmtexcoord,LIGHTMAPNUM(sampler)));
			//color = vec4(float(LIGHTMAPNUM(sampler))*0.02f);
			//color = texture(text_inArray31, vec3(0.5f,0.5f,1));
			if(haveDist){
				directionality = color.w;
			} else{
				directionality = 1.0f;
			}
			color.w = 1.0;
		} else{
			color = texture2D(text_in[sampler], lmtexcoord);
		}
#else
		color = texture2D(text_in[sampler], lmtexcoord);
#endif
		//return vec4(1.0f);
		//return color;
		if(havedeluxe || haveVertLightDir){
			float lightDistance = 1000.0f;
			if(havedeluxe){
#if LIGHTMAP_ARRAY
				direction = texture(text_inArray31, vec3(lmtexcoord,DELUXELIGHTMAPNUM(sampler,deluxeSampler)));
#else
				direction = texture2D(text_in[deluxeSampler], lmtexcoord); // visualize n
#endif
				//float baseMultiplier = 1.0f / max(0.00001,dot(normal,(direction).xyz));
				//return direction;
				//color.xyz = vec3(direction.w*0.001f);
				if(haveDist){
					lightDistance = direction.w;
				} else{
					lightDistance = 300.0f;
				}
				if(deluxeMapsRGBUniform > 0){
					direction.xyz = lineartosrgb(direction.xyz);
				}
				if(direction.x == 0 && direction.y == 0 && direction.z == 0){
					// fallback
					if(haveVertLightDir){
						direction = vec4((eyespacelightdir),1.0f);
						//color.r *= 4.0f;
					} else{
						// don't see how that would ever happen under normal circumstances but oh well, nice debugging?
						color.b *= 16.0f;
					}
				} else{
					direction.w = 1.0f;
					direction = (dirmat*direction);
					if(deluxeMappingFakeUniform > 0){
						// smooth things out if they got wonky
						direction.xyz = mix(lightReferenceNormal,direction.xyz,clamp(dot(lightReferenceNormal,direction.xyz)*5.0f,0.0f,1.0f));
					}
				}
				
			} else {
				direction = vec4((eyespacelightdir),1.0f);
			}
			haveDir = true;
			vec3 maybeMirroredNormal = twoSided && dot(lightReferenceNormal,direction.xyz) < 0 ? -lightReferenceNormal : lightReferenceNormal;
			//vec3 maybeMirroredNormalDefault = twoSided && dot(normal,direction.xyz) < 0 ? -normal : normal;
			//maybeMirroredNormal = maybeMirroredNormalDefault;
			float dotbase = dot((maybeMirroredNormal),(direction).xyz);
			float dotbaseHL  = 0.5f+0.5f*dotbase;
			float divider = max(0.05f,dotbaseHL*dotbaseHL); // 0.05f because that's about the limit before we start seeing ugly seams at lightmaps/deluxemaps wrapping around corners/light bleeding.
			//float divider = max(0.05f,dotbase); // 0.05f because that's about the limit before we start seeing ugly seams at lightmaps/deluxemaps wrapping around corners/light bleeding.
			vec3 maybeMirroredLightNormal = twoSided && dot(lightReferenceNormal,direction.xyz) < 0 ? -lightNormal : lightNormal;
			float alignment = max(0.05f,dot((maybeMirroredLightNormal),(direction).xyz));
			alignment /= divider;
			//return vec4(vec3(alignment),1.0f);
			//color /= max(0.00001,dot(normal,(direction).xyz));
			alignment = max(0.0f,alignment);
			//alignment = 1.0f;

			//do some specular
			vec3 lightVector1Norm = -normalize(direction.xyz);
			//vec3 maybeMirroredSpecLightNormal = maybeMirroredNormal;//twoSided && dot(vertexNormal,direction.xyz) < 0 ? -vertexNormal : vertexNormal;
			vec3 mirroredVec = lightVector1Norm - 2.0*maybeMirroredLightNormal*dot(lightVector1Norm,maybeMirroredLightNormal);
			vec3 mirroredVecNorm = normalize(mirroredVec);

			float specIntensity = pow(max(0.0,dot(mirroredVecNorm,viewerVectorNorm)),dLightSpecGammaUniform);
			
			// do schlick's approximation of fresnel. steep angles looking onto surface: more reflective
			specIntensity *= specIntensitySchlickMult;
			
			float totalDist = viewerDistance; // + dist // dont know distance to light
			//vec3 addVal = color.xyz*specIntensity*dLightSpecIntensityUniform/totalDist;
			float specIntensityTotal = 900.0f*specIntensity*dLightSpecIntensityUniform/totalDist/divider;

			color *=alignment*alignment*alignment+specIntensityTotal;
			color = max(vec4(0.0f),color);
			
			
		}
	}
	color*=lightStyles[style]*MULTDIVIDE255;
	if(style > 55 && dot(color.xyz,color.xyz) > dLightFastSkipThresholdUniform*dLightFastSkipThresholdUniform){ // TODO make style number dynamic (55). anything above that is considered a sun
		
		vec3 sundir = variousData.styleSundirections[style].xyz;
		vec3 projectedWorldPixel = worldPixel - worldPixel.z*(sundir / max(sundir.z,0.001f));
		vec2 uv = projectedWorldPixel.xy*0.00005f*cloudScaleUniform+(float((serverTimeUniform-serverTimeStartUniform))*0.000005f*cloudTimeScaleUniform + serverTimeFractionUniform* 0.000005f*cloudTimeScaleUniform)*vec2(1.0f,1.0f);
		
		if(shaderDebugUniform == 2){
			color.x *= fract(uv.s);
			color.y *= fract(uv.t);
			color.z = 0;
		} else{
			vec3 mult = texture2D(text_in[28],uv).xyz;
			vec3 multBlur = textureLod(text_in[28],uv,4.0f).xyz;

			vec4 worldDirection = normalize(worldModelViewMatrixReverseGeom*vec4(( haveDir? direction.xyz : lightReferenceNormal.xyz),0.0f));
			float weight =  clamp(dot(sundir,worldDirection.xyz)*1.0f,0.0f,1.0f);
			color.xyz *= pow(((1.0f-weight)*multBlur) + weight*mult,vec3(cloudPowerUniform))*cloudIntensityCompensateUniform;
		}
		
	}

	return color;
}

//float shortestDistanceLinesSquaredMeh(vec3 a0,vec3 a1, vec3 b0, vec3 b1,inout int type, float quitThreshold){
//	return anyInvocationARB(dLightFastUniform > 0) ? shortestDistanceLinesSquaredNew(a0,a1,b0,b1,type,quitThreshold):shortestDistanceLinesSquaredOld(a0,a1,b0,b1,type,quitThreshold);
//}

// 	1.660317619104158771	-0.58757266606617910577	-0.072916573137668344234
//	-0.12440670211719027597	1.1328007408693037184	-0.0083489374502384976625
//	-0.018111363657382022825	-0.10059653109674500886	1.1187664817637203281
const mat3 HDRtoSRGB = mat3(1.660317619104158771,	-0.58757266606617910577,	-0.072916573137668344234, -0.12440670211719027597,	1.1328007408693037184,	-0.0083489374502384976625, -0.018111363657382022825,	-0.10059653109674500886	,1.1187664817637203281);

const vec3 footadjust = vec3(0.0f,0.0f,-4.0f);

bool main_real(inout vec4 outFragColor, inout bool isinvisible)
{
	//gl_FragColor.xyz = vertexNormal;
	//gl_FragColor.w = 1.0f;
	//return;

	if(zPrepassUniform != 0){
		//return false;
	}

	int perlinFuckery = 0;

	bool twoSided = (renderFlagsUniform & RENDERFLAG_TWOSIDED) > 0;
	bool sceneView = (renderFlagsUniform & RENDERFLAG_SCENEVIEW) > 0;
	bool fastPreview = (renderFlagsUniform & RENDERFLAG_FASTPREVIEW) > 0;
	bool fastLighting = sceneView || fastPreview; // can add additional options
	bool superfastLighting = fastPreview;
	
	bool multitex = (stageImageBitmaskUniform & 2) > 0;
	bool standAloneLightmap = !multitex && (stageLightmapBitmaskUniform & 1) > 0;
	bool haveLightmap = (stageLightmapBitmaskUniform & 1) > 0 || multitex && (stageLightmapBitmaskUniform & 3) > 0;

#define MAX_SSR_MULTISAMPLE 5  // worldReflectMultiSampleUniform
	int ssrMultiSamples = clamp(worldReflectMultiSampleUniform+1,1,MAX_SSR_MULTISAMPLE);
	int ssrMultiSampleCount = ssrMultiSamples*ssrMultiSamples;

	bool tex0IsRect = (bindingRectImageBitmaskUniform & (1<<0)) > 0;
	bool tex1IsRect = (bindingRectImageBitmaskUniform & (1<<1)) > 0;

	vec2 rawUVCoords = my_TexCoord[0].st;
	if(tex0IsRect){
		rawUVCoords = gl_FragCoord.xy / textureSize(text_in[0],0); // dumb hack. prolly breaks with supersampling :/ TODO get the vidwidth as uniform
		//rawUVCoords = gl_TexCoord[0].st; // auto-generated (meh doesnt work)
	}
	
	// not sure about surfaceTypeUniform == SF_FACE rn, i did that cuz of my lil floorlight normals getting busted due to the thin strips
	vec3 lightReferenceNormal = (isModelUniform > 0 || stageForceNormalUniform > 0 || surfaceTypeUniform == SF_GRID || surfaceTypeUniform == SF_FACE) ? normalize(mat3(gl_ModelViewMatrix)*normalize(vertexNormal)) : normal; // can be normal instead. trying vertexnormal so things are smoother
	vec3 worldPixel = (worldModelViewMatrixReverseGeom*eyeSpaceCoordsGeom).xyz;
		
	bool haveVertLightDir = haveVertexLightDirectionUniform > 0 || projectorActiveUniform>0;
	vec3 projectorLightDir = - normalize(worldPixel-projectorPosUniform);

	#define PROJECTOR_DEBUG 0
	if(projectorActiveUniform > 0){
		//vec2 shadowBufferCoords = gl_FragCoord.xy / textureSize(text_in[27],0);
		//vec4 shadowBuffer = textureLod(text_in[27],shadowBufferCoords,0);
		vec4 shadowBuffer = texelFetch(text_in[27],ivec2(gl_FragCoord.xy),0);
		//if(worldPixel - projectorPos){
		//lightReferenceNormal
		//}
		// untested, but i figure its something like this...
		if(shadowBuffer.x != 0){
		#if PROJECTOR_DEBUG
			outFragColor = vec4(0,1,0,1);
		#else
			outFragColor = vec4(0);
		#endif
			isinvisible = true;
			return true; // ok? why do light calc for shit that isnt even visible
		}
		vec4 projectorEyePixel = projectorModelViewMatrixUniform*vec4(worldPixel,1.0f);
		vec4 clipSpace = projectorProjectionMatrixUniform*projectorEyePixel;
		if(clipSpace.w >= 0)
		{
		#if PROJECTOR_DEBUG
			outFragColor = vec4(0,0,1,1);
		#else
			outFragColor = vec4(0);
		#endif
			isinvisible = true;
			return true; // ok? why do light calc for shit that isnt even visible
		}
		clipSpace.xyz /= clipSpace.w;
		if(clipSpace.x < -1 || clipSpace.x > 1 || clipSpace.y < -1 || clipSpace.y > 1)
		{
		#if PROJECTOR_DEBUG
			outFragColor = vec4(1,0,0,1);
		#else
			outFragColor = vec4(0);
		#endif
			isinvisible = true;
			return true; // ok? why do light calc for shit that isnt even visible
		}
		rawUVCoords.s = clipSpace.x*0.5f + 0.5f;
		rawUVCoords.t = clipSpace.y*0.5f + 0.5f;
	}

	vec3 viewerVector = -eyeSpaceCoordsGeom.xyz;
	bool doingGigaEnvTCGen = false;
	bool isSimpleTCGenEnv = stageTCGenUniform == TCGEN_ENVIRONMENT_MAPPED && stageHasTCModUniform == 0;
	if( isSimpleTCGenEnv && gigaTCGenUniform > 0 && !tex0IsRect){
		vec3 viewer = (worldModelViewMatrixReverseGeom*vec4(viewerVector,0)).xyz;
		viewer = normalize(viewer);
		vec3 lightReferenceNormalWorld = (worldModelViewMatrixReverseGeom*vec4(lightReferenceNormal,0)).xyz;

		float d = dot(lightReferenceNormalWorld, viewer);

		vec3 reflected = lightReferenceNormalWorld*2.0f*d - viewer;

		rawUVCoords.x = 0.5f + reflected.y * 0.5f;
		rawUVCoords.y = 0.5f - reflected.z * 0.5f;
		//rawUVCoords = vec2(0);
		doingGigaEnvTCGen = true;
		//outFragColor = vec4(1);
		//return true;
	}

	vec2 uvCoords = rawUVCoords;

	vec2 uvCoordsWorldReflect[MAX_SSR_MULTISAMPLE*MAX_SSR_MULTISAMPLE];
	vec3 effectiveUVPixelPos = eyeSpaceCoordsGeom.xyz;
	vec3 effectiveUVPixelPosWorldReflect[MAX_SSR_MULTISAMPLE*MAX_SSR_MULTISAMPLE];
	effectiveUVPixelPosWorldReflect[0] = eyeSpaceCoordsGeom.xyz;
	vec4 color;
	vec4 colorWorldReflect[MAX_SSR_MULTISAMPLE*MAX_SSR_MULTISAMPLE];
	
	bool renderingSSRBuffer = (renderFlagsUniform & RENDERFLAG_RENDERINGWORLDREFLECT) > 0;
	bool vertexLit = (lightDir[0] != 0.0f || lightDir[1] != 0.0f || lightDir[2] != 0.0f) && haveVertexLightDirectionUniform > 0 && stageLightmapBitmaskUniform == 0;
	if(projectorActiveUniform > 0){
		vertexLit = true;
	}
	
	float thelod = textureQueryLod(text_in[0],uvCoords).x;
	thelod = thelod - biaslod(thelod);
	float gradMultiplier = jitterTotalFramesUniform == 0 ? 0.5f : 1.0f / sqrt(float(jitterTotalFramesUniform)/3.0f);
	//if(thermalVisionUniform > 0){
	//	gradMultiplier*=4.0f;
	//}
	float gradnoise = clamp(1.15f*2.0f*gaussian_rand(uvCoords),1.0f,1.3f); // try to smooth out the transition between levels of detail, as it forms a straight line thats visible on high frequency textures even with anisotropic filtering
	vec4 rawgrad = vec4(dFdx(uvCoords),dFdy(uvCoords));
	vec4 thegrad = rawgrad * gradMultiplier * gradnoise;
	vec4 thegradWorldReflect = rawgrad * worldReflectGradMultUniform * gradnoise;
	//textureGrad(0,uvCoords,thegrad.xy,thegrad.zw);


    if(fishEyeModeUniform == 0 && projectorActiveUniform == 0 && !tex0IsRect){
	
		uvCoords = rawUVCoords;	
	}

	color = sampleTextureSafe(0, uvCoords, thelod,thegrad);
	outFragColor = color;

	vec4 vertexLitMult = vec4(1.0f);
	if(vertexLit){
		vertexLitMult = vertColor;
		outFragColor.w *= vertColor.w;
	} else {
		outFragColor *= vertColor;
	}
	

	bool thermalVision = thermalVisionUniform > 0 && thermalVisionUniform <= 3;

	float effectiveAlpha = color.w*vertColor.w;

	//bool usesBlending = (appliedStateBitsUniform & GLS_SRCBLEND_BITS) > 0 && (appliedStateBitsUniform & GLS_DSTBLEND_BITS) > 0;

	bool mult1 = (rawStateBitsUniform & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_DST_COLOR;// (rawStateBitsUniform & GLS_SRCBLEND_DST_COLOR) > 0;
	bool mult2 = (rawStateBitsUniform & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_SRC_COLOR;//(rawStateBitsUniform & GLS_DSTBLEND_SRC_COLOR) > 0;
	bool isDecal = (mult1 || mult2);

	// a decal is multiplying what's behind it. let's not doubledip when it comes to applying lighting?
	// also, if cgen is const, good chance we are dealing with a light or sth, not sth that's actually being lit
	// so may as well just keep it as it is?
	bool allowDarken = !isDecal && stageColorGenUniform != CGEN_CONST; 

	if ((renderFlagsUniform & RENDERFLAG_NOLIGHTING) > 0){
		if(thermalVision){
			heatVision(outFragColor,vec3(0.0f),lightReferenceNormal);
		}
		if(vertexLit){
			outFragColor.xyz *= vertColor.xyz;
		}
		return true;
	//} else if(effectiveAlpha <= 0.0 && usesBlending) { // this causes issues (thermalvision 3) in its current form. leads to WEIRD negative values and all sorts of weird af shit
		//return true; // this seem fair?
	} else if(alphaFuncUniform > 0){
		if(
		alphaFuncUniform == ALPHA_GREATER && effectiveAlpha <= alphaFuncValueUniform
		|| alphaFuncUniform == ALPHA_LESS && effectiveAlpha >= alphaFuncValueUniform
		|| alphaFuncUniform == ALPHA_GEQUAL && effectiveAlpha < alphaFuncValueUniform
		){
			
			float decalSub = (mult1 && mult2) ? 0.5f : 1.0f;
			if(isDecal){
				outFragColor.xyz = vec3(decalSub);// decal. WEIRD
			}
			isinvisible = true;
			return true; // ok? why do light calc for shit that isnt even visible
		}
	}

	mat3 rotatemat = mat3(worldModelViewMatrixUniform);
	
	vec3 lightmapReferenceNormal = normalize(mat3(gl_ModelViewMatrix)*normalize(vertexNormal)); // can be normal instead. trying vertexnormal so things are smoother

	//vec3 lightNormal = normal;
	vec3 lightNormal = vertexLit ? lightReferenceNormal : lightmapReferenceNormal;
	if(!isSimpleTCGenEnv && projectorActiveUniform == 0){
		lightNormal = lightNormal;
	}
	vec3 lightNormalWorldReflect[MAX_SSR_MULTISAMPLE*MAX_SSR_MULTISAMPLE];
	lightNormalWorldReflect[0] = lightNormal;
	

	mat4 deluxedirmat = mat4(rotatemat)*lightdirtransform; // takes the raw deluxe map value and turns it into the light direction in eye space
	

	float boringShadowingIntensity = 1.0f;

	vec3 baseColorForLightingReal = outFragColor.xyz;
	vec3 baseColorForLighthmapLighting  = vec3(1.0);
	vec3 baseColorForTexLighting  = outFragColor.xyz;
	vec3 baseColorForLighting = (vertexLit || haveLightmap) ? baseColorForLighthmapLighting : baseColorForTexLighting;

	
	float boringShadowSubtractValBase = (1.0f - boringShadowingIntensity);
	vec3 boringShadowSubtractVal = baseColorForLightingReal * boringShadowSubtractValBase;
	
	vec3 addValue = vec3(0.0);

	float dLightFastSkipThresholdUniformSquared = dLightFastSkipThresholdUniform*dLightFastSkipThresholdUniform;
	
	vec3 viewerVectorNorm = normalize(viewerVector);
	float viewerDistance = length(viewerVector);
	float cosviewercomponent = 1.0 - max(0.0,dot(lightNormal,viewerVectorNorm));
	float specIntensitySchlickMult = dLightSpecBaseReflectivityUniform+(1.0-dLightSpecBaseReflectivityUniform)*cosviewercomponent*cosviewercomponent*cosviewercomponent*cosviewercomponent*cosviewercomponent;


	if(thermalVision){
		addValue *= 10.0f;
	}

	//addValue = pow(addValue,0.5f);
	//addValue = sqrt(addValue);
	addValue.x = pow(addValue.x,dLightAddPowUniform);
	addValue.y = pow(addValue.y,dLightAddPowUniform);
	addValue.z = pow(addValue.z,dLightAddPowUniform);
	addValue *= dLightAddPostPowMultUniform;

	vec4 lightmapStyleAdd = vec4(0);

	vec3 addValueForLightmap = addValue;

	vec3 baseLightDir = normalize(lightDir);
	if(projectorActiveUniform>0){
		baseLightDir = projectorLightDir;
	}
	vec3 eyeSpaceLightdir = normalize(rotatemat*baseLightDir);

	if(haveLightmap){// this is super lame xd. idk, cba to code something that actually makes sense :) at least it kinda works
		// styles
		if((stageLightmapBitmaskUniform & (1<<2))>0){
			lightmapStyleAdd += getLightmapIntensity(haveVertLightDir,2,17,my_TexCoord[2].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<17)) > 0, lightNormal,lightmapReferenceNormal,deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[1],worldPixel);		
		}
		if((stageLightmapBitmaskUniform & (1<<3))>0){
			lightmapStyleAdd += getLightmapIntensity(haveVertLightDir,3,18,my_TexCoord[3].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<18)) > 0, lightNormal,lightmapReferenceNormal,deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[2],worldPixel);		
		}
		if((stageLightmapBitmaskUniform & (1<<4))>0){
			lightmapStyleAdd += getLightmapIntensity(haveVertLightDir,4,19,my_TexCoord[4].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<19)) > 0, lightNormal,lightmapReferenceNormal,deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[3],worldPixel);		
		}
		if((stageLightmapBitmaskUniform & (1<<5))>0){
			lightmapStyleAdd += getLightmapIntensity(haveVertLightDir,5,20,my_TexCoord[5].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<20)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[4],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<6))>0){
			lightmapStyleAdd += getLightmapIntensity(haveVertLightDir,6,21,my_TexCoord[6].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<21)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[5],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<7))>0){
			lightmapStyleAdd += getLightmapIntensity(haveVertLightDir,7,22,my_TexCoord[7].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<22)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[6],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<8))>0){
			lightmapStyleAdd += getLightmapIntensity(haveVertLightDir,8,23,my_TexCoord[8].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<23)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[7],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<9))>0){
			lightmapStyleAdd += getLightmapIntensity(haveVertLightDir,9,24,my_TexCoord[9].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<24)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[8],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<10))>0){
			lightmapStyleAdd += getLightmapIntensity(haveVertLightDir,10,25,my_TexCoord[10].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<25)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[9],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<11))>0){
			lightmapStyleAdd += getLightmapIntensity(haveVertLightDir,11,26,my_TexCoord[11].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<26)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[10],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<12))>0){
			lightmapStyleAdd += getLightmapIntensity(haveVertLightDir,12,27,my_TexCoord[12].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<27)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[11],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<13))>0){
			lightmapStyleAdd += getLightmapIntensity(haveVertLightDir,13,28,my_TexCoord[13].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<28)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[12],worldPixel);			
		}/**/
		
		/*
		if((stageLightmapBitmaskUniform & (1<<2))>0){
			lightmapStyleAdd += texture2D(2, my_TexCoord[2].st);				
		}
		if((stageLightmapBitmaskUniform & (1<<3))>0){
			lightmapStyleAdd += texture2D(3, my_TexCoord[3].st);				
		}
		if((stageLightmapBitmaskUniform & (1<<4))>0){
			lightmapStyleAdd += texture2D(4, my_TexCoord[4].st);				
		}
		if((stageLightmapBitmaskUniform & (1<<5))>0){
			lightmapStyleAdd += texture2D(5, my_TexCoord[5].st);				
		}*/

		addValue *= baseColorForLightingReal; // because if we have a lightmap, we 100% used 1.0 as the baseColorForLighting, so we revert that here.
		baseColorForLighting.x = max(baseColorForLightingReal.x,addValueForLightmap.x);
		baseColorForLighting.y = max(baseColorForLightingReal.y,addValueForLightmap.y);
		baseColorForLighting.z = max(baseColorForLightingReal.z,addValueForLightmap.z);
		addValueForLightmap *= baseColorForLighting;

		if(isWorldBrushUniform > 0 && perlinFuckery > 0){
			if(noiseFuckeryLightmapUniform == 0 && perlinFuckery!=3 && perlinFuckery!=1 || noiseFuckeryLightmapUniform == 2){
				lightmapStyleAdd = vec4(0.0);
			}
			else if(perlinFuckery > 0 && noiseFuckeryLightmapIntensityUniform != 1.0) {
				lightmapStyleAdd.xyz = vec3(0.0)*(1.0-noiseFuckeryLightmapIntensityUniform)+(noiseFuckeryLightmapIntensityUniform*lightmapStyleAdd.xyz);
			}
		}
	}
	
	bool didThermal = false;

	if(vertexLit){
		//gl_FragColor.xyz *= vertexLitMult.xyz;
		//if(twoSided){
		//	gl_FragColor.x = 1.0f;
		//}
		//return;
		vec4 multnew = getVertexLightIntensity(vertexLitMult,eyeSpaceLightdir,lightReferenceNormal,lightNormal,viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,allowDarken);
		vertexLitMult = mix(vertexLitMult,multnew,isSimpleTCGenEnv ? 0.75f:1.0f);
		if(stageColorGenUniform == CGEN_LIGHTING_DIFFUSE || stageForceNormalUniform > 0){ // TODO fix this for flag?
			vertexLitMult.xyz += getVertexLightIntensity(vec4(ambientLight,1.0),lightReferenceNormal,lightReferenceNormal,lightNormal,viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,allowDarken).xyz * MULTDIVIDE255;
		}
		//vertexLitMult = vec4(lightDir*0.5f+vec3(0.5f),1.0f);
		outFragColor.xyz -= boringShadowSubtractVal;
		vertexLitMult.xyz += addValue;
		vertexLitMult.xyz -= boringShadowSubtractValBase*vertexLitMult.xyz;
		if(thermalVision){
			
			heatVision(outFragColor,vertexLitMult.xyz,lightReferenceNormal);
			didThermal= true;
		} else {
			
			addValue *= baseColorForLightingReal;
			outFragColor.xyz += addValue;
			outFragColor.xyz *= vertexLitMult.xyz;
		}
	} else {
	
		outFragColor.xyz *= (stageLightmapBitmaskUniform & 1) > 0 ? lightStyles[0].xyz*MULTDIVIDE255 : vec3(1.0f);
		outFragColor.xyz += (stageLightmapBitmaskUniform & 1) > 0 ? lightmapStyleAdd.xyz : vec3(0.0f);
		outFragColor.xyz -= boringShadowSubtractVal;
		outFragColor.xyz += (stageLightmapBitmaskUniform & 1) > 0 ? addValueForLightmap+lightmapStyleAdd.xyz : addValue;
	}

	vec4 color2 = vec4(0);
	if(multitex){
		if((stageLightmapBitmaskUniform & 2) >0){
			color2 = getLightmapIntensity(haveVertLightDir,1,16,my_TexCoord[1].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<16)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,0,worldPixel);
			if(isWorldBrushUniform > 0 && perlinFuckery > 0){
				if(noiseFuckeryLightmapUniform == 0 && perlinFuckery!=3 && perlinFuckery!=1 || noiseFuckeryLightmapUniform == 2){
					color2 = vec4(1.0);
				}
				else if(perlinFuckery > 0 && noiseFuckeryLightmapIntensityUniform != 1.0) {
					color2.xyz = vec3(1.0)*(1.0-noiseFuckeryLightmapIntensityUniform)+(noiseFuckeryLightmapIntensityUniform*color2.xyz);
				}
			}
		} else{
			color2 = texture2D(text_in[1], my_TexCoord[1].st);
		}
		//color2.xyz *= (stageLightmapBitmaskUniform & 2) > 0 ? lightStyles[0].xyz*MULTDIVIDE255 : vec3(1.0f);
		color2.xyz += (stageLightmapBitmaskUniform & 2) > 0 ? lightmapStyleAdd.xyz : vec3(0.0f);
		color2.xyz -= boringShadowSubtractValBase*color2.xyz;
		color2.xyz += (stageLightmapBitmaskUniform & 2) > 0 ? addValueForLightmap : addValue;
		
		bool doFinalThermal = false;
		bool doNormal = true;
		if(thermalVision){
			if((stageLightmapBitmaskUniform & 2) > 0){
				heatVision(outFragColor,color2.xyz,lightReferenceNormal);
				doNormal = false;
			didThermal= true;
			} else if((stageLightmapBitmaskUniform & 1) > 0){
				vec3 tmpLight = outFragColor.xyz;
				outFragColor.xyz = color2.xyz;
				heatVision(outFragColor,tmpLight,lightReferenceNormal);
				doNormal = false;
			didThermal= true;
			} else {
				doFinalThermal = true;
			}
		}
		if(doNormal){

			switch(multiTexModeUniform){
				case MYGL_ADD:
					outFragColor += color2;
				break;
				case MYGL_MODULATE:
					outFragColor *= color2;
				break;
				case MYGL_REPLACE:
					outFragColor = color2;
				break;
			}
		}
		if(doFinalThermal){
			heatVision(outFragColor,vec3(0.0f),lightReferenceNormal);
			didThermal= true;
		}


	}

	//if(stageLightmapBitmaskUniform > 0){
	//	outFragColor.x = stageLightmapBitmaskUniform;
	//	outFragColor.y = stageLightmapBitmaskUniform-1;
	//}
	//if(vertexLit){
	//	outFragColor.x = 1;
	//}
	
	if(thermalVision && !didThermal){
		heatVision(outFragColor,vec3(0.0f),lightReferenceNormal);
	}

	if((renderFlagsUniform & RENDERFLAG_SCENEVIEWBOUND) > 0 && (renderFlagsUniform & RENDERFLAG_ISGORE) == 0){
		vec3 oriColor = outFragColor.xyz;
		// lightNormal or lightReferenceNormal
		vec3 surfaceNormal = lightReferenceNormal;

		float specIntensitySchlickMultReflective = 0.5f+(1.0-0.5f)*cosviewercomponent*cosviewercomponent*cosviewercomponent*cosviewercomponent*cosviewercomponent;
		{
		
			
			vec3 normalPart = surfaceNormal * dot(surfaceNormal,viewerVectorNorm);
			vec3 viewerVectorMinusNormal = viewerVectorNorm - normalPart;
			vec3 outVec = normalPart - viewerVectorMinusNormal; // the non-normal part gets inverted

			outVec = normalize(outVec);
			vec2 uvRefl = get360UVFromVector(-outVec);

			vec4 thegrad = vec4(dFdx(uvRefl),dFdy(uvRefl));

			// at the 180/-180 boundary, a discontinuity is created, causing a visible seam. fix that up.
			if (abs(thegrad.x) > 0.5) thegrad.x -= sign(thegrad.x);
			if (abs(thegrad.z) > 0.5) thegrad.z -= sign(thegrad.z);
			thegrad *= gradMultiplier;
			outFragColor.xyz = textureGrad(text_in[29],fract(uvRefl),thegrad.xy,thegrad.zw).xyz;
			//outFragColor.xyz = sampleTextureSafe(30, vec2(xAngle,yAngle), thelod,thegrad).xyz;
			//outFragColor.xyz = sampleTextureSafe(30, uvCoords, thelod,thegrad).xyz;
			if(isWorldBrushUniform > 0){
				outFragColor.xyz =oriColor + outFragColor.xyz*max(worldNormal.z,0.0f);
			}
		}

	}

	return true;

}


void simplepath(void){
	bool multitex = (stageImageBitmaskUniform & 2) > 0;
	vec4 outFragColor = texture2D(text_in[0], my_TexCoord[0].st);
	if(multitex){
		
		vec4 color2 = texture2D(text_in[1], my_TexCoord[1].st);
		switch(multiTexModeUniform){
				case MYGL_ADD:
					outFragColor += color2;
				break;
				case MYGL_MODULATE:
					outFragColor *= color2;
				break;
				case MYGL_REPLACE:
					outFragColor = color2;
				break;
			}


	}
	outFragColor *= vertColor;
	gl_FragData[0] = outFragColor;
}



void main(void){
	vec4 outColor =vec4(1.0f);
	bool isInvisible = false;
	if(main_real(outColor, isInvisible)){
		//gl_FragColor.xyz = outColor.xyz;
		
		bool additive = (rawStateBitsUniform & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_ONE && (rawStateBitsUniform & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_ONE; //(rawStateBitsUniform & GLS_SRCBLEND_ONE) > 0 && (rawStateBitsUniform & GLS_DSTBLEND_ONE) > 0;
		bool weirdAdditive = (rawStateBitsUniform & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_ONE && (rawStateBitsUniform & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR; //(rawStateBitsUniform & GLS_SRCBLEND_ONE) > 0 && (rawStateBitsUniform & GLS_DSTBLEND_ONE) > 0;
		bool alphaAdditive = (rawStateBitsUniform & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_SRC_ALPHA && (rawStateBitsUniform & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR; //(rawStateBitsUniform & GLS_SRCBLEND_ONE) > 0 && (rawStateBitsUniform & GLS_DSTBLEND_ONE) > 0;
		bool alphaModulated = (rawStateBitsUniform & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_SRC_ALPHA; //(rawStateBitsUniform & GLS_SRCBLEND_ONE) > 0 && (rawStateBitsUniform & GLS_DSTBLEND_ONE) > 0;
		bool mult1 = (rawStateBitsUniform & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_DST_COLOR;
		bool mult2 = (rawStateBitsUniform & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_SRC_COLOR;
		bool isDecal = (mult1 || mult2);// && alphaFuncUniform > 0;
		
		//if(!additive){
		//	outColor = vec4(0.0f,0.0f,0.0f,1.0f);
		//	return;
		//}

		if(shaderDebugUniform == 1){
			outColor.xyz = vec3(0.05f);
		}
				
		if(myFogUniform != 0.0f && !isInvisible){
			float decalSub = (mult1 && mult2) ? 0.5f : 1.0f;
			float originalIntensity = exp(-myFogUniform*0.001f*length(eyeSpaceCoordsGeom.xyz));
			vec3 mixval = myFogColorUniform;
			vec3 mixvals = vec3(originalIntensity);
			if(isDecal){
				float f = 1.0f-originalIntensity;
				// this is the non-fogged underlying color we assume of the texture below the decal.
				// the more accurate this guess is, the more accurate the fog rendition will be
				// best we can do here is to just use some value that gives decent results.
				vec3 s = vec3(0.1f); 
				float t = mult1 != mult2 ? 1.0f : 2.0f;
				outColor.xyz = (f *mixval - (f - 1.0f) *outColor.xyz* s* t)/(t* (-f*s + f*mixval + s));
			}
			else if(additive || weirdAdditive){
				outColor.xyz -= (1.0f-mixvals)*outColor.xyz;
			} else if(alphaAdditive){
				outColor.w -= outColor.w -  (1.0f-originalIntensity)*outColor.w;
			} else {
				outColor.xyz = mix(mixval,outColor.xyz,mixvals);
			}
		}
		
		if(thermalVisionUniform == 4){
			float intensity = dot(rgbToGray*0.66f,outColor.xyz);
			float threshvalue = intensity > 0.19f ? 0.3f : 0.0f; //  0.877f srgb
			outColor.xyz = vec3(0.0f,intensity,threshvalue);
		} 
		
		//gl_FragColor = outColor;
		gl_FragData[0] = outColor;
		gl_FragData[1].x = length(eyeSpaceCoordsGeom.xyz);
		if((additive && length(outColor.xyz) < 0.1f) || isInvisible || alphaModulated && outColor.w < 0.2f){
			gl_FragData[1].x = uintBitsToFloat(0x7F800000);
			bool renderingSSRBuffer = (renderFlagsUniform & RENDERFLAG_RENDERINGWORLDREFLECT) > 0;
			if(renderingSSRBuffer && additive){
				outColor.xyz = vec3(0.0f);
			}
		}

		gl_FragData[0] = outColor;
	}// else{
	//	gl_FragColor = vec4(0.0f);
	//}
}



