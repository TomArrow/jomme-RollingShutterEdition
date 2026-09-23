/*
 *      tr_framebuffer.c
 *      
 *      Copyright 2007 Gord Allott <gordallott@gmail.com>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
*/
// tr_framebuffer.c: framebuffer object rendering path code
// Okay i am going to try and document what I doing here, appologies to anyone 
// that already understands this. basically the idea is that normally everything
// opengl renders will be rendered into client memory, that is the space the 
// graphics card reserves for anything thats going to be sent to the monitor.
// Using this method we instead redirect all the rendering to a seperate bit of 
// memory called a frame buffer. 
// we can then bind this framebuffer to a texture and render that texture to the
// client memory again so that the image will be sent to the monitor. this 
// redirection allows for some neat effects to be applied.

// Some ideas for what to use this path for:
//		- Bloom	-done
//		- Rotoscope cartoon effects (edge detect + colour mapping)
//		- Fake anti-aliasing. (edge detect and blur positive matches)
//		- Motion blur
//			- generate a speed vector based on how the camera has moved since 
//			  the last frame and use that to compute per pixel blur vectors 
//		- These would require mods to use some sort of framebuffer qvm api
//			- special effects for certain powerups 
//			- Image Blur when the player is hit
//			- Depth of field blur

//Sjoerd
//Butchered this up a bit for MME needs and only really has been tested for my ATI videocard

//#pragma once

#ifdef RELDEBUG
//#pragma optimize("", off)
#endif

#include "tr_local.h"
#include "tr_glsl.h"

#include <vector>

#ifdef CAPTURE_FLOAT
#include <cmath>
#endif

#ifdef _WIN32
#include "qgl.h"
#endif

#define MULTIATTACH 1

GLenum attachment1[3] = { GL_COLOR_ATTACHMENT0_EXT , GL_NONE, GL_NONE };
GLenum attachment2[3] = { GL_COLOR_ATTACHMENT1_EXT , GL_NONE, GL_NONE };
GLenum attachment3[3] = { GL_COLOR_ATTACHMENT2_EXT , GL_NONE, GL_NONE };
GLenum attachment1and2[3] = { GL_COLOR_ATTACHMENT0_EXT , GL_COLOR_ATTACHMENT1_EXT, GL_NONE };

extern bool g_SSBOsSupported;
extern ssboSupport_t g_SSBOProperties;

#define NUM_TEXTURE_SAMPLERS 31  // 27 = projector shadows, 28 = cloud image, 29 = sceneview image, 30 = sceneview secondary color buffer

class uniformLocations_t {
public:
	R_GLSL_Uniform viewOriginUniform;
	R_GLSL_Uniform pixelJitterUniform;
	R_GLSL_Uniform dofJitterUniform;
	R_GLSL_Uniform dofFocusUniform;
	R_GLSL_Uniform dofRadiusUniform;
	R_GLSL_Uniform fishEyeModeUniform;
	R_GLSL_Uniform fovXUniform;
	R_GLSL_Uniform fovYUniform;
	R_GLSL_Uniform pixelWidthUniform;
	R_GLSL_Uniform pixelHeightUniform;
	R_GLSL_Uniform jitterIndexUniform;
	R_GLSL_Uniform jitterTotalFramesUniform;
	R_GLSL_Uniform texAverageBrightnessUniform;
	R_GLSL_Uniform isLightmapUniform;
	R_GLSL_Uniform isWorldBrushUniform;
	R_GLSL_Uniform isSaberUniform;
	R_GLSL_Uniform parallaxMapLayersUniform;
	R_GLSL_Uniform parallaxMapDepthUniform;
	R_GLSL_Uniform parallaxMapGammaUniform;
	R_GLSL_Uniform thermalVisionUniform;
	R_GLSL_Uniform shaderDebugUniform;
	R_GLSL_Uniform blurEarlyStageUniform;
	R_GLSL_Uniform serverTimeUniform;
	R_GLSL_Uniform serverTimeStartUniform;
	R_GLSL_Uniform serverTimeFractionUniform;
	R_GLSL_Uniform noiseFuckeryUniform;
	R_GLSL_Uniform noiseFuckeryLightmapUniform;
	R_GLSL_Uniform noiseFuckeryHDRIntensityUniform;
	R_GLSL_Uniform noiseFuckeryLightmapIntensityUniform;
	R_GLSL_Uniform worldModelViewMatrixUniform;
	R_GLSL_Uniform soundDeformSampleRateUniform;
	R_GLSL_Uniform soundDeformSampleCountUniform;

	R_GLSL_Uniform clipPlanesUniform[6];

	R_GLSL_Uniform projectorModelViewMatrixUniform;
	R_GLSL_Uniform projectorProjectionMatrixUniform;
	R_GLSL_Uniform projectorPosUniform;
	R_GLSL_Uniform projectorActiveUniform;

	R_GLSL_Uniform soundDeformTimeUniform;
	R_GLSL_Uniform soundDeformIntensityUniform;
	R_GLSL_Uniform soundDeformSpreadSpeedUniform;
	R_GLSL_Uniform soundDeformSampleAvgWidthUniform;
	R_GLSL_Uniform soundDeformOriginUniform;
	R_GLSL_Uniform soundDeformDistanceScaleUniform;
	R_GLSL_Uniform soundDeformShortDistanceReductionUniform;
	R_GLSL_Uniform soundDeformModeUniform;

	R_GLSL_Uniform alphaFuncUniform;
	R_GLSL_Uniform alphaFuncValueUniform;
	R_GLSL_Uniform renderFlagsUniform;

	R_GLSL_Uniform zPrepassUniform;

	R_GLSL_Uniform deluxeMappingUniform;

	R_GLSL_Uniform text_in[NUM_TEXTURE_SAMPLERS];
	R_GLSL_Uniform text_inArray31;
	R_GLSL_Uniform stageImageBitmaskUniform;
	R_GLSL_Uniform stageLightmapBitmaskUniform;
	R_GLSL_Uniform bindingRectImageBitmaskUniform;
	R_GLSL_Uniform multiTexModeUniform;

	R_GLSL_Uniform haveVertexLightDirectionUniform;
	R_GLSL_Uniform isModelUniform;
	R_GLSL_Uniform surfaceTypeUniform;
	R_GLSL_Uniform stageColorGenUniform;
	R_GLSL_Uniform stageForceNormalUniform;

	R_GLSL_Uniform stageTCGenUniform;
	R_GLSL_Uniform stageHasTCModUniform;
	R_GLSL_Uniform gigaTCGenUniform;

	R_GLSL_Uniform rawStateBitsUniform;
	R_GLSL_Uniform appliedStateBitsUniform;

	R_GLSL_Uniform cloudScaleUniform;
	R_GLSL_Uniform cloudTimeScaleUniform;
	R_GLSL_Uniform cloudPowerUniform;
	R_GLSL_Uniform cloudIntensityCompensateUniform;

	R_GLSL_Uniform myFogUniform;
	R_GLSL_Uniform myFogColorUniform;

	R_GLSL_Uniform dLightFastUniform;
	R_GLSL_Uniform dLightJitterUniform;
	R_GLSL_Uniform dLightVoxelShadowsUniform;
	R_GLSL_Uniform dLightVoxelShadowJitterUniform;
	R_GLSL_Uniform dLightVoxelShadowJitterMethodUniform;
	R_GLSL_Uniform dLightIntensityUniform;
	R_GLSL_Uniform dLightFastSkipThresholdUniform;
	R_GLSL_Uniform dLightSpecIntensityUniform;
	R_GLSL_Uniform dLightSpecGammaUniform;
	R_GLSL_Uniform dLightSpecBaseReflectivityUniform;
	R_GLSL_Uniform dLightSpecDistanceDecayUniform;
	R_GLSL_Uniform dLightSpecDistanceMinUniform;
	R_GLSL_Uniform dLightAddPowUniform;
	R_GLSL_Uniform dLightAddPostPowMultUniform;
	R_GLSL_Uniform dLightsCountUniform; 
	R_GLSL_Uniform dLightsUniformOrigin[MAX_DLIGHTS];
	R_GLSL_Uniform dLightsUniformColor[MAX_DLIGHTS];
	R_GLSL_Uniform dLightsUniformRadius[MAX_DLIGHTS];
	R_GLSL_Uniform dLightsUniformMindist[MAX_DLIGHTS];
	R_GLSL_Uniform shadowLinesCountUniform;
	R_GLSL_Uniform shadowLinesPoint1[MAX_SHADOWLINES];
	R_GLSL_Uniform shadowLinesPoint2[MAX_SHADOWLINES];
	R_GLSL_Uniform shadowLinesWidth[MAX_SHADOWLINES];
	R_GLSL_Uniform shadowLinesA[MAX_SHADOWLINES];
	R_GLSL_Uniform shadowLinesB[MAX_SHADOWLINES];
	R_GLSL_Uniform cheapLightsCountUniform;

	R_GLSL_Uniform worldReflectNormalMixUniform;
	R_GLSL_Uniform worldReflectGradMultUniform;
	R_GLSL_Uniform worldReflectPuddleThreshUniform;
	R_GLSL_Uniform worldReflectMultiSampleUniform;

	R_GLSL_Uniform shaderStylesUniform[MAXLIGHTMAPS_REAL];
	R_GLSL_Uniform lightmapNumsUniform[NUM_TEXTURE_SAMPLERS];
};

typedef enum {
	INSANESHADER_NORMAL,
	INSANESHADER_WITHPERLINFUCKERY,
	INSANESHADER_TYPECOUNT
} insaneShaderType_t;

uniformLocations_t uniformLocationsTessArr[GLSLSHAD_MAX];
uniformLocations_t uniformLocationsArr[GLSLSHAD_MAX];
uniformLocations_t uniformLocationsPostProcessing[GLSLSHAD_MAX];

static GLuint shadowLineSSBOReference = 0;
static GLuint cheapLightSSBOReference = 0;
static GLuint musicDeformSSBOReference = 0;
static shadowline_t shadowLineSSBO[MAX_SHADOWLINES];
static dlightCheap_t cheapLightsSSBO[MAX_CHEAPLIGHTS];
static float* musicDeformSSBOData = NULL;
static GLuint lightStylesSSBOReference = 0;
static vec4_t lightStylesSSBO[MAX_LIGHT_STYLES];
static GLuint variousSSBODataReference = 0;
static variousSSBOData_t variousSSBOData;

static GLuint voxelSSBOReference = 0;
static uint32_t* voxelSSBOData = NULL;
static size_t voxelSSBODataSize = 0;

void R_FrameBuffer_CreateRollingShutterBuffers(int width, int height, int flags);

cvar_t *r_convertToHDR;
cvar_t *r_floatBuffer;
cvar_t *r_fbo;
cvar_t *r_fboGLSL;
cvar_t *r_fboGLSLOff;
cvar_t *r_fboGLSLNoiseFuckery;
cvar_t *r_fboGLSLNoiseFuckeryLightmap; // 0 = as noise fuckery mode wishes, 1 = always on, 2 = always off
cvar_t *r_fboGLSLNoiseFuckeryHDRIntensity;
cvar_t *r_fboGLSLNoiseFuckeryLightmapIntensity;
cvar_t *r_fboGLSLDLights;
cvar_t *r_fboGLSLDLightsFast;
cvar_t *r_fboGLSLDLightsVoxelShadows;
cvar_t *r_fboGLSLDLightsSpecGamma;
cvar_t *r_fboGLSLDLightsIntensity;
cvar_t *r_fboGLSLDLightsAddPow;
cvar_t *r_fboGLSLDLightsAddPostPowMult;
cvar_t *r_fboGLSLDLightsSpecIntensity;
cvar_t *r_fboGLSLDLightsSpecBaseReflectivity;
cvar_t *r_fboGLSLDLightsSpecDistanceDecay;
cvar_t *r_fboGLSLDLightsSpecDistanceMinUniform;
cvar_t *r_fboGLSLDLightsFastSkipThreshold;
cvar_t *r_fboGLSLFastPreview;
cvar_t *r_fboGLSLParallaxMapping;
cvar_t *r_fboGLSLParallaxMappingIntensity;
cvar_t *r_fboGLSLParallaxMappingDepth;
cvar_t *r_fboGLSLParallaxMappingGamma;
cvar_t *r_fboGLSLParallaxMappingLayers;
cvar_t *r_fboGLSLWorldReflectNormalMix;
cvar_t *r_fboGLSLWorldReflectGradMult;
cvar_t *r_fboGLSLWorldReflectPuddleTresh;
cvar_t *r_fboGLSLWorldReflectMultiSample;
cvar_t *r_fboGLSLThermalVision;
cvar_t *r_fboGLSLShaderDebug;
cvar_t *r_fboGLSLCloudShadowScale;
cvar_t *r_fboGLSLCloudShadowTimeScale;
cvar_t *r_fboGLSLCloudShadowPower;
cvar_t *r_fboGLSLCloudIntensityCompensate;
cvar_t *r_fboGLSLFog;
cvar_t *r_fboGLSLFogColor;
cvar_t *r_fboGLSLPreviewSecondary;
cvar_t *r_fboGLSLGigaTCGen;
cvar_t *r_fboGLSLProjector;
cvar_t *r_fboGLSLProjectorWorldShadow;
cvar_t *r_fboGLSLProjectorShader;
cvar_t *r_fboGLSLProjectorPos;
cvar_t *r_fboGLSLProjectorAng;
cvar_t *r_fboGLSLProjectorFov;
cvar_t *r_fboFishEye;
cvar_t *r_fboFishEyeNormalBlend; // doesnt do anything rn
cvar_t *r_fboFishEyeTessellate;
cvar_t *r_fboExposure;
cvar_t *r_fboCompensateSkyTint;
cvar_t *r_fboSuperSample;
cvar_t *r_fboRollingShutterSuperSample;
cvar_t *r_fboSuperSampleMipMap;
cvar_t *r_fboDepthBits;
cvar_t *r_fboDepthPacked;
cvar_t *r_fboStencilWhenNotPacked;
cvar_t *r_fboMultiSample;
cvar_t *r_fboBlur;
cvar_t *r_fboWidth;
cvar_t *r_fboHeight;
cvar_t *r_glDepthClamp;

qboolean mipMapsAlreadyGeneratedThisFrame = qfalse;

// This is a bit unstable/glitchy I think. But inside the game it seems to work. Consider experimental.
// Maybe also needs stepwise downscaling for values over 2? because GL_LINEAR takes only closest 4 pixels into account?
// Or figure how that mipmap stuff works?
int superSampleMultiplier =1; // outside of this file, only READ this. 
int rollingShutterSuperSampleMultiplier =1; // TODO Make it possible to supersample only vertically to get a lot of rolling shutter at lower cost.




#define GL_DEPTH_STENCIL_EXT					0x84F9
#define GL_UNSIGNED_INT_24_8_EXT				0x84FA
#define GL_DEPTH24_STENCIL8_EXT					0x88F0
#define GL_DEPTH32_STENCIL8						0x8CAD
#define GL_DEPTH_COMPONENT24					0x81A6
#define GL_DEPTH_COMPONENT32					0x81A7
#define GL_DEPTH_COMPONENT32F					0x8CAC
#define GL_DEPTH32F_STENCIL8					0x8CAD // Oh.. same value as GL_DEPTH32_STENCIL8! hmm..
#define GL_DEPTH_COMPONENT32F_NV				0x8DAB
#define GL_DEPTH32F_STENCIL8_NV					0x8DAC
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV		0x8DAD
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV_NV	0x8DAD
#define GL_DEPTH_CLAMP							0x864F

#define RGBA32F_ARB                      0x8814
#define RGBA16F_ARB                      0x881A

#ifndef GL_DEPTH_STENCIL_EXT
#define GL_DEPTH_STENCIL_EXT GL_DEPTH_STENCIL_NV
#endif

#ifndef GL_UNSIGNED_INT_24_8_EXT
#define GL_UNSIGNED_INT_24_8_EXT GL_UNSIGNED_INT_24_8_NV
#endif

//typedef frameBufferData_t* doubleFrameBufferData_t[2];






extern std::vector<int> pboRollingShutterProgresses;
extern std::vector<float> pboRollingShutterDrifts;
extern int rollingShutterBufferCount;
extern int progressOvershoot;
extern float drift;

fbo_t fbo; 
fboExtraUniforms_t fboUniformsEx; // extra uniforms


R_GLSL* thermalPostProcessingShader = NULL;
R_GLSL* hdrPqShader = NULL;
R_GLSL* fishEyeShader = NULL;
R_GLSL* fishEyeShaderTess = NULL;
//GLuint tmpPBOtexture;

void R_SetCorrectDrawBuffers();

void R_FrameBuffer_ReloadGLSL() {
	fbo.reloadGLSL = qtrue;
}

static int R_FrameBuffer_GetShaderbits() {
	return (r_fboGLSLNoiseFuckery->integer ? GLSLSHAD_PERLIN : 0) | (fbo.fishEyeData.doingZPrepass ? GLSLSHAD_ZPREPASS : 0);
}

extern int firstServerTime;

