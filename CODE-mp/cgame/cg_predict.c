// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_predict.c -- this file generates cg.predictedPlayerState by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement.
// It also handles local physics interaction, like fragments bouncing off walls

#include "cg_local.h"

static	pmove_t		cg_pmove;

static	int			cg_numSolidEntities;
static	centity_t	*cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT];
static	int			cg_numTriggerEntities;
static	centity_t	*cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];

/*
====================
CG_BuildSolidList

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection
====================
*/
void CG_BuildSolidList( void ) {
	int			i;
	centity_t	*cent;
	snapshot_t	*snap;
	entityState_t	*ent;

	cg_numSolidEntities = 0;
	cg_numTriggerEntities = 0;

	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
		snap = cg.nextSnap;
	} else {
		snap = cg.snap;
	}

	for ( i = 0 ; i < snap->numEntities ; i++ ) {
		cent = &cg_entities[ snap->entities[ i ].number ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM || ent->eType == ET_PUSH_TRIGGER || ent->eType == ET_TELEPORT_TRIGGER ) {
			cg_triggerEntities[cg_numTriggerEntities] = cent;
			cg_numTriggerEntities++;
			continue;
		}

		if ( cent->nextState.solid ) {
			cg_solidEntities[cg_numSolidEntities] = cent;
			cg_numSolidEntities++;
			continue;
		}
	}

	//ent from JA:
	//rww - Horrible, terrible, awful hack.
	//We don't send your client entity from the server,
	//so it isn't added into the solid list from the snapshot,
	//and in addition, it has no solid data. So we will force
	//adding it in based on a hardcoded player bbox size.
	//This will cause issues if the player box size is ever
	//changed..
	if (cg_numSolidEntities < MAX_ENTITIES_IN_SNAPSHOT)
	{
		vec3_t	playerMins = {-15, -15, DEFAULT_MINS_2};
		vec3_t	playerMaxs = {15, 15, DEFAULT_MAXS_2};
		int i, j, k;

		i = playerMaxs[0];
		if (i<1)
			i = 1;
		if (i>255)
			i = 255;

		// z is not symetric
		j = (-playerMins[2]);
		if (j<1)
			j = 1;
		if (j>255)
			j = 255;

		// and z playerMaxs can be negative...
		k = (playerMaxs[2]+32);
		if (k<1)
			k = 1;
		if (k>255)
			k = 255;

		cg_solidEntities[cg_numSolidEntities] = &cg_entities[cg.predictedPlayerState.clientNum];
		cg_solidEntities[cg_numSolidEntities]->currentState.solid = (k<<16) | (j<<8) | i;

		cg_numSolidEntities++;
	}

}

/*
====================
CG_ClipMoveToEntities

====================
*/
static void CG_ClipMoveToEntities ( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
							int skipNumber, int mask, trace_t *tr ) {
	int			i, x, zd, zu;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t 	cmodel;
	vec3_t		bmins, bmaxs;
	vec3_t		origin, angles;
	centity_t	*cent;

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];
		ent = &cent->currentState;

		if ( ent->number == skipNumber ) {
			continue;
		}

		if (ent->number > MAX_CLIENTS && cg.snap && ent->genericenemyindex && (ent->genericenemyindex-1024) == cg.snap->ps.clientNum)
		{ //rww - method of keeping objects from colliding in client-prediction (in case of ownership)
			continue;
		}

		if ( ent->solid == SOLID_BMODEL ) {
			// special value for bmodel
			cmodel = trap_CM_InlineModel( ent->modelindex );
			VectorCopy( cent->lerpAngles, angles );
			BG_EvaluateTrajectory( &cent->currentState.pos, cg.physicsTime, origin );
		} else {
			// encoded bbox
			x = (ent->solid & 255);
			zd = ((ent->solid>>8) & 255);
			zu = ((ent->solid>>16) & 255) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			cmodel = trap_CM_TempBoxModel( bmins, bmaxs );
			VectorCopy( vec3_origin, angles );
			VectorCopy( cent->lerpOrigin, origin );
		}


		trap_CM_TransformedBoxTrace ( &trace, start, end,
			mins, maxs, cmodel,  mask, origin, angles);

		if (trace.allsolid || trace.fraction < tr->fraction) {
			trace.entityNum = ent->number;
			*tr = trace;
		} else if (trace.startsolid) {
			tr->startsolid = qtrue;
		}
		if ( tr->allsolid ) {
			return;
		}
	}
}

