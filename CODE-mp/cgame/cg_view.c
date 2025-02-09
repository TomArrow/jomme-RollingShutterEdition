// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering
#include "cg_local.h"

#if !defined(CL_LIGHT_H_INC)
	#include "cg_lights.h"
#endif

#ifdef RELDEBUG
//#pragma optimize("", off)
#endif

#define MASK_CAMERACLIP (MASK_SOLID|CONTENTS_PLAYERCLIP)
#define CAMERA_SIZE	4

static int GetCameraClip( void ) {
	return (MASK_SOLID|CONTENTS_PLAYERCLIP);
}

/*
=============================================================================

  MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like 
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.

=============================================================================
*/

/*
=================
CG_TestModel_f

Creates an entity in front of the current position, which
can then be moved around
=================
*/
void CG_TestModel_f (void) {
	vec3_t		angles;

	memset( &cg.testModelEntity, 0, sizeof(cg.testModelEntity) );
	if ( trap_Argc() < 2 ) {
		return;
	}

	Q_strncpyz (cg.testModelName, CG_Argv( 1 ), MAX_QPATH );
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );

	if ( trap_Argc() == 3 ) {
		cg.testModelEntity.backlerp = atof( CG_Argv( 2 ) );
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if (! cg.testModelEntity.hModel ) {
		CG_Printf( "Can't register model\n" );
		return;
	}

	VectorMA( cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin );

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefViewAngles[1];
	angles[ROLL] = 0;

	AnglesToAxis( angles, cg.testModelEntity.axis );
	cg.testGun = qfalse;
}

/*
=================
CG_TestGun_f

Replaces the current view weapon with the given model
=================
*/
void CG_TestGun_f (void) {
	CG_TestModel_f();
	cg.testGun = qtrue;
	//cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;

	// rww - 9-13-01 [1-26-01-sof2]
	cg.testModelEntity.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;
}


void CG_TestModelNextFrame_f (void) {
	cg.testModelEntity.frame++;
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelPrevFrame_f (void) {
	cg.testModelEntity.frame--;
	if ( cg.testModelEntity.frame < 0 ) {
		cg.testModelEntity.frame = 0;
	}
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelNextSkin_f (void) {
	cg.testModelEntity.skinNum++;
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

void CG_TestModelPrevSkin_f (void) {
	cg.testModelEntity.skinNum--;
	if ( cg.testModelEntity.skinNum < 0 ) {
		cg.testModelEntity.skinNum = 0;
	}
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

static void CG_AddTestModel (void) {
	int		i;

	// re-register the model, because the level may have changed
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );
	if (! cg.testModelEntity.hModel ) {
		CG_Printf ("Can't register model\n");
		return;
	}

	// if testing a gun, set the origin reletive to the view origin
	if ( cg.testGun ) {
		VectorCopy( cg.refdef.vieworg, cg.testModelEntity.origin );
		VectorCopy( cg.refdef.viewaxis[0], cg.testModelEntity.axis[0] );
		VectorCopy( cg.refdef.viewaxis[1], cg.testModelEntity.axis[1] );
		VectorCopy( cg.refdef.viewaxis[2], cg.testModelEntity.axis[2] );

		// allow the position to be adjusted
		for (i=0 ; i<3 ; i++) {
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[0][i] * cg_gun_x.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[1][i] * cg_gun_y.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[2][i] * cg_gun_z.value;
		}
	}

	trap_R_AddRefEntityToScene( &cg.testModelEntity );
}



//============================================================================


/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/
static void CG_CalcVrect (void) {
	int		size;

	// the intermission should allways be full screen
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		size = 100;
	} else {
		// bound normal viewsize
		if (cg_viewsize.integer < 30) {
			trap_Cvar_Set ("cg_viewsize","30");
			size = 30;
		} else if (cg_viewsize.integer > 100) {
			trap_Cvar_Set ("cg_viewsize","100");
			size = 100;
		} else {
			size = cg_viewsize.integer;
		}

	}
	cg.refdef.width = cgs.glconfig.vidWidth*size/100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight*size/100;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)/2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)/2;
}

//==============================================================================

//==============================================================================
//==============================================================================
// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset( void ) {
	//mme
	// smooth out stair climbing
	float timeDelta = (cg.time - cg.playerCent->pe.stepTime) + cg.timeFraction;
	if ( timeDelta < STEP_TIME ) {
		cg.refdef.vieworg[2] -= cg.playerCent->pe.stepChange 
			* (STEP_TIME - timeDelta) / STEP_TIME;
	}
}

#define CAMERA_DAMP_INTERVAL	50
#define CAMERA_MIN_FPS			1

static vec3_t	cameramins = { -CAMERA_SIZE, -CAMERA_SIZE, -CAMERA_SIZE };
static vec3_t	cameramaxs = { CAMERA_SIZE, CAMERA_SIZE, CAMERA_SIZE };
vec3_t	camerafwd, cameraup;

vec3_t	cameraFocusAngles,			cameraFocusLoc;
vec3_t	cameraIdealTarget,			cameraIdealLoc;
vec3_t	cameraCurTarget={0,0,0},	cameraCurLoc={0,0,0};
vec3_t	cameraOldLoc={0,0,0},		cameraNewLoc={0,0,0};
int		cameraLastTime=0;
float	cameraLastTimeFrac=0.0f;

float	cameraLastYaw=0;
float	cameraStiffFactor=0.0f;

/*
===============
Notes on the camera viewpoint in and out...

cg.refdef.vieworg
--at the start of the function holds the player actor's origin (center of player model).
--it is set to the final view location of the camera at the end of the camera code.
cg.refdefViewAngles
--at the start holds the client's view angles
--it is set to the final view angle of the camera at the end of the camera code.

===============
*/

/*
===============
CG_CalcTargetThirdPersonViewLocation

===============
*/
static void CG_CalcIdealThirdPersonViewTarget(void)
{
	float thirdPersonVertOffset = cg_thirdPersonVertOffset.value;

	if (cg.playerPredicted && cg.snap && cg.snap->ps.usingATST) {
		thirdPersonVertOffset = 200;
	}

	// Initialize IdealTarget
	if (gCGHasFallVector) {
		VectorCopy(gCGFallVector, cameraFocusLoc);
	} else {
		VectorCopy(cg.refdef.vieworg, cameraFocusLoc);
	}

	// Add in the new viewheight
	if (cam_specEnt.integer != -1 && cam_specEnt.integer != cg.snap->ps.clientNum) {
		cameraFocusLoc[2] += cg_entities[cam_specEnt.integer].pe.viewHeight;
		cameraFocusLoc[2] += DEFAULT_VIEWHEIGHT;
	}
	else if (cg.playerPredicted) {
		cameraFocusLoc[2] += cg.snap->ps.viewheight;
	} else {
		cameraFocusLoc[2] += cg.playerCent->pe.viewHeight;
		if (mme_chaseViewHeightFix.integer) {
			cameraFocusLoc[2] += DEFAULT_VIEWHEIGHT; // No idea why this is necessary... don't ask me.
		}
	}

	// Add in a vertical offset from the viewpoint, which puts the actual target above the head, regardless of angle.
//	VectorMA(cameraFocusLoc, thirdPersonVertOffset, cameraup, cameraIdealTarget);
	
	// Add in a vertical offset from the viewpoint, which puts the actual target above the head, regardless of angle.
	VectorCopy( cameraFocusLoc, cameraIdealTarget );
	cameraIdealTarget[2] += cg_thirdPersonVertOffset.value;
	//VectorMA(cameraFocusLoc, cg_thirdPersonVertOffset.value, cameraup, cameraIdealTarget);
}

	

