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

typedef struct uniformLocations_t {
	GLint viewOriginUniform;
	GLint pixelJitterUniform;
	GLint dofJitterUniform;
	GLint dofFocusUniform;
	GLint dofRadiusUniform;
	GLint fishEyeModeUniform;
	GLint fovXUniform;
	GLint fovYUniform;
	GLint pixelWidthUniform;
	GLint pixelHeightUniform;
	GLint jitterIndexUniform;
	GLint jitterTotalFramesUniform;
	GLint texAverageBrightnessUniform;
	GLint isLightmapUniform;
	GLint isWorldBrushUniform;
	GLint isSaberUniform;
	GLint parallaxMapLayersUniform;
	GLint parallaxMapDepthUniform;
	GLint parallaxMapGammaUniform;
	GLint thermalVisionUniform;
	GLint shaderDebugUniform;
	GLint blurEarlyStageUniform;
	GLint serverTimeUniform;
	GLint serverTimeStartUniform;
	GLint serverTimeFractionUniform;
	GLint noiseFuckeryUniform;
	GLint noiseFuckeryLightmapUniform;
	GLint noiseFuckeryHDRIntensityUniform;
	GLint noiseFuckeryLightmapIntensityUniform;
	GLint worldModelViewMatrixUniform;
	GLint soundDeformSampleRateUniform;
	GLint soundDeformSampleCountUniform;

	GLint projectorModelViewMatrixUniform;
	GLint projectorProjectionMatrixUniform;
	GLint projectorPosUniform;
	GLint projectorActiveUniform;

	GLint soundDeformTimeUniform;
	GLint soundDeformIntensityUniform;
	GLint soundDeformSpreadSpeedUniform;
	GLint soundDeformSampleAvgWidthUniform;
	GLint soundDeformOriginUniform;
	GLint soundDeformDistanceScaleUniform;
	GLint soundDeformShortDistanceReductionUniform;
	GLint soundDeformModeUniform;

	GLint alphaFuncUniform;
	GLint alphaFuncValueUniform;
	GLint renderFlagsUniform;

	GLint zPrepassUniform;

	GLint deluxeMappingUniform;

	GLint text_in[NUM_TEXTURE_SAMPLERS];
	GLint text_inArray31;
	GLint stageImageBitmaskUniform;
	GLint stageLightmapBitmaskUniform;
	GLint bindingRectImageBitmaskUniform;
	GLint multiTexModeUniform;

	GLint haveVertexLightDirectionUniform;
	GLint isModelUniform;
	GLint surfaceTypeUniform;
	GLint stageColorGenUniform;
	GLint stageForceNormalUniform;

	GLint stageTCGenUniform;
	GLint stageHasTCModUniform;
	GLint gigaTCGenUniform;

	GLint rawStateBitsUniform;
	GLint appliedStateBitsUniform;

	GLint cloudScaleUniform;
	GLint cloudTimeScaleUniform;
	GLint cloudPowerUniform;
	GLint cloudIntensityCompensateUniform;

	GLint myFogUniform;
	GLint myFogColorUniform;

	GLint dLightFastUniform;
	GLint dLightJitterUniform;
	GLint dLightVoxelShadowsUniform;
	GLint dLightVoxelShadowJitterUniform;
	GLint dLightVoxelShadowJitterMethodUniform;
	GLint dLightIntensityUniform;
	GLint dLightFastSkipThresholdUniform;
	GLint dLightSpecIntensityUniform;
	GLint dLightSpecGammaUniform;
	GLint dLightSpecBaseReflectivityUniform;
	GLint dLightSpecDistanceDecayUniform;
	GLint dLightSpecDistanceMinUniform;
	GLint dLightAddPowUniform;
	GLint dLightAddPostPowMultUniform;
	GLint dLightsCountUniform; 
	GLint dLightsUniformOrigin[MAX_DLIGHTS];
	GLint dLightsUniformColor[MAX_DLIGHTS];
	GLint dLightsUniformRadius[MAX_DLIGHTS];
	GLint dLightsUniformMindist[MAX_DLIGHTS];
	GLint shadowLinesCountUniform;
	GLint shadowLinesPoint1[MAX_SHADOWLINES];
	GLint shadowLinesPoint2[MAX_SHADOWLINES];
	GLint shadowLinesWidth[MAX_SHADOWLINES];
	GLint shadowLinesA[MAX_SHADOWLINES];
	GLint shadowLinesB[MAX_SHADOWLINES];
	GLint cheapLightsCountUniform;

	GLint worldReflectNormalMixUniform;
	GLint worldReflectGradMultUniform;
	GLint worldReflectPuddleThreshUniform;
	GLint worldReflectMultiSampleUniform;

	GLint shaderStylesUniform[MAXLIGHTMAPS_REAL];
	GLint lightmapNumsUniform[NUM_TEXTURE_SAMPLERS];
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

		qglUniform3fv(uniformLocationsTess->viewOriginUniform, 1, tr.refdef.vieworg);
		qglUniform3fv(uniformLocationsTess->pixelJitterUniform, 1, fbo.fishEyeData.pixelJitter3D);
		qglUniform3fv(uniformLocationsTess->dofJitterUniform, 1, fbo.fishEyeData.dofJitter3D);
		qglUniform1f(uniformLocationsTess->dofFocusUniform, fbo.fishEyeData.dofFocus);
		qglUniform1f(uniformLocationsTess->dofRadiusUniform, fbo.fishEyeData.dofRadius);
		qglUniform1i(uniformLocationsTess->fishEyeModeUniform, fishEye);
		qglUniform1f(uniformLocationsTess->fovXUniform, fbo.fishEyeData.fovX);
		qglUniform1f(uniformLocationsTess->fovYUniform, fbo.fishEyeData.fovY);
		qglUniform1i(uniformLocationsTess->pixelWidthUniform, width*superSampleMultiplier);
		qglUniform1i(uniformLocationsTess->pixelHeightUniform, height * superSampleMultiplier);
		qglUniform1i(uniformLocationsTess->jitterIndexUniform, fbo.fishEyeData.jitterIndex);
		qglUniform1i(uniformLocationsTess->jitterTotalFramesUniform, fbo.fishEyeData.jitterTotalFrames);
		qglUniform1f(uniformLocationsTess->texAverageBrightnessUniform, fbo.fishEyeData.texAverageBrightness);
		qglUniform1i(uniformLocationsTess->isLightmapUniform, fbo.fishEyeData.isLightmap ? 1 : 0);
		qglUniform1i(uniformLocationsTess->isWorldBrushUniform, fbo.fishEyeData.isWorldBrush ? 1 : 0);
		qglUniform1i(uniformLocationsTess->isSaberUniform, fbo.fishEyeData.isSaber ? 1 : 0);
		qglUniform1i(uniformLocationsTess->parallaxMapLayersUniform, r_fboGLSLParallaxMappingLayers->integer);
		qglUniform1f(uniformLocationsTess->parallaxMapDepthUniform, r_fboGLSLParallaxMappingDepth->value);
		qglUniform1f(uniformLocationsTess->parallaxMapGammaUniform, r_fboGLSLParallaxMappingGamma->value);
		qglUniform1i(uniformLocationsTess->thermalVisionUniform, r_fboGLSLThermalVision->integer);
		qglUniform1i(uniformLocationsTess->shaderDebugUniform, r_fboGLSLShaderDebug->integer);
		qglUniform1i(uniformLocationsTess->serverTimeUniform, backEnd.refdef.time);
		qglUniform1i(uniformLocationsTess->serverTimeStartUniform, firstServerTime);
		qglUniform1f(uniformLocationsTess->serverTimeFractionUniform, backEnd.refdef.timeFraction);
		qglUniform1i(uniformLocationsTess->noiseFuckeryUniform, r_fboGLSLNoiseFuckery->integer);
		qglUniform1i(uniformLocationsTess->noiseFuckeryLightmapUniform, r_fboGLSLNoiseFuckeryLightmap->integer);
		qglUniform1f(uniformLocationsTess->noiseFuckeryLightmapIntensityUniform, r_fboGLSLNoiseFuckeryLightmapIntensity->value);
		qglUniform1f(uniformLocationsTess->noiseFuckeryHDRIntensityUniform, r_fboGLSLNoiseFuckeryHDRIntensity->value);
		qglUniformMatrix4fv(uniformLocationsTess->worldModelViewMatrixUniform, 1, GL_FALSE, backEnd.viewParms.world.modelMatrix);
		qglUniform1i(uniformLocationsTess->soundDeformSampleRateUniform, fbo.soundDeformSampleRate);
		qglUniform1i(uniformLocationsTess->soundDeformSampleCountUniform, fbo.soundDeformSampleCount);

		qglUniformMatrix4fv(uniformLocationsTess->projectorModelViewMatrixUniform, 1, GL_FALSE, tr.projector.modelMatrix);
		qglUniformMatrix4fv(uniformLocationsTess->projectorProjectionMatrixUniform, 1, GL_FALSE, tr.projector.projectionMatrix);
		qglUniform3fv(uniformLocationsTess->projectorPosUniform, 1, tr.projector.pos);
		qglUniform1i(uniformLocationsTess->projectorActiveUniform, fboUniformsEx.projectorActive);

		qglUniform1f(uniformLocationsTess->soundDeformTimeUniform, fbo.musicDeformData.time);
		qglUniform1f(uniformLocationsTess->soundDeformIntensityUniform, fbo.musicDeformData.intensity);
		qglUniform1f(uniformLocationsTess->soundDeformSpreadSpeedUniform, fbo.musicDeformData.spreadSpeed);
		qglUniform1i(uniformLocationsTess->soundDeformSampleAvgWidthUniform, fbo.musicDeformData.sampleAvgWidth);
		qglUniform3fv(uniformLocationsTess->soundDeformOriginUniform, 1, fbo.musicDeformData.origin);
		qglUniform1f(uniformLocationsTess->soundDeformDistanceScaleUniform, fbo.musicDeformData.distanceScale);
		qglUniform1f(uniformLocationsTess->soundDeformShortDistanceReductionUniform, fbo.musicDeformData.shortDistanceReduction);
		qglUniform1i(uniformLocationsTess->soundDeformModeUniform, fbo.musicDeformData.mode);

		qglUniform1i(uniformLocationsTess->alphaFuncUniform, fbo.fishEyeData.alphaFunc);
		qglUniform1f(uniformLocationsTess->alphaFuncValueUniform, fbo.fishEyeData.alphaFuncValue);
		qglUniform1i(uniformLocationsTess->renderFlagsUniform, fbo.fishEyeData.renderFlags | extraRenderFlags);

		qglUniform1i(uniformLocationsTess->zPrepassUniform, fbo.fishEyeData.doingZPrepass);

		qglUniform1i(uniformLocationsTess->deluxeMappingUniform, tr.deluxeMapping);

		qglUniform1i(uniformLocationsTess->haveVertexLightDirectionUniform, fbo.fishEyeData.haveVertexLightDirection ? 1 : 0);
		qglUniform1i(uniformLocationsTess->isModelUniform, fbo.fishEyeData.isModel ? 1 : 0);
		qglUniform1i(uniformLocationsTess->surfaceTypeUniform, (int)fbo.fishEyeData.surfaceType);
		qglUniform1i(uniformLocationsTess->stageColorGenUniform, fbo.fishEyeData.stageColorGen);
		qglUniform1i(uniformLocationsTess->stageForceNormalUniform, fbo.fishEyeData.stageForceNormal);

		qglUniform1i(uniformLocationsTess->stageTCGenUniform, fbo.fishEyeData.stageTCGen);
		qglUniform1i(uniformLocationsTess->stageHasTCModUniform, fbo.fishEyeData.stageHasTCMod);
		qglUniform1i(uniformLocationsTess->gigaTCGenUniform, r_fboGLSLGigaTCGen->integer);

		qglUniform1ui(uniformLocationsTess->rawStateBitsUniform, fbo.fishEyeData.stateBitsRaw);
		qglUniform1ui(uniformLocationsTess->appliedStateBitsUniform, fbo.fishEyeData.stateBitsApplied);

		qglUniform1i(uniformLocationsTess->stageImageBitmaskUniform, fbo.fishEyeData.stageImageBitmask);
		qglUniform1i(uniformLocationsTess->stageLightmapBitmaskUniform, fbo.fishEyeData.stageLightmapBitmask);
		qglUniform1ui(uniformLocationsTess->bindingRectImageBitmaskUniform, fboUniformsEx.textRectBitmask);
		qglUniform1i(uniformLocationsTess->multiTexModeUniform, fbo.fishEyeData.multiTexMode);

		qglUniform1f(uniformLocationsTess->cloudScaleUniform, r_fboGLSLCloudShadowScale->value);
		qglUniform1f(uniformLocationsTess->cloudTimeScaleUniform, r_fboGLSLCloudShadowTimeScale->value);
		qglUniform1f(uniformLocationsTess->cloudPowerUniform, r_fboGLSLCloudShadowPower->value);
		qglUniform1f(uniformLocationsTess->cloudIntensityCompensateUniform, intensityCompensateFactor);

		qglUniform1f(uniformLocationsTess->myFogUniform, r_fboGLSLFog->value);
		qglUniform3fv(uniformLocationsTess->myFogColorUniform, 1, tr.fboGLSLFogColor);

		qglUniform1f(uniformLocationsTess->worldReflectNormalMixUniform, r_fboGLSLWorldReflectNormalMix->value);
		qglUniform1f(uniformLocationsTess->worldReflectGradMultUniform, r_fboGLSLWorldReflectGradMult->value);
		qglUniform1f(uniformLocationsTess->worldReflectPuddleThreshUniform, r_fboGLSLWorldReflectPuddleTresh->value);
		qglUniform1i(uniformLocationsTess->worldReflectMultiSampleUniform, r_fboGLSLWorldReflectMultiSample->integer);

		qglUniform1i(uniformLocationsTess->dLightFastUniform, r_fboGLSLDLightsFast->integer);
		qglUniform1i(uniformLocationsTess->dLightVoxelShadowsUniform, r_fboGLSLDLightsVoxelShadows->integer);
		qglUniform3fv(uniformLocationsTess->dLightVoxelShadowJitterUniform, 1, fbo.fishEyeData.dlightVoxelShadowJitter3D);
		qglUniform1i(uniformLocationsTess->dLightVoxelShadowJitterMethodUniform, mme_voxelShadowLightQuickJitterMethod->integer);
		qglUniform3fv(uniformLocationsTess->dLightJitterUniform, 1, fbo.fishEyeData.dlightJitter3D);
		qglUniform1i(uniformLocationsTess->dLightsCountUniform, r_fboGLSLDLights->integer?  backEnd.refdef.num_dlights : 0);
		qglUniform1f(uniformLocationsTess->dLightSpecGammaUniform, r_fboGLSLDLightsSpecGamma->value);
		qglUniform1f(uniformLocationsTess->dLightSpecIntensityUniform, r_fboGLSLDLightsSpecIntensity->value);
		qglUniform1f(uniformLocationsTess->dLightSpecBaseReflectivityUniform, r_fboGLSLDLightsSpecBaseReflectivity->value);
		qglUniform1f(uniformLocationsTess->dLightSpecDistanceDecayUniform, r_fboGLSLDLightsSpecDistanceDecay->value);
		qglUniform1f(uniformLocationsTess->dLightSpecDistanceMinUniform, r_fboGLSLDLightsSpecDistanceMinUniform->value);
		qglUniform1f(uniformLocationsTess->dLightIntensityUniform, r_fboGLSLDLightsIntensity->value);
		qglUniform1f(uniformLocationsTess->dLightFastSkipThresholdUniform, r_fboGLSLDLightsFastSkipThreshold->value);
		qglUniform1f(uniformLocationsTess->dLightAddPowUniform, r_fboGLSLDLightsAddPow->value);
		qglUniform1f(uniformLocationsTess->dLightAddPostPowMultUniform, r_fboGLSLDLightsAddPostPowMult->value);
		//qglUniform3fv(uniformLocationsTess->dLightsUniform"), sizeof(dlight_t) / 4 / 4 * backEnd.refdef.num_dlights, (GLfloat*)&backEnd.refdef.dlights);
		qglUniform1i(uniformLocationsTess->shadowLinesCountUniform, r_fboGLSLDLights->integer ? backEnd.refdef.num_shadowlines : 0);
		qglUniform1i(uniformLocationsTess->cheapLightsCountUniform, r_fboGLSLDLights->integer ? backEnd.refdef.num_cheaplights : 0);
		if (r_fboGLSLDLights->integer) {
			/*for (int i = 0; i < backEnd.refdef.num_dlights; i++) {

				qglUniform3fv(uniformLocationsTess->dLightsUniformOrigin[i], 1, backEnd.refdef.dlights[i].origin);
				qglUniform3fv(uniformLocationsTess->dLightsUniformColor[i], 1, backEnd.refdef.dlights[i].color);
				qglUniform1f(uniformLocationsTess->dLightsUniformRadius[i], backEnd.refdef.dlights[i].radius);
			}*/
		}
		for (int i = 0; i < NUM_TEXTURE_SAMPLERS; i++) {
			qglUniform1i(uniformLocationsTess->text_in[i], i);
			qglUniform1i(uniformLocationsTess->lightmapNumsUniform[i], fboUniformsEx.lightmapNums[i]);
		}
		qglUniform1i(uniformLocationsTess->text_inArray31, 31);
		for (int i = 0; i < MAXLIGHTMAPS_REAL; i++) {
			qglUniform1i(uniformLocationsTess->shaderStylesUniform[i], fbo.fishEyeData.shaderStyles[i]);
		}

		if (fbo.fishEyeData.tessellationActive) {

			qglPatchParameteri(GL_PATCH_VERTICES, 3);
		}
	}
	else {
		qglUniform3fv(uniformLocations->viewOriginUniform, 1, tr.refdef.vieworg);
		qglUniform3fv(uniformLocations->pixelJitterUniform, 1, fbo.fishEyeData.pixelJitter3D);
		qglUniform3fv(uniformLocations->dofJitterUniform, 1, fbo.fishEyeData.dofJitter3D);
		qglUniform1f(uniformLocations->dofFocusUniform, fbo.fishEyeData.dofFocus);
		qglUniform1f(uniformLocations->dofRadiusUniform, fbo.fishEyeData.dofRadius);
		qglUniform1i(uniformLocations->fishEyeModeUniform, fishEye);
		qglUniform1f(uniformLocations->fovXUniform, fbo.fishEyeData.fovX);
		qglUniform1f(uniformLocations->fovYUniform, fbo.fishEyeData.fovY);
		qglUniform1i(uniformLocations->pixelWidthUniform, width * superSampleMultiplier);
		qglUniform1i(uniformLocations->pixelHeightUniform, height * superSampleMultiplier);
		qglUniform1i(uniformLocations->jitterIndexUniform, fbo.fishEyeData.jitterIndex);
		qglUniform1i(uniformLocations->jitterTotalFramesUniform, fbo.fishEyeData.jitterTotalFrames);
		qglUniform1f(uniformLocations->texAverageBrightnessUniform, fbo.fishEyeData.texAverageBrightness);
		qglUniform1i(uniformLocations->isLightmapUniform, fbo.fishEyeData.isLightmap ? 1 : 0);
		qglUniform1i(uniformLocations->isWorldBrushUniform, fbo.fishEyeData.isWorldBrush ? 1 : 0);
		qglUniform1i(uniformLocations->isSaberUniform, fbo.fishEyeData.isSaber ? 1 : 0);
		qglUniform1i(uniformLocations->parallaxMapLayersUniform, r_fboGLSLParallaxMappingLayers->integer);
		qglUniform1f(uniformLocations->parallaxMapDepthUniform, r_fboGLSLParallaxMappingDepth->value);
		qglUniform1f(uniformLocations->parallaxMapGammaUniform, r_fboGLSLParallaxMappingGamma->value);
		qglUniform1i(uniformLocations->thermalVisionUniform, r_fboGLSLThermalVision->integer);
		qglUniform1i(uniformLocations->shaderDebugUniform, r_fboGLSLShaderDebug->integer);
		qglUniform1i(uniformLocations->serverTimeUniform, backEnd.refdef.time);
		qglUniform1i(uniformLocations->serverTimeStartUniform, firstServerTime);
		qglUniform1f(uniformLocations->serverTimeFractionUniform, backEnd.refdef.timeFraction);
		qglUniform1i(uniformLocations->noiseFuckeryUniform, r_fboGLSLNoiseFuckery->integer);
		qglUniform1i(uniformLocations->noiseFuckeryLightmapUniform, r_fboGLSLNoiseFuckeryLightmap->integer);
		qglUniform1f(uniformLocations->noiseFuckeryLightmapIntensityUniform, r_fboGLSLNoiseFuckeryLightmapIntensity->value);
		qglUniform1f(uniformLocations->noiseFuckeryHDRIntensityUniform, r_fboGLSLNoiseFuckeryHDRIntensity->value);
		qglUniformMatrix4fv(uniformLocations->worldModelViewMatrixUniform, 1, GL_FALSE, backEnd.viewParms.world.modelMatrix);
		qglUniform1i(uniformLocations->soundDeformSampleRateUniform, fbo.soundDeformSampleRate);
		qglUniform1i(uniformLocations->soundDeformSampleCountUniform, fbo.soundDeformSampleCount);

		qglUniformMatrix4fv(uniformLocations->projectorModelViewMatrixUniform, 1, GL_FALSE, tr.projector.modelMatrix);
		qglUniformMatrix4fv(uniformLocations->projectorProjectionMatrixUniform, 1, GL_FALSE, tr.projector.projectionMatrix);
		qglUniform3fv(uniformLocations->projectorPosUniform, 1, tr.projector.pos);
		qglUniform1i(uniformLocations->projectorActiveUniform, fboUniformsEx.projectorActive);

		qglUniform1f(uniformLocations->soundDeformTimeUniform, fbo.musicDeformData.time);
		qglUniform1f(uniformLocations->soundDeformIntensityUniform, fbo.musicDeformData.intensity);
		qglUniform1f(uniformLocations->soundDeformSpreadSpeedUniform, fbo.musicDeformData.spreadSpeed);
		qglUniform1i(uniformLocations->soundDeformSampleAvgWidthUniform, fbo.musicDeformData.sampleAvgWidth);
		qglUniform3fv(uniformLocations->soundDeformOriginUniform,1, fbo.musicDeformData.origin);
		qglUniform1f(uniformLocations->soundDeformDistanceScaleUniform, fbo.musicDeformData.distanceScale);
		qglUniform1f(uniformLocations->soundDeformShortDistanceReductionUniform, fbo.musicDeformData.shortDistanceReduction);
		qglUniform1i(uniformLocations->soundDeformModeUniform, fbo.musicDeformData.mode);

		qglUniform1i(uniformLocations->alphaFuncUniform, fbo.fishEyeData.alphaFunc);
		qglUniform1f(uniformLocations->alphaFuncValueUniform, fbo.fishEyeData.alphaFuncValue);
		qglUniform1i(uniformLocations->renderFlagsUniform, fbo.fishEyeData.renderFlags | extraRenderFlags);

		qglUniform1i(uniformLocations->zPrepassUniform, fbo.fishEyeData.doingZPrepass);

		qglUniform1i(uniformLocations->deluxeMappingUniform, tr.deluxeMapping);

		qglUniform1i(uniformLocations->haveVertexLightDirectionUniform, fbo.fishEyeData.haveVertexLightDirection ? 1 : 0);
		qglUniform1i(uniformLocations->isModelUniform, fbo.fishEyeData.isModel ? 1 : 0);
		qglUniform1i(uniformLocations->surfaceTypeUniform, (int)fbo.fishEyeData.surfaceType);
		qglUniform1i(uniformLocations->stageColorGenUniform, fbo.fishEyeData.stageColorGen);
		qglUniform1i(uniformLocations->stageForceNormalUniform, fbo.fishEyeData.stageForceNormal);

		qglUniform1i(uniformLocations->stageTCGenUniform, fbo.fishEyeData.stageTCGen);
		qglUniform1i(uniformLocations->stageHasTCModUniform, fbo.fishEyeData.stageHasTCMod);
		qglUniform1i(uniformLocations->gigaTCGenUniform, r_fboGLSLGigaTCGen->integer);

		qglUniform1ui(uniformLocations->rawStateBitsUniform, fbo.fishEyeData.stateBitsRaw);
		qglUniform1ui(uniformLocations->appliedStateBitsUniform, fbo.fishEyeData.stateBitsApplied);

		qglUniform1i(uniformLocations->stageImageBitmaskUniform, fbo.fishEyeData.stageImageBitmask);
		qglUniform1i(uniformLocations->stageLightmapBitmaskUniform, fbo.fishEyeData.stageLightmapBitmask);
		qglUniform1ui(uniformLocations->bindingRectImageBitmaskUniform, fboUniformsEx.textRectBitmask);
		qglUniform1i(uniformLocations->multiTexModeUniform, fbo.fishEyeData.multiTexMode);

		qglUniform1f(uniformLocations->cloudScaleUniform, r_fboGLSLCloudShadowScale->value);
		qglUniform1f(uniformLocations->cloudTimeScaleUniform, r_fboGLSLCloudShadowTimeScale->value);
		qglUniform1f(uniformLocations->cloudPowerUniform, r_fboGLSLCloudShadowPower->value);
		qglUniform1f(uniformLocations->cloudIntensityCompensateUniform, intensityCompensateFactor);

		qglUniform1f(uniformLocations->myFogUniform, r_fboGLSLFog->value);
		qglUniform3fv(uniformLocations->myFogColorUniform, 1, tr.fboGLSLFogColor);

		qglUniform1f(uniformLocations->worldReflectNormalMixUniform, r_fboGLSLWorldReflectNormalMix->value);
		qglUniform1f(uniformLocations->worldReflectGradMultUniform, r_fboGLSLWorldReflectGradMult->value);
		qglUniform1f(uniformLocations->worldReflectPuddleThreshUniform, r_fboGLSLWorldReflectPuddleTresh->value);
		qglUniform1i(uniformLocations->worldReflectMultiSampleUniform, r_fboGLSLWorldReflectMultiSample->integer);

		qglUniform1i(uniformLocations->dLightFastUniform, r_fboGLSLDLightsFast->integer);
		qglUniform1i(uniformLocations->dLightVoxelShadowsUniform, r_fboGLSLDLightsVoxelShadows->integer);
		qglUniform3fv(uniformLocations->dLightVoxelShadowJitterUniform, 1, fbo.fishEyeData.dlightVoxelShadowJitter3D);
		qglUniform1i(uniformLocations->dLightVoxelShadowJitterMethodUniform, mme_voxelShadowLightQuickJitterMethod->integer);
		qglUniform3fv(uniformLocations->dLightJitterUniform,1, fbo.fishEyeData.dlightJitter3D);
		qglUniform1i(uniformLocations->dLightsCountUniform, r_fboGLSLDLights->integer ? backEnd.refdef.num_dlights : 0);
		qglUniform1f(uniformLocations->dLightSpecGammaUniform, r_fboGLSLDLightsSpecGamma->value);
		qglUniform1f(uniformLocations->dLightSpecIntensityUniform, r_fboGLSLDLightsSpecIntensity->value);
		qglUniform1f(uniformLocations->dLightSpecBaseReflectivityUniform, r_fboGLSLDLightsSpecBaseReflectivity->value);
		qglUniform1f(uniformLocations->dLightSpecDistanceDecayUniform, r_fboGLSLDLightsSpecDistanceDecay->value);
		qglUniform1f(uniformLocations->dLightSpecDistanceMinUniform, r_fboGLSLDLightsSpecDistanceMinUniform->value);
		qglUniform1f(uniformLocations->dLightIntensityUniform, r_fboGLSLDLightsIntensity->value);
		qglUniform1f(uniformLocations->dLightFastSkipThresholdUniform, r_fboGLSLDLightsFastSkipThreshold->value);
		qglUniform1f(uniformLocations->dLightAddPowUniform, r_fboGLSLDLightsAddPow->value);
		qglUniform1f(uniformLocations->dLightAddPostPowMultUniform, r_fboGLSLDLightsAddPostPowMult->value);
		//qglUniform3fv(uniformLocations->dLightsUniform"), sizeof(dlight_t) / 4 / 4 * backEnd.refdef.num_dlights, (GLfloat*)&backEnd.refdef.dlights);
		qglUniform1i(uniformLocations->shadowLinesCountUniform, r_fboGLSLDLights->integer ? backEnd.refdef.num_shadowlines : 0);
		qglUniform1i(uniformLocations->cheapLightsCountUniform, r_fboGLSLDLights->integer ? backEnd.refdef.num_cheaplights : 0);
		if (r_fboGLSLDLights->integer) {
			/*
			for (int i = 0; i < backEnd.refdef.num_dlights; i++) {

				qglUniform3fv(uniformLocations.dLightsUniformOrigin[i], 1, backEnd.refdef.dlights[i].origin);
				qglUniform3fv(uniformLocations.dLightsUniformColor[i], 1, backEnd.refdef.dlights[i].color);
				qglUniform1f(uniformLocations.dLightsUniformRadius[i], backEnd.refdef.dlights[i].radius);
			}*/
		}
		for (int i = 0; i < MAXLIGHTMAPS_REAL; i++) {
			qglUniform1i(uniformLocations->shaderStylesUniform[i],fbo.fishEyeData.shaderStyles[i]);
		}
		qglUniform1i(uniformLocations->text_inArray31, 31);
		for (int i = 0; i < NUM_TEXTURE_SAMPLERS; i++) {
			qglUniform1i(uniformLocations->text_in[i], i);
			qglUniform1i(uniformLocations->lightmapNumsUniform[i], fboUniformsEx.lightmapNums[i]);
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

		qglUniform1i(uniformLocationsTess->dLightsCountUniform, r_fboGLSLDLights->integer ? backEnd.refdef.num_dlights : 0);
		qglUniform1i(uniformLocationsTess->shadowLinesCountUniform, r_fboGLSLDLights->integer ? backEnd.refdef.num_shadowlines : 0);
		qglUniform1i(uniformLocationsTess->cheapLightsCountUniform, r_fboGLSLDLights->integer ? backEnd.refdef.num_cheaplights : 0);
		if (r_fboGLSLDLights->integer) {
			for (int i = 0; i < backEnd.refdef.num_dlights; i++) {

				qglUniform3fv(uniformLocationsTess->dLightsUniformOrigin[i], 1, backEnd.refdef.dlights[i].origin);
				qglUniform3fv(uniformLocationsTess->dLightsUniformColor[i], 1, backEnd.refdef.dlights[i].color);
				qglUniform1f(uniformLocationsTess->dLightsUniformRadius[i], backEnd.refdef.dlights[i].radius);
				qglUniform1f(uniformLocationsTess->dLightsUniformMindist[i], backEnd.refdef.dlights[i].mindist);
			}
		}

	}
	else {
		qglUniform1i(uniformLocations->dLightsCountUniform, r_fboGLSLDLights->integer ? backEnd.refdef.num_dlights : 0);
		qglUniform1i(uniformLocations->shadowLinesCountUniform, r_fboGLSLDLights->integer ? backEnd.refdef.num_shadowlines : 0);
		qglUniform1i(uniformLocations->cheapLightsCountUniform, r_fboGLSLDLights->integer ? backEnd.refdef.num_cheaplights : 0);
		if (r_fboGLSLDLights->integer) {
			for (int i = 0; i < backEnd.refdef.num_dlights; i++) {

				qglUniform3fv(uniformLocations->dLightsUniformOrigin[i], 1, backEnd.refdef.dlights[i].origin);
				qglUniform3fv(uniformLocations->dLightsUniformColor[i], 1, backEnd.refdef.dlights[i].color);
				qglUniform1f(uniformLocations->dLightsUniformRadius[i], backEnd.refdef.dlights[i].radius);
				qglUniform1f(uniformLocations->dLightsUniformMindist[i], backEnd.refdef.dlights[i].mindist);
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
		qglUseProgram(0);
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

	if ( !(r_fboGLSL->integer && ENABLEGLSL)) {
		if (fbo.fishEyeActive) {
			R_FrameBuffer_DeactivateFisheye();
		}
		return qfalse;
	}

	if (fbo.fishEyeTempDisabled > 0) {

		if (fbo.fishEyeTempDisabled-- == 1) {

			qglUseProgram(fbo.fishEyeData.tessellationActive ? fishEyeShaderTess->ShaderIdByBits(R_FrameBuffer_GetShaderbits()) : fishEyeShader->ShaderIdByBits(R_FrameBuffer_GetShaderbits()));
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

	if ( !(r_fboGLSL->integer && ENABLEGLSL)) {
		if (fbo.fishEyeActive) {
			R_FrameBuffer_DeactivateFisheye();
		}
		return qfalse;
	}

	qglUseProgram(fbo.fishEyeData.tessellationActive ? fishEyeShaderTess->ShaderIdByBits(R_FrameBuffer_GetShaderbits()) : fishEyeShader->ShaderIdByBits(R_FrameBuffer_GetShaderbits()));
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
	if (!(r_fboGLSL->integer && ENABLEGLSL)) {
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
	if (!(r_fboGLSL->integer && ENABLEGLSL)) {
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
	if (!(r_fboGLSL->integer && ENABLEGLSL)) {
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
	if (!(r_fboGLSL->integer && ENABLEGLSL)) {
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


	qglUseProgram(0);
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

	if (!(r_fboGLSL->integer && ENABLEGLSL)) {
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

	if (!(r_fboGLSL->integer && ENABLEGLSL)) {
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
		locs->viewOriginUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "viewOriginUniform");
		locs->pixelJitterUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "pixelJitterUniform");
		locs->dofJitterUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dofJitterUniform");
		locs->dofFocusUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dofFocusUniform");
		locs->dofRadiusUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dofRadiusUniform");
		locs->fishEyeModeUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "fishEyeModeUniform");
		locs->fovXUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "fovXUniform");
		locs->fovYUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "fovYUniform");
		locs->pixelWidthUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "pixelWidthUniform");
		locs->pixelHeightUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "pixelHeightUniform");
		locs->jitterIndexUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "jitterIndexUniform");
		locs->jitterTotalFramesUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "jitterTotalFramesUniform");
		locs->texAverageBrightnessUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "texAverageBrightnessUniform");
		locs->isLightmapUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "isLightmapUniform");
		locs->isWorldBrushUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "isWorldBrushUniform");
		locs->isSaberUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "isSaberUniform");
		locs->parallaxMapLayersUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "parallaxMapLayersUniform");
		locs->parallaxMapDepthUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "parallaxMapDepthUniform");
		locs->parallaxMapGammaUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "parallaxMapGammaUniform");
		locs->thermalVisionUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "thermalVisionUniform");
		locs->shaderDebugUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "shaderDebugUniform");
		locs->blurEarlyStageUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "blurEarlyStageUniform");
		locs->serverTimeUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "serverTimeUniform");
		locs->serverTimeStartUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "serverTimeStartUniform");
		locs->serverTimeFractionUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "serverTimeFractionUniform");
		locs->noiseFuckeryUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "noiseFuckeryUniform");
		locs->noiseFuckeryLightmapUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "noiseFuckeryLightmapUniform");
		locs->noiseFuckeryLightmapIntensityUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "noiseFuckeryLightmapIntensityUniform");
		locs->noiseFuckeryHDRIntensityUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "noiseFuckeryHDRIntensityUniform");
		locs->worldModelViewMatrixUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "worldModelViewMatrixUniform");
		locs->soundDeformSampleRateUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "soundDeformSampleRateUniform");
		locs->soundDeformSampleCountUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "soundDeformSampleCountUniform");

		locs->projectorModelViewMatrixUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "projectorModelViewMatrixUniform");
		locs->projectorProjectionMatrixUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "projectorProjectionMatrixUniform");
		locs->projectorPosUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "projectorPosUniform");
		locs->projectorActiveUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "projectorActiveUniform");

		locs->soundDeformTimeUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "soundDeformTimeUniform");
		locs->soundDeformIntensityUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "soundDeformIntensityUniform");
		locs->soundDeformSpreadSpeedUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "soundDeformSpreadSpeedUniform");
		locs->soundDeformSampleAvgWidthUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "soundDeformSampleAvgWidthUniform");
		locs->soundDeformOriginUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "soundDeformOriginUniform");
		locs->soundDeformDistanceScaleUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "soundDeformDistanceScaleUniform");
		locs->soundDeformShortDistanceReductionUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "soundDeformShortDistanceReductionUniform");
		locs->soundDeformModeUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "soundDeformModeUniform");

		locs->alphaFuncUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "alphaFuncUniform");
		locs->alphaFuncValueUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "alphaFuncValueUniform");
		locs->renderFlagsUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "renderFlagsUniform");

		locs->zPrepassUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "zPrepassUniform");

		locs->deluxeMappingUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "deluxeMappingUniform");

		locs->haveVertexLightDirectionUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "haveVertexLightDirectionUniform");
		locs->isModelUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "isModelUniform");
		locs->surfaceTypeUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "surfaceTypeUniform");
		locs->stageColorGenUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "stageColorGenUniform");
		locs->stageForceNormalUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "stageForceNormalUniform");

		locs->stageTCGenUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "stageTCGenUniform");
		locs->stageHasTCModUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "stageHasTCModUniform");
		locs->gigaTCGenUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "gigaTCGenUniform");

		locs->rawStateBitsUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "rawStateBitsUniform");
		locs->appliedStateBitsUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "appliedStateBitsUniform");

		locs->stageImageBitmaskUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "stageImageBitmaskUniform");
		locs->stageLightmapBitmaskUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "stageLightmapBitmaskUniform");
		locs->bindingRectImageBitmaskUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "bindingRectImageBitmaskUniform");
		locs->multiTexModeUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "multiTexModeUniform");

		for (int j = 0; j < NUM_TEXTURE_SAMPLERS; j++) {
			locs->text_in[j] = qglGetUniformLocation(program->ShaderIdByBits(i), va("text_in[%d]",j));
			locs->lightmapNumsUniform[j] = qglGetUniformLocation(program->ShaderIdByBits(i), va("lightmapNumsUniform[%d]",j));
		}
		locs->text_inArray31 = qglGetUniformLocation(program->ShaderIdByBits(i), "text_inArray31");

		locs->cloudScaleUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "cloudScaleUniform");
		locs->cloudTimeScaleUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "cloudTimeScaleUniform");
		locs->cloudPowerUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "cloudPowerUniform");
		locs->cloudIntensityCompensateUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "cloudIntensityCompensateUniform");

		locs->myFogUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "myFogUniform");
		locs->myFogColorUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "myFogColorUniform");

		locs->dLightFastUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightFastUniform");
		locs->dLightJitterUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightJitterUniform");
		locs->dLightVoxelShadowsUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightVoxelShadowsUniform");
		locs->dLightVoxelShadowJitterUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightVoxelShadowJitterUniform");
		locs->dLightVoxelShadowJitterMethodUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightVoxelShadowJitterMethodUniform");
		locs->dLightSpecGammaUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightSpecGammaUniform");
		locs->dLightSpecIntensityUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightSpecIntensityUniform");
		locs->dLightSpecBaseReflectivityUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightSpecBaseReflectivityUniform");
		locs->dLightSpecDistanceDecayUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightSpecDistanceDecayUniform");
		locs->dLightSpecDistanceMinUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightSpecDistanceMinUniform");
		locs->dLightIntensityUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightIntensityUniform");
		locs->dLightsCountUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightsCountUniform");
		locs->dLightFastSkipThresholdUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightFastSkipThresholdUniform");
		locs->dLightAddPowUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightAddPowUniform");
		locs->dLightAddPostPowMultUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "dLightAddPostPowMultUniform");
		for (int j = 0; j < MAX_DLIGHTS; j++) {
			locs->dLightsUniformColor[j] = qglGetUniformLocation(program->ShaderIdByBits(i), va("dLightsUniform[%d].color",j));
			locs->dLightsUniformOrigin[j] = qglGetUniformLocation(program->ShaderIdByBits(i), va("dLightsUniform[%d].origin",j));
			locs->dLightsUniformRadius[j] = qglGetUniformLocation(program->ShaderIdByBits(i), va("dLightsUniform[%d].radius",j));
			locs->dLightsUniformMindist[j] = qglGetUniformLocation(program->ShaderIdByBits(i), va("dLightsUniform[%d].mindist",j));
		}
		for (int j = 0; j < MAXLIGHTMAPS_REAL; j++) {
			locs->shaderStylesUniform[j] = qglGetUniformLocation(program->ShaderIdByBits(i), va("shaderStylesUniform[%d]",j));
		}
		locs->shadowLinesCountUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "shadowLinesCountUniform");
		locs->cheapLightsCountUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "cheapLightsCountUniform");
		for (int j = 0; j < MAX_SHADOWLINES; j++) {
			locs->shadowLinesPoint1[j] = qglGetUniformLocation(program->ShaderIdByBits(i), va("shadowLinesUniform[%d].point1",j));
			locs->shadowLinesPoint2[j] = qglGetUniformLocation(program->ShaderIdByBits(i), va("shadowLinesUniform[%d].point2",j));
			locs->shadowLinesWidth[j] = qglGetUniformLocation(program->ShaderIdByBits(i), va("shadowLinesUniform[%d].width",j));
			locs->shadowLinesA[j] = qglGetUniformLocation(program->ShaderIdByBits(i), va("shadowLinesUniform[%d].a",j));
			locs->shadowLinesB[j] = qglGetUniformLocation(program->ShaderIdByBits(i), va("shadowLinesUniform[%d].b",j));
		}

		locs->worldReflectNormalMixUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "worldReflectNormalMixUniform");
		locs->worldReflectGradMultUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "worldReflectGradMultUniform");
		locs->worldReflectPuddleThreshUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "worldReflectPuddleThreshUniform");
		locs->worldReflectMultiSampleUniform = qglGetUniformLocation(program->ShaderIdByBits(i), "worldReflectMultiSampleUniform");

		locs++;
	}
}

