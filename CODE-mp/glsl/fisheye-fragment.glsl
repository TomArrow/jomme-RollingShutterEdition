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

#define TEXTURE_COUNT 7

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

in vec3 debugColor;
varying vec4 vertColor;
varying vec3 lightDir;
varying vec3 ambientLight;
varying vec3 vertexNormal;
in vec3 texUVTransform[2];

varying vec4 my_TexCoord[TEXTURE_COUNT];

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
uniform int parallaxMapLayersUniform;
uniform float parallaxMapGammaUniform;
uniform float serverTimeUniform;
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

varying vec4 eyeSpaceCoordsGeom;
varying vec4 pureVertexCoordsGeom;

#define RENDERFLAG_SIMPLELIGHTING 1
#define RENDERFLAG_NOLIGHTING 2 // skyboxes and such

uniform int alphaFuncUniform; 
uniform float alphaFuncValueUniform;
uniform int renderFlagsUniform;

uniform int zPrepassUniform;

uniform int deluxeMappingUniform;

uniform int haveVertexLightDirectionUniform;
uniform int stageColorGenUniform;


// multipass stuff
#define MYGL_MODULATE                       0x2100
#define MYGL_DECAL                          0x2101
#define MYGL_ADD							0x0104
#define MYGL_REPLACE                        0x1E01
uniform int stageImageBitmaskUniform;
uniform int stageLightmapBitmaskUniform;
uniform int multiTexModeUniform;




float snoise(vec4 v);


struct dlight_t {
	//int				mType;

	vec3			origin;
	//vec3			mProjOrigin;		// projected light's origin

	vec3			color;				// range from 0.0 to 1.0, should be color normalized

	float			radius;
	/*float			mProjRadius;		// desired radius of light 

	int				additive;			// texture detail is lost tho when the lightmap is dark

	vec3			transformed;		// origin in local coordinate system
	vec3			mProjTransformed;	// projected light's origin in local coordinate system

	vec3			mDirection;
	vec3			mBasis2;
	vec3			mBasis3;

	vec3			mTransDirection;
	vec3			mTransBasis2;
	vec3			mTransBasis3;
	*/
};

uniform int dLightsCountUniform;
uniform dlight_t dLightsUniform[32]; 

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
};

uniform int shadowLinesCountUniform;
//uniform shadowline_t shadowLinesUniform[64*18]; 

