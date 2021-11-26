// Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

#include "tr_mme.h"
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>



bool shotBufPermInitialized = false;
byte* shotBufPerm;
std::thread* saveShotThread;
std::mutex saveShotThreadMutex;



static char *workAlloc = 0;
static char *workAlign = 0;
static int workSize, workUsed;
static qboolean allocFailed = qfalse;

static struct {
	mmeBlurControl_t control;
	mmeBlurBlock_t shot, depth, stencil;
	float	jitter[BLURMAX][2];
} blurData;



shotData_t shotData;

//Data to contain the blurring factors
static struct {
	mmeBlurControl_t control;
	mmeBlurBlock_t dof;
	float	jitter[BLURMAX][2];
} passData;

static struct {
	int pixelCount;
} mainData;

// MME cvars
cvar_t	*mme_aviFormat;

cvar_t	*mme_screenShotFormat;
cvar_t	*mme_screenShotGamma;
cvar_t	*mme_screenShotAlpha;
cvar_t	*mme_jpegQuality;
cvar_t	*mme_jpegDownsampleChroma;
cvar_t	*mme_jpegOptimizeHuffman;
cvar_t	*mme_tgaCompression;
cvar_t	*mme_pngCompression;
cvar_t	*mme_skykey;
cvar_t	*mme_worldShader;
cvar_t	*mme_worldDeform;
cvar_t	*mme_worldBlend;
cvar_t* mme_worldNoCull;
cvar_t	*mme_skyColor;
cvar_t	* mme_cinNoClamp;
cvar_t	*mme_pip;
cvar_t	*mme_blurFrames;
cvar_t	*mme_blurType;
cvar_t	*mme_blurOverlap;
cvar_t	*mme_blurGamma;
cvar_t	*mme_blurJitter;

cvar_t	*mme_dofFrames;
cvar_t	*mme_dofRadius;

cvar_t	*mme_cpuSSE2;
cvar_t	*mme_pbo;

cvar_t	*mme_renderWidth;
cvar_t	*mme_renderHeight;
cvar_t	*mme_workMegs;
cvar_t	*mme_depthFocus;
cvar_t	*mme_depthRange;
cvar_t	*mme_saveOverwrite;
cvar_t	*mme_saveShot;
cvar_t	* mme_saveAEKeyframes;
cvar_t	*mme_saveStencil;
cvar_t	*mme_saveDepth;
cvar_t	* mme_saveADM;
cvar_t  *mme_rollingShutterBlur;
cvar_t  *mme_rollingShutterPixels;
cvar_t  *mme_rollingShutterMultiplier;
cvar_t  * mme_mvShaderLoadOrder;


#ifdef JEDIACADEMY_GLOW
extern std::vector<GLuint> pboIds;
extern std::vector<int> pboRollingShutterProgresses;
extern int rollingShutterBufferCount;
extern int progressOvershoot;
#endif

static void R_MME_MakeBlurBlock( mmeBlurBlock_t *block, int size, mmeBlurControl_t* control ) {
	memset( block, 0, sizeof( *block ) );
	size = (size + 15) & ~15;
	block->count = size / sizeof ( __m64 );
	block->control = control;

	if ( control->totalFrames ) {
		//Allow for floating point buffer with sse
		block->accum = (__m64 *)(workAlign + workUsed);
		workUsed += size * 4;
		if ( workUsed > workSize ) {
			ri.Error( ERR_FATAL, "Failed to allocate %d bytes from the mme_workMegs buffer\n", workUsed );
		}
	} 
	if ( control->overlapFrames ) {
		block->overlap = (__m64 *)(workAlign + workUsed);
		workUsed += control->overlapFrames * size;
		if ( workUsed > workSize ) {
			ri.Error( ERR_FATAL, "Failed to allocate %d bytes from the mme_workMegs buffer\n", workUsed );
		}
	}
}

static void R_MME_CheckCvars( void ) {
	int pixelCount, blurTotal, passTotal;
	mmeBlurControl_t* blurControl = &blurData.control;
	mmeBlurControl_t* passControl = &passData.control;

	pixelCount = glConfig.vidHeight * glConfig.vidWidth;

	if (mme_blurFrames->integer > BLURMAX) {
		ri.Cvar_Set( "mme_blurFrames", va( "%d", BLURMAX) );
	} else if (mme_blurFrames->integer < 0) {
		ri.Cvar_Set( "mme_blurFrames", "0" );
	}

	if (mme_blurOverlap->integer > BLURMAX ) {
		ri.Cvar_Set( "mme_blurOverlap", va( "%d", BLURMAX) );
	} else if (mme_blurOverlap->integer < 0 ) {
		ri.Cvar_Set( "mme_blurOverlap", "0");
	}
	
	if (mme_dofFrames->integer > BLURMAX ) {
		ri.Cvar_Set( "mme_dofFrames", va( "%d", BLURMAX) );
	} else if (mme_dofFrames->integer < 0 ) {
		ri.Cvar_Set( "mme_dofFrames", "0");
	}

	blurTotal = mme_blurFrames->integer + mme_blurOverlap->integer ;
	passTotal = mme_dofFrames->integer;

	if ( (mme_blurType->modified || passTotal != passControl->totalFrames ||  blurTotal != blurControl->totalFrames || pixelCount != mainData.pixelCount || blurControl->overlapFrames != mme_blurOverlap->integer) && !allocFailed ) {
		workUsed = 0;
		
		mainData.pixelCount = pixelCount;

		blurCreate( blurControl, mme_blurType->string, blurTotal );
		blurControl->totalFrames = blurTotal;
		blurControl->totalIndex = 0;
		blurControl->overlapFrames = mme_blurOverlap->integer; 
		blurControl->overlapIndex = 0;

		R_MME_MakeBlurBlock( &blurData.shot, pixelCount * 3, blurControl );
//		R_MME_MakeBlurBlock( &blurData.stencil, pixelCount * 1, blurControl );
		R_MME_MakeBlurBlock( &blurData.depth, pixelCount * 1, blurControl );

		R_MME_JitterTable( blurData.jitter[0], blurTotal );

		//Multi pass data
		blurCreate( passControl, "median", passTotal );
		passControl->totalFrames = passTotal;
		passControl->totalIndex = 0;
		passControl->overlapFrames = 0;
		passControl->overlapIndex = 0;
		R_MME_MakeBlurBlock( &passData.dof, pixelCount * 3, passControl );
		R_MME_JitterTable( passData.jitter[0], passTotal );
	}
	mme_blurOverlap->modified = qfalse;
	mme_blurType->modified = qfalse;
	mme_blurFrames->modified = qfalse;
	mme_dofFrames->modified = qfalse;
}

