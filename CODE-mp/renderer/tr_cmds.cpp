#include "tr_local.h"
#include "../client/snd_local.h"

volatile renderCommandList_t	*renderCommandList;

volatile qboolean	renderThreadActive;

extern qboolean ParseDeformAlone(char** text, deformStage_t* output);
extern qboolean ParseBlendAlone(char** text, int* output);
/*
=====================
R_PerformanceCounters
=====================
*/
void R_PerformanceCounters( void ) {
	if ( !r_speeds->integer ) {
		// clear the counters even if we aren't printing
		Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
		Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
		return;
	}

	if (r_speeds->integer == 1) {
		const float texSize = R_SumOfUsedImages( qfalse )/(8*1048576.0f)*(r_texturebits->integer?r_texturebits->integer:glConfig.colorBits);
		ri.Printf (PRINT_ALL, "%i/%i shdrs/srfs %i leafs %i vrts %i/%i tris %.2fMB tex %.2f dc\n",
			backEnd.pc.c_shaders, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes, 
			backEnd.pc.c_indexes/3, backEnd.pc.c_totalIndexes/3, 
			texSize, backEnd.pc.c_overDraw / (float)(glConfig.vidWidth * glConfig.vidHeight) ); 
	} else if (r_speeds->integer == 2) {
		ri.Printf (PRINT_ALL, "(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out, 
			tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out );
		ri.Printf (PRINT_ALL, "(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out, 
			tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out );
	} else if (r_speeds->integer == 3) {
		ri.Printf (PRINT_ALL, "viewcluster: %i\n", tr.viewCluster );
	} else if (r_speeds->integer == 4) {
		if ( backEnd.pc.c_dlightVertexes ) {
			ri.Printf (PRINT_ALL, "dlight srf:%i  culled:%i  verts:%i  tris:%i\n", 
				tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled,
				backEnd.pc.c_dlightVertexes, backEnd.pc.c_dlightIndexes / 3 );
		}
	} 
	else if (r_speeds->integer == 5 )
	{
		ri.Printf( PRINT_ALL, "zFar: %.0f\n", tr.viewParms.zFar );
	}
	else if (r_speeds->integer == 6 )
	{
		ri.Printf( PRINT_ALL, "flare adds:%i tests:%i renders:%i\n", 
			backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders );
	}
	else if (r_speeds->integer == 7) {
		const float texSize = R_SumOfUsedImages(qtrue) / (1048576.0f);
		const float backBuff= glConfig.vidWidth * glConfig.vidHeight * glConfig.colorBits / (8.0f * 1024*1024);
		const float depthBuff= glConfig.vidWidth * glConfig.vidHeight * glConfig.depthBits / (8.0f * 1024*1024);
		const float stencilBuff= glConfig.vidWidth * glConfig.vidHeight * glConfig.stencilBits / (8.0f * 1024*1024);
		ri.Printf (PRINT_ALL, "Tex MB %.2f + buffers %.2f MB = Total %.2fMB\n",
			texSize, backBuff*2+depthBuff+stencilBuff, texSize+backBuff*2+depthBuff+stencilBuff); 
	}

	Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
	Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}


/*
====================
R_InitCommandBuffers
====================
*/
void R_InitCommandBuffers( void ) {
	glConfig.smpActive = qfalse;
	if ( r_smp->integer ) {
		ri.Printf( PRINT_ALL, "Trying SMP acceleration...\n" );
		if ( GLimp_SpawnRenderThread( RB_RenderThread ) ) {
			ri.Printf( PRINT_ALL, "...succeeded.\n" );
			glConfig.smpActive = qtrue;
		} else {
			ri.Printf( PRINT_ALL, "...failed.\n" );
		}
	}
}

/*
====================
R_ShutdownCommandBuffers
====================
*/
void R_ShutdownCommandBuffers( void ) {
	// kill the rendering thread
	if ( glConfig.smpActive ) {
		GLimp_WakeRenderer( NULL );
		glConfig.smpActive = qfalse;
	}
}

/*
====================
R_IssueRenderCommands
====================
*/
int	c_blockedOnRender;
int	c_blockedOnMain;

void R_IssueRenderCommands( qboolean runPerformanceCounters ) {
	renderCommandList_t	*cmdList;

	cmdList = &backEndData[tr.smpFrame]->commands;
	assert(cmdList); // bk001205
	// add an end-of-list command
	*(int *)(cmdList->cmds + cmdList->used) = RC_END_OF_LIST;

	// clear it out, in case this is a sync and not a buffer flip
	cmdList->used = 0;

	if ( glConfig.smpActive ) {
		// if the render thread is not idle, wait for it
		if ( renderThreadActive ) {
			c_blockedOnRender++;
			if ( r_showSmp->integer ) {
				ri.Printf( PRINT_ALL, "R" );
			}
		} else {
			c_blockedOnMain++;
			if ( r_showSmp->integer ) {
				ri.Printf( PRINT_ALL, "." );
			}
		}

		// sleep until the renderer has completed
		GLimp_FrontEndSleep();
	}

	// at this point, the back end thread is idle, so it is ok
	// to look at it's performance counters
	if ( runPerformanceCounters ) {
		R_PerformanceCounters();
	}

	// actually start the commands going
	if ( !r_skipBackEnd->integer ) {
		// let it start on the new batch
		if ( !glConfig.smpActive ) {
			RB_ExecuteRenderCommands( cmdList->cmds );
		} else {
			GLimp_WakeRenderer( cmdList );
		}
	}
}


