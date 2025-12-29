
#include "tr_local.h"

#if !defined(G2_H_INC)
	#include "../ghoul2/G2.h"
#endif
#include "../ghoul2/G2_local.h"
#include "matcomp.h"

#pragma warning (disable: 4512)	//default assignment operator could not be gened
#include "../qcommon/disablewarnings.h"

static	int			r_firstSceneDrawSurf;

static	int			r_numdlights;
static	int			r_firstSceneDlight;

static	int			r_numshadowlines;
static	int			r_firstSceneShadowLine;

static	int			r_numwindpoints;
static	int			r_firstSceneWindPoint;

static	int			r_numsceneviews;
static	int			r_firstSceneSceneView;

static	int			r_numcheaplights;
static	int			r_firstSceneCheapLight;

static	int64_t		r_numentities;
static	int			r_firstSceneEntity;
static	int			r_numminientities;
static	int			r_firstSceneMiniEntity;
static	int			refEntParent = -1;

static	int			r_numpolys;
static	int			r_firstScenePoly;

static	int			r_numpolyverts;


/*
====================
R_ToggleSmpFrame

====================
*/
void R_ToggleSmpFrame( void ) {
	if ( r_smp->integer ) {
		// use the other buffers next frame, because another CPU
		// may still be rendering into the current ones
		tr.smpFrame ^= 1;
	} else {
		tr.smpFrame = 0;
	}

	backEndData[tr.smpFrame]->commands.used = 0;

	r_firstSceneDrawSurf = 0;

	r_numdlights = 0;
	r_firstSceneDlight = 0;

	r_numshadowlines = 0;
	r_firstSceneShadowLine = 0;

	r_numsceneviews = 0;
	r_firstSceneSceneView = 0;

	r_numcheaplights = 0;
	r_firstSceneCheapLight = 0;

	r_numwindpoints = 0;
	r_firstSceneWindPoint = 0;

	r_numentities = 0;
	r_firstSceneEntity = 0;
	refEntParent = -1;
	r_numminientities = 0;
	r_firstSceneMiniEntity = 0;

	r_numpolys = 0;
	r_firstScenePoly = 0;

	r_numpolyverts = 0;
}


/*
====================
RE_ClearScene

====================
*/
void RE_ClearScene( void ) {
	r_firstSceneCheapLight = r_numcheaplights;
	r_firstSceneShadowLine = r_numshadowlines;
	r_firstSceneSceneView = r_numsceneviews;
	r_firstSceneDlight = r_numdlights;
	r_firstSceneEntity = r_numentities;
	r_firstScenePoly = r_numpolys;
	refEntParent = -1;
	r_firstSceneMiniEntity = r_numminientities;
}

/*
===========================================================================

DISCRETE POLYS

===========================================================================
*/

/*
=====================
R_AddPolygonSurfaces

Adds all the scene's polys into this view's drawsurf list
=====================
*/
void R_AddPolygonSurfaces( void ) {
	int			i;
	shader_t	*sh;
	srfPoly_t	*poly;

//	tr.currentEntityNum = ENTITYNUM_WORLD;
//	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;
	tr.currentEntityNum = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;

	for ( i = 0, poly = tr.refdef.polys; i < tr.refdef.numPolys ; i++, poly++ ) {
		sh = R_GetShaderByHandle( poly->hShader );
		R_AddDrawSurf( (surfaceType_t *)poly, sh, poly->fogIndex, qfalse );
	}
}

