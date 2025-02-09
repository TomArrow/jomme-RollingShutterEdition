#include "tr_mme.h"
#include <vector>

#ifdef JEDIACADEMY_GLOW
//extern GLuint pboIds[2];
extern std::vector<GLuint> pboIds;
#endif


std::vector<AECamPosition> AECamPositions;
std::vector<std::vector<AEPlayerPosition>> AEPlayerPositions;


void R_MME_GetShot( void* output, int rollingShutterFactor,int rollingShutterProgress,int rollingShutterPixels,int rollingShutterBufferIndex ) {
#ifdef JEDIACADEMY_GLOW
	//bool doSave = rollingShutterProgress == rollingShutterFactor - 1;
	rollingShutterProgress = rollingShutterFactor-rollingShutterProgress-1;

#ifdef CAPTURE_FLOAT
	int multiplier = 4;
#else
	int multiplier = 1;
#endif	
	
	{
		/*GLenum err;
		while ((err = qglGetError()) != GL_NO_ERROR)
		{
			// Process/log the error.
			ri.Printf(PRINT_WARNING, "WARNING: OpenGL error during capture (before): %d \n", (int)err);
		}*/

		// If you ever comment this in again for whatever reason, keep r_fboRollingShutterSupersample in mind.
		//int byteOffset = rollingShutterProgress * 3 * glConfig.vidWidth * rollingShutterPixels*multiplier;
		//GLvoid* byteOffsetAsPointerHack = (GLvoid*)byteOffset; // holy shit this is ugly.
		
		qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[0]);
		
		//R_FrameBuffer_RollingShutterCapture(rollingShutterBufferIndex, rollingShutterPixels * rollingShutterProgress, rollingShutterPixels);

		// map the PBO to process its data by CPU
		if (rollingShutterProgress == 0) {
#ifdef CAPTURE_FLOAT
			R_FrameBuffer_HDRConvert(HDRCONVSOURCE_FBO,rollingShutterBufferIndex);
			R_FrameBuffer_StartHDRRead();
			qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_BGR_EXT, GL_FLOAT, 0);
			R_FrameBuffer_EndHDRRead();
#endif
			 

			GLubyte* ptr = (GLubyte*)qglMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
			if (ptr) {
				memcpy(output, ptr, glConfig.vidHeight * glConfig.vidWidth * 3 * multiplier);
				qglUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
			}
		}
		// back to conventional pixel operation
		qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
	}

#else
	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, output ); 
#endif
}

// Non rolling shutter version
void R_MME_GetShot( void* output ) {
#ifdef JEDIACADEMY_GLOW

#ifdef CAPTURE_FLOAT
	int multiplier = 4;
#else
	int multiplier = 1;
#endif	
	
	{
		
		qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[0]);
		
		// map the PBO to process its data by CPU
#ifdef CAPTURE_FLOAT
		R_FrameBuffer_HDRConvert(HDRCONVSOURCE_MAINFBO);
		R_FrameBuffer_StartHDRRead();
		qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_BGR_EXT, GL_FLOAT, 0);
		R_FrameBuffer_EndHDRRead();
#endif
			 

		GLubyte* ptr = (GLubyte*)qglMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
		if (ptr) {
			memcpy(output, ptr, glConfig.vidHeight * glConfig.vidWidth * 3 * multiplier);
			qglUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
		}
		// back to conventional pixel operation
		qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
	}

#else
	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, output ); 
#endif
}