/*
====================
R_SyncRenderThread

Issue any pending commands and wait for them to complete.
After exiting, the render thread will have completed its work
and will remain idle and the main thread is free to issue
OpenGL calls until R_IssueRenderCommands is called.
====================
*/
void R_SyncRenderThread( void ) {
	if ( !tr.registered ) {
		return;
	}
	R_IssueRenderCommands( qfalse );

	if ( !glConfig.smpActive ) {
		return;
	}
	GLimp_FrontEndSleep();
}

/*
============
R_GetCommandBuffer

make sure there is enough command space, waiting on the
render thread if needed.
============
*/
void *R_GetCommandBuffer( int bytes ) {
	renderCommandList_t	*cmdList;

	cmdList = &backEndData[tr.smpFrame]->commands;

	// always leave room for the end of list command
	if ( cmdList->used + bytes + 4 > MAX_RENDER_COMMANDS ) {
		if ( bytes > MAX_RENDER_COMMANDS - 4 ) {
			ri.Error( ERR_FATAL, "R_GetCommandBuffer: bad size %i", bytes );
		}
		// if we run out of room, just start dropping commands
		return NULL;
	}

	cmdList->used += bytes;

	return cmdList->cmds + cmdList->used - bytes;
}


/*
=============
R_AddDrawSurfCmd

=============
*/
void	R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	drawSurfsCommand_t	*cmd;

	cmd = (drawSurfsCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_SURFS;

	cmd->drawSurfs = drawSurfs;
	cmd->numDrawSurfs = numDrawSurfs;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}

/*
=============
R_AddCaptureHackPortalsCmd

=============
*/
void	R_AddCaptureHackPortalsCmd( GLuint glImage ) {
	captureHackPortalsCommand_t	*cmd;

	cmd = (captureHackPortalsCommand_t*)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_CAPTURE_HACKPORTALS;

	cmd->glImage = glImage;
}


/*
=============
RE_SetColor

Passing NULL will set the color to white
=============
*/
void	RE_SetColor( const float *rgba ) {
	setColorCommand_t	*cmd;

	cmd = (setColorCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SET_COLOR;
	if ( !rgba ) {
		static float colorWhite[4] = { 1, 1, 1, 1 };

		rgba = colorWhite;
	}

	cmd->color[0] = rgba[0];
	cmd->color[1] = rgba[1];
	cmd->color[2] = rgba[2];
	cmd->color[3] = rgba[3];
}


/*
=============
RE_StretchPic
=============
*/
void RE_StretchPic ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	stretchPicCommand_t	*cmd;

	cmd = (stretchPicCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_STRETCH_PIC;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}


/*
=============
RE_DrawLine

x, y, x2 and y2 are in virtual screen coordinates
xadjust is 640 / virtual screen width
yadjust is 480 / virtual screen height
=============
*/
void RE_DrawLine ( float x, float y, float x2, float y2, float width, float s1, float t1,
	float s2, float t2, qhandle_t hShader, float xadjust, float yadjust )
{
	drawLineCommand_t*	cmd;

	cmd = (drawLineCommand_t*)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_LINE;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x * xadjust;
	cmd->y = y * yadjust;
	cmd->x2 = x2 * xadjust;
	cmd->y2 = y2 * yadjust;
	cmd->width = width;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

/*
=============
RE_RotatePic
=============
*/
void RE_RotatePic ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2,float a, qhandle_t hShader ) {
	rotatePicCommand_t	*cmd;

	cmd = (rotatePicCommand_t *) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_ROTATE_PIC;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
	cmd->a = a;
}

/*
=============
RE_RotatePic2
=============
*/
void RE_RotatePic2 ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2,float a, qhandle_t hShader ) {
	rotatePicCommand_t	*cmd;

	cmd = (rotatePicCommand_t *) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_ROTATE_PIC2;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
	cmd->a = a;
}