qboolean R_FrameBuffer_FishEyeSetUniforms(qboolean tess) {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if (!fishEyeShader || !fishEyeShader->IsWorking())
		return qfalse;

	fbo.screenWidth = glMMEConfig.glWidth;
	fbo.screenHeight = glMMEConfig.glHeight;

	int width = r_fboWidth->integer;
	int height = r_fboHeight->integer;
	//Illegal width/height use original opengl one
	if (width <= 0 || height <= 0) {
		width = fbo.screenWidth;
		height = fbo.screenHeight;
	}

	/* Sending this large amount of data... do it less often, in R_FrameBuffer_SendDLightInfo
	if (g_SSBOsSupported) {
		Com_Memcpy(shadowLineSSBO, backEnd.refdef.shadowlines, backEnd.refdef.num_shadowlines * sizeof(shadowline_t));
		qglBindBufferARB(GL_SHADER_STORAGE_BUFFER,shadowLineSSBOReference);
		qglBufferDataARB(GL_SHADER_STORAGE_BUFFER, sizeof(shadowLineSSBO), shadowLineSSBO, GL_DYNAMIC_DRAW_ARB);
		qglBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, shadowLineSSBOReference);
		qglBindBufferARB(GL_SHADER_STORAGE_BUFFER,0);
	}*/

	int shaderbits = R_FrameBuffer_GetShaderbits();

	uniformLocations_t* uniformLocationsTess = &uniformLocationsTessArr[shaderbits];
	uniformLocations_t* uniformLocations = &uniformLocationsArr[shaderbits];

	int fishEye = backEnd.viewParms.isSceneView && backEnd.viewParms.sceneView.is360 ? 2 : r_fboFishEye->integer;
	int extraRenderFlags = backEnd.viewParms.isSceneView ? RENDERFLAG_SCENEVIEW : 0;
	if (backEnd.viewParms.isSceneView && (backEnd.viewParms.sceneView.flags & SCENEVIEW_WORLDREFLECT)) {
		extraRenderFlags |= RENDERFLAG_RENDERINGWORLDREFLECT;
	}
	if (backEnd.currentEntity && backEnd.currentEntity->e.useSceneViewTexture) {
		extraRenderFlags |= RENDERFLAG_SCENEVIEWBOUND;
	}
	else if(backEnd.needSceneViewAttached) {
		extraRenderFlags |= RENDERFLAG_SCENEVIEWBOUND | RENDERFLAG_SCENEVIEWWORLDREFLECTBOUND;
	}
	if (r_fboGLSLFastPreview->integer && !tr.captureIsActive) {
		extraRenderFlags |= RENDERFLAG_FASTPREVIEW;
	}

	float intensityCompensateFactor = 1.0f;
	if (r_fboGLSLCloudIntensityCompensate->value != 0.0f && tr.cloudsImage && tr.cloudsImage->averageBrightnessLevel > 0.0f) {
		intensityCompensateFactor = 1.0f / tr.cloudsImage->averageBrightnessLevel;
		intensityCompensateFactor = 1.0f + (intensityCompensateFactor - 1.0f) * r_fboGLSLCloudIntensityCompensate->value;
	}

	if (tess) {

		uniformLocationsTess->viewOriginUniform.set3fv( 1, tr.refdef.vieworg);
		uniformLocationsTess->pixelJitterUniform.set3fv( 1, fbo.fishEyeData.pixelJitter3D);
		uniformLocationsTess->dofJitterUniform.set3fv( 1, fbo.fishEyeData.dofJitter3D);
		uniformLocationsTess->dofFocusUniform.set1f( fbo.fishEyeData.dofFocus);
		uniformLocationsTess->dofRadiusUniform.set1f( fbo.fishEyeData.dofRadius);
		uniformLocationsTess->fishEyeModeUniform.set1i( fishEye);
		uniformLocationsTess->fovXUniform.set1f( fbo.fishEyeData.fovX);
		uniformLocationsTess->fovYUniform.set1f( fbo.fishEyeData.fovY);
		uniformLocationsTess->pixelWidthUniform.set1i( width*superSampleMultiplier);
		uniformLocationsTess->pixelHeightUniform.set1i( height * superSampleMultiplier);
		uniformLocationsTess->jitterIndexUniform.set1i( fbo.fishEyeData.jitterIndex);
		uniformLocationsTess->jitterTotalFramesUniform.set1i( fbo.fishEyeData.jitterTotalFrames);
		uniformLocationsTess->texAverageBrightnessUniform.set1f( fbo.fishEyeData.texAverageBrightness);
		uniformLocationsTess->isLightmapUniform.set1i( fbo.fishEyeData.isLightmap ? 1 : 0);
		uniformLocationsTess->isWorldBrushUniform.set1i( fbo.fishEyeData.isWorldBrush ? 1 : 0);
		uniformLocationsTess->isSaberUniform.set1i( fbo.fishEyeData.isSaber ? 1 : 0);
		uniformLocationsTess->parallaxMapLayersUniform.set1i( r_fboGLSLParallaxMappingLayers->integer);
		uniformLocationsTess->parallaxMapDepthUniform.set1f( r_fboGLSLParallaxMappingDepth->value);
		uniformLocationsTess->parallaxMapGammaUniform.set1f( r_fboGLSLParallaxMappingGamma->value);
		uniformLocationsTess->thermalVisionUniform.set1i( r_fboGLSLThermalVision->integer);
		uniformLocationsTess->shaderDebugUniform.set1i( r_fboGLSLShaderDebug->integer);
		uniformLocationsTess->serverTimeUniform.set1i( backEnd.refdef.time);
		uniformLocationsTess->serverTimeStartUniform.set1i( firstServerTime);
		uniformLocationsTess->serverTimeFractionUniform.set1f( backEnd.refdef.timeFraction);
		uniformLocationsTess->noiseFuckeryUniform.set1i( r_fboGLSLNoiseFuckery->integer);
		uniformLocationsTess->noiseFuckeryLightmapUniform.set1i( r_fboGLSLNoiseFuckeryLightmap->integer);
		uniformLocationsTess->noiseFuckeryLightmapIntensityUniform.set1f( r_fboGLSLNoiseFuckeryLightmapIntensity->value);
		uniformLocationsTess->noiseFuckeryHDRIntensityUniform.set1f( r_fboGLSLNoiseFuckeryHDRIntensity->value);
		uniformLocationsTess->worldModelViewMatrixUniform.setMatrix4fv( 1, GL_FALSE, backEnd.viewParms.world.modelMatrix);
		uniformLocationsTess->soundDeformSampleRateUniform.set1i( fbo.soundDeformSampleRate);
		uniformLocationsTess->soundDeformSampleCountUniform.set1i( fbo.soundDeformSampleCount);

		for (int i = 0; i < 6; i++) {
			uniformLocationsTess->clipPlanesUniform[i].set4fv( 1, fboUniformsEx.clipPlanes[i]);
		}

		uniformLocationsTess->projectorModelViewMatrixUniform.setMatrix4fv( 1, GL_FALSE, tr.projector.modelMatrix);
		uniformLocationsTess->projectorProjectionMatrixUniform.setMatrix4fv( 1, GL_FALSE, tr.projector.projectionMatrix);
		uniformLocationsTess->projectorPosUniform.set3fv( 1, tr.projector.pos);
		uniformLocationsTess->projectorActiveUniform.set1i( fboUniformsEx.projectorActive);

		uniformLocationsTess->soundDeformTimeUniform.set1f( fbo.musicDeformData.time);
		uniformLocationsTess->soundDeformIntensityUniform.set1f( fbo.musicDeformData.intensity);
		uniformLocationsTess->soundDeformSpreadSpeedUniform.set1f( fbo.musicDeformData.spreadSpeed);
		uniformLocationsTess->soundDeformSampleAvgWidthUniform.set1i( fbo.musicDeformData.sampleAvgWidth);
		uniformLocationsTess->soundDeformOriginUniform.set3fv( 1, fbo.musicDeformData.origin);
		uniformLocationsTess->soundDeformDistanceScaleUniform.set1f( fbo.musicDeformData.distanceScale);
		uniformLocationsTess->soundDeformShortDistanceReductionUniform.set1f( fbo.musicDeformData.shortDistanceReduction);
		uniformLocationsTess->soundDeformModeUniform.set1i( fbo.musicDeformData.mode);

		uniformLocationsTess->alphaFuncUniform.set1i( fbo.fishEyeData.alphaFunc);
		uniformLocationsTess->alphaFuncValueUniform.set1f( fbo.fishEyeData.alphaFuncValue);
		uniformLocationsTess->renderFlagsUniform.set1i( fbo.fishEyeData.renderFlags | extraRenderFlags);

		uniformLocationsTess->zPrepassUniform.set1i( fbo.fishEyeData.doingZPrepass);

		uniformLocationsTess->deluxeMappingUniform.set1i( tr.deluxeMapping);

		uniformLocationsTess->haveVertexLightDirectionUniform.set1i( fbo.fishEyeData.haveVertexLightDirection ? 1 : 0);
		uniformLocationsTess->isModelUniform.set1i( fbo.fishEyeData.isModel ? 1 : 0);
		uniformLocationsTess->surfaceTypeUniform.set1i( (int)fbo.fishEyeData.surfaceType);
		uniformLocationsTess->stageColorGenUniform.set1i( fbo.fishEyeData.stageColorGen);
		uniformLocationsTess->stageForceNormalUniform.set1i( fbo.fishEyeData.stageForceNormal);

		uniformLocationsTess->stageTCGenUniform.set1i( fbo.fishEyeData.stageTCGen);
		uniformLocationsTess->stageHasTCModUniform.set1i( fbo.fishEyeData.stageHasTCMod);
		uniformLocationsTess->gigaTCGenUniform.set1i( r_fboGLSLGigaTCGen->integer);

		uniformLocationsTess->rawStateBitsUniform.set1ui( fbo.fishEyeData.stateBitsRaw);
		uniformLocationsTess->appliedStateBitsUniform.set1ui( fbo.fishEyeData.stateBitsApplied);

		uniformLocationsTess->stageImageBitmaskUniform.set1i( fbo.fishEyeData.stageImageBitmask);
		uniformLocationsTess->stageLightmapBitmaskUniform.set1i( fbo.fishEyeData.stageLightmapBitmask);
		uniformLocationsTess->bindingRectImageBitmaskUniform.set1ui( fboUniformsEx.textRectBitmask);
		uniformLocationsTess->multiTexModeUniform.set1i( fbo.fishEyeData.multiTexMode);

		uniformLocationsTess->cloudScaleUniform.set1f( r_fboGLSLCloudShadowScale->value);
		uniformLocationsTess->cloudTimeScaleUniform.set1f( r_fboGLSLCloudShadowTimeScale->value);
		uniformLocationsTess->cloudPowerUniform.set1f( r_fboGLSLCloudShadowPower->value);
		uniformLocationsTess->cloudIntensityCompensateUniform.set1f( intensityCompensateFactor);

		uniformLocationsTess->myFogUniform.set1f( r_fboGLSLFog->value);
		uniformLocationsTess->myFogColorUniform.set3fv( 1, tr.fboGLSLFogColor);

		uniformLocationsTess->worldReflectNormalMixUniform.set1f( r_fboGLSLWorldReflectNormalMix->value);
		uniformLocationsTess->worldReflectGradMultUniform.set1f( r_fboGLSLWorldReflectGradMult->value);
		uniformLocationsTess->worldReflectPuddleThreshUniform.set1f( r_fboGLSLWorldReflectPuddleTresh->value);
		uniformLocationsTess->worldReflectMultiSampleUniform.set1i( r_fboGLSLWorldReflectMultiSample->integer);

		uniformLocationsTess->dLightFastUniform.set1i( r_fboGLSLDLightsFast->integer);
		uniformLocationsTess->dLightVoxelShadowsUniform.set1i( r_fboGLSLDLightsVoxelShadows->integer);
		uniformLocationsTess->dLightVoxelShadowJitterUniform.set3fv( 1, fbo.fishEyeData.dlightVoxelShadowJitter3D);
		uniformLocationsTess->dLightVoxelShadowJitterMethodUniform.set1i( mme_voxelShadowLightQuickJitterMethod->integer);
		uniformLocationsTess->dLightJitterUniform.set3fv( 1, fbo.fishEyeData.dlightJitter3D);
		uniformLocationsTess->dLightsCountUniform.set1i( r_fboGLSLDLights->integer?  backEnd.refdef.num_dlights : 0);
		uniformLocationsTess->dLightSpecGammaUniform.set1f( r_fboGLSLDLightsSpecGamma->value);
		uniformLocationsTess->dLightSpecIntensityUniform.set1f( r_fboGLSLDLightsSpecIntensity->value);
		uniformLocationsTess->dLightSpecBaseReflectivityUniform.set1f( r_fboGLSLDLightsSpecBaseReflectivity->value);
		uniformLocationsTess->dLightSpecDistanceDecayUniform.set1f( r_fboGLSLDLightsSpecDistanceDecay->value);
		uniformLocationsTess->dLightSpecDistanceMinUniform.set1f( r_fboGLSLDLightsSpecDistanceMinUniform->value);
		uniformLocationsTess->dLightIntensityUniform.set1f( r_fboGLSLDLightsIntensity->value);
		uniformLocationsTess->dLightFastSkipThresholdUniform.set1f( r_fboGLSLDLightsFastSkipThreshold->value);
		uniformLocationsTess->dLightAddPowUniform.set1f( r_fboGLSLDLightsAddPow->value);
		uniformLocationsTess->dLightAddPostPowMultUniform.set1f( r_fboGLSLDLightsAddPostPowMult->value);
		//uniformLocationsTess->dLightsUniform").set3fv( sizeof(dlight_t) / 4 / 4 * backEnd.refdef.num_dlights, (GLfloat*)&backEnd.refdef.dlights);
		uniformLocationsTess->shadowLinesCountUniform.set1i( r_fboGLSLDLights->integer ? backEnd.refdef.num_shadowlines : 0);
		uniformLocationsTess->cheapLightsCountUniform.set1i( r_fboGLSLDLights->integer ? backEnd.refdef.num_cheaplights : 0);
		if (r_fboGLSLDLights->integer) {
			/*for (int i = 0; i < backEnd.refdef.num_dlights; i++) {

				uniformLocationsTess->dLightsUniformOrigin[i].set3fv( 1, backEnd.refdef.dlights[i].origin);
				uniformLocationsTess->dLightsUniformColor[i].set3fv( 1, backEnd.refdef.dlights[i].color);
				uniformLocationsTess->dLightsUniformRadius[i].set1f( backEnd.refdef.dlights[i].radius);
			}*/
		}
		for (int i = 0; i < NUM_TEXTURE_SAMPLERS; i++) {
			uniformLocationsTess->text_in[i].set1i( i);
			uniformLocationsTess->lightmapNumsUniform[i].set1i( fboUniformsEx.lightmapNums[i]);
		}
		uniformLocationsTess->text_inArray31.set1i( 31);
		for (int i = 0; i < MAXLIGHTMAPS_REAL; i++) {
			uniformLocationsTess->shaderStylesUniform[i].set1i( fbo.fishEyeData.shaderStyles[i]);
		}

		if (fbo.fishEyeData.tessellationActive) {

			qglPatchParameteri(GL_PATCH_VERTICES, 3);
		}
	}
	else {
		uniformLocations->viewOriginUniform.set3fv( 1, tr.refdef.vieworg);
		uniformLocations->pixelJitterUniform.set3fv( 1, fbo.fishEyeData.pixelJitter3D);
		uniformLocations->dofJitterUniform.set3fv( 1, fbo.fishEyeData.dofJitter3D);
		uniformLocations->dofFocusUniform.set1f( fbo.fishEyeData.dofFocus);
		uniformLocations->dofRadiusUniform.set1f( fbo.fishEyeData.dofRadius);
		uniformLocations->fishEyeModeUniform.set1i( fishEye);
		uniformLocations->fovXUniform.set1f( fbo.fishEyeData.fovX);
		uniformLocations->fovYUniform.set1f( fbo.fishEyeData.fovY);
		uniformLocations->pixelWidthUniform.set1i( width * superSampleMultiplier);
		uniformLocations->pixelHeightUniform.set1i( height * superSampleMultiplier);
		uniformLocations->jitterIndexUniform.set1i( fbo.fishEyeData.jitterIndex);
		uniformLocations->jitterTotalFramesUniform.set1i( fbo.fishEyeData.jitterTotalFrames);
		uniformLocations->texAverageBrightnessUniform.set1f( fbo.fishEyeData.texAverageBrightness);
		uniformLocations->isLightmapUniform.set1i( fbo.fishEyeData.isLightmap ? 1 : 0);
		uniformLocations->isWorldBrushUniform.set1i( fbo.fishEyeData.isWorldBrush ? 1 : 0);
		uniformLocations->isSaberUniform.set1i( fbo.fishEyeData.isSaber ? 1 : 0);
		uniformLocations->parallaxMapLayersUniform.set1i( r_fboGLSLParallaxMappingLayers->integer);
		uniformLocations->parallaxMapDepthUniform.set1f( r_fboGLSLParallaxMappingDepth->value);
		uniformLocations->parallaxMapGammaUniform.set1f( r_fboGLSLParallaxMappingGamma->value);
		uniformLocations->thermalVisionUniform.set1i( r_fboGLSLThermalVision->integer);
		uniformLocations->shaderDebugUniform.set1i( r_fboGLSLShaderDebug->integer);
		uniformLocations->serverTimeUniform.set1i( backEnd.refdef.time);
		uniformLocations->serverTimeStartUniform.set1i( firstServerTime);
		uniformLocations->serverTimeFractionUniform.set1f( backEnd.refdef.timeFraction);
		uniformLocations->noiseFuckeryUniform.set1i( r_fboGLSLNoiseFuckery->integer);
		uniformLocations->noiseFuckeryLightmapUniform.set1i( r_fboGLSLNoiseFuckeryLightmap->integer);
		uniformLocations->noiseFuckeryLightmapIntensityUniform.set1f( r_fboGLSLNoiseFuckeryLightmapIntensity->value);
		uniformLocations->noiseFuckeryHDRIntensityUniform.set1f( r_fboGLSLNoiseFuckeryHDRIntensity->value);
		uniformLocations->worldModelViewMatrixUniform.setMatrix4fv( 1, GL_FALSE, backEnd.viewParms.world.modelMatrix);
		uniformLocations->soundDeformSampleRateUniform.set1i( fbo.soundDeformSampleRate);
		uniformLocations->soundDeformSampleCountUniform.set1i( fbo.soundDeformSampleCount);

		for (int i = 0; i < 6; i++) {
			uniformLocations->clipPlanesUniform[i].set4fv( 1, fboUniformsEx.clipPlanes[i]);
		}

		uniformLocations->projectorModelViewMatrixUniform.setMatrix4fv( 1, GL_FALSE, tr.projector.modelMatrix);
		uniformLocations->projectorProjectionMatrixUniform.setMatrix4fv( 1, GL_FALSE, tr.projector.projectionMatrix);
		uniformLocations->projectorPosUniform.set3fv( 1, tr.projector.pos);
		uniformLocations->projectorActiveUniform.set1i( fboUniformsEx.projectorActive);

		uniformLocations->soundDeformTimeUniform.set1f( fbo.musicDeformData.time);
		uniformLocations->soundDeformIntensityUniform.set1f( fbo.musicDeformData.intensity);
		uniformLocations->soundDeformSpreadSpeedUniform.set1f( fbo.musicDeformData.spreadSpeed);
		uniformLocations->soundDeformSampleAvgWidthUniform.set1i( fbo.musicDeformData.sampleAvgWidth);
		uniformLocations->soundDeformOriginUniform.set3fv(1, fbo.musicDeformData.origin);
		uniformLocations->soundDeformDistanceScaleUniform.set1f( fbo.musicDeformData.distanceScale);
		uniformLocations->soundDeformShortDistanceReductionUniform.set1f( fbo.musicDeformData.shortDistanceReduction);
		uniformLocations->soundDeformModeUniform.set1i( fbo.musicDeformData.mode);

		uniformLocations->alphaFuncUniform.set1i( fbo.fishEyeData.alphaFunc);
		uniformLocations->alphaFuncValueUniform.set1f( fbo.fishEyeData.alphaFuncValue);
		uniformLocations->renderFlagsUniform.set1i( fbo.fishEyeData.renderFlags | extraRenderFlags);

		uniformLocations->zPrepassUniform.set1i( fbo.fishEyeData.doingZPrepass);

		uniformLocations->deluxeMappingUniform.set1i( tr.deluxeMapping);

		uniformLocations->haveVertexLightDirectionUniform.set1i( fbo.fishEyeData.haveVertexLightDirection ? 1 : 0);
		uniformLocations->isModelUniform.set1i( fbo.fishEyeData.isModel ? 1 : 0);
		uniformLocations->surfaceTypeUniform.set1i( (int)fbo.fishEyeData.surfaceType);
		uniformLocations->stageColorGenUniform.set1i( fbo.fishEyeData.stageColorGen);
		uniformLocations->stageForceNormalUniform.set1i( fbo.fishEyeData.stageForceNormal);

		uniformLocations->stageTCGenUniform.set1i( fbo.fishEyeData.stageTCGen);
		uniformLocations->stageHasTCModUniform.set1i( fbo.fishEyeData.stageHasTCMod);
		uniformLocations->gigaTCGenUniform.set1i( r_fboGLSLGigaTCGen->integer);

		uniformLocations->rawStateBitsUniform.set1ui( fbo.fishEyeData.stateBitsRaw);
		uniformLocations->appliedStateBitsUniform.set1ui( fbo.fishEyeData.stateBitsApplied);

		uniformLocations->stageImageBitmaskUniform.set1i( fbo.fishEyeData.stageImageBitmask);
		uniformLocations->stageLightmapBitmaskUniform.set1i( fbo.fishEyeData.stageLightmapBitmask);
		uniformLocations->bindingRectImageBitmaskUniform.set1ui( fboUniformsEx.textRectBitmask);
		uniformLocations->multiTexModeUniform.set1i( fbo.fishEyeData.multiTexMode);

		uniformLocations->cloudScaleUniform.set1f( r_fboGLSLCloudShadowScale->value);
		uniformLocations->cloudTimeScaleUniform.set1f( r_fboGLSLCloudShadowTimeScale->value);
		uniformLocations->cloudPowerUniform.set1f( r_fboGLSLCloudShadowPower->value);
		uniformLocations->cloudIntensityCompensateUniform.set1f( intensityCompensateFactor);

		uniformLocations->myFogUniform.set1f( r_fboGLSLFog->value);
		uniformLocations->myFogColorUniform.set3fv( 1, tr.fboGLSLFogColor);

		uniformLocations->worldReflectNormalMixUniform.set1f( r_fboGLSLWorldReflectNormalMix->value);
		uniformLocations->worldReflectGradMultUniform.set1f( r_fboGLSLWorldReflectGradMult->value);
		uniformLocations->worldReflectPuddleThreshUniform.set1f( r_fboGLSLWorldReflectPuddleTresh->value);
		uniformLocations->worldReflectMultiSampleUniform.set1i( r_fboGLSLWorldReflectMultiSample->integer);

		uniformLocations->dLightFastUniform.set1i( r_fboGLSLDLightsFast->integer);
		uniformLocations->dLightVoxelShadowsUniform.set1i( r_fboGLSLDLightsVoxelShadows->integer);
		uniformLocations->dLightVoxelShadowJitterUniform.set3fv( 1, fbo.fishEyeData.dlightVoxelShadowJitter3D);
		uniformLocations->dLightVoxelShadowJitterMethodUniform.set1i( mme_voxelShadowLightQuickJitterMethod->integer);
		uniformLocations->dLightJitterUniform.set3fv(1, fbo.fishEyeData.dlightJitter3D);
		uniformLocations->dLightsCountUniform.set1i( r_fboGLSLDLights->integer ? backEnd.refdef.num_dlights : 0);
		uniformLocations->dLightSpecGammaUniform.set1f( r_fboGLSLDLightsSpecGamma->value);
		uniformLocations->dLightSpecIntensityUniform.set1f( r_fboGLSLDLightsSpecIntensity->value);
		uniformLocations->dLightSpecBaseReflectivityUniform.set1f( r_fboGLSLDLightsSpecBaseReflectivity->value);
		uniformLocations->dLightSpecDistanceDecayUniform.set1f( r_fboGLSLDLightsSpecDistanceDecay->value);
		uniformLocations->dLightSpecDistanceMinUniform.set1f( r_fboGLSLDLightsSpecDistanceMinUniform->value);
		uniformLocations->dLightIntensityUniform.set1f( r_fboGLSLDLightsIntensity->value);
		uniformLocations->dLightFastSkipThresholdUniform.set1f( r_fboGLSLDLightsFastSkipThreshold->value);
		uniformLocations->dLightAddPowUniform.set1f( r_fboGLSLDLightsAddPow->value);
		uniformLocations->dLightAddPostPowMultUniform.set1f( r_fboGLSLDLightsAddPostPowMult->value);
		//uniformLocations->dLightsUniform").set3fv( sizeof(dlight_t) / 4 / 4 * backEnd.refdef.num_dlights, (GLfloat*)&backEnd.refdef.dlights);
		uniformLocations->shadowLinesCountUniform.set1i( r_fboGLSLDLights->integer ? backEnd.refdef.num_shadowlines : 0);
		uniformLocations->cheapLightsCountUniform.set1i( r_fboGLSLDLights->integer ? backEnd.refdef.num_cheaplights : 0);
		if (r_fboGLSLDLights->integer) {
			/*
			for (int i = 0; i < backEnd.refdef.num_dlights; i++) {

				uniformLocations.dLightsUniformOrigin[i].set3fv( 1, backEnd.refdef.dlights[i].origin);
				uniformLocations.dLightsUniformColor[i].set3fv( 1, backEnd.refdef.dlights[i].color);
				uniformLocations.dLightsUniformRadius[i].set1f( backEnd.refdef.dlights[i].radius);
			}*/
		}
		for (int i = 0; i < MAXLIGHTMAPS_REAL; i++) {
			uniformLocations->shaderStylesUniform[i].set1i(fbo.fishEyeData.shaderStyles[i]);
		}
		uniformLocations->text_inArray31.set1i( 31);
		for (int i = 0; i < NUM_TEXTURE_SAMPLERS; i++) {
			uniformLocations->text_in[i].set1i( i);
			uniformLocations->lightmapNumsUniform[i].set1i( fboUniformsEx.lightmapNums[i]);
		}
	}

	return qtrue;

#endif
}


