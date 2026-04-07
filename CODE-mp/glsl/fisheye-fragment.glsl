#version 400 compatibility
#define VOXELSTUFF 1
#extension GL_ARB_shader_storage_buffer_object : enable
#extension GL_ARB_shader_group_vote : enable
#if VOXELSTUFF
	#define USE64BITINDEX 0
	#if USE64BITINDEX
		#extension GL_ARB_gpu_shader_int64 : require
	#endif
#endif

#define TEXTURE_COUNT 14

#define TEXTUREGRAD 1

#define PERLINFVCKERY 1

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
#define FLOATSERVERTIME ((float(serverTimeUniform)+serverTimeFractionUniform)*1000.0f)



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


#if VOXELSTUFF
precision highp int;
#endif

#define NUM_GLSL_EXTRA_LIGHTMAPS_MAX 14

uniform sampler2D text_in0;
uniform sampler2D text_in1;
uniform sampler2D text_in2;
uniform sampler2D text_in3;
uniform sampler2D text_in4;
uniform sampler2D text_in5;
uniform sampler2D text_in6;
uniform sampler2D text_in7;
uniform sampler2D text_in8;
uniform sampler2D text_in9;
uniform sampler2D text_in10;
uniform sampler2D text_in11;
uniform sampler2D text_in12;
uniform sampler2D text_in13;
uniform sampler2D text_in14;
uniform sampler2D text_in15;
uniform sampler2D text_in16;
uniform sampler2D text_in17;
uniform sampler2D text_in18;
uniform sampler2D text_in19;
uniform sampler2D text_in20;
uniform sampler2D text_in21;
uniform sampler2D text_in22;
uniform sampler2D text_in23;
uniform sampler2D text_in24;
uniform sampler2D text_in25;
uniform sampler2D text_in26;
uniform sampler2D text_in27;
uniform sampler2D text_in28;
uniform sampler2D text_in29;
uniform sampler2D text_in30;
uniform sampler2D text_in31;