/*
===============
CG_CalcTargetThirdPersonViewLocation

===============
*/
static void CG_CalcIdealThirdPersonViewLocation(void)
{
	float thirdPersonRange = cg_thirdPersonRange.value;

	if (cg.snap && cg.snap->ps.usingATST)
	{
		thirdPersonRange = 300;
	}

	VectorMA(cameraIdealTarget, -(thirdPersonRange), camerafwd, cameraIdealLoc);
}



static void CG_ResetThirdPersonViewDamp(void)
{
	trace_t trace;

	// Cap the pitch within reasonable limits
	/*if (cameraFocusAngles[PITCH] > 89.0)
	{
		cameraFocusAngles[PITCH] = 89.0;
	}
	else if (cameraFocusAngles[PITCH] < -89.0)
	{
		cameraFocusAngles[PITCH] = -89.0;
	}*/

	AngleVectors(cameraFocusAngles, camerafwd, NULL, cameraup);

	// Set the cameraIdealTarget
	CG_CalcIdealThirdPersonViewTarget();

	// Set the cameraIdealLoc
	CG_CalcIdealThirdPersonViewLocation();

	// Now, we just set everything to the new positions.
	VectorCopy(cameraIdealLoc, cameraCurLoc);
	VectorCopy(cameraIdealTarget, cameraCurTarget);

	// First thing we do is trace from the first person viewpoint out to the new target location.
	CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurTarget, cg.playerCent->currentState.number, MASK_CAMERACLIP);
	if (trace.fraction <= 1.0)
	{
		VectorCopy(trace.endpos, cameraCurTarget);
	}

	// Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	CG_Trace(&trace, cameraCurTarget, cameramins, cameramaxs, cameraCurLoc, cg.playerCent->currentState.number, MASK_CAMERACLIP);
	if (trace.fraction <= 1.0)
	{
		VectorCopy(trace.endpos, cameraCurLoc);
	}

	cameraLastTime = cg.predictedPlayerState.commandTime;
	cameraLastTimeFrac = cg.predictedTimeFrac;
	cameraLastYaw = cameraFocusAngles[YAW];
	cameraStiffFactor = 0.0f;
}

// This is called every frame.
static void CG_UpdateThirdPersonTargetDamp(void)
{
	trace_t trace;
	vec3_t	targetdiff;
	vec3_t	oldDelta;
	vec3_t	idealDelta;
	float	dampfactor, dtime, ratio;

	VectorSubtract(cameraCurTarget, cameraIdealTarget, oldDelta);
	VectorCopy(cameraIdealTarget, idealDelta);

	// Set the cameraIdealTarget
	// Automatically get the ideal target, to avoid jittering.
	CG_CalcIdealThirdPersonViewTarget();

	VectorSubtract(cameraIdealTarget, idealDelta, idealDelta);

	if (cg_thirdPersonTargetDamp.value>=1.0)
	{	// No damping.
		VectorCopy(cameraIdealTarget, cameraCurTarget);
	}
	else if (cg_thirdPersonTargetDamp.value>=0.0)
	{	
		dampfactor = 1.0-cg_thirdPersonTargetDamp.value;	// We must exponent the amount LEFT rather than the amount bled off

		if ( mov_camerafps.integer >= CAMERA_MIN_FPS )
		{	// FPS-independent camera damping by fau
			vec3_t	newDelta;
			vec3_t	velocity;
			vec3_t	shift;
			float	invdtime;
			float	timeadjfactor;
			float	codampfactor;
			int		simulationtime;
			float	physicstime;

			// use physics time to get a real velocity
			physicstime = cg.predictedPlayerState.commandTime - cameraLastTime;
			physicstime += cg.predictedTimeFrac - cameraLastTimeFrac;
			if (physicstime <= 0.0f)
				return;
			simulationtime = 1000 / mov_camerafps.integer;
			dtime = physicstime / simulationtime;
			invdtime = simulationtime / physicstime;
			timeadjfactor = powf(dampfactor, dtime);
			// velocity is in units / simulated frame time
			VectorScale(idealDelta, invdtime, velocity);
			// shift = velocity * dampfactor / (1 - dampfactor)
			codampfactor = dampfactor / (1.0f - dampfactor);
			VectorScale(velocity, codampfactor, shift);
			// delta(dtime) = dampfactor^dtime * (delta(0) + shift) - shift
			newDelta[0] = timeadjfactor * (oldDelta[0] + shift[0]) - shift[0];
			newDelta[1] = timeadjfactor * (oldDelta[1] + shift[1]) - shift[1];
			newDelta[2] = timeadjfactor * (oldDelta[2] + shift[2]) - shift[2];

			VectorAdd(cameraIdealTarget, newDelta, cameraCurTarget);
		}
		else
		{
			// Calculate the difference from the current position to the new one.
			VectorSubtract(cameraIdealTarget, cameraCurTarget, targetdiff);

			// Now we calculate how much of the difference we cover in the time allotted.
			// The equation is (Damp)^(time)

			// Note that since there are a finite number of "practical" delta millisecond values possible,
			// the ratio should be initialized into a chart ultimately.
			// dtime = (float)(cg.time-cameraLastFrame) * (1.0/(float)CAMERA_DAMP_INTERVAL);	// Our dampfactor is geared towards a time interval equal to "1".
			// ratio = powi(dampfactor, dtime);

			// fau - and this is how original powi really worked in mp when fps > 20
			ratio = dampfactor;

			// This value is how much distance is "left" from the ideal.
			VectorMA(cameraIdealTarget, -ratio, targetdiff, cameraCurTarget);
			/////////////////////////////////////////////////////////////////////////////////////////////////////////
		}
	}

	// Now we trace to see if the new location is cool or not.

	// First thing we do is trace from the first person viewpoint out to the new target location.
	if (cam_specEnt.integer != -1 && cam_specEnt.integer != cg.snap->ps.clientNum) {
		CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurTarget, cam_specEnt.integer, MASK_CAMERACLIP);
	}
	else {
		CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurTarget, cg.playerCent->currentState.number, MASK_CAMERACLIP);
	}
	if (trace.fraction < 1.0)
	{
		VectorCopy(trace.endpos, cameraCurTarget);
	}

	// Note that previously there was an upper limit to the number of physics traces that are done through the world
	// for the sake of camera collision, since it wasn't calced per frame.  Now it is calculated every frame.
	// This has the benefit that the camera is a lot smoother now (before it lerped between tested points),
	// however two full volume traces each frame is a bit scary to think about.
}