qboolean R_FrameBuffer_SendDLightInfo() {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	qboolean tess = fbo.fishEyeData.tessellationActive;
	if (!fishEyeShader || !fishEyeShader->IsWorking())
		return qfalse;

	fbo.screenWidth = glMMEConfig.glWidth;
	fbo.screenHeight = glMMEConfig.glHeight;

	R_FrameBuffer_FishEyeSetUniforms(tess);

	int shaderbits = R_FrameBuffer_GetShaderbits();

	uniformLocations_t* uniformLocationsTess = &uniformLocationsTessArr[shaderbits];
	uniformLocations_t* uniformLocations = &uniformLocationsArr[shaderbits];

	if (tess) {

		uniformLocationsTess->dLightsCountUniform.set1i( r_fboGLSLDLights->integer ? backEnd.refdef.num_dlights : 0);
		uniformLocationsTess->shadowLinesCountUniform.set1i( r_fboGLSLDLights->integer ? backEnd.refdef.num_shadowlines : 0);
		uniformLocationsTess->cheapLightsCountUniform.set1i( r_fboGLSLDLights->integer ? backEnd.refdef.num_cheaplights : 0);
		if (r_fboGLSLDLights->integer) {
			for (int i = 0; i < backEnd.refdef.num_dlights; i++) {

				uniformLocationsTess->dLightsUniformOrigin[i].set3fv( 1, backEnd.refdef.dlights[i].origin);
				uniformLocationsTess->dLightsUniformColor[i].set3fv( 1, backEnd.refdef.dlights[i].color);
				uniformLocationsTess->dLightsUniformRadius[i].set1f( backEnd.refdef.dlights[i].radius);
				uniformLocationsTess->dLightsUniformMindist[i].set1f( backEnd.refdef.dlights[i].mindist);
			}
		}

	}
	else {
		uniformLocations->dLightsCountUniform.set1i( r_fboGLSLDLights->integer ? backEnd.refdef.num_dlights : 0);
		uniformLocations->shadowLinesCountUniform.set1i( r_fboGLSLDLights->integer ? backEnd.refdef.num_shadowlines : 0);
		uniformLocations->cheapLightsCountUniform.set1i( r_fboGLSLDLights->integer ? backEnd.refdef.num_cheaplights : 0);
		if (r_fboGLSLDLights->integer) {
			for (int i = 0; i < backEnd.refdef.num_dlights; i++) {

				uniformLocations->dLightsUniformOrigin[i].set3fv( 1, backEnd.refdef.dlights[i].origin);
				uniformLocations->dLightsUniformColor[i].set3fv( 1, backEnd.refdef.dlights[i].color);
				uniformLocations->dLightsUniformRadius[i].set1f( backEnd.refdef.dlights[i].radius);
				uniformLocations->dLightsUniformMindist[i].set1f( backEnd.refdef.dlights[i].mindist);
			}
		}
	}

	return qtrue;

#endif
}

qboolean R_FrameBuffer_SendDLightSSBOInfo() {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	qboolean tess = fbo.fishEyeData.tessellationActive;
	if (!fishEyeShader || !fishEyeShader->IsWorking())
		return qfalse;

	fbo.screenWidth = glMMEConfig.glWidth;
	fbo.screenHeight = glMMEConfig.glHeight;

	if (g_SSBOsSupported) {
		Com_Memcpy(shadowLineSSBO, backEnd.refdef.shadowlines, backEnd.refdef.num_shadowlines * sizeof(shadowline_t));
		qglBindBufferARB(GL_SHADER_STORAGE_BUFFER, shadowLineSSBOReference);
		qglBufferDataARB(GL_SHADER_STORAGE_BUFFER, sizeof(shadowLineSSBO), shadowLineSSBO, GL_DYNAMIC_DRAW_ARB);
		qglBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, shadowLineSSBOReference);
		qglBindBufferARB(GL_SHADER_STORAGE_BUFFER, 0);

		Com_Memcpy(cheapLightsSSBO, backEnd.refdef.cheaplights, backEnd.refdef.num_cheaplights * sizeof(dlightCheap_t));
		qglBindBufferARB(GL_SHADER_STORAGE_BUFFER, cheapLightSSBOReference);
		qglBufferDataARB(GL_SHADER_STORAGE_BUFFER, sizeof(cheapLightsSSBO), cheapLightsSSBO, GL_DYNAMIC_DRAW_ARB);
		qglBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, cheapLightSSBOReference);
		qglBindBufferARB(GL_SHADER_STORAGE_BUFFER, 0);

		Com_Memcpy(lightStylesSSBO, styleColors, sizeof(styleColors));
		qglBindBufferARB(GL_SHADER_STORAGE_BUFFER, lightStylesSSBOReference);
		qglBufferDataARB(GL_SHADER_STORAGE_BUFFER, sizeof(lightStylesSSBO), lightStylesSSBO, GL_DYNAMIC_DRAW_ARB);
		qglBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, lightStylesSSBOReference);
		qglBindBufferARB(GL_SHADER_STORAGE_BUFFER, 0);

		Com_Memcpy(variousSSBOData.styleSundirections, tr.sunDirections, sizeof(variousSSBOData.styleSundirections));
		qglBindBufferARB(GL_SHADER_STORAGE_BUFFER, variousSSBODataReference);
		qglBufferDataARB(GL_SHADER_STORAGE_BUFFER, sizeof(variousSSBOData), &variousSSBOData, GL_DYNAMIC_DRAW_ARB);
		qglBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, variousSSBODataReference);
		qglBindBufferARB(GL_SHADER_STORAGE_BUFFER, 0);
	}

	return qtrue;

#endif
}

qboolean R_FrameBuffer_TempDeactivateFisheye(qboolean glfinish = qtrue) {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if (!fishEyeShader || !fishEyeShader->IsWorking())
		return qfalse;

	if (fbo.fishEyeActive || fbo.fishEyeTempDisabled > 0) {
		fbo.fishEyeTempDisabled++;
		if (glfinish) {
			// We need this because when using those complex shaders things get really weird if we deactivate the shader and dont call glFinish before.
			// Stuff like image just never refreshing, image flickering back and forth betweeen an earlier and later one. Just weird shit.
			qglFinish(); 
		}
		R_GLSL::UseProgram(0,0);
		fbo.fishEyeActive = qfalse;
		return qtrue;
	}
	//else if (fbo.fishEyeTempDisabled > 0) {
	//	fbo.fishEyeTempDisabled++;
	//}

	return qtrue;

#endif
}
static qboolean R_FrameBuffer_ReactivateFisheye() {
#ifdef HAVE_GLES 
	//TODO
	return qfalse;
#else
	if (!fishEyeShader || !fishEyeShader->IsWorking())
		return qfalse;

	if ( !(r_fboGLSL->integer && ENABLEGLSL && !r_fboGLSLOff->integer)) {
		if (fbo.fishEyeActive) {
			R_FrameBuffer_DeactivateFisheye();
		}
		return qfalse;
	}

	if (fbo.fishEyeTempDisabled > 0) {

		if (fbo.fishEyeTempDisabled-- == 1) {


			(fbo.fishEyeData.tessellationActive ? fishEyeShaderTess : fishEyeShader)->UseThisProgram(R_FrameBuffer_GetShaderbits());
			fbo.fishEyeActive = qtrue;

			R_FrameBuffer_FishEyeSetUniforms(fbo.fishEyeData.tessellationActive);

			return qtrue;
		}

	}

	return qtrue;

#endif
}
qboolean R_FrameBuffer_SetProjection2D(qboolean is2D) {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if (!fishEyeShader || !fishEyeShader->IsWorking())
		return qfalse;

	fbo.drawing2D = is2D;

#if MULTIATTACH
	R_SetCorrectDrawBuffers();
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif

	return qtrue;

#endif
}



qboolean R_FrameBuffer_ActivateFisheye(vec_t* pixelJitter3D, vec_t* dofJitter3D, vec_t* voxelshadowJitter3D, vec_t* dlightJitter3D, float dofFocus, float dofRadius, float fovX, float fovY, int jitterIndex, int jitterTotalFrames) {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if (!fishEyeShader || !fishEyeShader->IsWorking())
		return qfalse;

	if ( !(r_fboGLSL->integer && ENABLEGLSL && !r_fboGLSLOff->integer)) {
		if (fbo.fishEyeActive) {
			R_FrameBuffer_DeactivateFisheye();
		}
		return qfalse;
	}

	(fbo.fishEyeData.tessellationActive ? fishEyeShaderTess : fishEyeShader)->UseThisProgram(R_FrameBuffer_GetShaderbits());
	fbo.fishEyeActive = qtrue;

	VectorCopy(dofJitter3D, fbo.fishEyeData.dofJitter3D);
	VectorCopy(pixelJitter3D, fbo.fishEyeData.pixelJitter3D);
	VectorCopy(voxelshadowJitter3D, fbo.fishEyeData.dlightVoxelShadowJitter3D);
	VectorCopy(dlightJitter3D, fbo.fishEyeData.dlightJitter3D);
	fbo.fishEyeData.dofFocus = dofFocus;
	fbo.fishEyeData.dofRadius = dofRadius;
	fbo.fishEyeData.fovX = fovX;
	fbo.fishEyeData.fovY = fovY;
	fbo.fishEyeData.jitterIndex = jitterIndex;
	fbo.fishEyeData.jitterTotalFrames = jitterTotalFrames;

	R_FrameBuffer_FishEyeSetUniforms(fbo.fishEyeData.tessellationActive);

	return qtrue;
#endif
}

qboolean R_FrameBuffer_SetDynamicUniforms(const float* texAverageBrightness, const bool* isLightmap, const bool* isWorldBrush, const bool* isSaber, const int* alphaFunc, const  float* alphaFuncValue, const bool* simpleLighting, const  bool* noLighting, const  bool* zPrepass,const shaderStage_t* stageInfoForMultipass) {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	bool uniformsSet = false;
	if (!(r_fboGLSL->integer && ENABLEGLSL && !r_fboGLSLOff->integer)) {
		return qfalse;
	}

	if (texAverageBrightness) {
		fbo.fishEyeData.texAverageBrightness = *texAverageBrightness;
	}
	if (isLightmap) {
		fbo.fishEyeData.isLightmap = *isLightmap;
		fbo.fishEyeData.stageLightmapBitmask = 0;
	}
	if (isWorldBrush) {
		fbo.fishEyeData.isWorldBrush = *isWorldBrush;
	}
	if (isSaber) {
		fbo.fishEyeData.isSaber = *isSaber;
	}
	if (alphaFunc) {
		fbo.fishEyeData.alphaFunc = *alphaFunc;
	}
	if (alphaFuncValue) {
		fbo.fishEyeData.alphaFuncValue = *alphaFuncValue;
	}
	if (simpleLighting) {
		if (*simpleLighting) {
			fbo.fishEyeData.renderFlags |= RENDERFLAG_SIMPLELIGHTING;
		}
		else {
			fbo.fishEyeData.renderFlags &= ~RENDERFLAG_SIMPLELIGHTING;
		}
	}
	if (noLighting) {
		if (*noLighting) {
			fbo.fishEyeData.renderFlags |= RENDERFLAG_NOLIGHTING; // for sky and such
		}
		else {
			fbo.fishEyeData.renderFlags &= ~RENDERFLAG_NOLIGHTING;
		}
	}
	if (stageInfoForMultipass) {
		fbo.fishEyeData.stageImageBitmask = 0;
		fbo.fishEyeData.stageLightmapBitmask = 0;
		fbo.fishEyeData.multiTexMode = stageInfoForMultipass->multitextureEnv;
		for (int i = 0; i < NUM_TEXTURE_BUNDLES; i++) {
			if (stageInfoForMultipass->bundle[i].image[0]) {
				fbo.fishEyeData.stageImageBitmask |= (1 << i);
				if (stageInfoForMultipass->bundle[i].isLightmap) {
					fbo.fishEyeData.stageLightmapBitmask |= (1 << i);
				}
				if (stageInfoForMultipass->bundle[i].deluxeMapImage[0]) {
					int actualIndex = i < 2 ? (2+ NUM_GLSL_EXTRA_LIGHTMAPS_MAX) : (i + 1 + NUM_GLSL_EXTRA_LIGHTMAPS_MAX);
					fbo.fishEyeData.stageImageBitmask |= (1 << actualIndex);
					fbo.fishEyeData.stageLightmapBitmask |= (1 << actualIndex);
				}
			}
		}
		if (r_lightmap->integer && fbo.fishEyeData.stageLightmapBitmask & (1<<1)) {
			fbo.fishEyeData.multiTexMode = GL_REPLACE;
		}
	}
	if (zPrepass) {
		bool mustSwitchProgram = false;
		if (fbo.fishEyeData.doingZPrepass != *zPrepass && fbo.fishEyeActive) {
			mustSwitchProgram = true;
		}
		fbo.fishEyeData.doingZPrepass = *zPrepass;
		if (mustSwitchProgram) {
			R_FrameBuffer_TempDeactivateFisheye(qfalse);
			R_FrameBuffer_ReactivateFisheye();
			uniformsSet = true;
		}
	}

	//if (!uniformsSet) {
		R_FrameBuffer_FishEyeSetUniforms(fbo.fishEyeData.tessellationActive);
	//}

	return qtrue;
#endif
}