void R_MME_GetDepth( byte *output ) {
	float focusStart, focusEnd, focusMul;
	float zBase, zAdd, zRange;
	int i, pixelCount;
	byte *temp;

	if ( mme_depthRange->value <= 0 )
		return;
	
	pixelCount = glConfig.vidWidth * glConfig.vidHeight;

	focusStart = mme_depthFocus->value - mme_depthRange->value;
	focusEnd = mme_depthFocus->value + mme_depthRange->value;
	focusMul = 255.0f / (2 * mme_depthRange->value);

	zRange = backEnd.sceneZfar - r_znear->value;
	zBase = ( backEnd.sceneZfar + r_znear->value ) / zRange;
	zAdd =  ( 2 * backEnd.sceneZfar * r_znear->value ) / zRange;

	temp = (byte *)ri.Hunk_AllocateTempMemory( pixelCount * sizeof( float ) );
	qglDepthRange(0, 1.0f );
	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_DEPTH_COMPONENT, GL_FLOAT, temp ); 
	/* Could probably speed this up a bit with SSE but frack it for now */
	for ( i=0 ; i < pixelCount; i++ ) {
		/* Read from the 0 - 1 depth */
		float zVal = ((float *)temp)[i];
		int outVal;
		/* Back to the original -1 to 1 range */
		zVal = zVal * 2.0f - 1.0f;
		/* Back to the original z values */
		zVal = zAdd / ( zBase - zVal );
		/* Clip and scale the range that's been selected */
		if (zVal <= focusStart)
			outVal = 0;
		else if (zVal >= focusEnd)
			outVal = 255;
		else 
			outVal = (zVal - focusStart) * focusMul;
		output[i] = outVal;
	}
	ri.Hunk_FreeTempMemory( temp );
}

void R_MME_SaveShot( mmeShot_t *shot, int width, int height, float fps, byte *inBuf, qboolean audio, int aSize, byte *aBuf ) {
	mmeShotFormat_t format;
	char *extension;
	char *outBuf;
	int outSize;
	char fileName[MAX_OSPATH];

	AECamPosition camPosition;
	camPosition.fov = tr.refdef.fov_x;
	VectorCopy(tr.refdef.vieworg, camPosition.viewOrg);
	VectorCopy(tr.refdef.viewAngles, camPosition.viewAngles);
	VectorCopy(tr.refdef.viewaxis[0], camPosition.viewAxis[0]);
	VectorCopy(tr.refdef.viewaxis[1], camPosition.viewAxis[1]);
	VectorCopy(tr.refdef.viewaxis[2], camPosition.viewAxis[2]);
	AECamPositions.push_back(camPosition);

	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (AEPlayerPositions.size() <= i) {
			AEPlayerPositions.push_back(std::vector<AEPlayerPosition>());
		}
		AEPlayerPosition playerPos;
		VectorCopy(tr.refdef.playerPositions[i], playerPos.origin);
		AEPlayerPositions[i].push_back(playerPos);
	}
	
	format = shot->format;
	switch (format) {
	case mmeShotFormatJPG:
		extension = "jpg";
		break;
	case mmeShotFormatTGA:
		/* Seems hardly any program can handle grayscale tga, switching to png */
		if (shot->type == mmeShotTypeGray) {
			format = mmeShotFormatPNG;
			extension = "png";
		} else {
			extension = "tga";
		}
		break;
	case mmeShotFormatPNG:
		extension = "png";
		break;
	case mmeShotFormatAVI:
		if (audio)
			mmeAviSound( &shot->avi, shot->name, shot->type, width, height, fps, aBuf, aSize );
		mmeAviShot( &shot->avi, shot->name, shot->type, width, height, fps, inBuf, audio );
		return;
	}

	if (shot->counter < 0) {
		int counter = 0;
		while ( counter < 1000000000) {
			Com_sprintf( fileName, sizeof(fileName), "%s.%010d.%s", shot->name, counter, extension);
			if (!ri.FS_FileExists( fileName ))
				break;
			if ( mme_saveOverwrite->integer ) 
				ri.FS_FileErase( fileName );
			counter++;
		}
		if ( mme_saveOverwrite->integer ) {
			shot->counter = 0;
		} else {
			shot->counter = counter;
		}
	} 

	Com_sprintf( fileName, sizeof(fileName), "%s.%010d.%s", shot->name, shot->counter, extension );
	shot->counter++;

	outSize = width * height * 4 + 2048;
	outBuf = (char *)ri.Hunk_AllocateTempMemory( outSize );
	switch ( format ) {
	case mmeShotFormatJPG:
		outSize = SaveJPG( mme_jpegQuality->integer, width, height, shot->type, inBuf, (byte *)outBuf, outSize );
		break;
	case mmeShotFormatTGA:
		outSize = SaveTGA( mme_tgaCompression->integer, width, height, shot->type, inBuf, (byte *)outBuf, outSize );
		break;
	case mmeShotFormatPNG:
		outSize = SavePNG( mme_pngCompression->integer, width, height, shot->type, inBuf, (byte *)outBuf, outSize );
		break;
	default:
		outSize = 0;
	}
	if (outSize)
		ri.FS_WriteFile( fileName, outBuf, outSize );
	ri.Hunk_FreeTempMemory( outBuf );
}