/*
=====================
RE_AddPolyToScene

=====================
*/
void RE_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys ) {
	srfPoly_t	*poly;
	int			i, j;
	int			fogIndex;
	fog_t		*fog;
	vec3_t		bounds[2];

	if ( !tr.registered ) {
		return;
	}

	if ( !hShader ) {
		//ri.Printf( PRINT_WARNING, "WARNING: RE_AddPolyToScene: NULL poly shader\n");
		return;
	}

	for ( j = 0; j < numPolys; j++ ) {
		if ( r_numpolyverts + numVerts > max_polyverts || r_numpolys >= max_polys ) {
	//ent:
	  /*
      NOTE TTimo this was initially a PRINT_WARNING
      but it happens a lot with high fighting scenes and particles
      since we don't plan on changing the const and making for room for those effects
      simply cut this message to developer only
      */
#ifdef DEBUG // Eats up a lot of performance to throw the printf.
			ri.Printf( PRINT_DEVELOPER, "WARNING: RE_AddPolyToScene: r_max_polys or r_max_polyverts reached\n");
#endif
			return;
		}

		poly = &backEndData[tr.smpFrame]->polys[r_numpolys];
		poly->surfaceType = SF_POLY;
		poly->hShader = hShader;
		poly->numVerts = numVerts;
		poly->verts = &backEndData[tr.smpFrame]->polyVerts[r_numpolyverts];
		
		Com_Memcpy( poly->verts, &verts[numVerts*j], numVerts * sizeof( *verts ) );

		// done.
		r_numpolys++;
		r_numpolyverts += numVerts;

		// if no world is loaded
		if ( tr.world == NULL ) {
			fogIndex = 0;
		}
		// see if it is in a fog volume
		else if ( tr.world->numfogs == 1 ) {
			fogIndex = 0;
		} else {
			// find which fog volume the poly is in
			VectorCopy( poly->verts[0].xyz, bounds[0] );
			VectorCopy( poly->verts[0].xyz, bounds[1] );
			for ( i = 1 ; i < poly->numVerts ; i++ ) {
				AddPointToBounds( poly->verts[i].xyz, bounds[0], bounds[1] );
			}
			for ( fogIndex = 1 ; fogIndex < tr.world->numfogs ; fogIndex++ ) {
				fog = &tr.world->fogs[fogIndex]; 
				if ( bounds[1][0] >= fog->bounds[0][0]
					&& bounds[1][1] >= fog->bounds[0][1]
					&& bounds[1][2] >= fog->bounds[0][2]
					&& bounds[0][0] <= fog->bounds[1][0]
					&& bounds[0][1] <= fog->bounds[1][1]
					&& bounds[0][2] <= fog->bounds[1][2] ) {
					break;
				}
			}
			if ( fogIndex == tr.world->numfogs ) {
				fogIndex = 0;
			}
		}
		poly->fogIndex = fogIndex;
	}
}


//=================================================================================


/*
=====================
RE_AddRefEntityToScene

=====================
*/
void RE_AddRefEntityToScene( const refEntity_t *ent ) {
	if ( !tr.registered ) {
		return;
	}
	//if ( r_numentities >= MAX_ENTITIES ) {
	if ( r_numentities >= MAX_REFENTITIES ) {
		return;
	}
	if ( ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE ) {
		ri.Error( ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType );
	}

	backEndData[tr.smpFrame]->entities[r_numentities].e = *ent;
	backEndData[tr.smpFrame]->entities[r_numentities].lightingCalculated = qfalse;

	if (ent->ghoul2)
	{
		CGhoul2Info_v	&ghoul2 = *((CGhoul2Info_v *)ent->ghoul2);

#ifdef _WIN32
		if (!ghoul2[0].mModel)
		{
			DebugBreak();
		}
#endif
	}

	if (ent->reType == RT_ENT_CHAIN)
	{
		refEntParent = r_numentities;
		backEndData[tr.smpFrame]->entities[r_numentities].e.uRefEnt.uMini.miniStart = r_numminientities - r_firstSceneMiniEntity;
		backEndData[tr.smpFrame]->entities[r_numentities].e.uRefEnt.uMini.miniCount = 0;
	}
	else
	{
		refEntParent = -1;
	}

	r_numentities++;
}


/************************************************************************************************
 * RE_AddMiniRefEntityToScene                                                                   *
 *    Adds a mini ref ent to the scene.  If the input parameter is null, it signifies the end   *
 *    of the chain.  Otherwise, if there is a valid chain parent, it will be added to that.     *
 *    If there is no parent, it will be added as a regular ref ent.                             *
 *                                                                                              *
 * Input                                                                                        *
 *    ent: the mini ref ent to be added                                                         *
 *                                                                                              *
 * Output / Return                                                                              *
 *    none                                                                                      *
 *                                                                                              *
 ************************************************************************************************/