qboolean R_FrameBuffer_SetDynamicUniforms2(const bool* haveVertexLightDir, const bool* isModel, surfaceType_t* surfaceType, const int* stageColorGen, const shaderStage_t* stage, const qboolean* stageForceNormal, const bool* nocull, const byte* shaderStyles, unsigned int* stateBitsRaw, unsigned int* stateBitsApplied, const bool* isGore) {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	bool uniformsSet = false;
	if (!(r_fboGLSL->integer && ENABLEGLSL && !r_fboGLSLOff->integer)) {
		return qfalse;
	}

	if (haveVertexLightDir) {
		fbo.fishEyeData.haveVertexLightDirection = *haveVertexLightDir;
	}
	if (isModel) {
		fbo.fishEyeData.isModel = *isModel;
	}
	if (surfaceType) {
		fbo.fishEyeData.surfaceType = *surfaceType;
	}
	if (stageColorGen) {
		fbo.fishEyeData.stageColorGen = *stageColorGen;
	}
	if (stage) {
		fbo.fishEyeData.stageTCGen = stage->bundle[0].image ? stage->bundle[0].tcGen : TCGEN_BAD;
		fbo.fishEyeData.stageHasTCMod = stage->bundle[0].image ? (qboolean)(stage->bundle[0].numTexMods > 0) : qfalse;
	}
	if (stageForceNormal) {
		fbo.fishEyeData.stageForceNormal = *stageForceNormal;
	}
	if (stateBitsRaw) {
		fbo.fishEyeData.stateBitsRaw = *stateBitsRaw;
	}
	if (stateBitsApplied) {
		fbo.fishEyeData.stateBitsApplied = *stateBitsApplied;
	}
	if (nocull) {
		if (*nocull) {
			fbo.fishEyeData.renderFlags |= RENDERFLAG_TWOSIDED; // for grass and foliage and such
		}
		else {
			fbo.fishEyeData.renderFlags &= ~RENDERFLAG_TWOSIDED;
		}
	}
	if (isGore) {
		if (*isGore) {
			fbo.fishEyeData.renderFlags |= RENDERFLAG_ISGORE; // for grass and foliage and such
		}
		else {
			fbo.fishEyeData.renderFlags &= ~RENDERFLAG_ISGORE;
		}
	}
	if (shaderStyles) {
		for (int i = 0; i < MAXLIGHTMAPS_REAL;i++) {
			fbo.fishEyeData.shaderStyles[i] = shaderStyles[i];
		}
	}

	//if (!uniformsSet) {
		R_FrameBuffer_FishEyeSetUniforms(fbo.fishEyeData.tessellationActive);
	//}

	return qtrue;
#endif
}

// ok finally sick of that old method. just set it in fboUniformsEx, then call this.
qboolean R_FrameBuffer_SetDynamicUniforms3() {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if (!(r_fboGLSL->integer && ENABLEGLSL && !r_fboGLSLOff->integer)) {
		return qfalse;
	}

	R_FrameBuffer_FishEyeSetUniforms(fbo.fishEyeData.tessellationActive);

	return qtrue;
#endif
}

qboolean R_FrameBuffer_SetMusicDeformData(float intensity, float time, float spreadSpeed, int sampleAvgWidth, const vec3_t origin, float distanceScale,int mode, float shortDistanceReduction) {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if (!(r_fboGLSL->integer && ENABLEGLSL && !r_fboGLSLOff->integer)) {
		return qfalse;
	}

	fbo.musicDeformData.intensity = intensity;
	fbo.musicDeformData.time = time;
	fbo.musicDeformData.spreadSpeed = spreadSpeed;
	fbo.musicDeformData.sampleAvgWidth = sampleAvgWidth;
	VectorCopy(origin,fbo.musicDeformData.origin);
	fbo.musicDeformData.distanceScale = distanceScale;
	fbo.musicDeformData.mode = mode;
	fbo.musicDeformData.shortDistanceReduction = shortDistanceReduction;

	R_FrameBuffer_FishEyeSetUniforms(fbo.fishEyeData.tessellationActive);

	return qtrue;
#endif
}

qboolean R_FrameBuffer_DeactivateFisheye() {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if (!fishEyeShader || !fishEyeShader->IsWorking())
		return qfalse;


	R_GLSL::UseProgram(0,0);
	fbo.fishEyeActive = qfalse;
	//fbo.fishEyeTempDisabled = 0;

	return qtrue;
#endif
}
qboolean R_FrameBuffer_FishEyeActivateTessellation() {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if (!fishEyeShader || !fishEyeShader->IsWorking())
		return qfalse;

	if (!fishEyeShaderTess || !fishEyeShaderTess->IsWorking())
		return qfalse;

	if (fbo.fishEyeData.tessellationActive) {
		// Already active
	}
	else {

		fbo.fishEyeData.tessellationActive = qtrue;
		R_FrameBuffer_TempDeactivateFisheye(qfalse);
		R_FrameBuffer_ReactivateFisheye();
	}

	return qtrue;
#endif
}
qboolean R_FrameBuffer_FishEyeDeactivateTessellation() {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if (!fishEyeShader || !fishEyeShader->IsWorking())
		return qfalse;

	if (!fishEyeShaderTess || !fishEyeShaderTess->IsWorking())
		return qfalse;

	if (!fbo.fishEyeData.tessellationActive) {
		// Already inactive
	}
	else {

		fbo.fishEyeData.tessellationActive = qfalse;
		R_FrameBuffer_TempDeactivateFisheye(qfalse);
		R_FrameBuffer_ReactivateFisheye();
	}

	return qtrue;
#endif
}

static GLenum fishEyeProcessGLMode(GLenum mode) {
	if (!fbo.fishEyeActive) return mode;

	if (((r_fboFishEye->integer && r_fboFishEyeTessellate->integer) || musicDeformSSBOData || backEnd.viewParms.isSceneView && backEnd.viewParms.sceneView.is360) && mode == GL_TRIANGLES) { // Tessellation only for triangles rn
		if (R_FrameBuffer_FishEyeActivateTessellation()) {
			return GL_PATCHES;
		}
		else {
			return mode;
		}
	}
	else {
		R_FrameBuffer_FishEyeDeactivateTessellation();
		return mode;
	}
}

static void APIENTRY fishEyeDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	dllDrawArraysReal(fishEyeProcessGLMode(mode), first, count);
}

static void APIENTRY fishEyeDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
	dllDrawElementsReal(fishEyeProcessGLMode(mode), count, type, indices);
}

static void APIENTRY fishEyeBegin(GLenum mode)
{
	dllBeginReal(fishEyeProcessGLMode(mode));
}



//two functions to bind and unbind the main framebuffer, generally just to be
//called externaly

void R_SetGL2DSize (int width, int height) {

	// set 2D virtual screen size
	qglViewport( 0, 0, width, height );
	qglScissor( 0, 0, width, height);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho (0, width, height, 0, 0, 1);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();
	//R_FrameBuffer_DeactivateFisheye();
}

void R_BindOwnAttachmentAsTexture(int index, bool makeMipMaps, int attachment) {
#ifdef HAVE_GLES
	//TODO
#else

	if (!(r_fboGLSL->integer && ENABLEGLSL && !r_fboGLSLOff->integer)) {
		return;
	}
	if (attachment < 2) {
		return;
	}

	GLuint sourceBuffer = fbo.main->tertiaryColor;
	switch (attachment) {
		default:
			return;
		case 2: 
			sourceBuffer = fbo.main->tertiaryColor;
			break;
	}

	if (glState.currenttextures[glState.currenttmu] != sourceBuffer) {
		//if (r_fboGLSLFastPreview->integer && !tr.captureIsActive) {

		//	qglBindTexture(GL_TEXTURE_2D, tr.defaultImage->texnum);
		//	glState.currenttextures[glState.currenttmu] = tr.defaultImage->texnum;
		//}
		//else 
		{

			qglBindTexture(GL_TEXTURE_2D, sourceBuffer);
			glState.currenttextures[glState.currenttmu] = sourceBuffer;

			if (attachment == 2) {
				if (makeMipMaps && fbo.main->tertiaryMipmapsGenerated) {
					qglGenerateMipmap(GL_TEXTURE_2D);
					fbo.main->tertiaryMipmapsGenerated = qtrue;
				}
			}
			//else {
			//	if (makeMipMaps && !(fbo.extraViewsMipMapsGenerated & (1 << index))) {
			//		qglGenerateMipmap(GL_TEXTURE_2D);
			//		fbo.extraViewsMipMapsGenerated |= (1 << index);
			//	}
			//}
		}
	};

#endif
}


void R_BindSceneViewImage( int index, bool makeMipMaps, int attachment) {
#ifdef HAVE_GLES
	//TODO
#else
	if (index < 0 || index >= MAX_SCENE_VIEWS) {
		return;
	}

	if (!(r_fboGLSL->integer && ENABLEGLSL && !r_fboGLSLOff->integer)) {
		return;
	}

	GLuint sourceBuffer = attachment == 1 ? fbo.extraViews[index]->secondaryColor : fbo.extraViews[index]->color;

	if ( glState.currenttextures[glState.currenttmu] != sourceBuffer) {
		if (r_fboGLSLFastPreview->integer && !tr.captureIsActive) {

			qglBindTexture(GL_TEXTURE_2D, tr.defaultImage->texnum);
			glState.currenttextures[glState.currenttmu] = tr.defaultImage->texnum;
		}
		else {

			qglBindTexture(GL_TEXTURE_2D, sourceBuffer);
			glState.currenttextures[glState.currenttmu] = sourceBuffer;
			
			if (attachment == 1) {
				if (makeMipMaps && !(fbo.extraViewsSecondaryMipMapsGenerated & (1 << index))) {
					qglGenerateMipmap(GL_TEXTURE_2D);
					fbo.extraViewsSecondaryMipMapsGenerated |= (1 << index);
				}
			}
			else {
				if (makeMipMaps && !(fbo.extraViewsMipMapsGenerated & (1 << index))) {
					qglGenerateMipmap(GL_TEXTURE_2D);
					fbo.extraViewsMipMapsGenerated |= (1 << index);
				}
			}
		}
	};

#endif
}

void R_DrawQuad( GLuint tex, int width, int height, bool forceMakeMipmaps = false) {
#ifdef HAVE_GLES
	//TODO
#else
	int oldTex = -1;
	GL_SelectTexture(0);
	qglEnable(GL_TEXTURE_2D);
	if ( glState.currenttextures[0] != tex ) {
		oldTex = glState.currenttextures[0];
		GL_SelectTexture( 0 );
		qglBindTexture(GL_TEXTURE_2D, tex);
		glState.currenttextures[0] = tex; 

		if (forceMakeMipmaps) {
			qglGenerateMipmap(GL_TEXTURE_2D);
		}
	};

	qglBegin(GL_QUADS);
	  qglTexCoord2f(0.0, 1.0); qglVertex2f(0.0  , 0.0   );	
	  qglTexCoord2f(1.0, 1.0); qglVertex2f(width, 0.0   );	
	  qglTexCoord2f(1.0, 0.0); qglVertex2f(width, height);	
	  qglTexCoord2f(0.0, 0.0); qglVertex2f(0.0  , height);	
	qglEnd();	

	if (oldTex != -1) { // dumb? idk
		qglBindTexture(GL_TEXTURE_2D, oldTex);
		glState.currenttextures[0] = oldTex;
	}
#endif
}



qboolean R_FrameBuffer_SetDrawingShadowPrepass(qboolean doingthat, qboolean clear, qboolean display) {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	float c;
	if (!fishEyeShader || !fishEyeShader->IsWorking())
		return qfalse;

	if (doingthat != fbo.drawingShadowPrepass) {
		fbo.main->tertiaryMipmapsGenerated = qfalse;
	}

	fbo.drawingShadowPrepass = doingthat;

	if (clear) {
		// Clear the buffer so we get the correct shadow info
#if MULTIATTACH
		qglDrawBuffers(2, attachment3);
#else
		qglDrawBuffer(GL_COLOR_ATTACHMENT2_EXT);
#endif
		qglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		qglClear(GL_COLOR_BUFFER_BIT);
	}


#if MULTIATTACH
	R_SetCorrectDrawBuffers();
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif

	if (!fbo.drawingShadowPrepass && display) {
		R_FrameBuffer_TempDeactivateFisheye();
#if MULTIATTACH
		qglDrawBuffers(2, attachment1);
#else
		qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
		c = 1.0f;
		qglColor4f(c, c, c, 1);
		GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHTEST_DISABLE);
		R_SetGL2DSize(glConfig.vidWidth, glConfig.vidHeight);
		R_DrawQuad(fbo.main->tertiaryColor, glConfig.vidWidth, glConfig.vidHeight);
		R_FrameBuffer_ReactivateFisheye();
#if MULTIATTACH
		R_SetCorrectDrawBuffers();
#else
		qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	}

	return qtrue;

#endif
}

void R_FrameBuffer_GenerateMainMipMaps() {
#ifdef HAVE_GLES
	//TODO
#else
	if (mipMapsAlreadyGeneratedThisFrame) {
		return;
	}
	// Create mipmaps if supersampling
	if (superSampleMultiplier != 1 && r_fboSuperSampleMipMap->integer) {
		qglEnable(GL_TEXTURE_2D);
		if (glState.currenttextures[0] != fbo.main->color) {
			GL_SelectTexture(0);
			qglBindTexture(GL_TEXTURE_2D, fbo.main->color);
			glState.currenttextures[0] = fbo.main->color;
			qglGenerateMipmap(GL_TEXTURE_2D);
			mipMapsAlreadyGeneratedThisFrame = qtrue;
		};
	}

#endif
}

void R_DrawQuadPartial(GLuint tex, int width, int height_arg,int offsetX, int offsetY_arg, int Ydivider) {
#ifdef HAVE_GLES
	//TODO
#else
	qglEnable(GL_TEXTURE_2D);
	if (glState.currenttextures[0] != tex) {
		GL_SelectTexture(0);
		qglBindTexture(GL_TEXTURE_2D, tex);
		glState.currenttextures[0] = tex;
	};

	/*float singlePixelTexWidth = 1.0f / (float)glConfig.vidWidth;
	float singlePixelTexHeight = 1.0f / (float)glConfig.vidHeight;
	int x2 = offsetX + width; // use instead of width
	int y2 = offsetY + height; // use instead of height
	int offsetYInverted = glConfig.vidHeight - 1 - offsetY;
	int y2Inverted = glConfig.vidHeight - 1 - y2;*/
	
	// Changed everything to float to support Ydivider for rollingshutter supersample
	float offsetY = (float)offsetY_arg/ (float)Ydivider;
	float height = (float)height_arg/ (float)Ydivider; // TODO Not sure if this will work properly as we will essentially draw lines that are half a pixel thick. Maybe this will not work or affect brightness, in which case we need to compensate that somehow.
	float singlePixelTexWidth = 1.0f / (float)glConfig.vidWidth;
	float singlePixelTexHeight = 1.0f / (float)glConfig.vidHeight;
	float x2 = offsetX + width; // use instead of width
	float y2 = offsetY + height; // use instead of height
	float offsetYInverted = (float)glConfig.vidHeight - 1.0f - offsetY;
	float y2Inverted = (float)glConfig.vidHeight - 1.0f - y2;



	// NOTE: Might also need some switching around of offsetX and X and width but I don't have a use case to test it
	// so I'm leaving it in this possibly broken state bc its good enough for rolling shutter
	// Rolling shutter always reads entire width so there's that...
	qglBegin(GL_QUADS);
	qglTexCoord2f(offsetX*singlePixelTexWidth, y2 * singlePixelTexHeight); qglVertex2f(offsetX, y2Inverted);
	qglTexCoord2f(x2 * singlePixelTexWidth, y2 * singlePixelTexHeight); qglVertex2f(x2, y2Inverted);
	qglTexCoord2f(x2 * singlePixelTexWidth, offsetY * singlePixelTexHeight); qglVertex2f(x2, offsetYInverted);
	qglTexCoord2f(offsetX * singlePixelTexWidth, offsetY * singlePixelTexHeight); qglVertex2f(offsetX, offsetYInverted);
	qglEnd();
#endif
}