/*
================
CG_Trace
================
*/
void	CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, 
					 int skipNumber, int mask ) {
	trace_t	t;

	trap_CM_BoxTrace ( &t, start, end, mins, maxs, 0, mask);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities (start, mins, maxs, end, skipNumber, mask, &t);

	*result = t;
}

/*
================
CG_PointContents
================
*/
int		CG_PointContents( const vec3_t point, int passEntityNum ) {
	int			i;
	entityState_t	*ent;
	centity_t	*cent;
	clipHandle_t cmodel;
	int			contents;

	contents = trap_CM_PointContents (point, 0);

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];

		ent = &cent->currentState;

		if ( ent->number == passEntityNum ) {
			continue;
		}

		if (ent->solid != SOLID_BMODEL) { // special value for bmodel
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		contents |= trap_CM_TransformedPointContents( point, cmodel, ent->origin, ent->angles );
	}

	return contents;
}

void CG_ComputeCommandSmoothPlayerstates(timedPlayerState_t** currentState, timedPlayerState_t** nextState, qboolean* isTeleport) {
	psHistory_t* hist = &cg.psHistory;
	timedPlayerState_t* tps = &hist->states[cg.currentPsHistory % MAX_STATE_HISTORY];
	timedPlayerState_t* nexttps = &hist->states[(cg.currentPsHistory + 1) % MAX_STATE_HISTORY];
	int nextTpsOffset = 1;
	// advance as needed
	while (nexttps->time - cg.time <= cg.timeFraction && cg.currentPsHistory + 1 < hist->nextSlot) {
		cg.currentPsHistory++;
		tps = &hist->states[cg.currentPsHistory % MAX_STATE_HISTORY];
		nexttps = &hist->states[(cg.currentPsHistory + 1) % MAX_STATE_HISTORY];
	}
	// curEsh should now be right.  nextEsh may be, unless player went a whole frame with no userCmd
	// in that case we probably want to skip it - note we still need to keep the thing around in case it has an event
	// otherwise we will miss that event
	*currentState = tps;
	qboolean nextIsTeleport = nexttps->isTeleport;
	while (tps->time >= nexttps->time && cg.currentPsHistory + nextTpsOffset < hist->nextSlot) {
		nextTpsOffset++;
		nexttps = &hist->states[(cg.currentPsHistory + nextTpsOffset) % MAX_STATE_HISTORY];
		nextIsTeleport |= nexttps->isTeleport;
	}
	*isTeleport = nextIsTeleport;
	if (cg.currentPsHistory + nextTpsOffset == hist->nextSlot) {
		// couldn't find a valid nexttps
#ifdef _DEBUG
		Com_Printf("couldn't find valid nexttps\n");
#endif
		* nextState = NULL;
		return;
	}
	*nextState = nexttps;
	return;
}