void blurCreate( mmeBlurControl_t* control, const char* type, int frames ) {
	float*  blurFloat = control->Float;
	float	blurMax, strength;
	float	blurHalf = 0.5f * ( frames - 1 );
	float	bestStrength;
	float	floatTotal;
	int		passes, bestTotal;
	int		i;
	
	if (blurHalf <= 0)
		return;

	if ( !Q_stricmp( type, "gaussian")) {
		for (i = 0; i < frames ; i++) {
			double xVal = ((i - blurHalf ) / blurHalf) * 3;
			double expVal = exp( - (xVal * xVal) / 2);
			double sqrtVal = 1.0f / sqrt( 2 * M_PI);
			blurFloat[i] = sqrtVal * expVal;
		}
	} else if (!Q_stricmp( type, "triangle")) {
		for (i = 0; i < frames; i++) {
			if ( i <= blurHalf )
				blurFloat[i] = 1 + i;
			else
				blurFloat[i] = 1 + ( frames - 1 - i);
		}
	} else {
		for (i = 0; i < frames; i++) {
			blurFloat[i] = 1;
		}
	}

	floatTotal = 0;
	blurMax = 0;
	for (i = 0; i < frames; i++) {
		if ( blurFloat[i] > blurMax )
			blurMax = blurFloat[i];
		floatTotal += blurFloat[i];
	}

	floatTotal = 1 / floatTotal;
	for (i = 0; i < frames; i++) 
		blurFloat[i] *= floatTotal;

	bestStrength = 0;
	bestTotal = 0;
	strength = 128;

	/* Check for best 256 match for MMX */
	for (passes = 32;passes >0;passes--) {
		int total = 0;
		for (i = 0; i < frames; i++) 
			total += strength * blurFloat[i];
		if (total > 256) {
			strength *= (256.0 / total);
		} else if (total <= 256) {
			if ( total > bestTotal) {
				bestTotal = total;
				bestStrength = strength;
			}
			strength *= (256.0 / total); 
		} else {
			bestTotal = total;
			bestStrength = strength;
			break;
		}
	}
	for (i = 0; i < frames; i++) {
		control->MMX[i] = bestStrength * blurFloat[i];
	}

	bestStrength = 0;
	bestTotal = 0;
	strength = 128;
	/* Check for best 32768 match for MMX */
	for (passes = 32;passes >0;passes--) {
		int total = 0;
		for (i = 0; i < frames; i++) 
			total += strength * blurFloat[i];
		if ( total > 32768 ) {
			strength *= (32768.0 / total);
		} else if (total <= 32767 ) {
			if ( total > bestTotal) {
				bestTotal = total;
				bestStrength = strength;
			}
			strength *= (32768.0 / total); 
		} else {
			bestTotal = total;
			bestStrength = strength;
			break;
		}
	}
	for (i = 0; i < frames; i++) {
		control->SSE[i] = bestStrength * blurFloat[i];
	}

	control->totalIndex = 0;
	control->totalIndex = frames;
	control->overlapFrames = 0;
	control->overlapIndex = 0;

	_mm_empty();
}

static void MME_AccumClearMMX( void* w, const void* r, short mul, int count ) {
	const __m64 * reader = (const __m64 *) r;
	__m64 *writer = (__m64 *) w;
	int i; 
	__m64 readVal, zeroVal, work0, work1, multiply;
	 multiply = _mm_set1_pi16( mul );
	 zeroVal = _mm_setzero_si64();
	 for (i = count; i>0 ; i--) {
		 readVal = *reader++;
		 work0 = _mm_unpacklo_pi8( readVal, zeroVal );
		 work1 = _mm_unpackhi_pi8( readVal, zeroVal );
		 work0 = _mm_mullo_pi16( work0, multiply );
		 work1 = _mm_mullo_pi16( work1, multiply );
		 writer[0] = work0;
		 writer[1] = work1;
		 writer += 2;
	 }
	 _mm_empty();
}