static void GetDesiredDepthType(GLenum& requestedType, GLenum& requestedFormat,int& requestedBitDepth, qboolean& isFloat, qboolean packed = qfalse) {
	requestedFormat = packed ? GL_UNSIGNED_INT_24_8_EXT : 0;
	isFloat = qfalse;
	if (!Q_stricmp(r_fboDepthBits->string,"32f")) {
		requestedBitDepth = 32;
		if (glConfig.depthMapFloatNV) {
			requestedType = packed ? GL_DEPTH32F_STENCIL8_NV : GL_DEPTH_COMPONENT32F_NV;
			requestedFormat = packed ? GL_FLOAT_32_UNSIGNED_INT_24_8_REV_NV : GL_FLOAT;
			isFloat = qtrue;
		}
		else if (glConfig.depthMapFloat) {
			requestedType = packed ? GL_DEPTH32F_STENCIL8 : GL_DEPTH_COMPONENT32F;
			requestedFormat = packed ? GL_FLOAT_32_UNSIGNED_INT_24_8_REV : GL_FLOAT;
			isFloat = qtrue;
		}
		else {
			if (packed) {
				// Afaik there is no packed 32 bit depth buffer format.
				ri.Printf(PRINT_WARNING, "Floating point depth buffer requested, but not supported by GPU. Requesting 24 bit packed instead.\n");
				requestedBitDepth = 24;
				requestedType = GL_DEPTH24_STENCIL8_EXT;
				requestedFormat = GL_UNSIGNED_INT_24_8_EXT;
			}
			else {
				ri.Printf(PRINT_WARNING, "Floating point depth buffer requested, but not supported by GPU. Requesting 32 bit instead.\n");
				requestedType = GL_DEPTH_COMPONENT32;
				requestedFormat = GL_UNSIGNED_INT;
			}
		}
	}
	else if(!r_fboDepthBits->integer) { // Nothing requested
		requestedBitDepth = 0;
		requestedType = packed ? GL_DEPTH24_STENCIL8_EXT : GL_DEPTH_COMPONENT;
		requestedFormat = packed ? GL_UNSIGNED_INT_24_8_EXT : GL_UNSIGNED_INT;
	}
	else {
		if (r_fboDepthBits->integer == 32) {
			requestedBitDepth = 32;
			requestedType = packed ? GL_DEPTH32_STENCIL8 : GL_DEPTH_COMPONENT32; // this is WRONG for packed. GL_DEPTH32_STENCIL8 doesn't actually exist, that's a float format, just wrongly named
			requestedFormat = packed ? GL_UNSIGNED_INT_24_8_EXT : GL_UNSIGNED_INT;
		}
		else if (r_fboDepthBits->integer == 24) {
			requestedBitDepth = 24;
			requestedType = packed ? GL_DEPTH24_STENCIL8_EXT : GL_DEPTH_COMPONENT24;
			requestedFormat = packed ? GL_UNSIGNED_INT_24_8_EXT : GL_UNSIGNED_INT;
		}
		else {
			requestedBitDepth = 0;
			requestedType = packed ? GL_DEPTH24_STENCIL8_EXT : GL_DEPTH_COMPONENT;
			requestedFormat = packed ? GL_UNSIGNED_INT_24_8_EXT : GL_UNSIGNED_INT;
			ri.Printf(PRINT_WARNING, "Invalid value of %d for r_fboDepthBits.\n", r_fboDepthBits->integer);
		}
	}
}

/*static qboolean ProcessGLDepthEnumBits(GLenum& bindType) {
	qboolean isDepth = (qboolean)(bindType == GL_DEPTH_COMPONENT || bindType == GL_DEPTH24_STENCIL8_EXT);
	if (isDepth && r_fboDepthBits->integer) {
		if (bindType == GL_DEPTH_COMPONENT) {

			switch (r_fboDepthBits->integer) {
			case 32:
				bindType = GL_DEPTH_COMPONENT32;
				break;
			case 24:
				bindType = GL_DEPTH_COMPONENT24;
				break;
			}
		}
		else if (bindType == GL_DEPTH24_STENCIL8_EXT) {

			switch (r_fboDepthBits->integer) {
			case 32:
				bindType = GL_DEPTH32_STENCIL8;
				break;
			}
		}
	}
	return isDepth;
}*/

static int CreateTextureBuffer( int width, int height, GLenum internalFormat, GLenum format, GLenum type, int superSample, int flags ) {
	int ret = 0;
	int error = qglGetError();
	bool mipmaps = r_fboSuperSampleMipMap->integer && superSample != 1 || (flags & FB_MIPMAP);
	bool filtering = superSample != 1 || (flags & FB_MIPMAP);

	qglGenTextures( 1, (GLuint *)&ret );
	qglBindTexture(	GL_TEXTURE_2D, ret );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, !filtering ?  GL_NEAREST : (mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR) );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (flags & FB_MAGLINEAR) ? GL_LINEAR : GL_NEAREST);

	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (flags & FB_REPEATEDGE) ? GL_REPEAT : GL_CLAMP_TO_EDGE );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (flags & FB_REPEATEDGE) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	qglTexImage2D(	GL_TEXTURE_2D, 0, internalFormat, width* superSample, height* superSample, 0, format, type, 0 );
	if (mipmaps) {
		qglGenerateMipmap(GL_TEXTURE_2D); 
	}
	error = qglGetError();
	return ret;
}

static int CreateRenderBuffer( int samples, int width, int height, GLenum bindType, int superSample) {
	int ret = 0;
#ifdef HAVE_GLES
	//TODO
#else
	qglGenRenderbuffers( 1, (GLuint *)&ret );
	qglBindRenderbuffer( GL_RENDERBUFFER_EXT, ret );

	if ( samples ) {
		qglRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER_EXT, samples, bindType, width* superSample, height * superSample);
	} else {
		qglRenderbufferStorage(	GL_RENDERBUFFER_EXT, bindType, width * superSample, height * superSample);
	}

#endif
	return ret;
}

//------------------------------
// better framebuffer creation
//------------------------------
// for this we do a more opengl way of figuring out what level of framebuffer
// objects are supported. we try each mode from 'best' to 'worst' until we 
// get a mode that works.


void R_FrameBufferDelete( frameBufferData_t* buffer ) {  
#ifdef HAVE_GLES
	//TODO
#else
	if ( !buffer )
		return;
	qglDeleteFramebuffers(1, &(buffer->fbo));
	if ( buffer->color ) {
		if ( buffer->flags & FB_MULTISAMPLE ) {
			qglDeleteRenderbuffers(1, &(buffer->color) );
		} else {
			qglDeleteTextures( 1, &(buffer->color) );
		}
	}
	if ( buffer->depth ) {
		qglDeleteRenderbuffers(1, &(buffer->depth) );
	}
	if ( buffer->stencil ) {
		qglDeleteRenderbuffers(1, &(buffer->stencil) );
	}
	if ( buffer->packed ) {
		if ( buffer->flags & FB_MULTISAMPLE ) {
			qglDeleteRenderbuffers(1, &(buffer->packed) );
		} else {
			qglDeleteTextures( 1, &(buffer->packed) );
		}
	}
	free( buffer );
#endif
}

frameBufferData_t* R_FrameBufferCreate( int width, int height, int flags, int superSample=1 ) {
#ifdef HAVE_GLES
	//TODO
	return NULL;
#else
	frameBufferData_t *buffer;
	GLuint status;
	GLenum desiredDepthType,desiredDepthFormat;
	qboolean desiringFloatDepth;
	int desiredDepthBitDepth = 0;
	int samples = 0;

	buffer = (frameBufferData_t *)malloc( sizeof( *buffer ) );
	memset( buffer, 0, sizeof( *buffer ) );
	buffer->flags = flags;
	buffer->width = width;
	buffer->height = height;

	//gen the frame buffer
	qglGenFramebuffers(1, &(buffer->fbo) );
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, buffer->fbo);

	if ( flags & FB_MULTISAMPLE ) {
		samples = r_fboMultiSample->integer;
	}

	if ( flags & FB_PACKED ) {
		GetDesiredDepthType(desiredDepthType, desiredDepthFormat,desiredDepthBitDepth, desiringFloatDepth, qtrue);
		if ( samples ) {
			buffer->packed = CreateRenderBuffer( samples, width, height, desiredDepthType, superSample );
			qglFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, buffer->packed );
			qglFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, buffer->packed );
		} else {
			// Setup depth_stencil texture (not mipmap)
			buffer->packed = CreateTextureBuffer( width, height, desiredDepthType, GL_DEPTH_STENCIL_EXT, desiredDepthFormat,superSample, flags);
			qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, buffer->packed, 0);
			qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, buffer->packed, 0);
		}
	} else {
		GetDesiredDepthType(desiredDepthType, desiredDepthFormat, desiredDepthBitDepth, desiringFloatDepth, qfalse);
		if (1 /*samples*/) {
			if (flags & FB_DEPTH) {
				buffer->depth = CreateRenderBuffer(samples, width, height, desiredDepthType, superSample);
				qglFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, buffer->depth);
			}
			if (flags & FB_STENCIL) {
				buffer->stencil = CreateRenderBuffer(samples, width, height, GL_STENCIL_INDEX8_EXT, superSample);
				qglFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, buffer->stencil);
			}
		}
		/*else { // This was just an attempt. Didn't really help with anything
			if (flags & FB_DEPTH) {
				//buffer->depth = CreateRenderBuffer(samples, width, height, desiredDepthType, superSample);
				buffer->depth = CreateTextureBuffer(width, height, desiredDepthType, GL_DEPTH_COMPONENT, desiredDepthFormat, superSample);
				//buffer->depth = CreateTextureBuffer(width, height, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, superSample);
				qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, buffer->depth,0);
			}
			if (flags & FB_STENCIL) {
				//buffer->stencil = CreateRenderBuffer(samples, width, height, GL_STENCIL_INDEX8_EXT, superSample);
				buffer->stencil = CreateTextureBuffer(width, height, GL_STENCIL_INDEX8_EXT, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, superSample);
				qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, buffer->stencil,0);
			}
		}*/		
	}
	/* Attach the color buffer */
	
	for (int i = 0; i < 3; i++) {
		GLuint* renderBuffer = &buffer->color;
		GLenum attachment = GL_COLOR_ATTACHMENT0_EXT;
		if (i == 1) {
			if (flags & FB_SECONDARYBUFFER) {
				renderBuffer = &buffer->secondaryColor; 
				attachment = GL_COLOR_ATTACHMENT1_EXT;
			}
			else {
				buffer->secondaryColor = 0;
				continue;
			}
		}
		if (i == 2) {
			if (flags & FB_TERTIARYBUFFER) {
				renderBuffer = &buffer->tertiaryColor; 
				attachment = GL_COLOR_ATTACHMENT2_EXT;
			}
			else {
				buffer->tertiaryColor = 0;
				continue;
			}
		}

		if (samples) {
			*renderBuffer = CreateRenderBuffer(samples, width, height, GL_RGBA, superSample);
			qglFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, attachment, GL_RENDERBUFFER_EXT, *renderBuffer);
		}
		else if (flags & FB_FLOAT16) {
			*renderBuffer = CreateTextureBuffer(width, height, RGBA16F_ARB, GL_RGBA, GL_FLOAT, superSample, flags);
			qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_2D, *renderBuffer, 0);
		}
		else if (flags & FB_FLOAT32) {
			*renderBuffer = CreateTextureBuffer(width, height, RGBA32F_ARB, GL_RGBA, GL_FLOAT, superSample, flags);
			qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_2D, *renderBuffer, 0);
		}
		else {
			*renderBuffer = CreateTextureBuffer(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, superSample, flags);
			qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_2D, *renderBuffer, 0);
		}
	}
		
	status = qglCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);

	if ( status != GL_FRAMEBUFFER_COMPLETE_EXT ) {
		switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            ri.Printf( PRINT_ALL, "Unsupported framebuffer format\n" );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            ri.Printf( PRINT_ALL, "Framebuffer incomplete, missing attachment\n" );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            ri.Printf( PRINT_ALL, "Framebuffer incomplete, duplicate attachment\n" );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            ri.Printf( PRINT_ALL, "Framebuffer incomplete, attached images must have same dimensions\n" );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
            ri.Printf( PRINT_ALL, "Framebuffer incomplete, attached images must have same format\n" );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            ri.Printf( PRINT_ALL, "Framebuffer incomplete, missing draw buffer\n" );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            ri.Printf( PRINT_ALL, "Framebuffer incomplete, missing read buffer\n" );
            break;
	    }
		R_FrameBufferDelete( buffer );
		return 0;
	}

	GLint depthBufferBits;
	qglGetIntegerv(GL_DEPTH_BITS, &depthBufferBits);
	if (flags & FB_PACKED || flags & FB_DEPTH) {
		if (desiredDepthBitDepth && desiredDepthBitDepth != depthBufferBits) {
			ri.Printf(PRINT_WARNING, "FBO: Tried to create %d bit depth buffer, OpenGL used %d bits instead.\n", r_fboDepthBits->integer, depthBufferBits);
		}
		else {
			if (desiredDepthBitDepth == 32 && desiringFloatDepth && glConfig.depthMapFloatNV) {
				glConfig.depthMapFloatNVActive = qtrue;
			}
			if (glConfig.depthMapFloatNVActive || r_zinvert->integer > 1) {
				// Activate zinvert trick for nvidia cards. Scaling the depth map to -1:1 instead of 0:1 along with a changed projection matrix (elsewhere in code).
				// This relies on detecting the nvidia extension but r_zinvert > 1 can force it since technically other GPUs are allowed not to clamp those values now (?)
				// however most GPUs likely still do.
				qglDepthRange = dllDepthRange = depthRangeScaledNV;
			}
			
			ri.Printf(PRINT_ALL, "FBO: %d bit depth buffer created.\n", depthBufferBits);
		}
	}



	return buffer;
#endif
}