// This can be called every interval, at the user's discretion.
static void CG_UpdateThirdPersonCameraDamp(void)
{
	trace_t trace;
	vec3_t	locdiff;
	vec3_t	oldDelta;
	vec3_t	idealDelta;
	float dampfactor, dtime, ratio;

	VectorSubtract(cameraCurLoc, cameraIdealLoc, oldDelta);
	VectorCopy(cameraIdealLoc, idealDelta);

	// Set the cameraIdealLoc
	CG_CalcIdealThirdPersonViewLocation();
	
	VectorSubtract(cameraIdealLoc, idealDelta, idealDelta);
	
	// First thing we do is calculate the appropriate damping factor for the camera.
	dampfactor=0.0;
	if (cg_thirdPersonCameraDamp.value != 0.0)
	{
		double pitch;

		// Note that the camera pitch has already been capped off to 89.
		pitch = Q_fabs(cameraFocusAngles[PITCH]);

		// The higher the pitch, the larger the factor, so as you look up, it damps a lot less.
		pitch /= 115.0f;
		dampfactor = (1.0-cg_thirdPersonCameraDamp.value)*(pitch*pitch);

		dampfactor += cg_thirdPersonCameraDamp.value;

		// Now we also multiply in the stiff factor, so that faster yaw changes are stiffer.
		if (cameraStiffFactor > 0.0f)
		{	// The cameraStiffFactor is how much of the remaining damp below 1 should be shaved off, i.e. approach 1 as stiffening increases.
			dampfactor += (1.0-dampfactor)*cameraStiffFactor;
		}
	}

	if (dampfactor>=1.0)
	{	// No damping.
		VectorCopy(cameraIdealLoc, cameraCurLoc);
	}
	else if (dampfactor>=0.0)
	{	
		dampfactor = 1.0-dampfactor;	// We must exponent the amount LEFT rather than the amount bled off

		if ( mov_camerafps.integer >= CAMERA_MIN_FPS )
		{	// FPS-independent camera damping by fau
			vec3_t	newDelta;
			vec3_t	velocity;
			vec3_t	shift;
			float	invdtime;
			float	timeadjfactor;
			float	codampfactor;
			float		simulationtime;
			float	physicstime;

			// use physics time to get a real velocity
			physicstime = cg.predictedPlayerState.commandTime - cameraLastTime;
			physicstime += cg.predictedTimeFrac - cameraLastTimeFrac;
			if (physicstime <= 0.0f)
				return;
			simulationtime = 1000.0f / mov_camerafps.value;
			dtime = physicstime / simulationtime;
			invdtime = simulationtime / physicstime;
			timeadjfactor = powf(dampfactor, dtime);
			// velocity is in units / simulated frame time
			VectorScale(idealDelta, invdtime, velocity);
			// shift = velocity * dampfactor / (1 - dampfactor)
			codampfactor = dampfactor / (1.0f - dampfactor);
			VectorScale(velocity, codampfactor, shift);
			// delta(dtime) = dampfactor^dtime * (delta(0) + shift) - shift
			newDelta[0] = timeadjfactor * (oldDelta[0] + shift[0]) - shift[0];
			newDelta[1] = timeadjfactor * (oldDelta[1] + shift[1]) - shift[1];
			newDelta[2] = timeadjfactor * (oldDelta[2] + shift[2]) - shift[2];

			VectorAdd(cameraIdealLoc, newDelta, cameraCurLoc);
		}
		else
		{
			// Calculate the difference from the current position to the new one.
			VectorSubtract(cameraIdealLoc, cameraCurLoc, locdiff);

			// Now we calculate how much of the difference we cover in the time allotted.
			// The equation is (Damp)^(time)
			// dtime = (float)(cg.time-cameraLastFrame) * (1.0/(float)CAMERA_DAMP_INTERVAL);	// Our dampfactor is geared towards a time interval equal to "1".

			// Note that since there are a finite number of "practical" delta millisecond values possible,
			// the ratio should be initialized into a chart ultimately.
			// ratio = powi(dampfactor, dtime);

			// fau - and this is how original powi really worked in mp when fps > 20
			ratio = dampfactor;

			// This value is how much distance is "left" from the ideal.
			VectorMA(cameraIdealLoc, -ratio, locdiff, cameraCurLoc);
			/////////////////////////////////////////////////////////////////////////////////////////////////////////
		}
	}

	// Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	CG_Trace(&trace, cameraCurTarget, cameramins, cameramaxs, cameraCurLoc, cg.playerCent->currentState.number, MASK_CAMERACLIP);

	if (trace.fraction < 1.0)
	{
		VectorCopy( trace.endpos, cameraCurLoc );

		//FIXME: when the trace hits movers, it gets very very jaggy... ?
		/*
		//this doesn't actually help any
		if ( trace.entityNum != ENTITYNUM_WORLD )
		{
			centity_t *cent = &cg_entities[trace.entityNum];
			gentity_t *gent = &g_entities[trace.entityNum];
			if ( cent != NULL && gent != NULL )
			{
				if ( cent->currentState.pos.trType == TR_LINEAR || cent->currentState.pos.trType == TR_LINEAR_STOP )
				{
					vec3_t	diff;
					VectorSubtract( cent->lerpOrigin, gent->currentOrigin, diff );
					VectorAdd( cameraCurLoc, diff, cameraCurLoc );
				}
			}
		}
		*/
	}

	// Note that previously there was an upper limit to the number of physics traces that are done through the world
	// for the sake of camera collision, since it wasn't calced per frame.  Now it is calculated every frame.
	// This has the benefit that the camera is a lot smoother now (before it lerped between tested points),
	// however two full volume traces each frame is a bit scary to think about.
}




/*
===============`
CG_OffsetThirdPersonView

===============
*/
extern vmCvar_t cg_thirdPersonHorzOffset;
static void CG_OffsetThirdPersonView( void ) 
{
	vec3_t diff;
	float thirdPersonHorzOffset = cg_thirdPersonHorzOffset.value;
	float deltayaw;
	float dtime;

	cameraStiffFactor = 0.0;

	// Set camera viewing direction.
	VectorCopy( cg.refdefViewAngles, cameraFocusAngles );

	// if dead, look at killer
	if (cam_specEnt.integer != -1 && cam_specEnt.integer != cg.snap->ps.clientNum) {
		 if (cg_entities[cam_specEnt.integer].currentState.eFlags & EF_DEAD) {
			cameraFocusAngles[YAW] = 0;
			cameraFocusAngles[ROLL] = 0;
		}
		else { // Add in the third Person Angle.
			cameraFocusAngles[YAW] += cg_thirdPersonAngle.value;
			cameraFocusAngles[PITCH] += cg_thirdPersonPitchOffset.value;
		}
	}
	else {
		if (cg.snap->ps.stats[STAT_HEALTH] <= 0 && cg.playerPredicted) {
			cameraFocusAngles[YAW] = cg.snap->ps.stats[STAT_DEAD_YAW];
		}
		else if (cg.playerCent->currentState.eFlags & EF_DEAD) {
			cameraFocusAngles[YAW] = 0;
			cameraFocusAngles[ROLL] = 0;
		}
		else { // Add in the third Person Angle.
			cameraFocusAngles[YAW] += cg_thirdPersonAngle.value;
			cameraFocusAngles[PITCH] += cg_thirdPersonPitchOffset.value;
		}
	}
	

	// The next thing to do is to see if we need to calculate a new camera target location.
	dtime = cg.predictedPlayerState.commandTime - cameraLastTime;
	dtime += cg.predictedTimeFrac - cameraLastTimeFrac;

	// If we went back in time for some reason, or if we just started, reset the sample.
	if (cameraLastTime == 0 || dtime < 0.0f)
	{
		CG_ResetThirdPersonViewDamp();
	}
	else
	{
		// Cap the pitch within reasonable limits
		if (cameraFocusAngles[PITCH] > 80.0)
		{
			cameraFocusAngles[PITCH] = 80.0;
		}
		else if (cameraFocusAngles[PITCH] < -80.0)
		{
			cameraFocusAngles[PITCH] = -80.0;
		}

		AngleVectors(cameraFocusAngles, camerafwd, NULL, cameraup);

		deltayaw = fabs(cameraFocusAngles[YAW] - cameraLastYaw);
		if (deltayaw > 180.0f)
		{ // Normalize this angle so that it is between 0 and 180.
			deltayaw = fabs(deltayaw - 360.0f);
		}
		if (mov_camerafps.integer >= CAMERA_MIN_FPS) {
			if ( dtime > 0.0f ) {
				cameraStiffFactor = deltayaw / dtime;
			} else {
				cameraStiffFactor = 0.0f;
			}
		} else {
			cameraStiffFactor = deltayaw / (cg.time - cg.oldTime);
		}
		if (cameraStiffFactor < 1.0)
		{
			cameraStiffFactor = 0.0;
		}
		else if (cameraStiffFactor > 2.5)
		{
			cameraStiffFactor = 0.75;
		}
		else
		{	// 1 to 2 scales from 0.0 to 0.5
			cameraStiffFactor = (cameraStiffFactor-1.0f)*0.5f;
		}
		cameraLastYaw = cameraFocusAngles[YAW];

		// Move the target to the new location.
		CG_UpdateThirdPersonTargetDamp();
		CG_UpdateThirdPersonCameraDamp();
	}

	// Now interestingly, the Quake method is to calculate a target focus point above the player, and point the camera at it.
	// We won't do that for now.

	// We must now take the angle taken from the camera target and location.
	/*VectorSubtract(cameraCurTarget, cameraCurLoc, diff);
	VectorNormalize(diff);
	vectoangles(diff, cg.refdefViewAngles);*/
	VectorSubtract(cameraCurTarget, cameraCurLoc, diff);
	if (VectorLengthSquared(diff) < 0.01f * 0.01f)
	{
	    //must be hitting something, need some value to calc angles, so use cam forward
	    VectorCopy( camerafwd, diff );
	}
	vectoangles(diff, cg.refdefViewAngles);

	// Temp: just move the camera to the side a bit
	if ( thirdPersonHorzOffset != 0.0f )
	{
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
		VectorMA( cameraCurLoc, thirdPersonHorzOffset, cg.refdef.viewaxis[1], cameraCurLoc );
	}

	// ...and of course we should copy the new view location to the proper spot too.
	VectorCopy(cameraCurLoc, cg.refdef.vieworg);

	// I commented these 2 out because it fixed an issue but uhm ... idk WHY it fixed the issue...
	cameraLastTime = cg.predictedPlayerState.commandTime;
	cameraLastTimeFrac = cg.predictedTimeFrac;
}