void RE_AddMiniRefEntityToScene( const miniRefEntity_t *ent ) 
{
	refEntity_t		*parent;

	if ( !tr.registered ) 
	{
		return;
	}
	if (!ent)
	{
		refEntParent = -1;
		return;
	}

	if ( r_numminientities >= MAX_MINI_ENTITIES) 
	{
		ri.Printf( PRINT_WARNING, S_COLOR_YELLOW "WARNING: Attempting to add too many miniRefEntity_t\n");
		return;
	}
	if ( ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE ) 
	{
		ri.Error( ERR_DROP, "RE_AddMiniRefEntityToScene: bad reType %i", ent->reType );
	}

	if (!r_numentities || refEntParent == -1)
	{
//		ri.Error( ERR_DROP, "RE_AddMiniRefEntityToScene: mini without parent ref ent");
		refEntity_t		tempEnt;

		memcpy(&tempEnt, ent, sizeof(*ent));
		memset(((char *)&tempEnt)+sizeof(*ent), 0, sizeof(tempEnt) - sizeof(*ent));
		RE_AddRefEntityToScene(&tempEnt);
		return;
	}

	parent = &backEndData[tr.smpFrame]->entities[refEntParent].e;
	parent->uRefEnt.uMini.miniCount++;

	backEndData[tr.smpFrame]->miniEntities[r_numminientities].e = *ent;
	r_numminientities++;
}

/*
=====================
RE_AddDynamicLightToScene

=====================
*/
void RE_AddDynamicLightToScene( const vec3_t org, float intensity, float r, float g, float b, int additive, float mindist ) {
	dlight_t	*dl;

	if ( !tr.registered ) {
		return;
	}
	if ( r_numdlights >= MAX_DLIGHTS_TO_SORT) {
		return;
	}
	if ( intensity <= 0 ) {
		return;
	}
	dl = &backEndData[tr.smpFrame]->dlights[r_numdlights++];
	VectorCopy (org, dl->origin);
	dl->radius = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->mindist = mindist;
	dl->additive = additive;
}

/*
=====================
RE_AddLightToScene

=====================
*/
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, float mindist) {
	dlight_t	*dl;

	if (r_newDLights->integer)
	{
		if ( !tr.registered ) {
			return;
		}
		if ( r_numdlights >= MAX_DLIGHTS_TO_SORT) {
			return;
		}
		//dl = &backEnd.refdef.dlights[r_numdlights++];
		dl = &backEndData[tr.smpFrame]->dlights[r_numdlights++];

		dl->mType=DLIGHT_VERTICAL;
		VectorCopy (org, dl->origin);
		dl->color[0] = r;
		dl->color[1] = g;
		dl->color[2] = b;
		dl->mindist = mindist;
		if ( intensity <= 0 ) 
		{
			// projected from viewer
			VectorCopy (tr.viewParms.ori.axis[0], dl->mDirection);
			VectorCopy (tr.viewParms.ori.axis[1], dl->mBasis2);
			VectorCopy (tr.viewParms.ori.axis[2], dl->mBasis3);
			dl->mType=DLIGHT_PROJECTED;
			dl->radius = -intensity;
		}
		else
		{
			dl->radius = intensity;
		}

	//	dl->additive = qtrue;
	}
	else
	{
		RE_AddDynamicLightToScene( org, intensity, r, g, b, qfalse, mindist );
	}
}

void RE_AddCheapLightToScene( const vec3_t org, float intensity, float r, float g, float b, float mindist) {

	dlightCheap_t* dl;

	if (!tr.registered) {
		return;
	}
	if (r_numcheaplights >= MAX_CHEAPLIGHTS_TO_SORT) {
		return;
	}
	if (intensity <= 0) {
		return;
	}
	dl = &backEndData[tr.smpFrame]->cheaplights[r_numcheaplights++];
	VectorCopy(org, dl->origin);
	dl->radius = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->mindist = mindist;
}
qboolean RE_GetShaderLightMultiplier( qhandle_t hshader, vec3_t color) {

	VectorClear(color);
	if (!tr.registered) {
		return qfalse;
	}
	shader_t* shader = R_GetShaderByHandle(hshader);
	if (shader->stages[0] && (shader->stages[0]->stateBits & GLS_SRCBLEND_ONE) && (shader->stages[0]->stateBits & GLS_DSTBLEND_ONE) && shader->stages[0]->bundle[0].image  ) {
		// this shader is additive. bingo.
		VectorCopy(shader->stages[0]->bundle[0].image[0]->averageColor,color);
		return qtrue;
	}
	return qfalse;
}

