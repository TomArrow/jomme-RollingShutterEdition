// tr_QuickSprite.cpp: implementation of the CQuickSpriteSystem class.
//
//////////////////////////////////////////////////////////////////////
//#include "../server/exe_headers.h"
#include "tr_local.h"

#include "tr_quicksprite.h"

void R_BindAnimatedImage( textureBundle_t *bundle );

//extern color4f_t	tmpScaledColors[SHADER_MAX_VERTEXES];

//////////////////////////////////////////////////////////////////////
// Singleton System
//////////////////////////////////////////////////////////////////////
CQuickSpriteSystem SQuickSprite;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CQuickSpriteSystem::CQuickSpriteSystem()
{
	int i;

	for (i=0; i<SHADER_MAX_VERTEXES; i+=4)
	{
		// Bottom right
		mTextureCoords[i+0][0] = 1.0;
		mTextureCoords[i+0][1] = 1.0;
		// Top right
		mTextureCoords[i+1][0] = 1.0;
		mTextureCoords[i+1][1] = 0.0;
		// Top left
		mTextureCoords[i+2][0] = 0.0;
		mTextureCoords[i+2][1] = 0.0;
		// Bottom left
		mTextureCoords[i+3][0] = 0.0;
		mTextureCoords[i+3][1] = 1.0;
	}
}

CQuickSpriteSystem::~CQuickSpriteSystem()
{

}


void CQuickSpriteSystem::Flush(void)
{
	if (mNextVert==0)
	{
		return;
	}

	//
	// render the main pass
	//
	R_BindAnimatedImage( mTexBundle );
	GL_State(mGLStateBits);

	//
	// set arrays and lock
	//
	qglTexCoordPointer( 2, GL_FLOAT, 0, mTextureCoords );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY);

	qglEnableClientState( GL_COLOR_ARRAY);
	for (int i = 0; i < mNextVert; i++) {
		Vector4Scale(mColors[i], floatColorsScaleFactor, mColorsScaled[i]);
	}
	qglColorPointer(4, GL_FLOAT, 0, mColorsScaled);
	//qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, mColors );

	qglVertexPointer (3, GL_FLOAT, 16, mVerts);

	if ( qglLockArraysEXT )
	{
		qglLockArraysEXT(0, mNextVert);
		GLimp_LogComment( "glLockArraysEXT\n" );
	}

	qglDrawArrays(GL_QUADS, 0, mNextVert);

	backEnd.pc.c_vertexes += mNextVert;
	backEnd.pc.c_indexes += mNextVert;
	backEnd.pc.c_totalIndexes += mNextVert;

	if (mUseFog)
	{
		//
		// render the fog pass
		//
		GL_Bind( tr.fogImage );
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );

		//
		// set arrays and lock
		//
		qglTexCoordPointer( 2, GL_FLOAT, 0, mFogTextureCoords);
//		qglEnableClientState( GL_TEXTURE_COORD_ARRAY);	// Done above

		qglDisableClientState( GL_COLOR_ARRAY );
		qglColor4ubv((GLubyte *)&mFogColor);

//		qglVertexPointer (3, GL_FLOAT, 16, mVerts);	// Done above

		qglDrawArrays(GL_QUADS, 0, mNextVert);

		// Second pass from fog
		backEnd.pc.c_totalIndexes += mNextVert;
	}

	// 
	// unlock arrays
	//
	if (qglUnlockArraysEXT) 
	{
		qglUnlockArraysEXT();
		GLimp_LogComment( "glUnlockArraysEXT\n" );
	}

	mNextVert=0;
}


void CQuickSpriteSystem::StartGroup(textureBundle_t *bundle, unsigned long glbits, unsigned long fogcolor )
{
	mNextVert = 0;

	mTexBundle = bundle;
	mGLStateBits = glbits;
	if (fogcolor)
	{
		mUseFog = qtrue;
		mFogColor = fogcolor;
	}
	else
	{
		mUseFog = qfalse;
	}

	qglDisable(GL_CULL_FACE);
}


void CQuickSpriteSystem::EndGroup(void)
{
	Flush();

	qglColor4ub(255,255,255,255);
	qglEnable(GL_CULL_FACE);
}




void CQuickSpriteSystem::Add(float *pointdata, color4f_t color, vec2_t fog)
{
	float *curcoord;
	float *curfogtexcoord;
	color4f_t *curcolor;

	if (mNextVert>SHADER_MAX_VERTEXES-4)
	{
		Flush();
	}

	curcoord = mVerts[mNextVert];
	memcpy(curcoord, pointdata, 4*sizeof(vec4_t));

	// Set up color
	curcolor = &mColors[mNextVert];
	Vector4Copy(color, *curcolor); curcolor++;
	Vector4Copy(color, *curcolor); curcolor++;
	Vector4Copy(color, *curcolor); curcolor++;
	Vector4Copy(color, *curcolor); curcolor++;
	//*curcolor++ = *(unsigned long *)color;
	//*curcolor++ = *(unsigned long *)color;
	//*curcolor++ = *(unsigned long *)color;
	//*curcolor++ = *(unsigned long *)color;

	if (fog)
	{
		curfogtexcoord = &mFogTextureCoords[mNextVert][0];
		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		mUseFog=qtrue;
	}
	else
	{
		mUseFog=qfalse;
	}

	mNextVert+=4;
}