/*
========================
CG_InterpolatePlayerState

Generates cg.predictedPlayerState by interpolating between
cg.snap->player_state and cg.nextFrame->player_state
========================
*/
void CG_InterpolatePlayerState( qboolean grabAngles ) {
	float			f;
	int				i;
	playerState_t	*out;
	snapshot_t		*prev, *next;
	playerState_t* curps = NULL, * nextps = NULL;
	qboolean		nextPsTeleport = qfalse;
	int currentTime = 0, nextTime = 0, currentServerTime = 0, nextServerTime = 0;

	out = &cg.predictedPlayerState;
	prev = cg.snap;
	next = cg.nextSnap;

	if (cg_commandSmooth.integer) {
		timedPlayerState_t* tps, * nexttps;
		CG_ComputeCommandSmoothPlayerstates(&tps, &nexttps, &nextPsTeleport);
		curps = &tps->ps;
		currentTime = tps->time;
		currentServerTime = tps->serverTime;
		if (nexttps) {
			nextps = &nexttps->ps;
			nextTime = nexttps->time;
			nextServerTime = nexttps->serverTime;
		}
	}
	else {
		curps = &prev->ps;
		currentTime = prev->serverTime;
		currentServerTime = prev->serverTime;
		if (next) {
			nextps = &next->ps;
			nextTime = next->serverTime;
			nextServerTime = next->serverTime;
		}
		else {
			nextps = NULL;
			nextTime = 0;
			nextServerTime = 0;
		}
		// TODO: Do I need to add nextPsTeleport = cg.nextFrameTeleport; here? its not here but in jamme
	}
	if (!nextps) {
		return;
	}
#ifdef _DEBUG
	if (!(currentTime <= cg.time + (double)cg.timeFraction && nextTime >= cg.time + (double)cg.timeFraction)) {
		Com_Printf("WARNING: Couldn't locate slots with correct time\n");
	}
	if (nextTime - currentTime < 0) {
		Com_Printf("WARNING: nexttps->time - tps->time < 0 (%d, %d)\n", nextTime, currentTime);
	}
#endif
	if (currentTime != nextTime) {
		f = ((double)cg.timeFraction + (cg.time - currentTime)) / (nextTime - currentTime);
#ifdef _DEBUG
		if (f > 1) {
			Com_Printf("EXTRAPOLATING (f=%f)\n", f);
		}
		else if (f < 0) {
			Com_Printf("GOING BACKWARDS (f=%f)\n", f);
		}
#endif
	}
	else {
		f = 0;
	}

	*out = *curps;
	out->stats[STAT_HEALTH] = cg.snap->ps.stats[STAT_HEALTH];

	// if we are still allowing local input, short circuit the view angles
	if ( grabAngles ) {
		usercmd_t	cmd;
		int			cmdNum;

		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd( cmdNum, &cmd );

		PM_UpdateViewAngles( out, &cmd );
	}

	// if the next frame is a teleport, we can't lerp to it
	if (nextPsTeleport) {
		return;
	}

	/*if ( !next || next->serverTime <= prev->serverTime ) {
		return;
	}*/

	i = nextps->bobCycle;
	if (i < curps->bobCycle) {
		i += 256;		// handle wraparound
	}
	out->bobCycle = curps->bobCycle + f * (i - curps->bobCycle);

	for ( i = 0 ; i < 3 ; i++ ) {
		out->origin[i] = curps->origin[i] + f * (nextps->origin[i] - curps->origin[i]);
		if ( !grabAngles ) {
			out->viewangles[i] = LerpAngle( 
				curps->viewangles[i], nextps->viewangles[i], f);
		}
		out->velocity[i] = curps->velocity[i] +
			f * (nextps->velocity[i] - curps->velocity[i]);
	}

	// requires commandSmooth 2, since it needs entity state history of the mover
	if (cg_commandSmooth.integer > 1) {
		// adjust for the movement of the groundentity
		// step 1: remove the delta introduced by cur->next origin interpolation
		int curTime = currentServerTime + (int)(f * (nextServerTime - currentServerTime));
		float curTimeFraction = (f * (nextServerTime - currentServerTime));
		curTimeFraction -= (long)curTimeFraction;
		CG_AdjustInterpolatedPositionForMover(out->origin,
			curps->groundEntityNum, curTime, curTimeFraction, currentServerTime, 0, out->origin);
		// step 2: mover state should now be what it was at currentServerTime, now redo calculation of mover effect to cg.time
		CG_AdjustInterpolatedPositionForMover(out->origin,
			curps->groundEntityNum, currentServerTime, 0, cg.time, cg.timeFraction, out->origin);
	}

	curps->stats[STAT_HEALTH] = cg.snap->ps.stats[STAT_HEALTH];
	BG_PlayerStateToEntityState(curps, &cg_entities[curps->clientNum].currentState, qfalse);
	BG_PlayerStateToEntityState(nextps, &cg_entities[nextps->clientNum].nextState, qfalse);
	cg.playerInterpolation = f;
	if (cg.timeFraction >= cg.nextSnap->serverTime - cg.time) {
		cg.physicsTime = cg.nextSnap->serverTime;
	}
	else {
		cg.physicsTime = cg.snap->serverTime;
	}

	if (cam_specEnt.integer != -1 && cam_specEnt.integer != cg.snap->ps.clientNum) // Awkwardly ported from Nerevar's cammod
	{
		centity_t* cent = &cg_entities[cam_specEnt.integer];
		for (i = 0; i < 3; i++) {
			//cam.specOrg[i] = cent->currentState.pos.trBase[i] + f * (cent->nextState.pos.trBase[i] - cent->currentState.pos.trBase[i]);

			//cam.specAng[i] = LerpAngle(cent->currentState.apos.trBase[i], cent->nextState.apos.trBase[i], f);

			cam.specVel[i] = cent->currentState.pos.trDelta[i] + f * (cent->nextState.pos.trDelta[i] - cent->currentState.pos.trDelta[i]);
		}
	}


	// TODO: What about this? not in jamme
	//cg.predictedTimeFrac = f * (next->ps.commandTime - prev->ps.commandTime);
	cg.predictedTimeFrac = f * (nextps->commandTime - curps->commandTime);
}