/*
=====================
RE_AddShadowLineToScene

=====================
*/
void RE_AddShadowLineToScene( const vec3_t p1, const vec3_t p2, float width, float a, float b, int flags ) {
	shadowline_t	*sl;
	vec4_t p1ToP2;

	if ( !tr.registered ) {
		return;
	}
	if ( r_numshadowlines >= MAX_SHADOWLINES_TO_SORT) {
		return;
	}

	sl = &backEndData[tr.smpFrame]->shadowLines[r_numshadowlines++];

	VectorCopy (p1, sl->point1);
	VectorCopy (p2, sl->point2);
	VectorSubtract(p2, p1, p1ToP2);
	VectorMA(p1,0.5f,p1ToP2,sl->middle);
	sl->halfLineLength = VectorLength(p1ToP2)*0.5f;
	sl->width = width;
	sl->a = a;
	sl->b = b;
	sl->flags = flags;

}

/*
=====================
RE_AddWindPointToScene

=====================
*/
void RE_AddWindPointToScene( const vec3_t origin, const vec3_t direction, float intensity, float radius) {
	windpoint_t	*wp;
	vec3_t dir;

	if ( !tr.registered ) {
		return;
	}
	if ( r_numwindpoints >= MAX_WINDPOINTS_TO_SORT) {
		return;
	}

	wp = &backEndData[tr.smpFrame]->windpoints[r_numwindpoints++];

	VectorCopy (origin, wp->origin);
	VectorCopy (direction, dir);
	VectorNormalize(dir);
	wp->direction[0] = dir[0] * wp->intensity;
	wp->direction[1] = dir[1] * wp->intensity;
	wp->centerDirScale = wp->intensity * (1.0f - sqrtf(dir[0] * dir[0] + dir[1] * dir[1])); // whatever part of the force isnt horizontal will just radiate outwards from origin

	wp->intensity = intensity;
	wp->radius = radius;

}

/*
=====================
RE_AddViewToScene

=====================
*/
int RE_AddViewToScene(const vec3_t origin, qboolean is360, qboolean copyAxis, const float* axis ) {
	sceneView_t	*sv;

	if ( !tr.registered ) {
		return -1;
	}
	if (r_numsceneviews >= MAX_SCENE_VIEWS) {
		return -1;
	}

	sv = &backEndData[tr.smpFrame]->sceneViews[r_numsceneviews];

	sv->id = r_numsceneviews;

	VectorCopy (origin, sv->origin);
	if (axis) {
		Com_Memcpy(sv->axis, axis, sizeof(float) * 9);
	}
	else {
		Com_Memset(sv->axis, 0, sizeof(sv->axis));
	}
	sv->is360 = is360;
	sv->copyAxis = copyAxis;

	r_numsceneviews++;

	return sv->id;
}

/*
=====================
RE_AddAdditiveLightToScene

=====================
*/
void RE_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b, float mindist) {
	RE_AddDynamicLightToScene( org, intensity, r, g, b, qtrue, mindist);
}


static int cmpDlightViewOrgDistance(const void* a, const void* b) {
	dlight_t* aa = (dlight_t*)a;
	dlight_t* bb = (dlight_t*)b;
	float dist1, dist2;

	if (!aa->pvsVisible) {
		return 1;
	}
	if (!bb->pvsVisible) {
		return -1;
	}
	

	dist1 = DistanceSquared(aa->origin,tr.refdef.vieworg);
	dist2 = DistanceSquared(bb->origin,tr.refdef.vieworg);

	return dist1 - dist2;
}
static int cmpWindPointViewOrgDistance(const void* a, const void* b) {
	windpoint_t* aa = (windpoint_t*)a;
	windpoint_t* bb = (windpoint_t*)b;
	float dist1, dist2;

	if (!aa->pvsVisible) {
		return 1;
	}
	if (!bb->pvsVisible) {
		return -1;
	}
	

	dist1 = DistanceSquared(aa->origin,tr.refdef.vieworg);
	dist2 = DistanceSquared(bb->origin,tr.refdef.vieworg);

	return dist1 - dist2;
}
static int cmpCheapLightViewOrgDistance(const void* a, const void* b) {
	dlightCheap_t* aa = (dlightCheap_t*)a;
	dlightCheap_t* bb = (dlightCheap_t*)b;
	float dist1, dist2;

	if (!(aa->flags & 4)) { // not visible
		return 1;
	}
	if (!(bb->flags & 4)) { // not visible
		return -1;
	}
	
	dist1 = DistanceSquared(aa->origin,tr.refdef.vieworg);
	dist2 = DistanceSquared(bb->origin,tr.refdef.vieworg);

	return dist1 - dist2;
}