static void MME_AccumAddMMX( void *w, const void* r, short mul, int count ) {
	const __m64 * reader = (const __m64 *) r;
	__m64 *writer = (__m64 *) w;
	int i;
	__m64 zeroVal, multiply;
	 multiply = _mm_set1_pi16( mul );
	 zeroVal = _mm_setzero_si64();
	 /* Add 2 pixels in a loop */
	 for (i = count ; i>0 ; i--) {
		 __m64 readVal = *reader++;
		 __m64 work0 = _mm_mullo_pi16( multiply, _mm_unpacklo_pi8( readVal, zeroVal ) );
		 __m64 work1 = _mm_mullo_pi16( multiply, _mm_unpackhi_pi8( readVal, zeroVal ) );
		 writer[0] = _mm_add_pi16( writer[0], work0 );
		 writer[1] = _mm_add_pi16( writer[1], work1 );
		 writer += 2;
	 }
	 _mm_empty();
}

static void MME_AccumShiftMMX( const void  *r, void *w, int count ) {
	const __m64 * reader = (const __m64 *) r;
	__m64 *writer = (__m64 *) w;

	int i;
	__m64 work0, work1, work2, work3;
	/* Handle 2 at once */
	for (i = count/2;i>0;i--) {
		work0 = _mm_srli_pi16 (reader[0], 8);
		work1 = _mm_srli_pi16 (reader[1], 8);
		work2 = _mm_srli_pi16 (reader[2], 8);
		work3 = _mm_srli_pi16 (reader[3], 8);
		reader += 4;
		writer[0] = _mm_packs_pu16( work0, work1 );
		writer[1] = _mm_packs_pu16( work2, work3 );
		writer += 2;
	}
	_mm_empty();
}

void R_MME_BlurAccumAdd( mmeBlurBlock_t *block, const __m64 *add ) {
	mmeBlurControl_t* control = block->control;
	int index = control->totalIndex;
	if ( mme_cpuSSE2->integer ) {
		if ( index == 0) {
			MME_AccumClearSSE( block->accum, add, control->SSE[ index ], block->count );
		} else {
			MME_AccumAddSSE( block->accum, add, control->SSE[ index ], block->count );
		}
	} else {
		if ( index == 0) {
			MME_AccumClearMMX( block->accum, add, control->MMX[ index ], block->count );
		} else {
			MME_AccumAddMMX( block->accum, add, control->MMX[ index ], block->count );
		}
	}
}

void R_MME_BlurOverlapAdd( mmeBlurBlock_t *block, int index ) {
	mmeBlurControl_t* control = block->control;
	index = ( index + control->overlapIndex ) % control->overlapFrames;
	R_MME_BlurAccumAdd( block, block->overlap + block->count * index );
}

void R_MME_BlurAccumShift( mmeBlurBlock_t *block  ) {
	if ( mme_cpuSSE2->integer ) {
		MME_AccumShiftSSE( block->accum, block->accum, block->count );
	} else {
		MME_AccumShiftMMX( block->accum, block->accum, block->count );
	}
}

//Replace rad with _rad gogo includes
/* Slightly stolen from blender */
static void RE_jitterate1(float *jit1, float *jit2, int num, float _rad1) {
	int i , j , k;
	float vecx, vecy, dvecx, dvecy, x, y, len;

	for (i = 2*num-2; i>=0 ; i-=2) {
		dvecx = dvecy = 0.0;
		x = jit1[i];
		y = jit1[i+1];
		for (j = 2*num-2; j>=0 ; j-=2) {
			if (i != j){
				vecx = jit1[j] - x - 1.0;
				vecy = jit1[j+1] - y - 1.0;
				for (k = 3; k>0 ; k--){
					if( fabs(vecx)<_rad1 && fabs(vecy)<_rad1) {
						len=  sqrt(vecx*vecx + vecy*vecy);
						if(len>0 && len<_rad1) {
							len= len/_rad1;
							dvecx += vecx/len;
							dvecy += vecy/len;
						}
					}
					vecx += 1.0;

					if( fabs(vecx)<_rad1 && fabs(vecy)<_rad1) {
						len=  sqrt(vecx*vecx + vecy*vecy);
						if(len>0 && len<_rad1) {
							len= len/_rad1;
							dvecx += vecx/len;
							dvecy += vecy/len;
						}
					}
					vecx += 1.0;

					if( fabs(vecx)<_rad1 && fabs(vecy)<_rad1) {
						len=  sqrt(vecx*vecx + vecy*vecy);
						if(len>0 && len<_rad1) {
							len= len/_rad1;
							dvecx += vecx/len;
							dvecy += vecy/len;
						}
					}
					vecx -= 2.0;
					vecy += 1.0;
				}
			}
		}

		x -= dvecx/18.0 ;
		y -= dvecy/18.0;
		x -= floor(x) ;
		y -= floor(y);
		jit2[i] = x;
		jit2[i+1] = y;
	}
	memcpy(jit1,jit2,2 * num * sizeof(float));
}