/* each loop LEFT shotData.take becomes true, but we don't want it when taking RIGHT (stereo) screenshot,
because we may want pause, and it will continue taking LEFT screenshot (and that's wrong) */
void R_MME_DoNotTake( ) {
	shotData.take = qfalse;
}

qboolean R_MME_JitterOrigin( float *x, float *y ) {
	mmeBlurControl_t* passControl = &passData.control;
	*x = 0;
	*y = 0;
	if ( !shotData.take || tr.finishStereo ) {
		shotData.take = qfalse;
		return qfalse;
	}
	if ( passControl->totalFrames ) {
		int i = passControl->totalIndex;
		float scale;
		float focus = shotData.dofFocus;
		float radius = shotData.dofRadius;
		R_MME_ClampDof(&focus, &radius);
		scale = radius * R_MME_FocusScale(focus);
		*x = scale * passData.jitter[i][0];
		*y = -scale * passData.jitter[i][1];
		return qtrue;
	} 
	return qfalse;
}

void R_MME_JitterView( float *pixels, float *eyes ) {
	mmeBlurControl_t* blurControl = &blurData.control;
	mmeBlurControl_t* passControl = &passData.control;
	if ( !shotData.take || tr.finishStereo ) {
		shotData.take = qfalse;
		return;
	}
	if ( blurControl->totalFrames ) {
		int i = blurControl->totalIndex;
		pixels[0] = mme_blurJitter->value * blurData.jitter[i][0];
		pixels[1] = mme_blurJitter->value * blurData.jitter[i][1];
	}
	if ( passControl->totalFrames ) {
		int i = passControl->totalIndex;
		float scale;
		float focus = shotData.dofFocus;
		float radius = shotData.dofRadius;
		R_MME_ClampDof(&focus, &radius);
		scale = r_znear->value / focus;
		scale *= radius * R_MME_FocusScale(focus);;
		eyes[0] = scale * passData.jitter[i][0];
		eyes[1] = scale * passData.jitter[i][1];
	}

}

int R_MME_MultiPassNext( ) {
	mmeBlurControl_t* control = &passData.control;
	byte* outAlloc;
	__m64 *outAlign;
	int index;
	if ( !shotData.take || tr.finishStereo ) {
		shotData.take = qfalse;
		return 0;
	}
	if ( !control->totalFrames )
		return 0;

	index = control->totalIndex;
	outAlloc = (byte *)ri.Hunk_AllocateTempMemory( mainData.pixelCount * 3 + 16);
	outAlign = (__m64 *)((((int)(outAlloc)) + 15) & ~15);

	GLimp_EndFrame();
	R_MME_GetShot( outAlign );
	R_MME_BlurAccumAdd( &passData.dof, outAlign );
	
	tr.capturingDofOrStereo = qtrue;

	ri.Hunk_FreeTempMemory( outAlloc );
	if ( ++(control->totalIndex) < control->totalFrames ) {
		int nextIndex = control->totalIndex;
		if ( ++(nextIndex) >= control->totalFrames && r_stereoSeparation->value == 0.0f )
			tr.latestDofOrStereoFrame = qtrue;
		return 1;
	}
	control->totalIndex = 0;
	R_MME_BlurAccumShift( &passData.dof );
	return 0;
}

static void R_MME_MultiShot( byte * target ) {
	if ( !passData.control.totalFrames ) {
		//Com_Printf("GetShot");
		R_MME_GetShot( target );
	}
	else {
		Com_Printf("MemCpy");
		Com_Memcpy( target, passData.dof.accum, mainData.pixelCount * 3 );
	}
}
static void R_MME_MultiShot( byte * target,int rollingShutterFactor,int rollingShutterProgress,int rollingShutterPixels,int rollingShutterBufferIndex) {
	if ( !passData.control.totalFrames ) {
		//Com_Printf("GetShot");
		R_MME_GetShot( target, rollingShutterFactor,rollingShutterProgress,rollingShutterPixels, rollingShutterBufferIndex);
	}
	else {
		Com_Printf("MemCpy");
		Com_Memcpy( target, passData.dof.accum, mainData.pixelCount * 3 );
	}
}