static int cmpShadowLineViewOrgDistance(const void* a, const void* b) {
	shadowline_t* aa = (shadowline_t*)a;
	shadowline_t* bb = (shadowline_t*)b;
	float dist1, dist2;

	if (!(aa->flags & 4)) { // not visible
		return 1;
	}
	if (!(bb->flags & 4)) { // not visible
		return -1;
	}

	dist1 = DistanceSquared(aa->middle,tr.refdef.vieworg);
	dist2 = DistanceSquared(bb->middle,tr.refdef.vieworg);

	return dist1 - dist2;
}

void RE_ApplyPostProcessing(qboolean captureShot) {
	postProcessCommand_t* cmd;

	if (!tr.registered) {
		return;
	}
	cmd = (postProcessCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd) {
		return;
	}
	cmd->capturing = captureShot;
	cmd->commandId = RC_POST_PROCESS;
}

/*
@@@@@@@@@@@@@@@@@@@@@
RE_RenderScene

Draw a 3D view into a part of the window, then return
to 2D drawing.

Rendering a scene may require multiple views to be rendered
to handle mirrors,
@@@@@@@@@@@@@@@@@@@@@
*/
static qboolean timeFractionSet = qfalse;
void RE_RenderScene( const refdef_t *fd ) {
	viewParms_t		parms;
	int				startTime;
	static	int		lastTime = 0;
	sceneView_t*	sceneViews; // extra views to be rendered for stuff like 360 reflections
	int				sceneViewCount;

	if ( !tr.registered ) {
		return;
	}
	GLimp_LogComment( "====== RE_RenderScene =====\n" );

	if ( r_norefresh->integer ) {
		return;
	}

	startTime = ri.Milliseconds();

	if (!tr.world && !( fd->rdflags & RDF_NOWORLDMODEL ) ) {
		ri.Error (ERR_DROP, "R_RenderScene: NULL worldmodel");
	}

	Com_Memcpy( tr.refdef.text, fd->text, sizeof( tr.refdef.text ) );

	tr.refdef.x = fd->x;
	tr.refdef.y = fd->y;
	tr.refdef.width = fd->width;
	tr.refdef.height = fd->height;
	tr.refdef.fov_x = fd->fov_x;
	tr.refdef.fov_y = fd->fov_y;

	VectorCopy( fd->vieworg, tr.refdef.vieworg );
	VectorCopy( fd->viewaxis[0], tr.refdef.viewaxis[0] );
	VectorCopy( fd->viewaxis[1], tr.refdef.viewaxis[1] );
	VectorCopy( fd->viewaxis[2], tr.refdef.viewaxis[2] );
	VectorCopy( fd->viewAngles, tr.refdef.viewAngles ); // for mme so we can export AE cam paths
	Com_Memcpy(tr.refdef.playerPositions,fd->playerPositions,sizeof(tr.refdef.playerPositions)); // Player paths

	tr.refdef.time = fd->time;
	if (!timeFractionSet)
		tr.refdef.timeFraction = 0.0f;
	timeFractionSet = qfalse;
	tr.refdef.frametime = fd->time - lastTime;
	lastTime = fd->time;
	if (tr.refdef.frametime > 500)
	{
		tr.refdef.frametime = 500;
	}
	else if (tr.refdef.frametime < 0)
	{
		tr.refdef.frametime = 0;
	}
	tr.refdef.rdflags = fd->rdflags;

	// copy the areamask data over and note if it has changed, which
	// will force a reset of the visible leafs even if the view hasn't moved
	tr.refdef.areamaskModified = qfalse;
	if ( ! (tr.refdef.rdflags & RDF_NOWORLDMODEL) ) {
		int		areaDiff;
		int		i;

		// compare the area bits
		areaDiff = 0;
		for (i = 0 ; i < MAX_MAP_AREA_BYTES/4 ; i++) {
			areaDiff |= ((int *)tr.refdef.areamask)[i] ^ ((int *)fd->areamask)[i];
			((int *)tr.refdef.areamask)[i] = ((int *)fd->areamask)[i];
		}

		if ( areaDiff ) {
			// a door just opened or something
			tr.refdef.areamaskModified = qtrue;
		}
	}


	// derived info

	tr.refdef.floatTime = tr.refdef.time * 0.001;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs = backEndData[tr.smpFrame]->drawSurfs;

	tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
	tr.refdef.entities = &backEndData[tr.smpFrame]->entities[r_firstSceneEntity];
	tr.refdef.miniEntities = &backEndData[tr.smpFrame]->miniEntities[r_firstSceneMiniEntity];

	sceneViews = &backEndData[tr.smpFrame]->sceneViews[r_firstSceneSceneView];
	sceneViewCount = r_numsceneviews - r_firstSceneSceneView;

	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights = &backEndData[tr.smpFrame]->dlights[r_firstSceneDlight];

	int numVisible = 0;
	for (int i = 0; i < tr.refdef.num_dlights; i++) {
		if (tr.refdef.dlights[i].pvsVisible = R_inPVSAndVisible(tr.refdef.vieworg, tr.refdef.dlights[i].origin)) {
			numVisible++;
		}
	}
	qsort(tr.refdef.dlights, tr.refdef.num_dlights, sizeof(dlight_t), cmpDlightViewOrgDistance);
	tr.refdef.num_dlights = numVisible;
	if (tr.refdef.num_dlights > MAX_DLIGHTS) {
		// sort by distance.
		tr.refdef.num_dlights = MAX_DLIGHTS;
	} 

	tr.refdef.num_windpoints = r_numwindpoints - r_firstSceneWindPoint;
	tr.refdef.windpoints = &backEndData[tr.smpFrame]->windpoints[r_firstSceneWindPoint];

	numVisible = 0;
	for (int i = 0; i < tr.refdef.num_windpoints; i++) {
		if (tr.refdef.windpoints[i].pvsVisible = R_inPVSAndVisible(tr.refdef.vieworg, tr.refdef.windpoints[i].origin)) {
			numVisible++;
		}
	}
	qsort(tr.refdef.windpoints, tr.refdef.num_windpoints, sizeof(windpoint_t), cmpWindPointViewOrgDistance);
	tr.refdef.num_windpoints = numVisible;
	if (tr.refdef.num_windpoints > MAX_WINDPOINTS) {
		// sort by distance.
		tr.refdef.num_windpoints = MAX_WINDPOINTS;
	}

	tr.refdef.num_shadowlines = r_numshadowlines - r_firstSceneShadowLine;
	tr.refdef.shadowlines = &backEndData[tr.smpFrame]->shadowLines[r_firstSceneShadowLine];

	numVisible = 0;
	for (int i = 0; i < tr.refdef.num_shadowlines; i++) {
		if (R_inPVSAndVisible(tr.refdef.vieworg, tr.refdef.shadowlines[i].middle)) {
			tr.refdef.shadowlines[i].flags |= 4;
			numVisible++;
		}
		else {
			tr.refdef.shadowlines[i].flags &= ~4;
		}
	}
	qsort(tr.refdef.shadowlines, tr.refdef.num_shadowlines, sizeof(shadowline_t), cmpShadowLineViewOrgDistance);
	tr.refdef.num_shadowlines = numVisible;
	if (tr.refdef.num_shadowlines > MAX_SHADOWLINES) {
		// sort by distance.
		tr.refdef.num_shadowlines = MAX_SHADOWLINES;
	}
	for (int i = 0; i < tr.refdef.num_shadowlines; i++) {
		if (tr.refdef.shadowlines[i].flags & 1) {
			// foot shadow
			R_LightDirForPoint(tr.refdef.shadowlines[i].middle, tr.refdef.shadowlines[i].lightdir,vec3_origin, tr.refdef.shadowlines[i].lightdir+3,tr.world);
		}
	}

	tr.refdef.num_cheaplights = r_numcheaplights - r_firstSceneCheapLight;
	tr.refdef.cheaplights = &backEndData[tr.smpFrame]->cheaplights[r_firstSceneCheapLight];

	numVisible = 0;
	for (int i = 0; i < tr.refdef.num_cheaplights; i++) {
		if (R_inPVSAndVisible(tr.refdef.vieworg, tr.refdef.cheaplights[i].origin)) {
			tr.refdef.cheaplights[i].flags |= 4;
			numVisible++;
		}
		else {
			tr.refdef.cheaplights[i].flags &= ~4;
		}
	}
	qsort(tr.refdef.cheaplights, tr.refdef.num_cheaplights, sizeof(dlightCheap_t), cmpCheapLightViewOrgDistance);
	tr.refdef.num_cheaplights = numVisible;
	if (tr.refdef.num_cheaplights > MAX_CHEAPLIGHTS) {
		// sort by distance.
		tr.refdef.num_cheaplights = MAX_CHEAPLIGHTS;
	}

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData[tr.smpFrame]->polys[r_firstScenePoly];

	// turn off dynamic lighting globally by clearing all the
	// dlights if it needs to be disabled or if vertex lighting is enabled
	if ( r_dynamiclight->integer == 0 ||
		 r_vertexLight->integer == 1 ) {
		tr.refdef.num_dlights = 0;
	}

	//R_FrameBuffer_SendDLightInfo();

	// a single frame may have multiple scenes draw inside it --
	// a 3D game view, 3D status bar renderings, 3D menus, etc.
	// They need to be distinguished by the light flare code, because
	// the visibility state for a given surface may be different in
	// each scene / view.
	tr.frameSceneNum++;
	tr.sceneCount++;

	// setup view parms for the initial view
	//
	// set up viewport
	// The refdef takes 0-at-the-top y coordinates, so
	// convert to GL's 0-at-the-bottom space
	//
	Com_Memset( &parms, 0, sizeof( parms ) );
	parms.viewportX = tr.refdef.x;
	parms.viewportY = glConfig.vidHeight - ( tr.refdef.y + tr.refdef.height );
	parms.viewportWidth = tr.refdef.width;
	parms.viewportHeight = tr.refdef.height;
	parms.isPortal = qfalse;

	parms.fovX = tr.refdef.fov_x;
	parms.fovY = tr.refdef.fov_y;

	VectorCopy( fd->vieworg, parms.ori.origin );
	VectorCopy( fd->viewaxis[0], parms.ori.axis[0] );
	VectorCopy( fd->viewaxis[1], parms.ori.axis[1] );
	VectorCopy( fd->viewaxis[2], parms.ori.axis[2] );

	VectorCopy( fd->vieworg, parms.pvsOrigin );

	if (!r_fboGLSLFastPreview->integer || tr.captureIsActive) {

		for (int i = 0; i < sceneViewCount; i++,sceneViews++) { // extra views for reflections and such
			viewParms_t sceneViewViewParms = parms;
			sceneViewViewParms.isSceneView = qtrue;
			sceneViewViewParms.sceneView = *sceneViews;
			VectorCopy(sceneViews->origin, sceneViewViewParms.ori.origin);
			VectorCopy(sceneViews->origin, sceneViewViewParms.pvsOrigin);
			if (!sceneViews->copyAxis) {
				VectorCopy(sceneViews->axis[0], sceneViewViewParms.ori.axis[0]);
				VectorCopy(sceneViews->axis[1], sceneViewViewParms.ori.axis[1]);
				VectorCopy(sceneViews->axis[2], sceneViewViewParms.ori.axis[2]);
			}
			R_RenderView(&sceneViewViewParms);
		}
	}

	R_RenderView( &parms );

	// the next scene rendered in this frame will tack on after this one
	r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
	r_firstSceneEntity = r_numentities;
	r_firstSceneSceneView = r_numsceneviews;
	r_firstSceneMiniEntity = r_numminientities;
	r_firstSceneCheapLight = r_numcheaplights;
	r_firstSceneDlight = r_numdlights;
	r_firstSceneShadowLine = r_numshadowlines;
	r_firstScenePoly = r_numpolys;

	refEntParent = -1;

	tr.frontEndMsec += ri.Milliseconds() - startTime;
}

void R_MME_Time(int time) {
	tr.refdef.time = time;
	tr.refdef.floatTime = time * 0.001f;
}
void R_MME_TimeFraction(float timeFraction) {
	tr.refdef.timeFraction = timeFraction;
	timeFractionSet = qtrue;
}