static void R_FrameBufferInitUniformLocs(R_GLSL* program,uniformLocations_t* locs) {
	for (int i = 0; i < GLSLSHAD_MAX; i++) {
		locs->viewOriginUniform.getUniformLocation(program->ShaderIdByBits(i), "viewOriginUniform");
		locs->pixelJitterUniform.getUniformLocation(program->ShaderIdByBits(i), "pixelJitterUniform");
		locs->dofJitterUniform.getUniformLocation(program->ShaderIdByBits(i), "dofJitterUniform");
		locs->dofFocusUniform.getUniformLocation(program->ShaderIdByBits(i), "dofFocusUniform");
		locs->dofRadiusUniform.getUniformLocation(program->ShaderIdByBits(i), "dofRadiusUniform");
		locs->fishEyeModeUniform.getUniformLocation(program->ShaderIdByBits(i), "fishEyeModeUniform");
		locs->fovXUniform.getUniformLocation(program->ShaderIdByBits(i), "fovXUniform");
		locs->fovYUniform.getUniformLocation(program->ShaderIdByBits(i), "fovYUniform");
		locs->pixelWidthUniform.getUniformLocation(program->ShaderIdByBits(i), "pixelWidthUniform");
		locs->pixelHeightUniform.getUniformLocation(program->ShaderIdByBits(i), "pixelHeightUniform");
		locs->jitterIndexUniform.getUniformLocation(program->ShaderIdByBits(i), "jitterIndexUniform");
		locs->jitterTotalFramesUniform.getUniformLocation(program->ShaderIdByBits(i), "jitterTotalFramesUniform");
		locs->texAverageBrightnessUniform.getUniformLocation(program->ShaderIdByBits(i), "texAverageBrightnessUniform");
		locs->isLightmapUniform.getUniformLocation(program->ShaderIdByBits(i), "isLightmapUniform");
		locs->isWorldBrushUniform.getUniformLocation(program->ShaderIdByBits(i), "isWorldBrushUniform");
		locs->isSaberUniform.getUniformLocation(program->ShaderIdByBits(i), "isSaberUniform");
		locs->parallaxMapLayersUniform.getUniformLocation(program->ShaderIdByBits(i), "parallaxMapLayersUniform");
		locs->parallaxMapDepthUniform.getUniformLocation(program->ShaderIdByBits(i), "parallaxMapDepthUniform");
		locs->parallaxMapGammaUniform.getUniformLocation(program->ShaderIdByBits(i), "parallaxMapGammaUniform");
		locs->thermalVisionUniform.getUniformLocation(program->ShaderIdByBits(i), "thermalVisionUniform");
		locs->shaderDebugUniform.getUniformLocation(program->ShaderIdByBits(i), "shaderDebugUniform");
		locs->blurEarlyStageUniform.getUniformLocation(program->ShaderIdByBits(i), "blurEarlyStageUniform");
		locs->serverTimeUniform.getUniformLocation(program->ShaderIdByBits(i), "serverTimeUniform");
		locs->serverTimeStartUniform.getUniformLocation(program->ShaderIdByBits(i), "serverTimeStartUniform");
		locs->serverTimeFractionUniform.getUniformLocation(program->ShaderIdByBits(i), "serverTimeFractionUniform");
		locs->noiseFuckeryUniform.getUniformLocation(program->ShaderIdByBits(i), "noiseFuckeryUniform");
		locs->noiseFuckeryLightmapUniform.getUniformLocation(program->ShaderIdByBits(i), "noiseFuckeryLightmapUniform");
		locs->noiseFuckeryLightmapIntensityUniform.getUniformLocation(program->ShaderIdByBits(i), "noiseFuckeryLightmapIntensityUniform");
		locs->noiseFuckeryHDRIntensityUniform.getUniformLocation(program->ShaderIdByBits(i), "noiseFuckeryHDRIntensityUniform");
		locs->worldModelViewMatrixUniform.getUniformLocation(program->ShaderIdByBits(i), "worldModelViewMatrixUniform");
		locs->soundDeformSampleRateUniform.getUniformLocation(program->ShaderIdByBits(i), "soundDeformSampleRateUniform");
		locs->soundDeformSampleCountUniform.getUniformLocation(program->ShaderIdByBits(i), "soundDeformSampleCountUniform");

		for (int j = 0; j < 6; j++) {
			locs->clipPlanesUniform[j].getUniformLocation(program->ShaderIdByBits(i), va("clipPlanesUniform[%d]",j));
		}

		locs->projectorModelViewMatrixUniform.getUniformLocation(program->ShaderIdByBits(i), "projectorModelViewMatrixUniform");
		locs->projectorProjectionMatrixUniform.getUniformLocation(program->ShaderIdByBits(i), "projectorProjectionMatrixUniform");
		locs->projectorPosUniform.getUniformLocation(program->ShaderIdByBits(i), "projectorPosUniform");
		locs->projectorActiveUniform.getUniformLocation(program->ShaderIdByBits(i), "projectorActiveUniform");

		locs->soundDeformTimeUniform.getUniformLocation(program->ShaderIdByBits(i), "soundDeformTimeUniform");
		locs->soundDeformIntensityUniform.getUniformLocation(program->ShaderIdByBits(i), "soundDeformIntensityUniform");
		locs->soundDeformSpreadSpeedUniform.getUniformLocation(program->ShaderIdByBits(i), "soundDeformSpreadSpeedUniform");
		locs->soundDeformSampleAvgWidthUniform.getUniformLocation(program->ShaderIdByBits(i), "soundDeformSampleAvgWidthUniform");
		locs->soundDeformOriginUniform.getUniformLocation(program->ShaderIdByBits(i), "soundDeformOriginUniform");
		locs->soundDeformDistanceScaleUniform.getUniformLocation(program->ShaderIdByBits(i), "soundDeformDistanceScaleUniform");
		locs->soundDeformShortDistanceReductionUniform.getUniformLocation(program->ShaderIdByBits(i), "soundDeformShortDistanceReductionUniform");
		locs->soundDeformModeUniform.getUniformLocation(program->ShaderIdByBits(i), "soundDeformModeUniform");

		locs->alphaFuncUniform.getUniformLocation(program->ShaderIdByBits(i), "alphaFuncUniform");
		locs->alphaFuncValueUniform.getUniformLocation(program->ShaderIdByBits(i), "alphaFuncValueUniform");
		locs->renderFlagsUniform.getUniformLocation(program->ShaderIdByBits(i), "renderFlagsUniform");

		locs->zPrepassUniform.getUniformLocation(program->ShaderIdByBits(i), "zPrepassUniform");

		locs->deluxeMappingUniform.getUniformLocation(program->ShaderIdByBits(i), "deluxeMappingUniform");

		locs->haveVertexLightDirectionUniform.getUniformLocation(program->ShaderIdByBits(i), "haveVertexLightDirectionUniform");
		locs->isModelUniform.getUniformLocation(program->ShaderIdByBits(i), "isModelUniform");
		locs->surfaceTypeUniform.getUniformLocation(program->ShaderIdByBits(i), "surfaceTypeUniform");
		locs->stageColorGenUniform.getUniformLocation(program->ShaderIdByBits(i), "stageColorGenUniform");
		locs->stageForceNormalUniform.getUniformLocation(program->ShaderIdByBits(i), "stageForceNormalUniform");

		locs->stageTCGenUniform.getUniformLocation(program->ShaderIdByBits(i), "stageTCGenUniform");
		locs->stageHasTCModUniform.getUniformLocation(program->ShaderIdByBits(i), "stageHasTCModUniform");
		locs->gigaTCGenUniform.getUniformLocation(program->ShaderIdByBits(i), "gigaTCGenUniform");

		locs->rawStateBitsUniform.getUniformLocation(program->ShaderIdByBits(i), "rawStateBitsUniform");
		locs->appliedStateBitsUniform.getUniformLocation(program->ShaderIdByBits(i), "appliedStateBitsUniform");

		locs->stageImageBitmaskUniform.getUniformLocation(program->ShaderIdByBits(i), "stageImageBitmaskUniform");
		locs->stageLightmapBitmaskUniform.getUniformLocation(program->ShaderIdByBits(i), "stageLightmapBitmaskUniform");
		locs->bindingRectImageBitmaskUniform.getUniformLocation(program->ShaderIdByBits(i), "bindingRectImageBitmaskUniform");
		locs->multiTexModeUniform.getUniformLocation(program->ShaderIdByBits(i), "multiTexModeUniform");

		for (int j = 0; j < NUM_TEXTURE_SAMPLERS; j++) {
			locs->text_in[j].getUniformLocation(program->ShaderIdByBits(i), va("text_in[%d]",j));
			locs->lightmapNumsUniform[j].getUniformLocation(program->ShaderIdByBits(i), va("lightmapNumsUniform[%d]",j));
		}
		locs->text_inArray31.getUniformLocation(program->ShaderIdByBits(i), "text_inArray31");

		locs->cloudScaleUniform.getUniformLocation(program->ShaderIdByBits(i), "cloudScaleUniform");
		locs->cloudTimeScaleUniform.getUniformLocation(program->ShaderIdByBits(i), "cloudTimeScaleUniform");
		locs->cloudPowerUniform.getUniformLocation(program->ShaderIdByBits(i), "cloudPowerUniform");
		locs->cloudIntensityCompensateUniform.getUniformLocation(program->ShaderIdByBits(i), "cloudIntensityCompensateUniform");

		locs->myFogUniform.getUniformLocation(program->ShaderIdByBits(i), "myFogUniform");
		locs->myFogColorUniform.getUniformLocation(program->ShaderIdByBits(i), "myFogColorUniform");

		locs->dLightFastUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightFastUniform");
		locs->dLightJitterUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightJitterUniform");
		locs->dLightVoxelShadowsUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightVoxelShadowsUniform");
		locs->dLightVoxelShadowJitterUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightVoxelShadowJitterUniform");
		locs->dLightVoxelShadowJitterMethodUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightVoxelShadowJitterMethodUniform");
		locs->dLightSpecGammaUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightSpecGammaUniform");
		locs->dLightSpecIntensityUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightSpecIntensityUniform");
		locs->dLightSpecBaseReflectivityUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightSpecBaseReflectivityUniform");
		locs->dLightSpecDistanceDecayUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightSpecDistanceDecayUniform");
		locs->dLightSpecDistanceMinUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightSpecDistanceMinUniform");
		locs->dLightIntensityUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightIntensityUniform");
		locs->dLightsCountUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightsCountUniform");
		locs->dLightFastSkipThresholdUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightFastSkipThresholdUniform");
		locs->dLightAddPowUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightAddPowUniform");
		locs->dLightAddPostPowMultUniform.getUniformLocation(program->ShaderIdByBits(i), "dLightAddPostPowMultUniform");
		for (int j = 0; j < MAX_DLIGHTS; j++) {
			locs->dLightsUniformColor[j].getUniformLocation(program->ShaderIdByBits(i), va("dLightsUniform[%d].color",j));
			locs->dLightsUniformOrigin[j].getUniformLocation(program->ShaderIdByBits(i), va("dLightsUniform[%d].origin",j));
			locs->dLightsUniformRadius[j].getUniformLocation(program->ShaderIdByBits(i), va("dLightsUniform[%d].radius",j));
			locs->dLightsUniformMindist[j].getUniformLocation(program->ShaderIdByBits(i), va("dLightsUniform[%d].mindist",j));
		}
		for (int j = 0; j < MAXLIGHTMAPS_REAL; j++) {
			locs->shaderStylesUniform[j].getUniformLocation(program->ShaderIdByBits(i), va("shaderStylesUniform[%d]",j));
		}
		locs->shadowLinesCountUniform.getUniformLocation(program->ShaderIdByBits(i), "shadowLinesCountUniform");
		locs->cheapLightsCountUniform.getUniformLocation(program->ShaderIdByBits(i), "cheapLightsCountUniform");
		for (int j = 0; j < MAX_SHADOWLINES; j++) {
			locs->shadowLinesPoint1[j].getUniformLocation(program->ShaderIdByBits(i), va("shadowLinesUniform[%d].point1",j));
			locs->shadowLinesPoint2[j].getUniformLocation(program->ShaderIdByBits(i), va("shadowLinesUniform[%d].point2",j));
			locs->shadowLinesWidth[j].getUniformLocation(program->ShaderIdByBits(i), va("shadowLinesUniform[%d].width",j));
			locs->shadowLinesA[j].getUniformLocation(program->ShaderIdByBits(i), va("shadowLinesUniform[%d].a",j));
			locs->shadowLinesB[j].getUniformLocation(program->ShaderIdByBits(i), va("shadowLinesUniform[%d].b",j));
		}

		locs->worldReflectNormalMixUniform.getUniformLocation(program->ShaderIdByBits(i), "worldReflectNormalMixUniform");
		locs->worldReflectGradMultUniform.getUniformLocation(program->ShaderIdByBits(i), "worldReflectGradMultUniform");
		locs->worldReflectPuddleThreshUniform.getUniformLocation(program->ShaderIdByBits(i), "worldReflectPuddleThreshUniform");
		locs->worldReflectMultiSampleUniform.getUniformLocation(program->ShaderIdByBits(i), "worldReflectMultiSampleUniform");

		locs++;
	}
}

static void ReLoadGLSL() {
	qboolean wasActive = qfalse;
	if (r_fboGLSL->integer && ENABLEGLSL) {

		if (fbo.fishEyeActive) {
			R_GLSL::UseProgram(0,0);
			wasActive = qtrue;
			fbo.fishEyeActive = qfalse;
		}

		if (fishEyeShader) {
			delete fishEyeShader;
			fishEyeShader = NULL;
		}
		if (fishEyeShaderTess) {
			delete fishEyeShaderTess;
			fishEyeShaderTess = NULL;
		}
		if (thermalPostProcessingShader) {
			delete thermalPostProcessingShader;
			thermalPostProcessingShader = NULL;
		}


		thermalPostProcessingShader = new R_GLSL("glsl/thermal-vertex.glsl", "", "", "", "glsl/thermal-fragment.glsl", qfalse);
		if (!thermalPostProcessingShader->IsWorking()) {
			ri.Printf(PRINT_WARNING, "WARNING: Thermal post processing Shader could not be compiled. Thermal vision post pro disabled.\n");
		}
		else {
			R_FrameBufferInitUniformLocs(thermalPostProcessingShader, uniformLocationsPostProcessing);
		}

		qglBegin = dllBegin = dllBeginReal;
		qglDrawArrays = dllDrawArrays = dllDrawArraysReal;
		qglDrawElements = dllDrawElements = dllDrawElementsReal;

		fishEyeShader = new R_GLSL("glsl/fisheye-vertex.glsl", "", "", "glsl/fisheye-geom.glsl", "glsl/fisheye-fragment.glsl", qfalse);
		if (!fishEyeShader->IsWorking()) {
			ri.Printf(PRINT_WARNING, "WARNING: Fisheye shader could not be compiled. Fisheye mode not available.\n");
		}
		else {

			R_FrameBufferInitUniformLocs(fishEyeShader, uniformLocationsArr);

			fishEyeShaderTess = new R_GLSL("glsl/fisheye-vertex.glsl", "glsl/fisheye-tessellation-control.glsl", "glsl/fisheye-tessellation-evaluation.glsl", "glsl/fisheye-geom.glsl", "glsl/fisheye-fragment.glsl", qfalse);
			if (!fishEyeShaderTess->IsWorking()) {
				ri.Printf(PRINT_WARNING, "WARNING: Fisheye shader with tessellation could not be compiled. Fisheye mode will not use hardware tessellation and will require tessellated maps instead.\n");
			}
			else {

				R_FrameBufferInitUniformLocs(fishEyeShaderTess, uniformLocationsTessArr);
				// Ok, we want tessellation. Let's intercept all drawing calls and convert them into GL_PATCHES calls if they are GL_TRIANGLES (or maybe later some other types we wanna support)
				qglBegin = dllBegin = fishEyeBegin;
				qglDrawArrays = dllDrawArrays = fishEyeDrawArrays;
				qglDrawElements = dllDrawElements = fishEyeDrawElements;
				if (wasActive) {

					(fbo.fishEyeData.tessellationActive ? fishEyeShaderTess : fishEyeShader)->UseThisProgram(R_FrameBuffer_GetShaderbits());
					fbo.fishEyeActive = qtrue;

					R_FrameBuffer_FishEyeSetUniforms(fbo.fishEyeData.tessellationActive);
				}
			}
		}
}
}

