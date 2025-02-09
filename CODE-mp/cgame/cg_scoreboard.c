// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"
#include "../ui/ui_shared.h"

#define TEH_CTF_SCOREBOARD

#define	SCOREBOARD_X		(0)

#define SB_HEADER			86
#define SB_TOP				(SB_HEADER+32)

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR		420

#define SB_NORMAL_HEIGHT	25
#define SB_INTER_HEIGHT		15 // interleaved height

#ifdef TEH_CTF_SCOREBOARD
#define SB_MAXCLIENTS_NORMAL  0
//((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER   ((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

// Used when interleaved



#define SB_LEFT_BOTICON_X	(SCOREBOARD_X+0)
#define SB_LEFT_HEAD_X		(SCOREBOARD_X+32)
#define SB_RIGHT_BOTICON_X	(SCOREBOARD_X+64)
#define SB_RIGHT_HEAD_X		(SCOREBOARD_X+96)
// Normal
#define SB_BOTICON_X_CONST (SCOREBOARD_X+32)
#define SB_HEAD_X_CONST (SCOREBOARD_X+64)
static int SB_BOTICON_X = (SCOREBOARD_X+32);
static int SB_HEAD_X = (SCOREBOARD_X+64);

static int SB_SCORELINE_X = 100;
#define SB_SCORELINE_X_CONST		100
#define SB_SCORELINE_WIDTH_CONST	(640 - SB_SCORELINE_X_CONST * 2)
static int SB_SCORELINE_WIDTH = (640 - SB_SCORELINE_X_CONST * 2);

#define SB_RATING_WIDTH	    0 // (6 * BIGCHAR_WIDTH)
/*static int SB_NAME_X = (SB_SCORELINE_X_CONST);
static int SB_SCORE_X = (SB_SCORELINE_X_CONST + .55 * SB_SCORELINE_WIDTH_CONST);
static int SB_PING_X = (SB_SCORELINE_X_CONST + .75 * SB_SCORELINE_WIDTH_CONST);
static int SB_TIME_X = (SB_SCORELINE_X_CONST + .90 * SB_SCORELINE_WIDTH_CONST);*/

static int SB_TIME_X = (SB_SCORELINE_X_CONST + .07 * SB_SCORELINE_WIDTH_CONST);
static int SB_PING_X = (SB_SCORELINE_X_CONST + .22 * SB_SCORELINE_WIDTH_CONST);
static int SB_SCORE_X = (SB_SCORELINE_X_CONST + .30 * SB_SCORELINE_WIDTH_CONST);
static int SB_NAME_X = (SB_SCORELINE_X_CONST + .48 * SB_SCORELINE_WIDTH_CONST);
#else
#define SB_MAXCLIENTS_NORMAL  ((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER   ((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

// Used when interleaved



#define SB_LEFT_BOTICON_X	(SCOREBOARD_X+0)
#define SB_LEFT_HEAD_X		(SCOREBOARD_X+32)
#define SB_RIGHT_BOTICON_X	(SCOREBOARD_X+64)
#define SB_RIGHT_HEAD_X		(SCOREBOARD_X+96)
// Normal
#define SB_BOTICON_X		(SCOREBOARD_X+32)
#define SB_HEAD_X			(SCOREBOARD_X+64)

#define SB_SCORELINE_X		100
#define SB_SCORELINE_WIDTH	(640 - SB_SCORELINE_X * 2)

#define SB_RATING_WIDTH	    0 // (6 * BIGCHAR_WIDTH)
#define SB_NAME_X			(SB_SCORELINE_X)
#define SB_SCORE_X			(SB_SCORELINE_X + .55 * SB_SCORELINE_WIDTH)
#define SB_PING_X			(SB_SCORELINE_X + .70 * SB_SCORELINE_WIDTH)
#define SB_TIME_X			(SB_SCORELINE_X + .85 * SB_SCORELINE_WIDTH)
#endif

// The new and improved score board
//
// In cases where the number of clients is high, the score board heads are interleaved
// here's the layout

//
//	0   32   80  112  144   240  320  400   <-- pixel position
//  bot head bot head score ping time name
//  
//  wins/losses are drawn on bot icon now