/*
===============
CG_OffsetThirdPersonView

===============
*//*
#define	FOCUS_DISTANCE	512
static void CG_OffsetThirdPersonView( void ) {
	vec3_t		forward, right, up;
	vec3_t		view;
	vec3_t		focusAngles;
	trace_t		trace;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };
	vec3_t		focusPoint;
	float		focusDist;
	float		forwardScale, sideScale;

	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;

	VectorCopy( cg.refdefViewAngles, focusAngles );

	// if dead, look at killer
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		focusAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
		cg.refdefViewAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
	}

	if ( focusAngles[PITCH] > 45 ) {
		focusAngles[PITCH] = 45;		// don't go too far overhead
	}
	AngleVectors( focusAngles, forward, NULL, NULL );

	VectorMA( cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint );

	VectorCopy( cg.refdef.vieworg, view );

	view[2] += 8;

	cg.refdefViewAngles[PITCH] *= 0.5;

	AngleVectors( cg.refdefViewAngles, forward, right, up );

	forwardScale = cos( cg_thirdPersonAngle.value / 180 * M_PI );
	sideScale = sin( cg_thirdPersonAngle.value / 180 * M_PI );
	VectorMA( view, -cg_thirdPersonRange.value * forwardScale, forward, view );
	VectorMA( view, -cg_thirdPersonRange.value * sideScale, right, view );

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything

	if (!cg_cameraMode.integer) {
		CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_CAMERACLIP);

		if ( trace.fraction != 1.0 ) {
			VectorCopy( trace.endpos, view );
			view[2] += (1.0 - trace.fraction) * 32;
			// try another trace to this position, because a tunnel may have the ceiling
			// close enogh that this is poking out

			CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_CAMERACLIP);
			VectorCopy( trace.endpos, view );
		}
	}


	VectorCopy( view, cg.refdef.vieworg );

	// select pitch to look at focus point from vieword
	VectorSubtract( focusPoint, cg.refdef.vieworg, focusPoint );
	focusDist = sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1 ) {
		focusDist = 1;	// should never happen
	}
	cg.refdefViewAngles[PITCH] = -180 / M_PI * atan2( focusPoint[2], focusDist );
	cg.refdefViewAngles[YAW] -= cg_thirdPersonAngle.value;
}


// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset( void ) {
	int		timeDelta;
	
	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	if ( timeDelta < STEP_TIME ) {
		cg.refdef.vieworg[2] -= cg.stepChange 
			* (STEP_TIME - timeDelta) / STEP_TIME;
	}
}*/