layout(std430, binding = 3) buffer shadowLinesLayout
{
    shadowline_t shadowLines[64*18];
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

vec2 parallaxMap(){
		vec2 uvCoords;
		//uvCoords.s = dot(eyeSpaceCoordsGeom.xyz,texUVTransform[0]);
		//uvCoords.t = dot(eyeSpaceCoordsGeom.xyz,texUVTransform[1]);
		vec4 color = texture2D(text_in0, my_TexCoord[0].st);
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
vec2 parallaxMapSteep(inout vec3 finalPosition){
		int layers = parallaxMapLayersUniform;
		vec2 uvCoords;

		float layerDepth = parallaxMapDepthUniform / float(layers);
		vec3 currentPlace = eyeSpaceCoordsGeom.xyz;
		float gamma = 1.0f/parallaxMapGammaUniform;

		//vec4 color = texture2D(text_in, my_TexCoord[0].st);

		vec3 viewVecNormalized = normalize(eyeSpaceCoordsGeom.xyz);
		vec3 depthComponent = normal * dot(normal,viewVecNormalized); // Get the depth component that a unity view vector gives us 
		vec3 viewVecFlat = viewVecNormalized - depthComponent;
		float viewVecMultiplier = layerDepth/length(depthComponent); // Calculate how much we have to multiple the unity view vector with to go one layer deeper.
		vec3 oneLayerProgressVec = viewVecFlat*viewVecMultiplier;

		//uvCoords.s = mod(dot(currentPlace,texUVTransform[0]),1.0);
		//uvCoords.t = mod(dot(currentPlace,texUVTransform[1]),1.0);
		uvCoords.s = dot(currentPlace,texUVTransform[0]);
		uvCoords.t = dot(currentPlace,texUVTransform[1]);

		float oldtexDepth = 0.0;
		float texDepth = 0.0f;
		for(int i=0; i< layers;i++){
			
			vec4 color = texture2D(text_in0, uvCoords);
			oldtexDepth = texDepth;
			texDepth = parallaxMapDepthUniform*(pow(max(min((color.x + color.y + color.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f),gamma)-1.0f);
			
			float newLayerDepth = -(layerDepth * float(i));
			if(texDepth > newLayerDepth){
				if(i > 0){
					//float oldLayerDepth = -(layerDepth * float(i-1));
					//float weight = (texDepth-newLayerDepth)/(oldLayerDepth-newLayerDepth-oldtexDepth+texDepth);
					float weight = (texDepth-newLayerDepth)/(layerDepth-oldtexDepth+texDepth);
					
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
	float timeVal =  serverTimeUniform*2.5;
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
    coords.w = serverTimeUniform*10.0;
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
	float timeVal =  serverTimeUniform*2.5;
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
	float timeVal =  serverTimeUniform*200.0;
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
	
	vec3 maybeMirroredLightNormal = twoSided && dot(referenceNormal,direction) < 0 ? -lightNormal : lightNormal;
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
			float specIntensityTotal = 300.0f*specIntensity*dLightSpecIntensityUniform/totalDist;

			color *= (1.0f-ambientFactor)*alignment*alignment*alignment+specIntensityTotal + ambientFactor;
			//color *= alignment*alignment*alignment+specIntensityTotal;
			//color *= 100.0f;

		//}
	}
	return color;
}

vec4 getLightmapIntensity(sampler2D sampler, sampler2D deluxeSampler, vec2 lmtexcoord, bool havedeluxe, vec3 lightNormal, mat4 dirmat, vec3 viewerVectorNorm, float specIntensitySchlickMult,float viewerDistance, bool twoSided){
	vec4 color;
	//if((stageLightmapBitmaskUniform & (1<<2))>0)
	{
		color = texture2D(sampler, lmtexcoord);		
		//return color;
		if(havedeluxe){
			vec4 direction = texture2D(deluxeSampler, lmtexcoord); // visualize n
			//float baseMultiplier = 1.0f / max(0.00001,dot(normal,(direction).xyz));
			//return direction;
			direction = (dirmat*direction);
			vec3 maybeMirroredNormal = twoSided && dot(normal,direction.xyz) < 0 ? -normal : normal;
			float divider = max(0.05f,dot((maybeMirroredNormal),(direction).xyz)); // 0.05f because that's about the limit before we start seeing ugly seams at lightmaps/deluxemaps wrapping around corners/light bleeding.
			vec3 maybeMirroredLightNormal = twoSided && dot(normal,direction.xyz) < 0 ? -lightNormal : lightNormal;
			float alignment = max(0.05f,dot((maybeMirroredLightNormal),(direction).xyz));
			alignment /= divider;
			//return vec4(vec3(alignment),1.0f);
			//color /= max(0.00001,dot(normal,(direction).xyz));
			alignment = max(0.0f,alignment);

			//do some specular
			vec3 lightVector1Norm = -normalize(direction.xyz);
			vec3 mirroredVec = lightVector1Norm - 2.0*maybeMirroredLightNormal*dot(lightVector1Norm,maybeMirroredLightNormal);
			vec3 mirroredVecNorm = normalize(mirroredVec);

			float specIntensity = pow(max(0.0,dot(mirroredVecNorm,viewerVectorNorm)),dLightSpecGammaUniform);
			
			// do schlick's approximation of fresnel. steep angles looking onto surface: more reflective
			specIntensity *= specIntensitySchlickMult;
			
			float totalDist = viewerDistance; // + dist // dont know distance to light
			//vec3 addVal = color.xyz*specIntensity*dLightSpecIntensityUniform/totalDist;
			float specIntensityTotal = 900.0f*specIntensity*dLightSpecIntensityUniform/totalDist/divider;

			color *=alignment*alignment*alignment+specIntensityTotal;

		}
	}
	return color;
}

vec3 calculateTextureNormal(vec2 uvCoords, vec3 startPosition, vec3 referenceNormal){
		//uvCoords.s = dot(eyeSpaceCoordsGeom.xyz,texUVTransform[0]);
		//uvCoords.t = dot(eyeSpaceCoordsGeom.xyz,texUVTransform[1]);
		vec4 color = texture2D(text_in0, uvCoords);
		//vec4 color = texture2D(text_in, uvCoords);
		float offset = 1.0f - max(min((color.x + color.y + color.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f);

		vec3 offset3d =  normalize(startPosition);
		vec3 normalComponent = referenceNormal * dot(referenceNormal,offset3d);
		offset3d -= normalComponent; // project onto surface aka get rid of any 3d component that aligns with the normal of the surface
		offset3d = normalize(offset3d)*0.1;

		vec3 transposedCoords = startPosition + offset3d;
		uvCoords.s = dot(transposedCoords,texUVTransform[0]);
		uvCoords.t = dot(transposedCoords,texUVTransform[1]);
		vec4 color2 = texture2D(text_in0, uvCoords);
		float offset2 = 1.0f - max(min((color2.x + color2.y + color2.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f);

		vec3 transposedCoords2 = startPosition + normalize(cross(offset3d,referenceNormal))*0.1;
		uvCoords.s = dot(transposedCoords2,texUVTransform[0]);
		uvCoords.t = dot(transposedCoords2,texUVTransform[1]);
		vec4 color3 = texture2D(text_in0, uvCoords);
		float offset3 = 1.0f - max(min((color3.x + color3.y + color3.z)/3.0f/texAverageBrightnessUniform,1.0f),0.0f);

		vec3 place1 = startPosition + normalComponent * offset;
		vec3 place2 = transposedCoords + normalComponent * offset2;
		vec3 place3 = transposedCoords2 + normalComponent * offset3;

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

void main(void)
{
	//gl_FragColor.xyz = vertexNormal;
	//gl_FragColor.w = 1.0f;
	//return;

	if(zPrepassUniform != 0){
		return;
	}
	//bool test[500];
    //const float depth = 5.0f;
#ifdef PERLINFUCKERY
	int perlinFuckery = noiseFuckeryUniform;
#else 
	int perlinFuckery = 0;
#endif

	bool twoSided = (renderFlagsUniform & 4) > 0;
	
	bool multitex = (stageImageBitmaskUniform & 2) > 0;
	bool standAloneLightmap = !multitex && (stageLightmapBitmaskUniform & 1) > 0;
	bool haveLightmap = (stageLightmapBitmaskUniform & 1) > 0 || multitex && (stageLightmapBitmaskUniform & 3) > 0;

	vec2 uvCoords = my_TexCoord[0].st;
	vec3 effectiveUVPixelPos = eyeSpaceCoordsGeom.xyz;
	vec4 color;
	
	bool vertexLit = (lightDir[0] != 0.0f || lightDir[1] != 0.0f || lightDir[2] != 0.0f) && haveVertexLightDirectionUniform > 0 && stageLightmapBitmaskUniform == 0;
	

    if(fishEyeModeUniform == 0){
	
		if(!standAloneLightmap && perlinFuckery == 0 && isWorldBrushUniform > 0 && (renderFlagsUniform & RENDERFLAG_SIMPLELIGHTING) == 0 && (renderFlagsUniform & RENDERFLAG_NOLIGHTING) == 0){
			uvCoords = parallaxMapLayersUniform < 2 ? parallaxMap():parallaxMapSteep(effectiveUVPixelPos);
		} else {
			uvCoords = my_TexCoord[0].st; // Don't parallax lightmaps
		}
		color = texture2D(text_in0, uvCoords);

		gl_FragColor = color; 
		//gl_FragColor.xyz+=debugColor;
		
	} else {
		
		color = texture2D(text_in0, uvCoords);
		gl_FragColor = color; 
		//gl_FragColor.xyz+=debugColor;
	}

	vec4 vertexLitMult = vec4(1.0f);
	if(vertexLit){
		vertexLitMult = vertColor;
		gl_FragColor.w *= vertColor.w;
	} else {
		gl_FragColor *= vertColor;
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


	float effectiveAlpha = color.w*vertColor.w;

	if(effectiveAlpha <= 0.0 || (renderFlagsUniform & RENDERFLAG_NOLIGHTING) > 0) {
		return; // this seem fair?
	} else if(alphaFuncUniform > 0){
		if(
		alphaFuncUniform == ALPHA_GREATER && effectiveAlpha <= alphaFuncValueUniform
		|| alphaFuncUniform == ALPHA_LESS && effectiveAlpha >= alphaFuncValueUniform
		|| alphaFuncUniform == ALPHA_GEQUAL && effectiveAlpha < alphaFuncValueUniform
		){
			return; // ok? why do light calc for shit that isnt even visible
		}
	}

#ifdef PERLINFUCKERY
	//{
		gl_FragColor.x =1;
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
			//gl_FragColor.xyz = perlinNoiseVariation3()+(color*vertColor).xyz*0.2;
			gl_FragColor.xyz = 10.0*perlinNoiseVariation3()*(color).xyz/texAverageBrightnessUniform+0.25*(color*vertColor).xyz+perlinNoiseVariation6Stack(pureVertexCoordsGeom,viewOriginUniform)+perlinNoiseVariation5(startCooords);
				break;
				case 4:
			gl_FragColor.xyz = perlinNoiseVariation4();
				break;
				case 5:
			gl_FragColor.xyz = perlinNoiseVariation4()*0.25+perlinNoiseVariation5(startCooords);
				break;
				case 6:
			gl_FragColor.xyz = perlinNoiseVariation6Stack(pureVertexCoordsGeom,viewOriginUniform);
				break;
			}
			if(noiseFuckeryHDRIntensityUniform == 1.0){
				gl_FragColor.xyz *= HDRtoSRGB;
			} else if(noiseFuckeryHDRIntensityUniform != 0.0){
				gl_FragColor.xyz = gl_FragColor.xyz*(1.0-noiseFuckeryHDRIntensityUniform)+(noiseFuckeryHDRIntensityUniform*(gl_FragColor.xyz*HDRtoSRGB));
			}
		}
		if(isLightmapUniform > 0 && isWorldBrushUniform > 0 && perlinFuckery > 0){
			if(noiseFuckeryLightmapUniform == 0 && perlinFuckery!=3 && perlinFuckery!=1 || noiseFuckeryLightmapUniform == 2){
				gl_FragColor.xyz = vec3(1.0,1.0,1.0);
			}
			else if(perlinFuckery > 0 && noiseFuckeryLightmapIntensityUniform != 1.0) {
				gl_FragColor.xyz = vec3(1.0)*(1.0-noiseFuckeryLightmapIntensityUniform)+(noiseFuckeryLightmapIntensityUniform*gl_FragColor.xyz);
			}
		} 

		//gl_FragColor.xyz+=eyeSpaceCoordsGeom.xyz/1000.0f; // cool effect lol
	//}
#endif
	mat3 rotatemat = mat3(worldModelViewMatrixUniform);
	
	vec3 lightReferenceNormal = stageColorGenUniform == CGEN_LIGHTING_DIFFUSE ? normalize(mat3(gl_ModelViewMatrix)*normalize(vertexNormal)) : normal; // can be normal instead. trying vertexnormal so things are smoother

	//vec3 lightNormal = normal;
	vec3 lightNormal = calculateTextureNormal(uvCoords,effectiveUVPixelPos,lightReferenceNormal);
	
	//mat3 rotatematrev = mat3(worldModelViewMatrixReverseGeom);
	//vec3 worldlightnormal = (rotatematrev*lightNormal).xyz;

	mat4 deluxedirmat = mat4(rotatemat)*lightdirtransform; // takes the raw deluxe map value and turns it into the light direction in eye space
	
	vec3 worldPixel = (worldModelViewMatrixReverseGeom*eyeSpaceCoordsGeom).xyz;

	float boringShadowingIntensity = 1.0f;

	vec3 baseColorForLightingReal = gl_FragColor.xyz;
	vec3 baseColorForLighthmapLighting  = vec3(1.0);
	vec3 baseColorForTexLighting  = gl_FragColor.xyz;
	vec3 baseColorForLighting = (vertexLit || haveLightmap) ? baseColorForLighthmapLighting : baseColorForTexLighting;

	if(isWorldBrushUniform > 0){
		// Bit of boring standard shadow and ambient occlusion to replace cg_shadows 1
		for(int s=0;s<shadowLinesCountUniform;s++){

			//if(distance(worldPixel.xyz,shadowLines[s].middle.xyz) > (shadowLines[s].halfLineLength+max(shadowLines[s].width,shadowLines[s].a))){
			//	continue;
			//}
			if(0 < (shadowLines[s].flags & 1)){ // Flag 1 means foot shadow
				vec3 delta = shadowLines[s].point1.xyz-worldPixel;

				if(delta.z < -0.1) continue; // foots must be above us.

				if(shadowLines[s].a > shadowLines[s].width){
					// with flag 1, parameter a tells us how great the Z-distance from the foot can be.
					// We simply adjust the z delta accordingly.
					delta.z *=  shadowLines[s].width/shadowLines[s].a;
				}

				float widenRatio = max(0.0,min(1.0,delta.z/shadowLines[s].width));

				float maxDistance = length(delta);
				float lightIntensityHere = min(1.0,max(0.0f,maxDistance / (shadowLines[s].width+widenRatio*shadowLines[s].b)));
				float maxWidenFade = shadowLines[s].b / (shadowLines[s].width+shadowLines[s].b);
				//lightIntensityHere = max(0.0f,1.0-pow(1.0-lightIntensityHere,4.0)*(1.0-(4.0*widenRatio)*maxWidenFade-0.3)));
				//lightIntensityHere = max(0.0f,1.0-pow(1.0-lightIntensityHere,1.0)*(1.0-(widenRatio)*maxWidenFade-0.4));
				lightIntensityHere = max(0.0f,1.0-(1.0-lightIntensityHere)*(1.0-(widenRatio)*maxWidenFade-0.4));
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
	
	vec3 viewerVector = -eyeSpaceCoordsGeom.xyz;
	vec3 viewerVectorNorm = normalize(viewerVector);
	float viewerDistance = length(viewerVector);
	float cosviewercomponent = 1.0 - max(0.0,dot(lightNormal,viewerVectorNorm));
	float specIntensitySchlickMult = dLightSpecBaseReflectivityUniform+(1.0-dLightSpecBaseReflectivityUniform)*cosviewercomponent*cosviewercomponent*cosviewercomponent*cosviewercomponent*cosviewercomponent;


	if(isSaberUniform == 0){ // Don't cast light onto saberblades
		
		for(int i=0;i<dLightsCountUniform;i++){
		
			vec3 dlightRawOrigin = dLightsUniform[i].origin;
			vec3 dlightOrigin = dlightRawOrigin+dLightJitterUniform;
			bool lightVoxelPathChecked = false;
			vec4 eyeCoordLight = worldModelViewMatrixUniform*vec4(dlightOrigin,1.0);
			vec3 lightVector1 = eyeCoordLight.xyz-eyeSpaceCoordsGeom.xyz;
			if(dot(lightVector1,lightReferenceNormal) <= 0.0 && !twoSided){
				continue; // this is the normal of the surface itself, not just of the current pixel. if the light is behind the surface... dont bother.
			}
			
			vec3 lightVectorNorm = normalize( lightVector1);
			vec3 maybeMirroredLightNormal  = twoSided && dot(lightReferenceNormal,lightVectorNorm) < 0 ? -lightNormal : lightNormal;
			float intensity = max(dot(maybeMirroredLightNormal,lightVectorNorm),0.0f);
			float dist = length(lightVector1);

			vec3 value = (baseColorForLighting*dLightsUniform[i].color*dLightsUniform[i].radius*50.0*dLightIntensityUniform)*intensity/(dist*dist);

			bool fastSkip = allInvocationsARB(intensity <= 0.0 || length(value) < dLightFastSkipThresholdUniform);

			float fastSkipThresMain = dLightFastSkipThresholdUniform/length(value);
		
			int mainLightShadowLinesCalculated = 0;
			float shadowedIntensity = 1.0f;

			if(!fastSkip){

				
#if VOXELSTUFF
				if(!lightVoxelPathChecked ){
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
				for(;s<shadowLinesCountUniform;s++){

					if(0 < (shadowLines[s].flags & 2)){ // this one's just used for some simplistic ambient occlusion
						continue;
					}
					//if(dot(shadowLines[s].point2.xyz-worldPixel,normal) <=0.0 && dot(shadowLines[s].point1.xyz-worldPixel,normal) <=0.0){ // actually makes performance worse
					//	continue;
					//}

					float maxDistPoint = shadowLines[s].halfLineLength + shadowLines[s].width;
					if(distanceToLineProperMaybefastSquared(shadowLines[s].middle.xyz,worldPixel,dlightOrigin) > maxDistPoint*maxDistPoint*10){
						continue;
					}

					int type= 0;
					float shadowLineWidthSquared = shadowLines[s].width*shadowLines[s].width;
					float maxDistanceSquared = shortestDistanceLinesSquared(worldPixel,dlightOrigin,shadowLines[s].point1.xyz,shadowLines[s].point2.xyz,type,shadowLines[s].width);
					shadowedIntensity *= clamp(maxDistanceSquared / shadowLineWidthSquared,0.0f,1.0f);

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

				float totalDist = dist + viewerDistance;

				vec3 addVal = (baseColorForLighting*dLightsUniform[i].color*dLightsUniform[i].radius)*specIntensity*dLightSpecIntensityUniform/totalDist;


				bool fastSkip2 = allInvocationsARB( length(addVal) < dLightFastSkipThresholdUniform || specIntensity <= 0);
				//bool fastSkip2 =  dot(addVal,addVal) < dLightFastSkipThresholdUniformSquared || specIntensity <= 0;

				float fastSkipThresSpec = dLightFastSkipThresholdUniform/length(addVal);
			
				if(!fastSkip2){
					//if( !mainLightShadowLinesCalculated){

#if VOXELSTUFF
					if(!lightVoxelPathChecked){
						vec3 voxeltarget = worldPixel +worldNormal*11.0;
						vec3 lightpos = transformDLightForVoxelShadow(dlightRawOrigin+worldNormal*11.0,voxeltarget);
						if(traceVoxel(lightpos,voxeltarget,collision)){
							continue;
						}
					}
					lightVoxelPathChecked = true;
#endif
					int s=mainLightShadowLinesCalculated;
					for(;s<shadowLinesCountUniform;s++){

						if(0 < (shadowLines[s].flags & 2)){ // this one's just used for some simplistic ambient occlusion
							continue;
						}
						
						float maxDistPoint = shadowLines[s].halfLineLength + shadowLines[s].width;
						if(distanceToLineProperMaybefastSquared(shadowLines[s].middle.xyz,worldPixel,dlightOrigin) > maxDistPoint*maxDistPoint){
							continue;
						}

						int type= 0;
						float shadowLineWidthSquared = shadowLines[s].width*shadowLines[s].width;
						// We can reuse shadowedIntensity if it was already calculated for the main light but otherwise we have to recalculate it here.
						float maxDistanceSquared = shortestDistanceLinesSquared(worldPixel,dlightOrigin,shadowLines[s].point1.xyz,shadowLines[s].point2.xyz,type,shadowLines[s].width);
						shadowedIntensity *= clamp(maxDistanceSquared / shadowLineWidthSquared,0.0f,1.0f);
						if(allInvocationsARB(shadowedIntensity < fastSkipThresSpec)){
							break;
						}
				
						// Actually dont do this, looks bad :) already occluded by geometry
						//float maxDistance = shortestDistanceLines(worldPixel,worldViewer,shadowLines[s].point1.xyz,shadowLines[s].point2.xyz,type);
						//float lightIntensityHere = max(0.0f,maxDistance / shadowLines[s].width);
						//shadowedIntensity = min(lightIntensityHere*lightIntensityHere,shadowedIntensity);
					}
					mainLightShadowLinesCalculated = s;
					//}
					//gl_FragColor.xyz += addVal * shadowedIntensity;
					addValue += addVal * shadowedIntensity;
				}

			}

		}
	
	}

	
	vec4 lightmapStyleAdd = vec4(0);

	vec3 addValueForLightmap = addValue;

	if(haveLightmap){// this is super lame xd. idk, cba to code something that actually makes sense :) at least it kinda works

		// styles
		if((stageLightmapBitmaskUniform & (1<<2))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in2,text_in17,my_TexCoord[2].st,(stageLightmapBitmaskUniform & (1<<17)) > 0, lightNormal,deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided);		
		}
		if((stageLightmapBitmaskUniform & (1<<3))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in3,text_in18,my_TexCoord[3].st,(stageLightmapBitmaskUniform & (1<<18)) > 0, lightNormal,deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided);		
		}
		if((stageLightmapBitmaskUniform & (1<<4))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in4,text_in19,my_TexCoord[4].st,(stageLightmapBitmaskUniform & (1<<19)) > 0, lightNormal,deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided);		
		}
		if((stageLightmapBitmaskUniform & (1<<5))>0){
			lightmapStyleAdd += getLightmapIntensity(text_in5,text_in20,my_TexCoord[5].st,(stageLightmapBitmaskUniform & (1<<20)) > 0,lightNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided);			
		}/*
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
	
	if(vertexLit){
		//gl_FragColor.xyz *= vertexLitMult.xyz;
		//if(twoSided){
		//	gl_FragColor.x = 1.0f;
		//}
		//return;
		vec3 eyeSpaceLightdir = normalize(rotatemat*lightDir);
		vertexLitMult = getVertexLightIntensity(vertexLitMult,eyeSpaceLightdir,lightReferenceNormal,lightNormal,viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided);
		if(stageColorGenUniform == CGEN_LIGHTING_DIFFUSE){
			vertexLitMult.xyz += getVertexLightIntensity(vec4(ambientLight,1.0),lightReferenceNormal,lightReferenceNormal,lightNormal,viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided).xyz * MULTDIVIDE255;
		}
		gl_FragColor.xyz -= boringShadowSubtractVal;
		vertexLitMult.xyz += addValue;
		vertexLitMult.xyz -= boringShadowSubtractValBase*vertexLitMult.xyz;
		addValue *= baseColorForLightingReal;
		gl_FragColor.xyz += addValue;
		gl_FragColor.xyz *= vertexLitMult.xyz;
	} else {

		gl_FragColor.xyz += (stageLightmapBitmaskUniform & 1) > 0 ? lightmapStyleAdd.xyz : vec3(0.0f);
		gl_FragColor.xyz -= boringShadowSubtractVal;
		gl_FragColor.xyz += (stageLightmapBitmaskUniform & 1) > 0 ? addValueForLightmap+lightmapStyleAdd.xyz : addValue;
	}

	vec4 color2 = vec4(0);
	if(multitex){
		if((stageLightmapBitmaskUniform & 2) >0){
			color2 = getLightmapIntensity(text_in1,text_in16,my_TexCoord[1].st,(stageLightmapBitmaskUniform & (1<<16)) > 0,lightNormal, deluxedirmat, viewerVectorNorm,specIntensitySchlickMult,viewerDistance,twoSided);
		} else{
			color2 = texture2D(text_in16, my_TexCoord[1].st);
		}
		color2.xyz += (stageLightmapBitmaskUniform & 2) > 0 ? lightmapStyleAdd.xyz : vec3(0.0f);
		color2.xyz -= boringShadowSubtractValBase*color2.xyz;
		color2.xyz += (stageLightmapBitmaskUniform & 2) > 0 ? addValueForLightmap : addValue;
		switch(multiTexModeUniform){
			case MYGL_ADD:
				gl_FragColor += color2;
			break;
			case MYGL_MODULATE:
				gl_FragColor *= color2;
			break;
			case MYGL_REPLACE:
				gl_FragColor = color2;
			break;
		}
	}

	//if( (stageLightmapBitmaskUniform & 1) > 0 && multitex){
	//	gl_FragColor.x = 1.0f;
	//}
	//if( (stageLightmapBitmaskUniform & 2) > 0 && multitex){
	//	gl_FragColor.z = 1.0f;
	//}
	//if((stageLightmapBitmaskUniform & (1<<2))>0 && multitex){
	//	gl_FragColor.z = 1.0f;
	//	gl_FragColor.xyz = addValueForLightmap.xyz;
	//}
	//if(dot(lightmapStyleAdd,lightmapStyleAdd) > 0.01f){
	//	//gl_FragColor.z = 1.0f;
	//	gl_FragColor.xyz = addValueForLightmap.xyz;
	//}
	

#if VOXELSTUFF
#if 0
	if(isWorldBrushUniform > 0){
		bvec3 voxelcolor = bvec3(0);
		traceVoxel(viewOriginUniform,worldPixel,voxelcolor);
		gl_FragColor.xyz = vec3(ivec3(voxelcolor));
	}
#endif
#if 0
	int voxelState = voxelSolid(ivec3(floor((worldPixel/float(VOXELGRIDEDGESIZE))+rangeadd)));
	if(voxelState > 0){
		gl_FragColor.x += 0.5;
	} else if(voxelState == -1){
		gl_FragColor.z += 0.5;
	} else if(voxelState == -2){
		gl_FragColor.y += 0.5;
	}
#endif
#endif

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