static qboolean localClient; // true if local client has been displayed


							 /*
=================
CG_DrawScoreboard
=================
*/
static void CG_DrawClientScore( int y, score_t *score, float *color, float fade, qboolean largeFormat ) 
{
	//vec3_t	headAngles;
	clientInfo_t	*ci;
	int iconx, headx;
	float		scale;

	if ( largeFormat )
	{
		scale = 1.0f;
	}
	else
	{
		scale = 0.75f;
	}

	if ( score->client < 0 || score->client >= cgs.maxclients ) {
		Com_Printf( "Bad score->client: %i\n", score->client );
		return;
	}
	
	ci = &cgs.clientinfo[score->client];

	iconx = SB_BOTICON_X + (SB_RATING_WIDTH / 2);
	headx = SB_HEAD_X + (SB_RATING_WIDTH / 2);

	// draw the handicap or bot skill marker (unless player has flag)
	if ( ci->powerups & ( 1 << PW_NEUTRALFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32*cgs.widthRatioCoef, 32, TEAM_FREE, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 16*cgs.widthRatioCoef, 16, TEAM_FREE, qfalse );
		}
	} else if ( ci->powerups & ( 1 << PW_REDFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx*cgs.screenXScale, y*cgs.screenYScale, 32*cgs.screenXScale*cgs.widthRatioCoef, 32*cgs.screenYScale, TEAM_RED, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx*cgs.screenXScale, y*cgs.screenYScale, 32*cgs.screenXScale*cgs.widthRatioCoef, 32*cgs.screenYScale, TEAM_RED, qfalse );
		}
	} else if ( ci->powerups & ( 1 << PW_BLUEFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx*cgs.screenXScale, y*cgs.screenYScale, 32*cgs.screenXScale*cgs.widthRatioCoef, 32*cgs.screenYScale, TEAM_BLUE, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx*cgs.screenXScale, y*cgs.screenYScale, 32*cgs.screenXScale*cgs.widthRatioCoef, 32*cgs.screenYScale, TEAM_BLUE, qfalse );
		}
	} else {
		// draw the wins / losses
		/*
		if ( cgs.gametype == GT_TOURNAMENT ) 
		{
			CG_DrawSmallStringColor( iconx, y + SMALLCHAR_HEIGHT/2, va("%i/%i", ci->wins, ci->losses ), color );
		}
		*/
		//rww - in duel, we now show wins/losses in place of "frags". This is because duel now defaults to 1 kill per round.
	}

	// highlight your position
	if ( score->client == cg.snap->ps.clientNum ) 
	{
		float	hcolor[4];
		int		rank;

		localClient = qtrue;

		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR 
			|| cgs.gametype >= GT_TEAM ) {
			rank = -1;
		} else {
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		}
		if ( rank == 0 ) {
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;
		} else if ( rank == 1 ) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;
		} else if ( rank == 2 ) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;
		} else {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0.7f;
		}

#ifndef TEH_CTF_SCOREBOARD
		hcolor[3] = fade * 0.7f;
		CG_FillRect( SB_SCORELINE_X - 5, y + 2, 640 - SB_SCORELINE_X * 2 + 10, largeFormat?SB_NORMAL_HEIGHT:SB_INTER_HEIGHT, hcolor );
#else
		hcolor[3] = fade * 0.4f;
		CG_FillRect( SB_SCORELINE_X - 5, y + 2, SB_SCORELINE_WIDTH + 10, largeFormat?SB_NORMAL_HEIGHT:SB_INTER_HEIGHT, hcolor );
#endif
	}