//#define GETTEXELPTR(x,y) (image->ptr + (image->size[0]-y-1)*image->size[0]*4 + x*4)
#define GETTEXELPTR(x,y) (image->ptr + (y)*image->size[0]*4 + x*4)
#define WRAPSAFEMODULO(value,size) (((value) % (size) + (size)) % (size))
void R_SampleFloatImage(floatTextureImage_t* image, vec2_t coords, vec3_t outColor) {
	float p[2];
	int pi[2][2];
	float r[2];

	for (int i = 0; i < 2; i++) {
		p[i] = coords[i] * (float)image->size[i] - 0.5f;
		pi[0][i] = floor(p[i]);
		pi[1][i] = pi[0][i] + 1.0f;
		//printf("ratio %f minus %f\n", p[i], (float)pi[0][i]);
		r[i] = p[i] - (float)pi[0][i];
		pi[0][i] = WRAPSAFEMODULO(pi[0][i], image->size[i]);
		pi[1][i] = WRAPSAFEMODULO(pi[1][i], image->size[i]);
	}

	vec3_t colors[2][2];
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			float* ptr = GETTEXELPTR(pi[i][0], pi[j][1]);
			VectorCopy(ptr, colors[i][j]);
		}
	}

	//printf("ratios %f %f\n", r[0], r[1]);

	//printf("c00 %f %f %f\n", colors[0][0][0], colors[0][0][1], colors[0][0][2]);
	//printf("c01 %f %f %f\n", colors[0][1][0], colors[0][1][1], colors[0][1][2]);
	//printf("c10 %f %f %f\n", colors[1][0][0], colors[1][0][1], colors[1][0][2]);
	//printf("c11 %f %f %f\n", colors[1][1][0], colors[1][1][1], colors[1][1][2]);

	vec3_t blends[2];
	VectorLerp(r[0], colors[0][0], colors[1][0], blends[0]);
	VectorLerp(r[0], colors[0][1], colors[1][1], blends[1]);
	VectorLerp(r[1], blends[0], blends[1], outColor);

}

static qboolean R_MME_LoadCloudsImage(const char* cloudsImagePatah) {

	int width, height, pixelCount;
	textureImage_t picWrap;
	tr.cloudsImageExists = qfalse;
	R_LoadImage(cloudsImagePatah, &picWrap, &width, &height);
	if (!picWrap.ptr) {
		return qfalse;
	}
	if (width <= 0 || height <= 0) {
		ri.Free(picWrap.ptr);
		return qfalse;
	}
	tr.cloudsImageExists = qtrue;

	for (int i = 0; i < MAX_CLOUDSIMAGE_MIPMAPS; i++) {
		if (tr.cloudsImageData[i].ptr) {
			ri.Free(tr.cloudsImageData[i].ptr);
		}
	}

	tr.cloudsImageData[0].size[0] = height;
	tr.cloudsImageData[0].size[1] = width;

	tr.cloudsImageData[0].ptr = (float*)ri.Malloc(width * height * 4 * sizeof(float), TAG_GENERAL, qfalse); // hm. what's the correct tag to use? this might go crashy crashy on vid_restart dunno
	float* tmpBuf = (float*)ri.Malloc(width * height * 4 * sizeof(float), TAG_GENERAL, qfalse);
	pixelCount = width * height;

	for (int i = 0; i < pixelCount; i++) {
		switch (picWrap.bpc) {
		case BPC_8BIT:
			tr.cloudsImageData[0].ptr[i * 4] = R_sRGBToLinear((float)((byte*)picWrap.ptr)[i * 4] / 255.0f);
			tr.cloudsImageData[0].ptr[i * 4 + 1] = R_sRGBToLinear((float)((byte*)picWrap.ptr)[i * 4 + 1] / 255.0f);
			tr.cloudsImageData[0].ptr[i * 4 + 2] = R_sRGBToLinear((float)((byte*)picWrap.ptr)[i * 4 + 2] / 255.0f);
			break;
		case BPC_16BIT:
			tr.cloudsImageData[0].ptr[i * 4] = R_sRGBToLinear( (float)((unsigned short*)picWrap.ptr)[i * 4] / (float)UINT16_MAX);
			tr.cloudsImageData[0].ptr[i * 4 + 1] = R_sRGBToLinear((float)((unsigned short*)picWrap.ptr)[i * 4 + 1] / (float)UINT16_MAX);
			tr.cloudsImageData[0].ptr[i * 4 + 2] = R_sRGBToLinear((float)((unsigned short*)picWrap.ptr)[i * 4 + 2] / (float)UINT16_MAX);
			break;
		case BPC_32BIT:
			tr.cloudsImageData[0].ptr[i * 4] = R_sRGBToLinear((float)((unsigned int*)picWrap.ptr)[i * 4] / (float)UINT_MAX);
			tr.cloudsImageData[0].ptr[i * 4 + 1] = R_sRGBToLinear((float)((unsigned int*)picWrap.ptr)[i * 4 + 1] / (float)UINT_MAX);
			tr.cloudsImageData[0].ptr[i * 4 + 2] = R_sRGBToLinear((float)((unsigned int*)picWrap.ptr)[i * 4 + 2] / (float)UINT_MAX);
			break;
		case BPC_32FLOAT:
			tr.cloudsImageData[0].ptr[i * 4] = ((float*)picWrap.ptr)[i * 4];
			tr.cloudsImageData[0].ptr[i * 4 + 1] = ((float*)picWrap.ptr)[i * 4 + 1];
			tr.cloudsImageData[0].ptr[i * 4 + 2] = ((float*)picWrap.ptr)[i * 4 + 2];
			break;
		}
		tr.cloudsImageData[0].ptr[i * 4 + 3] = 1.0f;
	}

	Com_Memcpy(tmpBuf, tr.cloudsImageData[0].ptr, width * height * 4 * sizeof(float));


	for (int i = 1; i < MAX_CLOUDSIMAGE_MIPMAPS; i++) {
		if (width > 1 && height > 1) {
			R_MipMap(tmpBuf,width,height);
			width >>= 1;
			height >>= 1;
		}
		tr.cloudsImageData[i].ptr = (float*)ri.Malloc(width * height * 4 * sizeof(float), TAG_GENERAL, qfalse); // hm. what's the correct tag to use? this might go crashy crashy on vid_restart dunno
		Com_Memcpy(tr.cloudsImageData[i].ptr, tmpBuf,  width * height * 4 * sizeof(float));
		tr.cloudsImageData[i].size[0] = width;
		tr.cloudsImageData[i].size[1] = height;
	}


	ri.Free(picWrap.ptr);
	ri.Free(tmpBuf);

	tr.cloudsImage = R_FindImageFile(cloudsImagePatah, qtrue, qtrue, qfalse, GL_REPEAT);

}