static void ReLoadGLSL() {
	qboolean wasActive = qfalse;
	if (r_fboGLSL->integer && ENABLEGLSL) {

		if (fbo.fishEyeActive) {
			qglUseProgram(0);
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

					qglUseProgram(fbo.fishEyeData.tessellationActive ? fishEyeShaderTess->ShaderIdByBits(R_FrameBuffer_GetShaderbits()) : fishEyeShader->ShaderIdByBits(R_FrameBuffer_GetShaderbits()));
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
		qglUseProgram(hdrPqShader->ShaderId(false,false));
		R_DrawQuad(fbo.rollingShutterBuffers[param].current->color, glConfig.vidWidth, glConfig.vidHeight);
		qglUseProgram(0);

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
		qglUseProgram(hdrPqShader->ShaderId(false,false));
		R_DrawQuad(fbo.colorSpaceConv->color, glConfig.vidWidth, glConfig.vidHeight);
		qglUseProgram(0); 

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
		qglUseProgram(hdrPqShader->ShaderId());
		R_DrawQuad(fbo.colorSpaceConv->color, glConfig.vidWidth, glConfig.vidHeight);
		qglUseProgram(0);*/
		
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
		qglUseProgram(hdrPqShader->ShaderId(false,false));
		R_DrawQuad(fbo.main->color, glConfig.vidWidth, glConfig.vidHeight);
		qglUseProgram(0);

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
	qglUseProgram(thermalPostProcessingShader->ShaderId(false, false));
	qglUniform1i(uniformLocationsPostProcessing[0].thermalVisionUniform, r_fboGLSLThermalVision->integer);
	qglUniform1i(uniformLocationsPostProcessing[0].shaderDebugUniform, r_fboGLSLShaderDebug->integer);
	qglUniform1i(uniformLocationsPostProcessing[0].serverTimeUniform, backEnd.refdef.time);
	qglUniform1f(uniformLocationsPostProcessing[0].serverTimeFractionUniform, backEnd.refdef.timeFraction);
	qglUniform1i(uniformLocationsPostProcessing[0].jitterIndexUniform, fbo.fishEyeData.jitterIndex);
	qglUniform1i(uniformLocationsPostProcessing[0].jitterTotalFramesUniform, fbo.fishEyeData.jitterTotalFrames);
	qglUniform1i(uniformLocationsPostProcessing[0].blurEarlyStageUniform, didEarlyBlur ? 2 : 0);
	R_DrawQuad(	fbo.postprocessing->color, glConfig.vidWidth * superSampleMultiplier, glConfig.vidHeight * superSampleMultiplier,true);
	qglUseProgram(0);
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