void R_FrameBuffer_Init( void ) {
#ifdef HAVE_GLES
	//TODO
#else
	int flags, width, height;
	GLenum tmp;

	fbo.fishEyeActive = qfalse;
	fbo.fishEyeData.tessellationActive = qfalse;
	fbo.fishEyeTempDisabled = 0;
	fbo.soundDeformSampleCount = 0;
	fbo.soundDeformSampleRate = 44100;

	memset( &fbo, 0, sizeof( fbo ) ); 
	memset(&fboUniformsEx, 0, sizeof(fboUniformsEx));
	r_fbo = ri.Cvar_Get( "r_fbo", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_fboGLSL = ri.Cvar_Get( "r_fboGLSL", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_fboGLSLOff = ri.Cvar_Get( "r_fboGLSLOff", "0", CVAR_TEMP );
	r_fboGLSLNoiseFuckery = ri.Cvar_Get( "r_fboGLSLNoiseFuckery", "5", CVAR_ARCHIVE);
	r_fboGLSLNoiseFuckeryLightmap = ri.Cvar_Get( "r_fboGLSLNoiseFuckeryLightmap", "0", CVAR_ARCHIVE);
	r_fboGLSLNoiseFuckeryLightmapIntensity = ri.Cvar_Get( "r_fboGLSLNoiseFuckeryLightmapIntensity", "1.0", CVAR_ARCHIVE);
	r_fboGLSLNoiseFuckeryHDRIntensity = ri.Cvar_Get( "r_fboGLSLNoiseFuckeryHDRIntensity", "1.0", CVAR_ARCHIVE);
	//r_fboGLSLParallaxMappingIntensity = ri.Cvar_Get( "r_fboGLSLParallaxMappingIntensity", "1.0", CVAR_ARCHIVE); // not used atm
	r_fboGLSLParallaxMappingDepth = ri.Cvar_Get( "r_fboGLSLParallaxMappingDepth", "10.0", CVAR_ARCHIVE);
	r_fboGLSLParallaxMappingGamma = ri.Cvar_Get( "r_fboGLSLParallaxMappingGamma", "10.0", CVAR_ARCHIVE);
	r_fboGLSLParallaxMappingLayers = ri.Cvar_Get( "r_fboGLSLParallaxMappingLayers", "200", CVAR_ARCHIVE);
	r_fboGLSLWorldReflectNormalMix = ri.Cvar_Get( "r_fboGLSLWorldReflectNormalMix", "0.2", CVAR_ARCHIVE);
	r_fboGLSLWorldReflectGradMult = ri.Cvar_Get( "r_fboGLSLWorldReflectGradMult", "1.0", CVAR_ARCHIVE);
	r_fboGLSLWorldReflectMultiSample = ri.Cvar_Get( "r_fboGLSLWorldReflectMultiSample", "0", CVAR_ARCHIVE);
	r_fboGLSLWorldReflectPuddleTresh = ri.Cvar_Get( "r_fboGLSLWorldReflectPuddleTresh", "0.5", CVAR_ARCHIVE);
	r_fboGLSLShaderDebug = ri.Cvar_Get( "r_fboGLSLShaderDebug", "0", CVAR_TEMP);
	r_fboGLSLThermalVision = ri.Cvar_Get( "r_fboGLSLThermalVision", "0", CVAR_TEMP);
	r_fboGLSLCloudShadowScale = ri.Cvar_Get( "r_fboGLSLCloudShadowScale", "1.0", CVAR_ARCHIVE);
	r_fboGLSLCloudShadowTimeScale = ri.Cvar_Get( "r_fboGLSLCloudShadowTimeScale", "1.5", CVAR_ARCHIVE);
	r_fboGLSLCloudShadowPower = ri.Cvar_Get( "r_fboGLSLCloudShadowPower", "0.7", CVAR_ARCHIVE);
	r_fboGLSLCloudIntensityCompensate = ri.Cvar_Get( "r_fboGLSLCloudIntensityCompensate", "1.0", CVAR_ARCHIVE);
	r_fboGLSLFog = ri.Cvar_Get( "r_fboGLSLFog", "0.0", CVAR_ARCHIVE);
	r_fboGLSLFogColor = ri.Cvar_Get( "r_fboGLSLFogColor", "0.5 0.5 0.5", CVAR_ARCHIVE);
	r_fboGLSLFogColor->modified = qtrue;
	r_fboGLSLPreviewSecondary = ri.Cvar_Get( "r_fboGLSLPreviewSecondary", "0", CVAR_TEMP );
	r_fboGLSLDLights = ri.Cvar_Get( "r_fboGLSLDLights", "1", CVAR_ARCHIVE );
	r_fboGLSLDLightsFast = ri.Cvar_Get( "r_fboGLSLDLightsFast", "1", CVAR_ARCHIVE );
	r_fboGLSLDLightsVoxelShadows = ri.Cvar_Get( "r_fboGLSLDLightsVoxelShadows", "1", CVAR_ARCHIVE );
	r_fboGLSLDLightsSpecIntensity = ri.Cvar_Get( "r_fboGLSLDLightsSpecIntensity", "5.0", CVAR_ARCHIVE);
	r_fboGLSLDLightsSpecBaseReflectivity = ri.Cvar_Get( "r_fboGLSLDLightsSpecBaseReflectivity", "0.1", CVAR_ARCHIVE);
	r_fboGLSLDLightsSpecDistanceDecay = ri.Cvar_Get( "r_fboGLSLDLightsSpecDistanceDecay", "300.0", CVAR_ARCHIVE);
	r_fboGLSLDLightsSpecDistanceMinUniform = ri.Cvar_Get( "r_fboGLSLDLightsSpecDistanceMin", "200.0", CVAR_ARCHIVE);
	r_fboGLSLDLightsIntensity = ri.Cvar_Get( "r_fboGLSLDLightsIntensity", "1.0", CVAR_ARCHIVE);
	r_fboGLSLDLightsSpecGamma = ri.Cvar_Get( "r_fboGLSLDLightsSpecGamma", "5.0", CVAR_ARCHIVE);
	r_fboGLSLDLightsAddPow = ri.Cvar_Get( "r_fboGLSLDLightsAddPow", "0.7", CVAR_ARCHIVE);
	r_fboGLSLDLightsAddPostPowMult = ri.Cvar_Get( "r_fboGLSLDLightsAddPostPowMult", "0.8", CVAR_ARCHIVE);
	r_fboGLSLDLightsFastSkipThreshold = ri.Cvar_Get( "r_fboGLSLDLightsFastSkipThreshold", "0.00001", CVAR_ARCHIVE);
	r_fboGLSLFastPreview = ri.Cvar_Get( "r_fboGLSLFastPreview", "1", CVAR_ARCHIVE);
	r_fboGLSLParallaxMapping = ri.Cvar_Get( "r_fboGLSLParallaxMapping", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_fboGLSLGigaTCGen = ri.Cvar_Get( "r_fboGLSLGigaTCGen", "1", CVAR_ARCHIVE);
	r_fboGLSLProjector = ri.Cvar_Get( "r_fboGLSLProjector", "0", CVAR_ARCHIVE);
	r_fboGLSLProjectorWorldShadow = ri.Cvar_Get( "r_fboGLSLProjectorWorldShadow", "1", CVAR_ARCHIVE);
	r_fboGLSLProjectorShader = ri.Cvar_Get( "r_fboGLSLProjectorShader", "textures/doomgiver/mapd2", CVAR_ARCHIVE);
	r_fboGLSLProjectorPos = ri.Cvar_Get( "r_fboGLSLProjectorPos", "-588 4516 216", CVAR_ARCHIVE);
	r_fboGLSLProjectorAng = ri.Cvar_Get( "r_fboGLSLProjectorAng", "0 90 0", CVAR_ARCHIVE);
	r_fboGLSLProjectorFov = ri.Cvar_Get( "r_fboGLSLProjectorFov", "40 30", CVAR_ARCHIVE);
	r_fboGLSLProjectorShader->modified = r_fboGLSLProjectorPos->modified = r_fboGLSLProjectorAng->modified = r_fboGLSLProjectorFov->modified = qtrue;
	r_fboFishEye = ri.Cvar_Get( "r_fboFishEye", "0", CVAR_ARCHIVE);
	r_fboFishEyeNormalBlend = ri.Cvar_Get( "r_fboFishEyeNormalBlend", "0.0", CVAR_ARCHIVE);
	r_fboFishEyeTessellate = ri.Cvar_Get( "r_fboFishEyeTessellate", "1", CVAR_ARCHIVE);
	r_fboDepthBits = ri.Cvar_Get( "r_fboDepthBits", "32f", CVAR_ARCHIVE | CVAR_LATCH);
	r_fboDepthPacked = ri.Cvar_Get( "r_fboDepthPacked", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_fboStencilWhenNotPacked = ri.Cvar_Get( "r_fboStencilWhenNotPacked", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_fboExposure = ri.Cvar_Get("r_fboExposure", "1.0", CVAR_ARCHIVE);
	r_fboCompensateSkyTint = ri.Cvar_Get("r_fboCompensateSkyTint", "0", CVAR_ARCHIVE);
	r_fboSuperSample = ri.Cvar_Get( "r_fboSuperSample", "0", CVAR_ARCHIVE | CVAR_LATCH);	
	r_fboSuperSampleMipMap = ri.Cvar_Get( "r_fboSuperSampleMipMap", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_fboRollingShutterSuperSample = ri.Cvar_Get("r_fboRollingShutterSuperSample", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_fboBlur = ri.Cvar_Get( "r_fboBlur", "2", CVAR_ARCHIVE | CVAR_LATCH);	
	r_fboWidth = ri.Cvar_Get( "r_fboWidth", "0", CVAR_ARCHIVE | CVAR_LATCH);	
	r_fboHeight = ri.Cvar_Get( "r_fboHeight", "0", CVAR_ARCHIVE | CVAR_LATCH);	
	r_glDepthClamp = ri.Cvar_Get( "r_glDepthClamp", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_fboMultiSample = ri.Cvar_Get( "r_fboMultiSample", "0", CVAR_ARCHIVE | CVAR_LATCH);	
	r_floatBuffer = ri.Cvar_Get( "r_floatBuffer", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_convertToHDR = ri.Cvar_Get( "r_convertToHDR", "1", CVAR_ARCHIVE | CVAR_LATCH);

	superSampleMultiplier = pow(2, r_fboSuperSample->integer);
	rollingShutterSuperSampleMultiplier = pow(2, min(r_fboRollingShutterSuperSample->integer, r_fboSuperSample->integer)); // Can't go any higher than normal supersample.

	fbo.soundDeformSampleRate = 44100; // safety

	// make sure all the commands added here are also		

	if (!glMMEConfig.framebufferObject ) {
		ri.Printf( PRINT_WARNING, "WARNING: Framebuffer rendering path disabled (no FBO support)\n");
		return;
	} else if (!glMMEConfig.shaderSupport) {
		ri.Printf(PRINT_WARNING, "WARNING: Framebuffer rendering path disabled (no shader support)\n");
		return;
	}

	if ( !r_fbo->integer ) {
		return;
	}

	ri.Printf( PRINT_ALL, "----- Enabling FrameBuffer Path -----\n" );


	//set our main screen flags
	qboolean floatDepthRequested;
	int requestedDepthBits;
	GetDesiredDepthType(tmp,tmp,requestedDepthBits,floatDepthRequested);
	qboolean requestingIntegral32 = (qboolean)(requestedDepthBits == 32 && !floatDepthRequested);
	flags = 0;
	if ( (glConfig.stencilBits > 0) ) {
		if (glConfig.packedDepthStencil && r_fboDepthPacked->integer && !requestingIntegral32) { // Not sure if I got this all right
			flags |= FB_PACKED;
		}
		else {
			flags |= FB_DEPTH;
			if (r_fboStencilWhenNotPacked->integer) {
				flags |= FB_STENCIL;
			}
		}
	} else {
		flags |= FB_DEPTH;
	}
	
	fbo.screenWidth = glMMEConfig.glWidth;
	fbo.screenHeight = glMMEConfig.glHeight;

	width = r_fboWidth->integer;
	height = r_fboHeight->integer;
	//Illegal width/height use original opengl one
	if ( width <= 0 || height <= 0 ) {
		width = fbo.screenWidth;
		height = fbo.screenHeight;
	}

	if (r_floatBuffer->integer > 2) {
		flags |= FB_FLOAT32;
	}
	else if (r_floatBuffer->integer) {
		flags |= FB_FLOAT16;
	}

	//create our main frame buffer
	fbo.main = R_FrameBufferCreate( width, height, flags | FB_SECONDARYBUFFER | FB_TERTIARYBUFFER,superSampleMultiplier );
	fbo.exposure = R_FrameBufferCreate( width, height, flags,superSampleMultiplier );
	fbo.postprocessing = R_FrameBufferCreate( width, height, flags | FB_MIPMAP | FB_MAGLINEAR, superSampleMultiplier ); // need mipmaps here because we rely on them for a kind of softening effect

	for (int i = 0; i < MAX_SCENE_VIEWS; i++) {
		fbo.extraViews[i] = R_FrameBufferCreate(width, height, flags | FB_MIPMAP | FB_MAGLINEAR | FB_REPEATEDGE | FB_SECONDARYBUFFER, superSampleMultiplier);
	}

	if (!fbo.main/* || !fbo.extra*/) {
		// if the main fbuffer failed then we should disable framebuffer 
		// rendering
		glMMEConfig.framebufferObject = qfalse;
		ri.Printf( PRINT_WARNING, "WARNING: Framebuffer creation failed\n");
		ri.Printf( PRINT_WARNING, "WARNING: Framebuffer rendering path disabled\n");
		//Reinit back to window rendering in case status fails
		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0 );
		return;
	}

	if ( r_fboMultiSample->integer ) {
		flags |= FB_MULTISAMPLE;
		flags &= ~FB_PACKED;
		flags |= FB_DEPTH;
		fbo.multiSample = R_FrameBufferCreate( width, height, flags );
	}

	glConfig.vidWidth = width;
	glConfig.vidHeight = height;

	if ( r_fboBlur->integer > 1 ) {
		flags = FB_FLOAT32;
		fbo.blur = R_FrameBufferCreate( width, height, flags );
	} else if ( r_fboBlur->integer  ) {
		flags = FB_FLOAT16;
		fbo.blur = R_FrameBufferCreate( width, height, flags );
	}

	if (r_floatBuffer->integer > 1) { // Rolling shutter buffers eat a lot of memory. Need r_floatBuffer of at least 3 to activate 32 bit floating points for rolling shutter.
		flags = FB_FLOAT32;
	}
	else if (r_floatBuffer->integer) {
		flags = FB_FLOAT16;
	}
	R_FrameBuffer_CreateRollingShutterBuffers(width, height,flags);

	if (r_convertToHDR->integer) {
		flags = FB_FLOAT16;
		fbo.colorSpaceConv = R_FrameBufferCreate(width, height, flags);
		fbo.colorSpaceConvResult = R_FrameBufferCreate(width, height, flags);
		hdrPqShader = new R_GLSL("glsl/hdrpq-vertex.glsl","","","","glsl/hdrpq-fragment.glsl",qfalse);
		if (!hdrPqShader->IsWorking()) {
			ri.Printf(PRINT_WARNING, "WARNING: HDR PQ Shader could not be compiled. HDR conversion disabled.\n");
		}
	}

	if (r_fboGLSL->integer && ENABLEGLSL) {
		if (g_SSBOsSupported) {
			qglGenBuffersARB(1, &shadowLineSSBOReference);
			qglGenBuffersARB(1, &cheapLightSSBOReference);
			qglGenBuffersARB(1, &lightStylesSSBOReference);
			qglGenBuffersARB(1, &musicDeformSSBOReference);
			qglGenBuffersARB(1, &voxelSSBOReference);
			qglGenBuffersARB(1, &variousSSBODataReference);
		}
	}
	ReLoadGLSL();

	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0 );
#endif
}



void R_FrameBuffer_CreateRollingShutterBuffers(int width, int height, int flags) {

	if (!mme_rollingShutterEnabled->integer) return;

	mmeRollingShutterInfo_t* rsInfo = R_MME_GetRollingShutterInfo();

	//int bufferCountNeededForRollingshutter = (int)(ceil(mme_rollingShutterMultiplier->value) + 0.5f); // ceil bc if value is 1.1 we need 2 buffers. +.5 to avoid float issues..
	//rollingShutterBufferCount = bufferCountNeededForRollingshutter;
	rollingShutterBufferCount = rsInfo->bufferCountNeededForRollingshutter;

	//int rollingShutterFactor = glConfig.vidHeight*rollingShutterSuperSampleMultiplier / mme_rollingShutterPixels->integer; // For example: 1080/1 = 1080

	// For example: (1080/9.8*10)-1080 = 22.040816326530612244897959183673
	// Or: (360/9.8*10)-360 = 7.3469387755102040816326530612245
	float progressOvershootFloat = ((float)rsInfo->rollingShutterFactor / rsInfo->rollingShutterMultiplier * (float)rsInfo->bufferCountNeededForRollingshutter) - (float)rsInfo->rollingShutterFactor;
	progressOvershoot = (int)progressOvershootFloat;
	// For example: 0.040816326530612244897959183673
	drift = progressOvershootFloat - (float)progressOvershoot;

	// create more pixel buffers if we need that for rolling shutter
	if (rollingShutterBufferCount > fbo.rollingShutterBuffers.size()) {
		fbo.rollingShutterBuffers.resize(rollingShutterBufferCount);
		pboRollingShutterProgresses.resize(rollingShutterBufferCount);
		pboRollingShutterDrifts.resize(rollingShutterBufferCount);
	}

	for (int i = 0; i < fbo.rollingShutterBuffers.size(); i++) {
		// Each rolling shutter buffer gets two buffers. That way we can make an ultra long motion blur (as long as the rolling shutter multiplier)
		fbo.rollingShutterBuffers[i].current = R_FrameBufferCreate(width, height, flags);
		fbo.rollingShutterBuffers[i].next = R_FrameBufferCreate(width, height, flags);

		// For example: -1 * (1080 / 10) = -108
		pboRollingShutterProgresses[i] = (int)(-(float)i * ((float)rsInfo->rollingShutterFactor / (float)rsInfo->bufferCountNeededForRollingshutter));
		pboRollingShutterDrifts[i] = 0.0f;
	}

}

static qboolean usedFloat;

/* Startframe checks if the framebuffer is still active or was just activated */
void R_FrameBuffer_StartFrame( void ) {
#ifdef HAVE_GLES
	//TODO
#else
	if ( !fbo.main ) {
		return;
	}

	if (fbo.reloadGLSL) {
		ReLoadGLSL();
		fbo.reloadGLSL = qfalse;
	}

	if ( fbo.multiSample ) {
		//Bind the framebuffer at the beginning to be drawn in
		qglBindFramebuffer( GL_FRAMEBUFFER_EXT, fbo.multiSample->fbo );
	} else {
		qglBindFramebuffer( GL_FRAMEBUFFER_EXT, fbo.main->fbo );
	}
#if MULTIATTACH
	R_SetCorrectDrawBuffers();
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	if (glConfig.depthClamp && r_glDepthClamp->integer) {
		qglEnable(GL_DEPTH_CLAMP);
	}
	qglClampColor(GL_CLAMP_VERTEX_COLOR_ARB,GL_FALSE);
	qglClampColor(GL_CLAMP_FRAGMENT_COLOR_ARB,GL_FALSE);
	qglClampColor(GL_CLAMP_READ_COLOR_ARB,GL_FALSE);
	usedFloat = qfalse;

	r_fboFishEyeTessellate = ri.Cvar_Get("r_fboFishEyeTessellate", "1", CVAR_ARCHIVE); // Updated on every frame.
	r_fboFishEye = ri.Cvar_Get("r_fboFishEye", "0", CVAR_ARCHIVE);

	if (r_fboGLSL->integer && ENABLEGLSL) {
		if (voxelGrid && (voxelGridUpdated & VOXELGRIDUPDATED_GLSL)) {
			if (g_SSBOsSupported) {

				if (voxelSSBOData) {
					delete[] voxelSSBOData;
					voxelSSBOData = NULL;
					voxelSSBODataSize = 0;
				}

				voxelSSBODataSize = voxelGridSize;
				voxelSSBODataSize /= 4 * 4;
				voxelSSBODataSize *= 4 * 4; // make it align well? idk if needed tbh

				voxelSSBOData = new uint32_t[voxelSSBODataSize/4];

				memcpy(voxelSSBOData, voxelGrid, voxelSSBODataSize);
				//memset(voxelSSBOData, 255, voxelSSBODataSize);
				//memset((byte*)voxelSSBOData+ 67305664, 255, voxelSSBODataSize- 67305664);
				uint32_t* dataSource = voxelSSBOData;

				qglBindBufferARB(GL_SHADER_STORAGE_BUFFER, voxelSSBOReference);
				qglBufferDataARB(GL_SHADER_STORAGE_BUFFER, voxelSSBODataSize, dataSource, GL_DYNAMIC_DRAW_ARB);
				qglBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, voxelSSBOReference);
				qglBindBufferARB(GL_SHADER_STORAGE_BUFFER, 0);

				voxelGridUpdated &= ~VOXELGRIDUPDATED_GLSL;
			}
		}

		if (tr.mmeMusicDeformIndex != fbo.soundDeformLastIndex) {
			fbo.soundDeformSampleCount = 0;
			static float empty[1]{ 0 };
			if (g_SSBOsSupported) {
				if (musicDeformSSBOData) {
					delete[] musicDeformSSBOData;
					musicDeformSSBOData = NULL;
				}
				int sampleCount = 0;
				float* dataSource = empty;
				if (tr.mmeMusicDeform) {
					fbo.soundDeformSampleCount = tr.mmeMusicDeformLength;
					int sampleCountVec4Align = (tr.mmeMusicDeformLength/4*4) + ((tr.mmeMusicDeformLength % 4) ? 4 : 0);// glsl doesnt support short
					musicDeformSSBOData = new float[sampleCountVec4Align];
					if (tr.mmeMusicDeformLength % 4) {
						musicDeformSSBOData[sampleCountVec4Align - 3] = 0;
						musicDeformSSBOData[sampleCountVec4Align - 2] = 0;
						musicDeformSSBOData[sampleCountVec4Align - 1] = 0;
					}
					for (int i = 0; i < tr.mmeMusicDeformLength; i++) {
						musicDeformSSBOData[i] = tr.mmeMusicDeform[i];
					}
					//Com_Memcpy(musicDeformSSBOData, tr.mmeMusicDeform, tr.mmeMusicDeformLength * sizeof(short));
					//if (tr.mmeMusicDeformLength % 4) {
					//	musicDeformSSBOData[sampleCountIntAlign - 1] = 0;
					//}
					dataSource = musicDeformSSBOData;
					sampleCount = sampleCountVec4Align;
				}
				qglBindBufferARB(GL_SHADER_STORAGE_BUFFER, musicDeformSSBOReference);
				qglBufferDataARB(GL_SHADER_STORAGE_BUFFER, sampleCount * sizeof(float), dataSource, GL_DYNAMIC_DRAW_ARB);
				qglBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, musicDeformSSBOReference);
				qglBindBufferARB(GL_SHADER_STORAGE_BUFFER, 0);
			}
			fbo.soundDeformSampleRate = tr.mmeMusicDeformSampleRate ? tr.mmeMusicDeformSampleRate : 44100;
			fbo.soundDeformLastIndex = tr.mmeMusicDeformIndex;
		}
	}

#endif
}

void R_SetCorrectDrawBuffers() {

	GLenum attachments[3] = { GL_COLOR_ATTACHMENT0_EXT , GL_NONE, GL_NONE };
	if (!fbo.drawing2D) {
		attachments[1] = GL_COLOR_ATTACHMENT1_EXT;
	}
	if (fbo.drawingShadowPrepass) {
		attachments[2] = GL_COLOR_ATTACHMENT2_EXT;
	}
	//qglDrawBuffers(2, fbo.drawing2D ? attachment1 : attachment1and2);
	qglDrawBuffers(3, attachments);
}

qboolean R_FrameBuffer_HDRConvert(HDRConvertSource source, int param) {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if (!hdrPqShader->IsWorking() || (source==HDRCONVSOURCE_FBO &&  !fbo.rollingShutterBuffers[param].current))
		return qfalse;
	
	R_FrameBuffer_TempDeactivateFisheye();

	if (source == HDRCONVSOURCE_FBO) {
		
		//qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.main->fbo);
		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.colorSpaceConvResult->fbo);
#if MULTIATTACH
		qglDrawBuffers(2, attachment1);
#else
		qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif

		qglColor4f(1, 1, 1, 1);
		GL_State(GLS_DEPTHTEST_DISABLE);
		R_SetGL2DSize(glConfig.vidWidth, glConfig.vidHeight);
		hdrPqShader->UseThisProgram(0);
		R_DrawQuad(fbo.rollingShutterBuffers[param].current->color, glConfig.vidWidth, glConfig.vidHeight);
		R_GLSL::UseProgram(0,0);

		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.main->fbo);
#if MULTIATTACH
		R_SetCorrectDrawBuffers();
#else
		qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
		qglReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	}
	else if (source == HDRCONVSOURCE_PBO) { // We assume the PBO is bound!  Note: This whole section isn't used (anymore?) so idk if it even works at all or ever worked. Forgot.

		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.colorSpaceConv->fbo);
#if MULTIATTACH
		qglDrawBuffers(2, attachment1);
#else
		qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif

		// Fix random black image when saber flare happens
		// Credit: https://community.khronos.org/t/gldrawpixels-or-how-to-lose-your-time-infinitely/44513/7
		//qglClear(GL_DEPTH_BUFFER_BIT); // this alone does NOT fix it
		qglDisable(GL_TEXTURE_2D);  //this is the one that fixed the black frame on its own
		//qglDisable(GL_LIGHTING);
		//qglDisable(GL_DEPTH_TEST);

		//The color used to blur add this frame
		qglColor4f(1, 1, 1, 1);
		GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHTEST_DISABLE);
		R_SetGL2DSize(glConfig.vidWidth, glConfig.vidHeight);


		qglDrawPixels(glConfig.vidWidth, glConfig.vidHeight, GL_BGR_EXT, GL_FLOAT, 0);

		//qglFinish();

		
		//R_DrawQuad(fbo.main->color, glConfig.vidWidth, glConfig.vidHeight);
		//Reset fbo
		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.colorSpaceConvResult->fbo);
#if MULTIATTACH
		qglDrawBuffers(2, attachment1);
#else
		qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif

		qglColor4f(1, 1, 1, 1);
		GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHTEST_DISABLE);
		hdrPqShader->UseThisProgram(0);
		R_DrawQuad(fbo.colorSpaceConv->color, glConfig.vidWidth, glConfig.vidHeight);
		R_GLSL::UseProgram(0,0); 

		// do i need to bindframebuffer main again here? let's say yes. if not, revert this. i added this long after the pbo version of this was no longer in use, if it ever was
		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.main->fbo);