static int parseVec4(const char* text, vec4_t out) {
	int matches = sscanf(text, "%f %f %f %f", &out[0], &out[1], &out[2], &out[3]);
	if (matches <= 0) {
		out[0] = out[1] = out[2] = out[3] = 1.0f;
	}
	else if (matches == 1) {
		// Only 1 number. Use as scale in general for colors.
		out[1] = out[2] = out[0];
		out[3] = 1.0f;
	}
	else if (matches == 3) { // Alpha not specified
		out[3] = 1.0f;
	}
	else if (matches == 2) { // First number is color scale, second is alpha
		out[3] = out[1];
		out[1] = out[2] = out[0];
	}
	else {
		// I guess we got all 4? All good.
	}
	return matches;
}

static int parseVec3(const char* text, vec3_t out) {
	int matches = sscanf(text, "%f %f %f", &out[0], &out[1], &out[2]);
	if (matches <= 0) {
		out[0] = out[1] = out[2] = 1.0f;
	}
	else if (matches == 1) {
		// Only 1 number. Use as scale in general for colors.
		out[1] = out[2] = out[0];
	}
	else if (matches == 3) { // Alpha not specified

	}
	else if (matches == 2) { // First number is color scale, second is alpha
		//out[3] = out[1];
		out[1] = out[2] = out[0];
	}
	else {
		// I guess we got all 3? All good.
	}
	return matches;
}

static int parseVec2(const char* text, float* out) {
	int matches = sscanf(text, "%f %f", &out[0], &out[1]);
	if (matches <= 0) {
		out[0] = out[1] = 1.0f;
	}
	else if (matches == 1) {
		// Only 1 number. Use as scale in general for colors.
		out[1] = out[0];
	}
	else {
		// I guess we got all 2? All good.
	}
	return matches;
}

void R_UpdateProjectorClipPlanesEye(int planesmask) {
	// had to randomly flip some signs around to make this baseline work.
	// TODO review this someday and make it actually consistent and logically sound
	for (int i = 0; i < 6; i++) {
		float* plane = &tr.projector.clipPlanesWorld[i][0];
		vec4_t tmp;
		tmp[0] = DotProduct(backEnd.viewParms.ori.axis[0], plane);
		tmp[1] = DotProduct(backEnd.viewParms.ori.axis[1], plane);
		tmp[2] = DotProduct(backEnd.viewParms.ori.axis[2], plane);
		tmp[3] = (-DotProduct(plane, backEnd.viewParms.ori.origin) - plane[3]);
		tr.projector.clipPlanes[i][0] = (-tmp[1]);
		tr.projector.clipPlanes[i][1] = (tmp[2]);
		tr.projector.clipPlanes[i][2] = (-tmp[0]);
		tr.projector.clipPlanes[i][3] = (-tmp[3]);
		if (planesmask & (1 << i))
		{
			Vector4Copy(tr.projector.clipPlanes[i],fboUniformsEx.clipPlanes[i]);
		}
	}
	R_FrameBuffer_SetDynamicUniforms3();
}

static int columnRowNormal[16] = {
	0,1,2,3,
	4,5,6,7,
	8,9,10,11,
	12,13,14,15
};
static int columnRowInvert[16] = {
	0,4,8,12,
	1,5,9,13,
	2,6,10,14,
	3,7,11,15
};