/*
===================
CG_TouchItem
===================
*/
static void CG_TouchItem( centity_t *cent ) {
	gitem_t		*item;

	if ( !cg_predictItems.integer ) {
		return;
	}
	if ( !BG_PlayerTouchesItem( &cg.predictedPlayerState, &cent->currentState, cg.time ) ) {
		return;
	}

	if (cent->currentState.eFlags & EF_ITEMPLACEHOLDER)
	{
		return;
	}

	if (cent->currentState.eFlags & EF_NODRAW)
	{
		return;
	}

	// never pick an item up twice in a prediction
	if ( cent->miscTime == cg.time ) {
		return;
	}

	if ( !BG_CanItemBeGrabbed( cgs.gametype, &cent->currentState, &cg.predictedPlayerState ) ) {
		return;		// can't hold it
	}

	item = &bg_itemlist[ cent->currentState.modelindex ];

	//Currently there is no reliable way of knowing if the client has touched a certain item before another if they are next to each other, or rather
	//if the server has touched them in the same order. This results often in grabbing an item in the prediction and the server giving you the other
	//item. So for now prediction of armor, health, and ammo is disabled.
/*
	if (item->giType == IT_ARMOR)
	{ //rww - this will be stomped next update, but it's set so that we don't try to pick up two shields in one prediction and have the server cancel one
	//	cg.predictedPlayerState.stats[STAT_ARMOR] += item->quantity;

		//FIXME: This can't be predicted properly at the moment
		return;
	}

	if (item->giType == IT_HEALTH)
	{ //same as above, for health
	//	cg.predictedPlayerState.stats[STAT_HEALTH] += item->quantity;

		//FIXME: This can't be predicted properly at the moment
		return;
	}

	if (item->giType == IT_AMMO)
	{ //same as above, for ammo
	//	cg.predictedPlayerState.ammo[item->giTag] += item->quantity;

		//FIXME: This can't be predicted properly at the moment
		return;
	}

	if (item->giType == IT_HOLDABLE)
	{ //same as above, for holdables
	//	cg.predictedPlayerState.stats[STAT_HOLDABLE_ITEMS] |= (1 << item->giTag);
	}
*/
	// Special case for flags.  
	// We don't predict touching our own flag
	if( cgs.gametype == GT_CTF || cgs.gametype == GT_CTY ) {
		if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_RED &&
			item->giTag == PW_REDFLAG)
			return;
		if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_BLUE &&
			item->giTag == PW_BLUEFLAG)
			return;
	}

	if (item->giType == IT_POWERUP &&
		(item->giTag == PW_FORCE_ENLIGHTENED_LIGHT || item->giTag == PW_FORCE_ENLIGHTENED_DARK))
	{
		if (item->giTag == PW_FORCE_ENLIGHTENED_LIGHT)
		{
			if (cg.predictedPlayerState.fd.forceSide != FORCE_LIGHTSIDE)
			{
				return;
			}
		}
		else
		{
			if (cg.predictedPlayerState.fd.forceSide != FORCE_DARKSIDE)
			{
				return;
			}
		}
	}


	// grab it
	BG_AddPredictableEventToPlayerstate( EV_ITEM_PICKUP, cent->currentState.number , &cg.predictedPlayerState);

	// remove it from the frame so it won't be drawn
	cent->currentState.eFlags |= EF_NODRAW;

	// don't touch it again this prediction
	cent->miscTime = cg.time;

	// if its a weapon, give them some predicted ammo so the autoswitch will work
	if ( item->giType == IT_WEAPON ) {
		cg.predictedPlayerState.stats[ STAT_WEAPONS ] |= 1 << item->giTag;
		if ( !cg.predictedPlayerState.ammo[ item->giTag ] ) {
			cg.predictedPlayerState.ammo[ item->giTag ] = 1;
		}
	}
}