#if MULTIATTACH
		R_SetCorrectDrawBuffers();
#else
		qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif

		//qglFinish();
	}
	else if(source == HDRCONVSOURCE_MAINFBO) {
		/*
		R_FrameBuffer_GenerateMainMipMaps();

		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.colorSpaceConv->fbo);
		qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
		//The color used to blur add this frame
		qglColor4f(1, 1, 1, 1);
		GL_State(GLS_DEPTHTEST_DISABLE);

		R_SetGL2DSize(glConfig.vidWidth, glConfig.vidHeight);
		R_DrawQuad(fbo.main->color, glConfig.vidWidth, glConfig.vidHeight);
		//Reset fbo
		//qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.main->fbo);
		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.main->fbo);
		qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

		qglColor4f(1, 1, 1, 1);
		GL_State(GLS_DEPTHTEST_DISABLE);
		hdrPqShader->UseThisProgram(0);
		R_DrawQuad(fbo.colorSpaceConv->color, glConfig.vidWidth, glConfig.vidHeight);
		R_GLSL::UseProgram(0,0);*/
		
		R_FrameBuffer_GenerateMainMipMaps();

		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.colorSpaceConvResult->fbo);
#if MULTIATTACH
		qglDrawBuffers(2, attachment1);
#else
		qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
		//The color used to blur add this frame
		qglColor4f(1, 1, 1, 1);
		GL_State(GLS_DEPTHTEST_DISABLE);

		R_SetGL2DSize(glConfig.vidWidth, glConfig.vidHeight);
		hdrPqShader->UseThisProgram(0);
		R_DrawQuad(fbo.main->color, glConfig.vidWidth, glConfig.vidHeight);
		R_GLSL::UseProgram(0,0);

		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.main->fbo);
#if MULTIATTACH
		R_SetCorrectDrawBuffers();
#else
		qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
		qglReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
		
	}

	R_FrameBuffer_ReactivateFisheye();
	return qtrue;
#endif
}





qboolean R_FrameBuffer_StartHDRRead() {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if (!hdrPqShader->IsWorking())
		return qfalse;
	
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.colorSpaceConvResult->fbo); 
#if MULTIATTACH
	qglDrawBuffers(2, attachment1);
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	qglReadBuffer(GL_COLOR_ATTACHMENT0_EXT);

	return qtrue;
#endif
}

qboolean R_FrameBuffer_EndHDRRead() {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if (!hdrPqShader->IsWorking())
		return qfalse;
	
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.main->fbo);
#if MULTIATTACH
	R_SetCorrectDrawBuffers();
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	qglReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	//qglReadBuffer(GL_BACK);

	return qtrue;
#endif
}

// We use a double buffer for the rolling shutter so that we can do ultra long motion blur by writing to the current AND next frame.
// When one frame is finished, we flip.
void R_FrameBuffer_RollingShutterFlipDoubleBuffer(int bufferIndex) {
	
	R_FrameBuffer_TempDeactivateFisheye();
	frameBufferData_t* tmp = fbo.rollingShutterBuffers[bufferIndex].next;
	fbo.rollingShutterBuffers[bufferIndex].next = fbo.rollingShutterBuffers[bufferIndex].current;
	fbo.rollingShutterBuffers[bufferIndex].current = tmp;

	// Clear the buffer for the next image so we can always use ADD blending.
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.rollingShutterBuffers[bufferIndex].next->fbo);
#if MULTIATTACH
	qglDrawBuffers(2, attachment1);
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	qglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	qglClear(GL_COLOR_BUFFER_BIT);
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.main->fbo);
#if MULTIATTACH
	R_SetCorrectDrawBuffers();
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	R_FrameBuffer_ReactivateFisheye();
}


qboolean R_FrameBuffer_RollingShutterCapture(int bufferIndex, int offset, int height,bool additive,bool toNextFrame,float weight) {

#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else

	frameBufferData_t* selectedFrameBufferData = toNextFrame ? fbo.rollingShutterBuffers[bufferIndex].next : fbo.rollingShutterBuffers[bufferIndex].current;
	
	float c;
	if (!selectedFrameBufferData)
		return qfalse;

	R_FrameBuffer_TempDeactivateFisheye();

	R_FrameBuffer_GenerateMainMipMaps();

	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, selectedFrameBufferData->fbo);
#if MULTIATTACH
	qglDrawBuffers(2, attachment1);
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	//The color used to blur add this frame
	//c = 1.0f;
	//c = 1.0f/(float)mme_rollingShutterBlur->integer;
	c = weight;
	qglColor4f(c, c , c, 1);
	if (!additive) {
		GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHTEST_DISABLE);
	}
	else {
		GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHTEST_DISABLE);
	}
	R_SetGL2DSize(glConfig.vidWidth, glConfig.vidHeight);
	R_DrawQuadPartial(fbo.main->color, glConfig.vidWidth, height, 0,offset,rollingShutterSuperSampleMultiplier);
	//Reset fbo
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.main->fbo);
#if MULTIATTACH
	R_SetCorrectDrawBuffers();
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	/*usedFloat = qtrue;
	if (frame == total - 1) {
		qglColor4f(1, 1, 1, 1);
		GL_State(GLS_DEPTHTEST_DISABLE);
		R_DrawQuad(fbo.blur->color, glConfig.vidWidth, glConfig.vidHeight);
	}*/
	R_FrameBuffer_ReactivateFisheye();
	return qtrue;
#endif
}

qboolean R_FrameBuffer_Blur( float scale, int frame, int total, qboolean forceWriteback) {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	float c;
	if ( !fbo.blur )
		return qfalse;

	R_FrameBuffer_TempDeactivateFisheye();

	R_FrameBuffer_GenerateMainMipMaps();

	qglBindFramebuffer( GL_FRAMEBUFFER_EXT, fbo.blur->fbo );
#if MULTIATTACH
	qglDrawBuffers(2, attachment1);
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	//The color used to blur add this frame
	c = scale;
	qglColor4f( c , c , c , 1 );
	if ( frame == 0 ) {
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHTEST_DISABLE );
	} else {
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHTEST_DISABLE );
	}
	R_SetGL2DSize( glConfig.vidWidth, glConfig.vidHeight );
	R_DrawQuad(	fbo.main->color, glConfig.vidWidth, glConfig.vidHeight );
	//Reset fbo
	qglBindFramebuffer( GL_FRAMEBUFFER_EXT, fbo.main->fbo );
#if MULTIATTACH
	qglDrawBuffers(2, attachment1);
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	usedFloat = qtrue;
	if ( frame == total - 1  || forceWriteback) {
		qglColor4f( 1, 1, 1, 1 );
		GL_State( GLS_DEPTHTEST_DISABLE );
		R_DrawQuad(	fbo.blur->color, glConfig.vidWidth, glConfig.vidHeight );
	}
#if MULTIATTACH
	R_SetCorrectDrawBuffers();
#endif

	R_FrameBuffer_ReactivateFisheye();

	return qtrue;
#endif
}



qboolean R_FrameBuffer_SaveSceneView( int index ) {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	float c;
	if ( !fbo.blur )
		return qfalse;

	R_FrameBuffer_TempDeactivateFisheye();

	R_FrameBuffer_GenerateMainMipMaps();

	qglBindFramebuffer( GL_FRAMEBUFFER_EXT, fbo.extraViews[index]->fbo );
#if MULTIATTACH
	qglDrawBuffers(2, attachment1);
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	//The color used to blur add this frame
	c = 1.0f;
	qglColor4f( c , c , c , 1 );
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHTEST_DISABLE);
	R_SetGL2DSize( glConfig.vidWidth, glConfig.vidHeight );
	R_DrawQuad(	fbo.main->color, glConfig.vidWidth, glConfig.vidHeight );


#if MULTIATTACH
	qglDrawBuffers(2, attachment2);
	qglColor4f(c, c, c, 1);
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHTEST_DISABLE);
	R_SetGL2DSize(glConfig.vidWidth, glConfig.vidHeight);
	R_DrawQuad(fbo.main->secondaryColor, glConfig.vidWidth, glConfig.vidHeight); // copy secondary color attachment. need for ssr
#endif

	//Reset fbo
	qglBindFramebuffer( GL_FRAMEBUFFER_EXT, fbo.main->fbo );
#if MULTIATTACH
	R_SetCorrectDrawBuffers();
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif

	mipMapsAlreadyGeneratedThisFrame = qfalse;

	fbo.extraViewsMipMapsGenerated &= ~(1 << index);
	fbo.extraViewsSecondaryMipMapsGenerated &= ~(1 << index);

	R_FrameBuffer_ReactivateFisheye();

	return qtrue;
#endif
}

#ifdef RELDEBUG
//#pragma optimize("", off)
#endif
const vec3_t brightnessWeightsPhotometric{0.2126f,0.7162f,0.0722f};
qboolean R_FrameBuffer_ApplyExposure( ) { // really kinda useless unless you want to reduce exposure sadly. Numbers are clamped.
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if ( !fbo.exposure )
		return qfalse;

	r_fboExposure = ri.Cvar_Get("r_fboExposure", "1.0", CVAR_ARCHIVE);
	r_fboCompensateSkyTint = ri.Cvar_Get("r_fboCompensateSkyTint", "0", CVAR_ARCHIVE);

	if (!Q_stricmp(r_fboExposure->string, "1.0") || !Q_stricmp(r_fboExposure->string, "1")) { // Determine if we need to do this at all.
		if (!tr.mmeFBOImageTintIsSet) {
			if (!r_fboCompensateSkyTint->integer) {
				return qtrue;
			}
			else if (r_fboCompensateSkyTint->integer && !tr.mmeSkyTintIsSet) {
				return qtrue;
			}
		}
	}

	//R_FrameBuffer_GenerateMainMipMaps();
	R_FrameBuffer_TempDeactivateFisheye();
	
	// First copy image into exposure FBO and apply exposure
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.exposure->fbo);
#if MULTIATTACH
	qglDrawBuffers(2, attachment1);
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	//The color used to blur add this frame 
	float multiplier = r_fboExposure->value;
	vec3_t tint{ 1.0f,1.0f,1.0f };
	VectorScale(tint,multiplier,tint);
	if (r_fboCompensateSkyTint->integer && tr.mmeSkyTintIsSet ) {

		vec3_t invertedSkyTint;
		VectorInvert(tr.mmeSkyTint, invertedSkyTint);
		if (r_fboCompensateSkyTint->integer == 3) {
			// compensate brightness - "photometric" mode
			float resultingBrightness = DotProduct(brightnessWeightsPhotometric, invertedSkyTint);
			VectorScale(invertedSkyTint, 1.0f / resultingBrightness,invertedSkyTint);
		}

		VectorMultiply(invertedSkyTint,tint,tint);
	}
	if (tr.mmeFBOImageTintIsSet ) {
		VectorMultiply(tr.mmeFBOImageTint,tint,tint);
	}
	qglColor4f(tint[0], tint[1], tint[2], 1.0f);
	//GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHTEST_DISABLE);
	GL_State( GLS_DEPTHTEST_DISABLE);
	R_SetGL2DSize(glConfig.vidWidth*superSampleMultiplier, glConfig.vidHeight * superSampleMultiplier);
	R_DrawQuad(fbo.main->color, glConfig.vidWidth * superSampleMultiplier, glConfig.vidHeight * superSampleMultiplier);
	
	// Now copy it back
	qglBindFramebuffer( GL_FRAMEBUFFER_EXT, fbo.main->fbo );
#if MULTIATTACH
	qglDrawBuffers(2, attachment1);
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	qglColor4f(1, 1, 1, 1);
	GL_State(GLS_DEPTHTEST_DISABLE );
	R_SetGL2DSize( glConfig.vidWidth * superSampleMultiplier, glConfig.vidHeight * superSampleMultiplier);
	R_DrawQuad(	fbo.exposure->color, glConfig.vidWidth * superSampleMultiplier, glConfig.vidHeight * superSampleMultiplier);
	mipMapsAlreadyGeneratedThisFrame = qfalse;
#if MULTIATTACH
	R_SetCorrectDrawBuffers();
#endif

	R_FrameBuffer_ReactivateFisheye();

	return qtrue;
#endif
}
#ifdef RELDEBUG
//#pragma optimize("", on)
#endif

qboolean R_FrameBuffer_ApplyPostProcessing(qboolean didEarlyBlur) {
#ifdef HAVE_GLES
	//TODO
	return qfalse;
#else
	if ( !fbo.postprocessing || r_fboGLSLThermalVision->integer != 2 && r_fboGLSLThermalVision->integer != 3 && r_fboGLSLThermalVision->integer != 4 || !thermalPostProcessingShader->IsWorking())
		return qfalse;


	//R_FrameBuffer_GenerateMainMipMaps();
	R_FrameBuffer_TempDeactivateFisheye();
	
	// First copy image into exposure FBO and apply exposure
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo.postprocessing->fbo);
#if MULTIATTACH
	qglDrawBuffers(2, attachment1);
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif

	qglColor4f(1.0f,1.0f,1.0f, 1.0f);

	GL_State( GLS_DEPTHTEST_DISABLE);
	R_SetGL2DSize(glConfig.vidWidth*superSampleMultiplier, glConfig.vidHeight * superSampleMultiplier);
	R_DrawQuad(fbo.main->color, glConfig.vidWidth * superSampleMultiplier, glConfig.vidHeight * superSampleMultiplier);
	
	// Now copy it back
	qglBindFramebuffer( GL_FRAMEBUFFER_EXT, fbo.main->fbo ); 
#if MULTIATTACH
	qglDrawBuffers(2, attachment1);
#else
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
	qglColor4f(1, 1, 1, 1);
	GL_State(GLS_DEPTHTEST_DISABLE );
	R_SetGL2DSize( glConfig.vidWidth * superSampleMultiplier, glConfig.vidHeight * superSampleMultiplier);
	thermalPostProcessingShader->UseThisProgram(0);
	uniformLocationsPostProcessing[0].thermalVisionUniform.set1i( r_fboGLSLThermalVision->integer);
	uniformLocationsPostProcessing[0].shaderDebugUniform.set1i( r_fboGLSLShaderDebug->integer);
	uniformLocationsPostProcessing[0].serverTimeUniform.set1i( backEnd.refdef.time);
	uniformLocationsPostProcessing[0].serverTimeFractionUniform.set1f( backEnd.refdef.timeFraction);
	uniformLocationsPostProcessing[0].jitterIndexUniform.set1i( fbo.fishEyeData.jitterIndex);
	uniformLocationsPostProcessing[0].jitterTotalFramesUniform.set1i( fbo.fishEyeData.jitterTotalFrames);
	uniformLocationsPostProcessing[0].blurEarlyStageUniform.set1i( didEarlyBlur ? 2 : 0);
	R_DrawQuad(	fbo.postprocessing->color, glConfig.vidWidth * superSampleMultiplier, glConfig.vidHeight * superSampleMultiplier,true);
	R_GLSL::UseProgram(0,0);
	mipMapsAlreadyGeneratedThisFrame = qfalse;

#if MULTIATTACH
	R_SetCorrectDrawBuffers();
#endif

	R_FrameBuffer_ReactivateFisheye();

	return qtrue;
#endif
}
#ifdef RELDEBUG
//#pragma optimize("", on)
#endif



void R_FrameBuffer_EndFrame( void ) {
#ifdef HAVE_GLES
	//TODO
#else
	if ( !fbo.main ) {
		return;
	}

	R_FrameBuffer_TempDeactivateFisheye();

	R_FrameBuffer_GenerateMainMipMaps();

	if ( fbo.multiSample ) {
		const frameBufferData_t* src = fbo.multiSample;
		const frameBufferData_t* dst = fbo.main;

		qglBindFramebuffer( GL_READ_FRAMEBUFFER_EXT, src->fbo );
		qglBindFramebuffer( GL_DRAW_FRAMEBUFFER_EXT, dst->fbo );
		qglBlitFramebufferEXT(0, 0, src->width, src->height, 0, 0, dst->width, dst->height, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
	}

	frameBufferData_t* sourceBuffer = usedFloat && !mme_blurEarly->integer ? fbo.blur : fbo.main;

	GL_State( GLS_DEPTHTEST_DISABLE );
	if (r_fbo->integer && r_fboOverbright->integer) {
		qglColor4f(tr.overbrightBitsMultiplier, tr.overbrightBitsMultiplier, tr.overbrightBitsMultiplier, 1);
	}
	else {
		qglColor4f(1, 1, 1, 1);
	}
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	qglEnable(GL_FRAMEBUFFER_SRGB);
	R_SetGL2DSize( fbo.screenWidth, fbo.screenHeight );

	switch (r_fboGLSLPreviewSecondary->integer) {
	default:
	case 0:
		R_DrawQuad(sourceBuffer->color, fbo.screenWidth, fbo.screenHeight);
		break;
	case 1:
		qglColor4f(0.0005f, 0.0005f, 0.0005f, 1.0f);
		R_DrawQuad(fbo.main->secondaryColor, fbo.screenWidth, fbo.screenHeight);
		break;
	case 2:
		qglColor4f(0.1f, 0.1f, 0.1f, 1.0f);
		R_DrawQuad(fbo.main->depth, fbo.screenWidth, fbo.screenHeight);
		break;
	case 3:
		qglColor4f(0.1f, 0.1f, 0.1f, 1.0f);
		R_DrawQuad(fbo.main->stencil, fbo.screenWidth, fbo.screenHeight);
		break;
	case 4:
		qglColor4f(0.1f, 0.1f, 0.1f, 1.0f);
		R_DrawQuad(fbo.main->packed, fbo.screenWidth, fbo.screenHeight);
		break;
	}


	usedFloat = qfalse;
	mipMapsAlreadyGeneratedThisFrame = qfalse;

	R_FrameBuffer_ReactivateFisheye();
#endif
}

void R_FrameBuffer_Shutdown( void ) {
//	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	R_FrameBufferDelete( fbo.main );
	R_FrameBufferDelete( fbo.exposure );
	R_FrameBufferDelete( fbo.postprocessing);
	R_FrameBufferDelete( fbo.blur );
	R_FrameBufferDelete( fbo.multiSample );
	R_FrameBufferDelete( fbo.colorSpaceConv );
	R_FrameBufferDelete( fbo.colorSpaceConvResult );

	for (int i = 0; i < fbo.rollingShutterBuffers.size(); i++) {
		R_FrameBufferDelete(fbo.rollingShutterBuffers[i].current);
		R_FrameBufferDelete(fbo.rollingShutterBuffers[i].next);

	}
}

#ifdef RELDEBUG
//#pragma optimize("", on)
#endif