static void R_UpdateProjectorClipPlanes() {
	int i;
	myGlMultMatrix(tr.projector.modelMatrix, tr.projector.projectionMatrix, tr.projector.projectionMatrixForClipPlanes);
#define SETCLIPPLANE(a,b,c,d,e,f) (tr.projector.clipPlanesWorld[a][b] = c tr.projector.projectionMatrixForClipPlanes[d] e tr.projector.projectionMatrixForClipPlanes[f])


	SETCLIPPLANE(CLIP_PLANE_RIGHT, 0, -, 3, +, 0);
	SETCLIPPLANE(CLIP_PLANE_RIGHT, 1, -, 7, +, 4);
	SETCLIPPLANE(CLIP_PLANE_RIGHT, 2, -, 11, +, 8);
	SETCLIPPLANE(CLIP_PLANE_RIGHT, 3, -, 15, +, 12);
	
	SETCLIPPLANE(CLIP_PLANE_LEFT, 0, -, 3, -, 0);
	SETCLIPPLANE(CLIP_PLANE_LEFT, 1, -, 7, -, 4);
	SETCLIPPLANE(CLIP_PLANE_LEFT, 2, -, 11, -, 8);
	SETCLIPPLANE(CLIP_PLANE_LEFT, 3, -, 15, -, 12);

	SETCLIPPLANE(CLIP_PLANE_BOTTOM, 0, -, 3, +, 1);
	SETCLIPPLANE(CLIP_PLANE_BOTTOM, 1, -, 7, +, 5);
	SETCLIPPLANE(CLIP_PLANE_BOTTOM, 2, -, 11, +, 9);
	SETCLIPPLANE(CLIP_PLANE_BOTTOM, 3, -, 15, +, 13);

	SETCLIPPLANE(CLIP_PLANE_TOP, 0, -, 3, -, 1);
	SETCLIPPLANE(CLIP_PLANE_TOP, 1, -, 7, -, 5);
	SETCLIPPLANE(CLIP_PLANE_TOP, 2, -, 11, -, 9);
	SETCLIPPLANE(CLIP_PLANE_TOP, 3, -, 15, -, 13);

	// these 2 dont work:
	// the above ones were already bullshit random flipping around signs till it worked
	// and these i dont really need so i didnt bother fixing them up
	// the whole math everywhere here is a giant confused heap of trash :)
	SETCLIPPLANE(CLIP_PLANE_FAR, 0, +, 3, -, 2);
	SETCLIPPLANE(CLIP_PLANE_FAR, 1, +, 7, -, 6);
	SETCLIPPLANE(CLIP_PLANE_FAR, 2, +, 11, -, 10);
	SETCLIPPLANE(CLIP_PLANE_FAR, 3, +, 15, -, 14);

	SETCLIPPLANE(CLIP_PLANE_NEAR, 0, +, 3, +, 2);
	SETCLIPPLANE(CLIP_PLANE_NEAR, 1, +, 7, +, 6);
	SETCLIPPLANE(CLIP_PLANE_NEAR, 2, +, 11, +, 10);
	SETCLIPPLANE(CLIP_PLANE_NEAR, 3, +, 15, +, 14);

	for (i = 0; i < 6; i++) {
		tr.projector.clipPlanesWorld[i][3] /= VectorNormalize(tr.projector.clipPlanesWorld[i]);

	}
	//tr.projector.clipPlanesWorld[2][0] = 0;
	//tr.projector.clipPlanesWorld[2][1] = 1;
	//tr.projector.clipPlanesWorld[2][2] = 0;
	//tr.projector.clipPlanesWorld[2][3] = 1744;
}