/*
===============
CG_OffsetFirstPersonView

===============
*/
static void CG_OffsetFirstPersonView( void ) {
	float			*origin;
	float			*angles;
	float			bob;
	float			delta;
	float			speed;
	float			f;
	vec3_t			predictedVelocity;
	float			timeDelta;
	
	//mme
	centity_t		*cent = cg.playerCent;
	playerEntity_t	*pe = &cent->pe;

	if (cam_specEnt.integer != -1 && cam_specEnt.integer != cg.snap->ps.clientNum) {
		cent = &cg_entities[cam_specEnt.integer];
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}
	if ( !cent ) {
		return;
	}

	origin = cg.refdef.vieworg;
	angles = cg.refdefViewAngles;

	// if dead, fix the angle and don't add any kick
	if (cam_specEnt.integer == -1 || cam_specEnt.integer == cg.snap->ps.clientNum) {
		if (cg.snap->ps.stats[STAT_HEALTH] <= 0 && cg.playerPredicted) {
			angles[ROLL] = 40;
			angles[PITCH] = -15;
			angles[YAW] = cg.snap->ps.stats[STAT_DEAD_YAW];
			origin[2] += cg.predictedPlayerState.viewheight;
			return;
		}
	}

	// add angles based on weapon kick
	VectorAdd (angles, cg.kick_angles, angles);

	// add angles based on damage kick
	if (cam_specEnt.integer == -1 || cam_specEnt.integer == cg.snap->ps.clientNum) {
		if (cg.damageTime && cg.playerPredicted) {
			float ratio = (cg.time - cg.damageTime) + cg.timeFraction;
			if (ratio < DAMAGE_DEFLECT_TIME) {
				ratio /= DAMAGE_DEFLECT_TIME;
				angles[PITCH] += ratio * cg.v_dmg_pitch;
				angles[ROLL] += ratio * cg.v_dmg_roll;
			}
			else {
				ratio = 1.0 - (ratio - DAMAGE_DEFLECT_TIME) / DAMAGE_RETURN_TIME;
				if (ratio > 0) {
					angles[PITCH] += ratio * cg.v_dmg_pitch;
					angles[ROLL] += ratio * cg.v_dmg_roll;
				}
			}
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = ( cg.time - cg.landTime) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	angles[PITCH] += ratio * cg.fall_value;
#endif

	// add angles based on velocity
	if (cam_specEnt.integer != -1 && cam_specEnt.integer != cg.snap->ps.clientNum) 
		VectorCopy(cam.specVel, predictedVelocity);
	else if (cg.playerPredicted)
		VectorCopy( cg.predictedPlayerState.velocity, predictedVelocity );
	else //mme
		VectorCopy( cent->currentState.pos.trDelta, predictedVelocity );

	delta = DotProduct ( predictedVelocity, cg.refdef.viewaxis[0]);
	angles[PITCH] += delta * cg_runpitch.value;
	
	delta = DotProduct ( predictedVelocity, cg.refdef.viewaxis[1]);
	angles[ROLL] -= delta * cg_runroll.value;

	// add angles based on bob

	// make sure the bob is visible even at low speeds
	speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

	if (cam_specEnt.integer == -1 || cam_specEnt.integer == cg.snap->ps.clientNum) {
		if (cg.playerPredicted) {
			delta = cg.bobfracsin * cg_bobpitch.value * speed;
			if (cg.predictedPlayerState.pm_flags & PMF_DUCKED)
				delta *= 3;		// crouching
			angles[PITCH] += delta;
			delta = cg.bobfracsin * cg_bobroll.value * speed;
			if (cg.predictedPlayerState.pm_flags & PMF_DUCKED)
				delta *= 3;		// crouching accentuates roll
			if (cg.bobcycle & 1)
				delta = -delta;
			angles[ROLL] += delta;
		}
	}
//===================================

	// add view height
	//mme
	origin[2] += pe->viewHeight;

	// smooth out duck height changes
	//mme
	timeDelta = (cg.time - pe->duckTime) + cg.timeFraction;
	if ( timeDelta >= 0 && timeDelta < DUCK_TIME) {
		origin[2] -= pe->duckChange 
			* (DUCK_TIME - timeDelta) / DUCK_TIME;
	}

	// add bob height
	bob = cg.bobfracsin * cg.xyspeed * cg_bobup.value;
	if (bob > 6) {
		bob = 6;
	}
	if (cam_specEnt.integer == -1 || cam_specEnt.integer == cg.snap->ps.clientNum) {
		if (cg.playerPredicted)
			origin[2] += bob;
	}

	// add fall height
	//mme
	delta = (cg.time - pe->landTime) + cg.timeFraction;
	if ( delta < LAND_DEFLECT_TIME ) {
		f = delta / LAND_DEFLECT_TIME;
		origin[2] += pe->landChange * f;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - ( delta / LAND_RETURN_TIME );
		origin[2] += pe->landChange * f;
	}

	// add step offset
	CG_StepOffset();

	// add kick offset

	VectorAdd (origin, cg.kick_origin, origin);

	// pivot the eye based on a neck length
#if 0
	{
#define	NECK_LENGTH		8
	vec3_t			forward, up;
 
	cg.refdef.vieworg[2] -= NECK_LENGTH;
	AngleVectors( cg.refdefViewAngles, forward, NULL, up );
	VectorMA( cg.refdef.vieworg, 3, forward, cg.refdef.vieworg );
	VectorMA( cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg );
	}
#endif
}

//======================================================================

void CG_ZoomDown_f( void ) { 
	if ( cg.zoomed ) {
		return;
	}
	cg.zoomed = qtrue;
	cg.zoomTime = cg.time;
}

void CG_ZoomUp_f( void ) { 
	if ( !cg.zoomed ) {
		return;
	}
	cg.zoomed = qfalse;
	cg.zoomTime = cg.time;
}



/*
====================
CG_CalcFovFromX

Calcs Y FOV from given X FOV
====================
*/
#define	WAVE_AMPLITUDE	1
#define	WAVE_FREQUENCY	0.4

qboolean CG_CalcFOVFromX( float fov_x ) 
{
	float	x;
//	float	phase;
//	float	v;
//	int		contents;
	float	fov_y;
	qboolean	inwater;

	x = cg.refdef.width / tan( fov_x / 360 * M_PI );
	fov_y = atan2( cg.refdef.height, x );
	fov_y = fov_y * 360 / M_PI;

	// there's a problem with this, it only takes the leafbrushes into account, not the entity brushes,
	//	so if you give slime/water etc properties to a func_door area brush in order to move the whole water 
	//	level up/down this doesn't take into account the door position, so warps the view the whole time
	//	whether the water is up or not. Fortunately there's only one slime area in Trek that you can be under,
	//	so lose it...
#if 0
/*
	// warp if underwater
	contents = CG_PointContents( cg.refdef.vieworg, -1 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ){
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin( phase );
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}
	else {
		inwater = qfalse;
	}
*/
#else
	inwater = qfalse;
#endif


	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	return (inwater);
}

inline float CG_DistanceAwareFovScale(float cgFov) {
	if (cg_distanceAwareFov.value) {
		vec3_t charCameraDistance;
		VectorSubtract(cg.predictedPlayerState.origin, cg.refdef.vieworg, charCameraDistance);
		float fullyCorrectedFovDelta = cgFov * cg_thirdPersonRange.value / VectorLength(charCameraDistance) - cgFov;
		return cgFov + fullyCorrectedFovDelta * cg_distanceAwareFov.value;
	}
	else {
		return cgFov;
	}
}

/*
====================
CG_CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
float zoomFov; //this has to be global client-side

static int CG_CalcFov( void ) {
	float	x;
	float	v;
	int		contents;
	float	fov_x, fov_y;
	float	f;
	int		inwater;
	//[TrueView]
	float	cgFov;
	//float	cgFov = cg_fov.value;

	if(cg.playerPredicted && (!cg.renderingThirdPerson
		&& (cg.trueView
		|| cg.predictedPlayerState.weapon == WP_SABER) 
		&& cg_trueFOV.value && (cg.predictedPlayerState.pm_type != PM_SPECTATOR)
		&& (cg.predictedPlayerState.pm_type != PM_INTERMISSION))) {
		cgFov = cg_trueFOV.value;
	} else if(!cg.renderingThirdPerson 
		&& (cg.trueView
		|| cg.playerCent->currentState.weapon == WP_SABER) 
		&& cg_trueFOV.value && (cg.predictedPlayerState.pm_type != PM_INTERMISSION))
	{
		cgFov = cg_trueFOV.value;
	}
	else
	{
		cgFov = cg_fov.value;
	}
	//[/TrueView]

	cgFov = CG_DistanceAwareFovScale(cgFov);

	if (cgFov < 1)
		cgFov = 1;
	else if (cgFov > 180)
		cgFov = 180;

	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		// if in intermission, use a fixed value
		fov_x = 80;//90;
	} else {
		// user selectable
		if ( cgs.dmflags & DF_FIXED_FOV ) {
			// dmflag to prevent wide fov for all clients
			fov_x = 80;//90;
		} else {
			fov_x = cgFov;
			if ( fov_x < 1 ) {
				fov_x = 1;
			} else if ( fov_x > 180 ) {
				fov_x = 180;
			}
		}

		//will probably only work with base and base-based mods
		if (!cg.playerPredicted && cg.zoomMode) {
			fov_x *= 0.46f;
			goto notZoom;
		}
		if (!cg.playerPredicted) goto notZoom;

		if (cg.predictedPlayerState.zoomMode == 2)
		{ //binoculars
			if (zoomFov > 40.0f)
			{
				zoomFov -= cg.frametime * 0.075f;

				if (zoomFov < 40.0f)
				{
					zoomFov = 40.0f;
				}
				else if (zoomFov > cgFov)
				{
					zoomFov = cgFov;
				}
			}

			fov_x = zoomFov;
		}
		else if (cg.predictedPlayerState.zoomMode)
		{
			if (!cg.predictedPlayerState.zoomLocked)
			{
				if (zoomFov > 50)
				{ //Now starting out at nearly half zoomed in
					zoomFov = 50;
				}
				zoomFov -= cg.frametime * 0.035f;//0.075f;

				if (zoomFov < MAX_ZOOM_FOV)
				{
					zoomFov = MAX_ZOOM_FOV;
				}
				else if (zoomFov > cgFov)
				{
					zoomFov = cgFov;
				}
				else
				{	// Still zooming
					static unsigned int zoomSoundTime = 0;

					if (zoomSoundTime < cg.time || zoomSoundTime > cg.time + 10000)
					{
						trap_S_StartSound(cg.refdef.vieworg, ENTITYNUM_WORLD, CHAN_LOCAL, cgs.media.disruptorZoomLoop);
						zoomSoundTime = cg.time + 300;
					}
				}
			}

			fov_x = zoomFov;
		}
		else 
		{
			zoomFov = 80;

			f = ((cg.time - cg.predictedPlayerState.zoomTime) + cg.timeFraction) / ZOOM_OUT_TIME;
			if ( f > 1.0 ) 
			{
				fov_x = fov_x;
			} 
			else 
			{
				fov_x = cg.predictedPlayerState.zoomFov + f * ( fov_x - cg.predictedPlayerState.zoomFov );
			}
		}
	}

notZoom:
	x = cg.refdef.width / tan( fov_x / 360 * M_PI );
	fov_y = atan2( cg.refdef.height, x );
	fov_y = fov_y * 360 / M_PI;

	// warp if underwater
	contents = CG_PointContents( cg.refdef.vieworg, -1 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ){
		v = WAVE_AMPLITUDE * sin((cg.time / 1000.0 + cg.timeFraction / 1000.0) * WAVE_FREQUENCY * M_PI * 2);
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}
	else {
		inwater = qfalse;
	}


	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	if (cg.predictedPlayerState.zoomMode)
	{
		cg.zoomSensitivity = zoomFov/cgFov;
	}
	else if ( !cg.zoomed ) {
		cg.zoomSensitivity = 1;
	} else {
		cg.zoomSensitivity = cg.refdef.fov_y / 75.0;
	}

	return inwater;
}


/*
===============
CG_DamageBlendBlob

===============
*/
void CG_DamageBlendBlob( void ) 
{
	float		t;
	int			maxTime;
	refEntity_t		ent;

	if ( !cg.damageValue ) {
		return;
	}

	if ( !cg.playerPredicted )
		return;

	maxTime = DAMAGE_TIME;
	t = (cg.time - cg.damageTime) + cg.timeFraction;
	if ( t <= 0 || t >= maxTime ) {
		return;
	}

	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	VectorMA( cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin );
	VectorMA( ent.origin, cg.damageX * -8, cg.refdef.viewaxis[1], ent.origin );
	VectorMA( ent.origin, cg.damageY * 8, cg.refdef.viewaxis[2], ent.origin );

	ent.radius = cg.damageValue * 3 * ( 1.0 - ((float)t / maxTime) );

	if (cg.snap->ps.damageType == 0)
	{ //pure health
		ent.customShader = cgs.media.viewPainShader;
		ent.shaderRGBA[0] = 180 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[1] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[2] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[3] = 255;
	}
	else if (cg.snap->ps.damageType == 1)
	{ //pure shields
		ent.customShader = cgs.media.viewPainShader_Shields;
		ent.shaderRGBA[0] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[1] = 180 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[2] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[3] = 255;
	}
	else
	{ //shields and health
		ent.customShader = cgs.media.viewPainShader_ShieldsAndHealth;
		ent.shaderRGBA[0] = 180 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[1] = 180 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[2] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[3] = 255;
	}
	trap_R_AddRefEntityToScene( &ent );
}

qboolean CheckOutOfConstrict(float curAng)
{
	float degrees_negative, degrees_positive;

	float angle_ideal = cg.constrictValue;
	float angle_current = curAng;

	float angle_dif = 0;

	if (angle_current < 0)
	{
		angle_current += 360;
	}
	if (angle_current > 360)
	{
		angle_current -= 360;
	}

	if (cg.doConstrict <= cg.time)
	{
		return qfalse;
	}

	if (angle_ideal <= angle_current)
	{
		degrees_negative = (angle_current - angle_ideal);

		degrees_positive = (360 - angle_current) + angle_ideal;
	}
	else
	{
		degrees_negative = angle_current + (360 - angle_ideal);

		degrees_positive = (angle_ideal - angle_current);
	}

	if (degrees_negative < degrees_positive)
	{
		angle_dif = degrees_negative;
	}
	else
	{
		angle_dif = degrees_positive;
	}

	if (angle_dif > 60)
	{
		return qtrue;
	}

	return qfalse;
}

int CG_DemosCalcViewValues( void ) {
	entityState_t *es = &cg.playerCent->currentState;
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );

	// calculate size of 3D view
	CG_CalcVrect();

	// intermission view
	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		playerState_t *ps = &cg.predictedPlayerState;
		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
		return CG_CalcFov();
	}

	if (cam_specEnt.integer == -1 || cam_specEnt.integer == cg.snap->ps.clientNum)
	{
		if (cg.playerPredicted) {
			cg.bobcycle = (cg.predictedPlayerState.bobCycle & 128) >> 7;
			cg.bobfracsin = fabs(sin((cg.predictedPlayerState.bobCycle & 127) / 127.0 * M_PI));
		}
		else {
			cg.bobcycle = 0;
			cg.bobfracsin = 0;
		}
		cg.xyspeed = sqrt(es->pos.trDelta[0] * es->pos.trDelta[0] +
			es->pos.trDelta[1] * es->pos.trDelta[1]);

		if (cg.xyspeed > 270)
		{
			cg.xyspeed = 270;
		}

		VectorCopy(cg.playerCent->lerpOrigin, cg.refdef.vieworg);
		VectorCopy(cg.playerCent->lerpAngles, cg.refdefViewAngles);
	}
	else { // Spectating different player


		cg.xyspeed = sqrt(cam.specVel[0] * cam.specVel[0] + cam.specVel[1] * cam.specVel[1]);

		if (cg.xyspeed > 270)
		{
			cg.xyspeed = 270;
		}

		//VectorCopy(cam.specOrg, cg.refdef.vieworg);
		//VectorCopy(cam.specAng, cg.refdefViewAngles);
		VectorCopy(cg_entities[cam_specEnt.integer].lerpOrigin, cg.refdef.vieworg);
		VectorCopy(cg_entities[cam_specEnt.integer].lerpAngles, cg.refdefViewAngles);
	}

	if (cg_cameraOrbit.integer) {
		if (cg.time > cg.nextOrbitTime) {
			cg.nextOrbitTime = cg.time + cg_cameraOrbitDelay.integer;
			cg_thirdPersonAngle.value += cg_cameraOrbit.value;
		}
	}
	// add error decay
	if ( cg_errorDecay.value > 0 ) {
		int		t;
		float	f;

		t = cg.time - cg.predictedErrorTime;
		f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
		if ( f > 0 && f < 1 ) {
			VectorMA( cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg );
		} else {
			cg.predictedErrorTime = 0;
		}
	}

	if ( cg.renderingThirdPerson && !cg.zoomMode) {
		// back away from character
		CG_OffsetThirdPersonView(); // commenting this out eradicates the command smoothing issue
	} else {
		// offset for local bobbing and kicks
		CG_OffsetFirstPersonView();
	}

	// position eye reletive to origin
	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

	if ( cg.hyperspace ) {
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}

/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/
static int CG_CalcViewValues( void ) {
	playerState_t	*ps;

	memset( &cg.refdef, 0, sizeof( cg.refdef ) );

	// strings for in game rendering
	// Q_strncpyz( cg.refdef.text[0], "Park Ranger", sizeof(cg.refdef.text[0]) );
	// Q_strncpyz( cg.refdef.text[1], "19", sizeof(cg.refdef.text[1]) );

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;
/*
	if (cg.cameraMode) {
		vec3_t origin, angles;
		if (trap_getCameraInfo(cg.time, &origin, &angles)) {
			VectorCopy(origin, cg.refdef.vieworg);
			angles[ROLL] = 0;
			VectorCopy(angles, cg.refdefViewAngles);
			AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
			return CG_CalcFov();
		} else {
			cg.cameraMode = qfalse;
		}
	}
*/
	// intermission view
	if ( ps->pm_type == PM_INTERMISSION ) {
		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
		return CG_CalcFov();
	}

	if(cam_specEnt.integer == -1 || cam_specEnt.integer == cg.snap->ps.clientNum)
	{ 
		cg.bobcycle = ( ps->bobCycle & 128 ) >> 7;
		cg.bobfracsin = fabs( sin( ( ps->bobCycle & 127 ) / 127.0 * M_PI ) );
		cg.xyspeed = sqrt( ps->velocity[0] * ps->velocity[0] +
			ps->velocity[1] * ps->velocity[1] );

		if (cg.xyspeed > 270)
		{
			cg.xyspeed = 270;
		}

		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
	}
	else { // Spectating different player


		cg.xyspeed = sqrt(cam.specVel[0] * cam.specVel[0] + cam.specVel[1] * cam.specVel[1]);

		if (cg.xyspeed > 270)
		{
			cg.xyspeed = 270;
		}

		//VectorCopy(cam.specOrg, cg.refdef.vieworg);
		//VectorCopy(cam.specAng, cg.refdefViewAngles);
		VectorCopy(cg_entities[cam_specEnt.integer].lerpOrigin, cg.refdef.vieworg);
		VectorCopy(cg_entities[cam_specEnt.integer].lerpAngles, cg.refdefViewAngles);
	}

	if (cg_cameraOrbit.integer) {
		if (cg.time > cg.nextOrbitTime) {
			cg.nextOrbitTime = cg.time + cg_cameraOrbitDelay.integer;
			cg_thirdPersonAngle.value += cg_cameraOrbit.value;
		}
	}
	// add error decay
	if ( cg_errorDecay.value > 0 ) {
		int		t;
		float	f;

		t = cg.time - cg.predictedErrorTime;
		f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
		if ( f > 0 && f < 1 ) {
			VectorMA( cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg );
		} else {
			cg.predictedErrorTime = 0;
		}
	}

	if ( cg.renderingThirdPerson && !cg.snap->ps.zoomMode) {
		// back away from character
		CG_OffsetThirdPersonView();
	} else {
		// offset for local bobbing and kicks
		CG_OffsetFirstPersonView();
	}

	// position eye reletive to origin
	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

	if ( cg.hyperspace ) {
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}


/*
=====================
CG_PowerupTimerSounds
=====================
*/
void CG_PowerupTimerSounds( void ) {
	int		i;
	int		t;

	// powerup timers going away
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		t = cg.snap->ps.powerups[i];
		if ( t <= cg.time ) {
			continue;
		}
		if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			continue;
		}
		if ( ( t - cg.time ) / POWERUP_BLINK_TIME != ( t - cg.oldTime ) / POWERUP_BLINK_TIME ) {
			//trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.wearOffSound );
		}
	}
}