vec4 sampleTextureSafe(sampler2D sampler,vec2 uvCoords,float thelod,vec4 thegrad){
#if TEXTUREGRAD
	return textureGrad(sampler,uvCoords,thegrad.xy,thegrad.zw);
#else
	return textureLod(sampler,uvCoords,thelod);
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

#if VOXELSTUFF
layout(std430, binding = 5) buffer voxelBitGridLayout
{
    uint voxelBitGrid[]; 
    //uvec4 voxelBitGrid[];   
};

#define VOXELGRIDSTEPSIZE 10

#if USE64BITINDEX
#define VOXELGRIDRANGE 1024L
#define VOXELGRIDEDGESIZE (((VOXELGRIDRANGE*2L+1L)/8L+1L)*8L) // +1 for 0. divide by 8, add 1, multiply by 8, to align sides and make them divisible by 8. for optimization and avoiding 64bit ints
#define VOXELINDEX(x,y,z) (VOXELGRIDEDGESIZE*((int64_t(x))*VOXELGRIDEDGESIZE + (int64_t(y))) + (int64_t(z)))
#else
#define VOXELGRIDRANGE 1024
#define VOXELGRIDEDGESIZE (((VOXELGRIDRANGE*2+1)/8+1)*8) // +1 for 0
const uint EDGE8TH = VOXELGRIDEDGESIZE/8;
#define VOXELINDEX(x,y,z) (EDGE8TH*((x)*VOXELGRIDEDGESIZE + (y)) + ((z)>>3)) // trick to predivide, and then the number stays smaller
#endif

const ivec3 rangeadd = ivec3(VOXELGRIDRANGE,VOXELGRIDRANGE,VOXELGRIDRANGE);

int voxelSolid(ivec3 pos){

#if USE64BITINDEX
	int64_t voxIndex = VOXELINDEX(pos.x,pos.y,pos.z);
	int64_t voxArrayOffset = voxIndex/32L;
	if(voxArrayOffset >= voxelBitGrid.length() || voxArrayOffset < 0) return -1;
	int64_t voxBit = 1<<(voxIndex & 31L);

	return (voxelBitGrid[uint(voxArrayOffset)] & uint(voxBit)) > 0 ? 1 : 0;
#else
	uint voxIndex = VOXELINDEX(pos.x,pos.y,pos.z);
	uint voxArrayOffset = voxIndex>>2;
	if(voxArrayOffset >= voxelBitGrid.length() || voxArrayOffset < 0) return -1;
	return int((voxelBitGrid[voxArrayOffset] >> ((pos.z & 7) + ((voxIndex & 3)<<3)) ) & 1);
#endif
}



const int RAYSTEPS = 64;

// based on "Branchless Voxel Raycasting" shadertoy by fb39ca4: https://www.shadertoy.com/view/4dX3zl
// gotta make this separate because recursion is not supported
// mode 0: search until free found, then return last solid
// mode 1: just do full count of steps
bool traceVoxelReverse(vec3 pos, vec3 end, int mode, int maxSteps, inout bvec3 collisions,  out ivec3 endpos){
	
	pos /= float(VOXELGRIDSTEPSIZE);
	end /= float(VOXELGRIDSTEPSIZE);
	pos += rangeadd;
	end += rangeadd;
	vec3 dir = end-pos;
	
	ivec3 voxpos = ivec3(floor(pos + 0.));
	ivec3 voxposend = ivec3(floor(end + 0.));	

	vec3 dist = abs(vec3(length(dir)) / dir);
	
    vec3 vsign = sign(dir);
	ivec3 isign = ivec3(vsign);

	vec3 side = 
    (
    vsign * ( vec3(voxpos) - pos)
    + (vsign * 0.5) 
    + 0.5 
    ) 
    * dist; 
	
    bool foundfree = false;

	for (int i = 0; i < maxSteps; i++) {
		bool found = voxelSolid(voxpos) == 1;
		if (!found && mode == 0) {
			foundfree = true;
			break;
		}

        collisions = lessThanEqual(side.xyz, min(side.yzx, side.zxy));	
			
		side += vec3(collisions) * dist;
		voxpos += ivec3(vec3(collisions)) * isign;
	}

	endpos = voxpos;
	
	return foundfree;
}


// based on "Branchless Voxel Raycasting" shadertoy by fb39ca4: https://www.shadertoy.com/view/4dX3zl
bool traceVoxel(vec3 pos, vec3 end, inout bvec3 collisions){
	if(voxelBitGrid.length()<10 || dLightVoxelShadowsUniform < 1) return false;

	
	// do a reverse search to find where the target surface reaches "air", to check against hitting that (cuz else we think we hit a wall before the target, but we really didn't)
	//ivec3 voxposend2 = ivec3(0);
	//traceVoxelReverse(end,pos,1,1,collisions,voxposend2); // just trace 1 step backwards. avoid lil microshadows

	pos /= float(VOXELGRIDSTEPSIZE);
	end /= float(VOXELGRIDSTEPSIZE);
	pos += rangeadd;
	end += rangeadd;
	vec3 dir = end-pos;

	//pos += 1.0;
	//end += 1.0;
	
	ivec3 voxpos = ivec3(floor(pos + 0.));
	ivec3 voxposend = ivec3(floor(end + 0.));

	vec3 dist = abs(vec3(length(dir)) / dir);
	
    vec3 vsign = sign(dir);
	ivec3 isign = ivec3(vsign);

	vec3 side = 
    (
    vsign * ( vec3(voxpos) - pos)
    + (vsign * 0.5) 
    + 0.5 
    ) 
    * dist; 
	
    bool foundany = false;
	//bool sawEmpty = false;
	bool found = false;

	for (int i = 0; i < RAYSTEPS; i++) {
		bool found = voxelSolid(voxpos) == 1;
		ivec3 enddist = voxpos-voxposend;
		bool closeToEnd = dot(enddist,enddist) <= 2;
		if (found /* && sawEmpty*/ || closeToEnd){// || voxpos == voxposend || voxpos == voxposend2) {
			foundany=found && !closeToEnd;// && voxpos != voxposend && voxpos != voxposend2;
			break;
		}
		//sawEmpty = sawEmpty || !found;

        collisions = lessThanEqual(side.xyz, min(side.yzx, side.zxy));	
			
		side += vec3(collisions) * dist;
		voxpos += ivec3(vec3(collisions)) * isign;
	}

	
	return foundany;
}

const vec3 upaxis = {0.0,0.0,1.0};
vec3 transformDLightForVoxelShadow(vec3 dlight, vec3 target){
	if(dLightVoxelShadowJitterMethodUniform == 2){
		vec3 dir = normalize(target-dlight);
		vec3 side = cross(dir,upaxis);
		vec3 up = cross(dir,side);
		return dlight + side*dLightVoxelShadowJitterUniform.x + up*dLightVoxelShadowJitterUniform.y;
	} else{
		return dlight + dLightVoxelShadowJitterUniform;
	}
}

#endif

vec2 parallaxMap(float thelod,vec4 thegrad, vec2 rawUV){
		vec2 uvCoords;
		//uvCoords.s = dot(eyeSpaceCoordsGeom.xyz,texUVTransform[0]);
		//uvCoords.t = dot(eyeSpaceCoordsGeom.xyz,texUVTransform[1]);
		//vec4 color = texture2D(text_in0, my_TexCoord[0].st);
		vec4 color = sampleTextureSafe(text_in0, rawUV, thelod,thegrad);
		//vec4 color = texture2D(text_in, uvCoords);
		float offset = 1.0f - max(min((color.x + color.y + color.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f);

		vec3 offset3d =  normalize(eyeSpaceCoordsGeom.xyz)*parallaxMapDepthUniform * offset;
		offset3d -= normal * dot(normal,offset3d); // project onto surface aka get rid of any 3d component that aligns with the normal of the surface

		vec3 transposedCoords = eyeSpaceCoordsGeom.xyz + offset3d;
		
		//uvCoords.s = mod(dot(transposedCoords,texUVTransform[0]),1);
		//uvCoords.t = mod(dot(transposedCoords,texUVTransform[1]),1);
		uvCoords.s = dot(transposedCoords,texUVTransform[0]);
		uvCoords.t = dot(transposedCoords,texUVTransform[1]);
		return uvCoords;
}
vec2 parallaxMapSteep(inout vec3 finalPosition, float thelod, vec4 thegrad){
		int layers = parallaxMapLayersUniform;
		vec2 uvCoords;

		float layerDepth = parallaxMapDepthUniform / float(layers);
		vec3 currentPlace = finalPosition;//eyeSpaceCoordsGeom.xyz;
		float gamma = 1.0f/parallaxMapGammaUniform;

		//vec4 color = texture2D(text_in, my_TexCoord[0].st);

		vec3 viewVecNormalized = normalize(eyeSpaceCoordsGeom.xyz);
		vec3 depthComponent = normal * dot(normal,viewVecNormalized); // Get the depth component that a unity view vector gives us 
		vec3 viewVecFlat = viewVecNormalized - depthComponent;
		float viewVecMultiplier = layerDepth/max(0.001f,length(depthComponent)); // Calculate how much we have to multiple the unity view vector with to go one layer deeper.
		vec3 oneLayerProgressVec = viewVecFlat*viewVecMultiplier;

		if(dot(oneLayerProgressVec,oneLayerProgressVec) > layerDepth*layerDepth){
			// in a distance aat flat angles, the progress vec becomes too big and we get ugly artifacts. rather limit the depth a bit than to have huge jumps over texture coordinates
			oneLayerProgressVec = normalize(oneLayerProgressVec)*layerDepth;
		}

		//uvCoords.s = mod(dot(currentPlace,texUVTransform[0]),1.0);
		//uvCoords.t = mod(dot(currentPlace,texUVTransform[1]),1.0);
		uvCoords.s = dot(currentPlace,texUVTransform[0]);
		uvCoords.t = dot(currentPlace,texUVTransform[1]);


		//uvCoords = fract(uvCoords);

		float oldtexDepth = 0.0;
		float texDepth = 0.0f;
		for(int i=0; i< layers;i++){
			
			//uvCoords = fract(uvCoords);
			//uvCoords = fract(uvCoords);
			//vec4 color = texture2D(text_in0, uvCoords);
			vec4 color = sampleTextureSafe(text_in0, uvCoords,thelod,thegrad);
			oldtexDepth = texDepth;
			texDepth = parallaxMapDepthUniform*(pow(max(min((color.x + color.y + color.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f),gamma)-1.0f);
			
			float newLayerDepth = -(layerDepth * float(i));
			if(texDepth > newLayerDepth){
				if(i > 0){
					//float oldLayerDepth = -(layerDepth * float(i-1));
					//float weight = (texDepth-newLayerDepth)/(oldLayerDepth-newLayerDepth-oldtexDepth+texDepth);
					float weight = (texDepth-newLayerDepth)/max(0.01f,layerDepth-oldtexDepth+texDepth);
					
					//float a = texDepth-(-(layerDepth * float(i)));
					//float b =  -(layerDepth * float(i-1)) - oldtexDepth;
					//float weight = b/(a+b);
					
					vec3 newPlace = currentPlace + oneLayerProgressVec;
					vec2 newUv;
					newUv.s =  dot(newPlace,texUVTransform[0]);
					newUv.t =  dot(newPlace,texUVTransform[1]);
					currentPlace = newPlace*(1.0-weight) + currentPlace*weight;
					uvCoords = newUv*(1.0-weight)+uvCoords*weight;
				}
				break;
			} else {
				currentPlace += oneLayerProgressVec;
				//uvCoords.s = mod(dot(currentPlace,texUVTransform[0]),1);
				//uvCoords.t = mod(dot(currentPlace,texUVTransform[1]),1);
				uvCoords.s = dot(currentPlace,texUVTransform[0]);
				uvCoords.t = dot(currentPlace,texUVTransform[1]);
			}
			if(texDepth == newLayerDepth){
				break;
			}
		}

		finalPosition = currentPlace;

		//float offset = 1.0f - max(min((color.x + color.y + color.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f);


		//float offset = 1.0f - max(min((color.x + color.y + color.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f);

		//vec3 offset3d =  normalize(eyeSpaceCoordsGeom.xyz)*parallaxMapDepthUniform * offset;
		//float depthHere = dot(normal,offset3d);
		//vec3 depthComponent = normal * depthHere;
		//offset3d -= depthComponent; // project onto surface aka get rid of any 3d component that aligns with the normal of the surface

		//vec3 transposedCoords = eyeSpaceCoordsGeom.xyz + offset3d;
		
		return uvCoords;
}
#ifdef PERLINFUCKERY
vec3 perlinNoiseVariation1(){ // Looks a bit like marble?
	vec3 res;
	vec4 coords = pureVertexCoordsGeom/2.0;
	float timeVal =  FLOATSERVERTIME*2.5;
    coords.w = timeVal;
	res.xyz = vec3( snoise(coords/10.0f));
	res.xyz += vec3( snoise(coords/20.0f));
	res.xyz += vec3( snoise(coords/40.0f));
	res.xyz += vec3( snoise(coords/80.0f));
	res.xyz += vec3( snoise(coords/160.0f));
	res.xyz += vec3( snoise(coords/320.0f));
	res.xyz += vec3( snoise(coords/640.0f));
	res.xyz += vec3( snoise(coords/1280.0f));
	//gl_FragColor.xyz += 8.0f;
	//gl_FragColor.xyz /= 16.0f;
	res.xyz = abs(res.xyz);
	res.xyz /= 4.0f;
	return res;
}
vec3 perlinNoiseVariation2(){ 
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

float perlinNoiseHelper(vec4 coords){
    float val;
    coords.w = FLOATSERVERTIME*10.0;
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
float perlinNoiseHelper2(vec4 coords){
    float val;
	float timeVal =  FLOATSERVERTIME*100.0;
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


vec3 perlinNoiseVariation3(){ 
	vec3 res;
    vec3 distort = vec3(perlinNoiseHelper(pureVertexCoordsGeom),perlinNoiseHelper(pureVertexCoordsGeom+vec4(40.3,3.4,100.5,1.0)),perlinNoiseHelper(pureVertexCoordsGeom+vec4(10.1,1.4,101.5,1.0)));
    vec3 distort2 = vec3(perlinNoiseHelper(pureVertexCoordsGeom+30.0*vec4(distort,1.0)),perlinNoiseHelper(pureVertexCoordsGeom+30.0*vec4(distort,1.0)+vec4(15.3,13.4,110.3,1.0)),perlinNoiseHelper(pureVertexCoordsGeom+30.0*vec4(distort,1.0)+vec4(13.1,11.4,151.5,1.0)));
    float finalVal = perlinNoiseHelper(pureVertexCoordsGeom+100.0*vec4(distort2,1.0));

	//finalVal*=5.0;
    //res =vec3(finalVal);
    /*if(finalVal > 0.9){
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
    }*/
	if(finalVal > 0.8){
        res = vec3(0.0,0.0,0.05);
    } else if(finalVal > 0.75){
        res = vec3(0.0,0.0,0.0);
    } else if(finalVal > 0.72){
        res = vec3(0.65,0.5,0.0);
    } else if(finalVal > 0.7){
        res = vec3(0.0,0.25,0.0);
    } else if(finalVal > 0.67){
        res = vec3(0.85,0.0,0.0);
    } 
    else if(finalVal > 0.667){
        res = vec3(100.0,0.0,0.0);
    } 
    else if(finalVal > 0.5){
        res = vec3(0.0,0.05,0.0);
    } else if(finalVal > 0.4){
        res = vec3(0.0,0.03,0.0);
    } else if(finalVal > 0.3){
        res = vec3(0.0,0.0,0.0);
    } else if(finalVal > 0.28){
        res = vec3(1.2,0.65,0.0);
    }else if(finalVal > 0.1){
        res = vec3(0.0,0.0,0.0);
    }else{
        
        res = vec3(0.0,0.3,0.0);
    }
	res.x = max(0.0,pow(res.x,2.4));
	res.y = max(0.0,pow(res.y,2.4));
	res.z = max(0.0,pow(res.z,2.4));
	return res;
}
vec3 perlinNoiseVariation4(){ 
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
vec3 perlinNoiseVariation5(vec4 coords){ 
	vec3 res;
	float val,val2;
	float timeVal =  FLOATSERVERTIME*2.5;
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
    val = pow(val ,2000.0+dist*2.0);
    val2 = pow(val2 ,40.0);
	//res.xyz = vec3(1.0)-res.xyz;
    res.xyz = vec3(val*10.0)+vec3(val2*0.5)*vec3(val2*0.5,val2*0.7,1.0);
	res.x = max(0.0,pow(res.x,2.4));
	res.y = max(0.0,pow(res.y,2.4));
	res.z = max(0.0,pow(res.z,2.4));
	return res;
}
vec3 perlinNoiseVariation6(vec4 coords){ 
	vec3 res;
	float val,val2;
	float timeVal =  FLOATSERVERTIME*2.5;
    coords.w = timeVal/16384.0;
	val = abs( snoise(coords/0.078125))/16384.0;
    coords.w = timeVal/8192.0;
	val += abs( snoise(coords/0.15625))/8192.0;
    coords.w = timeVal/4096.0;
	val += abs( snoise(coords/0.3125))/4096.0;
    coords.w = timeVal/2048.0;
	val += abs( snoise(coords/0.625))/2048.0;
    coords.w = timeVal/1024.0;
	val += abs( snoise(coords/1.25))/1024.0;
    coords.w = timeVal/512.0;
	val += abs( snoise(coords/2.5))/512.0;
    coords.w = timeVal/256.0;
	val += abs( snoise(coords/5.0))/256.0;
    coords.w = timeVal/128.0;
	val += abs( snoise(coords/10.0))/128.0;
    coords.w = timeVal/64.0;
	val += abs( snoise(coords/20.0))/64.0;
    coords.w = timeVal/32.0;
	val += abs( snoise(coords/40.0))/32.0;
    coords.w = timeVal/16.0;
	val += abs( snoise(coords/80.0))/16.0;
    coords.w = timeVal/8.0;
	val += abs( snoise(coords/160.0))/8.0;
    coords.w = timeVal/4.0;
	val += abs( snoise(coords/320.0))/4.0;
    coords.w = timeVal/2.0;
	val += abs( snoise(coords/640.0))/2.0;
    coords.w = timeVal;
	val += abs( snoise(coords/1280.0));
	//gl_FragColor.xyz += 8.0f;
	//gl_FragColor.xyz /= 16.0f;
	//res.xyz = abs(res.xyz);
	//res.xyz += 1.0;
	val *= 0.5;
	res = vec3(val);
	res.x = max(0.0,pow(res.x,2.4));
	res.y = max(0.0,pow(res.y,2.4));
	res.z = max(0.0,pow(res.z,2.4));
	return res;
}
vec3 perlinNoiseVariation6Stack(vec4 coords,vec3 vieworg){ 
	vec3 res;
	float val,val2;
	float timeVal =  FLOATSERVERTIME*200.0;
	const int layers = 10;

	float viewdist = distance(coords.xyz,vieworg);
	//return vec3(viewdist)/10000.0;;
	vec3 viewVec = coords.xyz-vieworg;
	vec3 viewVecPiece = viewVec/float(layers);
	coords.xyz = vieworg+viewVecPiece;
	val = 0;
	for(int i=0;i<layers;i++){
		coords.w = timeVal/16384.0;
		val += abs( snoise(coords/0.078125))/16384.0;
		coords.w = timeVal/8192.0;
		val += abs( snoise(coords/0.15625))/8192.0;
		coords.w = timeVal/4096.0;
		val += abs( snoise(coords/0.3125))/4096.0;
		coords.w = timeVal/2048.0;
		val += abs( snoise(coords/0.625))/2048.0;
		coords.w = timeVal/1024.0;
		val += abs( snoise(coords/1.25))/1024.0;
		coords.w = timeVal/512.0;
		val += abs( snoise(coords/2.5))/512.0;
		coords.w = timeVal/256.0;
		val += abs( snoise(coords/5.0))/256.0;
		coords.w = timeVal/128.0;
		val += abs( snoise(coords/10.0))/128.0;
		coords.w = timeVal/64.0;
		val += abs( snoise(coords/20.0))/64.0;
		coords.w = timeVal/32.0;
		val += abs( snoise(coords/40.0))/32.0;
		coords.w = timeVal/16.0;
		val += abs( snoise(coords/80.0))/16.0;
		coords.w = timeVal/8.0;
		val += abs( snoise(coords/160.0))/8.0;
		coords.w = timeVal/4.0;
		val += abs( snoise(coords/320.0))/4.0;
		coords.w = timeVal/2.0;
		val += abs( snoise(coords/640.0))/2.0;
		coords.w = timeVal;
		val += abs( snoise(coords/1280.0));
		coords.xyz += viewVecPiece;
	}

    
	val /= float(layers);
	val *= viewdist/10000.0;

	res = vec3(val);
	res.x = max(0.0,pow(res.x,2.4));
	res.y = max(0.0,pow(res.y,2.4));
	res.z = max(0.0,pow(res.z,2.4));
	return res;
}
#endif


// direction mustt be in eye space and normalized
vec4 getVertexLightIntensity(vec4 color, vec3 direction, vec3 referenceNormal, vec3 lightNormal, vec3 viewerVectorNorm, float specIntensitySchlickMult,float viewerDistance, bool twoSided){
	
	//return vec4(1.0f);
	vec3 maybeMirroredLightNormal =  twoSided && dot(referenceNormal,direction) < 0 ? -lightNormal : lightNormal;
	if(dot(direction,referenceNormal)<0 && !twoSided) return vec4(0.0f);
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

			color.xyz *= (1.0f-ambientFactor)*alignment*alignment*alignment+specIntensityTotal + ambientFactor;
			//color.xyz = vec3(totalDist);
			//color *= alignment*alignment*alignment+specIntensityTotal;
			//color *= 100.0f;

		//}
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

vec4 getLightmapIntensity(sampler2D sampler, sampler2D deluxeSampler, vec2 lmtexcoord, vec3 eyespacelightdir, bool havedeluxe, vec3 lightNormal, vec3 lightReferenceNormal, mat4 dirmat, vec3 viewerVectorNorm, float specIntensitySchlickMult,float viewerDistance, bool twoSided, int style, vec3 worldPixel){
	vec4 color;
	vec4 direction = vec4(1.0f);
	bool haveDir = false;
	//if((stageLightmapBitmaskUniform & (1<<2))>0)
	{
		//return vec4(-vertexNormal,1.0f)*0.1f;
		//vec2 thelod = textureQueryLod(sampler,lmtexcoord);
		color = texture2D(sampler, lmtexcoord);
		//return vec4(1.0f);
		//return color;
		if(havedeluxe || haveVertexLightDirectionUniform > 0){
			if(havedeluxe){
				direction = texture2D(deluxeSampler, lmtexcoord); // visualize n
				//float baseMultiplier = 1.0f / max(0.00001,dot(normal,(direction).xyz));
				//return direction;
				direction = (dirmat*direction);
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
			vec3 mult = texture2D(text_in29,uv).xyz;
			vec3 multBlur = textureLod(text_in29,uv,4.0f).xyz;

			vec4 worldDirection = normalize(worldModelViewMatrixReverseGeom*vec4(( haveDir? direction.xyz : lightReferenceNormal.xyz),0.0f));
			float weight =  clamp(dot(sundir,worldDirection.xyz)*1.0f,0.0f,1.0f);
			color.xyz *= powVec(((1.0f-weight)*multBlur) + weight*mult,cloudPowerUniform)*cloudIntensityCompensateUniform;
		}
		
	}
	return color;
}

vec3 calculateTextureNormal(vec2 uvCoords, vec3 startPosition, vec3 referenceNormal, float thelod, vec4 thegrad){
		
		//uvCoords.s = dot(eyeSpaceCoordsGeom.xyz,texUVTransform[0]);
		//uvCoords.t = dot(eyeSpaceCoordsGeom.xyz,texUVTransform[1]);
		//vec4 color = texture2D(text_in0, uvCoords);
		vec4 color = sampleTextureSafe(text_in0, uvCoords, thelod,thegrad);
		//vec4 color = texture2D(text_in, uvCoords);
		float offset = 1.0f - max(min((color.x + color.y + color.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f);

		//vec3 offset3d =  normalize(startPosition);
		//vec3 normalComponent = referenceNormal * dot(referenceNormal,offset3d);
		//offset3d -= normalComponent; // project onto surface aka get rid of any 3d component that aligns with the normal of the surface
		//if(dot(offset3d,offset3d) == 0){
		//	return referenceNormal;
		//}
		//offset3d = normalize(offset3d)*0.01f;
		vec3 notparallelaxis = (abs(referenceNormal.x) > 0.99) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
		vec3 offset3d = normalize(cross(notparallelaxis,referenceNormal))*0.01f;

		vec3 transposedCoords = startPosition + offset3d;
		uvCoords.s = dot(transposedCoords,texUVTransform[0]);
		uvCoords.t = dot(transposedCoords,texUVTransform[1]);
		//vec4 color2 = texture2D(text_in0, uvCoords);
		vec4 color2 = sampleTextureSafe(text_in0, uvCoords, thelod,thegrad);
		float offset2 = 1.0f - max(min((color2.x + color2.y + color2.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f);

		vec3 transposedCoords2 = startPosition + normalize(cross(offset3d,referenceNormal))*0.1;
		uvCoords.s = dot(transposedCoords2,texUVTransform[0]);
		uvCoords.t = dot(transposedCoords2,texUVTransform[1]);
		//vec4 color3 = texture2D(text_in0, uvCoords);
		vec4 color3 = sampleTextureSafe(text_in0, uvCoords, thelod,thegrad);
		float offset3 = 1.0f - max(min((color3.x + color3.y + color3.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f);

		vec3 place1 = startPosition - referenceNormal * offset;
		vec3 place2 = transposedCoords - referenceNormal * offset2;
		vec3 place3 = transposedCoords2 - referenceNormal * offset3;

		//vec3 crossed = cross(place2-place1,place3-place1);
		//return dot(crossed,crossed) > 0? -normalize(crossed) : vec3(0.0f);
		return -normalize(cross(place2-place1,place3-place1));
}


float distanceToLine(vec3 point, vec3 linePoint1, vec3 linePoint2){
	vec3 P1toPoint = linePoint2 - point;
	vec3 P1ToP2 = linePoint2 - linePoint1;

	return length(cross(P1toPoint, P1ToP2)) / length(P1ToP2);
}

float distanceToLineProper(vec3 point, vec3 linePoint1, vec3 linePoint2){
  vec3 P1toPoint = linePoint2 - point;
  vec3 P1ToP2 = linePoint2 - linePoint1;
  
  float dot1 = dot(point - linePoint1,linePoint2 - linePoint1);
  float dot2 = dot(point - linePoint2,linePoint1 - linePoint2);
  if(dot1> 0.0 && dot2 > 0.0){
  
    return length(cross(P1toPoint, P1ToP2)) / length(P1ToP2);
  } else if (dot1 > 0.0){
    return distance(linePoint2,point);
  } else if (dot2 > 0.0){
    return distance(linePoint1,point);
  } else {
    return 0.0;// shouldnt happen?
  }
}

float distanceToLineProperMaybefast(vec3 point, vec3 linePoint1, vec3 linePoint2) {
    vec3 thing1 = linePoint2 - linePoint1;
    vec3 thing2 = point - linePoint1;
    float ratio = clamp(dot(thing2, thing1) / dot(thing1, thing1), 0.0, 1.0);
    vec3 near = linePoint1 + ratio * thing1;
    return length(point - near);
}

float distanceToLineProperMaybefastSquared(vec3 point, vec3 linePoint1, vec3 linePoint2) {
    vec3 thing1 = linePoint2 - linePoint1;
    vec3 thing2 = point - linePoint1;
    float ratio = clamp(dot(thing2, thing1) / dot(thing1, thing1), 0.0, 1.0);
    vec3 near = linePoint1 + ratio * thing1;
	vec3 pointto = point-near;
    return dot(pointto,pointto);
}
float distanceToLineProperMaybefastSquaredInfinite(vec3 point, vec3 linePoint1, vec3 linePoint2) {
    vec3 thing1 = linePoint2 - linePoint1;
    vec3 thing2 = point - linePoint1;
    float ratio = dot(thing2, thing1) / dot(thing1, thing1);
    vec3 near = linePoint1 + ratio * thing1;
	vec3 pointto = point-near;
    return dot(pointto,pointto);
}

float shortestDistanceLinesSquared( vec3 a0, vec3 a1, vec3 b0, vec3 b1,inout int type, float quitThreshold) {
    vec3 u = a1 - a0;
    vec3 v = b1 - b0;
    vec3 w = a0 - b0;
	float d,e;

    float a = dot(u, u);
    float b = dot(u, v);
    float c = dot(v, v);
    float denom = a * c - b * b;

    d = dot(u, w);
    e = dot(v, w);

    float s = 0.0f;
    float t = 0.0f;

    if (denom != 0.0f) {
        s = clamp((b * e - c * d) / denom, 0.0f, 1.0f);
    }

    if (c != 0.0f) {
        t = clamp((b * s + e) / c, 0.0f, 1.0f);
    }

    if (a != 0.0f) {
        s = clamp((b * t - d) / a, 0.0f, 1.0f);
    }

    //vec3 closestPoint1 = a0 + s * u;
    //vec3 closestPoint2 = b0 + t * v;
	
	vec3 shortestvec = w + s * u -  t * v;
	return dot(shortestvec,shortestvec);
    //return length(closestPoint1 - closestPoint2);
}

float shortestDistanceLinesSquaredOld(vec3 a0,vec3 a1, vec3 b0, vec3 b1,inout int type, float quitThreshold){

  vec3 a=normalize(a1-a0);
	vec3 b=normalize(b1-b0);
	vec3 crossBoth = normalize(cross(b,a));
	float distanceInfinite = abs(dot(crossBoth,a1-b1));

	if(quitThreshold < distanceInfinite){ // attempt to exit early when possible
		return quitThreshold*quitThreshold;
	}

	vec3 perp1 = normalize(cross(crossBoth,a));
	vec3 perp2 = normalize(cross(crossBoth,b));

	float dot1_1 = dot(perp2,b0-a0);
	float dot1_2 = dot(perp2,b0-a1);
	float dot2_1 = dot(perp1,a0-b0);
	float dot2_2 = dot(perp1,a0-b1);

	float maxDistance = 1.0;

	if(sign(dot1_1) != sign(dot1_2) && sign(dot2_1) != sign(dot2_2)){
		// from the perspective of the cross vector, the lines overlap. We can calculate simple infinite distance
		 
		maxDistance = distanceInfinite;
		type = 0;
	} else if(sign(dot1_1) != sign(dot1_2)){
		vec3 pointToMeasure = abs(dot2_1) < abs(dot2_2) ? b0 : b1;
		maxDistance = distanceToLineProper(pointToMeasure,a0,a1);
		type = 1;
	} else if(sign(dot2_1) != sign(dot2_2)){ 
		vec3 pointToMeasure = abs(dot1_1) < abs(dot1_2) ? a0 : a1;
		maxDistance = distanceToLineProper(pointToMeasure,b0,b1);
		type = 2;
	} else {
		// point to point distance
		float dist1 = distanceToLineProper(b0,a0,a1);
		float dist2 = distanceToLineProper(b1,a0,a1);
		maxDistance = min(dist1,dist2);
		dist1 = distanceToLineProper(a0,b0,b1);
		dist2 = distanceToLineProper(a1,b0,b1);
		maxDistance = min(maxDistance,min(dist1,dist2));
		type = 3;
	}
	return maxDistance*maxDistance;
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
	//bool test[500];
    //const float depth = 5.0f;
#ifdef PERLINFUCKERY
	int perlinFuckery = noiseFuckeryUniform;
#else 
	int perlinFuckery = 0;
#endif

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

	vec2 rawUVCoords = my_TexCoord[0].st;
	
	vec3 lightReferenceNormal = (isModelUniform > 0 || stageForceNormalUniform > 0 || surfaceTypeUniform == SF_GRID) ? normalize(mat3(gl_ModelViewMatrix)*normalize(vertexNormal)) : normal; // can be normal instead. trying vertexnormal so things are smoother
	vec3 worldPixel = (worldModelViewMatrixReverseGeom*eyeSpaceCoordsGeom).xyz;
	vec3 viewerVector = -eyeSpaceCoordsGeom.xyz;
	bool doingGigaEnvTCGen = false;
	bool isSimpleTCGenEnv = stageTCGenUniform == TCGEN_ENVIRONMENT_MAPPED && stageHasTCModUniform == 0;
	if( isSimpleTCGenEnv && gigaTCGenUniform > 0){
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
	
	bool ssr = (renderFlagsUniform & RENDERFLAG_SCENEVIEWWORLDREFLECTBOUND) > 0;
	bool renderingSSRBuffer = (renderFlagsUniform & RENDERFLAG_RENDERINGWORLDREFLECT) > 0;
	bool vertexLit = (lightDir[0] != 0.0f || lightDir[1] != 0.0f || lightDir[2] != 0.0f) && haveVertexLightDirectionUniform > 0 && stageLightmapBitmaskUniform == 0;
	
	float thelod = textureQueryLod(text_in0,uvCoords).x;
	thelod = thelod - biaslod(thelod);
	float gradMultiplier = jitterTotalFramesUniform == 0 ? 0.5f : 1.0f / sqrt(float(jitterTotalFramesUniform)/3.0f);
	//if(thermalVisionUniform > 0){
	//	gradMultiplier*=4.0f;
	//}
	float gradnoise = clamp(1.15f*2.0f*gaussian_rand(uvCoords),1.0f,1.3f); // try to smooth out the transition between levels of detail, as it forms a straight line thats visible on high frequency textures even with anisotropic filtering
	vec4 rawgrad = vec4(dFdx(uvCoords),dFdy(uvCoords));
	vec4 thegrad = rawgrad * gradMultiplier * gradnoise;
	vec4 thegradWorldReflect = rawgrad * worldReflectGradMultUniform * gradnoise;
	//textureGrad(text_in0,uvCoords,thegrad.xy,thegrad.zw);

	if(ssr && ssrMultiSampleCount > 1){
		vec3 effectiveUVPixelStep[2] = {dFdx(effectiveUVPixelPos),dFdy(effectiveUVPixelPos)};
		vec2 baseuv = uvCoords-0.5f*rawgrad.xy-0.5f*rawgrad.zw;
		vec4 uvstep = rawgrad / float(ssrMultiSamples);
		vec3 baseUVpixel = effectiveUVPixelPos - 0.5f*effectiveUVPixelStep[0]- 0.5f*effectiveUVPixelStep[1];
		effectiveUVPixelStep[0] = effectiveUVPixelStep[0] / float(ssrMultiSamples);
		effectiveUVPixelStep[1] = effectiveUVPixelStep[1] / float(ssrMultiSamples);
		for(int x=0;x<ssrMultiSamples;x++){
			for(int y=0;y<ssrMultiSamples;y++){
				uvCoordsWorldReflect[y*ssrMultiSamples+x] = baseuv + uvstep.xy*float(x) + uvstep.zw*float(y);
				effectiveUVPixelPosWorldReflect[y*ssrMultiSamples+x] = baseUVpixel + effectiveUVPixelStep[0]*float(x) + effectiveUVPixelStep[1]*float(y);
			}
		}
	}

    if(fishEyeModeUniform == 0){
	
		if(!doingGigaEnvTCGen && !standAloneLightmap && perlinFuckery == 0 && isWorldBrushUniform > 0 && (renderFlagsUniform & RENDERFLAG_SIMPLELIGHTING) == 0 && (renderFlagsUniform & RENDERFLAG_NOLIGHTING) == 0){
			uvCoords = parallaxMapLayersUniform < 2 ? parallaxMap(thelod,thegrad,rawUVCoords):parallaxMapSteep(effectiveUVPixelPos,thelod,thegrad);
			if(ssr){
				for(int i=0;i<ssrMultiSampleCount;i++){
					uvCoordsWorldReflect[i] = parallaxMapLayersUniform < 2 ? parallaxMap(thelod,thegradWorldReflect,rawUVCoords):parallaxMapSteep(effectiveUVPixelPosWorldReflect[i],thelod,thegradWorldReflect);
				}
			}
		} else {
			uvCoords = rawUVCoords; // Don't parallax lightmaps
		}		
	}

	color = sampleTextureSafe(text_in0, uvCoords, thelod,thegrad);
	outFragColor = color;
	if(ssr){
		for(int i=0;i<ssrMultiSampleCount;i++){
			colorWorldReflect[i] = sampleTextureSafe(text_in0, uvCoordsWorldReflect[i], thelod,thegradWorldReflect);
		}
	}

	vec4 vertexLitMult = vec4(1.0f);
	if(vertexLit){
		vertexLitMult = vertColor;
		outFragColor.w *= vertColor.w;
	} else {
		outFragColor *= vertColor;
	}
	
	//if((stageLightmapBitmaskUniform & (1<<6))>0){
		
		//gl_FragColor = texture2D(text_in6, my_TexCoord[1].st);
		//return;
	//}
	//if((lightDir[0] != 0.0f || lightDir[1] != 0.0f || lightDir[2] != 0.0f) && haveVertexLightDirectionUniform > 0 && stageLightmapBitmaskUniform == 0){
		
		//gl_FragColor.xyz = lightDir+vec3(1.0f);
		//gl_FragColor.xyz = normalize(lightDir)+vec3(1.0f);
		//gl_FragColor.x = dot(lightDir,worldNormal);
		//gl_FragColor.x = dot(normalize(lightDir),worldNormal);
		//gl_FragColor.xyz = vec3(max(dot(lightDir,worldNormal),0.0f));
		//gl_FragColor.xyz = vec3(max(dot(normalize(lightDir),worldNormal),0.0f));
		//return;
	//}

	bool thermalVision = thermalVisionUniform > 0 && thermalVisionUniform <= 3;

	float effectiveAlpha = color.w*vertColor.w;

	//bool usesBlending = (appliedStateBitsUniform & GLS_SRCBLEND_BITS) > 0 && (appliedStateBitsUniform & GLS_DSTBLEND_BITS) > 0;

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
			
			//bool mult1 = (rawStateBitsUniform & GLS_SRCBLEND_DST_COLOR) > 0;
			//bool mult2 = (rawStateBitsUniform & GLS_DSTBLEND_SRC_COLOR) > 0;
			//bool isDecal = (mult1 || mult2) && alphaFuncUniform > 0;
			bool mult1 = (rawStateBitsUniform & GLS_SRCBLEND_BITS) == GLS_SRCBLEND_DST_COLOR;// (rawStateBitsUniform & GLS_SRCBLEND_DST_COLOR) > 0;
			bool mult2 = (rawStateBitsUniform & GLS_DSTBLEND_BITS) == GLS_DSTBLEND_SRC_COLOR;//(rawStateBitsUniform & GLS_DSTBLEND_SRC_COLOR) > 0;
			bool isDecal = (mult1 || mult2);// && alphaFuncUniform > 0;
			
			float decalSub = (mult1 && mult2) ? 0.5f : 1.0f;
			if(isDecal){
				outFragColor.xyz = vec3(decalSub);// decal. WEIRD
			}
			isinvisible = true;
			return true; // ok? why do light calc for shit that isnt even visible
		}
	}

#ifdef PERLINFUCKERY
	//{
		outFragColor.x =1;
		vec4 startCooords = pureVertexCoordsGeom*0.25;
		if(isWorldBrushUniform > 0 && isLightmapUniform == 0 && perlinFuckery > 0){
			//gl_FragColor.xyz+=pureVertexCoordsGeom.xyz/1000.0f; 
			switch(perlinFuckery){
				case 1:
			outFragColor.xyz = perlinNoiseVariation1();
				break;
				case 2:
			outFragColor.xyz = perlinNoiseVariation2();
				break;
				case 3:
			//gl_FragColor.xyz = perlinNoiseVariation3()+(color*vertColor).xyz*0.2;
			outFragColor.xyz = 10.0*perlinNoiseVariation3()*(color).xyz/texAverageBrightnessUniform+0.25*(color*vertColor).xyz+perlinNoiseVariation6Stack(pureVertexCoordsGeom,viewOriginUniform)+perlinNoiseVariation5(startCooords);
				break;
				case 4:
			outFragColor.xyz = perlinNoiseVariation4();
				break;
				case 5:
			outFragColor.xyz = perlinNoiseVariation4()*0.25+perlinNoiseVariation5(startCooords);
				break;
				case 6:
			outFragColor.xyz = perlinNoiseVariation6Stack(pureVertexCoordsGeom,viewOriginUniform);
				break;
			}
			if(noiseFuckeryHDRIntensityUniform == 1.0){
				outFragColor.xyz *= HDRtoSRGB;
			} else if(noiseFuckeryHDRIntensityUniform != 0.0){
				outFragColor.xyz = outFragColor.xyz*(1.0-noiseFuckeryHDRIntensityUniform)+(noiseFuckeryHDRIntensityUniform*(outFragColor.xyz*HDRtoSRGB));
			}
		}
		if(isLightmapUniform > 0 && isWorldBrushUniform > 0 && perlinFuckery > 0){
			if(noiseFuckeryLightmapUniform == 0 && perlinFuckery!=3 && perlinFuckery!=1 || noiseFuckeryLightmapUniform == 2){
				outFragColor.xyz = vec3(1.0,1.0,1.0);
			}
			else if(perlinFuckery > 0 && noiseFuckeryLightmapIntensityUniform != 1.0) {
				outFragColor.xyz = vec3(1.0)*(1.0-noiseFuckeryLightmapIntensityUniform)+(noiseFuckeryLightmapIntensityUniform*outFragColor.xyz);
			}
		} 

		//gl_FragColor.xyz+=eyeSpaceCoordsGeom.xyz/1000.0f; // cool effect lol
	//}
#endif
	mat3 rotatemat = mat3(worldModelViewMatrixUniform);
	
	vec3 lightmapReferenceNormal = normalize(mat3(gl_ModelViewMatrix)*normalize(vertexNormal)); // can be normal instead. trying vertexnormal so things are smoother

	//vec3 lightNormal = normal;
	vec3 lightNormal = vertexLit ? lightReferenceNormal : lightmapReferenceNormal;
	if(!isSimpleTCGenEnv){
		lightNormal = calculateTextureNormal(uvCoords,effectiveUVPixelPos,lightNormal,thelod,thegrad);
	}
	vec3 lightNormalWorldReflect[MAX_SSR_MULTISAMPLE*MAX_SSR_MULTISAMPLE];
	lightNormalWorldReflect[0] = lightNormal;
	if(ssr){
		for(int i=0; i< ssrMultiSampleCount;i++){
			lightNormalWorldReflect[i] = vertexLit ? lightReferenceNormal : lightmapReferenceNormal;
			
			if(!isSimpleTCGenEnv){
				lightNormalWorldReflect[i] = calculateTextureNormal(uvCoordsWorldReflect[i],effectiveUVPixelPosWorldReflect[i],lightNormalWorldReflect[i],thelod,thegradWorldReflect);
			}
		}
	}

	//outFragColor.xyz = lightNormal*0.5f+0.5f;
	//float test = 0.72f* length(fract(my_TexCoord[0].st-uvCoords));
	//outFragColor.xyz = vec3(test*test*test*test*test*test*test*test*test*test*test*test*test*test*test*test*test);
	//outFragColor.xyz = vec3((clamp(dot(lightNormal,lightReferenceNormal)-0.9f,0.0f,1.0f))*10.0f);
	//outFragColor.xyz = vec3(lightNormal.z);
	//outFragColor.xyz = vec3(fract(uvCoords).s,fract(uvCoords).t,0.0f);
	//return true;
	
	//mat3 rotatematrev = mat3(worldModelViewMatrixReverseGeom);
	//vec3 worldlightnormal = (rotatematrev*lightNormal).xyz;

	mat4 deluxedirmat = mat4(rotatemat)*lightdirtransform; // takes the raw deluxe map value and turns it into the light direction in eye space
	

	float boringShadowingIntensity = 1.0f;

	vec3 baseColorForLightingReal = outFragColor.xyz;
	vec3 baseColorForLighthmapLighting  = vec3(1.0);
	vec3 baseColorForTexLighting  = outFragColor.xyz;
	vec3 baseColorForLighting = (vertexLit || haveLightmap) ? baseColorForLighthmapLighting : baseColorForTexLighting;

	if(isWorldBrushUniform > 0){
		// Bit of boring standard shadow and ambient occlusion to replace cg_shadows 1
		for(int s=0;s<shadowLinesCountUniform;s++){

			//if(distance(worldPixel.xyz,shadowLines[s].middle.xyz) > (shadowLines[s].halfLineLength+max(shadowLines[s].width,shadowLines[s].a))){
			//	continue;
			//}
			if(0 < (shadowLines[s].flags & 1)){ // Flag 1 means foot shadow

				vec3 point1 = shadowLines[s].point1.xyz + footadjust;
				vec3 rawdelta = point1-worldPixel;
				vec3 delta = rawdelta;
				
				//if(delta.z < -0.1) continue; // foots must be above us.
				if(dot(delta.xy,delta.xy) >6400.0f) continue; // foots must be above us.
				

				if(shadowLines[s].a > shadowLines[s].width){
					// with flag 1, parameter a tells us how great the Z-distance from the foot can be.
					// We simply adjust the z delta accordingly.
					delta.z *=  shadowLines[s].width/shadowLines[s].a;
				}

				vec2 lineToPixel = normalize(worldPixel.xy - shadowLines[s].point1.xy);
				vec3 lightdirHere = shadowLines[s].lightdir.xyz;
				//vec3 lightdirHere = vec3(1.0f,0.0f,1.0f); // shadowLines[s].lightdir
				lightdirHere.z = max(0.5f,lightdirHere.z);
				lightdirHere = normalize(lightdirHere);
				//float directionoverlap = clamp(10.0f*(dot(lineToPixel.xy,normalize(test).xy)-0.9f),0.0f,1.0f);
				//float directionoverlap = clamp(10.0f*(dot(lineToPixel.xy,normalize(test).xy)-0.9f),0.0f,1.0f);

				//delta.xy /= max(1.0f,directionoverlap*directionoverlap*10.0f);

				//float maxDistance = max(0.0f,length(delta)-5.0f);
				//float maxDistanceSquared = maxDistance*maxDistance;

				

				// make a light vector on the normal surface. per normal unit.
				float tmp = dot(lightdirHere,worldNormal);
				vec3 ln = lightdirHere - worldNormal*tmp;
				ln /= tmp;

				// project shadowline onto surface
				vec3 p1 = point1-worldPixel;
				tmp = dot(worldNormal,p1);
				p1 -= worldNormal*tmp;
				p1 -= ln*tmp;
				p1 += worldPixel;
				vec3 p2 = shadowLines[s].point2.xyz+ footadjust-worldPixel;
				tmp = dot(worldNormal,p2);
				p2 -= worldNormal*tmp;
				p2 -= ln*tmp;
				p2 += worldPixel;

				//float softenFactor = length(p1-worldPixel)/10.0f;//length(rawdelta)/10.0f;

				vec3 linedir = normalize(p2-p1);
				rawdelta.z /= 2.0f;
				//float progressMult = clamp(dot(worldPixel-p1,linedir)/dot(linedir,p2-p1),0.0f,1.0f);
				float progressMult = clamp(length(rawdelta)/dot(linedir,p2-p1),0.0f,1.0f);
				float progressMultRaw = progressMult;
				progressMult = 1.0f-progressMult*progressMult;

				float maxDistanceSquared = distanceToLineProperMaybefast(worldPixel,p1,p2); // distanceToLineProperMaybefast
				//float blah = maxDistanceSquared;
				//int type = 0;
				//float maxDistanceSquared = shortestDistanceLinesSquared(worldPixel,worldPixel+lightdirHere*100.0f,point1,shadowLines[s].point2.xyz,type,shadowLines[s].width);
				//maxDistanceSquared = sqrt(maxDistanceSquared);
				maxDistanceSquared = max(maxDistanceSquared-2.0f,0.0f);
				maxDistanceSquared += length(delta.xy)*0.1f;
				maxDistanceSquared *= clamp(delta.z,1.0f,10.0f);
				//maxDistanceSquared /= clamp(length(delta)*progressMult,1.0f,10.0f); // funny artifacts
				maxDistanceSquared /= clamp(sqrt(length(delta))*progressMultRaw,1.0f,10.0f);
				//maxDistanceSquared /= 1.0f+softenFactor*10.0f;
				maxDistanceSquared *= maxDistanceSquared;
				float shadowLineWidthSquared = shadowLines[s].width*shadowLines[s].width;
				//progressMult = min(0.75f,progressMult);
				float lightIntensityHere =clamp((1.0f-progressMult)+ progressMult*maxDistanceSquared / shadowLineWidthSquared,0.0f,1.0f);
				lightIntensityHere = sqrt(lightIntensityHere);
				
				//lightIntensityHere = blah*0.1f;

				//float widenRatio = max(0.0,min(1.0,delta.z/shadowLines[s].width));

				//float maxDistance = length(delta);
				//float lightIntensityHere = min(1.0,max(0.0f,maxDistance / (shadowLines[s].width+widenRatio*shadowLines[s].b)));
				//float maxWidenFade = shadowLines[s].b / (shadowLines[s].width+shadowLines[s].b);
				//lightIntensityHere = max(0.0f,1.0-(1.0-lightIntensityHere)*(1.0-(widenRatio)*maxWidenFade-0.4));

				//lightIntensityHere = max(delta.z*10000.0f,0.0f);
				boringShadowingIntensity = min(lightIntensityHere,boringShadowingIntensity);

			}
			else if(0 < (shadowLines[s].flags & 2)){ // Flag 2 means ambient occlusion thing (we use less of them to not have even more performance loss)
				
				float maxDistance = distanceToLineProperMaybefast(worldPixel,shadowLines[s].point1.xyz,shadowLines[s].point2.xyz );
				
				float lightIntensityHere = min(1.0,max(0.0f,maxDistance / shadowLines[s].width)); // only max 0.2, this is supposed to be mild
				lightIntensityHere = 1.0-lightIntensityHere;
				//lightIntensityHere = max(0.0f,1.0-pow(1.0-lightIntensityHere,2.0)*0.5);
				lightIntensityHere = max(0.0f,1.0-lightIntensityHere*lightIntensityHere*0.5);
				boringShadowingIntensity = min(lightIntensityHere,boringShadowingIntensity);
			}
		}
	}
	
	float boringShadowSubtractValBase = (1.0f - boringShadowingIntensity);
	vec3 boringShadowSubtractVal = baseColorForLightingReal * boringShadowSubtractValBase;
	
	vec3 addValue = vec3(0.0);

#if VOXELSTUFF
	bvec3 collision;
#endif
	float dLightFastSkipThresholdUniformSquared = dLightFastSkipThresholdUniform*dLightFastSkipThresholdUniform;
	
	vec3 viewerVectorNorm = normalize(viewerVector);
	float viewerDistance = length(viewerVector);
	float cosviewercomponent = 1.0 - max(0.0,dot(lightNormal,viewerVectorNorm));
	float specIntensitySchlickMult = dLightSpecBaseReflectivityUniform+(1.0-dLightSpecBaseReflectivityUniform)*cosviewercomponent*cosviewercomponent*cosviewercomponent*cosviewercomponent*cosviewercomponent;


	if(isSaberUniform == 0){ // Don't cast light onto saberblades
	
		float specDistanceDecayExpMult = -1.0f/dLightSpecDistanceDecayUniform;
	
		// cheap lights. no shadows.
		for(int i=0;i<cheapLightsCountUniform;i++){
			vec3 dlightOrigin = cheaplights[i].origin.xyz;
			vec4 eyeCoordLight = worldModelViewMatrixUniform*vec4(dlightOrigin,1.0);
			vec3 lightVector1 = eyeCoordLight.xyz-eyeSpaceCoordsGeom.xyz;
			if(dot(lightVector1,lightReferenceNormal) <= 0.0 && !twoSided){
				continue; // this is the normal of the surface itself, not just of the current pixel. if the light is behind the surface... dont bother.
			}

			vec3 lightVectorNorm = normalize( lightVector1);
			vec3 maybeMirroredLightNormal  = twoSided && dot(lightReferenceNormal,lightVectorNorm) < 0 ? -lightNormal : lightNormal;
			float intensity = max(dot(maybeMirroredLightNormal,lightVectorNorm),0.0f);
			float dist = length(lightVector1);
			if(dist<dLightsUniform[i].mindist){
				dist *= 0.5f;
				dist += dLightsUniform[i].mindist*0.5f;
			}

			vec3 value = (baseColorForLighting*cheaplights[i].color.xyz*cheaplights[i].radius*50.0*dLightIntensityUniform)*intensity/(dist*dist);
			
			addValue += value;

			
			vec3 lightVector1Norm = -lightVectorNorm;

			// now mirror the lightVector around the normal
			vec3 mirroredVec = lightVector1Norm - 2.0*maybeMirroredLightNormal*dot(lightVector1Norm,maybeMirroredLightNormal);
			vec3 mirroredVecNorm = normalize(mirroredVec);

			float specIntensity = pow(max(0.0,dot(mirroredVecNorm,viewerVectorNorm)),dLightSpecGammaUniform);

				
			// do schlick's approximation of fresnel. steep angles looking onto surface: more reflective
			specIntensity *= specIntensitySchlickMult;

			specIntensity *= exp2(specDistanceDecayExpMult*max(0,dist-dLightSpecDistanceMinUniform)); 

			float totalDist = dist + viewerDistance;

			vec3 addVal = (baseColorForLighting*cheaplights[i].color.xyz*cheaplights[i].radius)*specIntensity*dLightSpecIntensityUniform/totalDist;
			addValue += addVal;
		}

		for(int i=0;i<dLightsCountUniform;i++){
		
			vec3 dlightRawOrigin = dLightsUniform[i].origin;
			vec3 dlightOrigin = dlightRawOrigin+dLightJitterUniform;
			bool lightVoxelPathChecked = false;
			vec4 eyeCoordLight = worldModelViewMatrixUniform*vec4(dlightOrigin,1.0);
			vec3 lightVector1 = eyeCoordLight.xyz-eyeSpaceCoordsGeom.xyz;
			if(dot(lightVector1,lightReferenceNormal) <= 0.0 && !twoSided){
				continue; // this is the normal of the surface itself, not just of the current pixel. if the light is behind the surface... dont bother.
			}

			vec3 lightVectorWorld = dlightOrigin-worldPixel;
			vec3 lightVectorWorldNorm = normalize(lightVectorWorld);
			
			vec3 lightVectorNorm = normalize( lightVector1);
			vec3 maybeMirroredLightNormal  = twoSided && dot(lightReferenceNormal,lightVectorNorm) < 0 ? -lightNormal : lightNormal;
			float intensity = max(dot(maybeMirroredLightNormal,lightVectorNorm),0.0f);
			float dist = length(lightVector1);
			if(dist<dLightsUniform[i].mindist){
				dist *= 0.5f;
				dist += dLightsUniform[i].mindist*0.5f;
			}

			vec3 value = (baseColorForLighting*dLightsUniform[i].color*dLightsUniform[i].radius*50.0*dLightIntensityUniform)*intensity/(dist*dist);

			bool fastSkip = allInvocationsARB(intensity <= 0.0 || length(value) < dLightFastSkipThresholdUniform);

			float fastSkipThresMain = dLightFastSkipThresholdUniform/length(value);
		
			int mainLightShadowLinesCalculated = 0;
			float shadowedIntensity = 1.0f;

			if(!fastSkip){

				
#if VOXELSTUFF
				if(!lightVoxelPathChecked && !superfastLighting){
					vec3 voxeltarget = worldPixel +worldNormal*11.0;
					vec3 lightpos = transformDLightForVoxelShadow(dlightRawOrigin+worldNormal*11.0,voxeltarget);
					if(traceVoxel(lightpos,voxeltarget,collision)){
						continue;
					}
				}
				lightVoxelPathChecked = true;
#endif
		
				//vec3 shadowDebugColor = vec3(1.0,1.0,1.0);
				//vec3 lightVectorAbs = worldPixel-dlightOrigin;
				//vec3 lightVectorAbsNorm = normalize(lightVectorAbs);
				int s =mainLightShadowLinesCalculated;
				if(!fastLighting){
					for(;s<shadowLinesCountUniform;s++){

						if(0 < (shadowLines[s].flags & 2)){ // this one's just used for some simplistic ambient occlusion
							continue;
						}
						//if(dot(shadowLines[s].point2.xyz-worldPixel,normal) <=0.0 && dot(shadowLines[s].point1.xyz-worldPixel,normal) <=0.0){ // actually makes performance worse
						//	continue;
						//}

						//lightVector1 : pointing to light 
						//
						vec3 vecToSL = shadowLines[s].middle.xyz - worldPixel;
						float distanceToSL = dot(lightVectorWorldNorm,vecToSL);
						if(distanceToSL < 0) {
							continue;
						}

						float shadowLineIntensity = (isModelUniform > 0 || stageForceNormalUniform > 0) ? clamp(distanceToSL,0.0f,10.0f)*0.1f : 1.0f;

						float maxDistPoint = shadowLines[s].halfLineLength + shadowLines[s].width;
						if(distanceToLineProperMaybefastSquared(shadowLines[s].middle.xyz,worldPixel,dlightOrigin) > maxDistPoint*maxDistPoint*10.0f){
							continue;
						}

						int type= 0;
						float shadowLineWidthSquared = shadowLines[s].width*shadowLines[s].width;
						float maxDistanceSquared = shortestDistanceLinesSquared(worldPixel,dlightOrigin,shadowLines[s].point1.xyz,shadowLines[s].point2.xyz,type,shadowLines[s].width);
						shadowedIntensity *= (1.0f-shadowLineIntensity) + shadowLineIntensity*clamp(maxDistanceSquared / shadowLineWidthSquared,0.0f,1.0f);

						if(allInvocationsARB(shadowedIntensity < fastSkipThresMain)){
							break;
						}

	//					switch(type){
	//						case 0:
	//						shadowDebugColor = vec3(1.0,0.0,0.0);
	//						break;
	//						case 1:
	//						shadowDebugColor = vec3(0.0,1.0,0.0);
	//						break;
	//						case 2:
	//						shadowDebugColor = vec3(0.0,0.0,1.0);
	//						break;
	//						case 3:
	//						shadowDebugColor = vec3(1.0,1.0,0.0);
	//						break;
	//					}

					}
				}
				mainLightShadowLinesCalculated= s;

				addValue+= value*shadowedIntensity;
				if(shadowedIntensity < 1.0){
					//gl_FragColor.xyz += shadowDebugColor*(1.0-shadowedIntensity);
				}
			}
			if(intensity > 0.0 && (renderFlagsUniform & RENDERFLAG_SIMPLELIGHTING) == 0){ // dont do specular for simple-lighting (render flags 1)
				// specular
			
				//vec3 lightVector = eyeSpaceCoordsGeom.xyz-eyeCoordLight.xyz;
				//vec3 lightVectorNorm = normalize(lightVector);
				vec3 lightVector1Norm = -lightVectorNorm;

				// now mirror the lightVector around the normal
				vec3 mirroredVec = lightVector1Norm - 2.0*maybeMirroredLightNormal*dot(lightVector1Norm,maybeMirroredLightNormal);
				vec3 mirroredVecNorm = normalize(mirroredVec);

				float specIntensity = pow(max(0.0,dot(mirroredVecNorm,viewerVectorNorm)),dLightSpecGammaUniform);

				
				// do schlick's approximation of fresnel. steep angles looking onto surface: more reflective
				specIntensity *= specIntensitySchlickMult;

				specIntensity *= exp2(specDistanceDecayExpMult*max(0,dist-dLightSpecDistanceMinUniform)); 

				float totalDist = dist + viewerDistance;

				vec3 addVal = (baseColorForLighting*dLightsUniform[i].color*dLightsUniform[i].radius)*specIntensity*dLightSpecIntensityUniform/totalDist;


				bool fastSkip2 = allInvocationsARB( length(addVal) < dLightFastSkipThresholdUniform || specIntensity <= 0);
				//bool fastSkip2 =  dot(addVal,addVal) < dLightFastSkipThresholdUniformSquared || specIntensity <= 0;

				float fastSkipThresSpec = dLightFastSkipThresholdUniform/length(addVal);
			
				if(!fastSkip2){
					//if( !mainLightShadowLinesCalculated){

#if VOXELSTUFF
					if(!lightVoxelPathChecked && !superfastLighting){
						vec3 voxeltarget = worldPixel +worldNormal*11.0;
						vec3 lightpos = transformDLightForVoxelShadow(dlightRawOrigin+worldNormal*11.0,voxeltarget);
						if(traceVoxel(lightpos,voxeltarget,collision)){
							continue;
						}
					}
					lightVoxelPathChecked = true;
#endif
					int s=mainLightShadowLinesCalculated;
					if(!fastLighting){
						for(;s<shadowLinesCountUniform;s++){

							if(0 < (shadowLines[s].flags & 2)){ // this one's just used for some simplistic ambient occlusion
								continue;
							}
						
							vec3 vecToSL = shadowLines[s].middle.xyz - worldPixel;
							float distanceToSL = dot(lightVectorWorldNorm,vecToSL);
							if(distanceToSL < 0) {
								continue;
							}

							float shadowLineIntensity = (isModelUniform > 0 || stageForceNormalUniform > 0) ? clamp(distanceToSL,0.0f,10.0f)*0.1f : 1.0f;

							float maxDistPoint = shadowLines[s].halfLineLength + shadowLines[s].width;
							if(distanceToLineProperMaybefastSquared(shadowLines[s].middle.xyz,worldPixel,dlightOrigin) > maxDistPoint*maxDistPoint){
								continue;
							}

							int type= 0;
							float shadowLineWidthSquared = shadowLines[s].width*shadowLines[s].width;
							// We can reuse shadowedIntensity if it was already calculated for the main light but otherwise we have to recalculate it here.
							float maxDistanceSquared = shortestDistanceLinesSquared(worldPixel,dlightOrigin,shadowLines[s].point1.xyz,shadowLines[s].point2.xyz,type,shadowLines[s].width);
							shadowedIntensity *= (1.0f-shadowLineIntensity) + shadowLineIntensity*clamp(maxDistanceSquared / shadowLineWidthSquared,0.0f,1.0f);
							if(allInvocationsARB(shadowedIntensity < fastSkipThresSpec)){
								break;
							}
				
							// Actually dont do this, looks bad :) already occluded by geometry
							//float maxDistance = shortestDistanceLines(worldPixel,worldViewer,shadowLines[s].point1.xyz,shadowLines[s].point2.xyz,type);
							//float lightIntensityHere = max(0.0f,maxDistance / shadowLines[s].width);
							//shadowedIntensity = min(lightIntensityHere*lightIntensityHere,shadowedIntensity);
						}
					}
					mainLightShadowLinesCalculated = s;
					//}
					//gl_FragColor.xyz += addVal * shadowedIntensity;
					addValue += addVal * shadowedIntensity;
				}

			}

		}
	
	}

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

	vec3 eyeSpaceLightdir = normalize(rotatemat*lightDir);

	if(haveLightmap){// this is super lame xd. idk, cba to code something that actually makes sense :) at least it kinda works

		// styles
		if((stageLightmapBitmaskUniform & (1<<2))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in2,text_in17,my_TexCoord[2].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<17)) > 0, lightNormal,lightmapReferenceNormal,deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[1],worldPixel);		
		}
		if((stageLightmapBitmaskUniform & (1<<3))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in3,text_in18,my_TexCoord[3].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<18)) > 0, lightNormal,lightmapReferenceNormal,deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[2],worldPixel);		
		}
		if((stageLightmapBitmaskUniform & (1<<4))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in4,text_in19,my_TexCoord[4].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<19)) > 0, lightNormal,lightmapReferenceNormal,deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[3],worldPixel);		
		}
		if((stageLightmapBitmaskUniform & (1<<5))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in5,text_in20,my_TexCoord[5].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<20)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[4],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<6))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in6,text_in21,my_TexCoord[6].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<21)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[5],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<7))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in7,text_in22,my_TexCoord[7].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<22)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[6],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<8))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in8,text_in23,my_TexCoord[8].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<23)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[7],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<9))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in9,text_in24,my_TexCoord[9].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<24)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[8],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<10))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in10,text_in25,my_TexCoord[10].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<25)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[9],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<11))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in11,text_in26,my_TexCoord[11].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<26)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[10],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<12))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in12,text_in27,my_TexCoord[12].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<27)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[11],worldPixel);			
		}
		if((stageLightmapBitmaskUniform & (1<<13))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in13,text_in28,my_TexCoord[13].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<28)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,shaderStylesUniform[12],worldPixel);			
		}/**/
		
		/*
		if((stageLightmapBitmaskUniform & (1<<2))>0){
			lightmapStyleAdd += texture2D(text_in2, my_TexCoord[2].st);				
		}
		if((stageLightmapBitmaskUniform & (1<<3))>0){
			lightmapStyleAdd += texture2D(text_in3, my_TexCoord[3].st);				
		}
		if((stageLightmapBitmaskUniform & (1<<4))>0){
			lightmapStyleAdd += texture2D(text_in4, my_TexCoord[4].st);				
		}
		if((stageLightmapBitmaskUniform & (1<<5))>0){
			lightmapStyleAdd += texture2D(text_in5, my_TexCoord[5].st);				
		}*/

		addValue *= baseColorForLightingReal; // because if we have a lightmap, we 100% used 1.0 as the baseColorForLighting, so we revert that here.
		baseColorForLighting.x = max(baseColorForLightingReal.x,addValueForLightmap.x);
		baseColorForLighting.y = max(baseColorForLightingReal.y,addValueForLightmap.y);
		baseColorForLighting.z = max(baseColorForLightingReal.z,addValueForLightmap.z);
		addValueForLightmap *= baseColorForLighting;
	}
	
	bool didThermal = false;

	if(vertexLit){
		//gl_FragColor.xyz *= vertexLitMult.xyz;
		//if(twoSided){
		//	gl_FragColor.x = 1.0f;
		//}
		//return;
		vec4 multnew = getVertexLightIntensity(vertexLitMult,eyeSpaceLightdir,lightReferenceNormal,lightNormal,viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided);
		vertexLitMult = mix(vertexLitMult,multnew,isSimpleTCGenEnv ? 0.75f:1.0f);
		if(stageColorGenUniform == CGEN_LIGHTING_DIFFUSE || stageForceNormalUniform > 0){ // TODO fix this for flag?
			vertexLitMult.xyz += getVertexLightIntensity(vec4(ambientLight,1.0),lightReferenceNormal,lightReferenceNormal,lightNormal,viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided).xyz * MULTDIVIDE255;
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
			color2 = getLightmapIntensity(text_in1,text_in16,my_TexCoord[1].st,eyeSpaceLightdir,(stageLightmapBitmaskUniform & (1<<16)) > 0,lightNormal,lightmapReferenceNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided,0,worldPixel);
		} else{
			color2 = texture2D(text_in16, my_TexCoord[1].st);
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
		if(ssr){
			#define SSR_MAX_STEPS 150
			#define SSR_STEP_SIZE 20
			
			vec2 depthTexSize = textureSize(text_in31,0);
			vec3 reflectionAccum = vec3(0.0f);
			for(int s=0;s<ssrMultiSampleCount;s++){ // todo make it alsoo do a new viewervector and all that with multisample? or is it negligible?
				if(isWorldBrushUniform > 0){
					surfaceNormal = mix(lightReferenceNormal,lightNormalWorldReflect[s],worldReflectNormalMixUniform);

					if(length(colorWorldReflect[s].xyz)/texAverageBrightnessUniform < worldReflectPuddleThreshUniform){
						// colorWorldReflect
						surfaceNormal =  mix(surfaceNormal,lightReferenceNormal,worldNormal.z*worldNormal.z);
					}
				}
				vec3 normalPart = surfaceNormal * dot(surfaceNormal,viewerVectorNorm);
				vec3 viewerVectorMinusNormal = viewerVectorNorm - normalPart;
				vec3 outVec = normalPart - viewerVectorMinusNormal; // the non-normal part gets inverted

				outVec = normalize(outVec);

				vec2 uvRefl;
				vec4 thegrad;
				bool found = false;
				vec3 newPos = eyeSpaceCoordsGeom.xyz + outVec;
				uvRefl = get360UVFromVector(-normalize(newPos));
				thegrad = vec4(dFdx(uvRefl),dFdy(uvRefl));
				if (abs(thegrad.x) > 0.5) thegrad.x -= sign(thegrad.x);
				if (abs(thegrad.z) > 0.5) thegrad.z -= sign(thegrad.z);
				thegrad *= 0.5f;
				//thegrad *= gradMultiplier; // gotta calc the grad up here cuz inside the loop dFdx and dFdy will break and cause artifaacts
				float oldDist = 0;
				float newDist = 0;
				vec2 uvReflOld = uvRefl;
				vec3 basePos = eyeSpaceCoordsGeom.xyz + lightReferenceNormal*3.0f;

				if(outVec.z > 0 && eyeSpaceCoordsGeom.z < 0 || outVec.z < 0 && eyeSpaceCoordsGeom.z > 0){ // btw -Z in eye space -> forward into the view from camera 
					// we are tracing backwards toward the camera.
					// dont hit stuff from behind, it turns into a mess with lightsabers (numerous ghosts) due to randomly hitting/not hitting them
					// based on the 20 unit step size (presumably)
					// nvm strike all this. that wasnt the reason. at least not the main one. it still helps in edge cases tho
					// but we still want to find stuff behind the camera (360), soo just forward until that.
					float forwardAmount = -eyeSpaceCoordsGeom.z/outVec.z;
					basePos += forwardAmount * outVec;
				}

				for(int i=0;i<SSR_MAX_STEPS;i++){
					newPos = basePos + float(i)*float(SSR_STEP_SIZE)*outVec;
					float dist = length(newPos);
					newPos = normalize(newPos);
					uvRefl = get360UVFromVector(-newPos);
					//float distComp = textureGrad(text_in31,fract(uvRefl),thegrad.xy,thegrad.zw).x;
					float distComp = texelFetch(text_in31,ivec2(fract(uvRefl)*depthTexSize),0).x; // texelfetch the depth to avoid ghosts of lightsabers
					newDist = abs(distComp-dist);
					//if(newDist < 20.0f){
					if(dist > distComp && newDist < float(SSR_STEP_SIZE)){
						found = true;
						//if(newDist > float(SSR_STEP_SIZE+5)){
							// we approached it from behind?
							//newDist = oldDist;
							//uvRefl = uvReflOld;
						//}
						break;
					}
					oldDist = newDist;
					uvReflOld = uvRefl;
				}
				if(found){
					uvRefl = mix(uvReflOld,uvRefl,oldDist/(newDist+oldDist));
					reflectionAccum += textureGrad(text_in30,fract(uvRefl),thegrad.xy,thegrad.zw).xyz;
					//outFragColor.xyz = textureGrad(text_in30,fract(uvRefl),thegrad.xy,thegrad.zw).xyz;
					//outFragColor.xyz =oriColor + outFragColor.xyz*max(worldNormal.z,0.0f)*specIntensitySchlickMultReflective;
				} else{
					//outFragColor.xyz =oriColor + vec3(1.0f,0.0f,0.0f)*max(worldNormal.z,0.0f);
				}
			
			}
			reflectionAccum /= float(ssrMultiSampleCount);
			outFragColor.xyz =oriColor + reflectionAccum*max(worldNormal.z,0.0f)*specIntensitySchlickMultReflective;
			//outFragColor.x = float(worldReflectMultiSampleUniform)*0.25f;
		} else{
		
			
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
			outFragColor.xyz = textureGrad(text_in30,fract(uvRefl),thegrad.xy,thegrad.zw).xyz;
			//outFragColor.xyz = sampleTextureSafe(text_in30, vec2(xAngle,yAngle), thelod,thegrad).xyz;
			//outFragColor.xyz = sampleTextureSafe(text_in30, uvCoords, thelod,thegrad).xyz;
			if(isWorldBrushUniform > 0){
				outFragColor.xyz =oriColor + outFragColor.xyz*max(worldNormal.z,0.0f);
			}
		}

	}

	return true;

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



#ifdef PERLINFUCKERY

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

  #endif

