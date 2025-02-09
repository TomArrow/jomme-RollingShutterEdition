// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#include "../ui/ui_shared.h"
extern menuDef_t *menuScoreboard;



void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer();
	if (!targetNum ) {
		return;
	}

	trap_Argv( 1, test, 4 );
	trap_SendConsoleCommand( va( "gc %i %i", targetNum, atoi( test ) ) );
}



/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer+10)));
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer-10)));
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f (void) {
	//CG_Printf ("%s (%i %i %i) : %i\n", cgs.mapname, (int)cg.refdef.vieworg[0],
	//	(int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2], 
	//	(int)cg.refdefViewAngles[YAW]);
	CG_Printf ("%s (%f %f %f) : %f %f %f\n", cgs.mapname, cg.refdef.vieworg[0],
		cg.refdef.vieworg[1], cg.refdef.vieworg[2], 
		cg.refdefViewAngles[PITCH], cg.refdefViewAngles[YAW], cg.refdefViewAngles[ROLL]);
}


static void CG_ScoresDown_f( void ) {

	CG_BuildSpectatorString();
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores ) {
			cg.showScores = qtrue;
			cg.numScores = 0;
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}

static void CG_ScoresUp_f( void ) {
	if ( cg.showScores ) {
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}

extern menuDef_t *menuScoreboard;
void Menu_Reset();			// FIXME: add to right include file

static void CG_scrollScoresDown_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qtrue);
	}
}


static void CG_scrollScoresUp_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qfalse);
	}
}


static void CG_spWin_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.winnerSound);
	//trap_S_StartLocalSound(cgs.media.winnerSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU WIN!", SCREEN_HEIGHT * .30, 0);
}

static void CG_spLose_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.loserSound);
	//trap_S_StartLocalSound(cgs.media.loserSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU LOSE...", SCREEN_HEIGHT * .30, 0);
}


static void CG_TellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_TellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_VoiceTellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_VoiceTellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_NextTeamMember_f( void ) {
  CG_SelectNextPlayer();
}

static void CG_PrevTeamMember_f( void ) {
  CG_SelectPrevPlayer();
}

// ASS U ME's enumeration order as far as task specific orders, OFFENSE is zero, CAMP is last
//
static void CG_NextOrder_f( void ) {
	clientInfo_t *ci = cgs.clientinfo + cg.snap->ps.clientNum;
	if (ci) {
		if (!ci->teamLeader && sortedTeamPlayers[cg_currentSelectedPlayer.integer] != cg.snap->ps.clientNum) {
			return;
		}
	}
	if (cgs.currentOrder < TEAMTASK_CAMP) {
		cgs.currentOrder++;

		if (cgs.currentOrder == TEAMTASK_RETRIEVE) {
			if (!CG_OtherTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

		if (cgs.currentOrder == TEAMTASK_ESCORT) {
			if (!CG_YourTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

	} else {
		cgs.currentOrder = TEAMTASK_OFFENSE;
	}
	cgs.orderPending = qtrue;
	cgs.orderTime = cg.time + 3000;
}


static void CG_ConfirmOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_YES));
	trap_SendConsoleCommand("+button5; wait; -button5");
	if (cg.time < cgs.acceptOrderTime) {
		trap_SendClientCommand(va("teamtask %d\n", cgs.acceptTask));
		cgs.acceptOrderTime = 0;
	}
}

static void CG_DenyOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_NO));
	trap_SendConsoleCommand("+button6; wait; -button6");
	if (cg.time < cgs.acceptOrderTime) {
		cgs.acceptOrderTime = 0;
	}
}

static void CG_TaskOffense_f (void ) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_CTY) {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONGETFLAG));
	} else {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONOFFENSE));
	}
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_OFFENSE));
}

static void CG_TaskDefense_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONDEFENSE));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_DEFENSE));
}

static void CG_TaskPatrol_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONPATROL));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_PATROL));
}

static void CG_TaskCamp_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONCAMPING));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_CAMP));
}

static void CG_TaskFollow_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOW));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_FOLLOW));
}

static void CG_TaskRetrieve_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONRETURNFLAG));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_RETRIEVE));
}

static void CG_TaskEscort_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOWCARRIER));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_ESCORT));
}

static void CG_TaskOwnFlag_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_IHAVEFLAG));
}

static void CG_TauntKillInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay kill_insult\n");
}

static void CG_TauntPraise_f (void ) {
	trap_SendConsoleCommand("cmd vsay praise\n");
}