/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and items
=========================
*/
static void CG_TouchTriggerPrediction( void ) {
	int			i;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t cmodel;
	centity_t	*cent;
	qboolean	spectator;

	// dead clients don't activate triggers
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	spectator = ( cg.predictedPlayerState.pm_type == PM_SPECTATOR );

	if ( cg.predictedPlayerState.pm_type != PM_NORMAL && cg.predictedPlayerState.pm_type != PM_FLOAT && !spectator ) {
		return;
	}

	for ( i = 0 ; i < cg_numTriggerEntities ; i++ ) {
		cent = cg_triggerEntities[ i ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM && !spectator ) {
			CG_TouchItem( cent );
			continue;
		}

		if ( ent->solid != SOLID_BMODEL ) {
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		trap_CM_BoxTrace( &trace, cg.predictedPlayerState.origin, cg.predictedPlayerState.origin, 
			cg_pmove.mins, cg_pmove.maxs, cmodel, -1 );

		if ( !trace.startsolid ) {
			continue;
		}

		if ( ent->eType == ET_TELEPORT_TRIGGER ) {
			cg.hyperspace = qtrue;
		} else if ( ent->eType == ET_PUSH_TRIGGER ) {
			BG_TouchJumpPad( &cg.predictedPlayerState, ent );
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( cg.predictedPlayerState.jumppad_frame != cg.predictedPlayerState.pmove_framecount ) {
		cg.predictedPlayerState.jumppad_frame = 0;
		cg.predictedPlayerState.jumppad_ent = 0;
	}
}

void CG_EntityStateToPlayerState( entityState_t *s, playerState_t *ps ) {
	int		i;

	ps->clientNum = s->number;

	VectorCopy( s->pos.trBase, ps->origin );

	VectorCopy( s->pos.trDelta, ps->velocity );

	VectorCopy( s->apos.trBase, ps->viewangles );

	ps->fd.forceMindtrickTargetIndex = s->trickedentindex;
	ps->fd.forceMindtrickTargetIndex2 = s->trickedentindex2;
	ps->fd.forceMindtrickTargetIndex3 = s->trickedentindex3;
	ps->fd.forceMindtrickTargetIndex4 = s->trickedentindex4;

	ps->saberLockFrame = s->forceFrame;

	ps->electrifyTime = s->emplacedOwner;

	ps->speed = s->speed;

	ps->genericEnemyIndex = s->genericenemyindex;

	ps->activeForcePass = s->activeForcePass;

	ps->movementDir = s->angles2[YAW];
	ps->legsAnim = s->legsAnim;
	ps->torsoAnim = s->torsoAnim;
	ps->clientNum = s->clientNum;

	ps->eFlags = s->eFlags;

	ps->saberInFlight = s->saberInFlight;
	ps->saberEntityNum = s->saberEntityNum;
	ps->saberMove = s->saberMove;
	ps->fd.forcePowersActive = s->forcePowersActive;

	if (s->bolt1)
	{
		ps->duelInProgress = qtrue;
	}
	else
	{
		ps->duelInProgress = qfalse;
	}

	if (s->bolt2)
	{
		ps->dualBlade = qtrue;
	}
	else
	{
		ps->dualBlade = qfalse;
	}

	ps->emplacedIndex = s->otherEntityNum2;

	ps->saberHolstered = s->shouldtarget; //reuse bool in entitystate for players differently
	ps->usingATST = s->teamowner;

	/*
	if (ps->genericEnemyIndex != -1)
	{
		s->eFlags |= EF_SEEKERDRONE;
	}
	*/
	ps->genericEnemyIndex = -1; //no real option for this

	//The client has no knowledge of health levels (except for the client entity)
	if (s->eFlags & EF_DEAD)
	{
		ps->stats[STAT_HEALTH] = 0;
	}
	else
	{
		ps->stats[STAT_HEALTH] = 100;
	}

	/*
	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}
	*/
	//Eh.

	ps->weapon = s->weapon;
	ps->groundEntityNum = s->groundEntityNum;

	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if (s->powerups & (1 << i))
		{
			ps->powerups[i] = 30;
		}
		else
		{
			ps->powerups[i] = 0;
		}
	}

	ps->loopSound = s->loopSound;
	ps->generic1 = s->generic1;
}

playerState_t cgSendPS[MAX_CLIENTS];

/*
=================
CG_PredictPlayerState

Generates cg.predictedPlayerState for the current cg.time
cg.predictedPlayerState is guaranteed to be valid after exiting.

For demo playback, this will be an interpolation between two valid
playerState_t.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new snapshot will usually have one or more new usercmd over the last,
but we simulate all unacknowledged commands each time, not just the new ones.
This means that on an internet connection, quite a few pmoves may be issued
each frame.

OPTIMIZE: don't re-simulate unless the newly arrived snapshot playerState_t
differs from the predicted one.  Would require saving all intermediate
playerState_t during prediction.

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/
void CG_PredictPlayerState( void ) {
	int			cmdNum, current, i;
	playerState_t	oldPlayerState;
	qboolean	moved;
	usercmd_t	oldestCmd;
	usercmd_t	latestCmd;

	cg.hyperspace = qfalse;	// will be set if touching a trigger_teleport

	// if this is the first frame we must guarantee
	// predictedPlayerState is valid even if there is some
	// other error condition
	if ( !cg.validPPS ) {
		cg.validPPS = qtrue;
		cg.predictedPlayerState = cg.snap->ps;
	}

	// demo playback just copies the moves
	if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		CG_InterpolatePlayerState( qfalse );
		return;
	}

	// non-predicting local movement will grab the latest angles
	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_InterpolatePlayerState( qtrue );
		return;
	}

	// prepare for pmove
	cg_pmove.ps = &cg.predictedPlayerState;
	cg_pmove.trace = CG_Trace;
	cg_pmove.pointcontents = CG_PointContents;
	if ( cg_pmove.ps->pm_type == PM_DEAD ) {
		cg_pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else {
		cg_pmove.tracemask = MASK_PLAYERSOLID;
	}
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		cg_pmove.tracemask &= ~CONTENTS_BODY;	// spectators can fly through bodies
	}
	cg_pmove.noFootsteps = ( cgs.dmflags & DF_NO_FOOTSTEPS ) > 0;

	// save the state before the pmove so we can detect transitions
	oldPlayerState = cg.predictedPlayerState;

	current = trap_GetCurrentCmdNumber();

	// if we don't have the commands right after the snapshot, we
	// can't accurately predict a current position, so just freeze at
	// the last good position we had
	cmdNum = current - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &oldestCmd );
	if ( oldestCmd.serverTime > cg.snap->ps.commandTime 
		&& oldestCmd.serverTime < cg.time ) {	// special check for map_restart
		if ( cg_showmiss.integer ) {
			CG_Printf ("exceeded PACKET_BACKUP on commands\n");
		}
		return;
	}

	// get the latest command so we can know which commands are from previous map_restarts
	trap_GetUserCmd( current, &latestCmd );

	// get the most recent information we have, even if
	// the server time is beyond our current cg.time,
	// because predicted player positions are going to 
	// be ahead of everything else anyway
	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
		cg.predictedPlayerState = cg.nextSnap->ps;
		cg.physicsTime = cg.nextSnap->serverTime;
	} else {
		cg.predictedPlayerState = cg.snap->ps;
		cg.physicsTime = cg.snap->serverTime;
	}

	if ( pmove_msec.integer < 8 ) {
		trap_Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
	}

	cg_pmove.pmove_fixed = pmove_fixed.integer;// | cg_pmove_fixed.integer;
	cg_pmove.pmove_msec = pmove_msec.integer;

	// run cmds
	moved = qfalse;
	for ( cmdNum = current - CMD_BACKUP + 1 ; cmdNum <= current ; cmdNum++ ) {
		// get the command
		trap_GetUserCmd( cmdNum, &cg_pmove.cmd );

		if ( cg_pmove.pmove_fixed ) {
			PM_UpdateViewAngles( cg_pmove.ps, &cg_pmove.cmd );
		}

		// don't do anything if the time is before the snapshot player time
		if ( cg_pmove.cmd.serverTime <= cg.predictedPlayerState.commandTime ) {
			continue;
		}

		// don't do anything if the command was from a previous map_restart
		if ( cg_pmove.cmd.serverTime > latestCmd.serverTime ) {
			continue;
		}

		// check for a prediction error from last frame
		// on a lan, this will often be the exact value
		// from the snapshot, but on a wan we will have
		// to predict several commands to get to the point
		// we want to compare
		if ( cg.predictedPlayerState.commandTime == oldPlayerState.commandTime ) {
			vec3_t	delta;
			float	len;

			if ( cg.thisFrameTeleport ) {
				// a teleport will not cause an error decay
				VectorClear( cg.predictedError );
				if ( cg_showmiss.integer ) {
					CG_Printf( "PredictionTeleport\n" );
				}
				cg.thisFrameTeleport = qfalse;
			} else {
				vec3_t	adjusted;
				CG_AdjustPositionForMover( cg.predictedPlayerState.origin, 
					cg.predictedPlayerState.groundEntityNum, cg.physicsTime, cg.oldTime, adjusted );

				if ( cg_showmiss.integer ) {
					if (!VectorCompare( oldPlayerState.origin, adjusted )) {
						CG_Printf("prediction error\n");
					}
				}
				VectorSubtract( oldPlayerState.origin, adjusted, delta );
				len = VectorLength( delta );
				if ( len > 0.1 ) {
					if ( cg_showmiss.integer ) {
						CG_Printf("Prediction miss: %f\n", len);
					}
					if ( cg_errorDecay.integer ) {
						int		t;
						float	f;

						t = cg.time - cg.predictedErrorTime;
						f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
						if ( f < 0 ) {
							f = 0;
						}
						if ( f > 0 && cg_showmiss.integer ) {
							CG_Printf("Double prediction decay: %f\n", f);
						}
						VectorScale( cg.predictedError, f, cg.predictedError );
					} else {
						VectorClear( cg.predictedError );
					}
					VectorAdd( delta, cg.predictedError, cg.predictedError );
					cg.predictedErrorTime = cg.oldTime;
				}
			}
		}

		if ( cg_pmove.pmove_fixed ) {
			cg_pmove.cmd.serverTime = ((cg_pmove.cmd.serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
		}

		cg_pmove.animations = bgGlobalAnimations;

		cg_pmove.gametype = cgs.gametype;

		for ( i = 0 ; i < MAX_CLIENTS ; i++ )
		{
			memset(&cgSendPS[i], 0, sizeof(cgSendPS[i]));
			CG_EntityStateToPlayerState(&cg_entities[i].currentState, &cgSendPS[i]);
			cg_pmove.bgClients[i] = &cgSendPS[i];
		}

		if (cg.snap && cg.snap->ps.saberLockTime > cg.time)
		{
			centity_t *blockOpp = &cg_entities[cg.snap->ps.saberLockEnemy];

			if (blockOpp)
			{
				vec3_t lockDir, lockAng;

				VectorSubtract( blockOpp->lerpOrigin, cg.snap->ps.origin, lockDir );
				//lockAng[YAW] = vectoyaw( defDir );
				vectoangles(lockDir, lockAng);

				VectorCopy(lockAng, cg_pmove.ps->viewangles);
			}
		}

		Pmove (&cg_pmove);

		for ( i = 0 ; i < MAX_CLIENTS ; i++ )
		{
			cg_entities[i].currentState.torsoAnim = cgSendPS[i].torsoAnim;
			cg_entities[i].currentState.legsAnim = cgSendPS[i].legsAnim;
			cg_entities[i].currentState.forceFrame = cgSendPS[i].saberLockFrame;
			cg_entities[i].currentState.saberMove = cgSendPS[i].saberMove;
		}

		moved = qtrue;

		// add push trigger movement effects
		CG_TouchTriggerPrediction();

		// check for predictable events that changed from previous predictions
		//CG_CheckChangedPredictableEvents(&cg.predictedPlayerState);
	}

	if ( cg_showmiss.integer > 1 ) {
		CG_Printf( "[%i : %i] ", cg_pmove.cmd.serverTime, cg.time );
	}

	if ( !moved ) {
		if ( cg_showmiss.integer ) {
			CG_Printf( "not moved\n" );
		}
		return;
	}

	// adjust for the movement of the groundentity
	CG_AdjustPositionForMover( cg.predictedPlayerState.origin, 
		cg.predictedPlayerState.groundEntityNum, 
		cg.physicsTime, cg.time, cg.predictedPlayerState.origin );

	if ( cg_showmiss.integer ) {
		if (cg.predictedPlayerState.eventSequence > oldPlayerState.eventSequence + MAX_PS_EVENTS) {
			CG_Printf("WARNING: dropped event\n");
		}
	}

	cg.predictedTimeFrac = 0.0f;

	// fire events and other transition triggered things
	CG_TransitionPlayerState( &cg.predictedPlayerState, &oldPlayerState );

	if ( cg_showmiss.integer ) {
		if (cg.eventSequence > cg.predictedPlayerState.eventSequence) {
			CG_Printf("WARNING: double event\n");
			cg.eventSequence = cg.predictedPlayerState.eventSequence;
		}
	}
}

void CG_StartBehindCamera(vec3_t start, vec3_t end, const vec3_t camOrg, const vec3_t camAxis[3], vec3_t entDirection) {
	vec3_t beamDirection, a, forwardDirection;

	AxisToAngles(camAxis, a);
	AngleVectors (a, forwardDirection, NULL, NULL);
	VectorNormalize(forwardDirection);
	VectorSubtract(start, camOrg, beamDirection);
	VectorNormalize(beamDirection);

	if (DotProduct(forwardDirection, beamDirection) < 0) {
		vec3_t beamEnd;
		trace_t trace;

		if (end) {
			vec3_t temp;
			VectorCopy(start, temp);
			VectorCopy(end, start);
			VectorCopy(temp, end);
			return;
		}
		if (!entDirection)
			return;

		VectorMA(start, 5000, entDirection, beamEnd);
		CG_Trace(&trace, start, NULL, NULL, beamEnd, -1, CONTENTS_SOLID);

		if (trace.fraction < 1.0f) {
			VectorCopy(trace.endpos, beamEnd);
			VectorInverse(entDirection);
			VectorCopy(beamEnd, start);
			return;
		}
	}
}