#ifdef TEH_CTF_SCOREBOARD
	CG_Text_Paint (SB_NAME_X, y, 1.0f * scale, colorWhite, ci->name,0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
#else
	CG_Text_Paint (SB_NAME_X, y, 0.9f * scale, colorWhite, ci->name,0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
#endif

	if ( ci->team != TEAM_SPECTATOR || cgs.gametype == GT_TOURNAMENT )
	{
		if (cgs.gametype == GT_TOURNAMENT)
		{
#ifdef TEH_CTF_SCOREBOARD
			CG_Text_Paint (SB_SCORE_X - CG_Text_Width( va("%i/%i", ci->wins, ci->losses), 1.0f * scale, FONT_SMALL ) / 2, y, 1.0f * scale, colorWhite, va("%i/%i", ci->wins, ci->losses),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
#else
			CG_Text_Paint (SB_SCORE_X, y, 1.0f * scale, colorWhite, va("%i/%i", ci->wins, ci->losses),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
#endif
		}
		else
		{
#ifdef TEH_CTF_SCOREBOARD
			CG_Text_Paint (SB_SCORE_X - CG_Text_Width( va("%i", score->score), 1.0f * scale, FONT_SMALL ) / 2, y, 1.0f * scale, colorWhite, va("%i", score->score),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
#else
			CG_Text_Paint (SB_SCORE_X, y, 1.0f * scale, colorWhite, va("%i", score->score),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
#endif
		}
	}

#ifdef TEH_CTF_SCOREBOARD
	CG_Text_Paint (SB_PING_X - CG_Text_Width( va("%i", score->ping), 1.0f * scale, FONT_SMALL ) / 2, y, 1.0f * scale, colorWhite, va("%i", score->ping),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	CG_Text_Paint (SB_TIME_X - CG_Text_Width( va("%i", score->time), 1.0f * scale, FONT_SMALL ) / 2, y, 1.0f * scale, colorWhite, va("%i", score->time),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
#else
	CG_Text_Paint (SB_PING_X, y, 1.0f * scale, colorWhite, va("%i", score->ping),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	CG_Text_Paint (SB_TIME_X, y, 1.0f * scale, colorWhite, va("%i", score->time),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
#endif

	// add the "ready" marker for intermission exiting
	if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << score->client ) ) {
#ifdef TEH_CTF_SCOREBOARD
		if( SB_SCORELINE_X < 320 ) {
			CG_Text_Paint (SB_SCORELINE_X - CG_Text_Width( CG_GetStripEdString("MP_INGAME", "READY"), 0.7f * scale, FONT_MEDIUM ) - 10, y + 2, 0.7f * scale, colorWhite, CG_GetStripEdString("INGAMETEXT", "READY"),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
		} else {
			CG_Text_Paint (SB_SCORELINE_X + SB_SCORELINE_WIDTH + 10, y + 2, 0.7f * scale, colorWhite, CG_GetStripEdString("INGAMETEXT", "READY"),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
		}
#else
		CG_Text_Paint (SB_NAME_X - 64, y + 2, 0.7f * scale, colorWhite, CG_GetStripEdString("INGAMETEXT", "READY"),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
#endif
	}
}

/*
=================
CG_TeamScoreboard
=================
*/
static int CG_TeamScoreboard( int y, team_t team, float fade, int maxClients, int lineHeight, qboolean countOnly ) 
{
	int		i;
	score_t	*score;
	float	color[4];
	int		count;
	clientInfo_t	*ci;
#ifdef TEH_CTF_SCOREBOARD
	int		avgping = 0;
	float	scale;
#endif

	color[0] = color[1] = color[2] = 1.0;
	color[3] = fade;

#ifdef TEH_CTF_SCOREBOARD
	if ( lineHeight == SB_NORMAL_HEIGHT )
	{
		scale = 1.0f;
	}
	else
	{
		scale = 0.75f;
	}
#endif

	count = 0;
#ifdef TEH_CTF_SCOREBOARD
	if( cgs.gametype == GT_CTF )
	{
		if( team == TEAM_RED ) { //red team on left
			if( !countOnly ) {
				CG_Text_Paint ((SB_NAME_X + SB_SCORELINE_X + SB_SCORELINE_WIDTH ) / 2 - CG_Text_Width( "^1Red Team", 1.0f * scale, FONT_SMALL ) / 2, y-3, 1.0f * scale, colorWhite, "^1Red Team",0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
				CG_Text_Paint (SB_SCORE_X - CG_Text_Width( va("%i", cg.teamScores[0]), 1.0f * scale, FONT_SMALL ) / 2, y-3, 1.0f * scale, colorWhite, va("%i", cg.teamScores[0]),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
			}
			count++;
			maxClients++;
		} else if( team == TEAM_BLUE ) {
			if( !countOnly ) {
				CG_Text_Paint ((SB_NAME_X + SB_SCORELINE_X  + SB_SCORELINE_WIDTH ) / 2 - CG_Text_Width( "^4Blue Team", 1.0f * scale, FONT_SMALL ) / 2, y-3, 1.0f * scale, colorWhite, "^4Blue Team",0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
				CG_Text_Paint (SB_SCORE_X - CG_Text_Width( va("%i", cg.teamScores[1]), 1.0f * scale, FONT_SMALL ) / 2, y-3, 1.0f * scale, colorWhite, va("%i", cg.teamScores[1]),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
			}
			count++;
			maxClients++;
		}
	}
#endif

	for ( i = 0 ; i < cg.numScores && count < maxClients ; i++ ) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if ( team != ci->team ) {
			continue;
		}

		if ( !countOnly )
		{
			CG_DrawClientScore( y + lineHeight * count, score, color, fade, lineHeight == SB_NORMAL_HEIGHT );
#ifdef TEH_CTF_SCOREBOARD
			if( score->ping != -1 ) avgping += score->ping;
#endif
		}

		count++;
	}

#ifdef TEH_CTF_SCOREBOARD
	if( !countOnly && cgs.gametype == GT_CTF && (team == TEAM_RED || team == TEAM_BLUE ) ) {
		if( count-1 )avgping /= count-1;
		CG_Text_Paint (SB_PING_X - CG_Text_Width( va("%i", avgping), 1.0f * scale, FONT_SMALL ) / 2, y-3, 1.0f * scale, colorWhite, va("%i", avgping),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}
#endif
	return count;
}

int CG_GetTeamCount(team_t team, int maxClients)
{
	int i = 0;
	int count = 0;
	clientInfo_t	*ci;
	score_t	*score;

	for ( i = 0 ; i < cg.numScores && count < maxClients ; i++ )
	{
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if ( team != ci->team )
		{
			continue;
		}

		count++;
	}

	return count;
}
/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawOldScoreboard( void ) {
	int		x, y, w, i, n1, n2;
	float	fade;
	float	*fadeColor;
	char	*s;
	int maxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;
	float	mySBScale;

	// don't draw amuthing if the menu or console is up
	if ( cg_paused.integer ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	if ( demo15detected && cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	qboolean isDead = cam_specEnt.integer == -1 ? cg.predictedPlayerState.pm_type == PM_DEAD : cg_entities[cam_specEnt.integer].currentState.eFlags & EF_DEAD;
	if ( cg.showScores || isDead ||
		 cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );
		
		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}

	mySBScale = 1.0f;
	SB_SCORELINE_WIDTH = SB_SCORELINE_WIDTH_CONST;
	SB_SCORELINE_X = SB_SCORELINE_X_CONST;
	SB_BOTICON_X = SB_BOTICON_X_CONST;
	SB_HEAD_X = SB_HEAD_X_CONST;
	if ( cgs.gametype == GT_CTF ) {
		SB_SCORELINE_X /= 2;
		SB_SCORELINE_WIDTH = (SB_SCORELINE_WIDTH + SB_SCORELINE_X) / 2;
		SB_BOTICON_X /= 2;
		SB_HEAD_X /= 2;
		mySBScale = 0.75f;
	}

	// fragged by ... line
	// or if in intermission and duel, prints the winner of the duel round
	if (cgs.gametype == GT_TOURNAMENT && cgs.duelWinner != -1 &&
		cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		s = va("%s^7 %s", cgs.clientinfo[cgs.duelWinner].name, CG_GetStripEdString("INGAMETEXT", "DUEL_WINS") );
		/*w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 40;
		CG_DrawBigString( x, y, s, fade );
		*/
		x = ( SCREEN_WIDTH ) / 2;
		y = 40;
		CG_Text_Paint ( x - CG_Text_Width ( s, 1.0f, FONT_MEDIUM ) / 2, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}
	else if (cgs.gametype == GT_TOURNAMENT && cgs.duelist1 != -1 && cgs.duelist2 != -1 &&
		cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		s = va("%s^7 %s %s", cgs.clientinfo[cgs.duelist1].name, CG_GetStripEdString("INGAMETEXT", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name );
		/*w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 40;
		CG_DrawBigString( x, y, s, fade );
		*/
		x = ( SCREEN_WIDTH ) / 2;
		y = 40;
		CG_Text_Paint ( x - CG_Text_Width ( s, 1.0f, FONT_MEDIUM ) / 2, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}
	else if ( cg.killerName[0] ) {
		s = va("%s %s", CG_GetStripEdString("INGAMETEXT", "KILLEDBY"), cg.killerName );
		/*w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 40;
		CG_DrawBigString( x, y, s, fade );
		*/
		x = ( SCREEN_WIDTH ) / 2;
		y = 40;
		CG_Text_Paint ( x - CG_Text_Width ( s, 1.0f, FONT_MEDIUM ) / 2, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}

	// current rank
	if ( cgs.gametype < GT_TEAM) {
		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) 
		{
			char sPlace[256];
			char sOf[256];
			char sWith[256];

			trap_SP_GetStringTextString("INGAMETEXT_PLACE",	sPlace,	sizeof(sPlace));
			trap_SP_GetStringTextString("INGAMETEXT_OF",	sOf,	sizeof(sOf));
			trap_SP_GetStringTextString("INGAMETEXT_WITH",	sWith,	sizeof(sWith));

			s = va("%s %s (%s %i) %s %i",
				CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),
				sPlace,
				sOf,
				cg.numScores,
				sWith,
				cg.snap->ps.persistant[PERS_SCORE] );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
			x = ( SCREEN_WIDTH ) / 2;
			y = 60;
			//CG_DrawBigString( x, y, s, fade );
			UI_DrawProportionalString(x, y, s, UI_CENTER|UI_DROPSHADOW, colorTable[CT_WHITE]);
		}
	} else {
		if ( cg.teamScores[0] == cg.teamScores[1] ) {
			s = va("Teams are tied at %i", cg.teamScores[0] );
		} else if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			s = va("Red leads %i to %i",cg.teamScores[0], cg.teamScores[1] );
		} else {
			s = va("Blue leads %i to %i",cg.teamScores[1], cg.teamScores[0] );
		}

		x = ( SCREEN_WIDTH ) / 2;
		y = 60;
		
#ifdef TEH_CTF_SCOREBOARD
		CG_Text_Paint ( SB_SCORELINE_X, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
#else
		CG_Text_Paint ( x - CG_Text_Width ( s, 1.0f, FONT_MEDIUM ) / 2, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
#endif
	}

	// scoreboard
#ifdef TEH_CTF_SCOREBOARD
	//y = SB_HEADER + (int)(32.0f - (32.0f * mySBScale ) )*2;
	y = SB_TOP - CG_Text_Height( 0, FONT_SMALL, mySBScale ) - 5;
	
	/*SB_NAME_X = (SB_SCORELINE_X);
	SB_SCORE_X = (SB_SCORELINE_X + .55 * SB_SCORELINE_WIDTH);
	SB_PING_X = (SB_SCORELINE_X + .75 * SB_SCORELINE_WIDTH);
	SB_TIME_X = (SB_SCORELINE_X + .90 * SB_SCORELINE_WIDTH);*/
	
	SB_TIME_X = (SB_SCORELINE_X + .05 * SB_SCORELINE_WIDTH);
	SB_PING_X = (SB_SCORELINE_X + .20 * SB_SCORELINE_WIDTH);
	SB_SCORE_X = (SB_SCORELINE_X + .37 * SB_SCORELINE_WIDTH);
	SB_NAME_X = (SB_SCORELINE_X + .50 * SB_SCORELINE_WIDTH);

	//CG_DrawPic ( SB_SCORELINE_X - 40, y - 5, SB_SCORELINE_WIDTH + 80, 40, trap_R_RegisterShaderNoMip ( "gfx/menus/menu_buttonback.tga" ) );

	CG_Text_Paint ( SB_NAME_X, y, mySBScale, colorWhite, "Name",0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	if (cgs.gametype == GT_TOURNAMENT)
	{
		char sWL[100];
		trap_SP_GetStringTextString("MP_INGAME_W_L", sWL,	sizeof(sWL));

		CG_Text_Paint ( SB_SCORE_X, y, mySBScale, colorWhite, sWL, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}
	else
	{
		CG_Text_Paint ( SB_SCORE_X - CG_Text_Width( "Score", mySBScale, FONT_SMALL) / 2, y, mySBScale, colorWhite, "Score", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}
	CG_Text_Paint ( SB_PING_X - CG_Text_Width( "Ping", mySBScale, FONT_SMALL) / 2, y, mySBScale, colorWhite, "Ping", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	CG_Text_Paint ( SB_TIME_X - CG_Text_Width( "Time", mySBScale, FONT_SMALL) / 2, y, mySBScale, colorWhite, "Time", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
#else
	y = SB_HEADER;

	CG_DrawPic ( SB_SCORELINE_X - 40, y - 5, SB_SCORELINE_WIDTH + 80, 40, trap_R_RegisterShaderNoMip ( "gfx/menus/menu_buttonback.tga" ) );

	// "NAME", "SCORE", "PING", "TIME" weren't localised, GODDAMMIT!!!!!!!!     
	//
	// Unfortunately, since it's so sodding late now and post release I can't enable the localisation code (REM'd) since some of 
	//	the localised strings don't fit - since no-one's ever seen them to notice this.  Smegging brilliant. Thanks people.
	//
	CG_Text_Paint ( SB_NAME_X, y, 1.0f, colorWhite, /*CG_GetStripEdString("MENUS3", "NAME")*/"Name",0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	if (cgs.gametype == GT_TOURNAMENT)
	{
		char sWL[100];
		trap_SP_GetStringTextString("INGAMETEXT_W_L", sWL,	sizeof(sWL));

		CG_Text_Paint ( SB_SCORE_X, y, 1.0f, colorWhite, sWL, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}
	else
	{
		CG_Text_Paint ( SB_SCORE_X, y, 1.0f, colorWhite, /*CG_GetStripEdString("MENUS3", "SCORE")*/"Score", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}
	CG_Text_Paint ( SB_PING_X, y, 1.0f, colorWhite, /*CG_GetStripEdString("MENUS0", "PING")*/"Ping", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	CG_Text_Paint ( SB_TIME_X, y, 1.0f, colorWhite, /*CG_GetStripEdString("MENUS3", "TIME")*/"Time", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
#endif
	y = SB_TOP;

	// If there are more than SB_MAXCLIENTS_NORMAL, use the interleaved scores
	if ( cg.numScores > SB_MAXCLIENTS_NORMAL
#ifdef TEH_CTF_SCOREBOARD
		|| cgs.gametype == GT_CTF
#endif
		) {
		maxClients = SB_MAXCLIENTS_INTER;
		lineHeight = SB_INTER_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 16;
	} else {
		maxClients = SB_MAXCLIENTS_NORMAL;
		lineHeight = SB_NORMAL_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 8;
	}

	localClient = qfalse;


	//I guess this should end up being able to display 19 clients at once.
	//In a team game, if there are 9 or more clients on the team not in the lead,
	//we only want to show 10 of the clients on the team in the lead, so that we
	//have room to display the clients in the lead on the losing team.

	//I guess this can be accomplished simply by printing the first teams score with a maxClients
	//value passed in related to how many players are on both teams.
#ifdef TEH_CTF_SCOREBOARD
	if ( cgs.gametype == GT_CTF ) {
		//
		// teamplay scoreboard
		//
		int team1MaxCl = CG_GetTeamCount(TEAM_RED, maxClients);
		int team2MaxCl = CG_GetTeamCount(TEAM_BLUE, maxClients);
		vec4_t		hcolor;
		
		topBorderSize = 0;
		bottomBorderSize = 4;
		
		//y += lineHeight/2;

		n1 = CG_TeamScoreboard( y, TEAM_RED, fade, team1MaxCl, lineHeight, qtrue );
		
		// time
		CG_DrawTeamBackground( SB_SCORELINE_X - 3, y + lineHeight, (SB_PING_X + SB_TIME_X) / 2 - (SB_SCORELINE_X - 3) - 1, (n1 - 1) * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
		// ping
		CG_DrawTeamBackground( (SB_PING_X + SB_TIME_X) / 2 + 1, y - topBorderSize, (SB_PING_X + SB_SCORE_X) / 2 - ((SB_PING_X + SB_TIME_X) / 2 + 1) - 1, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
		// score
		CG_DrawTeamBackground( (SB_SCORE_X + SB_PING_X) / 2 + 1, y - topBorderSize, (SB_NAME_X - 9) - ((SB_SCORE_X + SB_PING_X) / 2 + 1) - 1, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
		// name
		//CG_DrawTeamBackground( SB_NAME_X - 7, y + lineHeight, (SB_SCORELINE_WIDTH + SB_SCORELINE_X + 7) - (SB_NAME_X - 5) - 1, (n1 - 1) * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
		
		CG_TeamScoreboard( y, TEAM_RED, fade, team1MaxCl, lineHeight, qfalse );
		hcolor[0] = 0.66f;
		hcolor[1] = 0.66f;
		hcolor[2] = 0.66f;
		hcolor[3] = 1.0f;
		CG_FillRect( SB_SCORELINE_X - 5, y + lineHeight, SB_SCORELINE_WIDTH + 10, 1, hcolor );

		maxClients -= team1MaxCl;
		y = ((27 - CG_GetTeamCount(TEAM_SPECTATOR, maxClients)) * lineHeight) + BIGCHAR_HEIGHT;
		y -= 11;
		
		n1 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients, lineHeight, qfalse );
		//y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		
		SB_SCORELINE_X = 320 + (SB_SCORELINE_X / 2);
		/*SB_NAME_X = (SB_SCORELINE_X);
		SB_SCORE_X = (SB_SCORELINE_X + .55 * SB_SCORELINE_WIDTH);
		SB_PING_X = (SB_SCORELINE_X + .75 * SB_SCORELINE_WIDTH);
		SB_TIME_X = (SB_SCORELINE_X + .90 * SB_SCORELINE_WIDTH);*/
		SB_TIME_X = (SB_SCORELINE_X + .05 * SB_SCORELINE_WIDTH);
		SB_PING_X = (SB_SCORELINE_X + .20 * SB_SCORELINE_WIDTH);
		SB_SCORE_X = (SB_SCORELINE_X + .37 * SB_SCORELINE_WIDTH);
		SB_NAME_X = (SB_SCORELINE_X + .50 * SB_SCORELINE_WIDTH);
		
		SB_BOTICON_X = 640 - 32 - SB_BOTICON_X;
		SB_HEAD_X = 640 - 32 - SB_BOTICON_X;
		
		y = SB_TOP - CG_Text_Height( 0, FONT_SMALL, mySBScale ) - 5;
	
		//CG_DrawPic ( SB_SCORELINE_X - 40, y - 5, SB_SCORELINE_WIDTH + 80, 40, trap_R_RegisterShaderNoMip ( "gfx/menus/menu_buttonback.tga" ) );
	
		CG_Text_Paint ( SB_NAME_X, y, mySBScale, colorWhite, "Name",0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		CG_Text_Paint ( SB_SCORE_X - CG_Text_Width( "Score", mySBScale, FONT_SMALL) / 2, y, mySBScale, colorWhite, "Score", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		CG_Text_Paint ( SB_PING_X - CG_Text_Width( "Ping", mySBScale, FONT_SMALL) / 2, y, mySBScale, colorWhite, "Ping", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		CG_Text_Paint ( SB_TIME_X - CG_Text_Width( "Time", mySBScale, FONT_SMALL) / 2, y, mySBScale, colorWhite, "Time", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		
		y = SB_TOP;
		
		n2 = CG_TeamScoreboard( y, TEAM_BLUE, fade, team2MaxCl, lineHeight, qtrue );
		
		// time
		CG_DrawTeamBackground( SB_SCORELINE_X - 3, y + lineHeight, (SB_PING_X + SB_TIME_X) / 2 - (SB_SCORELINE_X - 3) - 1, (n2 - 1) * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
		// ping
		CG_DrawTeamBackground( (SB_PING_X + SB_TIME_X) / 2 + 1, y - topBorderSize, (SB_PING_X + SB_SCORE_X) / 2 - ((SB_PING_X + SB_TIME_X) / 2 + 1) - 1, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
		// score
		CG_DrawTeamBackground( (SB_SCORE_X + SB_PING_X) / 2 + 1, y - topBorderSize, (SB_NAME_X - 9) - ((SB_SCORE_X + SB_PING_X) / 2 + 1) - 1, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
		// name
		//CG_DrawTeamBackground( SB_NAME_X - 7, y + lineHeight, (SB_SCORELINE_WIDTH + SB_SCORELINE_X + 7) - (SB_NAME_X - 5) - 1, (n2 - 1) * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
		
		CG_TeamScoreboard( y, TEAM_BLUE, fade, team2MaxCl, lineHeight, qfalse );
		hcolor[0] = 0.66f;
		hcolor[1] = 0.66f;
		hcolor[2] = 0.66f;
		hcolor[3] = 1.0f;
		CG_FillRect( SB_SCORELINE_X - 5, y + lineHeight, SB_SCORELINE_WIDTH + 10, 1, hcolor );
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
		
	} else
#endif
	if ( cgs.gametype >= GT_TEAM ) {
		//
		// teamplay scoreboard
		//
		y += lineHeight/2;

		if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			int team1MaxCl = CG_GetTeamCount(TEAM_RED, maxClients);
			int team2MaxCl = CG_GetTeamCount(TEAM_BLUE, maxClients);

			if (team1MaxCl > 10 && (team1MaxCl+team2MaxCl) > maxClients)
			{
				team1MaxCl -= team2MaxCl;
				//subtract as many as you have to down to 10, once we get there
				//we just set it to 10

				if (team1MaxCl < 10)
				{
					team1MaxCl = 10;
				}
			}

			team2MaxCl = (maxClients-team1MaxCl); //team2 can display however many is left over after team1's display

			n1 = CG_TeamScoreboard( y, TEAM_RED, fade, team1MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, 640 - SB_SCORELINE_X * 2 + 10, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			CG_TeamScoreboard( y, TEAM_RED, fade, team1MaxCl, lineHeight, qfalse );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

			//maxClients -= n1;

			n2 = CG_TeamScoreboard( y, TEAM_BLUE, fade, team2MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, 640 - SB_SCORELINE_X * 2 + 10, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			CG_TeamScoreboard( y, TEAM_BLUE, fade, team2MaxCl, lineHeight, qfalse );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;

			//maxClients -= n2;

			maxClients -= (team1MaxCl+team2MaxCl);
		} else {
			int team1MaxCl = CG_GetTeamCount(TEAM_BLUE, maxClients);
			int team2MaxCl = CG_GetTeamCount(TEAM_RED, maxClients);

			if (team1MaxCl > 10 && (team1MaxCl+team2MaxCl) > maxClients)
			{
				team1MaxCl -= team2MaxCl;
				//subtract as many as you have to down to 10, once we get there
				//we just set it to 10

				if (team1MaxCl < 10)
				{
					team1MaxCl = 10;
				}
			}

			team2MaxCl = (maxClients-team1MaxCl); //team2 can display however many is left over after team1's display

			n1 = CG_TeamScoreboard( y, TEAM_BLUE, fade, team1MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, 640 - SB_SCORELINE_X * 2 + 10, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			CG_TeamScoreboard( y, TEAM_BLUE, fade, team1MaxCl, lineHeight, qfalse );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

			//maxClients -= n1;

			n2 = CG_TeamScoreboard( y, TEAM_RED, fade, team2MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, 640 - SB_SCORELINE_X * 2 + 10, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			CG_TeamScoreboard( y, TEAM_RED, fade, team2MaxCl, lineHeight, qfalse );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;

			//maxClients -= n2;

			maxClients -= (team1MaxCl+team2MaxCl);
		}
		n1 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients, lineHeight, qfalse );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

	} else {
		//
		// free for all scoreboard
		//
		n1 = CG_TeamScoreboard( y, TEAM_FREE, fade, maxClients, lineHeight, qfalse );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		n2 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight, qfalse );
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
	}

	if (!localClient) {
		// draw local client at the bottom
		for ( i = 0 ; i < cg.numScores ; i++ ) {
			if ( cg.scores[i].client == cg.snap->ps.clientNum ) {
				CG_DrawClientScore( y, &cg.scores[i], fadeColor, fade, lineHeight == SB_NORMAL_HEIGHT );
				break;
			}
		}
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
}

//================================================================================