inline float pq(float in) {
	static const float m1 = 1305.0f / 8192.0f;
	static const float m2 = 2523.0f / 32.0f;
	static const float c1 = 107.0f / 128.0f;
	static const float c2 = 2413.0f / 128.0f;
	static const float c3 = 2392.0f / 128.0f;
	return std::pow((c1 + c2 * std::pow(in, m1)) / (1 + c3 * std::pow(in, m1)), m2);
}



void R_MME_FlushMultiThreading() {
	{
		std::lock_guard<std::mutex> guard(saveShotThreadMutex);
		if (saveShotThread) {
			saveShotThread->join();
			delete saveShotThread;
			saveShotThread = nullptr;
		}
	}
}

qboolean R_MME_TakeShot( void ) {
	int pixelCount;
	byte inSound[MME_SAMPLERATE] = {0};
	int sizeSound = 0;
	qboolean audio = qfalse, audioTaken = qfalse;
	qboolean doGamma;
	mmeBlurControl_t* blurControl = &blurData.control;

	//int mme_rollingShutterPixels = Cvar_Get("mme_rollingShutterPixels","1",);
	int rollingShutterFactor = glConfig.vidHeight/mme_rollingShutterPixels->integer;

	//static int rollingShutterProgress = 0;

	if ( !shotData.take || allocFailed || tr.finishStereo )
		return qfalse;
	shotData.take = qfalse;

	pixelCount = glConfig.vidHeight * glConfig.vidWidth;

	doGamma = (qboolean)(( mme_screenShotGamma->integer || (tr.overbrightBits > 0) ) && (glConfig.deviceSupportsGamma ));
	R_MME_CheckCvars();

	//Special early version using the framebuffer
/*	if ( mme_saveShot->integer && blurControl->totalFrames > 0 &&
		R_FrameBuffer_Blur( blurControl->Float[ blurControl->totalIndex ], blurControl->totalIndex, blurControl->totalFrames ) ) {
		float fps;
		byte *shotBuf;
		if ( ++(blurControl->totalIndex) < blurControl->totalFrames ) 
			return qtrue;
		blurControl->totalIndex = 0;
		shotBuf = (byte *)ri.Hunk_AllocateTempMemory( pixelCount * 3 );
		R_MME_MultiShot( shotBuf );
		if ( doGamma ) 
			R_GammaCorrect( shotBuf, pixelCount * 3 );

		fps = shotData.fps / ( blurControl->totalFrames );
		audio = ri.S_MMEAviImport(inSound, &sizeSound);
		R_MME_SaveShot( &shotData.main, glConfig.vidWidth, glConfig.vidHeight, fps, shotBuf, audio, sizeSound, inSound );
		ri.Hunk_FreeTempMemory( shotBuf );
		return qtrue;
	}*/

	/* Test if we need to do blurred shots */
	if ( blurControl->totalFrames > 0 ) {
		mmeBlurBlock_t *blurShot = &blurData.shot;
		mmeBlurBlock_t *blurDepth = &blurData.depth;
//		mmeBlurBlock_t *blurStencil = &blurData.stencil;

		/* Test if we blur with overlapping frames */
		if ( blurControl->overlapFrames ) {
			/* First frame in a sequence, fill the buffer with the last frames */
			if (blurControl->totalIndex == 0) {
				int i;
				for ( i = 0; i < blurControl->overlapFrames; i++ ) {
					if ( mme_saveShot->integer ) {
						R_MME_BlurOverlapAdd( blurShot, i );
					}
					if ( mme_saveDepth->integer ) {
						R_MME_BlurOverlapAdd( blurDepth, i );
					}
//					if ( mme_saveStencil->integer ) {
//						R_MME_BlurOverlapAdd( blurStencil, i );
//					}
					blurControl->totalIndex++;
				}
			}
			if ( mme_saveShot->integer == 1 ) {
				byte* shotBuf = R_MME_BlurOverlapBuf( blurShot );
				R_MME_MultiShot( shotBuf ); 
				if ( doGamma && mme_blurGamma->integer ) {
					R_GammaCorrect( shotBuf, glConfig.vidWidth * glConfig.vidHeight * 3 );
				}
				R_MME_BlurOverlapAdd( blurShot, 0 );
			}
			if ( mme_saveDepth->integer == 1 ) {
				R_MME_GetDepth( R_MME_BlurOverlapBuf( blurDepth ) ); 
				R_MME_BlurOverlapAdd( blurDepth, 0 );
			}
//			if ( mme_saveStencil->integer == 1 ) {
//				R_MME_GetStencil( R_MME_BlurOverlapBuf( blurStencil ) ); 
//				R_MME_BlurOverlapAdd( blurStencil, 0 );
//			}
			blurControl->overlapIndex++;
			blurControl->totalIndex++;
		} else {
			byte *outAlloc;
			__m64 *outAlign;
			outAlloc = (byte *)ri.Hunk_AllocateTempMemory( pixelCount * 3 + 16);
			outAlign = (__m64 *)((((int)(outAlloc)) + 15) & ~15);

			if ( mme_saveShot->integer == 1 ) {
				R_MME_MultiShot( (byte*)outAlign );
				if ( doGamma && mme_blurGamma->integer ) {
					R_GammaCorrect( (byte *) outAlign, pixelCount * 3 );
				}
				R_MME_BlurAccumAdd( blurShot, outAlign );
			}

			if ( mme_saveDepth->integer == 1 ) {
				R_MME_GetDepth( (byte *)outAlign );
				R_MME_BlurAccumAdd( blurDepth, outAlign );
			}

//			if ( mme_saveStencil->integer == 1 ) {
//				R_MME_GetStencil( (byte *)outAlign );
//				R_MME_BlurAccumAdd( blurStencil, outAlign );
//			}
			ri.Hunk_FreeTempMemory( outAlloc );
			blurControl->totalIndex++;
		}

		if ( blurControl->totalIndex >= blurControl->totalFrames ) {
			float fps;
			blurControl->totalIndex = 0;

			fps = shotData.fps / ( blurControl->totalFrames );
		
			if ( mme_saveShot->integer == 1 ) {
				R_MME_BlurAccumShift( blurShot );
				if (doGamma && !mme_blurGamma->integer)
					R_GammaCorrect( (byte *)blurShot->accum, pixelCount * 3);
			}
			if ( mme_saveDepth->integer == 1 )
				R_MME_BlurAccumShift( blurDepth );
//			if ( mme_saveStencil->integer == 1 )
//				R_MME_BlurAccumShift( blurStencil );
		
			audio = ri.S_MMEAviImport(inSound, &sizeSound);
			audioTaken = qtrue;
			// Big test for an rgba shot
			if ( mme_saveShot->integer == 1 && shotData.main.type == mmeShotTypeRGBA ) {
				int i;
				byte *alphaShot = (byte *)ri.Hunk_AllocateTempMemory( pixelCount * 4);
				byte *rgbData = (byte *)(blurShot->accum );
				if ( mme_saveDepth->integer == 1 ) {
					byte *depthData = (byte *)( blurDepth->accum );
					for ( i = 0;i < pixelCount; i++ ) {
						alphaShot[i*4+0] = rgbData[i*3+0];
						alphaShot[i*4+1] = rgbData[i*3+1];
						alphaShot[i*4+2] = rgbData[i*3+2];
						alphaShot[i*4+3] = depthData[i];
					}
/*				} else if ( mme_saveStencil->integer == 1) {
					byte *stencilData = (byte *)( blurStencil->accum );
					for ( i = 0;i < pixelCount; i++ ) {
						alphaShot[i*4+0] = rgbData[i*3+0];
						alphaShot[i*4+1] = rgbData[i*3+1];
						alphaShot[i*4+2] = rgbData[i*3+2];
						alphaShot[i*4+3] = stencilData[i];
					}
*/				}
				R_MME_SaveShot( &shotData.main, glConfig.vidWidth, glConfig.vidHeight, fps, alphaShot, audio, sizeSound, inSound );
				ri.Hunk_FreeTempMemory( alphaShot );
			} else {
				if ( mme_saveShot->integer == 1 )
					R_MME_SaveShot( &shotData.main, glConfig.vidWidth, glConfig.vidHeight, fps, (byte *)( blurShot->accum ), audio, sizeSound, inSound );
				if ( mme_saveDepth->integer == 1 )
					R_MME_SaveShot( &shotData.depth, glConfig.vidWidth, glConfig.vidHeight, fps, (byte *)( blurDepth->accum ), audio, sizeSound, inSound );
//				if ( mme_saveStencil->integer == 1 )
//					R_MME_SaveShot( &shotData.stencil, glConfig.vidWidth, glConfig.vidHeight, fps, (byte *)( blurStencil->accum), audio, sizeSound, inSound );
			}
		}
	} 
	//Com_Printf("FrameInTakeShot");
	if ( mme_saveShot->integer > 1 || (!blurControl->totalFrames && mme_saveShot->integer )) {
		
		
		//byte *shotBuf = (byte *)ri.Hunk_AllocateTempMemory( pixelCount * 5 );
		if (!shotBufPermInitialized) {
#ifdef CAPTURE_FLOAT
			shotBufPerm = (byte*)ri.Hunk_AllocateTempMemory(pixelCount * 5*4);
#else
			shotBufPerm = (byte*)ri.Hunk_AllocateTempMemory(pixelCount * 5);
#endif
			shotBufPermInitialized = true;
		}
		//byte *shotBuf = (byte *)ri.Hunk_AllocateTempMemory( pixelCount * 5 );

		bool hdrConversionDone = false;

		for (int i = 0; i < rollingShutterBufferCount; i++) {

			int& rollingShutterProgress = pboRollingShutterProgresses[i];

			int rsBlurFrameCount = (int)(mme_rollingShutterBlur->value*(float)rollingShutterFactor/mme_rollingShutterMultiplier->value);
			float intensityMultiplier = 1.0f / (float)rsBlurFrameCount;

			//if(rollingShutterProgress >= 0){ // the later pbos have negative offsets because they start capturing later
			if(rollingShutterProgress >= -rsBlurFrameCount){ // the later pbos have negative offsets because they start capturing later. We also make use of this for blur as far as possible.
				

				int rollingShutterProgressReversed = rollingShutterFactor - rollingShutterProgress - 1;

				// 1. Check lines we can write into the current frame.
				//		In short, we can write from the current block up to rsBlurFrameCount blocks to the future,
				//		long as we don't shoot into the next picture.
				int howManyBlocks = min(rsBlurFrameCount, rollingShutterFactor-rollingShutterProgress);
				int negativeOffset = mme_rollingShutterPixels->integer *(howManyBlocks - 1); // Opengl is from bottom up, so we gotta move things around...
				R_FrameBuffer_RollingShutterCapture(i, mme_rollingShutterPixels->integer* rollingShutterProgressReversed- negativeOffset, mme_rollingShutterPixels->integer* howManyBlocks, true, false, intensityMultiplier);

				// 2. Check lines we can write into the next frame
				//		This applies basically only if our blur multiplier is bigger than  ceil(rollingshuttermultiplier)-rollingshuttermultiplier.
				int preProgressOvershootFrames = rsBlurFrameCount -progressOvershoot;
				if (preProgressOvershootFrames > 0) {
					// Possible that we write some.
					int framesLeftToWrite = rollingShutterFactor - rollingShutterProgress;
					if (framesLeftToWrite <= preProgressOvershootFrames) { // Otherwise too early.
						int blockOffset = preProgressOvershootFrames - framesLeftToWrite - rsBlurFrameCount;
						int blockOffsetReversed = rollingShutterFactor - blockOffset - 1;
						int howManyBlocks = min(rsBlurFrameCount, rollingShutterFactor - blockOffset);
						int negativeOffset = mme_rollingShutterPixels->integer * (howManyBlocks - 1); // Opengl is from bottom up, so we gotta move things around...
						R_FrameBuffer_RollingShutterCapture(i, mme_rollingShutterPixels->integer* blockOffsetReversed- negativeOffset, mme_rollingShutterPixels->integer* howManyBlocks, true, true, intensityMultiplier);
					}
				}


				//R_FrameBuffer_RollingShutterCapture(i, mme_rollingShutterPixels->integer* rollingShutterProgressReversed, mme_rollingShutterPixels->integer,true,false);
				 

				if (rollingShutterProgress == rollingShutterFactor-1){

					R_MME_FlushMultiThreading();
					R_MME_MultiShot(shotBufPerm, rollingShutterFactor, rollingShutterProgress, mme_rollingShutterPixels->integer, i);

					bool dither = true;

					std::lock_guard<std::mutex> guard(saveShotThreadMutex);
					saveShotThread = new std::thread([&, dither,pixelCount]{
						qboolean audio=qfalse, audioTaken = qfalse;
						int sizeSound = 0;
						byte inSound[MME_SAMPLERATE] = { 0 };

#ifdef CAPTURE_FLOAT



						float* asFloatBuffer = (float*)shotBufPerm;
						if (dither) {

							// Floyd-Steinberg dither
							float oldPixel = 0.0f, newPixel = 0.0f, quantError = 0.0f;
							int stride = glConfig.vidWidth * 3;

							for (int i = 0; i < pixelCount * 3; i++) {

								oldPixel = asFloatBuffer[i]; // Important note: shader adds 0.5 for the rounded casting. keep in mind.
								newPixel = 0.5f + (float)(int)std::clamp(oldPixel, 0.5f, 255.5f);
								shotBufPerm[i] = newPixel;
								// Can we just remove the 0.5 stuff altogether if we add 0.5f to newpixel on generation?
								// oldPixel-0.5f-newPixel == oldPixel - (newPixel+0.5f)? == oldPixel - newPixel - 0.5f. yup, seems so.
								quantError = oldPixel - newPixel;
								asFloatBuffer[i + 3] += quantError * 7.0f / 16.0f; // This is the pixel to the right
								asFloatBuffer[i + stride - 3] += quantError * 3.0f / 16.0f; // This is the pixel to the left in lower row
								asFloatBuffer[i + stride] += quantError * 5.0f / 16.0f; // This is the pixel to below
								asFloatBuffer[i + stride + 3] += quantError * 1.0f / 16.0f; // This is the pixel to below, to the right

								// Normally we'd increase the buffer size because the bottom row of the dithering needs extra space
								// but the shotbuffer is already 5*pixelCount because it was meant to account for depth and whatnot?
							}
						}
						else {

							for (int i = 0; i < pixelCount; i++) {

								shotBufPerm[i * 3 + 0] = asFloatBuffer[i * 3 + 0];
								shotBufPerm[i * 3 + 1] = asFloatBuffer[i * 3 + 1];
								shotBufPerm[i * 3 + 2] = asFloatBuffer[i * 3 + 2];
							}
						}

#endif

						shotData.main.type = mmeShotTypeBGR;

						if (!audioTaken)
							audio = ri.S_MMEAviImport(inSound, &sizeSound);

						audioTaken = qtrue;

						R_MME_SaveShot(&shotData.main, glConfig.vidWidth, glConfig.vidHeight, shotData.fps, shotBufPerm, audio, sizeSound, inSound);
						
						//delete shotDataThreadCopy;
					});

				}
			}
			rollingShutterProgress++;
			if (rollingShutterProgress == rollingShutterFactor) {
				R_FrameBuffer_RollingShutterFlipDoubleBuffer(i);
				//rollingShutterProgress = 0;
				rollingShutterProgress = -progressOvershoot; // Since the rolling shutter multiplier can be a non-integer, sometimes we have to pause rendering frames for a little. Imagine if the rolling shutter is half the shutter speed. Then half the time we're not actually recording anything.
			}

		}
		//ri.Hunk_FreeTempMemory( shotBuf );
	}

	if ( shotData.main.type == mmeShotTypeRGB ) {
/*		if ( mme_saveStencil->integer > 1 || ( !blurControl->totalFrames && mme_saveStencil->integer) ) {
			byte *stencilShot = (byte *)ri.Hunk_AllocateTempMemory( pixelCount * 1);
			R_MME_GetStencil( stencilShot );
			R_MME_SaveShot( &shotData.stencil, glConfig.vidWidth, glConfig.vidHeight, shotData.fps, stencilShot, audio, sizeSound, inSound );
			ri.Hunk_FreeTempMemory( stencilShot );
		}
*/		if ( mme_saveDepth->integer > 1 || ( !blurControl->totalFrames && mme_saveDepth->integer) ) {
			byte *depthShot = (byte *)ri.Hunk_AllocateTempMemory( pixelCount * 1);
			R_MME_GetDepth( depthShot );
			if (!audioTaken && ((mme_saveDepth->integer > 1 && mme_saveShot->integer > 1)
				|| (mme_saveDepth->integer == 1 && mme_saveShot->integer == 1)))
				audio = ri.S_MMEAviImport(inSound, &sizeSound);
			R_MME_SaveShot( &shotData.depth, glConfig.vidWidth, glConfig.vidHeight, shotData.fps, depthShot, audio, sizeSound, inSound );
			ri.Hunk_FreeTempMemory( depthShot );
		}
	}

	

	return qtrue;
}