/*
=====================
CG_AddBufferedSound
=====================
*/
void CG_AddBufferedSound( sfxHandle_t sfx ) {
	if ( !sfx )
		return;
	cg.soundBuffer[cg.soundBufferIn] = sfx;
	cg.soundBufferIn = (cg.soundBufferIn + 1) % MAX_SOUNDBUFFER;
	if (cg.soundBufferIn == cg.soundBufferOut) {
		cg.soundBufferOut++;
	}
}

/*
=====================
CG_PlayBufferedSounds
=====================
*/
void CG_PlayBufferedSounds( void ) {
	if ( cg.soundTime < cg.time ) {
		if (cg.soundBufferOut != cg.soundBufferIn && cg.soundBuffer[cg.soundBufferOut]) {
			trap_S_StartLocalSound(cg.soundBuffer[cg.soundBufferOut], CHAN_ANNOUNCER);
			cg.soundBuffer[cg.soundBufferOut] = 0;
			cg.soundBufferOut = (cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
			cg.soundTime = cg.time + 750;
		}
	}
}

void CG_UpdateSoundTrackers()
{
	int num;
	centity_t *cent;

	for ( num = 0 ; num < ENTITYNUM_NONE ; num++ ) {
		cent = &cg_entities[num];

		if (cent && (cent->currentState.eFlags & EF_SOUNDTRACKER) && cent->currentState.number == num)
		{ //keep sound for this entity updated in accordance with its attached entity at all times
			if (cg.playerCent && cent->currentState.trickedentindex == cg.playerCent->currentState.number)
			{ //this is actually the player, so center the sound origin right on top of us
				VectorCopy(cg.refdef.vieworg, cent->lerpOrigin);
				trap_S_UpdateEntityPosition( cent->currentState.number, cent->lerpOrigin );
			}
			else if (cg.snap && !cg.playerCent && cent->currentState.trickedentindex == cg.snap->ps.clientNum)
			{
				VectorCopy(cg.snap->ps.origin, cent->lerpOrigin);
				trap_S_UpdateEntityPosition( cent->currentState.number, cent->lerpOrigin );
			}
			else
			{
				trap_S_UpdateEntityPosition( cent->currentState.number, cg_entities[cent->currentState.trickedentindex].lerpOrigin );
			}
		}
	}
}

//=========================================================================

/*
================================
Screen Effect stuff starts here
================================
*/
#define	CAMERA_DEFAULT_FOV			90.0f
#define MAX_SHAKE_INTENSITY			16.0f

cgscreffects_t cgScreenEffects;

void CG_SE_UpdateShake( vec3_t origin, vec3_t angles )
{
	vec3_t	moveDir;
	float	intensity_scale, intensity;
	int		i;

	if ( cgScreenEffects.shake_duration <= 0 )
		return;

	if ( cg.time > ( cgScreenEffects.shake_start + cgScreenEffects.shake_duration ) )
	{
		cgScreenEffects.shake_intensity = 0;
		cgScreenEffects.shake_duration = 0;
		cgScreenEffects.shake_start = 0;
		return;
	}

	cgScreenEffects.FOV = CAMERA_DEFAULT_FOV;
	cgScreenEffects.FOV2 = CAMERA_DEFAULT_FOV;

	//intensity_scale now also takes into account FOV with 90.0 as normal
	intensity_scale = 1.0f - ( (float) ( cg.time - cgScreenEffects.shake_start ) / (float) cgScreenEffects.shake_duration ) * (((cgScreenEffects.FOV+cgScreenEffects.FOV2)/2.0f)/90.0f);

	intensity = cgScreenEffects.shake_intensity * intensity_scale;

	for ( i = 0; i < 3; i++ )
	{
		moveDir[i] = ( crandom() * intensity );
	}

	//Move the camera
	VectorAdd( origin, moveDir, origin );

	for ( i=0; i < 2; i++ ) // Don't do ROLL
		moveDir[i] = ( crandom() * intensity );

	//Move the angles
	VectorAdd( angles, moveDir, angles );
}

void CG_SE_UpdateMusic(void)
{
	if (cgScreenEffects.music_volume_multiplier < 0.1)
	{
		cgScreenEffects.music_volume_multiplier = 1.0;
		return;
	}

	if (cgScreenEffects.music_volume_time < cg.time)
	{
		if (cgScreenEffects.music_volume_multiplier != 1.0 || cgScreenEffects.music_volume_set)
		{
			char musMultStr[512];

			cgScreenEffects.music_volume_multiplier += 0.1;
			if (cgScreenEffects.music_volume_multiplier > 1.0)
			{
				cgScreenEffects.music_volume_multiplier = 1.0;
			}

			Com_sprintf(musMultStr, sizeof(musMultStr), "%f", cgScreenEffects.music_volume_multiplier);
			trap_Cvar_Set("s_musicMult", musMultStr);

			if (cgScreenEffects.music_volume_multiplier == 1.0)
			{
				cgScreenEffects.music_volume_set = qfalse;
			}
			else
			{
				cgScreenEffects.music_volume_time = cg.time + 200;
			}
		}

		return;
	}

	if (!cgScreenEffects.music_volume_set)
	{ //if the volume_time is >= cg.time, we should have a volume multiplier set
		char musMultStr[512];

		Com_sprintf(musMultStr, sizeof(musMultStr), "%f", cgScreenEffects.music_volume_multiplier);
		trap_Cvar_Set("s_musicMult", musMultStr);
		cgScreenEffects.music_volume_set = qtrue;
	}
}

/*
=================
CG_CalcScreenEffects

Currently just for screen shaking (and music volume management)
=================
*/
void CG_CalcScreenEffects(void) {
	if (fx_Vibrate.integer == 0)
		CG_SE_UpdateShake(cg.refdef.vieworg, cg.refdefViewAngles);
	CG_SE_UpdateMusic();
}

void CGCam_Shake( float intensity, int duration ) {
	if ( intensity > MAX_SHAKE_INTENSITY )
		intensity = MAX_SHAKE_INTENSITY;

	cgScreenEffects.shake_intensity = intensity;
	cgScreenEffects.shake_duration = duration;
	cgScreenEffects.shake_start = cg.time;
}

void CGCam_SetMusicMult( float multiplier, int duration )
{
	if (multiplier < 0.1f)
	{
		multiplier = 0.1f;
	}

	if (multiplier > 1.0f)
	{
		multiplier = 1.0f;
	}

	cgScreenEffects.music_volume_multiplier = multiplier;
	cgScreenEffects.music_volume_time = cg.time + duration;
	cgScreenEffects.music_volume_set = qfalse;
}

/*
================================
Screen Effect stuff ends here
================================
*/

/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/
extern void CG_SetPredictedThirdPerson(void);
extern void trap_S_UpdateScale( float scale );
void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, int demoPlayback ) {
	int		inwater;

	cg.time = serverTime;
	cg.demoPlayback = demoPlayback;
	
	if (cg.snap && ui_myteam.integer != cg.snap->ps.persistant[PERS_TEAM])
	{
		trap_Cvar_Set ( "ui_myteam", va("%i", cg.snap->ps.persistant[PERS_TEAM]) );
	}

	// update cvars
	CG_UpdateCvars();

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if ( cg.infoScreenText[0] != 0 ) {
		CG_DrawInformation();
		return;
	}

	trap_FX_AdjustTime( cg.time, cg.frametime, 0, cg.refdef.vieworg, cg.refdef.viewaxis );

	CG_RunLightStyles();

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds(qfalse);

	// clear all the render lists
	trap_R_ClearScene();

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();

	trap_ROFF_UpdateEntities();

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if ( !cg.snap || ( cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
		CG_DrawInformation();
		return;
	}

	// let the client system know what our weapon and zoom settings are
	if (cg.snap && cg.snap->ps.saberLockTime > cg.time)
	{
		trap_SetUserCmdValue( cg.weaponSelect, 0.01, cg.forceSelect, cg.itemSelect );
	}
	else if (cg.snap && cg.snap->ps.usingATST)
	{
		trap_SetUserCmdValue( cg.weaponSelect, 0.2, cg.forceSelect, cg.itemSelect );
	}
	else
	{
		trap_SetUserCmdValue( cg.weaponSelect, cg.zoomSensitivity, cg.forceSelect, cg.itemSelect );
	}

	CG_PreparePacketEntities( );

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;
	//mme
	cg.playerCent = &cg_entities[cg.predictedPlayerState.clientNum];
	cg.playerPredicted = qtrue;

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

	// generate and add the entity from the playerstate
	CG_CheckPlayerG2Weapons(&cg.predictedPlayerState, &cg_entities[cg.predictedPlayerState.clientNum]);
	BG_PlayerStateToEntityState(&cg.predictedPlayerState, &cg_entities[cg.snap->ps.clientNum].currentState, qfalse);
	cg_entities[cg.snap->ps.clientNum].currentValid = qtrue;
	VectorCopy( cg_entities[cg.snap->ps.clientNum].currentState.pos.trBase, cg_entities[cg.snap->ps.clientNum].lerpOrigin );
	VectorCopy( cg_entities[cg.snap->ps.clientNum].currentState.apos.trBase, cg_entities[cg.snap->ps.clientNum].lerpAngles );

	// decide on third person view
	cg.trueView = (cg.playerCent->currentState.weapon == WP_SABER && cg_trueSaber.integer)
		|| (cg.playerCent->currentState.weapon != WP_SABER && cg_trueGuns.integer);
	cg.zoomMode = cg.snap->ps.zoomMode || cg.predictedPlayerState.zoomMode;
	CG_SetPredictedThirdPerson();

	// update speedometer
	CG_AddSpeed();

	// build cg.refdef
	inwater = CG_CalcViewValues();
	
	cg.fallingToDeath = cg.snap->ps.fallingToDeath;

	CG_CalcScreenEffects();

	// first person blend blobs, done after AnglesToAxis
	if ( !cg.renderingThirdPerson ) {
		CG_DamageBlendBlob();
	}

	// build the render lists
	if ( !cg.hyperspace ) {
		CG_AddPacketEntities();			// adter calcViewValues, so predicted player state is correct
		CG_AddMarks();
		CG_AddParticles ();
		CG_AddLocalEntities();
	}
	CG_AddViewWeapon( &cg.predictedPlayerState );
	Cam_Draw3d(); //3D

	if ( !cg.hyperspace) {
		trap_FX_AddScheduledEffects();
	}

	// add buffered sounds
	CG_PlayBufferedSounds();

	// play buffered voice chats
	CG_PlayBufferedVoiceChats();

	// finish up the rest of the refdef
	if ( cg.testModelEntity.hModel ) {
		CG_AddTestModel();
	}
	cg.refdef.time = cg.time;
	memcpy( cg.refdef.areamask, cg.snap->areamask, sizeof( cg.refdef.areamask ) );

	// warning sounds when powerup is wearing off
	CG_PowerupTimerSounds();
	// if there are any entities flagged as sound trackers and attached to other entities, update their sound pos
	CG_UpdateSoundTrackers();
 
	if (gCGHasFallVector) {
		vec3_t lookAng;
		VectorSubtract(cg.playerCent->lerpOrigin, cg.refdef.vieworg, lookAng);
		VectorNormalize(lookAng);
		vectoangles(lookAng, lookAng);
		VectorCopy(gCGFallVector, cg.refdef.vieworg);
		AnglesToAxis(lookAng, cg.refdef.viewaxis);
	}

	trap_S_UpdateScale( 1.0f );
	// update audio positions
	trap_S_Respatialize( cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater );

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	if ( stereoView != STEREO_RIGHT ) {
		cg.frametime = cg.time - cg.oldTime;
		if ( cg.frametime < 0 ) {
			cg.frametime = 0;
		}
		cg.oldTime = cg.time;
		CG_AddLagometerFrameInfo();
	}
	if (cg_timescale.value != cg_timescaleFadeEnd.value) {
		if (cg_timescale.value < cg_timescaleFadeEnd.value) {
			cg_timescale.value += cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (cg_timescale.value > cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		else {
			cg_timescale.value -= cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (cg_timescale.value < cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		if (cg_timescaleFadeSpeed.value) {
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value));
		}
	}

	// actually issue the rendering calls
	CG_DrawActive( stereoView );

	Cam_Draw2d(); //2D

	if ( cg_stats.integer ) {
		CG_Printf( "cg.clientFrame:%i\n", cg.clientFrame );
	}
}

//[TrueView]
//Checks to see if the current camera position is valid based on the last known safe location.  If it's not safe, place
//the camera at the last position safe location
void CheckCameraLocation( vec3_t OldeyeOrigin ) {
	trace_t			trace;
	refdef_t		*refdef = &cg.refdef;//CG_GetRefdef();

	CG_Trace(&trace, OldeyeOrigin, cameramins, cameramaxs, refdef->vieworg, cg.playerCent->currentState.number, GetCameraClip());
	if (trace.fraction <= 1.0) {
		VectorCopy(trace.endpos, refdef->vieworg);
	}
}
//[/TrueView]

#ifdef RELDEBUG
//#pragma optimize("", on)
#endif