static void RE_jitterate2(float *jit1, float *jit2, int num, float _rad2) {
	int i, j;
	float vecx, vecy, dvecx, dvecy, x, y;

	for (i=2*num -2; i>= 0 ; i-=2){
		dvecx = dvecy = 0.0;
		x = jit1[i];
		y = jit1[i+1];
		for (j =2*num -2; j>= 0 ; j-=2){
			if (i != j){
				vecx = jit1[j] - x - 1.0;
				vecy = jit1[j+1] - y - 1.0;

				if( fabs(vecx)<_rad2) dvecx+= vecx*_rad2;
				vecx += 1.0;
				if( fabs(vecx)<_rad2) dvecx+= vecx*_rad2;
				vecx += 1.0;
				if( fabs(vecx)<_rad2) dvecx+= vecx*_rad2;

				if( fabs(vecy)<_rad2) dvecy+= vecy*_rad2;
				vecy += 1.0;
				if( fabs(vecy)<_rad2) dvecy+= vecy*_rad2;
				vecy += 1.0;
				if( fabs(vecy)<_rad2) dvecy+= vecy*_rad2;

			}
		}

		x -= dvecx/2 ;
		y -= dvecy/2;
		x -= floor(x) ;
		y -= floor(y);
		jit2[i] = x;
		jit2[i+1] = y;
	}
	memcpy(jit1,jit2,2 * num * sizeof(float));
}

void R_MME_JitterTable(float *jitarr, int num) {

	float jit2tmp[12 + 256*2];
	float* jit2;
	float x, _rad1, _rad2, _rad3;
	int i;
	bool mustDelete = false;

	if(num==0)
		return;
	if (num > 256) {
		jit2 = new float[12 + num * 2];
		mustDelete = true;
	}
	else {
		jit2 = jit2tmp;
	}

	_rad1=  1.0/sqrt((float)num);
	_rad2= 1.0/((float)num);
	_rad3= sqrt((float)num)/((float)num);

	x= 0;
	for(i=0; i<2*num; i+=2) {
		jitarr[i]= x+ _rad1*(0.5-random());
		jitarr[i+1]= ((float)i/2)/num +_rad1*(0.5-random());
		x+= _rad3;
		x -= floor(x);
	}

	for (i=0 ; i<24 ; i++) {
		RE_jitterate1(jitarr, jit2, num, _rad1);
		RE_jitterate1(jitarr, jit2, num, _rad1);
		RE_jitterate2(jitarr, jit2, num, _rad2);
	}
	
	/* finally, move jittertab to be centered around (0,0) */
	for(i=0; i<2*num; i+=2) {
		jitarr[i] -= 0.5;
		jitarr[i+1] -= 0.5;
	}

	if (mustDelete) {
		delete[] jit2;
	}
}

#define FOCUS_CENTRE 128.0f //if focus is 128 or less than it starts blurring far obejcts very slowly

float R_MME_FocusScale(float focus) {
	return (focus < FOCUS_CENTRE) ? ((focus / FOCUS_CENTRE) * (1.0f + ((1.0f - (focus / FOCUS_CENTRE)) * (1.1f - 1.0f)))) : 1.0f; 
}

void R_MME_ClampDof(float *focus, float *radius) {
	if (*radius <= 0.0f && *focus <= 0.0f) *radius = mme_dofRadius->value;
	if (*radius < 0.0f) *radius = 0.0f;	
	if (*focus <= 0.0f) *focus = mme_depthFocus->value;
	if (*focus < 0.001f) *focus = 0.001f;
}