static void R_UpdateProjectorModelMatrix() {

	float	viewerMatrix[16];
	const float* origin = tr.projector.pos;
	vec3_t		axis[3];		// orientation in world
	AnglesToAxis(tr.projector.ang, axis);

	viewerMatrix[0] = axis[0][0];
	viewerMatrix[4] = axis[0][1];
	viewerMatrix[8] = axis[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] * viewerMatrix[8];

	viewerMatrix[1] = axis[1][0];
	viewerMatrix[5] = axis[1][1];
	viewerMatrix[9] = axis[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] * viewerMatrix[9];

	viewerMatrix[2] = axis[2][0];
	viewerMatrix[6] = axis[2][1];
	viewerMatrix[10] = axis[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] * viewerMatrix[10];

	viewerMatrix[3] = 0;
	viewerMatrix[7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	myGlMultMatrix(viewerMatrix, s_flipMatrix, tr.projector.modelMatrix);

	__gluInvertMatrixf(tr.projector.modelMatrix,tr.projector.modelMatrixInverse);

	R_UpdateProjectorClipPlanes();
}

static void R_UpdateProjectorMatrix(void) {
	float	xmin, xmax, ymin, ymax;
	float	width, height, depth;
	float	zNear, zFar, zProj, stereoSep;
	float	dx=0, dy=0;
	int		jitterIndex = 0;
	int		jitterTotalFrames = 0;
	vec2_t	pixelJitter, eyeJitter;
	vec3_t	lightsVoxelJitter = { 0 };
	vec3_t	lightsJitter = { 0 };

	//
	// set up projection matrix
	//
	zNear = r_znear->value;
	zFar = 4000;// backEnd.viewParms.zFar; //hmm


	zProj = r_zproj->value;
	stereoSep = 0;// r_stereoSeparation->value;

	ymax = zNear * tan(tr.projector.fov[1] * M_PI / 360.0f);
	ymin = -ymax;

	xmax = zNear * tan(tr.projector.fov[0] * M_PI / 360.0f);
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;
	depth = zFar - zNear;

	pixelJitter[0] = pixelJitter[1] = 0;
	eyeJitter[0] = eyeJitter[1] = 0;


	/* Jitter the view */
	/*
	if (stereoSep <= 0.0f) {
		R_MME_JitterView(pixelJitter, eyeJitter, lightsVoxelJitter, lightsJitter, &jitterIndex, &jitterTotalFrames);
	}
	else if (stereoSep > 0.0f) {
		R_MME_JitterViewStereo(pixelJitter, eyeJitter); // didnt implement light jitter for stero :)
	}

	dx = (pixelJitter[0] * width) / backEnd.viewParms.viewportWidth;
	dy = (pixelJitter[1] * height) / backEnd.viewParms.viewportHeight;
	dx += eyeJitter[0];
	dy += eyeJitter[1];*/


	xmin += dx; xmax += dx;
	ymin += dy; ymax += dy;

	//qglMatrixMode(GL_PROJECTION);
	//qglPushMatrix();
	//qglLoadIdentity();
	//qglFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
	//qglGetFloatv(GL_PROJECTION_MATRIX, tr.projector.projectionMatrix);
	//qglPopMatrix();

	tr.projector.projectionMatrix[0] = 2 * zNear / width;
	tr.projector.projectionMatrix[4] = 0;
	tr.projector.projectionMatrix[8] = (xmax + xmin + 2 * stereoSep) / width;	// normally 0
	tr.projector.projectionMatrix[12] = 2 * zProj * stereoSep / width;

	tr.projector.projectionMatrix[1] = 0;
	tr.projector.projectionMatrix[5] = 2 * zNear / height;
	tr.projector.projectionMatrix[9] = (ymax + ymin) / height;	// normally 0
	tr.projector.projectionMatrix[13] = 0;

	tr.projector.projectionMatrix[2] = 0;
	tr.projector.projectionMatrix[6] = 0;
	//if (r_zinvert->integer) {
	//	tr.projector.projectionMatrix[10] = -(zNear) / depth;
	//	tr.projector.projectionMatrix[14] = -zFar * zNear / depth;
	//}
	//else 
	{
		tr.projector.projectionMatrix[10] = -(zFar + zNear) / depth;
		tr.projector.projectionMatrix[14] = -2 * zFar * zNear / depth;
	}

	tr.projector.projectionMatrix[3] = 0;
	tr.projector.projectionMatrix[7] = 0;
	tr.projector.projectionMatrix[11] = -1;
	tr.projector.projectionMatrix[15] = 0;

	R_UpdateProjectorClipPlanes();

}



/*
====================
RE_BeginFrame

If running in stereo, RE_BeginFrame will be called twice
for each RE_EndFrame
====================
*/
void RE_BeginFrame( stereoFrame_t stereoFrame ) {
	drawBufferCommand_t	*cmd;

	if ( !tr.registered ) {
		return;
	}
	glState.finishCalled = qfalse;

	tr.frameCount++;
	tr.frameSceneNum = 0;

	backEnd.sceneZfar = 2048;

	//
	// do overdraw measurement
	//
	if ( r_measureOverdraw->integer )
	{
		if ( glConfig.stencilBits < 4 )
		{
			ri.Printf( PRINT_ALL, "Warning: not enough stencil bits to measure overdraw: %d\n", glConfig.stencilBits );
			ri.Cvar_Set( "r_measureOverdraw", "0" );
			r_measureOverdraw->modified = qfalse;
		}
		else if ( r_shadows->integer == 2 || r_shadows->integer == 4 )
		{
			ri.Printf( PRINT_ALL, "Warning: stencil shadows and overdraw measurement are mutually exclusive\n" );
			ri.Cvar_Set( "r_measureOverdraw", "0" );
			r_measureOverdraw->modified = qfalse;
		}
		else
		{
			R_SyncRenderThread();
			qglEnable( GL_STENCIL_TEST );
			qglStencilMask( ~0U );
			qglClearStencil( 0U );
			qglStencilFunc( GL_ALWAYS, 0U, ~0U );
			qglStencilOp( GL_KEEP, GL_INCR, GL_INCR );
		}
		r_measureOverdraw->modified = qfalse;
	}
	else
	{
		// this is only reached if it was on and is now off
		if ( r_measureOverdraw->modified ) {
			R_SyncRenderThread();
			qglDisable( GL_STENCIL_TEST );
		}
		r_measureOverdraw->modified = qfalse;
	}

	//
	// texturemode stuff
	//
	if ( r_textureMode->modified || r_ext_texture_filter_anisotropic->modified) {
		R_SyncRenderThread();
		GL_TextureMode( r_textureMode->string );
		r_textureMode->modified = qfalse;
		r_ext_texture_filter_anisotropic->modified = qfalse;
	}

	//
	// gamma stuff
	//
	if ( r_gamma->modified ) {
		r_gamma->modified = qfalse;

		R_SyncRenderThread();
		R_SetColorMappings();
	}

	//
	// console font stuff
	//
	if (r_consoleFont->modified) {
		r_consoleFont->modified = qfalse;

		if (r_consoleFont->integer == 1)
			R_RemapShader("gfx/2d/charsgrid_med", "gfx/2d/code_new_roman", 0);
		else if (r_consoleFont->integer == 2)
			R_RemapShader("gfx/2d/charsgrid_med", "gfx/2d/mplus_1m_bold", 0);
		else
			R_RemapShader("gfx/2d/charsgrid_med", "gfx/2d/charsgrid_med", 0);
	}

    // check for errors
    if ( !r_ignoreGLErrors->integer ) {
        int	err;

		R_SyncRenderThread();
        if ( ( err = qglGetError() ) != GL_NO_ERROR ) {
            ri.Error( ERR_FATAL, "RE_BeginFrame() - glGetError() failed (0x%x)!\n", err );
        }
    }

	if ( mme_worldShader->modified) {
		if (R_FindShaderText( mme_worldShader->string )) {
			tr.mmeWorldShader = R_FindShader( mme_worldShader->string, lightmapsNone, stylesDefault, qtrue );
		} else {
			tr.mmeWorldShader = 0;
		}
		mme_worldShader->modified = qfalse;
	}

	if ( r_fboGLSLProjector ) { // yes im checking if the cvar exists, not if integer is 1
		if (r_fboGLSLProjectorShader->modified) {
			if (R_FindShaderText(r_fboGLSLProjectorShader->string)) {
				tr.projector.shader = R_FindShader(r_fboGLSLProjectorShader->string, lightmapsNone, stylesDefault, qtrue);
			}
			else {
				tr.projector.shader = 0;
			}
			r_fboGLSLProjectorShader->modified = qfalse;
		}
		if (r_fboGLSLProjectorPos->modified) {
			const char* s = r_fboGLSLProjectorPos->string;
			if (!parseVec3(s, tr.projector.pos)) {
				VectorSet(tr.projector.pos, -588, 4516, 216);
			}
			R_UpdateProjectorModelMatrix();
			r_fboGLSLProjectorPos->modified = qfalse;
		}
		if (r_fboGLSLProjectorAng->modified) {
			const char* s = r_fboGLSLProjectorAng->string;
			if (!parseVec3(s, tr.projector.ang)) {
				VectorSet(tr.projector.ang, 0, 90, 0);
			}
			R_UpdateProjectorModelMatrix();
			r_fboGLSLProjectorAng->modified = qfalse;
		}
		if (r_fboGLSLProjectorFov->modified) {
			const char* s = r_fboGLSLProjectorFov->string;
			if (!parseVec2(s, tr.projector.fov)) {
				tr.projector.fov[0] = 40;
				tr.projector.fov[1] = 30;
			}
			R_UpdateProjectorMatrix();
			r_fboGLSLProjectorFov->modified = qfalse;
		}
	}

	if ( mme_skyShader->modified) {
		if (R_FindShaderText(mme_skyShader->string )) {
			tr.mmeSkyShader = R_FindShader(mme_skyShader->string, lightmapsNone, stylesDefault, qtrue );
		} else {
			tr.mmeSkyShader = 0;
		}
		mme_skyShader->modified = qfalse;
	}

	if (mme_cloudsImage->modified || !tr.cloudsImageInited) {
		R_MME_LoadCloudsImage(mme_cloudsImage->string);
		tr.cloudsImageInited = qtrue;
		mme_cloudsImage->modified = qfalse;
	}
	
	if (mme_musicdeform->modified) {
		if (tr.mmeMusicDeform) {
			delete[] tr.mmeMusicDeform;
			tr.mmeMusicDeform = NULL;
			tr.mmeMusicDeformIndex = 0;
			tr.mmeMusicDeformLength = 0;
		}
		if (mme_musicdeform->string && !(strlen(mme_musicdeform->string) == 1 && mme_musicdeform->string[0] == '0')) {
			char musicDeformSoundName[MAX_QPATH];
			Q_strncpyz(musicDeformSoundName, mme_musicdeform->string, sizeof(musicDeformSoundName));
			if (S_FileExists(musicDeformSoundName)) {
				openSound_t* thesound = S_SoundOpen(musicDeformSoundName);
				tr.mmeMusicDeformLength = thesound->totalSamples;
				tr.mmeMusicDeformIndex++;
				tr.mmeMusicDeformSampleRate = thesound->rate;
				tr.mmeMusicDeform = new short[thesound->totalSamples];
				S_SoundRead(thesound, qfalse, thesound->totalSamples, tr.mmeMusicDeform);
				S_SoundClose(thesound);
			}
		}
		/*if (R_FindShaderText(mme_musicdeform->string)) {

			tr.mmeSkyShader = R_FindShader(mme_skyShader->string, lightmapsNone, stylesDefault, qtrue );
		} else {
			tr.mmeSkyShader = 0;
		}*/
		mme_musicdeform->modified = qfalse;
	}

	if (mme_worldDeform->modified) {
		tr.mmeWorldDeformIsSet = qfalse;
		if (Q_stricmp(mme_worldDeform->string,"0")) {
			char* deformTextPointer = mme_worldDeform->string;
			if (ParseDeformAlone(&deformTextPointer, &tr.mmeWorldDeform)) {
				tr.mmeWorldDeformIsSet = qtrue;
			}
		}
		mme_worldDeform->modified = qfalse;
	}
	if (mme_worldBlend->modified) {
		tr.mmeWorldBlendIsSet = qfalse;

		if (Q_stricmp(mme_worldBlend->string, "0")) {
			char* blendTextPointer = mme_worldBlend->string;
			if (ParseBlendAlone(&blendTextPointer, &tr.mmeWorldBlend)) {
				tr.mmeWorldBlendIsSet = qtrue;
			}
		}
		mme_worldBlend->modified = qfalse;
	}
	if (mme_skyColor->modified) {
		tr.mmeSkyColorIsSet = qfalse;
		if (Q_stricmp(mme_skyColor->string, "0")) {
			char* skyColorTextPointer = mme_skyColor->string;
			if (!COM_ParseVec4((const char**)&skyColorTextPointer, &tr.mmeSkyColor)) {
				tr.mmeSkyColorIsSet = qtrue;
			}
		}
		mme_skyColor->modified = qfalse;
	}
	if (r_fboGLSLFogColor && r_fboGLSLFogColor->modified) {
		char* fogColorTextPointer = r_fboGLSLFogColor->string;
		if (COM_ParseVec3((const char**)&fogColorTextPointer, &tr.fboGLSLFogColor)) {
			VectorSet(tr.fboGLSLFogColor,0.1f,0.1f,0.1f);
		}
		r_fboGLSLFogColor->modified = qfalse;
	}
	if (mme_skyTint->modified) {
		tr.mmeSkyTintIsSet = qfalse;
		if (Q_stricmp(mme_skyTint->string, "0")) {
			char* skyTintTextPointer = mme_skyTint->string;
			if (!COM_ParseVec4((const char**)&skyTintTextPointer, &tr.mmeSkyTint)) {
				tr.mmeSkyTintIsSet = qtrue;
			}
		}
		mme_skyTint->modified = qfalse;
	}
	if (mme_fboImageTint->modified) {
		tr.mmeFBOImageTintIsSet = qfalse;
		if (Q_stricmp(mme_fboImageTint->string, "0")) {
			char* imageTintTextPointer = mme_fboImageTint->string;
			if (!COM_ParseVec4((const char**)&imageTintTextPointer, &tr.mmeFBOImageTint)) {
				tr.mmeFBOImageTintIsSet = qtrue;
			}
		}
		mme_fboImageTint->modified = qfalse;
	}

	if (r_stencilShadowColor->modified) {
		const char* stencilShadowColorTextPointer = r_stencilShadowColor->string;
		if (!parseVec4(stencilShadowColorTextPointer, tr.stencilShadowColor)) {
			Vector4Set(tr.stencilShadowColor,0.6f,0.6f,0.6f,1.0f);
		}
		r_stencilShadowColor->modified = qfalse;
	}

	//
	// draw buffer stuff
	//
	cmd = (drawBufferCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_BUFFER;

	if ( glConfig.stereoEnabled ) {
		if ( stereoFrame == STEREO_LEFT ) {
			cmd->buffer = (int)GL_BACK_LEFT;
		} else if ( stereoFrame == STEREO_RIGHT ) {
			cmd->buffer = (int)GL_BACK_RIGHT;
		} else {
			ri.Error( ERR_FATAL, "RE_BeginFrame: Stereo is enabled, but stereoFrame was %i", stereoFrame );
		}
	} else {
/*		if (r_stereoSeparation->value != 0) {
			if ( stereoFrame == STEREO_LEFT ) {
				cmd->buffer = (int)GL_BACK_LEFT;
			} else if ( stereoFrame == STEREO_RIGHT ) {
				cmd->buffer = (int)GL_BACK_RIGHT;
			} else {
				ri.Error( ERR_FATAL, "RE_BeginFrame: Stereo is enabled, but stereoFrame was %i", stereoFrame );
			}
		} else */if ( stereoFrame != STEREO_CENTER ) {
			ri.Error( ERR_FATAL, "RE_BeginFrame: Stereo is disabled, but stereoFrame was %i", stereoFrame );
		}
		if ( !Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) ) {
			cmd->buffer = (int)GL_FRONT;
		} else {
			cmd->buffer = (int)GL_BACK;
		}
	}
}


/*
=============
RE_EndFrame

Returns the number of msec spent in the back end
=============
*/
void RE_EndFrame( int *frontEndMsec, int *backEndMsec ) {
	swapBuffersCommand_t	*cmd;

	if ( !tr.registered ) {
		return;
	}
	cmd = (swapBuffersCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SWAP_BUFFERS;

	R_IssueRenderCommands( qtrue );

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current ones
	R_ToggleSmpFrame();

	if ( frontEndMsec ) {
		*frontEndMsec = tr.frontEndMsec;
	}
	tr.frontEndMsec = 0;
	if ( backEndMsec ) {
		*backEndMsec = backEnd.pc.msec;
	}
	backEnd.pc.msec = 0;
}