const void *R_MME_CaptureShotCmd( const void *data ) {
	const captureCommand_t *cmd = (const captureCommand_t *)data;

	if (!cmd->name[0])
		return (const void *)(cmd + 1);

	shotData.take = qtrue;
	shotData.fps = cmd->fps;
	shotData.dofFocus = cmd->focus;
	shotData.dofRadius = cmd->radius;
	if (strcmp( cmd->name, shotData.main.name) || mme_screenShotFormat->modified || mme_screenShotAlpha->modified ) {
		/* Also reset the the other data */
		blurData.control.totalIndex = 0;
		if ( workAlign )
			Com_Memset( workAlign, 0, workUsed );
		Com_sprintf( shotData.main.name, sizeof( shotData.main.name ), "%s", cmd->name );
		Com_sprintf( shotData.depth.name, sizeof( shotData.depth.name ), "%s.depth", cmd->name );
		Com_sprintf( shotData.stencil.name, sizeof( shotData.stencil.name ), "%s.stencil", cmd->name );
		
		mme_screenShotFormat->modified = qfalse;
		mme_screenShotAlpha->modified = qfalse;

		if (!Q_stricmp(mme_screenShotFormat->string, "jpg")) {
			shotData.main.format = mmeShotFormatJPG;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "tga")) {
			shotData.main.format = mmeShotFormatTGA;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "png")) {
			shotData.main.format = mmeShotFormatPNG;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "avi")) {
			shotData.main.format = mmeShotFormatAVI;
		} else {
			shotData.main.format = mmeShotFormatTGA;
		}
		
		//grayscale works fine only with compressed avi :(
		if (shotData.main.format != mmeShotFormatAVI || !mme_aviFormat->integer) {
			shotData.depth.format = mmeShotFormatPNG;
			shotData.stencil.format = mmeShotFormatPNG;
		} else {
			shotData.depth.format = mmeShotFormatAVI;
			shotData.stencil.format = mmeShotFormatAVI;
		}

		shotData.main.type = mmeShotTypeRGB;
		if ( mme_screenShotAlpha->integer ) {
			if ( shotData.main.format == mmeShotFormatPNG )
				shotData.main.type = mmeShotTypeRGBA;
			else if ( shotData.main.format == mmeShotFormatTGA )
				shotData.main.type = mmeShotTypeRGBA;
		}
		shotData.main.counter = -1;
		shotData.depth.type = mmeShotTypeGray;
		shotData.depth.counter = -1;
		shotData.stencil.type = mmeShotTypeGray;
		shotData.stencil.counter = -1;	
	}
	return (const void *)(cmd + 1);	
}