static void CG_TauntTaunt_f (void ) {
	trap_SendConsoleCommand("cmd vtaunt\n");
}

static void CG_TauntDeathInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay death_insult\n");
}

static void CG_TauntGauntlet_f (void ) {
	trap_SendConsoleCommand("cmd vsay kill_guantlet\n");
}

static void CG_TaskSuicide_f (void ) {
	int		clientNum;
	char	command[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	Com_sprintf( command, 128, "tell %i suicide", clientNum );
	trap_SendClientCommand( command );
}



/*
==================
CG_TeamMenu_f
==================
*/
/*
static void CG_TeamMenu_f( void ) {
  if (trap_Key_GetCatcher() & KEYCATCH_CGAME) {
    CG_EventHandling(CGAME_EVENT_NONE);
    trap_Key_SetCatcher(0);
  } else {
    CG_EventHandling(CGAME_EVENT_TEAMMENU);
    //trap_Key_SetCatcher(KEYCATCH_CGAME);
  }
}
*/

/*
==================
CG_EditHud_f
==================
*/
/*
static void CG_EditHud_f( void ) {
  //cls.keyCatchers ^= KEYCATCH_CGAME;
  //VM_Call (cgvm, CG_EVENT_HANDLING, (cls.keyCatchers & KEYCATCH_CGAME) ? CGAME_EVENT_EDITHUD : CGAME_EVENT_NONE);
}
*/


/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f( void ) {
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "developer", var, sizeof( var ) );
	if ( !atoi(var) ) {
		return;
	}
	if (cg_cameraOrbit.value != 0) {
		trap_Cvar_Set ("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}


//jk2pro stuff ported from eternaljk2mv
typedef struct bitInfo_S {
	const char* string;
} bitInfo_T;

static bitInfo_T strafeTweaks[] = {
	{ "Original style" },//0
	{ "Updated style" },//1
	{ "Cgaz style" },//2
	{ "Warsow style" },//3
	{ "Sound" },//4
	{ "W" },//5
	{ "WA" },//6
	{ "WD" },//7
	{ "A" },//8
	{ "D" },//9
	{ "Rear" },//10
	{ "Center" },//11
	{ "Accel bar" },//12
	{ "Weze style" },//13
	{ "Line Crosshair" }//13
};
static const int MAX_STRAFEHELPER_TWEAKS = ARRAY_LEN(strafeTweaks);

void CG_StrafeHelper_f(void) {
	if (trap_Argc() == 1) {
		int i = 0;
		for (i = 0; i < MAX_STRAFEHELPER_TWEAKS; i++) {
			if ((cg_strafeHelper.integer & (1 << i))) {
				Com_Printf("%2d [X] %s\n", i, strafeTweaks[i].string);
			}
			else {
				Com_Printf("%2d [ ] %s\n", i, strafeTweaks[i].string);
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_STRAFEHELPER_TWEAKS) - 1;

		trap_Argv(1, arg, sizeof(arg));
		index = atoi(arg);

		if (index < 0 || index >= MAX_STRAFEHELPER_TWEAKS) {
			Com_Printf("strafeHelper: Invalid range: %i [0, %i]\n", index, MAX_STRAFEHELPER_TWEAKS - 1);
			return;
		}

		if ((index == 0 || index == 1 || index == 2 || index == 3 || index == 13)) { //Radio button these options
																					 //Toggle index, and make sure everything else in this group (0,1,2,3,13) is turned off
			int groupMask = (1 << 0) + (1 << 1) + (1 << 2) + (1 << 3) + (1 << 13);
			int value = cg_strafeHelper.integer;

			groupMask &= ~(1 << index); //Remove index from groupmask
			value &= ~(groupMask); //Turn groupmask off
			value ^= (1 << index); //Toggle index item

			trap_Cvar_Set("cg_strafeHelper", va("%i", value));
		}
		else {
			trap_Cvar_Set("cg_strafeHelper", va("%i", (1 << index) ^ (cg_strafeHelper.integer & mask)));
		}
		trap_Cvar_Update(&cg_strafeHelper);

		Com_Printf("%s %s^7\n", strafeTweaks[index].string, ((cg_strafeHelper.integer & (1 << index))
			? "^2Enabled" : "^1Disabled"));
	}
}

static bitInfo_T speedometerSettings[] = { // MAX_WEAPON_TWEAKS tweaks (24)
	{ "Enable speedometer" },//0
	{ "Pre-speed display" },//1
	{ "Jump height display" },//2
	{ "Jump distance display" },//3
	{ "Vertical speed indicator" },//4
	{ "Yaw speed indicator" },//5
	{ "Accel meter" },//6
	{ "Speed graph" },//7
	{ "Display speed in kilometers instead of units" },//8
	{ "Display speed in imperial miles instead of units" },//9
	{ "Disable speed display" },//10
};
static const int MAX_SPEEDOMETER_SETTINGS = ARRAY_LEN(speedometerSettings);

void cg_speedometer_f(void)
{
	if (trap_Argc() == 1) {
		int i = 0, display = 0;

		for (i = 0; i < MAX_SPEEDOMETER_SETTINGS; i++) {
			if ((cg_speedometer.integer & (1 << i))) {
				Com_Printf("%2d [X] %s\n", display, speedometerSettings[i].string);
			}
			else {
				Com_Printf("%2d [ ] %s\n", display, speedometerSettings[i].string);
			}
			display++;
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index, index2;
		const uint32_t mask = (1 << MAX_SPEEDOMETER_SETTINGS) - 1;

		trap_Argv(1, arg, sizeof(arg));
		index = atoi(arg);
		index2 = index;

		if (index2 < 0 || index2 >= MAX_SPEEDOMETER_SETTINGS) {
			Com_Printf("style: Invalid range: %i [0, %i]\n", index2, MAX_SPEEDOMETER_SETTINGS - 1);
			return;
		}

		if (index == 8 || index == 9) { //Radio button these options
		//Toggle index, and make sure everything else in this group (8,9) is turned off
			int groupMask = (1 << 8) + (1 << 9);
			int value = cg_speedometer.integer;

			groupMask &= ~(1 << index); //Remove index from groupmask
			value &= ~(groupMask); //Turn groupmask off
			value ^= (1 << index); //Toggle index item

			trap_Cvar_Set("cg_speedometer", va("%i", value));
		}
		else {
			trap_Cvar_Set("cg_speedometer", va("%i", (1 << index) ^ (cg_speedometer.integer & mask)));
		}
		trap_Cvar_Update(&cg_speedometer);

		Com_Printf("%s %s^7\n", speedometerSettings[index2].string, ((cg_speedometer.integer & (1 << index2))
			? "^2Enabled" : "^1Disabled"));
	}
}

void CG_ClientList_f(void)
{
	clientInfo_t* ci;
	int i;
	int count = 0;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ci = &cgs.clientinfo[i];
		if (!ci->infoValid)
			continue;

		switch (ci->team)
		{
		case TEAM_FREE:
			if (cgs.isCTFMod && cgs.CTF3ModeActive) {
				CG_Printf("%2d " S_COLOR_YELLOW "Y   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
					ci->name, (ci->botSkill != 0) ? " (bot)" : "");
			}
			else {
				CG_Printf("%2d " S_COLOR_YELLOW "F   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
					ci->name, (ci->botSkill != 0) ? " (bot)" : "");
			}
			break;

		case TEAM_RED:
			CG_Printf("%2d " S_COLOR_RED "R   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
				ci->name, (ci->botSkill != 0) ? " (bot)" : "");
			break;

		case TEAM_BLUE:
			CG_Printf("%2d " S_COLOR_BLUE "B   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
				ci->name, (ci->botSkill != 0) ? " (bot)" : "");
			break;

		default:
		case TEAM_SPECTATOR:
			CG_Printf("%2d " S_COLOR_YELLOW "S   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
				ci->name, (ci->botSkill != 0) ? " (bot)" : "");
			break;
		}

		count++;
	}

	CG_Printf("Listed %2d clients\n", count);
}

/*
static void CG_Camera_f( void ) {
	char name[1024];
	trap_Argv( 1, name, sizeof(name));
	if (trap_loadCamera(name)) {
		cg.cameraMode = qtrue;
		trap_startCamera(cg.time);
	} else {
		CG_Printf ("Unable to load camera %s\n",name);
	}
}
*/


typedef struct {
	char	*cmd;
	void	(*function)(void);
} consoleCommand_t;

static consoleCommand_t	commands[] = {
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "viewpos", CG_Viewpos_f },
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "tell_target", CG_TellTarget_f },
	{ "tell_attacker", CG_TellAttacker_f },
	{ "vtell_target", CG_VoiceTellTarget_f },
	{ "vtell_attacker", CG_VoiceTellAttacker_f },
	{ "tcmd", CG_TargetCommand_f },
	{ "nextTeamMember", CG_NextTeamMember_f },
	{ "prevTeamMember", CG_PrevTeamMember_f },
	{ "nextOrder", CG_NextOrder_f },
	{ "confirmOrder", CG_ConfirmOrder_f },
	{ "denyOrder", CG_DenyOrder_f },
	{ "taskOffense", CG_TaskOffense_f },
	{ "taskDefense", CG_TaskDefense_f },
	{ "taskPatrol", CG_TaskPatrol_f },
	{ "taskCamp", CG_TaskCamp_f },
	{ "taskFollow", CG_TaskFollow_f },
	{ "taskRetrieve", CG_TaskRetrieve_f },
	{ "taskEscort", CG_TaskEscort_f },
	{ "taskSuicide", CG_TaskSuicide_f },
	{ "taskOwnFlag", CG_TaskOwnFlag_f },
	{ "tauntKillInsult", CG_TauntKillInsult_f },
	{ "tauntPraise", CG_TauntPraise_f },
	{ "tauntTaunt", CG_TauntTaunt_f },
	{ "tauntDeathInsult", CG_TauntDeathInsult_f },
	{ "tauntGauntlet", CG_TauntGauntlet_f },
	{ "spWin", CG_spWin_f },
	{ "spLose", CG_spLose_f },
	{ "scoresDown", CG_scrollScoresDown_f },
	{ "scoresUp", CG_scrollScoresUp_f },
	{ "startOrbit", CG_StartOrbit_f },
	//{ "camera", CG_Camera_f },
	{ "loaddeferred", CG_LoadDeferredPlayers },
	{ "invnext", CG_NextInventory_f },
	{ "invprev", CG_PrevInventory_f },
	{ "forcenext", CG_NextForcePower_f },
	{ "forceprev", CG_PrevForcePower_f },

	//jk2pro stuff ported from eternal
	{ "strafeHelper", CG_StrafeHelper_f },
	{ "speedometer", cg_speedometer_f },
	{ "clientlist", CG_ClientList_f },
};


/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	const char	*cmd;
	int		i;

	cmd = CG_Argv(0);

	for ( i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		if ( !Q_stricmp( cmd, commands[i].cmd ) ) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	int		i;

	for ( i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		trap_AddCommand( commands[i].cmd );
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand ("forcechanged");
	trap_AddCommand ("sv_invnext");
	trap_AddCommand ("sv_invprev");
	trap_AddCommand ("sv_forcenext");
	trap_AddCommand ("sv_forceprev");
	trap_AddCommand ("sv_saberswitch");
	trap_AddCommand ("engage_duel");
	trap_AddCommand ("force_heal");
	trap_AddCommand ("force_speed");
	trap_AddCommand ("force_throw");
	trap_AddCommand ("force_pull");
	trap_AddCommand ("force_distract");
	trap_AddCommand ("force_rage");
	trap_AddCommand ("force_protect");
	trap_AddCommand ("force_absorb");
	trap_AddCommand ("force_healother");
	trap_AddCommand ("force_forcepowerother");
	trap_AddCommand ("force_seeing");
	trap_AddCommand ("use_seeker");
	trap_AddCommand ("use_field");
	trap_AddCommand ("use_bacta");
	trap_AddCommand ("use_electrobinoculars");
	trap_AddCommand ("zoom");
	trap_AddCommand ("use_sentry");
	trap_AddCommand ("bot_order");
	trap_AddCommand ("saberAttackCycle");
	trap_AddCommand ("kill");
	trap_AddCommand ("say");
	trap_AddCommand ("say_team");
	trap_AddCommand ("tell");
	trap_AddCommand ("vsay");
	trap_AddCommand ("vsay_team");
	trap_AddCommand ("vtell");
	trap_AddCommand ("vtaunt");
	trap_AddCommand ("vosay");
	trap_AddCommand ("vosay_team");
	trap_AddCommand ("votell");
	trap_AddCommand ("give");
	trap_AddCommand ("god");
	trap_AddCommand ("notarget");
	trap_AddCommand ("noclip");
	trap_AddCommand ("team");
	trap_AddCommand ("follow");
	trap_AddCommand ("levelshot");
	trap_AddCommand ("addbot");
	trap_AddCommand ("setviewpos");
	trap_AddCommand ("callvote");
	trap_AddCommand ("vote");
	trap_AddCommand ("callteamvote");
	trap_AddCommand ("teamvote");
	trap_AddCommand ("stats");
	trap_AddCommand ("teamtask");
	trap_AddCommand ("loaddefered");	// spelled wrong, but not changing for demo
}