void R_MME_Capture( const char *shotName, float fps, float focus, float radius ) {
	captureCommand_t *cmd;
	
	if ( !tr.registered || !fps ) {
		return;
	}
	cmd = (captureCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	if (mme_dofFrames->integer > 0)
		tr.capturingDofOrStereo = qtrue;
	cmd->commandId = RC_CAPTURE;
	cmd->fps = fps;
	cmd->focus = focus;
	cmd->radius = radius;
	Q_strncpyz( cmd->name, shotName, sizeof( cmd->name ));
}

void R_MME_BlurInfo( int* total, int *index ) {
	*total = mme_blurFrames->integer;
	*index = blurData.control.totalIndex;
	if (*index )
		*index -= blurData.control.overlapFrames;
}

extern std::vector<AECamPosition> AECamPositions;
void R_MME_WriteAECamPath() {
	char fileName[MAX_OSPATH];
	int i;
	/* First see if the file already exist */
	for (i = 0; i < AVI_MAX_FILES; i++) {
		Com_sprintf(fileName, sizeof(fileName), "%s.AECamPath.%03d.txt", shotData.main.name, i);
		if (!FS_FileExists(fileName))
			break;
	}

	fileHandle_t file = FS_FOpenFileWrite(fileName);

	std::string tmpString = "Adobe After Effects 8.0 Keyframe Data\r\n\r\n";
	tmpString += "\tUnits Per Second\t";
	tmpString += std::to_string(shotData.fps);
	tmpString += "\r\n\tSource Width\t";
	tmpString += std::to_string(glConfig.vidWidth);
	tmpString += "\r\n\tSource Height\t";
	tmpString += std::to_string(glConfig.vidHeight);
	tmpString += "\r\n\tSource Pixel Aspect Ratio\t";
	tmpString += std::to_string(1);
	tmpString += "\r\n\tComp Pixel Aspect Ratio\t";
	tmpString += std::to_string(1);
	tmpString += "\r\n";

	// Zoom first. (how to do this exactly?)
	tmpString += "\r\nCamera Options\tZoom\r\n\tFrame\tpixels\t\r\n";
	for (int i = 0; i < AECamPositions.size(); i++) {
		tmpString += "\t";
		tmpString += std::to_string(i);
		tmpString += "\t";
		// Convert fov to zoom
		float zoom = glConfig.vidWidth / (2 * tan(AECamPositions[i].fov/2 * M_PI / 180));

		tmpString += std::to_string(zoom);
		tmpString += "\t\r\n";
	}
	/*
	// X Rotation
	tmpString += "\r\nTransform\tX Rotation\r\n\tFrame\tdegrees\t\r\n";
	for (int i = 0; i < AECamPositions.size(); i++) {
		tmpString += "\t";
		tmpString += std::to_string(i);
		tmpString += "\t";
		float rot = AECamPositions[i].viewAngles[0];

		tmpString += std::to_string(rot);
		tmpString += "\t\r\n";
	}
	
	// Y Rotation
	tmpString += "\r\nTransform\tY Rotation\r\n\tFrame\tdegrees\t\r\n";
	for (int i = 0; i < AECamPositions.size(); i++) {
		tmpString += "\t";
		tmpString += std::to_string(i);
		tmpString += "\t";
		float rot = AECamPositions[i].viewAngles[1];

		tmpString += std::to_string(rot);
		tmpString += "\t\r\n";
	}*/

	// Z Rotation
	tmpString += "\r\nTransform\tRotation\r\n\tFrame\tdegrees\t\r\n";
	for (int i = 0; i < AECamPositions.size(); i++) {
		tmpString += "\t";
		tmpString += std::to_string(i);
		tmpString += "\t";
		float rot = AECamPositions[i].viewAngles[2];

		tmpString += std::to_string(rot);
		tmpString += "\t\r\n";
	}

	// Orientation
	/*tmpString += "\r\nTransform\tOrientation\r\n\tFrame\tX degrees\t\r\n";
	for (int i = 0; i < AECamPositions.size(); i++) {
		tmpString += "\t";
		tmpString += std::to_string(i);
		tmpString += "\t";

		tmpString += std::to_string(AECamPositions[i].viewAngles[0]);
		tmpString += "\t";
		tmpString += std::to_string(AECamPositions[i].viewAngles[1]);
		tmpString += "\t";
		tmpString += std::to_string(AECamPositions[i].viewAngles[2]);
		tmpString += "\t\r\n";
	}*/

	// Position
	tmpString += "\r\nTransform\tPosition\r\n\tFrame\tX pixels\tY pixels\tZ pixels\t\r\n";
	for (int i = 0; i < AECamPositions.size(); i++) {
		tmpString += "\t";
		tmpString += std::to_string(i);
		tmpString += "\t";
		tmpString += std::to_string(AECamPositions[i].viewOrg[0]);
		tmpString += "\t";
		tmpString += std::to_string(-AECamPositions[i].viewOrg[2]);// In AE x is right/left, y is height and z is depth. Also, needs inversion of height
		tmpString += "\t";
		tmpString += std::to_string(AECamPositions[i].viewOrg[1]);
		tmpString += "\t\r\n";
	}
	
	tmpString += "\r\nTransform\tPoint of Interest\r\n\tFrame\tX pixels\tY pixels\tZ pixels\t\r\n";
	for (int i = 0; i < AECamPositions.size(); i++) {
		tmpString += "\t";
		tmpString += std::to_string(i);
		tmpString += "\t";
		vec3_t dir; 
		vec3_t poi;
		VectorScale(AECamPositions[i].viewAxis[0],100,dir); // just so its easier to see in AE
		VectorAdd(AECamPositions[i].viewOrg, dir, poi);
		tmpString += std::to_string(poi[0]);
		tmpString += "\t";
		tmpString += std::to_string(-poi[2]); // In AE x is right/left, y is height and z is depth. Also, needs inversion of height
		tmpString += "\t";
		tmpString += std::to_string(poi[1]);
		tmpString += "\t\r\n";
	}

	tmpString += "\r\n\r\nEnd of Keyframe Data\r\n";


	FS_Write(tmpString.c_str(),tmpString.size(),file);

	FS_FCloseFile(file);

	AECamPositions.clear();
}

void R_MME_Shutdown(void) {

	R_MME_FlushMultiThreading();
	R_MME_WriteAECamPath();
	aviClose( &shotData.main.avi );
	aviClose( &shotData.depth.avi );
	aviClose( &shotData.stencil.avi );
}

void R_MME_Init(void) {

	// MME cvars
	mme_aviFormat = ri.Cvar_Get ("mme_aviFormat", "0", CVAR_ARCHIVE);

	mme_jpegQuality = ri.Cvar_Get ("mme_jpegQuality", "90", CVAR_ARCHIVE);
	mme_jpegDownsampleChroma = ri.Cvar_Get ("mme_jpegDownsampleChroma", "0", CVAR_ARCHIVE);
	mme_jpegOptimizeHuffman = ri.Cvar_Get ("mme_jpegOptimizeHuffman", "1", CVAR_ARCHIVE);
	mme_screenShotFormat = ri.Cvar_Get ("mme_screenShotFormat", "png", CVAR_ARCHIVE);
	mme_screenShotGamma = ri.Cvar_Get ("mme_screenShotGamma", "0", CVAR_ARCHIVE);
	mme_screenShotAlpha = ri.Cvar_Get ("mme_screenShotAlpha", "0", CVAR_ARCHIVE);
	mme_tgaCompression = ri.Cvar_Get ("mme_tgaCompression", "1", CVAR_ARCHIVE);
	mme_pngCompression = ri.Cvar_Get("mme_pngCompression", "5", CVAR_ARCHIVE);
	mme_skykey = ri.Cvar_Get( "mme_skykey", "0", CVAR_ARCHIVE );
	mme_pip = ri.Cvar_Get( "mme_pip", "0", CVAR_CHEAT );	//-
	mme_worldShader = ri.Cvar_Get( "mme_worldShader", "0", CVAR_CHEAT );
	mme_worldDeform = ri.Cvar_Get( "mme_worldDeform", "0", CVAR_CHEAT );
	mme_worldBlend = ri.Cvar_Get( "mme_worldBlend", "0", CVAR_CHEAT );
	mme_worldNoCull = ri.Cvar_Get( "mme_worldNoCull", "0", CVAR_CHEAT );
	mme_skyColor = ri.Cvar_Get( "mme_skyColor", "0", CVAR_CHEAT );
	mme_cinNoClamp = ri.Cvar_Get( "mme_cinNoClamp", "0", CVAR_ARCHIVE);
	mme_renderWidth = ri.Cvar_Get( "mme_renderWidth", "0", CVAR_LATCH | CVAR_ARCHIVE );
	mme_renderHeight = ri.Cvar_Get( "mme_renderHeight", "0", CVAR_LATCH | CVAR_ARCHIVE );

	mme_blurFrames = ri.Cvar_Get ( "mme_blurFrames", "0", CVAR_ARCHIVE );
	mme_blurOverlap = ri.Cvar_Get ("mme_blurOverlap", "0", CVAR_ARCHIVE );
	mme_blurType = ri.Cvar_Get ( "mme_blurType", "gaussian", CVAR_ARCHIVE );
	mme_blurGamma = ri.Cvar_Get ( "mme_blurGamma", "0", CVAR_ARCHIVE );
	mme_blurJitter = ri.Cvar_Get ( "mme_blurJitter", "1", CVAR_ARCHIVE );

	mme_dofFrames = ri.Cvar_Get ( "mme_dofFrames", "0", CVAR_ARCHIVE );
	mme_dofRadius = ri.Cvar_Get ( "mme_dofRadius", "2", CVAR_ARCHIVE );

	mme_cpuSSE2 = ri.Cvar_Get ( "mme_cpuSSE2", "0", CVAR_ARCHIVE );
	mme_pbo = ri.Cvar_Get ( "mme_pbo", "1", CVAR_ARCHIVE );
	
	mme_depthRange = ri.Cvar_Get ( "mme_depthRange", "512", CVAR_ARCHIVE );
	mme_depthFocus = ri.Cvar_Get ( "mme_depthFocus", "1024", CVAR_ARCHIVE );
	mme_saveOverwrite = ri.Cvar_Get ( "mme_saveOverwrite", "0", CVAR_ARCHIVE );
	mme_saveStencil = ri.Cvar_Get ( "mme_saveStencil", "0", CVAR_INTERNAL);//CVAR_ARCHIVE ); //need to rewrite tr_backend.cpp :s
	mme_saveADM = ri.Cvar_Get ( "mme_saveADM", "1", CVAR_ARCHIVE );
	mme_saveDepth = ri.Cvar_Get ( "mme_saveDepth", "0", CVAR_ARCHIVE );
	mme_saveShot = ri.Cvar_Get ( "mme_saveShot", "1", CVAR_ARCHIVE );
	mme_saveAEKeyframes = ri.Cvar_Get ( "mme_saveAEKeyframes", "1", CVAR_ARCHIVE );
	mme_workMegs = ri.Cvar_Get ( "mme_workMegs", "128", CVAR_LATCH | CVAR_ARCHIVE );

	mme_rollingShutterBlur = ri.Cvar_Get ( "mme_rollingShutterBlur", "0.5", CVAR_ARCHIVE ); // float. like rollingshuttermultiplier.
	mme_rollingShutterPixels = ri.Cvar_Get ( "mme_rollingShutterPixels", "1", CVAR_ARCHIVE );
	mme_rollingShutterMultiplier = ri.Cvar_Get ( "mme_rollingShutterMultiplier", "1", CVAR_ARCHIVE );
	mme_mvShaderLoadOrder = ri.Cvar_Get ( "mme_mvShaderLoadOrder", "1", CVAR_ARCHIVE );

	mme_worldShader->modified = qtrue;
	mme_worldDeform->modified = qtrue;
	mme_worldBlend->modified = qtrue;
	mme_skyColor->modified = qtrue;

	Com_Memset( &shotData, 0, sizeof(shotData));
	//CANATODO, not exactly the best way to do this probably, but it works
	if (!workAlloc) {
		workSize = mme_workMegs->integer;
		if (workSize < 64)
			workSize = 64;
		else if (workSize > 512)
			workSize = 512;
		workSize *= 1024 * 1024 / 2; //dividing by 2 because other half is used in stereo
		workAlloc = (char *)calloc( workSize + 16, 1 );
		if (!workAlloc) {
			ri.Printf(PRINT_ALL, "Failed to allocate %d bytes for mme work buffer\n", workSize );
			allocFailed = qtrue;
			return;
		}
		workAlign = (char *)(((int)workAlloc + 15) & ~15);
	}
}
