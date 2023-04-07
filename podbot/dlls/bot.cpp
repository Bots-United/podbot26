// ####################################
// #                                  #
// #       Ping of Death - Bot        #
// #                by                #
// #    Markus Klinge aka Count Floyd #
// #                                  #
// ####################################
//
// Started from the HPB-Bot Alpha Source
// by Botman so Credits for a lot of the basic
// HL Server/Client Stuff goes to him
//
// bot.cpp
//
//
  
#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "log.h"
#include "waypoint.h"
#include "bot_weapons.h"
#include "bot_chat.h"
#include "bot_globals.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifndef __linux__
extern HINSTANCE h_Library;
#else
extern void *h_Library;
#endif


// Nice little Insult to appear in the DLL &
// scare off patching Lamers...
char szCrackerNote[]={
"So you think you could just patch some ASCII Values "
"to fake your own Bot ? FUCK YOU LAMER!"};


extern bot_weapon_select_t cs_weapon_select[NUM_WEAPONS+1];
extern PATH *paths[MAX_WAYPOINTS];
extern bot_weapon_t weapon_defs[MAX_WEAPONS]; // array of weapon definitions
extern unsigned short FixedUnsigned16( float value, float scale );
extern int *g_pFloydPathMatrix;
extern int *g_pWithHostageDistMatrix;
extern int *g_pWithHostagePathMatrix;

static FILE *fp;

#define PLAYER_SEARCH_RADIUS 40.0

bot_t bots[MAXBOTSNUMBER];   // max of 32 bots in a game

// These are skill based Delays for an Enemy Surprise Delay
// and the Pause/Camping Delays (weak Bots are longer surprised and
// do Pause/Camp longer as well)
skilldelay_t BotSkillDelays[6] = {
{0.8,1.0,9,30.0,60.0},{0.6,0.8,8,25.0,55.0},
{0.4,0.6,7,20.0,50.0},{0.2,0.3,6,15.0,45.0},
{0.1,0.2,5,10.0,35.0},{0.0,0.1,3,10.0,20.0}};


// Max Speed for Bots carrying that Weapon
// Not needed anymore since I use the Engine
// to set the speeds
/*weapon_movespeed_t WeaponMaxSpeed[]={
	{CS_WEAPON_P228,250.0},
	{CS_WEAPON_SCOUT,260.0},
	{CS_WEAPON_HEGRENADE,260.0},
	{CS_WEAPON_XM1014,240.0},
	{CS_WEAPON_C4,250.0},
	{CS_WEAPON_MAC10,250.0},
	{CS_WEAPON_AUG,240.0},
	{CS_WEAPON_SMOKEGRENADE,260.0},
	{CS_WEAPON_ELITE,250.0},
	{CS_WEAPON_USP,250.0},
	{CS_WEAPON_GLOCK18,250.0},
	{CS_WEAPON_AWP,210.0},
	{CS_WEAPON_MP5NAVY,250.0},
	{CS_WEAPON_M249,220.0},
	{CS_WEAPON_M3,230.0},
	{CS_WEAPON_M4A1,230.0},
	{CS_WEAPON_TMP,250.0},
	{CS_WEAPON_G3SG1,200.0},
	{CS_WEAPON_FLASHBANG,260.0},
	{CS_WEAPON_DEAGLE,250.0},
	{CS_WEAPON_SG552,235.0},
	{CS_WEAPON_AK47,221.0},
	{CS_WEAPON_KNIFE,260.0},
	{CS_WEAPON_P90,250.0},
	{CS_WEAPON_UMP45,250.0},
	{CS_WEAPON_FIVESEVEN,250.0},
	{CS_WEAPON_SG550,210.0},
	{0,0.0}};*/

// Table with all available Actions for the Bots
// (filtered in & out in BotSetConditions)
// Some of them have subactions included

bottask_t taskFilters[] =
{
	{NULL,NULL,TASK_NORMAL,0.0,-1,TRUE,0.0},
	{NULL,NULL,TASK_PAUSE,0.0,-1,FALSE,0.0},
	{NULL,NULL,TASK_MOVETOPOSITION,0.0,-1,TRUE,0.0},
	{NULL,NULL,TASK_FOLLOWUSER,0.0,-1,TRUE,0.0},
	{NULL,NULL,TASK_WAITFORGO,0.0,-1,TRUE,0.0},
	{NULL,NULL,TASK_PICKUPITEM,0.0,-1,TRUE,0.0},
	{NULL,NULL,TASK_CAMP,0.0,-1,TRUE,0.0},
	{NULL,NULL,TASK_PLANTBOMB,0.0,-1,FALSE,0.0},
	{NULL,NULL,TASK_DEFUSEBOMB,0.0,-1,FALSE,0.0},
	{NULL,NULL,TASK_ATTACK,0.0,-1,FALSE,0.0},
	{NULL,NULL,TASK_ENEMYHUNT,0.0,-1,FALSE,0.0},
	{NULL,NULL,TASK_SEEKCOVER,0.0,-1,FALSE,0.0},
	{NULL,NULL,TASK_THROWHEGRENADE,0.0,-1,FALSE,0.0},
	{NULL,NULL,TASK_THROWFLASHBANG,0.0,-1,FALSE,0.0},
	{NULL,NULL,TASK_THROWSMOKEGRENADE,0.0,-1,FALSE,0.0},
	{NULL,NULL,TASK_SHOOTBREAKABLE,0.0,-1,FALSE,0.0},
	{NULL,NULL,TASK_HIDE,0.0,-1,FALSE,0.0},
	{NULL,NULL,TASK_BLINDED,0.0,-1,FALSE,0.0},
	{NULL,NULL,TASK_SPRAYLOGO,0.0,-1,FALSE,0.0}
};

// Holds the Position of the planted Bomb
Vector g_vecBomb;

// Ptr to Hosting Edict
extern edict_t *pHostEdict;


inline edict_t *CREATE_FAKE_CLIENT( const char *netname )
{
   return (*g_engfuncs.pfnCreateFakeClient)( netname );
}

inline char *GET_INFOBUFFER( edict_t *e )
{
   return (*g_engfuncs.pfnGetInfoKeyBuffer)( e );
}

inline char *GET_INFO_KEY_VALUE( char *infobuffer, char *key )
{
   return (g_engfuncs.pfnInfoKeyValue( infobuffer, key ));
}

inline void SET_CLIENT_KEY_VALUE( int clientIndex, char *infobuffer,
                                  char *key, char *value )
{
   (*g_engfuncs.pfnSetClientKeyValue)( clientIndex, infobuffer, key, value );
}


// this is the LINK_ENTITY_TO_CLASS function that creates a player (bot)
void player( entvars_t *pev )
{
   static LINK_ENTITY_FUNC otherClassName = NULL;
   if (otherClassName == NULL)
      otherClassName = (LINK_ENTITY_FUNC)GetProcAddress(h_Library, "player");
   if (otherClassName != NULL)
   {
      (*otherClassName)(pev);
   }
}


// Initialises a Bot after Creation & at the start of each
// Round
void BotSpawnInit( bot_t *pBot) 
{
#ifdef DEBUG
	report_log (LOG_BOTBASE, "BotSpawnInit","name=%s",pBot->name);
#endif
	int i;

	// Delete all allocated Path Nodes
	DeleteSearchNodes(pBot);

	pBot->wpt_origin = Vector(0, 0, 0);
	pBot->dest_origin = Vector(0, 0, 0);
	pBot->curr_wpt_index = -1;
	pBot->curr_travel_flags = 0;
	pBot->curr_move_flags = 0;
	pBot->vecDesiredVelocity = Vector(0,0,0);
	pBot->prev_goal_index = -1;
	pBot->chosengoal_index= -1;
	pBot->prev_wpt_index[0] = -1;
	pBot->prev_wpt_index[1] = -1;
	pBot->prev_wpt_index[2] = -1;
	pBot->prev_wpt_index[3] = -1;
	pBot->prev_wpt_index[4] = -1;
	pBot->f_wpt_timeset=gpGlobals->time;
	switch(pBot->bot_personality)
	{
		// Normal
	case 0:
		if(RANDOM_LONG(1,100) > 50)
			pBot->byPathType = 1;
		else
			pBot->byPathType = 2;
		break;
		// Psycho
	case 1:
		pBot->byPathType = 0;
		break;
		// Coward
	case 2:
		pBot->byPathType = 2;
		break;
	}

	// Clear all States & Tasks
	pBot->iStates=0;
	BotResetTasks(pBot);

	pBot->bCheckMyTeam = TRUE;


	// Reset Frame Calculation
	pBot->msecnum = 0;
	pBot->msecdel = 0.0;
	pBot->msecval = 0.0;
	
	pBot->bot_health = 100;
	
	pBot->bIsVIP = FALSE;
	pBot->bIsLeader = FALSE;
	pBot->fTimeTeamOrder = 0.0;

	pBot->fMinSpeed = 260.0;
	pBot->prev_speed = 0.0;  // fake "paused" since bot is NOT stuck
	pBot->v_prev_origin = Vector(9999.0, 9999.0, 9999.0);
	pBot->prev_time = gpGlobals->time;


	pBot->bCanDuck=FALSE;
	pBot->f_view_distance=4096.0;
	pBot->f_maxview_distance=4096.0;


//	pBot->bInMessageLoop=FALSE;	

	pBot->ladder_dir = 0;
	pBot->f_use_ladder_time = 0.0;
	pBot->pBotPickupItem = NULL;
	pBot->pItemIgnore = NULL;
	pBot->f_itemcheck_time = 0.0;


	pBot->pShootBreakable=NULL;
	pBot->vecBreakable = Vector(0,0,0);
	pBot->f_use_button_time=0.0;
	
	pBot->f_wall_check_time = 0.0;
	pBot->f_wall_on_right = 0.0;
	pBot->f_wall_on_left = 0.0;
	pBot->f_dont_avoid_wall_time = 0.0;

	BotResetCollideState(pBot);
	
	pBot->pBotEnemy = NULL;
	pBot->pLastVictim = NULL;
	pBot->pLastEnemy = NULL;
	pBot->vecLastEnemyOrigin = Vector(0,0,0);
	pBot->pTrackingEdict = NULL;
	pBot->fTimeNextTracking = 0.0;

	pBot->fEnemyUpdateTime = 0.0;
	pBot->f_bot_see_enemy_time = 0.0;
	pBot->f_shootatdead_time = 0.0;
	pBot->oldcombatdesire=0.0;

	pBot->pAvoidGrenade=NULL;
	pBot->cAvoidGrenade=0;

	// Reset Damage
	pBot->iLastDamageType = -1;

	pBot->iVoteKickIndex=0;
	pBot->iLastVoteKick=0;
	pBot->iVoteMap=0;
	
	pBot->vecPosition=Vector(0,0,0);

	pBot->f_ideal_reaction_time=BotSkillDelays[pBot->bot_skill/20].fMinSurpriseDelay;
	pBot->f_actual_reaction_time=BotSkillDelays[pBot->bot_skill/20].fMinSurpriseDelay;

	pBot->pBotUser = NULL;
	pBot->f_bot_use_time = 0.0;
	
	for(i=0;i<MAX_HOSTAGES;i++)
		pBot->pHostages[i]=NULL;

	pBot->f_hostage_check_time=gpGlobals->time;

	pBot->bIsReloading=FALSE;
	pBot->f_shoot_time = gpGlobals->time;
	pBot->fTimeLastFired = 0.0;
	pBot->iBurstShotsFired = 0;
	pBot->fTimeFirePause = 0.0;

	pBot->f_primary_charging = -1.0;
	pBot->f_secondary_charging = -1.0;

	pBot->f_grenade_check_time=0.0;
	pBot->bUsingGrenade = FALSE;
	
	pBot->charging_weapon_id = 0;
	
	pBot->f_blind_time = 0.0;
	pBot->f_jumptime=0.0;

	pBot->f_sound_update_time = gpGlobals->time;
	pBot->f_heard_sound_time = 0.0;
	
	pBot->fTimePrevThink = gpGlobals->time;
	pBot->fTimeFrameInterval = 0.0;

	pBot->f_bomb_time=0.0;

	pBot->b_lift_moving = FALSE;

	pBot->SaytextBuffer.fTimeNextChat = gpGlobals->time;
	pBot->SaytextBuffer.iEntityIndex = -1;
	pBot->SaytextBuffer.szSayText[0] = 0x0;

	// If Bot died, clear all Weapon Stuff and force buying again
	if(pBot->bDead)
	{
		memset(&(pBot->current_weapon), 0, sizeof(pBot->current_weapon));
		memset(&(pBot->m_rgAmmo), 0, sizeof(pBot->m_rgAmmo));
		pBot->iBuyCount=0;
		pBot->pSelected_weapon=NULL;
		pBot->bot_armor = 0;
	}
	// if not, check if he's got a primary weapon 
	else
	{
		if(BotHasPrimaryWeapon(pBot))
			pBot->iBuyCount=3;		// only buy ammo & items
		else
		{
			pBot->iBuyCount=0;
			pBot->pSelected_weapon=NULL;
		}
		pBot->pEdict->v.button |= IN_RELOAD;
	}
	pBot->f_zoomchecktime=0.0;
	pBot->iNumWeaponPickups=0;

	pBot->f_StrafeSetTime=0.0;
	pBot->byCombatStrafeDir=0;
	pBot->byFightStyle=0;
	pBot->f_lastfightstylecheck=0.0;

	pBot->bCheckWeaponSwitch=TRUE;
	pBot->bBuyingFinished=FALSE;
	pBot->bInBuyzone=FALSE;
	pBot->BuyAction=MSG_CS_IDLE;

	pBot->pRadioEntity=NULL;
	pBot->iRadioOrder=0;

	pBot->bLogoSprayed=FALSE;
	pBot->bDefendedBomb = FALSE;

	pBot->need_to_initialize = FALSE;
	pBot->f_spawn_time=gpGlobals->time;
	pBot->f_lastchattime=gpGlobals->time;
	pBot->pEdict->v.v_angle.y = pBot->pEdict->v.ideal_yaw;

	for(i=0;i<2;i++)
	{
		pBot->rgfYawHistory[i] = 0;
		pBot->rgfPitchHistory[i] = 0;
	}

	// Clear its Message Queue
	for(i=0;i<32;i++)
		pBot->aMessageQueue[i]=MSG_CS_IDLE;

	pBot->iActMessageIndex=0;
	pBot->iPushMessageIndex=0;
	// And put Buying into its Message Queue 
    BotPushMessageQueue(pBot,MSG_CS_BUY);
	bottask_t TempTask = {NULL,NULL,TASK_NORMAL,TASKPRI_NORMAL,-1,0.0,TRUE};
	BotPushTask(pBot,&TempTask);
}


// This Function creates the Fakeclient (Bot) - passed Arguments:
// pPlayer - ptr to Edict (or NULL) calling the routine
// arg1 - Skill
// arg2 - Team
// arg3 - Class (Model)
// arg4 - Botname
void BotCreate( edict_t *pPlayer, const char *arg1, const char *arg2, const char *arg3, const char *arg4)
{
	// If still creating - exit or HL will crash
	if(g_bBusyCreatingBot)
		return;
	g_bBusyCreatingBot=TRUE;
	edict_t *BotEnt;
	bot_t *pBot;
	char c_name[BOT_NAME_LEN+1];
	char c_skill[7];
	int skill;
	bool found = FALSE;
	int index;
	
#ifdef DEBUG
	report_log (LOG_BOTBASE, "BotCreate","arg1=%s arg2=%s arg3=%s arg4=%s",arg1,arg2,arg3,arg4);
#endif
	
	// Don't allow creating Bots when no waypoints are loaded
	if(g_iNumWaypoints<1)
	{
		if (pPlayer)
			UTIL_HostPrint(HUD_PRINTNOTIFY,"No Waypoints for this Map, can't create Bot!\n");
		else if (IS_DEDICATED_SERVER())
			printf("No Waypoints for this Map, can't create Bot!\n");
		botcreation_time=0.0;
		g_bBusyCreatingBot=FALSE;
		return;
	}
	// if Waypoints have changed don't allow it because Distance Tables are
	// messed up
	else if(g_bWaypointsChanged)
	{
		if (pPlayer)
			UTIL_HostPrint(HUD_PRINTNOTIFY,"Waypoints changed/not initialised, can't create Bot!\n");
		else if (IS_DEDICATED_SERVER())
			printf( "Waypoints changed/not initialised, can't create Bot!\n");
		botcreation_time=0.0;
		g_bBusyCreatingBot=FALSE;
		return;
	}


	// If Skill is given, assign it
	if(arg1 != NULL && *arg1!=0)
		skill=atoi(arg1);
	// else give random skill
	else
	{
		if(g_iMinBotSkill == g_iMaxBotSkill)
			skill = g_iMinBotSkill;
		else
			skill = RANDOM_LONG(g_iMinBotSkill,g_iMaxBotSkill);
	}
	// Skill safety check
	if(skill>100 || skill<0)
	{
		if (pPlayer)
			UTIL_HostPrint(HUD_PRINTNOTIFY, "Can't create Bot! No matching Skill !\n");
		g_bBusyCreatingBot=FALSE;
		return;
	}

	// Create Random Personality
	int iPersonality=RANDOM_LONG(0,2);

	// If No Name is given, do our name stuff
	if(arg4 == NULL || *arg4==0)
	{
		// If Detailnames are on, attach Clan Tag
		if(g_bDetailNames)
		{
			strcpy(c_name,"[POD]");
			switch(iPersonality)
			{
			case 1:
				c_name[2]='*';
				break;
			case 2:
				c_name[2]='0';
			}
		}
		
		// Rotate used Names Array up
		pUsedBotNames[7]=pUsedBotNames[6];
		pUsedBotNames[6]=pUsedBotNames[5];
		pUsedBotNames[5]=pUsedBotNames[4];
		pUsedBotNames[4]=pUsedBotNames[3];
		pUsedBotNames[3]=pUsedBotNames[2];
		pUsedBotNames[2]=pUsedBotNames[1];
		pUsedBotNames[1]=pUsedBotNames[0];
		
		bool bBotnameUsed=TRUE;
		int iCount=0;

		STRINGNODE* pTempNode=NULL;
		// Find a Botname from Botnames.txt which isn't used yet
		while(bBotnameUsed)
		{
			assert(pBotNames!=NULL);
			do
			{
				int iRand=RANDOM_LONG(0,iNumBotnames-1);
				pTempNode=GetNodeStringNames(pBotNames,iRand);
			} while(pTempNode==NULL);
			if(pTempNode->pszString==NULL)
				continue;

			bBotnameUsed=FALSE;
			for(int i=0;i<8;i++)
			{
				if(pUsedBotNames[i]==pTempNode)
				{
					iCount++;
					if(iCount<iNumBotnames-1)
						bBotnameUsed=TRUE;
				}
			}
			
		}
		// Save new Name
		assert(pTempNode!=NULL);
		assert(pTempNode->pszString!=NULL);
		pUsedBotNames[0]=pTempNode;
		if(g_bDetailNames)
			strcat(c_name,pTempNode->pszString);
		else
			strcpy(c_name,pTempNode->pszString);
	}
	else
		strcpy(c_name,arg4);


	// Find a free slot in our Table of Bots
	index = 0;
	while ((bots[index].is_used) && (index < MAXBOTSNUMBER))
		index++;
	
	// Too many existing Bots in Game
	if (index == MAXBOTSNUMBER)
	{
		if (pPlayer)
			UTIL_HostPrint(HUD_PRINTNOTIFY, "Can't create bot! Too many Clients...\n");
		botcreation_time=0.0;
		g_bBusyCreatingBot=FALSE;
		return;
	}

	// If this a new Bot (not respawned from a previous game) then
	// append the skill to its name
	if(bots[index].respawn_state != RESPAWN_IS_RESPAWNING)
	{
		sprintf(c_skill," (%d)",skill);
		if(g_bDetailNames)
			strcat(c_name,c_skill);
	}

	// This call creates the Fakeclient
	BotEnt = CREATE_FAKE_CLIENT( c_name );

	// Did the Call succeed ?
	if (FNullEnt( BotEnt ))
	{
		if (pPlayer)
			UTIL_HostPrint(HUD_PRINTNOTIFY, "Max. Players reached.  Can't create bot!\n");
		else if (IS_DEDICATED_SERVER())
			printf("Max. Players reached.  Can't create bot!\n");
		botcreation_time=0.0;
	}
	// YEP! Our little Bot has spawned !
	else
	{
		char ptr[128];  // allocate space for message from ClientConnect
		char *infobuffer;
		int clientIndex;
		
		// Notify calling Player of the creation
		if (IS_DEDICATED_SERVER())
			printf("Creating bot...\n");
		else if (pPlayer)
			UTIL_HostPrint(HUD_PRINTNOTIFY, "Creating bot...\n");

		// Fix from Lean Hartwig at Topica
		FREE_PRIVATE(BotEnt);
		
		// create the player entity by calling MOD's player function
		// (from LINK_ENTITY_TO_CLASS for player object)
		player( VARS(BotEnt) );
		
		// Set all Infobuffer Keys for this Bot
		infobuffer = GET_INFOBUFFER( BotEnt );
		clientIndex = ENTINDEX( BotEnt );
		
		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "model", "gina" );
		
		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "rate", "3500.000000");
		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "cl_updaterate", "20");
		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "cl_lw", "1");
		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "cl_lc", "1");
		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "tracker", "0");
		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "cl_dlmax", "128");
		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "lefthand", "1");
		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "friends", "0");
		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "dm", "0");
		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "ah", "1");
		// - End Infobuffer - 
		
		// Connect this client with the local loopback
		ClientConnect( BotEnt, c_name, "127.0.0.1", ptr );
		
		// Pieter van Dijk - use instead of DispatchSpawn() - Hip Hip Hurray!
		ClientPutInServer( BotEnt );
		
		// Setting these Flags causes the Engine to not send
		// Network Messages over to this Client (Bot)
		BotEnt->v.flags |= FL_FAKECLIENT;
		BotEnt->v.spawnflags |= FL_FAKECLIENT;
		
		// initialize all the variables for this bot...
		pBot = &bots[index];
		
		pBot->is_used = TRUE;
		pBot->respawn_state = RESPAWN_IDLE;
		pBot->name[0] = 0;  // name not set by server yet
		
		
		pBot->pEdict = BotEnt;
		
		pBot->not_started = 1;  // hasn't joined game yet
		
		pBot->start_action = MSG_CS_IDLE;

		pBot->bot_money = 0;

		// Assign a random Spraypaint
		if(g_iNumLogos == 0)
			pBot->iSprayLogo = 0;
		else
			pBot->iSprayLogo=RANDOM_LONG(0,g_iNumLogos);

		// Assign how talkative this Bot will be
		pBot->SaytextBuffer.fChatDelay = RANDOM_FLOAT(4.0,10.0);
		pBot->SaytextBuffer.cChatProbability = RANDOM_LONG(1,100);

		pBot->bDead = TRUE;
		
	
		BotEnt->v.idealpitch = BotEnt->v.v_angle.x;
		BotEnt->v.ideal_yaw = BotEnt->v.v_angle.y;
		BotEnt->v.yaw_speed = BOT_YAW_SPEED;
		BotEnt->v.pitch_speed = BOT_PITCH_SPEED;
		
		pBot->bot_skill = skill;
		pBot->bot_personality=iPersonality;
		// Set the Base Fear/Agression Levels for this Personality
		switch(iPersonality)
		{
			// Normal
		case 0:
			pBot->fBaseAgressionLevel=RANDOM_FLOAT(0.5,0.6);
			pBot->fBaseFearLevel=RANDOM_FLOAT(0.5,0.6);
			break;
			// Psycho
		case 1:
			pBot->fBaseAgressionLevel=RANDOM_FLOAT(0.7,1.0);
			pBot->fBaseFearLevel=RANDOM_FLOAT(0.0,0.4);
			break;
			// Coward
		case 2:
			pBot->fBaseAgressionLevel=RANDOM_FLOAT(0.0,0.4);
			pBot->fBaseFearLevel=RANDOM_FLOAT(0.7,1.0);
			break;
		}

		// Copy them over to the temp Level Variables
		pBot->fAgressionLevel=pBot->fBaseAgressionLevel;
		pBot->fFearLevel=pBot->fBaseFearLevel;
		pBot->fNextEmotionUpdate=gpGlobals->time+0.5;

		// Bot needs to call BotSpawnInit to fully initialize
		pBot->need_to_initialize=TRUE;

#ifdef DEBUG
		pBot->bDebug = FALSE;
#endif

		// Just to be sure
		pBot->iActMessageIndex=0;
		pBot->iPushMessageIndex=0;
		
		// Assign Team & Class
		if(arg2 != NULL && *arg2!=0)
			pBot->bot_team=atoi(arg2);
		else
			pBot->bot_team = 5;
		if(arg3 != NULL && *arg3!=0)
			pBot->bot_class=atoi(arg3);
		else
			pBot->bot_class = 5;

		BotSpawnInit(pBot);
	}
	g_bBusyCreatingBot=FALSE;
}


// Creation of Bot who controls a Human Player (Edict)
// basically unfinished and mostly a slightly changed opy of
// BotCreate
void CreateBorgBot(edict_t *pPlayer)
{
	// Don't allow creating Bots when no waypoints are loaded
	if(g_iNumWaypoints<1)
	{
		UTIL_HostPrint(HUD_PRINTNOTIFY,"No Waypoints for this Map, can't create Bot!\n");
		return;
	}
	int index = 0;
	while ((bots[index].is_used) && (index <MAXBOTSNUMBER))
		index++;
	if (index == MAXBOTSNUMBER)
	{
		UTIL_HostPrint(HUD_PRINTNOTIFY,"Can't create Bot - too many Bots !\n");
		return;
	}

	// initialize all the variables for this bot...
#ifdef DEBUG
	report_log (LOG_BOTBASE, "CreateBorgBot",NULL);
#endif
	
	bot_t *pBot = &bots[index];
	
	pBot->is_used = TRUE;
	pBot->respawn_state = RESPAWN_IDLE;
	pBot->pEdict = pPlayer;
	strcpy(pBot->name, STRING(pBot->pEdict->v.netname));
	
	pPlayer->v.fixangle = 1;
	
	pBot->not_started = 0;
	
	pBot->start_action = MSG_CS_IDLE;
	
	pBot->bot_money = 0;
	if(IsAlive(pPlayer))
		pBot->bDead = FALSE;
	else
		pBot->bDead = TRUE;

	edict_t *BotEnt=pPlayer;
	
	BotEnt->v.yaw_speed = BOT_YAW_SPEED;
	BotEnt->v.pitch_speed = BOT_PITCH_SPEED;
	UTIL_ClampAngle(&BotEnt->v.ideal_yaw);
	UTIL_ClampAngle(&BotEnt->v.idealpitch);
	
	BotEnt->v.v_angle.x=BotEnt->v.idealpitch;
	BotEnt->v.v_angle.y=BotEnt->v.ideal_yaw;
	BotEnt->v.angles.y=BotEnt->v.v_angle.y;
	BotEnt->v.angles.x=0;
	BotEnt->v.angles.z=0;

	pBot->bot_skill = 100;
	pBot->bot_personality = 2;
	pBot->fBaseAgressionLevel = RANDOM_FLOAT(0.0,0.5);
	pBot->fBaseFearLevel=RANDOM_FLOAT(0.7,1.0);

	pBot->fAgressionLevel=pBot->fBaseAgressionLevel;
	pBot->fFearLevel = pBot->fBaseFearLevel;
	pBot->need_to_initialize=FALSE;
	// Just to be sure
	pBot->iActMessageIndex=0;
	pBot->iPushMessageIndex=0;
	
	pBot->bot_team=UTIL_GetTeam(pPlayer)+1;
	pBot->bot_class = 0;

	int i;

	DeleteSearchNodes(pBot);
	pBot->v_prev_origin = Vector(9999.0, 9999.0, 9999.0);

	pBot->wpt_origin = Vector(0, 0, 0);
	pBot->dest_origin = Vector(0, 0, 0);
	pBot->curr_wpt_index = -1;
	pBot->prev_wpt_index[0] = -1;
	pBot->prev_wpt_index[1] = -1;
	pBot->prev_wpt_index[2] = -1;
	pBot->prev_wpt_index[3] = -1;
	pBot->prev_wpt_index[4] = -1;
	pBot->prev_goal_index=-1;
	pBot->f_wpt_timeset=gpGlobals->time;
	
	pBot->iStates=0;
	BotResetTasks(pBot);

	pBot->msecnum = 0;
	pBot->msecdel = 0.0;
	pBot->msecval = 0.0;
	
	pBot->bot_health = 100;
	
	pBot->bIsVIP=FALSE;

	pBot->f_max_speed = CVAR_GET_FLOAT("sv_maxspeed");
	pBot->prev_speed = 0.0;  // fake "paused" since bot is NOT stuck


	pBot->f_view_distance=4096.0;
	pBot->f_maxview_distance=4096.0;

	pBot->ladder_dir = 0;
	pBot->f_use_ladder_time = 0.0;
	pBot->pBotPickupItem=NULL;
	
	pBot->f_wall_check_time = 0.0;
	pBot->f_wall_on_right = 0.0;
	pBot->f_wall_on_left = 0.0;
	pBot->f_dont_avoid_wall_time = 0.0;
	pBot->f_collide_time=0.0;

	pBot->pBotEnemy = NULL;
	pBot->vecLastEnemyOrigin=Vector(0,0,0);
	pBot->f_bot_see_enemy_time = 0.0;

	pBot->vecPosition = Vector(0,0,0);

	pBot->f_ideal_reaction_time=BotSkillDelays[pBot->bot_skill/20].fMinSurpriseDelay;
	pBot->f_actual_reaction_time=BotSkillDelays[pBot->bot_skill/20].fMinSurpriseDelay;

	pBot->pBotUser = NULL;
	pBot->f_bot_use_time = 0.0;
	
	for(i=0;i<MAX_HOSTAGES;i++)
		pBot->pHostages[i]=NULL;

	pBot->f_hostage_check_time=gpGlobals->time;

	pBot->bIsReloading=FALSE;
	pBot->f_shoot_time = gpGlobals->time;
	pBot->f_primary_charging = -1.0;
	pBot->f_secondary_charging = -1.0;
	pBot->bUsingGrenade = FALSE;

	pBot->charging_weapon_id = 0;
	
	pBot->f_jumptime=0;

	pBot->f_sound_update_time = gpGlobals->time;
	pBot->f_heard_sound_time = 0.0;
	
	pBot->f_bomb_time=0;

	pBot->b_lift_moving = FALSE;

	if(pBot->bDead)
	{
		memset(&(pBot->current_weapon), 0, sizeof(pBot->current_weapon));
		memset(&(pBot->m_rgAmmo), 0, sizeof(pBot->m_rgAmmo));
		pBot->iBuyCount=0;
		pBot->pSelected_weapon=NULL;
		pBot->bot_armor = 0;
	}
	else
	{
		if(BotHasPrimaryWeapon(pBot))
			pBot->iBuyCount=3;		// only buy ammo & items
		else
			pBot->iBuyCount=0;
	}
	pBot->f_zoomchecktime=0.0;
	pBot->iNumWeaponPickups=0;

	pBot->f_StrafeSetTime=0.0;
	pBot->byCombatStrafeDir=0;
	pBot->byFightStyle=0;
	pBot->f_lastfightstylecheck=0.0;

	pBot->bCheckWeaponSwitch=TRUE;
	pBot->bBuyingFinished=FALSE;
	pBot->bInBuyzone=FALSE;
	pBot->BuyAction=MSG_CS_IDLE;

	pBot->pRadioEntity=NULL;
	pBot->iRadioOrder=0;

	pBot->bLogoSprayed=FALSE;

	pBot->need_to_initialize = FALSE;
	pBot->f_spawn_time=gpGlobals->time;
	pBot->f_lastchattime=gpGlobals->time;
	for(i=0;i<32;i++)
		pBot->aMessageQueue[i]=MSG_CS_IDLE;

	pBot->pEdict->v.flags |= FL_FAKECLIENT;
	pBot->pEdict->v.spawnflags |= FL_FAKECLIENT;
	pBot->iActMessageIndex=0;
	pBot->iPushMessageIndex=0;
    BotPushMessageQueue(pBot,MSG_CS_BUY);
	g_bIsBorg = TRUE;
}


// Handles the selection of Teams & Class
void BotStartGame( bot_t *pBot )
{
   char c_team[32];
   char c_class[32];

#ifdef DEBUG
	report_log (LOG_BOTBASE, "BotStartGame","Action=%d",pBot->start_action);
#endif
   edict_t *pEdict = pBot->pEdict;

      // handle Counter-Strike stuff here...

      if (pBot->start_action == MSG_CS_MOTD)
	  {
		  pBot->pEdict->v.button = IN_ATTACK;
		  return;
	  }
	  else if (pBot->start_action == MSG_CS_TEAM_SELECT)
      {
         pBot->start_action = MSG_CS_IDLE;  // switch back to idle

         if ((pBot->bot_team != 1) && (pBot->bot_team != 2) &&
             (pBot->bot_team != 5))
            pBot->bot_team = 0;

         if (pBot->bot_team == 0)
            pBot->bot_team = RANDOM_LONG(1, 2);

         // select the team the bot wishes to join...
         if (pBot->bot_team == 1)
            strcpy(c_team, "1");
         else if (pBot->bot_team == 2)
            strcpy(c_team, "2");
         else
            strcpy(c_team, "5");

         FakeClientCommand(pEdict, "menuselect", c_team, NULL);

         return;
      }
      else if (pBot->start_action == MSG_CS_CT_SELECT)  // counter terrorist
      {
         pBot->start_action = MSG_CS_IDLE;  // switch back to idle

         if ((pBot->bot_class < 1) || (pBot->bot_class > 4))
            pBot->bot_class = 0;  // use random if invalid

         if (pBot->bot_class == 0)
            pBot->bot_class = RANDOM_LONG(1, 4);

         // select the class the bot wishes to use...
         if (pBot->bot_class == 1)
            strcpy(c_class, "1");
         else if (pBot->bot_class == 2)
            strcpy(c_class, "2");
         else if (pBot->bot_class == 3)
            strcpy(c_class, "3");
         else if (pBot->bot_class == 4)
            strcpy(c_class, "4");
         else
            strcpy(c_class, "5");  // random

         FakeClientCommand(pEdict, "menuselect", c_class, NULL);

         // bot has now joined the game (doesn't need to be started)
         pBot->not_started = 0;
         return;
      }
      else if (pBot->start_action == MSG_CS_T_SELECT)  // terrorist select
      {
         pBot->start_action = MSG_CS_IDLE;  // switch back to idle

         if ((pBot->bot_class < 1) || (pBot->bot_class > 4))
            pBot->bot_class = 0;  // use random if invalid

         if (pBot->bot_class == 0)
            pBot->bot_class = RANDOM_LONG(1, 4);

         // select the class the bot wishes to use...
         if (pBot->bot_class == 1)
            strcpy(c_class, "1");
         else if (pBot->bot_class == 2)
            strcpy(c_class, "2");
         else if (pBot->bot_class == 3)
            strcpy(c_class, "3");
         else if (pBot->bot_class == 4)
            strcpy(c_class, "4");
         else
            strcpy(c_class, "5");  // random

         FakeClientCommand(pEdict, "menuselect", c_class, NULL);

         // bot has now joined the game (doesn't need to be started)
         pBot->not_started = 0;
      }
}

// Called from Usercommand "removebots"
// Removes all Bots from the Server, sets minbots/maxbots to 0
void UserRemoveAllBots(edict_t *pPlayer)
{
	max_bots=0;
	min_bots=0;
	for (int index = 0; index <MAXBOTSNUMBER; index++)
	{
		// Reset our Creation Tab if there are still Bots waiting
		// to be spawned
		BotCreateTab[index].bNeedsCreation=FALSE;
		BotCreateTab[index].pCallingEnt=NULL;
		BotCreateTab[index].name[0]='\0';
		BotCreateTab[index].team[0]='\0';
		BotCreateTab[index].skill[0]='\0';

		if (bots[index].is_used)  // is this slot used?
		{
			char cmd[40];
			
			sprintf(cmd, "kick \"%s\"\n", bots[index].name);
			
			bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
			
			SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
		}
	}
	UTIL_HostPrint(HUD_PRINTNOTIFY,"All Bots removed !\n");
}

// Called by UserCommand "killbots"
// Kills all Bots
void UserKillAllBots(edict_t *pPlayer)
{
	bot_t *pBot;

	for(int i=0;i<MAXBOTSNUMBER;i++)
	{
		pBot = &bots[i];
		if((pBot->is_used == TRUE) && (IsAlive(pBot->pEdict)))
		{
			// If a Bot gets killed it decreases his frags, so add
			// 1 here to not give human players an advantage using this
			// command
			pBot->pEdict->v.frags +=1;
			pBot->need_to_initialize=TRUE;
			ClientKill(pBot->pEdict);
		}
	}
	UTIL_HostPrint(HUD_PRINTNOTIFY, "All Bots killed !\n");
}

// Called by UserCommand "newround"
// Kills all Clients in Game including humans
void UserNewroundAll(edict_t *pEntity)
{
	edict_t *pPlayer;

	for (int ind = 1; ind <= gpGlobals->maxClients; ind++)
	{
		pPlayer = INDEXENT(ind);
		
		// is this player slot valid
		if ((pPlayer) && (!pPlayer->free))
		{
			
			if(pPlayer->v.flags & (FL_CLIENT | FL_FAKECLIENT))
			{
				if(IsAlive(pPlayer))
				{
					pPlayer->v.frags +=1;
					ClientKill(pPlayer);
				}
			}
		}
	}
	UTIL_HostPrint(HUD_PRINTNOTIFY, "Round Restarted !\n");
}


void UserSelectWeaponMode(edict_t *pEntity,int iSelection)
{
	int rgiWeaponTab[7][NUM_WEAPONS] = {
	// Knife only
	{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
	// Pistols only
	{-1,-1,-1,2,2,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
	// Shotgun only
	{-1,-1,-1,-1,-1,-1,-1,2,2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
	// Machine Guns only
	{-1,-1,-1,-1,-1,-1,-1,-1,-1,2,2,2,2,2,-1,-1,-1,-1,-1,-1,-1,-1,2},
	// Rifles only
	{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,0,1,1,-1,-1,-1,-1,-1},
	// Snipers only
	{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,2,2,0,1,-1},
	// Standard
	{-1,-1,-1,2,2,0,1,2,2,2,1,2,0,2,0,0,1,1,2,2,0,1,2}};

	char szMode[7][12] = {
		{"Knife"},
		{"Pistol"},
		{"Shotgun"},
		{"Machine Gun"},
		{"Rifle"},
		{"Sniper"},
		{"Standard"}};

	iSelection--;
	for(int i=0;i<NUM_WEAPONS;i++)
	{
		cs_weapon_select[i].iTeamStandard = rgiWeaponTab[iSelection][i];
		cs_weapon_select[i].iTeamAS = rgiWeaponTab[iSelection][i];
	}

	if(iSelection == 0)
		g_bJasonMode = TRUE;
	else
		g_bJasonMode = FALSE;

	char szMessage[128];

	sprintf(szMessage,"%s Weapon Mode selected!\n",&szMode[iSelection][0]);
	UTIL_HostPrint(HUD_PRINTNOTIFY, szMessage);
}


// List Bots on the Server
void UserListBots(edict_t *pPlayer)
{
	bot_t *pBot;
	char szMessage[80];

	for(int i=0;i<MAXBOTSNUMBER;i++)
	{
		pBot = &bots[i];
		if(pBot->is_used)
		{
				sprintf(szMessage,"%d - %s\n",i,pBot->name);
				UTIL_HostPrint(HUD_PRINTCONSOLE, szMessage);
		}
	}
}

// Get the current Message from the Bots Message Queue
int BotGetMessageQueue(bot_t *pBot)
{
	int iMSG=pBot->aMessageQueue[pBot->iActMessageIndex++];
	pBot->iActMessageIndex&=0x1f;	//Wraparound
	return iMSG;
}

// Put a Message into the Message Queue
void BotPushMessageQueue(bot_t *pBot,int iMessage)
{
	if(iMessage == MSG_CS_SAY)
	{
		// Notify other Bots of the spoken Text otherwise
		// Bots won't respond to other Bots (Network Messages
		// aren't sent from Bots)
		int iEntIndex = ENTINDEX(pBot->pEdict);
		char szMessage[256];

		// Add this so the Chat Parser doesn't get confused
		strcpy(szMessage,STRING(pBot->pEdict->v.netname));
		strcat(szMessage,":");
		strcat(szMessage,pBot->szMiscStrings);
		bool bIsAlive = IsAlive(pBot->pEdict);
		for (int bot_index = 0; bot_index < MAXBOTSNUMBER; bot_index++)
		{
			bot_t *pOtherBot = &bots[bot_index];
			if (pOtherBot->is_used && pOtherBot->respawn_state == RESPAWN_IDLE
				&& pOtherBot != pBot)  
			{
				if(bIsAlive == IsAlive(pOtherBot->pEdict))
				{
					pOtherBot->SaytextBuffer.iEntityIndex = iEntIndex;
					strcpy(pOtherBot->SaytextBuffer.szSayText,szMessage);
				}
				pOtherBot->SaytextBuffer.fTimeNextChat = gpGlobals->time + pOtherBot->SaytextBuffer.fChatDelay;
			}
		}
	}
	pBot->aMessageQueue[pBot->iPushMessageIndex++]=iMessage;
	pBot->iPushMessageIndex&=0x1f;	//Wraparound
}



int BotInFieldOfView(bot_t *pBot, Vector dest)
{
   // find angles from source to destination...
   Vector entity_angles = UTIL_VecToAngles( dest );

   // make yaw angle 0 to 360 degrees if negative...
   if (entity_angles.y < 0)
      entity_angles.y += 360;

   // get bot's current view angle...
   float view_angle = pBot->pEdict->v.v_angle.y;

   // make view angle 0 to 360 degrees if negative...
   if (view_angle < 0)
      view_angle += 360;

   // return the absolute value of angle to destination entity
   // zero degrees means straight ahead,  45 degrees to the left or
   // 45 degrees to the right is the limit of the normal view angle

   // rsm - START angle bug fix
   int angle = abs((int)view_angle - (int)entity_angles.y);

   if (angle > 180)
      angle = 360 - angle;

   return angle;
   // rsm - END
}


bool BotItemIsVisible( bot_t *pBot, Vector vecDest,char* pszItemName)
{
	TraceResult tr;
	
	// trace a line from bot's eyes to destination...
	UTIL_TraceLine( pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs,
		vecDest, ignore_monsters,
		pBot->pEdict->v.pContainingEntity, &tr );
	
	// check if line of sight to object is not blocked (i.e. visible)
	if (tr.flFraction != 1.0)
	{
		if(strcmp(STRING(tr.pHit->v.classname),pszItemName)==0)
			return TRUE;
		return FALSE;
	}
	return TRUE;
}

bool BotEntityIsVisible( bot_t *pBot, Vector vecDest )
{
	TraceResult tr;
	
	// trace a line from bot's eyes to destination...
	UTIL_TraceLine( pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs,
		vecDest, ignore_monsters,
		pBot->pEdict->v.pContainingEntity, &tr );
	
	// check if line of sight to object is not blocked (i.e. visible)
	if (tr.flFraction != 1.0)
		return FALSE;
	return TRUE;
}

// Check if Bot 'sees' a SmokeGrenade to simulate the effect
// of being blinded by smoke + notice Bot of Grenades flying towards
// him
// TODO: Split this up and make it more reliable 
void BotCheckSmokeGrenades(bot_t *pBot)
{
	edict_t *pEdict=pBot->pEdict;
	edict_t *pent = pBot->pAvoidGrenade;
	Vector vecView = pEdict->v.origin + pEdict->v.view_ofs;
	float fDistance;

	// Check if old ptr to Grenade is invalid
	if(pent!=NULL)
	{
		if(FNullEnt(pent))
		{
			pBot->pAvoidGrenade=NULL;
			pBot->cAvoidGrenade=0;
		}
		else if((pent->v.flags & FL_ONGROUND) ||
			(pent->v.effects & EF_NODRAW))
		{
			pBot->pAvoidGrenade=NULL;
			pBot->cAvoidGrenade=0;
		}
	}
	else
		pBot->cAvoidGrenade=0;

	pent=NULL;

	// Find all Grenades on the map
	while ((pent = UTIL_FindEntityByClassname( pent, "grenade" )) != NULL)
	{
		// If Grenade is invisible don't care for it 
		if (pent->v.effects & EF_NODRAW)
			continue;
		// Check if visible to the Bot
		if(!BotEntityIsVisible(pBot,pent->v.origin) && 
			BotInFieldOfView( pBot, pent->v.origin - vecView ) > pEdict->v.fov/2)
			continue;
		if(pBot->pAvoidGrenade == NULL)
		{
			// Is this a flying Grenade ?
			if((pent->v.flags & FL_ONGROUND)==0)
			{
				fDistance = (pent->v.origin - pEdict->v.origin).Length();
				float fDistanceMoved = ((pent->v.origin + pent->v.velocity*pBot->fTimeFrameInterval) - pEdict->v.origin).Length();
				// Is the Grenade approaching this Bot ?
				if(fDistanceMoved<fDistance && fDistance<600)
				{
					Vector2D	vec2DirToPoint; 
					Vector2D	vec2RightSide;
					
					// to start strafing, we have to first figure out if the target is on the left side or right side
					UTIL_MakeVectors (pEdict->v.angles );
					
					vec2DirToPoint = ( pEdict->v.origin - pent->v.origin).Make2D().Normalize();
					vec2RightSide = gpGlobals->v_right.Make2D().Normalize();
					if ( DotProduct ( vec2DirToPoint, vec2RightSide ) > 0 )
						pBot->cAvoidGrenade=-1;
					else
						pBot->cAvoidGrenade=1;
					pBot->pAvoidGrenade=pent;
					return;
				}
			}
		}

		// Is this a SmokeGrenade and on Ground (smoking) ?
		if(!FStrEq(STRING(pent->v.model), "models/w_smokegrenade.mdl") ||
			(pent->v.flags & FL_ONGROUND)==0)
			continue;
		fDistance = (pent->v.origin - pEdict->v.origin).Length();
		// Shrink Bots Viewing Distance to Smoke Grens Distance
		if(pBot->f_view_distance>fDistance)
			pBot->f_view_distance=fDistance;
	}
}

// Returns the best weapon of this Bot (based on Personality Prefs)
int GetBestWeaponCarried(bot_t *pBot)
{
	int* ptrWeaponTab=ptrWeaponPrefs[pBot->bot_personality];
	bot_weapon_select_t *pWeaponTab=&cs_weapon_select[0];
	int iWeaponIndex;
//		if ((pBot->bot_weapons & (1<<pWeaponTab[*ptrWeaponTab].iId)))
	int iWeapons=pBot->pEdict->v.weapons;

	for(int i=0;i<NUM_WEAPONS;i++)
	{
		if ((pBot->pEdict->v.weapons & (1<<pWeaponTab[*ptrWeaponTab].iId)))
			iWeaponIndex=i;
		ptrWeaponTab++;
	}
	return iWeaponIndex;
}

// Compares Weapons on the Ground to the one the Bot is using
int BotRateGroundWeapon(bot_t *pBot,edict_t *pent)
{
	int iHasWeapon=GetBestWeaponCarried(pBot);

	int* ptrWeaponTab=ptrWeaponPrefs[pBot->bot_personality];
	bot_weapon_select_t *pWeaponTab=&cs_weapon_select[0];

	int iGroundIndex=0;
	char szModelName[40];

	strcpy(szModelName, STRING(pent->v.model));

	for(int i=0;i<NUM_WEAPONS;i++)
	{
		if(FStrEq(pWeaponTab[*ptrWeaponTab].model_name,szModelName))
		{
			iGroundIndex=i;
			break;
		}
		ptrWeaponTab++;
	}

	// Don't care for pistols
	// TODO Include Pistols as well...
	if(iGroundIndex<7)
		iGroundIndex=0;

	return(iGroundIndex-iHasWeapon);
}


// Checks if Bot is blocked by a Shootable Breakable in his
// moving direction
bool BotFindBreakable(bot_t *pBot)
{
	TraceResult tr;
	edict_t *pEdict = pBot->pEdict;
	edict_t *pent;

#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotFindBreakable",NULL);
#endif

	Vector v_src = pEdict->v.origin;
	Vector vecDirection=(pBot->dest_origin - v_src).Normalize();
	Vector v_dest = v_src + vecDirection * 50;
	UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters,dont_ignore_glass,
		pEdict->v.pContainingEntity, &tr);
	if (tr.flFraction != 1.0)
	{
		pent=tr.pHit;
		if(strcmp("func_breakable",STRING(pent->v.classname))==0)
		{
			if(IsShootableBreakable(pent))
			{
				pBot->vecBreakable=tr.vecEndPos;
				return TRUE;
			}
		}
	}
	v_src = pEdict->v.origin+pEdict->v.view_ofs;
	vecDirection=(pBot->dest_origin - v_src).Normalize();
	v_dest = v_src + vecDirection * 50;
	UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters,dont_ignore_glass,
		pEdict->v.pContainingEntity, &tr);
	if (tr.flFraction != 1.0)
	{
		pent=tr.pHit;
		if(strcmp("func_breakable",STRING(pent->v.classname))==0)
		{
			if(IsShootableBreakable(pent))
			{
				pBot->vecBreakable=tr.vecEndPos;
				return TRUE;
			}
		}
	}

	pBot->pShootBreakable=NULL;
	pBot->vecBreakable=Vector(0,0,0);
	return FALSE;
}


// Finds Items to collect or use in the near of a Bot
void BotFindItem( bot_t *pBot )
{
	edict_t *pEdict = pBot->pEdict;
	edict_t *pent = NULL;
	edict_t *pPickupEntity = NULL;
	float radius = 500;

	// Don't try to pickup anything while on ladder...
	if(pEdict->v.movetype == MOVETYPE_FLY)
	{
		pBot->pBotPickupItem = NULL;
		pBot->iPickupType = PICKUP_NONE;
		return;
	}

	if(pBot->pBotPickupItem != NULL)
	{
		bool bItemExists = FALSE;
		pPickupEntity = pBot->pBotPickupItem;
		while ((pent = UTIL_FindEntityInSphere( pent, pEdict->v.origin, radius )) != NULL)
		{
			if (pent->v.effects & EF_NODRAW)
			{
				// someone owns this weapon or it hasn't respawned yet
				continue;
			}
			if(pent	== pPickupEntity)
			{
				Vector vecPosition;
				if (strncmp("func_", STRING(pent->v.classname), 5) == 0
				|| FBitSet(pent->v.flags, FL_MONSTER))
				{
					vecPosition = VecBModelOrigin(pent);
				}
				else
					vecPosition = pent->v.origin;

				if(BotItemIsVisible(pBot,vecPosition,(char*)STRING(pent->v.classname)))
					bItemExists = TRUE;
				break;
			}
		}
		if(bItemExists)
			return;
		else
		{
			pBot->pBotPickupItem=NULL;
			pBot->iPickupType=PICKUP_NONE;
		}
	}

	pent = NULL;
	pPickupEntity = NULL;
	int iPickType=0;
	Vector pickup_origin;
	Vector entity_origin;
	bool bCanPickup;
	float distance,min_distance;
	char item_name[40];
	Vector vecEnd;
//	int angle_to_entity;

	pBot->pBotPickupItem = NULL;
	pBot->iPickupType=PICKUP_NONE;
	min_distance = radius + 1.0;
	
	while ((pent = UTIL_FindEntityInSphere( pent, pEdict->v.origin, radius )) != NULL)
	{
		bCanPickup = FALSE;  // assume can't use it until known otherwise
		if (pent->v.effects & EF_NODRAW || pent == pBot->pItemIgnore)
		{
			// someone owns this weapon or it hasn't respawned yet
			continue;
		}

		strcpy(item_name, STRING(pent->v.classname));
		
		// see if this is a "func_" type of entity (func_button, etc.)...
		if (strncmp("func_", item_name, 5) == 0
		|| FBitSet(pent->v.flags, FL_MONSTER))
		{
			// BModels have 0,0,0 for origin so must use VecBModelOrigin...
			entity_origin = VecBModelOrigin(pent);
		}
		else
			entity_origin = pent->v.origin;
		
		vecEnd = entity_origin;
		
		// check if line of sight to object is not blocked (i.e. visible)
		if(BotItemIsVisible(pBot,vecEnd,(char*)STRING(pent->v.classname)))
		{
			if(strcmp("func_button", item_name) == 0)
			{
				bCanPickup = TRUE;
				iPickType = PICKUP_BUTTON;
			}
			else if(strncmp("hostage_entity", item_name, 14) == 0)
			{
				if(pent->v.velocity == Vector(0,0,0))
				{
					bCanPickup = TRUE;
					iPickType = PICKUP_HOSTAGE;
				}
			}
			else if ((strncmp("weaponbox", item_name, 9) == 0) && (!FStrEq(STRING(pent->v.model), "models/w_backpack.mdl")) &&
				(!pBot->bUsingGrenade))
			{
				bCanPickup = TRUE;
				iPickType = PICKUP_WEAPON;
			}
			else if ((strncmp("weaponbox", item_name, 9) == 0) && (FStrEq(STRING(pent->v.model), "models/w_backpack.mdl")) )
			{
				bCanPickup = TRUE;
				iPickType = PICKUP_DROPPED_C4;
			}
			else if ((strncmp("grenade", item_name, 7) == 0) && (FStrEq(STRING(pent->v.model), "models/w_c4.mdl")) )
			{
				bCanPickup = TRUE;
				iPickType = PICKUP_PLANTED_C4;
			}
		}
		
		if (bCanPickup) // if the bot found something it can pickup...
		{
			distance = (entity_origin - pEdict->v.origin).Length( );
			
			// see if it's the closest item so far...
			if (distance < min_distance)
			{
				int i;
				// Found a Button ?
				if(iPickType == PICKUP_BUTTON)
				{
					if(pBot->f_use_button_time+5.0<gpGlobals->time)
					{
						edict_t *pTrigger = NULL;
						pTrigger = UTIL_FindEntityByTargetname(pTrigger,STRING(pent->v.target));
						if(pTrigger!=NULL)
						{
							// Exclude Monitors from using
							if(strcmp("trigger_camera",STRING(pTrigger->v.classname))==0)
								bCanPickup = FALSE;
							
						}
					}
					else
						bCanPickup = FALSE;
				}
				// Found weapon on ground ?
				else if(iPickType == PICKUP_WEAPON)
				{
					if(pBot->bIsVIP || BotRateGroundWeapon(pBot,pent)<=0
						|| pBot->iNumWeaponPickups>g_iMaxWeaponPickup)
					{
						bCanPickup = FALSE;
					}
				}

				// Terrorist Team specific
				else if(pBot->iTeam == 0)
				{
					if(iPickType == PICKUP_HOSTAGE)
					{

						if(g_bHostageRescued)
						{
							if(RANDOM_LONG(1,100)>20)
								bCanPickup = FALSE;
						}
						else
						{
							if(pBot->f_hostage_check_time + 10.0 < gpGlobals->time)
							{
								// Check if Hostage has been moved away
								float distance;
								float mindistance=9999;
								
								for (i=0; i < g_iNumWaypoints; i++)
								{
									if(paths[i]->flags & W_FL_GOAL)
									{
										distance = (paths[i]->origin - entity_origin).Length();
										if(distance < mindistance)
										{
											mindistance=distance;
											if(mindistance < 300)
											{
												bCanPickup = FALSE;
												break;
											}
										}
									}
								}
								pBot->f_hostage_check_time = gpGlobals->time;
							}
							else
								bCanPickup = FALSE;
						}

						if(bCanPickup)
						{
							for(i=0;i<MAXBOTSNUMBER;i++)
							{
								if(bots[i].is_used == TRUE)
								{
									if(!IsAlive(bots[i].pEdict))
										continue;
									for(int h=0;h<MAX_HOSTAGES;h++)
									{
										if(bots[i].pHostages[h] == pent)
										{
											bCanPickup = FALSE;
											break;
										}
									}
								}
							}
						}

					}
					else if(iPickType == PICKUP_PLANTED_C4)
					{
						bCanPickup = FALSE;
						if(!pBot->bDefendedBomb)
						{
							pBot->bDefendedBomb = TRUE;
							int iIndex = BotFindDefendWaypoint(pBot,entity_origin);
							float fTraveltime = GetTravelTime(pBot->f_max_speed,pEdict->v.origin,paths[iIndex]->origin);
							float fTimeMidBlowup = CVAR_GET_FLOAT("mp_c4timer");
							fTimeMidBlowup = g_fTimeBombPlanted + (fTimeMidBlowup/2);
							fTimeMidBlowup -= fTraveltime;
							if(fTimeMidBlowup > gpGlobals->time)
							{
								// Remove any Move Tasks
								BotRemoveCertainTask(pBot,TASK_MOVETOPOSITION);
								// Push camp task on to stack
								bottask_t TempTask = {NULL,NULL,TASK_CAMP,TASKPRI_CAMP,-1,
									fTimeMidBlowup,TRUE};
								BotPushTask(pBot,&TempTask);
								// Push Move Command
								TempTask.iTask = TASK_MOVETOPOSITION;
								TempTask.fDesire = TASKPRI_MOVETOPOSITION;
								TempTask.iData = iIndex;
								BotPushTask(pBot,&TempTask);
								pBot->iCampButtons|=IN_DUCK;
							}
							else
							{
								// Issue an additional Radio Message
								BotPlayRadioMessage(pBot,RADIO_SHESGONNABLOW);
							}
						}
					}
				}
				// CT Team specific
				else
				{
					if(iPickType == PICKUP_HOSTAGE)
					{
						for(i=0;i<MAXBOTSNUMBER;i++)
						{
							if(bots[i].is_used == TRUE)
							{
								if(!IsAlive(bots[i].pEdict))
									continue;
								for(int h=0;h<MAX_HOSTAGES;h++)
								{
									if(bots[i].pHostages[h] == pent)
									{
										bCanPickup = FALSE;
										break;
									}
								}
							}
						}
					}
					else if(iPickType == PICKUP_PLANTED_C4)
					{
						edict_t *pPlayer;
						float distance;
						// search the world for players...
						for (int i = 0; i < gpGlobals->maxClients; i++)
						{
							if(ThreatTab[i].IsUsed == FALSE
								|| ThreatTab[i].IsAlive == FALSE
								|| ThreatTab[i].iTeam != pBot->iTeam
								|| ThreatTab[i].pEdict == pEdict)
								continue;
							pPlayer = ThreatTab[i].pEdict;
							// find the distance to the target waypoint
							distance = (pPlayer->v.origin - entity_origin).Length();
							if(distance<60)
							{
								bCanPickup = FALSE;
								if(!pBot->bDefendedBomb)
								{
									pBot->bDefendedBomb = TRUE;
									int iIndex = BotFindDefendWaypoint(pBot,entity_origin);
									float fTraveltime = GetTravelTime(pBot->f_max_speed,pEdict->v.origin,paths[iIndex]->origin);
									float fTimeBlowup = CVAR_GET_FLOAT("mp_c4timer");
									fTimeBlowup += g_fTimeBombPlanted - fTraveltime;
									// Remove any Move Tasks
									BotRemoveCertainTask(pBot,TASK_MOVETOPOSITION);
									// Push camp task on to stack
									bottask_t TempTask = {NULL,NULL,TASK_CAMP,TASKPRI_CAMP,-1,
										fTimeBlowup,TRUE};
									BotPushTask(pBot,&TempTask);
									// Push Move Command
									TempTask.iTask = TASK_MOVETOPOSITION;
									TempTask.fDesire = TASKPRI_MOVETOPOSITION;
									TempTask.iData = iIndex;
									BotPushTask(pBot,&TempTask);
									pBot->iCampButtons|=IN_DUCK;
									return;
								}
							}
						}
					}
					else if(iPickType == PICKUP_DROPPED_C4)
					{
						pBot->pItemIgnore = pent;
						bCanPickup = FALSE;
						if(pBot->bot_health < 50 || RANDOM_LONG(1,100) > 50)
						{
							// Push camp task on to stack
							bottask_t TempTask = {NULL,NULL,TASK_CAMP,TASKPRI_CAMP,-1,
								gpGlobals->time + RANDOM_FLOAT(30.0,60.0),TRUE};
							BotPushTask(pBot,&TempTask);
							// Push Move Command
							int iIndex = BotFindDefendWaypoint(pBot,entity_origin);
							TempTask.iTask = TASK_MOVETOPOSITION;
							TempTask.fDesire = TASKPRI_MOVETOPOSITION;
							TempTask.iData = iIndex;
							BotPushTask(pBot,&TempTask);
							pBot->iCampButtons|=IN_DUCK;
							return;
						}
					}
				}


				if(bCanPickup)
				{
					min_distance = distance;        // update the minimum distance
					pPickupEntity = pent;        // remember this entity
					pickup_origin = entity_origin;  // remember location of entity
					pBot->iPickupType = iPickType;
				}
				else
					iPickType = PICKUP_NONE;
			}
		}
	}  // end while loop

	
	if (pPickupEntity != NULL)
	{
		for(int i=0;i<MAXBOTSNUMBER;i++)
		{
			if(bots[i].is_used == TRUE)
			{
				if(!IsAlive(bots[i].pEdict))
					continue;
				if(bots[i].pBotPickupItem == pPickupEntity)
				{
					pBot->pBotPickupItem = NULL;
					pBot->iPickupType = PICKUP_NONE;
					return;
				}
			}
		}
		//Check if Item is too high to reach
		Vector vecBot=pEdict->v.origin + pEdict->v.view_ofs;
        if (pickup_origin.z > vecBot.z)
		{
			pBot->pBotPickupItem = NULL;
			pBot->iPickupType = PICKUP_NONE;
			return;
		}
		// Check if getting the item would hurt Bot		
		if(!IsDeadlyDrop(pBot,pickup_origin))
		{
			pBot->pBotPickupItem = pPickupEntity;  // save the item bot is trying to get
			if(PICKUP_BUTTON == pBot->iPickupType)
			{
				pBot->f_use_button_time = gpGlobals->time;
			}
		}
		else
		{
			pBot->pBotPickupItem = NULL;
			pBot->iPickupType = PICKUP_NONE;
		}
	}
}



void BotChangePitch( bot_t *pBot, float speed )
{
	edict_t *pEdict = pBot->pEdict;
	float ideal;
	float current;
	float current_180;  // current +/- 180 degrees
	float diff;
	float fOld = 0;
	
	UTIL_ClampAngle(&pEdict->v.idealpitch);

	if(g_bInstantTurns)
	{
		pEdict->v.v_angle.x = pEdict->v.idealpitch;
		return;
	}
	// turn from the current v_angle pitch to the idealpitch by selecting
	// the quickest way to turn to face that direction
	
	current = pEdict->v.v_angle.x;
	
	ideal = pEdict->v.idealpitch;
	
	// find the difference in the current and ideal angle
	diff = abs(current - ideal);
	
	// check if difference is less than the max degrees per turn
	if (diff < speed)
		speed = diff;  // just need to turn a little bit (less than max)
	
	// Sum up old Pitch Changes 
	fOld += pBot->rgfPitchHistory[0];
	fOld += pBot->rgfPitchHistory[1];

	fOld += speed;
	fOld /= 3;

	pBot->rgfPitchHistory[1] = pBot->rgfPitchHistory[0]; 
	pBot->rgfPitchHistory[0] = speed;

	speed = fOld;

	// here we have four cases, both angle positive, one positive and
	// the other negative, one negative and the other positive, or
	// both negative.  handle each case separately...
	
	if ((current >= 0) && (ideal >= 0))  // both positive
	{
		if (current > ideal)
			speed = -speed;
	}
	else if ((current >= 0) && (ideal < 0))
	{
		current_180 = current - 180;
		
		if (current_180 <= ideal)
			speed = -speed;
	}
	else if ((current < 0) && (ideal >= 0))
	{
		current_180 = current + 180;
		if (current_180 <= ideal)
			speed = -speed;
	}
	else  // (current < 0) && (ideal < 0)  both negative
	{
		if (current > ideal)
			speed = -speed;
	}


	current += speed;

	pEdict->v.v_angle.x = current;
}



void BotChangeYaw( bot_t *pBot, float speed )
{
	edict_t *pEdict = pBot->pEdict;
	float ideal;
	float current;
	float current_180;  // current +/- 180 degrees
	float diff;
	float fOld = 0;

	UTIL_ClampAngle(&pEdict->v.ideal_yaw);
	if(g_bInstantTurns)
	{
		pEdict->v.v_angle.y = pEdict->v.ideal_yaw;
		return;
	}
	
	// turn from the current v_angle yaw to the ideal_yaw by selecting
	// the quickest way to turn to face that direction
	
	current = pEdict->v.v_angle.y;
	
	ideal = pEdict->v.ideal_yaw;
	
	// find the difference in the current and ideal angle
	diff = abs(current - ideal);
	
	// check if difference is less than the max degrees per turn
	if (diff < speed)
		speed = diff;  // just need to turn a little bit (less than max)
	
	// Sum up old Yaw Changes 
	fOld += pBot->rgfYawHistory[0];
	fOld += pBot->rgfYawHistory[1];

	fOld += speed;
	fOld /= 3;

	pBot->rgfYawHistory[1] = pBot->rgfYawHistory[0]; 
	pBot->rgfYawHistory[0] = speed;

	speed = fOld;

	// here we have four cases, both angle positive, one positive and
	// the other negative, one negative and the other positive, or
	// both negative.  handle each case separately...
	
	if ((current >= 0) && (ideal >= 0))  // both positive
	{
		if (current > ideal)
			speed = -speed;
	}
	else if ((current >= 0) && (ideal < 0))
	{
		current_180 = current - 180;
		
		if (current_180 <= ideal)
			speed = -speed;
	}
	else if ((current < 0) && (ideal >= 0))
	{
		current_180 = current + 180;
		if (current_180 <= ideal)
			speed = -speed;
	}
	else  // (current < 0) && (ideal < 0)  both negative
	{
		if (current > ideal)
			speed = -speed;
	}
	
	current += speed;

	pEdict->v.v_angle.y = current;
	
}


// Finds a Waypoint in the near of the Bot if he lost
// his Path or if Pathfinding needs to be started over again 
bool BotFindWaypoint( bot_t *pBot )
{
   int i, wpt_index[3], select_index;
   int covered_wpt=-1;
   float distance, min_distance[3];
   Vector v_src, v_dest;
   TraceResult tr;
   bool bWaypointInUse;

   edict_t *pEdict = pBot->pEdict;

   for (i=0; i < 3; i++)
   {
      wpt_index[i] = -1;
      min_distance[i] = 9999.0;
   }

   bool bHasHostage=BotHasHostage(pBot);

   for (i=0; i < g_iNumWaypoints; i++)
   {
      // ignore current waypoint and previous recent waypoints...
      if ((i == pBot->curr_wpt_index) ||
          (i == pBot->prev_wpt_index[0]) ||
          (i == pBot->prev_wpt_index[1]) ||
          (i == pBot->prev_wpt_index[2]) ||
          (i == pBot->prev_wpt_index[3]) ||
          (i == pBot->prev_wpt_index[4]))
         continue;


      if ( WaypointReachable(pEdict->v.origin, paths[i]->origin, pEdict) )
      {

		  // Don't use duck Waypoints if Bot got a hostage
		  if(bHasHostage)
		  {
			  if(paths[i]->flags & W_FL_NOHOSTAGE)
				  continue;
		  }
		  bot_t *pOtherBot;
		  bWaypointInUse=FALSE;
		  
		  // check if this Waypoint is already in use by another bot
		  for(int c=0;c<MAXBOTSNUMBER;c++)
		  {
			  pOtherBot = &bots[c];
			  if(pOtherBot->is_used)
			  {
				  if((IsAlive(pOtherBot->pEdict)) && (pOtherBot!= pBot))
				  {
					  if(pOtherBot->curr_wpt_index==i)
					  {
						  covered_wpt=i;
						  bWaypointInUse=TRUE;
						  break;
					  }
				  }
			  }
		  }
		  if(bWaypointInUse)
			  continue;
         distance = (paths[i]->origin - pEdict->v.origin).Length();

         if (distance < min_distance[0])
         {
            wpt_index[0] = i;
            min_distance[0] = distance;
         }
         else if (distance < min_distance[1])
         {
            wpt_index[1] = i;
            min_distance[1] = distance;
         }
         else if (distance < min_distance[2])
         {
            wpt_index[2] = i;
            min_distance[2] = distance;
         }
      }
   }

   select_index = -1;

   if (wpt_index[2] != -1)
	   i = RANDOM_LONG(0, 2);
   else if (wpt_index[1] != -1)
	   i = RANDOM_LONG(0, 1);
   else if (wpt_index[0] != -1)
	   i = 0;
   else if (covered_wpt != -1)
   {
	   wpt_index[0]=covered_wpt;
	   i = 0;
   }
   else
   {
	   wpt_index[0]=RANDOM_LONG(0,g_iNumWaypoints-1);
	   i = 0;
   }
   
   select_index = wpt_index[i];

   pBot->prev_wpt_index[4] = pBot->prev_wpt_index[3];
   pBot->prev_wpt_index[3] = pBot->prev_wpt_index[2];
   pBot->prev_wpt_index[2] = pBot->prev_wpt_index[1];
   pBot->prev_wpt_index[1] = pBot->prev_wpt_index[0];
   pBot->prev_wpt_index[0] = pBot->curr_wpt_index;

   pBot->curr_wpt_index = select_index;
   pBot->f_wpt_timeset=gpGlobals->time;
   pBot->f_collide_time=gpGlobals->time;
#ifdef DEBUG
	report_log (LOG_BOTNAV, "BotFindWaypoint","Nr=%d",select_index);
#endif
   return TRUE;
}

// Checks if the last Waypoint the Bot was heading for is
// still valid
inline void GetValidWaypoint(bot_t *pBot)
{
#ifdef DEBUG
	report_log (LOG_BOTNAV, "GetValidWaypoint","curr_wpt_index=%d timeset:%f",
		pBot->curr_wpt_index,pBot->f_wpt_timeset-gpGlobals->time);
#endif
	edict_t *pEdict=pBot->pEdict;

	// If Bot hasn't got a Waypoint we need a new one anyway
	if(pBot->curr_wpt_index == -1)
	{
		DeleteSearchNodes(pBot);
		BotFindWaypoint(pBot);
		pBot->wpt_origin = paths[pBot->curr_wpt_index]->origin;
		// Set this to be sure
		pBot->dest_origin = pBot->wpt_origin;
		// FIXME
		// Do some error checks if we got a waypoint
	}
	// If time to get there expired get new one as well
	else if((pBot->f_wpt_timeset+3.0<gpGlobals->time) && (pBot->pBotEnemy == NULL))
	{
		DeleteSearchNodes(pBot);
		BotFindWaypoint(pBot);
		pBot->wpt_origin = paths[pBot->curr_wpt_index]->origin;
		// Set this to be sure
		pBot->dest_origin = pBot->wpt_origin;
	}
}


void CTBombPointClear(int iIndex)
{
	for(int i=0;i<MAXNUMBOMBSPOTS;i++)
	{
		if(g_rgiBombSpotsVisited[i]==-1)
		{
			g_rgiBombSpotsVisited[i] = iIndex;
			return;
		}
		else if(g_rgiBombSpotsVisited[i]==iIndex)
			return;
	}
}

// Little Helper routine to check if a Bomb Waypoint got visited
bool WasBombPointVisited(int iIndex)
{
#ifdef DEBUG
	report_log (LOG_BOTNAV, "WasBombPointVisited","Index=%d",iIndex);
#endif

	for(int i=0;i<MAXNUMBOMBSPOTS;i++)
	{
		if(g_rgiBombSpotsVisited[i]==-1)
			return FALSE;
		else if(g_rgiBombSpotsVisited[i]==iIndex)
			return TRUE;
	}
	return FALSE;
}

// Stores the Bomb Position as a Vector
Vector GetBombPosition()
{
	Vector vecBomb=Vector(0,0,0);
	edict_t *pent = NULL;

	while ((pent = UTIL_FindEntityByClassname( pent, "grenade" )) != NULL)
	{
		if(FStrEq(STRING(pent->v.model), "models/w_c4.mdl"))
		{
			vecBomb = pent->v.origin;
			break;
		}
	}
	assert(vecBomb != Vector(0,0,0));
	return(vecBomb);
}


// Returns if Bomb is hearable and if so the exact Position as a Vector
bool BotHearsBomb(Vector vecBotPos,Vector *pBombPos)
{
	if(g_vecBomb==Vector(0,0,0))
		g_vecBomb=GetBombPosition();

	float f_distance = (vecBotPos - g_vecBomb).Length();
	if(f_distance<BOMBMAXHEARDISTANCE)
	{
		*pBombPos = g_vecBomb;
		return TRUE;
	}
	return FALSE;
}

// Finds the Best Goal (Bomb) Waypoint for CTs when searching for a
// planted Bomb
int BotChooseBombWaypoint(bot_t *pBot)
{
	float min_distance=9999.0;
	float act_distance;
	int iGoal=0;

	edict_t *pent=NULL;
	edict_t *pEdict=pBot->pEdict;
	Vector vecPosition;

	bool bHearBombTicks=BotHearsBomb(pEdict->v.origin,&vecPosition);
	if(!bHearBombTicks)
		vecPosition=pEdict->v.origin;


	// Find nearest Goal Waypoint either to Bomb (if "heard" or Player)
	for(int i=0;i<g_iNumGoalPoints;i++)
	{
		act_distance=(paths[g_rgiGoalWaypoints[i]]->origin - vecPosition).Length();
		if(act_distance<min_distance)
		{
			min_distance=act_distance;
			iGoal=g_rgiGoalWaypoints[i];
		}
	}

	int iCount=0;
	while(WasBombPointVisited(iGoal))
	{
		if(g_iNumGoalPoints == 0)
			iGoal = g_rgiGoalWaypoints[0];
		else
			iGoal=g_rgiGoalWaypoints[RANDOM_LONG(0,g_iNumGoalPoints)];
		iCount++;
		if(iCount>g_iNumGoalPoints)
			break;
	}

#ifdef DEBUG
	report_log (LOG_BOTNAV, "BotChooseBombWaypoint","HearBomb=%d iGoal=%d",
		bHearBombTicks,iGoal);
#endif
	return(iGoal);
}


// Tries to find a good waypoint which
// a) has a Line of Sight to vecPosition and
// b) provides enough cover
// c) is far away from the defending position
int BotFindDefendWaypoint(bot_t *pBot,Vector vecPosition)
{
	int wpt_index[8];
	int min_distance[8];
	int iDistance,i;
	float fMin=9999.0;
	float fPosMin=9999.0;
	float fDistance;
	int iSourceIndex;
	int iPosIndex;

	edict_t *pEdict = pBot->pEdict;

	for (i=0; i < 8; i++)
	{
		wpt_index[i] = -1;
		min_distance[i] = 128;
	}

	// Get nearest waypoint to Bot & Position 
	for (i=0; i < g_iNumWaypoints; i++)
	{
		fDistance = (paths[i]->origin - pEdict->v.origin).Length();
		if (fDistance < fMin)
		{
			iSourceIndex = i;
			fMin = fDistance;
		}
		fDistance = (paths[i]->origin - vecPosition).Length();
		if (fDistance < fPosMin)
		{
			iPosIndex = i;
			fPosMin = fDistance;
		}
	}

	TraceResult tr;
	Vector vecSource;
	Vector vecDest = paths[iPosIndex]->origin;

	// Find Best Waypoint now
	for (i=0; i < g_iNumWaypoints; i++)
	{
		// Exclude Ladder, Goal, Rescue & current Waypoints 
		if((paths[i]->flags & W_FL_LADDER) ||
			(i==iSourceIndex) ||
			!WaypointIsVisible(i,iPosIndex))
		{
			continue;
		}

		// Use the 'real' Pathfinding Distances
		iDistance = GetPathDistance(iSourceIndex,i);
		
		if(iDistance<1024)
		{
			vecSource = paths[i]->origin;
			UTIL_TraceLine( vecSource, vecDest, ignore_monsters,
				ignore_glass,pEdict->v.pContainingEntity, &tr);
			if (tr.flFraction != 1.0)
				continue;

			if (iDistance > min_distance[0])
			{
				wpt_index[0] = i;
				min_distance[0] = iDistance;
			}
			else if (iDistance > min_distance[1])
			{
				wpt_index[1] = i;
				min_distance[1] = iDistance;
			}
			else if (iDistance > min_distance[2])
			{
				wpt_index[2] = i;
				min_distance[2] = iDistance;
			}
			else if (iDistance > min_distance[3])
			{
				wpt_index[3] = i;
				min_distance[3] = iDistance;
			}
			else if (iDistance > min_distance[4])
			{
				wpt_index[4] = i;
				min_distance[4] = iDistance;
			}
			else if (iDistance > min_distance[5])
			{
				wpt_index[5] = i;
				min_distance[5] = iDistance;
			}
			else if (iDistance > min_distance[6])
			{
				wpt_index[6] = i;
				min_distance[6] = iDistance;
			}
			else if (iDistance > min_distance[7])
			{
				wpt_index[7] = i;
				min_distance[7] = iDistance;
			}
		}
	}

	// Use statistics if we have them...
	if(g_bUseExperience)
	{
		for(i=0;i<8;i++)
		{
			if(wpt_index[i]!=-1)
			{
				if(pBot->iTeam == 0)
				{
					int iExp = (pBotExperienceData+(wpt_index[i]*g_iNumWaypoints)+wpt_index[i])->uTeam0Damage;
					iExp = (iExp * 100)/240;
					min_distance[i] = (iExp * 100)/8192;
					min_distance[i] += iExp;
					iExp >>= 1;
					iExp = (pBotExperienceData+(wpt_index[i]*g_iNumWaypoints)+wpt_index[i])->uTeam1Damage;
					iExp = (iExp * 100)/240;
					min_distance[i] = (iExp * 100)/8192;
					min_distance[i] += iExp;
				}
				else
				{
					int iExp = (pBotExperienceData+(wpt_index[i]*g_iNumWaypoints)+wpt_index[i])->uTeam1Damage;
					iExp = (iExp * 100)/240;
					min_distance[i] = (iExp * 100)/8192;
					min_distance[i] += iExp;
					iExp >>= 1;
					iExp = (pBotExperienceData+(wpt_index[i]*g_iNumWaypoints)+wpt_index[i])->uTeam0Damage;
					iExp = (iExp * 100)/240;
					min_distance[i] = (iExp * 100)/8192;
					min_distance[i] += iExp;
				}
			}
		}

	}
	// If not use old method of relying on the wayzone radius
	else
	{
		for(i=0;i<8;i++)
		{
			if(wpt_index[i]!=-1)
				min_distance[i]-=paths[wpt_index[i]]->Radius*3;
		}
	}

	int index1;
	int index2;
	int tempindex;
	bool bOrderchange;

	// Sort resulting Waypoints for farest distance
	do
	{
		bOrderchange=FALSE;
		for(i=0;i<3;i++)
		{
			index1=wpt_index[i];
			index2=wpt_index[i+1];
			if((index1!=-1) && (index2!=-1))
			{
				if(min_distance[i] > min_distance[i+1])
				{
					tempindex=wpt_index[i];
					wpt_index[i]=wpt_index[i+1];
					wpt_index[i+1]=tempindex;
					tempindex=min_distance[i];
					min_distance[i]=min_distance[i+1];
					min_distance[i+1]=tempindex;
					bOrderchange=TRUE;
				}
			}
		}
	} while(bOrderchange);

	for(i=0;i<8;i++)
	{
		if(wpt_index[i]!=-1)
			return (wpt_index[i]);
	}
	// Worst case:
	// If no waypoint was found, just use a random one
	return(RANDOM_LONG(0,g_iNumWaypoints-1));
}

// Tries to find a good Cover Waypoint if Bot wants to hide
int BotFindCoverWaypoint(bot_t *pBot,float maxdistance)
{
	int i;
	int wpt_index[8];
	int distance,enemydistance,iEnemyWPT;
	int rgiEnemyIndices[MAX_PATH_INDEX];
	int iEnemyNeighbours = 0;
	int min_distance[8];
	float f_min=9999.0;
	float f_enemymin=9999.0;
	Vector vecEnemy = pBot->vecLastEnemyOrigin;
	
	edict_t *pEdict=pBot->pEdict;
	int iSourceIndex=pBot->curr_wpt_index;

	// Get Enemies Distance
	// FIXME: Would be better to use the actual Distance returned
	// from Pathfinding because we might get wrong distances here
	float f_distance = (vecEnemy-pEdict->v.origin).Length();
	
	// Don't move to a position nearer the enemy
	if(maxdistance>f_distance)
		maxdistance=f_distance;
	if(maxdistance<300)
		maxdistance=300;

	for (i=0; i < 8; i++)
	{
		wpt_index[i] = -1;
		min_distance[i] = maxdistance;
	}


	// Get nearest waypoint to enemy & Bot 
	for (i=0; i < g_iNumWaypoints; i++)
	{
		f_distance = (paths[i]->origin - pEdict->v.origin).Length();
		if (f_distance < f_min)
		{
			iSourceIndex = i;
			f_min = f_distance;
		}
		f_distance = (paths[i]->origin - vecEnemy).Length();
		if (f_distance < f_enemymin)
		{
			iEnemyWPT = i;
			f_enemymin = f_distance;
		}
	}
	// Now Get Enemies Neigbouring Waypoints
	for(i=0;i<MAX_PATH_INDEX;i++)
	{
		if(paths[iEnemyWPT]->index[i] != -1)
		{
			rgiEnemyIndices[iEnemyNeighbours] = paths[iEnemyWPT]->index[i];
			iEnemyNeighbours++;
		}
	}

	pBot->curr_wpt_index = iSourceIndex;

	bool bNeighbourVisible;

	// Find Best Waypoint now
	for (i=0; i < g_iNumWaypoints; i++)
	{
		// Exclude Ladder, current Waypoints 
		if((paths[i]->flags & W_FL_LADDER) ||
			(i == iSourceIndex) ||
			WaypointIsVisible(iEnemyWPT,i))
		{
			continue;
		}

		// Now check neighbour Waypoints for Visibility
		bNeighbourVisible = FALSE;
		for(int j=0;j<iEnemyNeighbours;j++)
		{
			if(WaypointIsVisible(rgiEnemyIndices[j],i))
			{
				bNeighbourVisible = TRUE;
				break;
			}
		}
		if(bNeighbourVisible)
			continue;


		// Use the 'real' Pathfinding Distances
		distance = GetPathDistance(iSourceIndex,i);
		enemydistance = GetPathDistance(iEnemyWPT,i);
		
		if(distance<enemydistance)
		{
			if (distance < min_distance[0])
			{
				wpt_index[0] = i;
				min_distance[0] = distance;
			}
			else if (distance < min_distance[1])
			{
				wpt_index[1] = i;
				min_distance[1] = distance;
			}
			else if (distance < min_distance[2])
			{
				wpt_index[2] = i;
				min_distance[2] = distance;
			}
			else if (distance < min_distance[3])
			{
				wpt_index[3] = i;
				min_distance[3] = distance;
			}
			else if (distance < min_distance[4])
			{
				wpt_index[4] = i;
				min_distance[4] = distance;
			}
			else if (distance < min_distance[5])
			{
				wpt_index[5] = i;
				min_distance[5] = distance;
			}
			else if (distance < min_distance[6])
			{
				wpt_index[6] = i;
				min_distance[6] = distance;
			}
			else if (distance < min_distance[7])
			{
				wpt_index[7] = i;
				min_distance[7] = distance;
			}
		}
	}

	// Use statistics if we have them...
	if(g_bUseExperience)
	{
		if(pBot->iTeam == 0)
		{
			for(i=0;i<8;i++)
			{
				if(wpt_index[i]!=-1)
				{
					int iExp = (pBotExperienceData+(wpt_index[i]*g_iNumWaypoints)+wpt_index[i])->uTeam0Damage;
					iExp = (iExp * 100)/240;
					min_distance[i] = (iExp * 100)/8192;
					min_distance[i] += iExp;
					iExp >>= 1;
				}
			}
		}
		else
		{
			for(i=0;i<8;i++)
			{
				if(wpt_index[i]!=-1)
				{
					int iExp = (pBotExperienceData+(wpt_index[i]*g_iNumWaypoints)+wpt_index[i])->uTeam1Damage;
					iExp = (iExp * 100)/240;
					min_distance[i] = (iExp * 100)/8192;
					min_distance[i] += iExp;
					iExp >>= 1;
				}
			}
		}

	}
	// If not use old method of relying on the wayzone radius
	else
	{
		for(i=0;i<8;i++)
		{
			if(wpt_index[i]!=-1)
				min_distance[i]+=paths[wpt_index[i]]->Radius*3;
		}
	}

	int index1;
	int index2;
	int tempindex;
	bool bOrderchange;

	// Sort resulting Waypoints for nearest distance
	do
	{
		bOrderchange=FALSE;
		for(i=0;i<3;i++)
		{
			index1=wpt_index[i];
			index2=wpt_index[i+1];
			if((index1!=-1) && (index2!=-1))
			{
				if(min_distance[i] > min_distance[i+1])
				{
					tempindex=wpt_index[i];
					wpt_index[i]=wpt_index[i+1];
					wpt_index[i+1]=tempindex;
					tempindex=min_distance[i];
					min_distance[i]=min_distance[i+1];
					min_distance[i+1]=tempindex;
					bOrderchange=TRUE;
				}
			}
		}
	} while(bOrderchange);

	Vector v_source;
	Vector v_dest;
	TraceResult tr;

	// Take the first one which isn't spotted by the enemy
	for(i=0;i<8;i++)
	{
		if(wpt_index[i]!=-1)
		{
			v_source = pBot->vecLastEnemyOrigin+Vector(0,0,36);
			v_dest = paths[wpt_index[i]]->origin;
			UTIL_TraceLine( v_source, v_dest, ignore_monsters,
				ignore_glass,pEdict->v.pContainingEntity, &tr);
			if (tr.flFraction < 1.0)
			{
#ifdef DEBUG
	report_log (LOG_BOTNAV, "BotFindCoverWaypoint"," (covered) Index=%d",wpt_index[i]);
#endif
				return (wpt_index[i]);
			}
		}
	}

	// If all are seen by the enemy, take the first one 
	if(wpt_index[0]!=-1)
	{
#ifdef DEBUG
	report_log (LOG_BOTNAV, "BotFindCoverWaypoint"," (uncovered) Index=%d",wpt_index[0]);
#endif
		return (wpt_index[0]);
	}

	// Worst case:
	// If no waypoint was found, just use a random one
	return(RANDOM_LONG(0,g_iNumWaypoints-1));
}


// Checks if Waypoint A has a Connection to Waypoint Nr. B
inline bool IsConnectedWithWaypoint(int a,int b)
{
	int ix=0;
	while (ix < MAX_PATH_INDEX)
	{
		if(paths[a]->index[ix]==b)
			return TRUE;
		ix++;
	}
	return FALSE;
}


// Does a realtime postprocessing of Waypoints returned from
// Pathfinding, to vary Paths and find beest Waypoints on the way 
bool GetBestNextWaypoint(bot_t *pBot)
{
	int iNextWaypointIndex=pBot->pWaypointNodes->NextNode->iIndex;
	int iCurrentWaypointIndex=pBot->pWaypointNodes->iIndex;
	int iPrevWaypointIndex=pBot->curr_wpt_index;

	int ix = 0;
	int iUsedWaypoints[MAXBOTSNUMBER];
	int num;
	int c;
	edict_t *pEdict=pBot->pEdict;
	bot_t *pOtherBot;
	bool bWaypointInUse = FALSE;

	// Get waypoints used by other Bots
	for(c=0;c<MAXBOTSNUMBER;c++)
	{
		pOtherBot = &bots[c];
		if(pOtherBot->is_used)
		{
			if((IsAlive(pOtherBot->pEdict)) && (pOtherBot!= pBot))
				iUsedWaypoints[c] = pOtherBot->curr_wpt_index;
		}
	}
	for(c=0;c<MAXBOTSNUMBER;c++)
	{
		if(iUsedWaypoints[c] == iCurrentWaypointIndex)
		{
			bWaypointInUse = TRUE;
			break;
		}
	}

	if(bWaypointInUse)
	{
		while (ix < MAX_PATH_INDEX)
		{
			num=paths[iPrevWaypointIndex]->index[ix];
			if(num!=-1)
			{
				if(IsConnectedWithWaypoint(num,iNextWaypointIndex) &&
					IsConnectedWithWaypoint(num,iPrevWaypointIndex))
				{
					// Don't use ladder waypoints as alternative
					if(paths[num]->flags & W_FL_LADDER)
					{
						ix++;
						continue;
					}
					// check if this Waypoint is already in use by another bot
					bWaypointInUse = FALSE;
					for(c=0;c<MAXBOTSNUMBER;c++)
					{
						if(iUsedWaypoints[c] == num)
						{
							bWaypointInUse = TRUE;
							break;
						}
					}
					
					// Waypoint not used by another Bot - feel free to use it
					if(!bWaypointInUse)
					{
						pBot->pWaypointNodes->iIndex = num;
						return TRUE;
					}
				}
			}
			ix++;
		}
	}
	return FALSE;
}



bool IsOnLadder( edict_t *pEdict )
{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "IsOnLadder",NULL);
#endif
	return(pEdict->v.movetype == MOVETYPE_FLY);
}


// Advances in our Pathfinding list and sets the appropiate
// Destination Origins for this Bot
bool BotHeadTowardWaypoint( bot_t *pBot )
{
	Vector v_src, v_dest;
	TraceResult tr;
	edict_t *pEdict = pBot->pEdict;
	
#ifdef DEBUG
	report_log (LOG_BOTNAV, "BotHeadTowardWaypoint",NULL);
#endif
	
	
	// Check if old waypoints is still reliable 
	GetValidWaypoint(pBot);
	// No Waypoints from pathfinding ?  
	if(pBot->pWaypointNodes == NULL)
		return FALSE;
	
	// Advance in List
	pBot->pWaypointNodes = pBot->pWaypointNodes->NextNode;

	// Reset Travel Flags (jumping etc.)
	pBot->curr_travel_flags = 0;
	

	// We're not at the end of the List ?
	if(pBot->pWaypointNodes != NULL)
	{
		
		// If in between a route, postprocess the waypoint
		// (find better alternatives)
		if(pBot->pWaypointNodes != pBot->pWayNodesStart
		&& pBot->pWaypointNodes->NextNode != NULL)
		{
			GetBestNextWaypoint(pBot);
			int iTask = BotGetSafeTask(pBot)->iTask;
			
			pBot->fMinSpeed = pBot->f_max_speed;
			if(iTask == TASK_NORMAL && !g_bBombPlanted)
			{
				pBot->iCampButtons = 0;
				int iWaypoint = pBot->pWaypointNodes->NextNode->iIndex;
				float fKills;
				if(pBot->iTeam == 0)
					fKills = (pBotExperienceData+(iWaypoint*g_iNumWaypoints)+iWaypoint)->uTeam0Damage;
				else
					fKills = (pBotExperienceData+(iWaypoint*g_iNumWaypoints)+iWaypoint)->uTeam1Damage;

				if(fKills > 1 && g_fTimeRoundMid > gpGlobals->time && g_cKillHistory > 0)
				{
					fKills = (fKills * 100) / g_cKillHistory;
					fKills /= 100;
					switch(pBot->bot_personality)
					{
						// Psycho
					case 1:
						fKills /= 3;
						break;
					default:
						fKills /= 2;
					}

					if(pBot->fBaseAgressionLevel < fKills)
					{
						float fTime = g_fTimeRoundMid - gpGlobals->time;
						fTime = pBot->fFearLevel * fTime;
						// Push camp task on to stack
						bottask_t TempTask = {NULL,NULL,TASK_CAMP,TASKPRI_CAMP,-1,
							gpGlobals->time + fTime,TRUE};
						BotPushTask(pBot,&TempTask);
						// Push Move Command
						TempTask.iTask = TASK_MOVETOPOSITION;
						TempTask.fDesire = TASKPRI_MOVETOPOSITION;
						TempTask.iData = BotFindDefendWaypoint(pBot,paths[iWaypoint]->origin);
						BotPushTask(pBot,&TempTask);
						pBot->iCampButtons|=IN_DUCK;
					}
				}
				else if(g_bBotsCanPause && !pBot->bOnLadder
					&& !pBot->bInWater && pBot->curr_move_flags == 0
					&& (pEdict->v.flags & FL_ONGROUND))
				{
					if(fKills == pBot->fBaseAgressionLevel)
						pBot->iCampButtons |= IN_DUCK;
					else if(RANDOM_LONG(1,100) > pBot->bot_skill + RANDOM_LONG(1,20))
						pBot->fMinSpeed = 120.0;
				}
			}

		}

		if(pBot->pWaypointNodes != NULL)
		{

			int iDestIndex = pBot->pWaypointNodes->iIndex;
			
			// Find out about connection flags
			if(pBot->curr_wpt_index != -1)
			{
				PATH *p = paths[pBot->curr_wpt_index];
				int i = 0;
				
				while (i < MAX_PATH_INDEX)
				{
					if (p->index[i] == iDestIndex)
					{
						pBot->curr_travel_flags = p->connectflag[i];
						pBot->curr_move_flags = pBot->curr_travel_flags;
						pBot->vecDesiredVelocity = p->vecConnectVel[i];
						break;
					}
					i++;
				}
				
				// Bot not already on ladder ?
				// But will be soon ?
				if(paths[iDestIndex]->flags & W_FL_LADDER
					&& !pBot->bOnLadder)
				{
					int iLadderWaypoint = iDestIndex;
					
					bot_t *pOtherBot;
					int iBotsIndex = UTIL_GetBotIndex(pEdict);
					// Get ladder waypoints used by other (first moving) Bots
					for(int c=0;c<iBotsIndex;c++)
					{
						pOtherBot = &bots[c];
						if(pOtherBot->is_used)
						{
							if((IsAlive(pOtherBot->pEdict)) && (pOtherBot!= pBot))
							{
								// If another Bot uses this ladder, wait 3 secs
								if(pOtherBot->curr_wpt_index == iLadderWaypoint)
								{
									bottask_t TempTask = {NULL,NULL,TASK_PAUSE,TASKPRI_PAUSE,-1,
										gpGlobals->time+3.0,FALSE};
									BotPushTask(pBot,&TempTask);
									return TRUE;
								}
							}
						}
					}
				}
			}
			pBot->curr_wpt_index = iDestIndex;
		}
	}
	
	pBot->wpt_origin = paths[pBot->curr_wpt_index]->origin;

	// If wayzone radius != 0 vary origin a bit depending on
	// Body Angles
	if(paths[pBot->curr_wpt_index]->Radius != 0)
	{
		UTIL_MakeVectors(pEdict->v.angles);
		Vector v_x = pBot->wpt_origin + gpGlobals->v_right * RANDOM_FLOAT(-paths[pBot->curr_wpt_index]->Radius,paths[pBot->curr_wpt_index]->Radius);
		Vector v_y = pBot->wpt_origin + gpGlobals->v_forward * RANDOM_FLOAT(0,paths[pBot->curr_wpt_index]->Radius);
		pBot->wpt_origin = (v_x+v_y)/2;
	}

	pBot->f_wpt_timeset = gpGlobals->time;


	// Is it a Ladder ? Then we need to adjust the waypoint origin
	// to make sure Bot doesn't face wrong direction
	if(paths[pBot->curr_wpt_index]->flags & W_FL_LADDER)
	{
		char item_name[40];
		edict_t *pent = NULL;
		edict_t *pLadder = NULL;
		Vector entity_origin;
	
		// Find the Ladder Entity
		while ((pent = UTIL_FindEntityInSphere( pent, pBot->wpt_origin, 100 )) != NULL)
		{
			strcpy(item_name, STRING(pent->v.classname));
			if (strcmp("func_ladder", item_name) == 0)
			{
				// BModels have 0,0,0 for origin so must use VecBModelOrigin...
				entity_origin = VecBModelOrigin(pent);
				pBot->vecLadderOrigin = entity_origin;
				pLadder = pent;
			}
		}
		

		/* This Routine tries to find the direction the ladder is facing, so
		I can adjust the ladder wpt origin to be some units into the direction the Bot wants
		to move because otherwise the Bot would turn all the time while climbing.
		It does so by comparing distances between ladder & original wpt origin when applying different ladder angles, taking
		the nearest one.
		There probably is a better way to find out, but it works... 
		*/
		Vector vecDirWaypoint = Vector(0,0,0);

		if(!FNullEnt(pLadder))
		{

			// Use the ladder entity angles
			UTIL_MakeVectors(pLadder->v.angles);
			
			// Force ladder midpoint to be a wpt's height
			entity_origin.z = pBot->wpt_origin.z;
			
			float min_distance = 9999.0;
			
			// Check out all Directions
			Vector vecTempEnt = entity_origin+gpGlobals->v_forward*16;
			float f_distance = (vecTempEnt - pBot->wpt_origin).Length();
			if(f_distance<min_distance)
			{
				min_distance = f_distance;
				vecDirWaypoint = gpGlobals->v_forward;
			}
			vecTempEnt = entity_origin+(-gpGlobals->v_forward*16);
			f_distance = (vecTempEnt - pBot->wpt_origin).Length();
			if(f_distance<min_distance)
			{
				min_distance = f_distance;
				vecDirWaypoint = -gpGlobals->v_forward;
			}
			vecTempEnt = entity_origin+gpGlobals->v_right*16;
			f_distance = (vecTempEnt - pBot->wpt_origin).Length();
			if(f_distance<min_distance)
			{
				min_distance = f_distance;
				vecDirWaypoint = gpGlobals->v_right;
			}
			vecTempEnt = entity_origin+ (-gpGlobals->v_right*16);
			f_distance = (vecTempEnt - pBot->wpt_origin).Length();
			if(f_distance<min_distance)
			{
				min_distance = f_distance;
				vecDirWaypoint = -gpGlobals->v_right;
			}
		}

		// Here we've got the direction

		
		float fHelperZ = pBot->wpt_origin.z;

		if(pBot->wpt_origin.z < pEdict->v.origin.z)
		{
			// WPT is below Bot, so we need to climb down and need
			// to move wpt origin in the forward direction of the ladder
			pBot->ladder_dir = LADDER_DOWN;
			pBot->wpt_origin = pBot->wpt_origin+vecDirWaypoint*1;
		}
		else
		{
			// WPT is above Bot, so we need to climb up and need
			// to move wpt origin in the backward direction of the ladder
			pBot->ladder_dir = LADDER_UP;
			pBot->wpt_origin = pBot->wpt_origin+(-vecDirWaypoint*1);
		}

		// Restore original height of the wpt
		pBot->wpt_origin.z = fHelperZ;

		// Uncomment this to have a temp line showing the direction
		// to the ladder waypoint

		/*MESSAGE_BEGIN(MSG_ALL, SVC_TEMPENTITY);
		WRITE_BYTE( TE_SHOWLINE);
		WRITE_COORD(pEdict->v.origin.x);
		WRITE_COORD(pEdict->v.origin.y);
		WRITE_COORD(pEdict->v.origin.z);
		WRITE_COORD(pBot->wpt_origin.x);
		WRITE_COORD(pBot->wpt_origin.y);
		WRITE_COORD(pBot->wpt_origin.z);
		MESSAGE_END();*/
		
	}
	pBot->vecDirWaypoint = pBot->wpt_origin - pEdict->v.origin;
	pBot->vecDirWaypoint.z = 0.0;
	pBot->vecDirWaypoint = pBot->vecDirWaypoint.Normalize();
	return TRUE;
}



bool IsInWater( edict_t *pEdict)
{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "IsInWater",NULL);
#endif
	if ((pEdict->v.waterlevel == 2) ||
		(pEdict->v.waterlevel == 3))
	{
		return TRUE;
	}
	else
		return FALSE;
}



// Check if Bot is near to a Door
bool IsNearDoor(bot_t *pBot)
{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "IsNearDoor",NULL);
#endif
	edict_t *pent=NULL;
	edict_t *pEdict = pBot->pEdict;

	while ((pent = UTIL_FindEntityInSphere( pent, pEdict->v.origin, 100.0 )) != NULL)
	{
		if ((strcmp("func_door_rotating",STRING(pent->v.classname))==0) ||
		   (strcmp("func_door",STRING(pent->v.classname))==0))
		{
			if(BotEntityIsVisible(pBot,VecBModelOrigin(pent)))
				return TRUE;
		}

	}
	return FALSE;
}




// Not used at the moment - could be for unwaypointed movement
bool BotStuckInCorner( bot_t *pBot )
{
   TraceResult tr;
   Vector v_src, v_dest;
   edict_t *pEdict = pBot->pEdict;
   
   UTIL_MakeVectors( pEdict->v.v_angle );

   // trace 45 degrees to the right...
   v_src = pEdict->v.origin;
   v_dest = v_src + gpGlobals->v_forward*20 + gpGlobals->v_right*20;

   UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   if (tr.flFraction >= 1.0)
      return FALSE;  // no wall, so not in a corner

   // trace 45 degrees to the left...
   v_src = pEdict->v.origin;
   v_dest = v_src + gpGlobals->v_forward*20 - gpGlobals->v_right*20;

   UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   if (tr.flFraction >= 1.0)
      return FALSE;  // no wall, so not in a corner

   return TRUE;  // bot is in a corner
}


// Not used at the moment - could be for unwaypointed movement
void BotTurnAtWall( bot_t *pBot, TraceResult *tr )
{
   edict_t *pEdict = pBot->pEdict;
   Vector Normal;
   float Y, Y1, Y2, D1, D2, Z;

   // Find the normal vector from the trace result.  The normal vector will
   // be a vector that is perpendicular to the surface from the TraceResult.

   Normal = UTIL_VecToAngles(tr->vecPlaneNormal);

   // Since the bot keeps it's view angle in -180 < x < 180 degrees format,
   // and since TraceResults are 0 < x < 360, we convert the bot's view
   // angle (yaw) to the same format at TraceResult.

   Y = pEdict->v.v_angle.y;
   Y = Y + 180;
   if (Y > 359) Y -= 360;

   // Turn the normal vector around 180 degrees (i.e. make it point towards
   // the wall not away from it.  That makes finding the angles that the
   // bot needs to turn a little easier.

   Normal.y = Normal.y - 180;
   if (Normal.y < 0)
   Normal.y += 360;

   // Here we compare the bots view angle (Y) to the Normal - 90 degrees (Y1)
   // and the Normal + 90 degrees (Y2).  These two angles (Y1 & Y2) represent
   // angles that are parallel to the wall surface, but heading in opposite
   // directions.  We want the bot to choose the one that will require the
   // least amount of turning (saves time) and have the bot head off in that
   // direction.

   Y1 = Normal.y - 90;
   if (RANDOM_LONG(1, 100) <= 50)
   {
      Y1 = Y1 - RANDOM_FLOAT(5.0, 20.0);
   }
   if (Y1 < 0) Y1 += 360;

   Y2 = Normal.y + 90;
   if (RANDOM_LONG(1, 100) <= 50)
   {
      Y2 = Y2 + RANDOM_FLOAT(5.0, 20.0);
   }
   if (Y2 > 359) Y2 -= 360;

   // D1 and D2 are the difference (in degrees) between the bot's current
   // angle and Y1 or Y2 (respectively).

   D1 = abs(Y - Y1);
   if (D1 > 179) D1 = abs(D1 - 360);
   D2 = abs(Y - Y2);
   if (D2 > 179) D2 = abs(D2 - 360);

   // If difference 1 (D1) is more than difference 2 (D2) then the bot will
   // have to turn LESS if it heads in direction Y1 otherwise, head in
   // direction Y2.  I know this seems backwards, but try some sample angles
   // out on some graph paper and go through these equations using a
   // calculator, you'll see what I mean.

   if (D1 > D2)
      Z = Y1;
   else
      Z = Y2;

   // convert from TraceResult 0 to 360 degree format back to bot's
   // -180 to 180 degree format.

   if (Z > 180)
      Z -= 360;

   // set the direction to head off into...
   pEdict->v.ideal_yaw = Z;
}





// Checks if Bot is blocked in his movement direction (excluding doors)
bool BotCantMoveForward( bot_t *pBot, Vector vNormal,TraceResult *tr )
{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotCantMoveForward",NULL);
#endif

   edict_t *pEdict = pBot->pEdict;


   // use some TraceLines to determine if anything is blocking the current
   // path of the bot.

   Vector v_src, v_forward, v_center;

   v_center=pEdict->v.angles;
   v_center.z=0;
   v_center.x=0;
   UTIL_MakeVectors(v_center);

   // first do a trace from the bot's eyes forward...

   v_src = pEdict->v.origin + pEdict->v.view_ofs;  // EyePosition()
   v_forward = v_src + vNormal * 24;


   // trace from the bot's eyes straight forward...
   UTIL_TraceLine( v_src, v_forward, ignore_monsters,
                   pEdict->v.pContainingEntity, tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
	   if ((strcmp("func_door_rotating",STRING(tr->pHit->v.classname))==0) ||
		   (strcmp("func_door",STRING(tr->pHit->v.classname))==0))
		   return FALSE;
	   else
			return TRUE;  // bot's head will hit something
   }

   // bot's head is clear, check at shoulder level...
   // trace from the bot's shoulder left diagonal forward to the right shoulder...
   v_src = pEdict->v.origin + pEdict->v.view_ofs + Vector(0, 0, -16) + gpGlobals->v_right * -16;
   v_forward= pEdict->v.origin + pEdict->v.view_ofs + Vector(0, 0, -16) + gpGlobals->v_right * 16 + vNormal * 24;


   UTIL_TraceLine( v_src, v_forward, ignore_monsters,
                   pEdict->v.pContainingEntity, tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
	if ((strcmp("func_door_rotating",STRING(tr->pHit->v.classname))!=0) ||
		(strcmp("func_door",STRING(tr->pHit->v.classname))!=0))
		return TRUE;  // bot's body will hit something
   }

   // bot's head is clear, check at shoulder level...
   // trace from the bot's shoulder right diagonal forward to the left shoulder...
   v_src = pEdict->v.origin + pEdict->v.view_ofs + Vector(0, 0, -16) + gpGlobals->v_right * 16;
   v_forward= pEdict->v.origin + pEdict->v.view_ofs + Vector(0, 0, -16) + gpGlobals->v_right * -16 + vNormal * 24;


   UTIL_TraceLine( v_src, v_forward, ignore_monsters,
                   pEdict->v.pContainingEntity, tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
	if ((strcmp("func_door_rotating",STRING(tr->pHit->v.classname))!=0) ||
		(strcmp("func_door",STRING(tr->pHit->v.classname))!=0))
		return TRUE;  // bot's body will hit something
   }


   // Now check below Waist

   if (pEdict->v.flags & FL_DUCKING)
   {
	   v_src = pEdict->v.origin + Vector(0, 0, -19 + 19);
	   v_forward = v_src + Vector(0,0,10)+vNormal * 24;
	   

	   UTIL_TraceLine( v_src, v_forward, ignore_monsters,
		   pEdict->v.pContainingEntity, tr);
	   
	   // check if the trace hit something...
	   if (tr->flFraction < 1.0)
	   {
		   if ((strcmp("func_door_rotating",STRING(tr->pHit->v.classname))!=0) ||
			   (strcmp("func_door",STRING(tr->pHit->v.classname))!=0))
			   return TRUE;  // bot's body will hit something
	   }
	   v_src = pEdict->v.origin + Vector(0, 0, -19 + 19);
	   v_forward = v_src + vNormal * 24;
	   

	   UTIL_TraceLine( v_src, v_forward, ignore_monsters,
		   pEdict->v.pContainingEntity, tr);
	   
	   // check if the trace hit something...
	   if (tr->flFraction < 1.0)
	   {
		   if ((strcmp("func_door_rotating",STRING(tr->pHit->v.classname))!=0) ||
			   (strcmp("func_door",STRING(tr->pHit->v.classname))!=0))
			   return TRUE;  // bot's body will hit something
	   }
   }
   else
   {
	   // Trace from the left Waist to the right forward Waist Pos
	   v_src = pEdict->v.origin + Vector(0, 0, -17) + gpGlobals->v_right * -16;
	   v_forward = pEdict->v.origin + Vector(0, 0, -17) + gpGlobals->v_right * 16 + vNormal * 24;
	   

	   // trace from the bot's waist straight forward...
	   UTIL_TraceLine( v_src, v_forward, ignore_monsters,
		   pEdict->v.pContainingEntity, tr);
	   
	   // check if the trace hit something...
	   if (tr->flFraction < 1.0)
	   {
		   if ((strcmp("func_door_rotating",STRING(tr->pHit->v.classname))!=0) ||
			   (strcmp("func_door",STRING(tr->pHit->v.classname))!=0))
			   return TRUE;  // bot's body will hit something
	   }
	   // Trace from the left Waist to the right forward Waist Pos
	   v_src = pEdict->v.origin + Vector(0, 0, -17) + gpGlobals->v_right * 16;
	   v_forward = pEdict->v.origin + Vector(0, 0, -17) + gpGlobals->v_right * -16 + vNormal * 24;
	   

	   UTIL_TraceLine( v_src, v_forward, ignore_monsters,
		   pEdict->v.pContainingEntity, tr);
	   
	   // check if the trace hit something...
	   if (tr->flFraction < 1.0)
	   {
		   if ((strcmp("func_door_rotating",STRING(tr->pHit->v.classname))!=0) ||
			   (strcmp("func_door",STRING(tr->pHit->v.classname))!=0))
			   return TRUE;  // bot's body will hit something
	   }
   }

   return FALSE;  // bot can move forward, return false
}


// Check if Bot can move sideways
bool BotCanStrafeLeft(bot_t *pBot, TraceResult *tr )
{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotCanStrafeLeft",NULL);
#endif

	edict_t *pEdict = pBot->pEdict;


   Vector v_src, v_left;

   UTIL_MakeVectors( pEdict->v.v_angle );
   v_src = pEdict->v.origin;
   v_left = v_src + gpGlobals->v_right * -40;
   // trace from the bot's waist straight left...
   UTIL_TraceLine( v_src, v_left, ignore_monsters,
                   pEdict->v.pContainingEntity, tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
      return FALSE;  // bot's body will hit something
   }
   v_left=v_left+ gpGlobals->v_forward * 40;
   // trace from the strafe pos straight forward...
   UTIL_TraceLine( v_src, v_left, ignore_monsters,
                   pEdict->v.pContainingEntity, tr);
   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
      return FALSE;  // bot's body will hit something
   }
   return TRUE;
}

bool BotCanStrafeRight(bot_t *pBot, TraceResult *tr )
{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotCanStrafeRight",NULL);
#endif

   edict_t *pEdict = pBot->pEdict;


   Vector v_src, v_right;

   UTIL_MakeVectors( pEdict->v.v_angle );
   v_src = pEdict->v.origin;
   v_right = v_src + gpGlobals->v_right * 40;
   // trace from the bot's waist straight right...
   UTIL_TraceLine( v_src, v_right, ignore_monsters,
                   pEdict->v.pContainingEntity, tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
      return FALSE;  // bot's body will hit something
   }
   v_right=v_right + gpGlobals->v_forward * 40;
   // trace from the strafe pos straight forward...
   UTIL_TraceLine( v_src, v_right, ignore_monsters,
                   pEdict->v.pContainingEntity, tr);
   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
      return FALSE;  // bot's body will hit something
   }
   return TRUE;
}


// Check if Bot can jump over some obstacle
bool BotCanJumpUp( bot_t *pBot,Vector vNormal )
{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotCanJumpUp",NULL);
#endif

	TraceResult tr;
	Vector v_jump, v_source, v_dest;
	edict_t *pEdict = pBot->pEdict;

	// Can't jump if not on ground and not on ladder/swimming 
	if(((pEdict->v.flags & FL_ONGROUND)==0)
		&& (pBot->bOnLadder || !pBot->bInWater))
	{
		return FALSE;
	}

   // convert current view angle to vectors for TraceLine math...
	
	v_jump = pEdict->v.angles;
	v_jump.x = 0;  // reset pitch to 0 (level horizontally)
	v_jump.z = 0;  // reset roll to 0 (straight up and down)
	
	UTIL_MakeVectors( v_jump );
	
	// Check for normal jump height first...
	
	v_source = pEdict->v.origin + Vector(0, 0, -36 + 45);
	v_dest = v_source + vNormal * 32;
	
	// trace a line forward at maximum jump height...
	UTIL_TraceLine( v_source, v_dest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);

	if (tr.flFraction < 1.0)
		goto CheckDuckJump;
	else
	{
		// now trace from jump height upward to check for obstructions...
		v_source=v_dest;
		v_dest.z =v_dest.z + 37;

		UTIL_TraceLine( v_source, v_dest, ignore_monsters,
			pEdict->v.pContainingEntity, &tr);
		
		if (tr.flFraction < 1.0)
			return FALSE;
	}


	// now check same height to one side of the bot...
	v_source = pEdict->v.origin + gpGlobals->v_right * 16 + Vector(0, 0, -36 + 45);
	v_dest = v_source + vNormal * 32;
	
	// trace a line forward at maximum jump height...
	UTIL_TraceLine( v_source, v_dest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	
	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0)
		goto CheckDuckJump;
	// now trace from jump height upward to check for obstructions...
	v_source=v_dest;
	v_dest.z =v_dest.z + 37;
	UTIL_TraceLine( v_source, v_dest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	
	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0)
		return FALSE;

	
	// now check same height on the other side of the bot...
	v_source = pEdict->v.origin + (-gpGlobals->v_right * 16) + Vector(0, 0, -36 + 45);
	v_dest = v_source + vNormal * 32;
	
	// trace a line forward at maximum jump height...
	UTIL_TraceLine( v_source, v_dest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	
	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0)
		goto CheckDuckJump;
	
	// now trace from jump height upward to check for obstructions...
	v_source=v_dest;
	v_dest.z =v_dest.z + 37;
	UTIL_TraceLine( v_source, v_dest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	
	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0)
		return FALSE;
	pBot->bUseDuckJump=TRUE;
	return TRUE;


	// Here we check if a Duck Jump would work...
CheckDuckJump:	
	// use center of the body first...
	
	// maximum duck jump height is 62, so check one unit above that (63)
	v_source = pEdict->v.origin + Vector(0, 0, -36 + 63);
	v_dest = v_source + vNormal * 32;
	
	// trace a line forward at maximum jump height...
	UTIL_TraceLine( v_source, v_dest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);

	if (tr.flFraction < 1.0)
		return FALSE;
	else
	{
		// now trace from jump height upward to check for obstructions...
		v_source=v_dest;
		v_dest.z =v_dest.z + 37;
		UTIL_TraceLine( v_source, v_dest, ignore_monsters,
			pEdict->v.pContainingEntity, &tr);
		
		// if trace hit something, check duckjump
		if (tr.flFraction < 1.0)
			return FALSE;
	}


	// now check same height to one side of the bot...
	v_source = pEdict->v.origin + gpGlobals->v_right * 16 + Vector(0, 0, -36 + 63);
	v_dest = v_source + vNormal * 32;
	
	// trace a line forward at maximum jump height...
	UTIL_TraceLine( v_source, v_dest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	
	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0)
		return FALSE;
	// now trace from jump height upward to check for obstructions...
	v_source=v_dest;
	v_dest.z =v_dest.z + 37;
	UTIL_TraceLine( v_source, v_dest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	
	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0)
		return FALSE;

	
	// now check same height on the other side of the bot...
	v_source = pEdict->v.origin + (-gpGlobals->v_right * 16) + Vector(0, 0, -36 + 63);
	v_dest = v_source + vNormal * 32;
	
	// trace a line forward at maximum jump height...
	UTIL_TraceLine( v_source, v_dest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	
	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0)
		return FALSE;
	
	// now trace from jump height upward to check for obstructions...
	v_source=v_dest;
	v_dest.z =v_dest.z + 37;
	UTIL_TraceLine( v_source, v_dest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	
	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0)
		return FALSE;
	pBot->bUseDuckJump=TRUE;
	return TRUE;
}


// Check if Bot can duck under Obstacle
bool BotCanDuckUnder( bot_t *pBot,Vector vNormal )
{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotCanDuckUnder",NULL);
#endif

	TraceResult tr;
	Vector v_duck, v_baseheight,v_source, v_dest;
	edict_t *pEdict = pBot->pEdict;
	
	// convert current view angle to vectors for TraceLine math...
	
	v_duck = pEdict->v.angles;
	v_duck.x = 0;  // reset pitch to 0 (level horizontally)
	v_duck.z = 0;  // reset roll to 0 (straight up and down)
	
	UTIL_MakeVectors( v_duck );
	
	// use center of the body first...
	
	if (pEdict->v.flags & FL_DUCKING)
		v_baseheight = pEdict->v.origin+Vector(0, 0, -17);
	else
		v_baseheight = pEdict->v.origin;
	v_source = v_baseheight;
	v_dest = v_source + vNormal * 32;
	
	// trace a line forward at duck height...
	UTIL_TraceLine( v_source, v_dest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0)
		return FALSE;
	
	// now check same height to one side of the bot...
	v_source = v_baseheight + gpGlobals->v_right * 16;
	v_dest = v_source + vNormal * 32;
	
	// trace a line forward at duck height...
	UTIL_TraceLine( v_source, v_dest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0)
		return FALSE;
	
	// now check same height on the other side of the bot...
	v_source = v_baseheight + (-gpGlobals->v_right * 16);
	v_dest = v_source + vNormal * 32;
	
	// trace a line forward at duck height...
	UTIL_TraceLine( v_source, v_dest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	
	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0)
		return FALSE;
	
	return TRUE;
}


// Check if Bot can still follow a User
bool BotFollowUser( bot_t *pBot )
{
   bool user_visible;
   float f_distance;

   if(pBot->pBotUser==NULL)
	   return FALSE;
   if (!IsAlive( pBot->pBotUser ))
   {
      // the bot's user is dead!
      pBot->pBotUser = NULL;
      return FALSE;
   }

   edict_t *pEdict = pBot->pEdict;

   Vector vecUser;

   user_visible = FVisible(pBot->pBotUser->v.origin,pEdict);

   // check if the "user" is still visible or if the user has been visible
   // in the last 5 seconds (or the player just starting "using" the bot)

   if (user_visible || (pBot->f_bot_use_time + 5 > gpGlobals->time))
   {
      if (user_visible)
         pBot->f_bot_use_time = gpGlobals->time;  // reset "last visible time"

      Vector v_user = pBot->pBotUser->v.origin - pEdict->v.origin;
      f_distance = v_user.Length( );  // how far away is the "user"?
	  if(v_user.z > -1)
		  pBot->ladder_dir=LADDER_DOWN;
	  else
		  pBot->ladder_dir=LADDER_UP;

      if (f_distance > 150)      // run if distance to enemy is far
         pBot->f_move_speed = pBot->f_max_speed;
      else if (f_distance > 80)  // don't move
	  {
         pBot->f_move_speed = 0.0;
	  }
      else
	  {
         pBot->f_move_speed = -pBot->f_max_speed;
	  }
#ifdef DEBUG
	report_log (LOG_BOTNAV, "BotFollowUser",NULL);
#endif
      return TRUE;
   }
   else
   {
      // person to follow has gone out of sight...
      pBot->pBotUser = NULL;

      return FALSE;
   }
}

bool BotIsBlockedLeft( bot_t *pBot )
{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotIsBlockedLeft",NULL);
#endif

	edict_t *pEdict = pBot->pEdict;
	Vector v_src, v_left;
	TraceResult tr;
	
	int iDirection=48;
	
	if(pBot->f_move_speed<0)
		iDirection=-48;
	UTIL_MakeVectors( pEdict->v.angles );
	
	// do a trace to the left...
	
	v_src = pEdict->v.origin;
	v_left = v_src + (gpGlobals->v_forward*iDirection)+(gpGlobals->v_right * -48);  // 48 units to the left
	
	UTIL_TraceLine( v_src, v_left, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	
	// check if the trace hit something...
	if (tr.flFraction < 1.0)
	{
	   if ((strcmp("func_door_rotating",STRING(tr.pHit->v.classname))==0) ||
		   (strcmp("func_door",STRING(tr.pHit->v.classname))==0))
		   return FALSE;
	   else
		   return TRUE;
	}
	
	return FALSE;
}


bool BotIsBlockedRight( bot_t *pBot )
{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotIsBlockedRight",NULL);
#endif

	edict_t *pEdict = pBot->pEdict;
	Vector v_src, v_left;
	TraceResult tr;
	
	int iDirection=48;
	
	if(pBot->f_move_speed<0)
		iDirection=-48;
	UTIL_MakeVectors( pEdict->v.angles );
	
	// do a trace to the left...
	
	v_src = pEdict->v.origin;
	v_left = v_src + (gpGlobals->v_forward*iDirection)+(gpGlobals->v_right * 48);  // 48 units to the right
	
	UTIL_TraceLine( v_src, v_left, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	
	// check if the trace hit something...
	if (tr.flFraction < 1.0)
	{
	   if ((strcmp("func_door_rotating",STRING(tr.pHit->v.classname))==0) ||
		   (strcmp("func_door",STRING(tr.pHit->v.classname))==0))
		   return FALSE;
	   else
		   return TRUE;
	}
	
	return FALSE;
}


bool BotCheckWallOnLeft( bot_t *pBot )
{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotCheckWallOnLeft",NULL);
#endif
   edict_t *pEdict = pBot->pEdict;
   Vector v_src, v_left;
   TraceResult tr;

   UTIL_MakeVectors( pEdict->v.angles );

   // do a trace to the left...

   v_src = pEdict->v.origin;
   v_left = v_src + (-gpGlobals->v_right * 40);  // 40 units to the left

   UTIL_TraceLine( v_src, v_left, ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
   {
      if (pBot->f_wall_on_left < 1.0)
         pBot->f_wall_on_left = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}


bool BotCheckWallOnRight( bot_t *pBot )
{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotCheckWallOnRight",NULL);
#endif

   edict_t *pEdict = pBot->pEdict;
   Vector v_src, v_right;
   TraceResult tr;

   UTIL_MakeVectors( pEdict->v.angles );

   // do a trace to the right...

   v_src = pEdict->v.origin;
   v_right = v_src + gpGlobals->v_right * 40;  // 40 units to the right

   UTIL_TraceLine( v_src, v_right, ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
   {
      if (pBot->f_wall_on_right < 1.0)
         pBot->f_wall_on_right = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}

// Check if View on last Enemy Position is
// blocked - replace with better Vector then
// Mostly used for getting a good Camping Direction
// Vector if not camping on a camp waypoint 
void BotGetCampDirection(bot_t *pBot,Vector *vecDest)
{
	TraceResult tr;
	edict_t *pEdict=pBot->pEdict;

	Vector vecSource = pEdict->v.origin + pEdict->v.view_ofs;  // EyePosition()

	UTIL_TraceLine( vecSource, *vecDest, ignore_monsters,
			pEdict->v.pContainingEntity, &tr);
				
	// check if the trace hit something...
	if (tr.flFraction < 1.0)
	{
		float f_length=(tr.vecEndPos-vecSource).Length();
		if(f_length>100)
			return;
		float distance1,distance2;
		float min_distance1=9999.0;
		float min_distance2=9999.0;
		int indexbot,indexenemy,i;

		// Find Nearest Waypoint to Bot and Position
		for (i=0; i < g_iNumWaypoints; i++)
		{
			distance1 = (paths[i]->origin - pEdict->v.origin).Length();
			
			if (distance1 < min_distance1)
			{
				min_distance1=distance1;
				indexbot=i;
			}
			distance2 = (paths[i]->origin - *vecDest).Length();
			
			if (distance2 < min_distance2)
			{
				min_distance2=distance2;
				indexenemy=i;
			}
		}

		min_distance1=9999.0;
		PATH *p=paths[indexbot];
		int iLookAtWaypoint=-1;
		for(i=0;i<MAX_PATH_INDEX;i++)
		{
			if(p->index[i]==-1)
				continue;
			distance1 = GetPathDistance(p->index[i],indexenemy);
			
			if (distance1 < min_distance1)
			{
				min_distance1=distance1;
				iLookAtWaypoint=i;
			}
		}
		if(iLookAtWaypoint!=-1 && iLookAtWaypoint<g_iNumWaypoints)
			*vecDest=paths[iLookAtWaypoint]->origin;
	}
}

// Inserts the Radio Message into the Message Queue
void BotPlayRadioMessage(bot_t *pBot,int iMessage)
{
#ifdef DEBUG
	report_log (LOG_BOTBASE, "BotPlayRadioMessage","Message=%d",iMessage);
#endif
	pBot->iRadioSelect = iMessage;
	BotPushMessageQueue(pBot,MSG_CS_RADIO);
}


// Checks and executes pending Messages 
void BotCheckMessageQueue(bot_t *pBot)
{
	// No new message ?
	if(pBot->iActMessageIndex == pBot->iPushMessageIndex)
		return;
	// Bot not started yet ?
	if(pBot->need_to_initialize)
		return;

	// Get Message from Stack
	int iCurrentMSG = BotGetMessageQueue(pBot);
	
	// Nothing to do ?
	if(iCurrentMSG == MSG_CS_IDLE)
		return;

	char szMenuSelect[40];
	edict_t *pEdict = pBot->pEdict;
	int team = UTIL_GetTeam(pBot->pEdict);

#ifdef DEBUG
	report_log (LOG_BOTBASE, "BotCheckMessageQueue","MSG=%d",iCurrentMSG);
#endif
	switch(iCurrentMSG)
	{
		// General Buy Message
	case MSG_CS_BUY:
//		fp=fopen("bot.txt","a"); fprintf(fp, "MSG_CS_BUY (Pre): %s - iBuyCount=%d\n",pBot->name,pBot->iBuyCount); fclose(fp);
		// Buy Weapon
		if(pBot->f_spawn_time+2.0>gpGlobals->time)
		//	if(pBot->bInBuyzone==FALSE)
		{
			// Keep sending Message if we're not in Buyzone
			BotPushMessageQueue(pBot,MSG_CS_BUY);
		    return;
		}

		// ERROR: Cancel buying
		if(pBot->iBuyCount>6)
			return;

		// If Fun-Mode no need to buy
		if(g_bJasonMode)
		{
			pBot->iBuyCount = 7;
			pBot->bBuyingFinished = TRUE;
			SelectWeaponByName(pBot,"weapon_knife");
			return;
		}

		
		// Prevent VIP from buying
		if(g_iMapType==MAP_AS)
		{
			char *infobuffer;
			char model_name[32];
			
			infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)( pEdict );
			strcpy(model_name, (g_engfuncs.pfnInfoKeyValue(infobuffer, "model")));
			if ((strcmp(model_name, "vip") == 0))  // VIP ?
			{
				pBot->bIsVIP = TRUE;
				pBot->bBuyingFinished = TRUE;
				pBot->iBuyCount = 7;
				return;
			}

		}

		BotPushMessageQueue(pBot,MSG_CS_IDLE);

#ifdef DEBUG
	report_log (LOG_BOTBASE, "BotCheckMessageQueue","Choosing Weapon");
#endif
		// Needs to choose a prefered Weapon ?
		if(pBot->iBuyCount==0)
		{
			int iCount = 0;
			int iFoundWeapons = 0;
			int iBuyChoices[NUM_WEAPONS];
			if(pBot->bot_money>650)
			{
				// Select the Priority Tab for this Personality
				int* ptrWeaponTab = ptrWeaponPrefs[pBot->bot_personality];
				// Start from most desired Weapon
				ptrWeaponTab+=NUM_WEAPONS;

				do
				{
					ptrWeaponTab--;
					assert(*ptrWeaponTab>-1 && *ptrWeaponTab<NUM_WEAPONS); 
					pBot->pSelected_weapon = &cs_weapon_select[*ptrWeaponTab];
					iCount++;
					// Weapon available for every Team ?
					if(g_iMapType==MAP_AS)
					{
						if(pBot->pSelected_weapon->iTeamAS!=2)
						{
							if(pBot->pSelected_weapon->iTeamAS != team)
								continue;
						}
					}
					else
					{
						if(pBot->pSelected_weapon->iTeamStandard != 2)
						{
							if(pBot->pSelected_weapon->iTeamStandard != team)
								continue;
						}
					}
					if(pBot->pSelected_weapon->iPrice<=pBot->bot_money)
					{
//						assert(*ptrWeaponTab<7);
						iBuyChoices[iFoundWeapons++] = *ptrWeaponTab;
					}
				} while(iCount<NUM_WEAPONS && iFoundWeapons<4);
			}

			// Found a desired weapon ?
			if(iFoundWeapons>0)
			{
				int iChosenWeapon;
				// Choose randomly from the best ones...
				if(iFoundWeapons>1)
					iChosenWeapon = iBuyChoices[RANDOM_LONG(0,iFoundWeapons-1)];
				else
					iChosenWeapon = iBuyChoices[iFoundWeapons-1];
				pBot->pSelected_weapon = &cs_weapon_select[iChosenWeapon];
				pBot->iBuyCount = 1;
#ifdef DEBUG
	report_log (LOG_BOTBASE, "BotCheckMessageQueue","Chosen Weapon:%s",&cs_weapon_select[iChosenWeapon].weapon_name);
#endif
			}
			// No choice, quit buying weapon
			else
			{
				pBot->pSelected_weapon = NULL;
				pBot->iBuyCount = 3;
			}
		}
		FakeClientCommand(pEdict, "buy",NULL, NULL);
		break;

	// General Radio Message issued
	case MSG_CS_RADIO:

		// If last Bot Radio Command (global) happened just a
		// second ago, delay response
		if(g_rgfLastRadioTime[team]+1.0<gpGlobals->time)
		{
			// If same message like previous just do a yes/no 
			if(pBot->iRadioSelect != RADIO_AFFIRMATIVE
				&& pBot->iRadioSelect != RADIO_NEGATIVE
				&& pBot->iRadioSelect != RADIO_REPORTINGIN)
			{
				if(pBot->iRadioSelect == g_rgfLastRadio[team] &&
					g_rgfLastRadioTime[team]+1.5>gpGlobals->time)
				{
					pBot->iRadioSelect = RADIO_AFFIRMATIVE;
				}
				else
					g_rgfLastRadio[team] = pBot->iRadioSelect;
			}

			if(pBot->iRadioSelect == RADIO_REPORTINGIN)
			{

				char szReport[80];
				int iTask = BotGetSafeTask(pBot)->iTask;
				int iWPT = pBot->pTasks->iData;
				
				switch(iTask)
				{
				case TASK_NORMAL:
					if(iWPT != -1)
					{
						if(paths[iWPT]->flags & W_FL_GOAL)
							sprintf(szReport,"Heading for a Map Goal!\n");
						else if(paths[iWPT]->flags & W_FL_RESCUE)
							sprintf(szReport,"Heading to Rescue Point\n");
						else if(paths[iWPT]->flags & W_FL_CAMP)
							sprintf(szReport,"Moving to Camp Spot\n");
						else
							sprintf(szReport,"Roaming around\n");
					}
					else
						sprintf(szReport,"Roaming around\n");
					break;
				case TASK_MOVETOPOSITION:
					sprintf(szReport,"Moving to position\n");
					break;
				case TASK_FOLLOWUSER:
					if(pBot->pBotUser != NULL)
						sprintf(szReport,"Following %s\n",STRING(pBot->pBotUser->v.netname));
					break;
				case TASK_WAITFORGO:
					sprintf(szReport,"Waiting for GO!\n");
					break;
				case TASK_CAMP:
					sprintf(szReport,"Camping...\n");
					break;
				case TASK_PLANTBOMB:
					sprintf(szReport,"Planting the Bomb!\n");
					break;
				case TASK_DEFUSEBOMB:
					sprintf(szReport,"Defusing the Bomb!\n");
					break;
				case TASK_ATTACK:
					if(pBot->pBotEnemy != NULL)
						sprintf(szReport,"Attacking %s\n",STRING(pBot->pBotEnemy->v.netname));
					break;
				case TASK_ENEMYHUNT:
					if(pBot->pLastEnemy != NULL)
						sprintf(szReport,"Hunting %s\n",STRING(pBot->pLastEnemy->v.netname));
					break;
				case TASK_SEEKCOVER:
					sprintf(szReport,"Fleeing from Battle\n");
					break;
				case TASK_HIDE:
					sprintf(szReport,"Hiding from Enemy\n");
					break;
				default:
					sprintf(szReport,"Nothing special here...\n");
					break;
				}
				
				UTIL_TeamSayText(szReport,pBot);
			}
			
			if(pBot->iRadioSelect<RADIO_GOGOGO)
				FakeClientCommand(pBot->pEdict, "radio1", NULL, NULL);
			else if(pBot->iRadioSelect<RADIO_AFFIRMATIVE)
			{
				pBot->iRadioSelect-=(RADIO_GOGOGO-1);
				FakeClientCommand(pBot->pEdict, "radio2", NULL, NULL);
			}
			else
			{
				pBot->iRadioSelect-=(RADIO_AFFIRMATIVE-1);
				FakeClientCommand(pBot->pEdict, "radio3", NULL, NULL);
			}
			// Store last radio usage
			g_rgfLastRadioTime[team] = gpGlobals->time;
		}
		else
			BotPushMessageQueue(pBot,MSG_CS_RADIO);
		break;

	// Select correct Menu Item for this Radio Message
	case MSG_CS_RADIO_SELECT:
		itoa(pBot->iRadioSelect,&szMenuSelect[0],10);
		szMenuSelect[1]=0;
		FakeClientCommand(pEdict, "menuselect",&szMenuSelect[0], NULL);
		break;

		// Team independant Saytext
	case MSG_CS_SAY:
		UTIL_SayText(pBot->szMiscStrings,pBot);
		break;
			// Team dependant Saytext
	case MSG_CS_TEAMSAY:
		UTIL_TeamSayText(pBot->szMiscStrings,pBot);
		break;
	case MSG_CS_IDLE:
	default:
		return;
	}
}


// Does all the work in selecting correct Buy Menus
// for most Weapons/Items
void BotBuyStuff(bot_t *pBot)
{
	char szMenuSelect[40];
	edict_t *pEdict = pBot->pEdict;

#ifdef DEBUG
	report_log (LOG_BOTBASE, "BotBuyStuff","Action=%d",pBot->BuyAction);
#endif
	// Needs a Weapon ?
	if(pBot->BuyAction == MSG_CS_BUYWEAPON_SELECT)
	{
		pBot->BuyAction = MSG_CS_IDLE;
		if(pBot->pSelected_weapon == NULL || pBot->iBuyCount>1)
		{
#ifdef _DEBUG
			fp=fopen("PODERROR.txt","a"); fprintf(fp, "ERROR no Weapon selected: MSG_CS_BUYWEAPON_GROUPSELECT: %s - iBuyCount=%d\n",pBot->name,pBot->iBuyCount); fclose(fp);
#endif
			return;
		}
//		fp=fopen("bot.txt","a"); fprintf(fp, "MSG_CS_BUYWEAPON_GROUPSELECT: %s - iBuyCount=%d\n",pBot->name,pBot->iBuyCount); fclose(fp);
		pBot->iBuyCount = 2;
		sprintf(szMenuSelect,"%d",pBot->pSelected_weapon->iBuyGroup);
		FakeClientCommand(pEdict, "menuselect",szMenuSelect, NULL);
		return;
	}

	if(pBot->BuyAction == MSG_CS_WEAPONBUY)
	{
		pBot->BuyAction = MSG_CS_IDLE;
		if((pBot->pSelected_weapon == NULL) || (pBot->iBuyCount != 2))
		{
			fp=fopen("bot.txt","a"); fprintf(fp, "ERROR no Weapon selected: MSG_CS_BUYWEAPON_PSSRM: %s - iBuyCount=%d\n",pBot->name,pBot->iBuyCount); fclose(fp);
			return;
		}
//		fp=fopen("bot.txt","a"); fprintf(fp, "MSG_CS_BUYWEAPON_PSSRM: %s - iBuyCount=%d\n",pBot->name,pBot->iBuyCount); fclose(fp);
		pBot->iBuyCount=3;
		sprintf(szMenuSelect,"%d",pBot->pSelected_weapon->iBuySelect);
		FakeClientCommand(pEdict, "menuselect",szMenuSelect, NULL);
		BotPushMessageQueue(pBot,MSG_CS_BUY);
		return;
	}

	// Needs Ammo ?
	if(pBot->BuyAction==MSG_CS_BUYAMMO)
	{
		pBot->BuyAction=MSG_CS_IDLE;
		if(BotHasPrimaryWeapon(pBot)==FALSE) // Pistol ?
			FakeClientCommand(pEdict, "menuselect","7", NULL);
		else
			FakeClientCommand(pEdict, "menuselect","6", NULL);
//		fp=fopen("bot.txt","a"); fprintf(fp, "MSG_CS_BUYAMMO: %s - iBuyCount=%d\n",pBot->name,pBot->iBuyCount); fclose(fp);
		pBot->iBuyCount=4;
		BotPushMessageQueue(pBot,MSG_CS_BUY);
		return;
	}

	// Needs an Item ?
	if(pBot->BuyAction==MSG_CS_BUYITEM)
	{
		pBot->BuyAction=MSG_CS_IDLE;
//		fp=fopen("bot.txt","a"); fprintf(fp, "MSG_CS_BUYITEM: %s - iBuyCount=%d\n",pBot->name,pBot->iBuyCount); fclose(fp);
		pBot->iBuyCount++;
		FakeClientCommand(pEdict, "menuselect","8", NULL);
		return;
	}

	if(pBot->BuyAction==MSG_CS_BUYITEM_SELECT)
	{
		pBot->BuyAction=MSG_CS_IDLE;
//		fp=fopen("bot.txt","a"); fprintf(fp, "MSG_CS_BUYITEM_SELECT: %s - iBuyCount=%d\n",pBot->name,pBot->iBuyCount); fclose(fp);
		
		// Care first about buying Armor
		if(pBot->iBuyCount==5)
		{
			if((pBot->bot_armor!=0) || (pBot->bot_money<650))
				strcpy(&szMenuSelect[0],"0");
			else if(pBot->bot_money>1000)
				strcpy(&szMenuSelect[0],"2");
			else
				strcpy(&szMenuSelect[0],"1");

		}
		// Buy Grenade or Defuse Kit
		else
		{
			pBot->bInBuyzone=FALSE;
			if(pBot->bot_money<300)
				strcpy(&szMenuSelect[0],"0");
			else
			{
				int iGrenadeType=RANDOM_LONG(1,100);
				// Focus on HE Grenades
				if(iGrenadeType<70)
					iGrenadeType=4;
				else if(iGrenadeType<90)
					iGrenadeType=5;
				else if(iGrenadeType<100)
					iGrenadeType=3;

				// If Defusion Map & Counter buy Defusion Kit instead
				if((g_iMapType==MAP_DE) && (UTIL_GetTeam(pEdict)==1))
				{
					if(iGrenadeType==5 || iGrenadeType==3)
						iGrenadeType=6;
				}
				sprintf(&szMenuSelect[0],"%d",iGrenadeType);
			}
			pBot->bBuyingFinished=TRUE;
		}
		pBot->iBuyCount++;
		FakeClientCommand(pEdict, "menuselect",&szMenuSelect[0], NULL);
		BotPushMessageQueue(pBot,MSG_CS_BUY);
		return;
	}

}


// Called after each End of the Round to update knowledge about
// the most dangerous waypoints for each Team.
void UpdateGlobalExperienceData()
{
	// No waypoints, no experience used or waypoints edited ?
	if(g_iNumWaypoints<1 || !g_bUseExperience ||
		g_bWaypointsChanged)
	{
		return;
	}

#ifdef DEBUG
	report_log (LOG_BOTBASE, "UpdateGlobalExperienceData","g_iNumWaypoints=%d g_bUseExperience=%d g_bWaypointsChanged=%d",
		g_iNumWaypoints,g_bUseExperience,g_bWaypointsChanged);
#endif
	unsigned short min_damage;
	unsigned short act_damage;
	int iBestIndex,i,j;
	bool bRecalcKills=FALSE;


	// Get the most dangerous Waypoint for this Position
	// for Team 0
	for (i=0; i < g_iNumWaypoints; i++)
	{
		min_damage=0;
		iBestIndex=-1;
		for(j=0; j<g_iNumWaypoints; j++)
		{
			if(i == j)
				continue;
			act_damage=(pBotExperienceData+(i*g_iNumWaypoints)+j)->uTeam0Damage;
			if(act_damage>min_damage)
			{
				min_damage=act_damage;
				iBestIndex=j;
			}
		}

		if(min_damage>MAX_DAMAGE_VAL)
			bRecalcKills=TRUE;
		(pBotExperienceData+(i*g_iNumWaypoints)+i)->iTeam0_danger_index=(short)iBestIndex;
	}

	// Get the most dangerous Waypoint for this Position
	// for Team 1
	for (i=0; i < g_iNumWaypoints; i++)
	{
		min_damage=0;
		iBestIndex=-1;
		for(j=0; j<g_iNumWaypoints; j++)
		{
			if(i == j)
				continue;
			act_damage=(pBotExperienceData+(i*g_iNumWaypoints)+j)->uTeam1Damage;
			if(act_damage>min_damage)
			{
				min_damage=act_damage;
				iBestIndex=j;
			}
		}

		if(min_damage>=MAX_DAMAGE_VAL)
			bRecalcKills=TRUE;

		(pBotExperienceData+(i*g_iNumWaypoints)+i)->iTeam1_danger_index=(short)iBestIndex;
	}

	// Adjust Values if overflow is about to happen
	if(bRecalcKills)
	{
		int iClip;

		for (i=0; i < g_iNumWaypoints; i++)
		{
			for (j=0; j < g_iNumWaypoints; j++)
			{
				if(i == j)
					continue;
				iClip=(pBotExperienceData+(i*g_iNumWaypoints)+j)->uTeam0Damage;
				iClip-=MAX_DAMAGE_VAL/2;
				if(iClip<0)
					iClip=0;
				(pBotExperienceData+(i*g_iNumWaypoints)+j)->uTeam0Damage=(unsigned short)iClip;
				iClip=(pBotExperienceData+(i*g_iNumWaypoints)+j)->uTeam1Damage;
				iClip-=MAX_DAMAGE_VAL/2;
				if(iClip<0)
					iClip=0;
				(pBotExperienceData+(i*g_iNumWaypoints)+j)->uTeam1Damage=(unsigned short)iClip;
			}
		}
	}

	g_cKillHistory++;
	if(g_cKillHistory == MAX_KILL_HIST)
	{
		unsigned char byShift = gpGlobals->maxClients / 2;
		for(i=0;i<g_iNumWaypoints;i++)
		{
			(pBotExperienceData+(i*g_iNumWaypoints)+i)->uTeam0Damage /= byShift;
			(pBotExperienceData+(i*g_iNumWaypoints)+i)->uTeam1Damage /= byShift;
		}
		g_cKillHistory = 1;
	}


#ifdef DEBUG
	report_log (LOG_BOTBASE, "UpdateGlobalExperienceData","Done!");
#endif
}


// Gets called each time a Bot gets damaged by some enemy.
// Tries to achieve a statistic about most/less dangerous waypoints
// for a destination goal used for pathfinding
void BotCollectGoalExperience(bot_t *pBot,int iDamage, int iTeam)
{
	if(g_iNumWaypoints<1 || !g_bUseExperience ||
		g_bWaypointsChanged
		|| pBot->chosengoal_index<0 || pBot->prev_goal_index <0)
	{
		return;
	}

	// Only rate Goal Waypoint if Bot died because of the damage
	// FIXME: Could be done a lot better, however this cares most
	// about damage done by sniping or really deadly weapons
	if(pBot->bot_health-iDamage<=0)
	{
		int iWPTValue;
		if(iTeam==0)
		{
			iWPTValue=(pBotExperienceData+(pBot->chosengoal_index*g_iNumWaypoints)+pBot->prev_goal_index)->wTeam0Value;
			iWPTValue -= pBot->bot_health/20;
			if(iWPTValue < -MAX_GOAL_VAL)
				iWPTValue = -MAX_GOAL_VAL;
			else if(iWPTValue > MAX_GOAL_VAL)
				iWPTValue = MAX_GOAL_VAL;
			(pBotExperienceData+(pBot->chosengoal_index*g_iNumWaypoints)+pBot->prev_goal_index)->wTeam0Value=(signed short)iWPTValue;
		}
		else
		{
			iWPTValue=(pBotExperienceData+(pBot->chosengoal_index*g_iNumWaypoints)+pBot->prev_goal_index)->wTeam1Value;
			iWPTValue-=pBot->bot_health/20;
			if(iWPTValue<-MAX_GOAL_VAL)
				iWPTValue=-MAX_GOAL_VAL;
			else if(iWPTValue>MAX_GOAL_VAL)
				iWPTValue=MAX_GOAL_VAL;
			(pBotExperienceData+(pBot->chosengoal_index*g_iNumWaypoints)+pBot->prev_goal_index)->wTeam1Value=(signed short)iWPTValue;
		}
	}
}

// Gets called each time a Bot gets damaged by some enemy.
// Stores the damage (teamspecific) done to the Victim
// FIXME: Should probably rate damage done by humans higher...
void BotCollectExperienceData(edict_t *pVictimEdict,edict_t *pAttackerEdict, int iDamage)
{
	if(pVictimEdict==NULL || pAttackerEdict==NULL || !g_bUseExperience)
		return;
	int iVictimTeam=UTIL_GetTeam(pVictimEdict);
	int iAttackerTeam=UTIL_GetTeam(pAttackerEdict);

	if(iVictimTeam==iAttackerTeam)
		return;

	bot_t *pBot=UTIL_GetBotPointer(pVictimEdict);

	// If these are Bots also remember damage to rank the
	// destination of the Bot
	if(pBot!=NULL)
		pBot->f_goal_value-=iDamage;
	pBot=UTIL_GetBotPointer(pAttackerEdict);
	if(pBot!=NULL)
		pBot->f_goal_value+=iDamage;

	float distance;
	float min_distance_victim=9999.0;
	float min_distance_attacker=9999.0;

	int VictimIndex;
	int AttackerIndex;


	// Find Nearest Waypoint to Attacker/Victim
	for (int i=0; i < g_iNumWaypoints; i++)
	{
		distance = (paths[i]->origin - pVictimEdict->v.origin).Length();
		
		if (distance < min_distance_victim)
		{
			min_distance_victim=distance;
			VictimIndex=i;
		}
		distance = (paths[i]->origin - pAttackerEdict->v.origin).Length();
		if (distance < min_distance_attacker)
		{
			min_distance_attacker=distance;
			AttackerIndex=i;
		}
	}

	if(pVictimEdict->v.health - iDamage < 0)
	{
		if(iVictimTeam == 0)
			(pBotExperienceData+(VictimIndex*g_iNumWaypoints)+VictimIndex)->uTeam0Damage++;
		else
			(pBotExperienceData+(VictimIndex*g_iNumWaypoints)+VictimIndex)->uTeam1Damage++;
		if(	(pBotExperienceData+(VictimIndex*g_iNumWaypoints)+VictimIndex)->uTeam0Damage > 240)
			(pBotExperienceData+(VictimIndex*g_iNumWaypoints)+VictimIndex)->uTeam0Damage = 240;
		if(	(pBotExperienceData+(VictimIndex*g_iNumWaypoints)+VictimIndex)->uTeam1Damage > 240)
			(pBotExperienceData+(VictimIndex*g_iNumWaypoints)+VictimIndex)->uTeam1Damage = 240;
	}

	if(VictimIndex == AttackerIndex)
		return;

	int iValue;

	// Store away the damage done
	if(iVictimTeam==0)
	{
		iValue=(pBotExperienceData+(VictimIndex*g_iNumWaypoints)+AttackerIndex)->uTeam0Damage;
		iValue+=((float)iDamage/10.0);
		if(iValue>MAX_DAMAGE_VAL)
			iValue=MAX_DAMAGE_VAL;
		(pBotExperienceData+(VictimIndex*g_iNumWaypoints)+AttackerIndex)->uTeam0Damage=(unsigned short)iValue;
	}
	else
	{
		iValue=(pBotExperienceData+(VictimIndex*g_iNumWaypoints)+AttackerIndex)->uTeam1Damage;
		iValue+=((float)iDamage/10.0);
		if(iValue>MAX_DAMAGE_VAL)
			iValue=MAX_DAMAGE_VAL;
		(pBotExperienceData+(VictimIndex*g_iNumWaypoints)+AttackerIndex)->uTeam1Damage=(unsigned short)iValue;
	}
}


// Carried out each Frame.
// Does all of the sensing, calculates Emotions and finally
// sets the desired Action after applying all of the Filters
void BotSetConditions(bot_t *pBot)
{
#ifdef DEBUG
	report_log (LOG_BOTTASK, "BotSetConditions",NULL);
#endif
	float f_distance;
	edict_t *pEdict=pBot->pEdict;
	int bot_team=UTIL_GetTeam(pEdict);
	bool bUsingGrenade = pBot->bUsingGrenade;

	pBot->iAimFlags = 0;


	// Slowly increase/decrease dynamic Emotions back to their
	// Base Level
	if(pBot->fNextEmotionUpdate < gpGlobals->time)
	{
		if(pBot->fAgressionLevel>pBot->fBaseAgressionLevel)
			pBot->fAgressionLevel-=0.05;
		else
			pBot->fAgressionLevel+=0.05;
		if(pBot->fFearLevel>pBot->fBaseFearLevel)
			pBot->fFearLevel-=0.05;
		else
			pBot->fFearLevel+=0.05;

		if(pBot->fAgressionLevel < 0.0)
			pBot->fAgressionLevel = 0.0;
		if(pBot->fFearLevel < 0.0)
			pBot->fFearLevel = 0.0;

#ifdef DEBUG
	report_log (LOG_BOTTASK, "BotSetConditions","Emotions Update - BaseFear=%f CurrFear=%f BaseAgr=%f CurrAgr=%f",
		pBot->fBaseFearLevel,pBot->fFearLevel,pBot->fBaseAgressionLevel,pBot->fAgressionLevel);
#endif
		pBot->fNextEmotionUpdate = gpGlobals->time+0.5;
	}

	// Does Bot see an Enemy ?
	if(BotFindEnemy( pBot ))
	{
		pBot->iStates |= STATE_SEEINGENEMY;
	}
	else
	{
		pBot->iStates &= ~STATE_SEEINGENEMY;
		pBot->pBotEnemy=NULL;
	}

	// Did Bot just kill an Enemy ?
	if(pBot->pLastVictim!=NULL)
	{
		if(UTIL_GetTeam(pBot->pLastVictim)!=bot_team)
		{
			// Add some agression because we just killed somebody
			// MUWHAHA !!
			pBot->fAgressionLevel+=0.1;
			if(pBot->fAgressionLevel>1.0)
				pBot->fAgressionLevel=1.0;

			// Taunt Enemy if we feel like it
			int iRandomMessage=RANDOM_LONG(0,100);
			if(g_bBotChat)
			{
				if(iRandomMessage<20)
				{
					STRINGNODE *pTempNode=NULL;
damnsafetycheck:
					do
					{
						pTempNode=GetNodeString(pKillChat,RANDOM_LONG(0,iNumKillChats-1));
					} while(pTempNode==NULL);
					if(pTempNode->pszString==NULL)
						goto damnsafetycheck;

					BotPrepareChatMessage(pBot,pTempNode->pszString);
					BotPushMessageQueue(pBot,MSG_CS_SAY);
				}
			}

			// Sometimes give some radio message
			if(iRandomMessage<50)
				BotPlayRadioMessage(pBot,RADIO_ENEMYDOWN);
		}
		pBot->pLastVictim=NULL;
	}

	// Check if our current enemy is still valid
	if(pBot->pLastEnemy != NULL && !pBot->pLastEnemy->free)
	{
		if(!IsAlive(pBot->pLastEnemy) && pBot->f_shootatdead_time < gpGlobals->time)
		{
			pBot->vecLastEnemyOrigin=Vector(0,0,0);
			pBot->pLastEnemy=NULL;
		}
	}
	else
	{
		pBot->pLastEnemy = NULL;
		pBot->vecLastEnemyOrigin = Vector(0,0,0);
	}


	bool bCheckNoiseOrigin=FALSE;

	// Check sounds of other players
	// FIXME: Hearing is done by simulating and aproximation
	// Need to check if hooking the Server Playsound Routines
	// wouldn't give better results because the current method
	// is far too sensitive and unreliable

	// Don't listen if seeing enemy, just checked for sounds or
	// being blinded ('cause its inhuman)
	if ((pBot->f_sound_update_time <= gpGlobals->time) &&
		(pBot->pBotEnemy == NULL) && (pBot->f_blind_time < gpGlobals->time))
	{
		int ind;
		edict_t *pPlayer;
		heartab_t DistTab[32];

		int TabIndex=0;
		Vector v_distance;
		float fDistance;
		pBot->f_sound_update_time = gpGlobals->time + g_fTimeSoundUpdate;

		// Let Hearing be affected by Movement Speed
		float fSensitivity=pEdict->v.velocity.Length2D();
		if(fSensitivity!=0.0)
		{
			fSensitivity/=240.0;
			fSensitivity=-fSensitivity;
		}
		fSensitivity+=1.5;

		// If Bot just shot, half hearing Sensibility
		if(pEdict->v.oldbuttons & IN_ATTACK)
			fSensitivity*=0.5;

		// Setup Engines Potentially Audible Set for this Bot
		Vector vecOrg = pEdict->v.origin + pEdict->v.view_ofs;
		if ( pEdict->v.flags & FL_DUCKING )
		{
			vecOrg = vecOrg + ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
		}
		unsigned char *pas = ENGINE_SET_PAS ( (float *)&vecOrg );

		// Loop through all enemy clients to check for hearable stuff
		for (ind = 0; ind < gpGlobals->maxClients; ind++)
		{
			if((ThreatTab[ind].IsUsed==FALSE) || (ThreatTab[ind].IsAlive==FALSE) ||
				(ThreatTab[ind].iTeam==bot_team) ||	(ThreatTab[ind].pEdict==pEdict) ||
				(ThreatTab[ind].fTimeSoundLasting < gpGlobals->time))
				continue;
			
			pPlayer = ThreatTab[ind].pEdict;

			// Despite its name it also checks for sounds...
			// NOTE: This only checks if sounds could be heard from
			// this position in theory but doesn't care for Volume or
			// real Sound Events. Even if there's no noise it returns true,
			// so we still have the work of simulating sound levels
			if(!ENGINE_CHECK_VISIBILITY(pPlayer, pas ) )
				continue;
			
			fDistance = (ThreatTab[ind].vecSoundPosition - pEdict->v.origin).Length();
			if(fDistance > ThreatTab[ind].fHearingDistance)
				continue;
			DistTab[TabIndex].distance = fDistance;
			DistTab[TabIndex].pEdict = pPlayer;
			TabIndex++;
		}
		
		// Did the Bot hear someone ?
		if(TabIndex!=0)
		{
			pBot->f_heard_sound_time = gpGlobals->time;

			// Did Bot hear more than 1 player ?
			if(TabIndex>1)
			{
				// Sort Distances of Enemies and take
				// the nearest one
				
				heartab_t temptab;
				bool bSorting;
				do
				{
					bSorting=FALSE;
					for(int j=0;j<(TabIndex-1);j++)
					{
						if(DistTab[j].distance>DistTab[j+1].distance)
						{
							temptab=DistTab[j];
							DistTab[j]=DistTab[j+1];
							DistTab[j+1]=temptab;
							bSorting=TRUE;
						}
					}
				} while(bSorting);
			}
			
			// Change to best weapon if heard something
			if(pBot->current_weapon.iId == CS_WEAPON_KNIFE
				&& !g_bJasonMode) // Knife and not JasonMode ?
			{
				BotSelectBestWeapon(pBot);
			}
			pBot->iStates |= STATE_HEARINGENEMY;
			pPlayer=DistTab[0].pEdict;

			// Didn't Bot already have an enemy ? Take this one...
			if(pBot->vecLastEnemyOrigin == Vector(0,0,0))
			{
				pBot->pLastEnemy = pPlayer;
				pBot->vecLastEnemyOrigin = pPlayer->v.origin;
			}
			// Bot had an enemy, check if it's the heard one
			else
			{
				if(pPlayer == pBot->pLastEnemy)
				{
					// Bot sees enemy ? then bail out !
					if(pBot->iStates & STATE_SEEINGENEMY)
						goto endhearing;
					pBot->vecLastEnemyOrigin = pPlayer->v.origin;
				}
				else
				{
					// If Bot had an enemy but the heard one is nearer, take it instead
					f_distance=(pBot->vecLastEnemyOrigin - pEdict->v.origin).Length();
					if((f_distance>(pPlayer->v.origin - pEdict->v.origin).Length())
						&& (pBot->f_bot_see_enemy_time+2.0<gpGlobals->time))
					{
						pBot->pLastEnemy=pPlayer;
						pBot->vecLastEnemyOrigin=pPlayer->v.origin;
					}
					else
						goto endhearing;
				}
			}

			unsigned char cHit;
			Vector vecVisPos;
			
			// Check if heard enemy can be seen
			if(FBoxVisible(pEdict,pPlayer,&vecVisPos,&cHit))
			{
				pBot->pBotEnemy = pPlayer;
				pBot->pLastEnemy = pPlayer;
				pBot->vecLastEnemyOrigin = vecVisPos;
				pBot->ucVisibility = cHit;
				pBot->iStates |= STATE_SEEINGENEMY;
				pBot->f_bot_see_enemy_time = gpGlobals->time;
			}
			// Check if heard enemy can be shoot through some obstacle
			else
			{
				if((pBot->pLastEnemy==pPlayer) &&
					(pBot->f_bot_see_enemy_time+3.0 > gpGlobals->time))
				{
					int iShootThruFreq=BotAimTab[pBot->bot_skill/20].iHeardShootThruProb;
					if(g_bShootThruWalls && WeaponShootsThru(pBot->current_weapon.iId) &&
						(RANDOM_LONG(1,100)<iShootThruFreq))
					{
						if(IsShootableThruObstacle(pEdict,pPlayer->v.origin))
						{
							pBot->pBotEnemy = pPlayer;
							pBot->pLastEnemy = pPlayer;
							pBot->vecLastEnemyOrigin = pPlayer->v.origin;
							pBot->iStates |= STATE_SEEINGENEMY;
							pBot->iStates |= STATE_SUSPECTENEMY;
							pBot->f_bot_see_enemy_time=gpGlobals->time;
						}
					}
				}
			}
		}
	}
	else
		pBot->iStates &= ~STATE_HEARINGENEMY;

endhearing:

	if(pBot->pBotEnemy == NULL && pBot->pLastEnemy != NULL)
	{
		pBot->iAimFlags |= AIM_PREDICTPATH;
		if(BotEntityIsVisible(pBot,pBot->vecLastEnemyOrigin))
			pBot->iAimFlags |= AIM_LASTENEMY;
	}

	
	// Check if throwing a Grenade is a good thing to do...
	if(pBot->f_grenade_check_time < gpGlobals->time && !bUsingGrenade
	&& !pBot->bIsReloading && pBot->current_weapon.iId != CS_WEAPON_INSWITCH)
	{
		// Check again in some seconds
		pBot->f_grenade_check_time = gpGlobals->time + g_fTimeGrenadeUpdate;

		if(pBot->pLastEnemy != NULL && IsAlive(pBot->pLastEnemy)
		&& !(pBot->iStates & STATE_SEEINGENEMY))
		{
			// Check if we have Grenades to throw
			int iGrenadeType=BotCheckGrenades(pBot);

			// If we don't have grenades no need to check
			// it this round again
			if(iGrenadeType == -1)
				pBot->f_grenade_check_time = gpGlobals->time+9999.0;
			
			if(iGrenadeType != -1)
			{
				edict_t *pEnemy = pBot->pLastEnemy;
				Vector v_distance = pBot->vecLastEnemyOrigin - pEdict->v.origin;
				f_distance = v_distance.Length();
				// Too high to throw ?
				if(pEnemy->v.origin.z > pEdict->v.origin.z + 500.0)
					f_distance=9999;

				// Enemy within a good Throw distance ?				
				if(f_distance > 600 && f_distance < 1800)
				{
					Vector v_enemypos;
					Vector vecSource;
					Vector v_dest;
					TraceResult tr;
					bool bThrowGrenade = TRUE;
					int iThrowIndex;
					Vector vecPredict;
					int rgi_WaypointTab[4];
					int iIndexCount;
					float fRadius;
					
					// Care about different Grenades
					switch(iGrenadeType)
					{
					case CS_WEAPON_HEGRENADE:
						if(NumTeammatesNearPos(pBot,pBot->pLastEnemy->v.origin,256) > 0)
							bThrowGrenade = FALSE;
						else
						{
							vecPredict = pBot->pLastEnemy->v.velocity * 0.5;
							vecPredict.z = 0.0;
							vecPredict = vecPredict + pBot->pLastEnemy->v.origin;
							fRadius = pBot->pLastEnemy->v.velocity.Length2D();
							if(fRadius < 128)
								fRadius = 128;
							iIndexCount = 4;
							WaypointFindInRadius(vecPredict,fRadius,&rgi_WaypointTab[0],&iIndexCount);
							while(iIndexCount > -1)
							{
								bThrowGrenade = TRUE;
								pBot->vecThrow = paths[rgi_WaypointTab[iIndexCount--]]->origin;
								vecSource = VecCheckThrow ( pEdict, GetGunPosition(pEdict), pBot->vecThrow, 1.0);
								if(vecSource == g_vecZero)
									vecSource = VecCheckToss ( pEdict, GetGunPosition(pEdict), pBot->vecThrow);
								if(vecSource == g_vecZero)
									bThrowGrenade = FALSE;
								else
								{
									break;
								}
							}
						}

						// Start throwing ?
						if(bThrowGrenade)
							pBot->iStates |= STATE_THROWHEGREN;
						else
							pBot->iStates &= ~STATE_THROWHEGREN;
						break;
						
					case CS_WEAPON_FLASHBANG:
						vecPredict = pBot->pLastEnemy->v.velocity * 0.5;
						vecPredict.z = 0.0;
						vecPredict = vecPredict + pBot->pLastEnemy->v.origin;
						iThrowIndex = WaypointFindNearestToMove(vecPredict);
						pBot->vecThrow = paths[iThrowIndex]->origin;
						if(NumTeammatesNearPos(pBot,pBot->vecThrow,256) > 0)
							bThrowGrenade = FALSE;
						if(bThrowGrenade)
						{
							vecSource = VecCheckThrow ( pEdict, GetGunPosition(pEdict), pBot->vecThrow, 1.0);
							if(vecSource == g_vecZero)
								vecSource = VecCheckToss ( pEdict, GetGunPosition(pEdict), pBot->vecThrow);
							if(vecSource == g_vecZero)
								bThrowGrenade = FALSE;
						}
						if(bThrowGrenade)
							pBot->iStates |= STATE_THROWFLASHBANG;
						else
							pBot->iStates &= ~STATE_THROWFLASHBANG;
						break;
						
					case CS_WEAPON_SMOKEGRENADE:
						// Check if Enemy is directly facing us
						// Don't throw if that's the case !!
						if(bThrowGrenade && pBot->pBotEnemy != NULL) 
						{
							if(GetShootingConeDeviation(pBot->pBotEnemy,&pEdict->v.origin) >= 0.9)
								bThrowGrenade = FALSE;
						}
						if(bThrowGrenade)
							pBot->iStates |= STATE_THROWSMOKEGREN;
						else
							pBot->iStates &= ~STATE_THROWSMOKEGREN;
						break;
					}
					bottask_t TempTask = {NULL,NULL,TASK_THROWHEGRENADE,TASKPRI_THROWGRENADE,-1,
						gpGlobals->time + TIME_GRENPRIME,FALSE};
					
					if(pBot->iStates & STATE_THROWHEGREN)
					{
						BotPushTask(pBot,&TempTask);
					}
					else if(pBot->iStates & STATE_THROWFLASHBANG)
					{
						TempTask.iTask = TASK_THROWFLASHBANG;
						BotPushTask(pBot,&TempTask);
					}
					else if(pBot->iStates & STATE_THROWSMOKEGREN)
					{
						TempTask.iTask = TASK_THROWSMOKEGRENADE;
						BotPushTask(pBot,&TempTask);
					}
				}
				else
				{
					pBot->iStates &= ~STATE_THROWHEGREN;
					pBot->iStates &= ~STATE_THROWFLASHBANG;
					pBot->iStates &= ~STATE_THROWSMOKEGREN;
				}
			}
			else
			{
				pBot->iStates &= ~STATE_THROWHEGREN;
				pBot->iStates &= ~STATE_THROWFLASHBANG;
				pBot->iStates &= ~STATE_THROWSMOKEGREN;
			}
		}
	}
	else
	{
		pBot->iStates &= ~STATE_THROWHEGREN;
		pBot->iStates &= ~STATE_THROWFLASHBANG;
		pBot->iStates &= ~STATE_THROWSMOKEGREN;
	}

	// Check if there are Items needing to be used/collected
	if(pBot->f_itemcheck_time<gpGlobals->time
		|| pBot->pBotPickupItem!=NULL)
	{
		pBot->f_itemcheck_time=gpGlobals->time+g_fTimePickupUpdate;
		BotFindItem(pBot);
	}

	float fTempFear = pBot->fFearLevel;
	float fTempAgression = pBot->fAgressionLevel;

	// Decrease Fear if Teammates near 
	int iFriendlyNum=0;
	if(pBot->vecLastEnemyOrigin != Vector(0,0,0))
		iFriendlyNum = NumTeammatesNearPos(pBot,pEdict->v.origin,300) - NumEnemiesNearPos(pBot,pBot->vecLastEnemyOrigin,500);
	if(iFriendlyNum > 0)
		fTempFear = fTempFear*0.5;

	// Increase/Decrease Fear/Agression if Bot uses a sniping weapon
	// to be more careful
	if(BotUsesSniper(pBot))
	{
		fTempFear=fTempFear*1.5;
		fTempFear=fTempAgression*0.5;
	}

	
	// Initialize & Calculate the Desire for all Actions based on
	// distances, Emotions and other Stuff

	BotGetSafeTask(pBot);

	// Bot found some Item to use ?
	if(pBot->pBotPickupItem)
	{
		pBot->iStates |= STATE_PICKUPITEM;
		edict_t *pItem=pBot->pBotPickupItem;
		Vector vecPickme;
		if (strncmp("func_", STRING(pItem->v.classname), 5) == 0)
			vecPickme = VecBModelOrigin(pItem);
		else
			vecPickme = pItem->v.origin;
		f_distance = (vecPickme - pEdict->v.origin).Length();
		f_distance = 500-f_distance;
		f_distance = (100*f_distance)/500;
		if(f_distance>50)
			f_distance=50;
		taskFilters[TASK_PICKUPITEM].fDesire=f_distance;
	}
	else
	{
		pBot->iStates &= ~STATE_PICKUPITEM;
		taskFilters[TASK_PICKUPITEM].fDesire=0.0;
	}

	float fLevel;


	float fMaxView = pBot->f_maxview_distance;

	// Calculate Desire to Attack
	if(pBot->iStates & STATE_SEEINGENEMY)
	{
		if(BotReactOnEnemy(pBot))
			taskFilters[TASK_ATTACK].fDesire = TASKPRI_ATTACK;
		else
			taskFilters[TASK_ATTACK].fDesire = 0;
	}
	else
		taskFilters[TASK_ATTACK].fDesire = 0;


	// Calculate Desires to seek Cover or Hunt

	if(pBot->pLastEnemy != NULL)
	{
		f_distance = (pBot->vecLastEnemyOrigin - pEdict->v.origin).Length();
		float fRetreatLevel=(100-pBot->bot_health)*fTempFear;
		float fTimeSeen = pBot->f_bot_see_enemy_time - gpGlobals->time;
		float fTimeHeard = pBot->f_heard_sound_time - gpGlobals->time;
		float fRatio;
		if(fTimeSeen > fTimeHeard)
		{
			fTimeSeen += 10.0;
			fRatio = fTimeSeen / 10;
		}
		else
		{
			fTimeHeard += 10.0;
			fRatio = fTimeHeard / 10;
		}

		taskFilters[TASK_SEEKCOVER].fDesire = fRetreatLevel * fRatio;
		
		// If half of the Round is over, allow hunting
		// FIXME: It probably should be also team/map dependant
		if(pBot->pBotEnemy == NULL && g_fTimeRoundMid < gpGlobals->time && !pBot->bUsingGrenade)
		{
			fLevel = 4096.0 - ((1.0-fTempAgression)*f_distance);
			fLevel = (100*fLevel)/4096.0;
			fLevel = fLevel-fRetreatLevel;
			if(fLevel > 89)
				fLevel = 89;
			taskFilters[TASK_ENEMYHUNT].fDesire = fLevel;
		}
		else
			taskFilters[TASK_ENEMYHUNT].fDesire = 0;
	}
	else
	{
		taskFilters[TASK_SEEKCOVER].fDesire=0;
		taskFilters[TASK_ENEMYHUNT].fDesire=0;
	}


	// Blinded Behaviour
	if(pBot->f_blind_time>gpGlobals->time)
		taskFilters[TASK_BLINDED].fDesire = TASKPRI_BLINDED;
	else
		taskFilters[TASK_BLINDED].fDesire = 0.0;


#ifdef DEBUG
	report_log (LOG_BOTTASK, "BotSetConditions","Pickup = %f",taskFilters[TASK_PICKUPITEM].fDesire);
	report_log (LOG_BOTTASK, "BotSetConditions","Hide = %f",taskFilters[TASK_HIDE].fDesire);
	report_log (LOG_BOTTASK, "BotSetConditions","Attack = %f",taskFilters[TASK_ATTACK].fDesire);
	report_log (LOG_BOTTASK, "BotSetConditions","EnemyHunt = %f",taskFilters[TASK_ENEMYHUNT].fDesire);
	report_log (LOG_BOTTASK, "BotSetConditions","SeekCover = %f",taskFilters[TASK_SEEKCOVER].fDesire);
	report_log (LOG_BOTTASK, "BotSetConditions","Blind = %f",taskFilters[TASK_BLINDED].fDesire);
#endif
	

	/* Now we've initialised all the desires go through the hard work
		of filtering all Actions against each other to pick the most
		rewarding one to the Bot
		Credits for the basic Idea of filtering comes out of the paper
		"Game Agent Control Using Parallel Behaviors"
		by Robert Zubek

	FIXME: Instead of going through all of the Actions it might be
	better to use some kind of decision tree to sort out impossible
	actions

	Most of the values were found out by Trial-and-Error and a Helper
	Utility I wrote so there could still be some weird behaviours, it's
	hard to check them all out */


	pBot->oldcombatdesire=hysteresisdesire(taskFilters[TASK_ATTACK].fDesire,40.0,90.0,pBot->oldcombatdesire);
	taskFilters[TASK_ATTACK].fDesire=pBot->oldcombatdesire;
	bottask_t *ptaskOffensive=&taskFilters[TASK_ATTACK];

	bottask_t * ptaskPickup=&taskFilters[TASK_PICKUPITEM];

	// Calc Survive (Cover/Hide)
	bottask_t* ptaskSurvive=thresholddesire(&taskFilters[TASK_SEEKCOVER],40.0,0.0);
	ptaskSurvive=subsumedesire(&taskFilters[TASK_HIDE],ptaskSurvive);

	// Don't allow hunting if Desire's 60<
	bottask_t* pDefault=thresholddesire(&taskFilters[TASK_ENEMYHUNT],60.0,0.0);

	// If offensive Task, don't allow picking up stuff
	ptaskOffensive=subsumedesire(ptaskOffensive,ptaskPickup);

	// Default normal & defensive Tasks against Offensive Actions
	bottask_t* pSub1=maxdesire(ptaskOffensive,pDefault);

	// Reason about fleeing instead
	bottask_t* pFinal=maxdesire(ptaskSurvive,pSub1);

	pFinal=subsumedesire(&taskFilters[TASK_BLINDED],pFinal);

	if(pBot->pTasks != NULL)
		pFinal = maxdesire(pFinal,pBot->pTasks);

#ifdef DEBUG
	report_log (LOG_BOTTASK, "BotSetConditions","Final Task = %d - Desire = %f",pFinal->iTask,pFinal->fDesire);
#endif

	// Push the final Behaviour in our Tasklist to carry out
	BotPushTask(pBot,pFinal);
}


void BotResetTasks(bot_t *pBot)
{
#ifdef DEBUG
	report_log (LOG_BOTTASK, "BotResetTasks",NULL);
#endif

	if(pBot->pTasks == NULL)
		return;
	bottask_t *pNextTask = pBot->pTasks->pNextTask;
	bottask_t* pPrevTask = pBot->pTasks;
	
	while(pPrevTask != NULL)
	{
		pPrevTask = pBot->pTasks->pPreviousTask;
		delete pBot->pTasks;
		pBot->pTasks = pPrevTask;
	}

	pBot->pTasks = pNextTask;
	while(pNextTask != NULL)
	{
		pNextTask = pBot->pTasks->pNextTask;
		delete pBot->pTasks;
		pBot->pTasks = pNextTask;
	}
	pBot->pTasks = NULL;
}


void BotPushTask(bot_t *pBot,bottask_t *pTask)
{
#ifdef DEBUG
	report_log (LOG_BOTTASK, "BotPushTask",NULL);
#endif

	if(pBot->pTasks != NULL)
	{
		if(pBot->pTasks->iTask == pTask->iTask)
		{
			if(pBot->pTasks->fDesire != pTask->fDesire)
				pBot->pTasks->fDesire = pTask->fDesire;
			return;
		}
		else
			DeleteSearchNodes(pBot);
	}

	pBot->fNoCollTime = gpGlobals->time + 0.5;

	bottask_t* pNewTask = new bottask_t;
	pNewTask->iTask = pTask->iTask;
	pNewTask->fDesire = pTask->fDesire;
	pNewTask->iData = pTask->iData;
	pNewTask->fTime = pTask->fTime;
	pNewTask->bCanContinue = pTask->bCanContinue;

	pNewTask->pPreviousTask = NULL;
	pNewTask->pNextTask = NULL;

	if(pBot->pTasks != NULL)
	{
		while(pBot->pTasks->pNextTask)
			pBot->pTasks = pBot->pTasks->pNextTask;

		pBot->pTasks->pNextTask = pNewTask;
		pNewTask->pPreviousTask = pBot->pTasks;
	}

	pBot->pTasks = pNewTask;


	// Leader Bot ?
	if(pBot->bIsLeader)
	{
		// Reorganize Team if fleeing
		if(pNewTask->iTask == TASK_SEEKCOVER)
			BotCommandTeam(pBot);
	}
}

bottask_t *BotGetSafeTask(bot_t *pBot)
{
	if(pBot->pTasks == NULL)
	{
		bottask_t TempTask = {NULL,NULL,TASK_NORMAL,TASKPRI_NORMAL,-1,0.0,TRUE};
		BotPushTask(pBot,&TempTask);
	}

	return pBot->pTasks;
}

void BotRemoveCertainTask(bot_t *pBot,int iTaskNum)
{
#ifdef DEBUG
	report_log (LOG_BOTTASK, "BotRemoveCertainTask","Task %d",iTaskNum);
#endif

	if(pBot->pTasks == NULL)
		return;
	bottask_t *pTask = pBot->pTasks;
	
	while(pTask->pPreviousTask != NULL)
		pTask = pTask->pPreviousTask;

	bottask_t *pNextTask;
	bottask_t *pPrevTask;

	while(pTask != NULL)
	{
		pNextTask = pTask->pNextTask;
		pPrevTask = pTask->pPreviousTask;
		
		if(pTask->iTask == iTaskNum)
		{
			if(pPrevTask != NULL)
				pPrevTask->pNextTask = pNextTask;
			if(pNextTask != NULL)
				pNextTask->pPreviousTask = pPrevTask;
			delete pTask;
		}
		pTask = pNextTask;
	}
	pBot->pTasks = pPrevTask;
	DeleteSearchNodes(pBot);
}


// Called whenever a Task is completed
void BotTaskComplete(bot_t *pBot)
{
	if(pBot->pTasks == NULL)
	{
		// Delete all Pathfinding Nodes
		DeleteSearchNodes(pBot);
		return;
	}
#ifdef DEBUG
	report_log (LOG_BOTTASK, "BotTaskComplete",NULL);
#endif

	bottask_t* pPrevTask;
	do
	{
		pPrevTask = pBot->pTasks->pPreviousTask;
		delete pBot->pTasks;
		pBot->pTasks = NULL;
		
		if(pPrevTask != NULL)
		{
			pPrevTask->pNextTask = NULL;
			pBot->pTasks = pPrevTask;
		}

		if(pBot->pTasks == NULL)
			break;
	} while(!pBot->pTasks->bCanContinue);

	// Delete all Pathfinding Nodes
	DeleteSearchNodes(pBot);
}



// Adjust all Bot Body and View Angles to face an absolute Vector
inline void BotFacePosition(bot_t *pBot,Vector vecPos,bool bInstantAdjust)
{
	edict_t *pEdict = pBot->pEdict;
	Vector vecDirection;
	
	vecDirection = UTIL_VecToAngles(vecPos - GetGunPosition(pEdict));

	vecDirection = vecDirection - pEdict->v.punchangle;

	vecDirection.x = -vecDirection.x;
	pEdict->v.ideal_yaw = vecDirection.y;
	pEdict->v.idealpitch = vecDirection.x;
}


bool BotEnemyIsThreat(bot_t *pBot)
{
	if(pBot->pBotEnemy == NULL || (pBot->iStates & STATE_SUSPECTENEMY)
	|| BotGetSafeTask(pBot)->iTask == TASK_SEEKCOVER)
		return FALSE;

	edict_t *pEdict = pBot->pEdict;

	float f_distance;

	if(pBot->pBotEnemy != NULL)
	{
		Vector vDest = pBot->pBotEnemy->v.origin - pBot->pEdict->v.origin;
		f_distance = vDest.Length();
	}
	else
		return FALSE;

	// If Bot is camping, he should be firing anyway and NOT
	// leaving his position
	if(BotGetSafeTask(pBot)->iTask == TASK_CAMP)
		return FALSE;

	// If Enemy is near or facing us directly
	if(f_distance < 256 || GetShootingConeDeviation(pBot->pBotEnemy,&pEdict->v.origin) >= 0.9)
		return TRUE;

	return FALSE;
}

// Check if Task has to be interrupted because an
// Enemy is near (run Attack Actions then)
bool BotReactOnEnemy(bot_t *pBot)
{
#ifdef DEBUG
	report_log (LOG_BOTCOMBAT, "BotReactOnEnemy","is true");
#endif

	if(BotEnemyIsThreat(pBot))
	{
		if(pBot->fEnemyReachableTimer < gpGlobals->time)
		{
			int iBotIndex = WaypointFindNearestToMove(pBot->pEdict->v.origin);
			int iEnemyIndex = WaypointFindNearestToMove(pBot->pBotEnemy->v.origin);
			int fLinDist = (pBot->pBotEnemy->v.origin - pBot->pEdict->v.origin).Length();
			int fPathDist = GetPathDistance(iBotIndex,iEnemyIndex);
			if(fPathDist - fLinDist > 112)
				pBot->bEnemyReachable = FALSE;
			else
				pBot->bEnemyReachable = TRUE;
			pBot->fEnemyReachableTimer = gpGlobals->time + 1.0;
		}
		
		if(pBot->bEnemyReachable)
		{
			// Override existing movement by attack movement
			pBot->f_wpt_timeset = gpGlobals->time;		
//			BotDoAttackMovement(pBot);
			return TRUE;
		}
	}
	return FALSE;
}

// Checks if Line of Sight established to last Enemy
bool BotLastEnemyVisible(bot_t *pBot)
{
	edict_t *pEdict = pBot->pEdict;

	TraceResult tr;
	
	// trace a line from bot's eyes to destination...
	UTIL_TraceLine( pEdict->v.origin + pEdict->v.view_ofs,
		pBot->vecLastEnemyOrigin, ignore_monsters,
		pEdict->v.pContainingEntity, &tr );
	
	// check if line of sight to object is not blocked (i.e. visible)
	if (tr.flFraction >= 1.0)
		return TRUE;
	return FALSE;
}


bool BotLastEnemyShootable(bot_t *pBot)
{
	if(!(pBot->iAimFlags & AIM_LASTENEMY))
		return FALSE;
	Vector2D vec2LOS;
	edict_t *pEdict=pBot->pEdict;
	float flDot = GetShootingConeDeviation(pEdict,&pBot->vecLastEnemyOrigin);
	if(flDot >= 0.90)
		return TRUE;
	return FALSE;
}


// Does the main Path Navigation...
bool BotDoWaypointNav(bot_t *pBot)
{
#ifdef DEBUG
	report_log (LOG_BOTNAV, "BotDoWaypointNav","curr_wpt_index=%d",pBot->curr_wpt_index);
#endif
	edict_t *pEdict = pBot->pEdict;
	// check if we need to find a waypoint...
	if (pBot->curr_wpt_index == -1)
	{
		GetValidWaypoint(pBot);
		int select_index = pBot->curr_wpt_index;
		pBot->wpt_origin = paths[select_index]->origin;
		if(paths[select_index]->Radius != 0)
		{
			UTIL_MakeVectors(pEdict->v.angles);
			Vector v_x = pBot->wpt_origin + gpGlobals->v_right * RANDOM_FLOAT(-paths[select_index]->Radius,paths[select_index]->Radius);
			Vector v_y = pBot->wpt_origin + gpGlobals->v_forward * RANDOM_FLOAT(0,paths[select_index]->Radius);
			pBot->wpt_origin = (v_x+v_y)/2;
		}
		pBot->f_wpt_timeset = gpGlobals->time;
	}

	pBot->dest_origin = pBot->wpt_origin;
	float wpt_distance = (pEdict->v.origin - pBot->wpt_origin).Length();
	
	bool bIsDucking = FALSE;
	if ((pEdict->v.flags & FL_DUCKING) == FL_DUCKING)
		bIsDucking = TRUE;

	// This waypoint has additional Travel Flags - care about them
	if(pBot->curr_travel_flags & C_FL_JUMP)
	{
		/*float current = pEdict->v.angles.y;
		float ideal = pEdict->v.ideal_yaw;
	    float diff = abs(current - ideal);
		if (diff <= 1)
		{
			pEdict->v.velocity=pBot->vecDesiredVelocity;
			pEdict->v.button |= IN_JUMP;
			pBot->curr_travel_flags&=~C_FL_JUMP;
			pBot->vecDesiredVelocity=Vector(0,0,0);
		}*/

		// If Bot's on ground or on ladder we're free to jump.
		// Yes, I'm cheating here by setting the correct velocity
		// for the jump. Pressing the jump button gives the illusion
		// of the Bot actual jumping
		if((pEdict->v.flags & FL_ONGROUND) || pBot->bOnLadder)
		{
			pEdict->v.velocity = pBot->vecDesiredVelocity;
			pEdict->v.button |= IN_JUMP;
			pBot->curr_travel_flags &= ~C_FL_JUMP;
			pBot->vecDesiredVelocity = Vector(0,0,0);
			pBot->bCheckTerrain = FALSE;
		}
	}
	// Special Ladder Handling
	else if(paths[pBot->curr_wpt_index]->flags & W_FL_LADDER)
	{
		if(pBot->wpt_origin.z<pEdict->v.origin.z)
		{
			if((pEdict->v.movetype != MOVETYPE_FLY) && 
				(pEdict->v.flags & FL_ONGROUND) && !bIsDucking)
			{
				// Slowly approach ladder if going down
				pBot->f_move_speed = wpt_distance;
				if(pBot->f_move_speed<150.0)
					pBot->f_move_speed = 150.0;
				else if(pBot->f_move_speed>pBot->f_max_speed)
					pBot->f_move_speed = pBot->f_max_speed;
			}
		}
/*		if(pBot->ladder_dir==LADDER_DOWN &&
			pEdict->v.movetype != MOVETYPE_FLY &&
			((pEdict->v.flags & FL_ONGROUND)==0))
		{
			// If Bot is falling down, try to catch ladder
			if(pBot->f_jumptime+0.5<gpGlobals->time)
			{
				UTIL_MakeVectors(pEdict->v.angles);
				Vector2D vecPos = (pBot->wpt_origin - pEdict->v.origin).Make2D();
				vecPos = vecPos.Normalize();
				float flDot= DotProduct (vecPos , gpGlobals->v_forward.Make2D() );
				if ( flDot < 0.50 )
					pBot->f_move_speed=-pBot->f_max_speed;
			}
		}*/
	}

	float fDesiredDistance;

	// Initialize the radius for a special waypoint type, where the wpt
	// is considered to be reached
	if (bIsDucking || (paths[pBot->curr_wpt_index]->flags & W_FL_GOAL))
	{
		fDesiredDistance = 25;
	}
	else if(paths[pBot->curr_wpt_index]->flags & W_FL_LADDER)
		fDesiredDistance = 15;
	else
		fDesiredDistance = 50;

	int index = pBot->curr_wpt_index;

	// Check if Waypoint has a special Travelflag, so they
	// need to be reached more precisely
	for(int i=0;i<MAX_PATH_INDEX;i++)
	{
		if(paths[index]->connectflag[i] != 0)
		{
			fDesiredDistance = 0;
			break;
		}
	}


	// Needs precise placement - check if we
	// get past the point
	if(fDesiredDistance<16 && wpt_distance<30)
	{
		Vector v_src = pEdict->v.origin;
		v_src = v_src + (pEdict->v.velocity * pBot->fTimeFrameInterval);
		float fDistanceNextFrame = (v_src - pBot->wpt_origin).Length();
		if(fDistanceNextFrame>wpt_distance)
			fDesiredDistance = wpt_distance+1;
	}


	if (wpt_distance < fDesiredDistance)
	{
		// Did we reach a destination Waypoint ?
		if(BotGetSafeTask(pBot)->iData == pBot->curr_wpt_index)
		{
#ifdef DEBUG
	report_log (LOG_BOTNAV, "BotDoWaypointNav","Reached Goal WPT -> %d",pBot->curr_wpt_index);
#endif
			// Add Goal Values
			if(g_bUseExperience && pBot->chosengoal_index!=-1)
			{
				int iWPTValue;
				int iStartIndex = pBot->chosengoal_index;
				int iGoalIndex = pBot->curr_wpt_index;
				if(UTIL_GetTeam(pBot->pEdict) == 0)
				{
					iWPTValue = (pBotExperienceData+(iStartIndex*g_iNumWaypoints)+iGoalIndex)->wTeam0Value;
					iWPTValue+=pBot->bot_health/20;
					iWPTValue+=pBot->f_goal_value/20;
					if(iWPTValue < -MAX_GOAL_VAL)
						iWPTValue = -MAX_GOAL_VAL;
					else if(iWPTValue > MAX_GOAL_VAL)
						iWPTValue = MAX_GOAL_VAL;
					(pBotExperienceData+(iStartIndex*g_iNumWaypoints)+iGoalIndex)->wTeam0Value=iWPTValue;
				}
				else
				{
					iWPTValue = (pBotExperienceData+(iStartIndex*g_iNumWaypoints)+iGoalIndex)->wTeam1Value;
					iWPTValue+=pBot->bot_health/20;
					iWPTValue+=pBot->f_goal_value/20;
					if(iWPTValue < -MAX_GOAL_VAL)
						iWPTValue = -MAX_GOAL_VAL;
					else if(iWPTValue > MAX_GOAL_VAL)
						iWPTValue = MAX_GOAL_VAL;
					(pBotExperienceData+(iStartIndex*g_iNumWaypoints)+iGoalIndex)->wTeam1Value=iWPTValue;
				}
			}
			return TRUE;
		}
		else if(pBot->pWaypointNodes == NULL)
			return FALSE;

		// Defusion Map ?
		if(g_iMapType == MAP_DE)
		{
			// Bomb planted and CT ?
			if(g_bBombPlanted && UTIL_GetTeam(pEdict))
			{
				int iWPTIndex = BotGetSafeTask(pBot)->iData;
				if(iWPTIndex != -1)
				{
					float fDistance = (pEdict->v.origin - paths[iWPTIndex]->origin).Length();
					// Bot within 'hearable' Bomb Tick Noises ?
					if(fDistance<BOMBMAXHEARDISTANCE)
					{
						Vector vecBombPos;
						// Does hear Bomb ?
						if(BotHearsBomb(pEdict->v.origin,&vecBombPos))
						{
							fDistance = (vecBombPos - paths[iWPTIndex]->origin).Length();
							if(fDistance > 512.0)
							{
								// Doesn't hear so not a good goal
								CTBombPointClear(iWPTIndex);
								BotTaskComplete(pBot);
							}
						}
						else
						{
							// Doesn't hear so not a good goal
							CTBombPointClear(iWPTIndex);
							BotTaskComplete(pBot);
						}
					}
				}
			}
		}

		// Do the actual movement checking
		BotHeadTowardWaypoint(pBot);
	}
	return FALSE;
}



inline bool BotGoalIsValid(bot_t* pBot)
{
	int iGoal=BotGetSafeTask(pBot)->iData;

#ifdef DEBUG
	report_log (LOG_BOTNAV, "BotGoalIsValid","curr=%d goal=%d pNodes=%x",
		pBot->curr_wpt_index,iGoal,pBot->pWaypointNodes);
#endif
	// Not decided about a goal
	if(iGoal==-1)
		return FALSE;
	// No Nodes needed
	else if(iGoal == pBot->curr_wpt_index)
		return TRUE;
	// No Path calculated
	else if(pBot->pWaypointNodes==NULL)
		return FALSE;

	// Got Path - check if still valid
	PATHNODE *Node=pBot->pWaypointNodes;
	while(Node->NextNode!=NULL)
	{
		Node=Node->NextNode;
	}
	if(Node->iIndex==iGoal)
		return TRUE;
	return FALSE;
}


// Radio Handling and Reactings to them
void BotCheckRadioCommands(bot_t *pBot)
{
	// Entity who used this Radio Command
	edict_t *pPlayer=pBot->pRadioEntity;
	// Bots Entity
	edict_t *pEdict=pBot->pEdict;

	float f_distance = (pPlayer->v.origin - pEdict->v.origin).Length();
#ifdef DEBUG
	if(pBot->iRadioOrder != 0)
		report_log (LOG_BOTBASE, "BotCheckRadioCommands","Order=%d",pBot->iRadioOrder);
#endif

	switch(pBot->iRadioOrder)
	{
	case RADIO_FOLLOWME:
		// check if line of sight to object is not blocked (i.e. visible)
		if (BotEntityIsVisible( pBot, pPlayer->v.origin))
		{
			// If Bot isn't already 'used' then follow him about
			// half of the time
			if((pBot->pBotUser == NULL) && (RANDOM_LONG(0,100)<50))
			{
				int iNumFollowers=0;
				// Check if no more followers are allowed
				for (int i=0; i <MAXBOTSNUMBER; i++)
				{
					if (bots[i].is_used && IsAlive(bots[i].pEdict))
					{
						if(bots[i].pBotUser==pPlayer)
							iNumFollowers++;
					}
				}
				
				if(iNumFollowers<g_iMaxNumFollow)
				{
					BotPlayRadioMessage(pBot,RADIO_AFFIRMATIVE);
					pBot->pBotUser = pPlayer;
					// don't pause/camp/follow anymore
					int iTask = BotGetSafeTask(pBot)->iTask;
					if(iTask == TASK_PAUSE || iTask == TASK_CAMP) 
						pBot->pTasks->fTime = gpGlobals->time;
					pBot->f_bot_use_time=gpGlobals->time;
					bottask_t TempTask = {NULL,NULL,TASK_FOLLOWUSER,TASKPRI_FOLLOWUSER,-1,0.0,TRUE};
					BotPushTask(pBot,&TempTask);
				}
				else
					BotPlayRadioMessage(pBot,RADIO_NEGATIVE);
			}
			else if(pBot->pBotUser == pPlayer)
				BotPlayRadioMessage(pBot,RADIO_IMINPOSITION);
			else
				BotPlayRadioMessage(pBot,RADIO_NEGATIVE);
		}
		break;

	case RADIO_HOLDPOSITION:
		if(pBot->pBotUser!=NULL)
		{
			if(pBot->pBotUser==pPlayer)
			{
				pBot->pBotUser=NULL;
				BotPlayRadioMessage(pBot,RADIO_AFFIRMATIVE);
				pBot->iCampButtons=0;
				bottask_t TempTask = {NULL,NULL,TASK_PAUSE,TASKPRI_PAUSE,-1,
					gpGlobals->time+RANDOM_FLOAT(30.0,60.0),FALSE};
				BotPushTask(pBot,&TempTask);
			}
		}
		break;

	// Someone called for Assistance
	case RADIO_TAKINGFIRE:
		if(pBot->pBotUser==NULL)
		{
			if(pBot->pBotEnemy==NULL)
			{
				// Decrease Fear Levels to lower probability
				// of Bot seeking Cover again
				pBot->fFearLevel-=0.2;
				if(pBot->fFearLevel<0.0)
					pBot->fFearLevel=0.0;
				BotPlayRadioMessage(pBot,RADIO_AFFIRMATIVE);
				// don't pause/camp anymore 
				int iTask = BotGetSafeTask(pBot)->iTask;
				if(iTask == TASK_PAUSE || iTask == TASK_CAMP) 
					pBot->pTasks->fTime = gpGlobals->time;
				pBot->f_bot_use_time=gpGlobals->time;
				pBot->vecPosition=pPlayer->v.origin;
				DeleteSearchNodes(pBot);
				bottask_t TempTask = {NULL,NULL,TASK_MOVETOPOSITION,
					TASKPRI_MOVETOPOSITION,-1,0.0,TRUE};
				BotPushTask(pBot,&TempTask);
			}
			else
				BotPlayRadioMessage(pBot,RADIO_NEGATIVE);
		}
		break;

	case RADIO_NEEDBACKUP:
		if((pBot->pBotEnemy==NULL && (BotEntityIsVisible( pBot, pPlayer->v.origin))) ||
			(f_distance<2048))
		{
			pBot->fFearLevel-=0.1;
			if(pBot->fFearLevel<0.0)
				pBot->fFearLevel=0.0;
			BotPlayRadioMessage(pBot,RADIO_AFFIRMATIVE);
			// don't pause/camp anymore 
			int iTask = BotGetSafeTask(pBot)->iTask;
			if(iTask == TASK_PAUSE || iTask == TASK_CAMP) 
				pBot->pTasks->fTime = gpGlobals->time;
			pBot->f_bot_use_time=gpGlobals->time;
			pBot->vecPosition=pPlayer->v.origin;
			DeleteSearchNodes(pBot);
			bottask_t TempTask = {NULL,NULL,TASK_MOVETOPOSITION,
				TASKPRI_MOVETOPOSITION,-1,0.0,TRUE};
			BotPushTask(pBot,&TempTask);
		}
		else
			BotPlayRadioMessage(pBot,RADIO_NEGATIVE);

	case RADIO_GOGOGO:
		if(pPlayer==pBot->pBotUser)
		{
			BotPlayRadioMessage(pBot,RADIO_AFFIRMATIVE);
			pBot->pBotUser=NULL;
			pBot->fFearLevel-=0.3;
			if(pBot->fFearLevel<0.0)
				pBot->fFearLevel=0.0;
		}
		else if((pBot->pBotEnemy==NULL && (BotEntityIsVisible( pBot, pPlayer->v.origin))) ||
			(f_distance<2048))
		{
			int iTask = BotGetSafeTask(pBot)->iTask;
			if(iTask == TASK_PAUSE || iTask == TASK_CAMP) 
			{
				pBot->fFearLevel-=0.3;
				if(pBot->fFearLevel<0.0)
					pBot->fFearLevel = 0.0;
				BotPlayRadioMessage(pBot,RADIO_AFFIRMATIVE);
				// don't pause/camp anymore 
				pBot->pTasks->fTime = gpGlobals->time;
				pBot->f_bot_use_time = gpGlobals->time;
				pBot->pBotUser = NULL;
				UTIL_MakeVectors( pPlayer->v.v_angle );
				pBot->vecPosition = pPlayer->v.origin+gpGlobals->v_forward * RANDOM_LONG(1024,2048);
				DeleteSearchNodes(pBot);
				bottask_t TempTask = {NULL,NULL,TASK_MOVETOPOSITION,
					TASKPRI_MOVETOPOSITION,-1,0.0,TRUE};
				BotPushTask(pBot,&TempTask);
			}
		}
		else
			BotPlayRadioMessage(pBot,RADIO_NEGATIVE);
		break;

	case RADIO_STORMTHEFRONT:
		if((pBot->pBotEnemy==NULL && (BotEntityIsVisible( pBot, pPlayer->v.origin))) ||
			(f_distance<1024))
		{
			BotPlayRadioMessage(pBot,RADIO_AFFIRMATIVE);
			// don't pause/camp anymore 
			int iTask = BotGetSafeTask(pBot)->iTask;
			if(iTask == TASK_PAUSE || iTask == TASK_CAMP) 
				pBot->pTasks->fTime = gpGlobals->time;
			pBot->f_bot_use_time=gpGlobals->time;
			pBot->pBotUser=NULL;
			UTIL_MakeVectors( pPlayer->v.v_angle );
			pBot->vecPosition=pPlayer->v.origin+gpGlobals->v_forward * RANDOM_LONG(1024,2048);
			DeleteSearchNodes(pBot);
			bottask_t TempTask = {NULL,NULL,TASK_MOVETOPOSITION,
				TASKPRI_MOVETOPOSITION,-1,0.0,TRUE};
			BotPushTask(pBot,&TempTask);
			pBot->fFearLevel-=0.3;
			if(pBot->fFearLevel<0.0)
				pBot->fFearLevel=0.0;
			pBot->fAgressionLevel+=0.3;
			if(pBot->fAgressionLevel>1.0)
				pBot->fAgressionLevel=1.0;
		}
		break;

	case RADIO_FALLBACK:
		if((pBot->pBotEnemy==NULL && (BotEntityIsVisible( pBot, pPlayer->v.origin))) ||
			(f_distance<1024))
		{
			pBot->fFearLevel+=0.5;
			if(pBot->fFearLevel>1.0)
				pBot->fFearLevel=1.0;
			pBot->fAgressionLevel-=0.5;
			if(pBot->fAgressionLevel<0.0)
				pBot->fAgressionLevel=0.0;
			BotPlayRadioMessage(pBot,RADIO_AFFIRMATIVE);
			if(BotGetSafeTask(pBot)->iTask == TASK_CAMP) 
				pBot->pTasks->fTime += RANDOM_FLOAT(10.0,15.0);
			else
			{
				// don't pause/camp anymore 
				int iTask = BotGetSafeTask(pBot)->iTask;
				if(iTask == TASK_PAUSE) 
					pBot->pTasks->fTime = gpGlobals->time;
				pBot->f_bot_use_time=gpGlobals->time;
				pBot->pBotUser=NULL;

				// FIXME : Bot doesn't see enemy yet!
				pBot->f_bot_see_enemy_time = gpGlobals->time;

				// If Bot has no enemy
				if(pBot->vecLastEnemyOrigin==Vector(0,0,0))
				{
					int iTeam=UTIL_GetTeam(pEdict);
					float distance;
					float nearestdistance=9999.0;
					// Take nearest enemy to ordering Player
					for (int ind = 0; ind < gpGlobals->maxClients; ind++)
					{
						if((ThreatTab[ind].IsUsed==FALSE) || (ThreatTab[ind].IsAlive==FALSE) ||
							(ThreatTab[ind].iTeam==iTeam))
							continue;
						edict_t *pEnemy=ThreatTab[ind].pEdict;
						distance = (pPlayer->v.origin - pEnemy->v.origin).Length();
						if (distance < nearestdistance)
						{
							nearestdistance = distance;
							pBot->pLastEnemy=pEnemy;
							pBot->vecLastEnemyOrigin=pEnemy->v.origin;
						}

					}
				}
				DeleteSearchNodes(pBot);
			}
		}
		break;

	case RADIO_REPORTTEAM:
		BotPlayRadioMessage(pBot,RADIO_REPORTINGIN);
		break;

	case RADIO_SECTORCLEAR:
		// Is Bomb planted and it's a Counter
		if(g_bBombPlanted)
		{
			// Check if it's a Counter Command
			if((UTIL_GetTeam(pPlayer)==1) && (UTIL_GetTeam(pBot->pEdict)==1))
			{
				if(g_fTimeNextBombUpdate<gpGlobals->time)
				{
					float min_distance = 9999.0;
					// Find Nearest Bomb Waypoint to Player
					for (int i=0; i <= g_iNumGoalPoints; i++)
					{
						f_distance = (paths[g_rgiGoalWaypoints[i]]->origin - pPlayer->v.origin).Length();
						
						if (f_distance < min_distance)
						{
							min_distance=f_distance;
							g_iLastBombPoint=g_rgiGoalWaypoints[i];
						}
					}
					
					// Enter this WPT Index as taboo wpt
					CTBombPointClear(g_iLastBombPoint);
					g_fTimeNextBombUpdate=gpGlobals->time+0.5;
				}
				// Does this Bot want to defuse ?
				if(BotGetSafeTask(pBot)->iTask == TASK_NORMAL)
				{
					// Is he approaching this goal ?
					if(pBot->pTasks->iData == g_iLastBombPoint)
					{
						pBot->pTasks->iData = -1;
						BotPlayRadioMessage(pBot,RADIO_AFFIRMATIVE);
					}
				}
			}
		}
		break;
		
	case RADIO_GETINPOSITION:
		if((pBot->pBotEnemy==NULL && (BotEntityIsVisible( pBot, pPlayer->v.origin))) ||
			(f_distance<1024))
		{
			BotPlayRadioMessage(pBot,RADIO_AFFIRMATIVE);
			if(BotGetSafeTask(pBot)->iTask == TASK_CAMP) 
				pBot->pTasks->fTime = gpGlobals->time + RANDOM_FLOAT(30.0,60.0);
			else
			{
				// don't pause anymore 
				int iTask = BotGetSafeTask(pBot)->iTask;
				if(iTask == TASK_PAUSE) 
					pBot->pTasks->fTime = gpGlobals->time;
				pBot->f_bot_use_time=gpGlobals->time;
				pBot->pBotUser=NULL;

				// FIXME : Bot doesn't see enemy yet!
				pBot->f_bot_see_enemy_time = gpGlobals->time;

				// If Bot has no enemy
				if(pBot->vecLastEnemyOrigin==Vector(0,0,0))
				{
					int iTeam=UTIL_GetTeam(pEdict);
					float distance;
					float nearestdistance=9999.0;
					// Take nearest enemy to ordering Player
					for (int ind = 0; ind < gpGlobals->maxClients; ind++)
					{
						if((ThreatTab[ind].IsUsed==FALSE) || (ThreatTab[ind].IsAlive==FALSE) ||
							(ThreatTab[ind].iTeam==iTeam))
							continue;
						edict_t *pEnemy=ThreatTab[ind].pEdict;
						distance = (pPlayer->v.origin - pEnemy->v.origin).Length();
						if (distance < nearestdistance)
						{
							nearestdistance = distance;
							pBot->pLastEnemy=pEnemy;
							pBot->vecLastEnemyOrigin=pEnemy->v.origin;
						}

					}
				}
				DeleteSearchNodes(pBot);
				// Push camp task on to stack
				bottask_t TempTask = {NULL,NULL,TASK_CAMP,TASKPRI_CAMP,-1,
					gpGlobals->time+RANDOM_FLOAT(30.0,60.0),TRUE};
				BotPushTask(pBot,&TempTask);
				// Push Move Command
				TempTask.iTask = TASK_MOVETOPOSITION;
				TempTask.fDesire = TASKPRI_MOVETOPOSITION;
				TempTask.iData = BotFindDefendWaypoint(pBot,pPlayer->v.origin);
				BotPushTask(pBot,&TempTask);
				pBot->iCampButtons|=IN_DUCK;
			}
		}
		break;
	}

	// Radio Command has been handled, reset
	pBot->iRadioOrder=0;
}


// Chooses a Destination (Goal) Waypoint for a Bot

int BotFindGoal(bot_t *pBot)
{
	bool bHasHostage=BotHasHostage(pBot);
	int iTeam=UTIL_GetTeam(pBot->pEdict);

#ifdef DEBUG
	report_log (LOG_BOTNAV, "BotFindGoal","HasHostage=%d Team=%d",
		bHasHostage,iTeam);
#endif

	// Pathfinding Behaviour depending on Maptype
	if(g_bUseExperience)
	{
		int iTactic;
		int iOffensive;
		int iDefensive;
		int iGoalDesire;
		int iForwardDesire;
		int iCampDesire;
		int iBackoffDesire;
		int iTacticChoice;
		int *pOffensiveWPTS=NULL;
		int iNumOffensives;
		int *pDefensiveWPTS=NULL;
		int iNumDefensives;

		if(iTeam==0)
		{
			pOffensiveWPTS=&g_rgiCTWaypoints[0];
			iNumOffensives=g_iNumCTPoints;
			pDefensiveWPTS=&g_rgiTerrorWaypoints[0];
			iNumDefensives=g_iNumTerrorPoints;
		}
		else
		{
			pOffensiveWPTS=&g_rgiTerrorWaypoints[0];
			iNumOffensives=g_iNumTerrorPoints;
			pDefensiveWPTS=&g_rgiCTWaypoints[0];
			iNumDefensives=g_iNumCTPoints;
		}
		// Terrorist carrying the C4 ?
		//	if(pBot->bot_weapons & (1<<CS_WEAPON_C4))
		if(pBot->pEdict->v.weapons & (1<<CS_WEAPON_C4)
			|| pBot->bIsVIP)
		{
			iTactic=3;
			goto tacticchosen;
		}
		else if(bHasHostage && iTeam==1)
		{
			for (int i=0; i < g_iNumWaypoints; i++)
			{
				if(paths[i]->flags & W_FL_RESCUE)
				{
					return(i);
				}
			}
		}
		iOffensive=pBot->fAgressionLevel*100;
		iDefensive=pBot->fFearLevel*100;
		switch(g_iMapType)
		{
		case MAP_AS:
		case MAP_CS:
			if(iTeam==0)
			{
				iDefensive+=25;
				iOffensive-=25;
			}
			break;

		case MAP_DE:
			if(iTeam==1)
			{
				if(g_bBombPlanted)
				{
					if(g_bBotChat)
					{
						if(g_bBombSayString)
						{
							STRINGNODE *pTempNode=NULL;
damncheck1:
							do
							{
								pTempNode=GetNodeString(pBombChat,RANDOM_LONG(0,iNumBombChats-1));
							} while(pTempNode==NULL);
							if(pTempNode->pszString==NULL)
								goto damncheck1;
							BotPrepareChatMessage(pBot,pTempNode->pszString);
							BotPushMessageQueue(pBot,MSG_CS_SAY);
							g_bBombSayString=FALSE;
						}
					}
					return(BotChooseBombWaypoint(pBot));
				}
				iDefensive+=25;
				iOffensive-=25;
			}
			break;
		}

		iGoalDesire=RANDOM_LONG(0,100)+iOffensive;
		iForwardDesire=RANDOM_LONG(0,100)+iOffensive;
		iCampDesire=RANDOM_LONG(0,100)+iDefensive;
		iBackoffDesire=RANDOM_LONG(0,100)+iDefensive;

		iTacticChoice=iBackoffDesire;
		iTactic=0;

		if(iCampDesire>iTacticChoice)
		{
			iTacticChoice=iCampDesire;
			iTactic=1;
		}
		if(iForwardDesire>iTacticChoice)
		{
			iTacticChoice=iForwardDesire;
			iTactic=2;
		}
		if(iGoalDesire>iTacticChoice)
		{
			iTacticChoice=iGoalDesire;
			iTactic=3;
		}

		
tacticchosen:
		int iGoalChoices[4];
		float iGoalDistances[4];

		int i;

		for (i=0; i < 4; i++)
		{
			iGoalChoices[i] = -1;
			iGoalDistances[i] = 9999.0;
		}

		// Defensive Goal
		if(iTactic==0)
		{
			for(i=0;i<4;i++)
			{
				iGoalChoices[i]=pDefensiveWPTS[RANDOM_LONG(0,iNumDefensives)];
				assert(iGoalChoices[i]>=0 && iGoalChoices[i]<g_iNumWaypoints);
			}
		}
		// Camp Waypoint Goal
		else if(iTactic==1)
		{
			for(i=0;i<4;i++)
			{
				iGoalChoices[i]=g_rgiCampWaypoints[RANDOM_LONG(0,g_iNumCampPoints)];
				assert(iGoalChoices[i]>=0 && iGoalChoices[i]<g_iNumWaypoints);
			}
		}
		// Offensive Goal
		else if(iTactic==2)
		{
			for(i=0;i<4;i++)
			{
				iGoalChoices[i]=pOffensiveWPTS[RANDOM_LONG(0,iNumOffensives)];
				assert(iGoalChoices[i]>=0 && iGoalChoices[i]<g_iNumWaypoints);
			}
		}
		// Map Goal Waypoint
		else if(iTactic==3)
		{
			for(i=0;i<4;i++)
			{
				iGoalChoices[i]=g_rgiGoalWaypoints[RANDOM_LONG(0,g_iNumGoalPoints)];
				assert(iGoalChoices[i]>=0 && iGoalChoices[i]<g_iNumWaypoints);
			}
		}

		if(pBot->curr_wpt_index == -1 ||
			pBot->curr_wpt_index >= g_iNumWaypoints)
		{
			pBot->curr_wpt_index=WaypointFindNearestToMove(pBot->pEdict->v.origin);
		}

		int iBotIndex=pBot->curr_wpt_index;
		int iTestIndex;
		bool bSorting;

		do
		{
			bSorting=FALSE;
			for(i=0;i<3;i++)
			{
				iTestIndex=iGoalChoices[i+1];
				if(iTestIndex<0)
					break;

				if(iTeam==0)
				{
					if((pBotExperienceData+(iBotIndex*g_iNumWaypoints)+iGoalChoices[i])->wTeam0Value<
						(pBotExperienceData+(iBotIndex*g_iNumWaypoints)+iTestIndex)->wTeam0Value)
					{
						iGoalChoices[i+1]=iGoalChoices[i];
						iGoalChoices[i]=iTestIndex;
						bSorting=TRUE;
					}
				}
				else
				{
					if((pBotExperienceData+(iBotIndex*g_iNumWaypoints)+iGoalChoices[i])->wTeam1Value<
						(pBotExperienceData+(iBotIndex*g_iNumWaypoints)+iTestIndex)->wTeam1Value)
					{
						iGoalChoices[i+1]=iGoalChoices[i];
						iGoalChoices[i]=iTestIndex;
						bSorting=TRUE;
					}
				}
				
			}
		} while(bSorting);

		return iGoalChoices[0];
	}
	else
	{
		int iRandomPick=RANDOM_LONG(1,100);
		
		switch(g_iMapType)
		{
		case MAP_AS:
			if(pBot->bIsVIP)
			{
				// 90% choose a Goal Waypoint
				if(iRandomPick<70)
					return(g_rgiGoalWaypoints[RANDOM_LONG(0,g_iNumGoalPoints)]);
				else if(iRandomPick<90)
					return(g_rgiCTWaypoints[RANDOM_LONG(0,g_iNumCTPoints)]);
				else
					return(g_rgiTerrorWaypoints[RANDOM_LONG(0,g_iNumTerrorPoints)]);
			}
			else
			{
				if(iTeam==0)
				{
					if(iRandomPick<30)
						return(g_rgiGoalWaypoints[RANDOM_LONG(0,g_iNumGoalPoints)]);
					else if(iRandomPick<60)
						return(g_rgiTerrorWaypoints[RANDOM_LONG(0,g_iNumTerrorPoints)]);
					else if(iRandomPick<90)
						return(g_rgiCampWaypoints[RANDOM_LONG(0,g_iNumCampPoints)]);
					else
						return(g_rgiCTWaypoints[RANDOM_LONG(0,g_iNumCTPoints)]);
				}
				else
				{
					if(iRandomPick<50)
						return(g_rgiGoalWaypoints[RANDOM_LONG(0,g_iNumGoalPoints)]);
					else if(iRandomPick<70)
						return(g_rgiTerrorWaypoints[RANDOM_LONG(0,g_iNumTerrorPoints)]);
					else if(iRandomPick<90)
						return(g_rgiCampWaypoints[RANDOM_LONG(0,g_iNumCampPoints)]);
					else
						return(g_rgiCTWaypoints[RANDOM_LONG(0,g_iNumCTPoints)]);
				}
			}
			break;
			
			
		case MAP_CS:
			bHasHostage=BotHasHostage(pBot);
			if(iTeam==0)
			{
				if(iRandomPick<30)
					return(g_rgiGoalWaypoints[RANDOM_LONG(0,g_iNumGoalPoints)]);
				else if(iRandomPick<60)
					return(g_rgiTerrorWaypoints[RANDOM_LONG(0,g_iNumTerrorPoints)]);
				else if(iRandomPick<90)
					return(g_rgiCampWaypoints[RANDOM_LONG(0,g_iNumCampPoints)]);
				else
					return(g_rgiCTWaypoints[RANDOM_LONG(0,g_iNumCTPoints)]);
			}
			else
			{
				if(bHasHostage)
				{
					for (int i=0; i < g_iNumWaypoints; i++)
					{
						if(paths[i]->flags & W_FL_RESCUE)
						{
							return(i);
						}
					}
				}
				else if(iRandomPick<50)
					return(g_rgiGoalWaypoints[RANDOM_LONG(0,g_iNumGoalPoints)]);
				else if(iRandomPick<70)
					return(g_rgiTerrorWaypoints[RANDOM_LONG(0,g_iNumTerrorPoints)]);
				else if(iRandomPick<90)
					return(g_rgiCampWaypoints[RANDOM_LONG(0,g_iNumCampPoints)]);
				else
					return(g_rgiCTWaypoints[RANDOM_LONG(0,g_iNumCTPoints)]);
			}
			break;
			
		case MAP_DE:
			if(iTeam==0)
			{
				// Terrorist carrying the C4 ?
				if(pBot->pEdict->v.weapons & (1<<CS_WEAPON_C4))
					//						if(pBot->bot_weapons & (1<<CS_WEAPON_C4))
					return(g_rgiGoalWaypoints[RANDOM_LONG(0,g_iNumGoalPoints)]);
				else if(iRandomPick<30)
					return(g_rgiGoalWaypoints[RANDOM_LONG(0,g_iNumGoalPoints)]);
				else if(iRandomPick<60)
					return(g_rgiCTWaypoints[RANDOM_LONG(0,g_iNumCTPoints)]);
				else if(iRandomPick<90)
					return(g_rgiCampWaypoints[RANDOM_LONG(0,g_iNumCampPoints)]);
				else
					return(g_rgiTerrorWaypoints[RANDOM_LONG(0,g_iNumTerrorPoints)]);
			}
			else
			{
				if(g_bBombPlanted)
				{
					if(g_bBotChat)
					{
						if(g_bBombSayString)
						{
							STRINGNODE *pTempNode=NULL;
damncheck:
							do
							{
								pTempNode=GetNodeString(pBombChat,RANDOM_LONG(0,iNumBombChats-1));
							} while(pTempNode==NULL);
							if(pTempNode->pszString==NULL)
								goto damncheck;
							BotPrepareChatMessage(pBot,pTempNode->pszString);
							BotPushMessageQueue(pBot,MSG_CS_SAY);
							g_bBombSayString=FALSE;
						}
					}
					return(BotChooseBombWaypoint(pBot));
				}
				else if(iRandomPick<50)
					return(g_rgiGoalWaypoints[RANDOM_LONG(0,g_iNumGoalPoints)]);
				else if(iRandomPick<70)
					return(g_rgiCTWaypoints[RANDOM_LONG(0,g_iNumCTPoints)]);
				else if(iRandomPick<90)
					return(g_rgiCampWaypoints[RANDOM_LONG(0,g_iNumCampPoints)]);
				else
					return(g_rgiTerrorWaypoints[RANDOM_LONG(0,g_iNumTerrorPoints)]);
			}
			break;
	}
	}
	return (0);
}

void TurnOffFunStuff(edict_t *pPlayer)
{
	for(int i=0;i<MAXBOTSNUMBER;i++)
	{
		bot_t *pBot = &bots[i];
		if((pBot->is_used))
		{
			edict_t *pEdict=pBot->pEdict;
			pEdict->v.rendermode = kRenderNormal;
			pEdict->v.renderfx = kRenderFxNone;
			pEdict->v.rendercolor.x = pPlayer->v.rendercolor.x;
			pEdict->v.rendercolor.y = pPlayer->v.rendercolor.y;
			pEdict->v.rendercolor.z = pPlayer->v.rendercolor.z;
			pEdict->v.renderamt = pPlayer->v.renderamt;
			pEdict->v.effects = pPlayer->v.effects & EF_NODRAW;
		}
	}
	g_fTimeFunUpdate=0.0;
	if(CVAR_GET_FLOAT("sv_gravity")!=800.0)
		 CVAR_SET_FLOAT("sv_gravity",800.0);
}


void DoFunStuff(bot_t *pBot)
{
	edict_t *pEdict = pBot->pEdict;
	Vector vecMisc;
	int i;

	switch(g_cFunMode)
	{
	// Cyberspace
	case 1:
		if(g_fTimeFunUpdate <= gpGlobals->time)
		{
			for (i = 0; i <= gpGlobals->maxClients+1; i++ )
			{
				if((ThreatTab[i].IsUsed == FALSE))
					continue;
				pEdict = ThreatTab[i].pEdict;
				if (pEdict->v.effects & EF_NODRAW)
					continue;
				if(ThreatTab[i].iTeam == 0)
				{
					pEdict->v.renderfx = kRenderFxGlowShell;
					pEdict->v.rendercolor.x = 255;  // red
					pEdict->v.rendercolor.y = 0;  // green
					pEdict->v.rendercolor.z = 0; // blue
					pEdict->v.renderamt = 10;  // glow shell distance from entity
				}
				else
				{
					pEdict->v.renderfx = kRenderFxGlowShell;
					pEdict->v.rendercolor.x = 0;  // red
					pEdict->v.rendercolor.y = 0;  // green
					pEdict->v.rendercolor.z = 255; // blue
					pEdict->v.renderamt = 10;  // glow shell distance from entity
				}
			}
			g_fTimeFunUpdate = gpGlobals->time+9999.0;
		}
		break;
	// New Year
	case 2:
		if (pEdict->v.effects & EF_NODRAW)
			break;
		vecMisc=pEdict->v.origin+pEdict->v.view_ofs;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecMisc );
		WRITE_BYTE( TE_SPARKS );
		WRITE_COORD( vecMisc.x );
		WRITE_COORD( vecMisc.y );
		WRITE_COORD( vecMisc.z );
		MESSAGE_END();
		break;

	// Haunted
	case 3:
		pEdict->v.rendermode = kRenderTransTexture;
		pEdict->v.renderamt = 100;
		break;

	// Light
	case 4:
		pEdict->v.effects|=EF_BRIGHTLIGHT;
		break;

	// Stoned
	case 5:
		for (i = 0; i <= gpGlobals->maxClients+1; i++ )
		{
			if((ThreatTab[i].IsUsed==FALSE))
				continue;
			pEdict = ThreatTab[i].pEdict;
			if (pEdict->v.effects & EF_NODRAW)
				continue;
/*			int iRand=RANDOM_LONG(1,4);
			if(iRand==1)
			{
				pEdict->v.rendermode = kRenderTransTexture;
				pEdict->v.renderamt = 100;
			}
			else if(iRand==2)
			{
				pEdict->v.renderfx = kRenderFxGlowShell;
				pEdict->v.rendercolor.x = RANDOM_LONG(0,255);  // red
				pEdict->v.rendercolor.y = 0;  // green
				pEdict->v.rendercolor.z = RANDOM_LONG(0,255); // blue
				pEdict->v.renderamt = 10;  // glow shell distance from entity
			}*/
		}
		if(g_fTimeFunUpdate<=gpGlobals->time)
		{
//			pEdict->v.renderfx = kRenderFxExplode;
//			pEdict->v.renderamt = 128;
			UTIL_ScreenShake( pEdict->v.origin, 2048.0, 1.0, 10.0, 0 );
			g_fTimeFunUpdate = gpGlobals->time+RANDOM_FLOAT(10.0,20.0);
		}
		break;

	// Mars Mode - Gravity is set in clientcommand
	case 6:
		break;

	// Chicken again
	case 100:
		if(g_fTimeFunUpdate<=gpGlobals->time)
		{
			if (pEdict->v.effects & EF_NODRAW)
				break;
			(*g_engfuncs.pfnSetClientKeyValue)(ENTINDEX(pEdict), (*g_engfuncs.pfnGetInfoKeyBuffer)(pEdict),
				"model", "../chick" );
			g_fTimeFunUpdate = gpGlobals->time+0.5;
		}
		break;

	}
}

int GetHighestFragsBot(int iTeam)
{
	bot_t *pFragBot;
	int iBestIndex = 0;
	float fBestFrags = -1;
	// Search Bots in this team
	for (int bot_index = 0; bot_index < MAXBOTSNUMBER; bot_index++)
	{
		pFragBot = &bots[bot_index];
		if (pFragBot->is_used && pFragBot->respawn_state == RESPAWN_IDLE)  
		{
			if(IsAlive(pFragBot->pEdict) && UTIL_GetTeam(pFragBot->pEdict) == iTeam)
			{
				if(pFragBot->pEdict->v.frags > fBestFrags)
				{
					iBestIndex = bot_index;
					fBestFrags = pFragBot->pEdict->v.frags;
				}
			}
		}
	}
	return(iBestIndex);
}

void SelectLeaderEachTeam(bot_t *pBot, int iTeam)
{
	bot_t *pBotLeader;
	edict_t *pEdict = pBot->pEdict;

	switch(g_iMapType)
	{
	case MAP_AS:
		if(pBot->bIsVIP && !g_bLeaderChosenCT)
		{
			// VIP Bot is the leader 
			pBot->bIsLeader = TRUE;
			if(RANDOM_LONG(1,100)<50)
			{
				BotPlayRadioMessage(pBot,RADIO_FOLLOWME);
				pBot->iCampButtons=0;
				bottask_t TempTask = {NULL,NULL,TASK_PAUSE,TASKPRI_PAUSE,-1,
					gpGlobals->time+3.0,FALSE};
				BotPushTask(pBot,&TempTask);
			}
			g_bLeaderChosenCT = TRUE;
		}
		else if(iTeam == 0 && !g_bLeaderChosenT)
		{
			pBotLeader = &bots[GetHighestFragsBot(iTeam)];
			pBotLeader->bIsLeader = TRUE;
			if(RANDOM_LONG(1,100)<50)
			{
				BotPlayRadioMessage(pBotLeader,RADIO_FOLLOWME);
				bottask_t TempTask = {NULL,NULL,TASK_PAUSE,TASKPRI_PAUSE,-1,
					gpGlobals->time+3.0,FALSE};
				BotPushTask(pBotLeader,&TempTask);
			}
			g_bLeaderChosenT = TRUE;
		}
		break;
		
	case MAP_CS:
		if(iTeam == 0)
		{
			pBotLeader = &bots[GetHighestFragsBot(iTeam)];
			pBotLeader->bIsLeader = TRUE;
			if(RANDOM_LONG(1,100)<50)
			{
				BotPlayRadioMessage(pBotLeader,RADIO_FOLLOWME);
				bottask_t TempTask = {NULL,NULL,TASK_PAUSE,TASKPRI_PAUSE,-1,
					gpGlobals->time+3.0,FALSE};
				BotPushTask(pBotLeader,&TempTask);
			}
		}
		else
		{
			pBotLeader = &bots[GetHighestFragsBot(iTeam)];
			pBotLeader->bIsLeader = TRUE;
			if(RANDOM_LONG(1,100)<50)
			{
				BotPlayRadioMessage(pBotLeader,RADIO_FOLLOWME);
				bottask_t TempTask = {NULL,NULL,TASK_PAUSE,TASKPRI_PAUSE,-1,
					gpGlobals->time+3.0,FALSE};
				BotPushTask(pBotLeader,&TempTask);
			}
		}
		break;
		
	case MAP_DE:
		if(iTeam == 0 && !g_bLeaderChosenT)
		{
			if(pEdict->v.weapons & (1<<CS_WEAPON_C4))
			{
				// Bot carrying the Bomb is the leader 
				pBot->bIsLeader = TRUE;
				// Terrorist carrying a Bomb needs to have some
				// company so order some Bots sometimes
				if(RANDOM_LONG(1,100)<50)
				{
					BotPlayRadioMessage(pBot,RADIO_FOLLOWME);
					pBot->iCampButtons=0;
					bottask_t TempTask = {NULL,NULL,TASK_PAUSE,TASKPRI_PAUSE,-1,
						gpGlobals->time+3.0,FALSE};
					BotPushTask(pBot,&TempTask);
				}
				g_bLeaderChosenT = TRUE;
			}
		}
		else if(!g_bLeaderChosenCT)
		{
			pBotLeader = &bots[GetHighestFragsBot(iTeam)];
			pBotLeader->bIsLeader = TRUE;
			if(RANDOM_LONG(1,100)<50)
			{
				BotPlayRadioMessage(pBotLeader,RADIO_FOLLOWME);
				bottask_t TempTask = {NULL,NULL,TASK_PAUSE,TASKPRI_PAUSE,-1,
					gpGlobals->time+3.0,FALSE};
				BotPushTask(pBotLeader,&TempTask);
			}
			g_bLeaderChosenCT = TRUE;
		}
		break;
	}
}

void BotChooseAimDirection(bot_t *pBot)
{
	bool bCanChoose = TRUE;
	unsigned int iFlags = pBot->iAimFlags;

	// Don't allow Bot to look at danger positions under certain
	// circumstances
	if(!(iFlags & (AIM_GRENADE|AIM_ENEMY|AIM_ENTITY)))
	{
		if(pBot->bOnLadder || pBot->bInWater || pBot->bNearDoor
			|| (pBot->curr_move_flags & C_FL_JUMP)
			|| (pBot->iWPTFlags & W_FL_LADDER))
		{
			iFlags &= ~(AIM_LASTENEMY|AIM_PREDICTPATH);
			bCanChoose = FALSE;
		}
	}

	if(iFlags & AIM_OVERRIDE)
	{
		pBot->vecLookAt = pBot->vecCamp;
		report_log (LOG_BOTAIM, "Aim Override","Vector(%f,%f,%f)",
			pBot->vecLookAt.x,pBot->vecLookAt.y,pBot->vecLookAt.z);
	}
	else if(iFlags & AIM_GRENADE)
	{
		pBot->vecLookAt = GetGunPosition(pBot->pEdict) + (pBot->vecGrenade.Normalize() * 1);
		report_log (LOG_BOTAIM, "Aim Grenade","Vector(%f,%f,%f)",
			pBot->vecLookAt.x,pBot->vecLookAt.y,pBot->vecLookAt.z);
	}
	else if(iFlags & AIM_ENEMY)
	{
		pBot->vecLookAt = pBot->vecEnemy;
		BotFocusEnemy(pBot);
		report_log (LOG_BOTAIM, "Aim Enemy","Vector(%f,%f,%f)",
			pBot->vecLookAt.x,pBot->vecLookAt.y,pBot->vecLookAt.z);
	}
	else if(iFlags & AIM_ENTITY)
	{
		pBot->vecLookAt = pBot->vecEntity;
		report_log (LOG_BOTAIM, "Aim Entity","Vector(%f,%f,%f)",
			pBot->vecLookAt.x,pBot->vecLookAt.y,pBot->vecLookAt.z);
	}
	else if(iFlags & AIM_LASTENEMY)
	{
		pBot->vecLookAt = pBot->vecLastEnemyOrigin;
		// Did Bot just see Enemy and is quite agressive ?
		if(pBot->f_bot_see_enemy_time - pBot->f_actual_reaction_time +
			pBot->fBaseAgressionLevel > gpGlobals->time)
		{
			// Not using a Sniper Weapon ?
			if(!BotUsesSniper(pBot))
			{
				// Feel free to fire if shootable
				if (BotLastEnemyShootable(pBot))
					pBot->bWantsToFire = TRUE;
			}
		}
		report_log (LOG_BOTAIM, "Aim LastEnemy","Vector(%f,%f,%f)",
			pBot->vecLookAt.x,pBot->vecLookAt.y,pBot->vecLookAt.z);
	}
	else if(iFlags & AIM_PREDICTPATH)
	{
		bool bRecalcPath = TRUE;
		if(pBot->pTrackingEdict == pBot->pLastEnemy)
		{
			if(pBot->fTimeNextTracking < gpGlobals->time)
				bRecalcPath = FALSE;
		}
		
		if(bRecalcPath)
		{
			pBot->vecLookAt = paths[GetAimingWaypoint(pBot,pBot->vecLastEnemyOrigin,8)]->origin;
			pBot->vecCamp = pBot->vecLookAt;
			pBot->fTimeNextTracking = gpGlobals->time + 0.5;
			pBot->pTrackingEdict = pBot->pLastEnemy;
		}
		else
			pBot->vecLookAt = pBot->vecCamp;
		report_log (LOG_BOTAIM, "Aim Predict","Vector(%f,%f,%f)",
			pBot->vecLookAt.x,pBot->vecLookAt.y,pBot->vecLookAt.z);
	}
	else if(iFlags & AIM_CAMP)
	{
		pBot->vecLookAt = pBot->vecCamp;
		report_log (LOG_BOTAIM, "Aim Camp","Vector(%f,%f,%f)",
			pBot->vecLookAt.x,pBot->vecLookAt.y,pBot->vecLookAt.z);
	}
	else if(iFlags & AIM_DEST)
	{
		pBot->vecLookAt = pBot->dest_origin;
		report_log (LOG_BOTAIM, "Aim Dest","Vector(%f,%f,%f)",
			pBot->vecLookAt.x,pBot->vecLookAt.y,pBot->vecLookAt.z);

		if(bCanChoose && g_bUseExperience
		&& pBot->curr_wpt_index != -1)
		{
			if((paths[pBot->curr_wpt_index]->flags & W_FL_LADDER) == 0)
			{
				int iIndex = pBot->curr_wpt_index;
				if(pBot->iTeam == 0)
				{
					if((pBotExperienceData+(iIndex*g_iNumWaypoints)+iIndex)->iTeam0_danger_index!=-1)
					{
						pBot->vecLookAt=paths[(pBotExperienceData+(iIndex*g_iNumWaypoints)+iIndex)->iTeam0_danger_index]->origin; 
						report_log (LOG_BOTAIM, "Aim Dest","Exp - Vector(%f,%f,%f)",
							pBot->vecLookAt.x,pBot->vecLookAt.y,pBot->vecLookAt.z);
					}
				}
				else
				{
					if((pBotExperienceData+(iIndex*g_iNumWaypoints)+iIndex)->iTeam1_danger_index!=-1)
					{
						pBot->vecLookAt=paths[(pBotExperienceData+(iIndex*g_iNumWaypoints)+iIndex)->iTeam1_danger_index]->origin; 
						report_log (LOG_BOTAIM, "Aim Dest","Exp - Vector(%f,%f,%f)",
							pBot->vecLookAt.x,pBot->vecLookAt.y,pBot->vecLookAt.z);
					}
				}
			}
		}
	}

	if(pBot->vecLookAt == Vector(0,0,0))
		pBot->vecLookAt = pBot->dest_origin;

	/*	else
	{
		UTIL_DebugMSG("TASK: %d NO AIMING SET!\n",pBot->iTask);
	}*/
}

void BotSetStrafeSpeed(bot_t *pBot,Vector vecMoveDir,float fStrafeSpeed)
{
	edict_t *pEdict = pBot->pEdict;
	Vector2D vec2LOS;
	Vector vecDestination = pEdict->v.origin + vecMoveDir * 2; 
	float    flDot;
	
	UTIL_MakeVectors ( pEdict->v.angles);
	
	vec2LOS = ( vecMoveDir - pEdict->v.origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();
	
	flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );
		
	if ( flDot > 0)
		pBot->f_sidemove_speed = fStrafeSpeed;
	else
		pBot->f_sidemove_speed = -fStrafeSpeed;
}


// This function gets called each Frame and is the core of
// all Bot AI. From here all other Subroutines are called
void BotThink(bot_t *pBot)
{
	int index = 0;
	Vector v_diff;             // vector from previous to current location
	float moved_distance;      // length of v_diff vector (distance bot moved)
	TraceResult tr;
	bool bBotMovement = FALSE;
	
	
#ifdef DEBUG
	set_log_botindex(UTIL_GetBotIndex(pBot->pEdict));
	report_log (LOG_BOTBASE, "---> BotThink","\n%s Health=%d Armor=%d",
		STRING(pBot->pEdict->v.netname),pBot->bot_health,pBot->bot_armor);
	assert(pBot->bDebug == FALSE);
#endif

	edict_t *pEdict = pBot->pEdict;
	pEdict->v.flags |= FL_FAKECLIENT;

	// Needs to be set for Borg Mode...
	if(g_bIsBorg && pEdict == pHostEdict)
	{
		pEdict->v.flags &= ~FL_FROZEN;
		pEdict->v.flags &= ~FL_CLIENT;
		pEdict->v.fixangle = 1;
	}

	pBot->fTimeFrameInterval = gpGlobals->time - pBot->fTimePrevThink;
	pBot->fTimePrevThink = gpGlobals->time;

	
	if (pBot->name[0] == 0)  // name filled in yet?
		strcpy(pBot->name, STRING(pBot->pEdict->v.netname));
	
	
	// TheFatal - START from Advanced Bot Framework (Thanks Rich!)
	
	// adjust the millisecond delay based on the frame rate interval...
	if (pBot->msecdel <= gpGlobals->time)
	{
		pBot->msecdel = gpGlobals->time + 0.5;
		if (pBot->msecnum > 0)
			pBot->msecval = 450.0/pBot->msecnum;
		pBot->msecnum = 0;
	}
	else
		pBot->msecnum++;

	if (pBot->msecval < 1)    // don't allow msec to be less than 1...
		pBot->msecval = 1;
	
	if (pBot->msecval > 100)  // ...or greater than 100
		pBot->msecval = 100;
	
	// TheFatal - END
	
	pEdict->v.button = 0;
	pBot->f_move_speed = 0.0;
	pBot->f_sidemove_speed=0.0;
	pBot->bDead = IsAlive(pEdict);
	pBot->bDead ^= 1;
	// if the bot hasn't selected stuff to start the game yet, go do that...
	if (pBot->not_started)
	{
		// Select Team & Class
		BotStartGame( pBot );
	}
	// Bot needs to initialize itself ?
	else if (pBot->need_to_initialize)
	{
		BotSpawnInit(pBot);
	}
	// In Cs Health seems to be refilled in Spectator Mode
	// that's why I'm doing this extra
	else if(pBot->bDead)
	{
		char szVote[4];

		// We got a Teamkiller ? Vote him away...
		if((pBot->iVoteKickIndex!=pBot->iLastVoteKick) && g_bAllowVotes)
		{
			sprintf(szVote,"%d",pBot->iVoteKickIndex);
			FakeClientCommand(pBot->pEdict,"vote",(char*)&szVote[0],NULL);
			pBot->iLastVoteKick = pBot->iVoteKickIndex;
		}
		// Host wants the Bots to vote for a map ?
		else if(pBot->iVoteMap!=0)
		{
			sprintf(szVote,"%d",pBot->iVoteMap);
			FakeClientCommand(pBot->pEdict,"votemap",(char*)&szVote[0],NULL);
			pBot->iVoteMap = 0;
		}
		// Bot chatting turned on ?
		if(g_bBotChat)
		{
			if((!BotRepliesToPlayer(pBot)) && (pBot->f_lastchattime+10.0<gpGlobals->time) &&
				(g_fLastChatTime+5.0<gpGlobals->time))
			{
				// Say a Text every now and then
				if (RANDOM_LONG(1,1500) < 2)
				{
					pBot->f_lastchattime = gpGlobals->time;
					g_fLastChatTime =gpGlobals->time;
					// Rotate used Strings Array up
					pUsedDeadStrings[7] = pUsedDeadStrings[6];
					pUsedDeadStrings[6] = pUsedDeadStrings[5];
					pUsedDeadStrings[5] = pUsedDeadStrings[4];
					pUsedDeadStrings[4] = pUsedDeadStrings[3];
					pUsedDeadStrings[3] = pUsedDeadStrings[2];
					pUsedDeadStrings[2] = pUsedDeadStrings[1];
					pUsedDeadStrings[1] = pUsedDeadStrings[0];
					
					bool bStringUsed = TRUE;
					STRINGNODE* pTempNode = NULL;
					int iCount=0;
					while(bStringUsed)
					{
						do
						{
							pTempNode=GetNodeString(pDeadChat,RANDOM_LONG(0,iNumDeadChats-1));
						} while(pTempNode==NULL);
						if(pTempNode->pszString == NULL)
							continue;
						bStringUsed = FALSE;
						for(int i=0;i<8;i++)
						{
							if(pUsedDeadStrings[i]==pTempNode)
							{
								iCount++;
								if(iCount<9)
									bStringUsed = TRUE;
							}
						}
						
					}
					// Save new String
					pUsedDeadStrings[0] = pTempNode;
					BotPrepareChatMessage(pBot,pTempNode->pszString);
					BotPushMessageQueue(pBot,MSG_CS_SAY);
				}
			}
		}
	}
	// Bot is still buying - don't move
	else if(!pBot->bDead && !pBot->bBuyingFinished)
	{
		BotBuyStuff(pBot);

		// Bot has spawned 10 secs ago ? Something went wrong
		// in buying so cancel it
		if(pBot->f_spawn_time+10.0<gpGlobals->time)
			pBot->bBuyingFinished = TRUE;
	}
	else
		bBotMovement = TRUE;

	// Check for pending Messages
	BotCheckMessageQueue(pBot);

	if(!bBotMovement || pBot->f_max_speed < 10)
	{
		if(pEdict == pHostEdict)
		{
			if(pBot->bDead)
			{
				static Vector vecOld1 = Vector(0,0,0);
				static Vector vecOld2 = Vector(0,0,0);
				static Vector vecOld3 = Vector(0,0,0);
				static Vector vecNew = Vector(0,0,0);
				static int iOldSpecEnt = 1;

				edict_t *pPlayer;
				for (int player_index = 2; player_index <= gpGlobals->maxClients;player_index++)
				{
					pPlayer = INDEXENT(player_index);
					if(!FNullEnt(pPlayer) && IsAlive(pPlayer))
					{
						vecOld3 = vecOld2;
						vecOld2 = vecOld1;
						vecOld1 = vecNew;
						vecNew = pPlayer->v.origin;
						BotFacePosition(pBot,vecNew,TRUE);
						vecNew = vecNew + pPlayer->v.view_ofs;
						Vector vecVel = pPlayer->v.velocity.Normalize();
						if(pPlayer->v.velocity.Length2D() < 50.0)
						{
							vecVel = pPlayer->v.v_angle;
							vecVel.x = 0.0;
							UTIL_MakeVectors(vecVel);
							vecVel = gpGlobals->v_forward * 0.5;
							vecVel.z = -0.4;
						}
						vecVel.z *= 0.5;
						vecNew = vecNew + -vecVel * 96;
						pEdict->v.origin = (vecOld3 + vecOld2 + vecOld3  + vecNew) * 0.25;
						if(player_index != iOldSpecEnt)
						{
							iOldSpecEnt = player_index;
							char szMsg[128];
							sprintf(szMsg, "Borg Cam now featuring %s\n",STRING(pPlayer->v.netname));
							UTIL_HostPrint(HUD_PRINTNOTIFY, szMsg);
						}
						break;
					}
				}

			}
		}
		pEdict->v.flags |= FL_FAKECLIENT;
		g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, 0.0,
			0, 0, pEdict->v.button, 0, pBot->msecval);
		return;
	}

	if(pBot->bCheckMyTeam)
	{
		pBot->iTeam = UTIL_GetTeam(pEdict);
		pBot->bCheckMyTeam = FALSE;
	}

	pBot->bOnLadder = IsOnLadder(pEdict);
	pBot->bInWater = IsInWater(pEdict);
	pBot->bNearDoor = IsNearDoor(pBot);

	// Check if we already switched weapon mode
	if((pBot->bCheckWeaponSwitch) && (pBot->f_spawn_time + 4.0 < gpGlobals->time))
	{
		// Bot owns a Sniper Weapon ? Try to switch back to no
		// zoom so Bot isn't moving slow
		if(BotUsesSniper(pBot))
		{
			if(pEdict->v.fov < 90)
				pEdict->v.button |= IN_ATTACK2;
		}
		// If no sniper weapon use a secondary mode at random
		// times
		else
		{
			if((RANDOM_LONG(1,100)<30) && 
				(pBot->current_weapon.iId!=CS_WEAPON_KNIFE))
			{
				pEdict->v.button |= IN_ATTACK2;
			}
		}

		// If Psycho Bot switch to Knife
		if(pBot->bot_personality==1)
			SelectWeaponByName(pBot, "weapon_knife");
		if(RANDOM_LONG(1,100)<20)
		{
			bottask_t TempTask = {NULL,NULL,TASK_SPRAYLOGO,TASKPRI_SPRAYLOGO,-1,
				gpGlobals->time + 1.0,FALSE};
			BotPushTask(pBot,&TempTask);
		}

		// Select a Leader Bot for this team 
		SelectLeaderEachTeam(pBot, pBot->iTeam);

		pBot->bCheckWeaponSwitch=FALSE;
	}


	/* FIXME: The following Timers aren't frame independant so
	it varies on slower/faster computers*/
	// Increase Reaction Time
	pBot->f_actual_reaction_time+=0.2;
	if(pBot->f_actual_reaction_time>pBot->f_ideal_reaction_time)
		pBot->f_actual_reaction_time=pBot->f_ideal_reaction_time;
	
	// Bot could be blinded by FlashBang or Smoke, recover from it
	pBot->f_view_distance+=3.0;
	if(pBot->f_view_distance>pBot->f_maxview_distance)
		pBot->f_view_distance=pBot->f_maxview_distance;

	// Maxspeed is set in ClientSetMaxspeed
	pBot->f_move_speed = pBot->f_max_speed;

	if (pBot->prev_time <= gpGlobals->time)
	{
		// see how far bot has moved since the previous position...
		v_diff = pBot->v_prev_origin - pEdict->v.origin;
		moved_distance = v_diff.Length();
		
		// save current position as previous
		pBot->v_prev_origin = pEdict->v.origin;
		pBot->prev_time = gpGlobals->time + 0.2;
	}
	else
	{
		moved_distance = 2.0;
	}

	Vector v_direction;
	Vector v_angles;

	// If there's some Radio Message to respond, check it
	if(pBot->iRadioOrder!=0)
		BotCheckRadioCommands(pBot);

	// Do all Sensing, calculate/filter all Actions here 
	BotSetConditions(pBot);

	int iDestIndex = pBot->curr_wpt_index;
	pBot->bCheckTerrain = TRUE;
	bool bMoveToGoal = TRUE;
	Vector v_dest;
	Vector v_src;
	bool bShootLastPosition = FALSE;
	bool bMadShoot = FALSE;
	pBot->bWantsToFire = FALSE;

	// Get affected by SmokeGrenades (very basic!) and sense
	// Grenades flying towards us
	BotCheckSmokeGrenades(pBot);
	pBot->bUsingGrenade = FALSE;

#ifdef DEBUG
	report_log (LOG_BOTTASK, "BotThink","Task = %d",BotGetSafeTask(pBot)->iTask);
#endif

	switch(BotGetSafeTask(pBot)->iTask)
	{
		// Normal (roaming) Task
	case TASK_NORMAL:
		pBot->iAimFlags |= AIM_DEST;
		// User forced a Waypoint as a Goal ?
		if(g_iDebugGoalIndex != -1)
		{
			if(BotGetSafeTask(pBot)->iData != g_iDebugGoalIndex)
			{
				DeleteSearchNodes(pBot);
				pBot->pTasks->iData = g_iDebugGoalIndex;
			}
		}
		if(BotUsesSniper(pBot) && pBot->pBotEnemy == NULL)
		{
			// Try zooming out to move fast again
			if(pEdict->v.fov != 90)
				pEdict->v.button|=IN_ATTACK2;
		}

		int i;
		// If Bomb planted and it's a Counter
		// calculate new path to Bomb Point if he's not already heading for
		if((g_bBombPlanted) && (pBot->iTeam == 1))
		{
			if(BotGetSafeTask(pBot)->iData!=-1)
			{
				if(!(paths[pBot->pTasks->iData]->flags & W_FL_GOAL))
				{
					DeleteSearchNodes(pBot);
					pBot->pTasks->iData = -1;
				}
			}
		}

		// Reached the destination (goal) waypoint ? 
		if(BotDoWaypointNav(pBot))
		{

			// Spray Logo sometimes if allowed to do so
			if(!pBot->bLogoSprayed && g_bBotSpray)
			{
				if(RANDOM_LONG(1,100)<50)
				{
					bottask_t TempTask = {NULL,NULL,TASK_SPRAYLOGO,TASKPRI_SPRAYLOGO,-1,
						gpGlobals->time + 1.0,FALSE};
					BotPushTask(pBot,&TempTask);
				}
			}
			BotTaskComplete(pBot);
			pBot->prev_goal_index=-1;

			// Reached Waypoint is a Camp Waypoint
			if(paths[pBot->curr_wpt_index]->flags & W_FL_CAMP)
			{
				// Check if Bot has got a primary weapon and hasn't camped before
				if((BotHasPrimaryWeapon(pBot)) && (pBot->fTimeCamping+10.0<gpGlobals->time))
				{
					bool bCampingAllowed = TRUE;
					// Check if it's not allowed for this team to camp here
					if(pBot->iTeam == 0)
					{
						if(paths[pBot->curr_wpt_index]->flags & W_FL_COUNTER)
							bCampingAllowed = FALSE;
					}
					else
					{
						if(paths[pBot->curr_wpt_index]->flags & W_FL_TERRORIST)
							bCampingAllowed = FALSE;
					}
					
					// Check if another Bot is already camping here
					for(int c=0;c<MAXBOTSNUMBER;c++)
					{
						bot_t *pOtherBot = &bots[c];
						if(pOtherBot->is_used)
						{
							if(pOtherBot == pBot)
								continue;
							if((IsAlive(pOtherBot->pEdict)) && (pOtherBot->iTeam == pBot->iTeam))
							{
								if(pOtherBot->curr_wpt_index == pBot->curr_wpt_index)
								{
									bCampingAllowed = FALSE;
								}
							}
						}
					}
					if(bCampingAllowed)
					{
						// Crouched camping here ?
						if(paths[pBot->curr_wpt_index]->flags & W_FL_CROUCH)
							pBot->iCampButtons = IN_DUCK;
						else
							pBot->iCampButtons = 0;
						if(!(pBot->iStates & STATE_SEEINGENEMY))
						{
							pEdict->v.button |= IN_RELOAD;	// reload - just to be sure
							pBot->bIsReloading = TRUE;
						}
						UTIL_MakeVectors( pEdict->v.v_angle );
						pBot->fTimeCamping=gpGlobals->time+
							RANDOM_FLOAT(BotSkillDelays[pBot->bot_skill/20].fBotCampStartDelay,BotSkillDelays[pBot->bot_skill/20].fBotCampEndDelay);
						bottask_t TempTask = {NULL,NULL,TASK_CAMP,TASKPRI_CAMP,-1,pBot->fTimeCamping,TRUE};
						BotPushTask(pBot,&TempTask);
						Vector v_src;
						v_src.x=paths[pBot->curr_wpt_index]->fcampstartx;
						v_src.y=paths[pBot->curr_wpt_index]->fcampstarty;
						v_src.z=0;
						pBot->vecCamp = v_src;
						pBot->iAimFlags |= AIM_CAMP;
						pBot->iCampDirection=0;
						// Tell the world we're camping
						BotPlayRadioMessage(pBot,RADIO_IMINPOSITION);
						bMoveToGoal = FALSE;
						pBot->bCheckTerrain = FALSE;
						pBot->f_move_speed = 0;
						pBot->f_sidemove_speed = 0;
					}
				}
			}
			else
			{
				// Some Goal Waypoints are map dependant so check it out...
				switch(g_iMapType)
				{
				case MAP_CS:
					// CT Bot has some stupid hossies following ?
					if(BotHasHostage(pBot) && pBot->iTeam == 1)
					{
						// and reached a Rescue Point ?
						if((paths[pBot->curr_wpt_index]->flags & W_FL_RESCUE))
						{
							// Clear Array of Hostage ptrs
							for(i=0;i<MAX_HOSTAGES;i++)
								pBot->pHostages[i]=NULL;
							// Notify T Bots that there's a rescue going on
							g_bHostageRescued = TRUE;
						}
					}
					break;
				case MAP_DE:
					//Reached Goal Waypoint
					if(paths[pBot->curr_wpt_index]->flags & W_FL_GOAL)
					{
						// Is it a Terrorist carrying the bomb ?
						if((pBot->iTeam==0) && (pEdict->v.weapons & (1<<CS_WEAPON_C4)))
//						if((iTeam==0) && (pBot->bot_weapons & (1<<CS_WEAPON_C4)))
						{
							UTIL_SelectItem(pEdict, "weapon_c4");
							bottask_t TempTask = {NULL,NULL,TASK_PLANTBOMB,TASKPRI_PLANTBOMB,-1,0.0,FALSE};
							BotPushTask(pBot,&TempTask);
							pBot->f_bomb_time = gpGlobals->time+4.0;
							// Tell Teammates to move over here...
							BotPlayRadioMessage(pBot,RADIO_NEEDBACKUP);
						}
						// Counter searching the Bomb ?
						else if((pBot->iTeam == 1) && (g_bBombPlanted))
						{
							CTBombPointClear(pBot->curr_wpt_index);
							BotPlayRadioMessage(pBot,RADIO_SECTORCLEAR);
						}
					}
					break;
				}
			}
		}
		// No more Nodes to follow - search new ones
		else if(!BotGoalIsValid(pBot))
		{
			pBot->f_move_speed = pBot->f_max_speed;
			DeleteSearchNodes(pBot);
			// Did we already decide about a goal before ?
			if(BotGetSafeTask(pBot)->iData != -1)
				iDestIndex=pBot->pTasks->iData;
			else
				iDestIndex=BotFindGoal(pBot);
			pBot->prev_goal_index=iDestIndex;
			// Remember Index
			BotGetSafeTask(pBot)->iData=iDestIndex;
			if(iDestIndex!=pBot->curr_wpt_index)
			{
				// Do Pathfinding if it's not the current waypoint
				pBot->pWayNodesStart = FindPath(pBot,pBot->curr_wpt_index,iDestIndex,pBot->byPathType);
				pBot->pWaypointNodes = pBot->pWayNodesStart;
			}
		}
		else
		{
			if(!(pEdict->v.flags & FL_DUCKING)
				&& pBot->fMinSpeed != pBot->f_max_speed)
			{
				pBot->f_move_speed = pBot->fMinSpeed;
			}
		}
		break;

		// Bot sprays messy Logos all over the place...
	case TASK_SPRAYLOGO:
		pBot->iAimFlags |= AIM_ENTITY;
		// Bot didn't spray this round ?
		if(!pBot->bLogoSprayed && pBot->pTasks->fTime > gpGlobals->time)
		{
			UTIL_MakeVectors ( pEdict->v.v_angle);
			Vector vecSprayPos = pEdict->v.origin+pEdict->v.view_ofs+gpGlobals->v_forward*128;
			UTIL_TraceLine(pEdict->v.origin+pEdict->v.view_ofs,vecSprayPos,
				ignore_monsters,pEdict->v.pContainingEntity, &tr);
			// No Wall in Front ?
			if (tr.flFraction >= 1.0)
				vecSprayPos.z-=128.0;
			pBot->vecEntity = vecSprayPos;

			if(pBot->pTasks->fTime - 0.5 < gpGlobals->time)
			{
				// Emit Spraycan sound
				EMIT_SOUND_DYN2(pEdict, CHAN_VOICE, "player/sprayer.wav", 1.0,
					ATTN_NORM, 0, 100);
				UTIL_TraceLine(pEdict->v.origin+pEdict->v.view_ofs,pEdict->v.origin+pEdict->v.view_ofs+gpGlobals->v_forward*128,
					ignore_monsters,pEdict->v.pContainingEntity, &tr);
				// Paint the actual Logo Decal
				UTIL_DecalTrace(&tr,&szSprayNames[pBot->iSprayLogo][0]);
				pBot->bLogoSprayed = TRUE;
			}
		}
		else
			BotTaskComplete(pBot);
		bMoveToGoal = FALSE;
		pBot->bCheckTerrain = FALSE;
		pBot->f_wpt_timeset = gpGlobals->time;		
		pBot->f_move_speed = 0;
		pBot->f_sidemove_speed=0.0;
		break;

		// Hunt down Enemy
	case TASK_ENEMYHUNT:
		pBot->iAimFlags |= AIM_DEST;
		// Reached last Enemy Pos ?
		if(pBot->pBotEnemy != NULL || pBot->pLastEnemy == NULL)
		{
			// Forget about it...
			BotRemoveCertainTask(pBot,TASK_ENEMYHUNT);
			pBot->prev_goal_index = -1;
		}
		else if(BotDoWaypointNav(pBot))
		{
			// Forget about it...
			BotTaskComplete(pBot);
			pBot->prev_goal_index = -1;
			pBot->vecLastEnemyOrigin = Vector(0,0,0);
		}
		// Do we need to calculate a new Path ?
		else if(pBot->pWaypointNodes==NULL)
		{
			DeleteSearchNodes(pBot);
			// Is there a remembered Index ?
			if(BotGetSafeTask(pBot)->iData!=-1 &&
				(BotGetSafeTask(pBot)->iData<g_iNumWaypoints))
			{
				iDestIndex=pBot->pTasks->iData;
			}
			// No. We need to find a new one
			else
			{
				float min_distance;
				float distance;
				min_distance = 9999.0;
				// Search nearest waypoint to enemy
				for (i=0; i < g_iNumWaypoints; i++)
				{
					distance = (paths[i]->origin - pBot->vecLastEnemyOrigin).Length();
					
					if (distance < min_distance)
					{
						iDestIndex = i;
						min_distance = distance;
					}
				}
			}
			
			// Remember Index
			pBot->prev_goal_index=iDestIndex;
			BotGetSafeTask(pBot)->iData=iDestIndex;
			pBot->pWayNodesStart=FindPath(pBot,pBot->curr_wpt_index,iDestIndex,0);
			pBot->pWaypointNodes=pBot->pWayNodesStart;
		}

		// Bots skill higher than 70 ?
		if(pBot->bot_skill > 70)
		{
			// Then make him move slow if near Enemy 
			if (!(pBot->curr_move_flags & C_FL_JUMP))
			{
				if(pBot->curr_wpt_index!=-1)
				{
					if((paths[pBot->curr_wpt_index]->Radius < 32) && !pBot->bOnLadder
					&& !pBot->bInWater && pBot->f_bot_see_enemy_time+1.0 < gpGlobals->time)
					{
						pEdict->v.button |=IN_DUCK;
					}
				}
				Vector v_diff = pBot->vecLastEnemyOrigin - pEdict->v.origin;
				float fDistance=v_diff.Length();
				if(fDistance<700.0)
				{
					if (!(pEdict->v.flags & FL_DUCKING) )
						pBot->f_move_speed = 120.0;
				}
			}
		}
		break;

		// Bot seeks Cover from Enemy
	case TASK_SEEKCOVER:
		pBot->iAimFlags |= AIM_DEST;
		if(pBot->pLastEnemy == NULL || !IsAlive(pBot->pLastEnemy))
		{
			BotTaskComplete(pBot);
			pBot->prev_goal_index = -1;
		}
		// Reached final Cover waypoint ?
		else if(BotDoWaypointNav(pBot))
		{
			pBot->byPathType = 2;
			// Yep. Activate Hide Behaviour
			BotTaskComplete(pBot);
			pBot->prev_goal_index=-1;
			bottask_t TempTask = {NULL,NULL,TASK_HIDE,TASKPRI_HIDE,-1,
				gpGlobals->time + RANDOM_FLOAT(2.0,10.0),FALSE};
			BotPushTask(pBot,&TempTask);
			v_dest = pBot->vecLastEnemyOrigin;
			// Get a valid look direction
			BotGetCampDirection(pBot,&v_dest);
			pBot->iAimFlags |= AIM_CAMP;
			pBot->vecCamp = v_dest;
			pBot->iCampDirection = 0;
			
			// Chosen Waypoint is a Camp Waypoint ? 
			if(paths[pBot->curr_wpt_index]->flags & W_FL_CAMP)
			{
				// Use the existing camp wpt prefs
				if(paths[pBot->curr_wpt_index]->flags & W_FL_CROUCH)
					pBot->iCampButtons=IN_DUCK;
				else
					pBot->iCampButtons=0;
			}
			else
			{
				// Choose a crouch or stand pos
				if((RANDOM_LONG(1,100)<30) &&
					(paths[pBot->curr_wpt_index]->Radius<32))
					pBot->iCampButtons=IN_DUCK;
				else
					pBot->iCampButtons=0;
				// Enter look direction from previously calculated
				// positions
				paths[pBot->curr_wpt_index]->fcampstartx=v_dest.x;
				paths[pBot->curr_wpt_index]->fcampstarty=v_dest.y;
				paths[pBot->curr_wpt_index]->fcampendx=v_dest.x;
				paths[pBot->curr_wpt_index]->fcampendy=v_dest.y;
			}
			int iId=pBot->current_weapon.iId; 
			if((pBot->bIsReloading==FALSE) && ((pBot->current_weapon.iClip<5) && (pBot->current_weapon.iAmmo1!=0) && (weapon_defs[iId].iAmmo1 != -1)))
			{
				pEdict->v.button |= IN_RELOAD;	// reload - just to be sure
				pBot->bIsReloading=TRUE;
			}
			pBot->f_move_speed=0;
			pBot->f_sidemove_speed=0;
			bMoveToGoal=FALSE;
			pBot->bCheckTerrain=FALSE;
		}
		// We didn't choose a Cover Waypoint yet or lost it due
		// to an attack ?
		else if(!BotGoalIsValid(pBot))
		{
			DeleteSearchNodes(pBot);
			if(BotGetSafeTask(pBot)->iData!=-1)
			{
				iDestIndex=pBot->pTasks->iData;
			}
			else
			{
				iDestIndex = BotFindCoverWaypoint(pBot,1024);
				if(iDestIndex == -1)
					iDestIndex = RANDOM_LONG(0,g_iNumWaypoints-1);
			}
			pBot->iCampDirection = 0;
			pBot->prev_goal_index = iDestIndex;
			pBot->pTasks->iData = iDestIndex;
			if(iDestIndex != pBot->curr_wpt_index)
			{
				pBot->pWayNodesStart = FindPath(pBot,pBot->curr_wpt_index,iDestIndex,0);
				pBot->pWaypointNodes = pBot->pWayNodesStart;
			}
			
		}
		break;

		// Plain Attacking
	case TASK_ATTACK:
		bMoveToGoal = FALSE;
		pBot->bCheckTerrain = FALSE;
		if(pBot->pBotEnemy != NULL)
			BotDoAttackMovement(pBot);
		else
		{
			BotTaskComplete(pBot);
			pBot->dest_origin = pBot->vecLastEnemyOrigin;
		}
		pBot->f_wpt_timeset=gpGlobals->time;		
		break;

		// Bot is pausing
	case TASK_PAUSE:
		bMoveToGoal = FALSE;
		pBot->bCheckTerrain = FALSE;
		pBot->f_wpt_timeset = gpGlobals->time;		
		pBot->f_move_speed = 0.0;
		pBot->f_sidemove_speed = 0.0;
		pBot->iAimFlags |= AIM_DEST;
		// Is Bot blinded and above average skill ?
		if(pBot->f_view_distance<500.0 && pBot->bot_skill>60)
		{
			// Go mad !
			pBot->f_move_speed = -abs((pBot->f_view_distance-500.0)/2);
			if(pBot->f_move_speed<-pBot->f_max_speed)
				pBot->f_move_speed = -pBot->f_max_speed;
			Vector v_direction;
			UTIL_MakeVectors( pEdict->v.v_angle );
			pBot->vecCamp = GetGunPosition(pEdict) + gpGlobals->v_forward * 500;
			pBot->iAimFlags |= AIM_OVERRIDE;
			pBot->bWantsToFire = TRUE;
		}
		else
			pEdict->v.button|=pBot->iCampButtons;
		// Stop camping if Time over or
		// gets Hurt by something else than bullets
		if(pBot->pTasks->fTime < gpGlobals->time || pBot->iLastDamageType > 0)
			BotTaskComplete(pBot);
		break;

	// Blinded (flashbanged) Behaviour
	case TASK_BLINDED:

		bMoveToGoal = FALSE;
		pBot->bCheckTerrain = FALSE;
		pBot->f_wpt_timeset = gpGlobals->time;

		switch(pBot->bot_personality)
		{
			// Normal
		case 0:
			if(pBot->bot_skill>60 && pBot->vecLastEnemyOrigin != Vector(0,0,0))
				bShootLastPosition = TRUE;
			break;
			// Psycho
		case 1:
			if(pBot->vecLastEnemyOrigin != Vector(0,0,0))
				bShootLastPosition = TRUE;
			else
				bMadShoot = TRUE;
		}
		// If Bot remembers last Enemy Position 
		if(bShootLastPosition)
		{
			// Face it and shoot
			pBot->vecLookAt = pBot->vecLastEnemyOrigin;
			pBot->bWantsToFire = TRUE;
		}
		// If Bot is mad
		else if(bMadShoot)
		{
			// Just shoot in forward direction
			UTIL_MakeVectors( pEdict->v.v_angle );
			pBot->vecLookAt = GetGunPosition(pEdict) + gpGlobals->v_forward * 500;
			pBot->bWantsToFire = TRUE;
		}

		pBot->f_move_speed = pBot->f_blindmovespeed_forward;
		pBot->f_sidemove_speed = pBot->f_blindmovespeed_side;

		if(pBot->f_blind_time < gpGlobals->time)
			BotTaskComplete(pBot);
		break;

		// Camping Behaviour
	case TASK_CAMP:
		{
		pBot->iAimFlags |= AIM_CAMP;
		pBot->bCheckTerrain = FALSE;
		bMoveToGoal = FALSE;
		// half the reaction time if camping because you're more
		// aware of enemies if camping
		pBot->f_ideal_reaction_time=(RANDOM_FLOAT(BotSkillDelays[pBot->bot_skill/20].fMinSurpriseDelay,
			BotSkillDelays[pBot->bot_skill/20].fMaxSurpriseDelay))/2;
		pBot->f_wpt_timeset = gpGlobals->time;		
		pBot->f_move_speed = 0;
		pBot->f_sidemove_speed=0.0;
		GetValidWaypoint(pBot);
		int iRND=RANDOM_LONG(1,100);
		if(iRND<7)
		{
			float diff;
			
			// Take a random pitch
			v_dest.x = RANDOM_FLOAT(paths[pBot->curr_wpt_index]->fcampstartx,paths[pBot->curr_wpt_index]->fcampendx);
			v_dest.z = 0;
			
			// Switch from 1 direction to the other
			if(pBot->iCampDirection<1)
			{
				v_dest.y=paths[pBot->curr_wpt_index]->fcampstarty;
				diff = abs(pEdict->v.v_angle.y - v_dest.y);
				if(diff<=1)
					pBot->iCampDirection^=1;
				pBot->vecCamp = v_dest;
			}
			else
			{
				v_dest.y=paths[pBot->curr_wpt_index]->fcampendy;
				diff = abs(pEdict->v.v_angle.y - v_dest.y);
				if(diff<=1)
					pBot->iCampDirection^=1;
				pBot->vecCamp = v_dest;
			}
		}
		// Press remembered crouch Button
		pEdict->v.button|=pBot->iCampButtons;

		// Stop camping if Time over or
		// gets Hurt by something else than bullets
		if(pBot->pTasks->fTime < gpGlobals->time || pBot->iLastDamageType > 0)
			BotTaskComplete(pBot);
		break;
		}

		// Hiding Behaviour
	case TASK_HIDE:
		pBot->iAimFlags |= AIM_CAMP;
		pBot->bCheckTerrain=FALSE;
		bMoveToGoal=FALSE;
		// half the reaction time if camping
		pBot->f_ideal_reaction_time=(RANDOM_FLOAT(BotSkillDelays[pBot->bot_skill/20].fMinSurpriseDelay,
			BotSkillDelays[pBot->bot_skill/20].fMaxSurpriseDelay))/2;
		pBot->f_wpt_timeset=gpGlobals->time;		
		pBot->f_move_speed = 0;
		pBot->f_sidemove_speed=0.0;
		GetValidWaypoint(pBot);
		// If we see an enemy and aren't at a good camping
		// point leave the spot
		if(pBot->iStates & STATE_SEEINGENEMY)
		{
			if(!(paths[pBot->curr_wpt_index]->flags & W_FL_CAMP))
			{
				BotTaskComplete(pBot);
				pBot->iCampButtons=0;
				pBot->prev_goal_index=-1;
				if(pBot->pBotEnemy != NULL)
					BotDoAttackMovement(pBot);
				break;
			}
		}
		// If we don't have an enemy we're also free to leave
		else if(pBot->vecLastEnemyOrigin == Vector(0,0,0))
		{
			BotTaskComplete(pBot);
			pBot->iCampButtons=0;
			pBot->prev_goal_index=-1;
			if(BotGetSafeTask(pBot)->iTask == TASK_HIDE)
				BotTaskComplete(pBot);
			break;
		}

		pEdict->v.button|=pBot->iCampButtons;
		pBot->f_wpt_timeset = gpGlobals->time;
		// Stop camping if Time over or
		// gets Hurt by something else than bullets
		if(pBot->pTasks->fTime < gpGlobals->time || pBot->iLastDamageType > 0)
			BotTaskComplete(pBot);
		break;

	// Moves to a Position specified in pBot->vecPosition
	// Has a higher Priority than TASK_NORMAL
	case TASK_MOVETOPOSITION:
		pBot->iAimFlags |= AIM_DEST;
		// Reached destination ?
		if(BotDoWaypointNav(pBot))
		{
			// We're done
			BotTaskComplete(pBot);
			pBot->prev_goal_index = -1;
			pBot->vecPosition = Vector(0,0,0);
		}
		// Didn't choose Goal Waypoint yet ?
		else if(!BotGoalIsValid(pBot))
		{
			DeleteSearchNodes(pBot);
			if(BotGetSafeTask(pBot)->iData!=-1 &&
				(BotGetSafeTask(pBot)->iData<g_iNumWaypoints))
			{
				iDestIndex=pBot->pTasks->iData;
			}
			else
			{
				float min_distance=9999.0;
				float distance;
				// Search nearest waypoint to Position
				for (i=0; i < g_iNumWaypoints; i++)
				{
					distance = (paths[i]->origin - pBot->vecPosition).Length();
					
					if (distance < min_distance)
					{
						iDestIndex = i;
						min_distance = distance;
					}
				}
			}
			pBot->prev_goal_index=iDestIndex;
			pBot->pTasks->iData=iDestIndex;
			pBot->pWayNodesStart=FindPath(pBot,pBot->curr_wpt_index,iDestIndex,0);
			pBot->pWaypointNodes=pBot->pWayNodesStart;
			
		}
		break;

		// Planting the Bomb right now
	case TASK_PLANTBOMB:
		pBot->iAimFlags |= AIM_DEST;
		// We're still in the planting time and got the c4 ?
		if((pBot->f_bomb_time>gpGlobals->time) &&
			(pEdict->v.weapons & (1<<CS_WEAPON_C4)))
		{
			bMoveToGoal=FALSE;
			pBot->bCheckTerrain=FALSE;
			pBot->f_wpt_timeset=gpGlobals->time;		
			pEdict->v.button|=IN_ATTACK;
			pEdict->v.button|=IN_DUCK;
			pBot->f_move_speed=0;
			pBot->f_sidemove_speed=0;
		}
		// Done with planting
		else
		{
			pBot->f_bomb_time = 0.0;
			BotTaskComplete(pBot);
			if(!(pEdict->v.weapons & (1<<CS_WEAPON_C4)))
			{
				// stop all Counter Bots from camping, so they search the
				// Bomb
				for (int i = 0; i < gpGlobals->maxClients; i++)
				{
					if((ThreatTab[i].IsUsed==FALSE) || (ThreatTab[i].IsAlive==FALSE) || (ThreatTab[i].iTeam!=1))
						continue;
					int index = UTIL_GetBotIndex(ThreatTab[i].pEdict);
					if(index!=-1)
						BotRemoveCertainTask(&bots[index],TASK_CAMP);
				}
				// Notify the Team of this heroic action
				strcpy(pBot->szMiscStrings,"Planted the Bomb!\n");
				BotPushMessageQueue(pBot,MSG_CS_TEAMSAY);
				
				DeleteSearchNodes(pBot);
				// Push camp task on to stack
				float f_c4timer = CVAR_GET_FLOAT("mp_c4timer");
				bottask_t TempTask = {NULL,NULL,TASK_CAMP,TASKPRI_CAMP,-1,
					gpGlobals->time+(f_c4timer/2),TRUE};
				BotPushTask(pBot,&TempTask);
				// Push Move Command
				TempTask.iTask = TASK_MOVETOPOSITION;
				TempTask.fDesire = TASKPRI_MOVETOPOSITION;
				TempTask.iData = BotFindDefendWaypoint(pBot,pEdict->v.origin);
				BotPushTask(pBot,&TempTask);
				pBot->iCampButtons|=IN_DUCK;
			}
		}
		break;

		// Bomb defusing Behaviour
	case TASK_DEFUSEBOMB:
		pBot->iAimFlags |= AIM_ENTITY;
		bMoveToGoal=FALSE;
		pBot->bCheckTerrain=FALSE;

		pBot->f_wpt_timeset=gpGlobals->time;
		// Bomb still there ?
		if(pBot->pBotPickupItem!=NULL)
		{
			// Get Face Position
			pBot->vecEntity = pBot->pBotPickupItem->v.origin;
			pEdict->v.button|=IN_USE;
			pEdict->v.button|=IN_DUCK;
		}
		else
			BotTaskComplete(pBot);
		pBot->f_move_speed=0;
		pBot->f_sidemove_speed=0;
		break;

		// Follow User Behaviour
	case TASK_FOLLOWUSER:
		pBot->iAimFlags |= AIM_DEST;
		// Follow User ?
		if(BotFollowUser(pBot))
		{
			// Imitate Users crouching
			pEdict->v.button |= pBot->pBotUser->v.button & IN_DUCK;
			pBot->dest_origin = pBot->pBotUser->v.origin;
			pBot->vecLookAt = pBot->dest_origin;
		}
		else
			BotTaskComplete(pBot);
		break;

		// HE Grenade Throw Behaviour
	case TASK_THROWHEGRENADE:
		pBot->iAimFlags |= AIM_GRENADE;

		v_dest = pBot->vecThrow;
		if(!(pBot->iStates & STATE_SEEINGENEMY))
		{
			pBot->f_move_speed = 0.0;
			pBot->f_sidemove_speed = 0.0;
			bMoveToGoal = FALSE;
		}
		else
		{
			if(!(pBot->iStates & STATE_SUSPECTENEMY) && pBot->pBotEnemy != NULL)
			{
				v_dest = pBot->pBotEnemy->v.origin;
				v_src = pBot->pBotEnemy->v.velocity;
				v_src.z = 0.0;
				v_dest = v_dest + (v_src * 0.3);
			}
		}
		pBot->bUsingGrenade = TRUE;
		pBot->bCheckTerrain = FALSE;

		pBot->vecGrenade = VecCheckThrow ( pEdict, GetGunPosition(pEdict), v_dest, 1.0);
		if(pBot->vecGrenade == g_vecZero && !bMoveToGoal)
			pBot->vecGrenade = VecCheckToss ( pEdict, GetGunPosition(pEdict), v_dest);

		// We're done holding the Button (about to throw) ?
		if(pBot->vecGrenade == g_vecZero || pBot->pTasks->fTime < gpGlobals->time
		||	(pEdict->v.weapons & (1<<CS_WEAPON_HEGRENADE)==0))
		{
			if(pBot->vecGrenade == g_vecZero)
			{
				pBot->f_grenade_check_time = gpGlobals->time + RANDOM_FLOAT(2,5);
				BotSelectBestWeapon(pBot);
			}
			BotTaskComplete(pBot);
		}
		else
		{
			pEdict->v.button |= IN_ATTACK;
			// Select Grenade if we're not already using it
			if(pBot->current_weapon.iId != CS_WEAPON_HEGRENADE)
			{
				if(pBot->current_weapon.iId != CS_WEAPON_INSWITCH)
				{
					SelectWeaponByName(pBot,"weapon_hegrenade");
					pBot->pTasks->fTime = gpGlobals->time + TIME_GRENPRIME;
				}
				else
					pBot->pTasks->fTime = gpGlobals->time + TIME_GRENPRIME;
			}
		}
		pEdict->v.button |= pBot->iCampButtons;
		break;

		// Flashbang Throw Behaviour
		// Basically the same code like for HE's
	case TASK_THROWFLASHBANG:
		pBot->iAimFlags |= AIM_GRENADE;
		v_dest = pBot->vecThrow;
		if(!(pBot->iStates & STATE_SEEINGENEMY))
		{
			pBot->f_move_speed = 0.0;
			pBot->f_sidemove_speed = 0.0;
			bMoveToGoal = FALSE;
		}
		else
		{
			if(!(pBot->iStates & STATE_SUSPECTENEMY) && pBot->pBotEnemy != NULL)
			{
				v_dest = pBot->pBotEnemy->v.origin;
				v_src = pBot->pBotEnemy->v.velocity;
				v_src.z = 0.0;
				v_dest = v_dest + (v_src * 0.3);
			}
		}

		pBot->bUsingGrenade = TRUE;
		pBot->bCheckTerrain = FALSE;

		pBot->vecGrenade = VecCheckThrow ( pEdict, GetGunPosition(pEdict), v_dest, 1.0);
		if(pBot->vecGrenade == g_vecZero && !bMoveToGoal)
			pBot->vecGrenade = VecCheckToss ( pEdict, GetGunPosition(pEdict), v_dest);

		// We're done holding the Button (about to throw) ?
		if(pBot->vecGrenade == g_vecZero || pBot->pTasks->fTime<gpGlobals->time || 
			(pEdict->v.weapons & (1<<CS_WEAPON_FLASHBANG) == 0))
		{
			if(pBot->vecGrenade == g_vecZero)
			{
				pBot->f_grenade_check_time = gpGlobals->time + RANDOM_FLOAT(2,5);
				BotSelectBestWeapon(pBot);
			}
			BotTaskComplete(pBot);
		}
		else
		{
			pEdict->v.button |= IN_ATTACK;
			if(pBot->current_weapon.iId != CS_WEAPON_FLASHBANG)
			{
				if(pBot->current_weapon.iId != CS_WEAPON_INSWITCH)
				{
					SelectWeaponByName(pBot,"weapon_flashbang");
					pBot->pTasks->fTime = gpGlobals->time + TIME_GRENPRIME;
				}
				else
					pBot->pTasks->fTime = gpGlobals->time + TIME_GRENPRIME;
			}
		}
		pEdict->v.button |= pBot->iCampButtons;
		break;

		// Smoke Grenade Throw Behaviour
		// A bit different to the others because it mostly
		// tries to throw the Gren on the ground
	case TASK_THROWSMOKEGRENADE:
		pBot->iAimFlags |= AIM_GRENADE;
		if(!(pBot->iStates & STATE_SEEINGENEMY))
		{
			pBot->f_move_speed = 0.0;
			pBot->f_sidemove_speed = 0.0;
			bMoveToGoal = FALSE;
		}
		pBot->bCheckTerrain = FALSE;
		pBot->bUsingGrenade = TRUE;
		v_src = pBot->vecLastEnemyOrigin;
		v_src = v_src - pEdict->v.velocity;
		// Predict where the enemy is in 0.5 secs 
		if(pBot->pBotEnemy != NULL)
			v_src = v_src + pBot->pBotEnemy->v.velocity*0.5;
		pBot->vecGrenade = v_src;
		if(pBot->pTasks->fTime < gpGlobals->time || 
			(pEdict->v.weapons & (1<<CS_WEAPON_SMOKEGRENADE)==0))
		{
			BotTaskComplete(pBot);
			break;
		}
		pEdict->v.button |= IN_ATTACK;
		if(pBot->current_weapon.iId != CS_WEAPON_SMOKEGRENADE)
		{
			if(pBot->current_weapon.iId != CS_WEAPON_INSWITCH)
			{
				SelectWeaponByName(pBot,"weapon_smokegrenade");
				pBot->pTasks->fTime = gpGlobals->time + TIME_GRENPRIME;
			}
			else
				pBot->pTasks->fTime=gpGlobals->time+TIME_GRENPRIME;
		}
		break;

		// Shooting breakables in the way action
	case TASK_SHOOTBREAKABLE:
		// I'm setting these brackets to avoid the compile error
		// of uninitialised vars
		{

		pBot->iAimFlags |= AIM_OVERRIDE;
			// Breakable destroyed ?
		if(!BotFindBreakable(pBot))
		{
			BotTaskComplete(pBot);
			break;
		}
		pEdict->v.button|=pBot->iCampButtons;

		pBot->bCheckTerrain=FALSE;
		bMoveToGoal=FALSE;
		pBot->f_wpt_timeset=gpGlobals->time;
		v_src = pBot->vecBreakable;
		pBot->vecCamp = v_src;
		
		float flDot = GetShootingConeDeviation(pEdict,&v_src);
		// Is Bot facing the Breakable ?
		if(flDot>=0.90)
		{
			pBot->f_move_speed = 0.0;
			pBot->f_sidemove_speed=0.0;
			pBot->bWantsToFire=TRUE;
		}
		else
		{
			pBot->bCheckTerrain=TRUE;
			bMoveToGoal=TRUE;
		}
		break;
		}

	// Picking up Items and stuff behaviour
	case TASK_PICKUPITEM:
		if(pBot->pBotPickupItem == NULL
		|| FNullEnt(pBot->pBotPickupItem))
		{
			pBot->pBotPickupItem = NULL;
			BotTaskComplete(pBot);
			break;
		}
		// func Models need special origin handling
		if (strncmp("func_", STRING(pBot->pBotPickupItem->v.classname), 5) == 0
		|| FBitSet(pBot->pBotPickupItem->v.flags, FL_MONSTER ))
		{
			v_dest = VecBModelOrigin(pBot->pBotPickupItem);
		}
		else
			v_dest = pBot->pBotPickupItem->v.origin;

		pBot->dest_origin = v_dest;
		pBot->vecEntity = v_dest;
		// find the distance to the item
		float f_item_distance = (v_dest - pEdict->v.origin).Length();
		
		switch(pBot->iPickupType)
		{
		case PICKUP_WEAPON:
			pBot->iAimFlags |= AIM_DEST;
			// Near to Weapon ?
			if(f_item_distance<50)
			{
				// Get current best weapon to check if it's a primary
				// in need to be dropped
				int iWeaponNum = HighestWeaponOfEdict(pEdict);
				if(iWeaponNum > 6)
				{
					if(pBot->current_weapon.iId != cs_weapon_select[iWeaponNum].iId)
					{
						if(pBot->current_weapon.iId != CS_WEAPON_INSWITCH)
							SelectWeaponbyNumber(pBot,iWeaponNum);
					}
					else
						FakeClientCommand(pEdict, "drop","", NULL);

				}
				else
					pBot->iNumWeaponPickups++;
			}
			break;

		case PICKUP_HOSTAGE:
			pBot->iAimFlags |= AIM_ENTITY;
			v_src = pEdict->v.origin + pEdict->v.view_ofs;

			if(f_item_distance<50)
			{
				float angle_to_entity = BotInFieldOfView( pBot, v_dest - v_src );
				// Bot faces hostage ?
				if(angle_to_entity<=10)
				{
					pEdict->v.button|=IN_USE;
					// Notify Team
					strcpy(pBot->szMiscStrings,"Using a Hostage!\n");
					BotPushMessageQueue(pBot,MSG_CS_TEAMSAY);

					// Store ptr to hostage so other Bots don't steal
					// from this one or Bot tries to reuse it
					for(int i=0;i<MAX_HOSTAGES;i++)
					{
						if(pBot->pHostages[i]==NULL)
						{
							pBot->pHostages[i]=pBot->pBotPickupItem;
							pBot->pBotPickupItem=NULL;
							break;
						}
					}
				}
			}
			break;

		case PICKUP_PLANTED_C4:
			pBot->iAimFlags |= AIM_ENTITY;
			// Bomb Defuse needs to be checked
			if((pBot->iTeam == 1) && (f_item_distance<50))
			{
				bMoveToGoal = FALSE;
				pBot->bCheckTerrain = FALSE;
				pBot->f_move_speed = 0;
				pBot->f_sidemove_speed = 0;
				pBot->f_bomb_time = gpGlobals->time;
				pEdict->v.button|=IN_DUCK;
				// Notify Team of defusing
				BotPlayRadioMessage(pBot,RADIO_NEEDBACKUP);
				strcpy(pBot->szMiscStrings,"Trying to defuse the Bomb!\n");
				BotPushMessageQueue(pBot,MSG_CS_TEAMSAY);
				bottask_t TempTask = {NULL,NULL,TASK_DEFUSEBOMB,TASKPRI_DEFUSEBOMB,-1,0.0,FALSE};
				BotPushTask(pBot,&TempTask);
				pBot->f_bomb_time = gpGlobals->time+8.0;
			}
			break;

		case PICKUP_BUTTON:
			pBot->iAimFlags |= AIM_ENTITY;
			// It's safer...
			if(pBot->pBotPickupItem == NULL)
			{
				BotTaskComplete(pBot);
				pBot->iPickupType = PICKUP_NONE;
				break;
			}

			// find angles from bot origin to entity...
			v_src = pEdict->v.origin + pEdict->v.view_ofs;
			float angle_to_entity = BotInFieldOfView( pBot, v_dest - v_src );
			// Near to the Button ?
			if(f_item_distance<50)
			{
				pBot->f_move_speed=0.0;
				pBot->f_sidemove_speed=0.0;
				bMoveToGoal=FALSE;
				pBot->bCheckTerrain=FALSE;
				// Facing it directly ?
				if(angle_to_entity<=10)
				{
					pBot->pItemIgnore = pBot->pBotPickupItem;
					pEdict->v.button |= IN_USE;
					pBot->pBotPickupItem = NULL;
					pBot->iPickupType = PICKUP_NONE;
					BotTaskComplete(pBot);
					pBot->f_use_button_time = gpGlobals->time;
				}
			}
			break;

		}
		break;
	}

	// --- End of executing Task Actions ---

	// Get current Waypoint Flags
	// FIXME: Would make more sense to store it each time
	// in the Bot struct when a Bot gets a new wpt instead doing this here
	if(pBot->curr_wpt_index!=-1)
		pBot->iWPTFlags = paths[pBot->curr_wpt_index]->flags;
	else
		pBot->iWPTFlags = 0;

	BotChooseAimDirection(pBot);
	BotFacePosition(pBot,pBot->vecLookAt,FALSE);
	
	// Get Speed Multiply Factor by dividing Target FPS by
	// real FPS
	float fSpeedFactor = CVAR_GET_FLOAT("fps_max")/(1.0/gpGlobals->frametime);
	if(fSpeedFactor < 1)
		fSpeedFactor = 1.0;
	
	if(pBot->iAimFlags > AIM_ENEMY)
	{
		BotChangeYaw(pBot, pEdict->v.yaw_speed * fSpeedFactor);
		BotChangePitch(pBot,pEdict->v.pitch_speed * fSpeedFactor);
	}
	else
	{
		BotChangeYaw(pBot, pEdict->v.yaw_speed * 0.3 * fSpeedFactor);
		BotChangePitch(pBot,pEdict->v.pitch_speed * 0.3 * fSpeedFactor);
	}

	pEdict->v.angles.x = -pEdict->v.v_angle.x / 3;
	pEdict->v.angles.y = pEdict->v.v_angle.y;
	UTIL_ClampVector(&pEdict->v.angles);
	UTIL_ClampVector(&pEdict->v.v_angle);
	pEdict->v.angles.z = pEdict->v.v_angle.z = 0;

	// Enemy behind Obstacle ? Bot wants to fire ?
	if(pBot->iStates & STATE_SUSPECTENEMY && pBot->bWantsToFire)
	{
		int iTask = pBot->pTasks->iTask;
		// Don't allow shooting through walls when camping 
		if(iTask == TASK_PAUSE || iTask == TASK_CAMP)
			pBot->bWantsToFire = FALSE;
	}

	// The Bots wants to fire at something ?
	if(pBot->bWantsToFire && !pBot->bUsingGrenade
		&&	pBot->f_shoot_time <= gpGlobals->time)
	{
#ifdef DEBUG
	report_log (LOG_BOTCOMBAT, "BotThink","BotWantsToFire");
#endif
		// If Bot didn't fire a bullet
		// try again next frame
		if(!BotFireWeapon(pBot->vecLookAt - GetGunPosition(pEdict),pBot))
			pBot->f_shoot_time = gpGlobals->time;
	}

	// Set the reaction time (surprise momentum) different each frame
	// according to skill
	pBot->f_ideal_reaction_time = RANDOM_FLOAT(BotSkillDelays[pBot->bot_skill/20].fMinSurpriseDelay,
		BotSkillDelays[pBot->bot_skill/20].fMaxSurpriseDelay);

	// Calculate 2 direction Vectors, 1 without the up/down component
	v_direction = pBot->dest_origin - (pEdict->v.origin + (pEdict->v.velocity * pBot->fTimeFrameInterval));
	Vector vecDirectionNormal = v_direction.Normalize();
	Vector vecDirection = vecDirectionNormal;
	vecDirectionNormal.z = 0.0;

	Vector vecMoveAngles = UTIL_VecToAngles(v_direction);
	vecMoveAngles.x = -vecMoveAngles.x;
	vecMoveAngles.z = 0;
	UTIL_ClampVector(&vecMoveAngles);

	// Allowed to move to a destination position ?
	if(bMoveToGoal)
	{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotThink","bMoveToGoal");
#endif
		GetValidWaypoint(pBot);

		// Press duck button if we need to
		if((paths[pBot->curr_wpt_index]->flags & W_FL_CROUCH)
		&& !(paths[pBot->curr_wpt_index]->flags & W_FL_CAMP)
		&& (!(pEdict->v.flags & FL_ONGROUND) && !pBot->bInWater) )
		{
			pEdict->v.button|=IN_DUCK;
		}

		float fDistance = (pBot->dest_origin - (pEdict->v.origin + (pEdict->v.velocity * pBot->fTimeFrameInterval))).Length2D();
		if(fDistance < (pBot->f_move_speed * pBot->fTimeFrameInterval))
			pBot->f_move_speed = fDistance;

		if(pBot->f_move_speed < 2.0)
			pBot->f_move_speed = 2.0;

		pBot->fSideSpeed = pBot->f_sidemove_speed;
		pBot->fForwardSpeed = pBot->f_move_speed;
		pBot->fTimeWaypointMove = gpGlobals->time;

	// Special Movement for climbing & swimming here
		if(pBot->bOnLadder || pBot->bInWater)
		{
			Vector vecViewPos = pEdict->v.origin + pEdict->v.view_ofs;
			
			// Check if we need to go forward or back
			int iAngle = BotInFieldOfView( pBot, pBot->dest_origin - vecViewPos );
			// Press the correct buttons
			if(iAngle > 90)
				pEdict->v.button |= IN_BACK;
			else
				pEdict->v.button |= IN_FORWARD;
			if(pBot->bOnLadder)
			{
				// Ladders need view angles pointing up/down
				if(pBot->ladder_dir == LADDER_UP)
				{
					if(pEdict->v.button & IN_FORWARD)
						pEdict->v.v_angle.x = -60;
					else
						pEdict->v.v_angle.x = 60;
				}
				else
				{
					if(pEdict->v.button & IN_FORWARD)
						pEdict->v.v_angle.x = 60;
					else
						pEdict->v.v_angle.x = -60;
				}
				pEdict->v.angles.x = -pEdict->v.v_angle.x / 3;
				vecMoveAngles.x = pEdict->v.v_angle.x;
			}
			else
			{
				if(vecMoveAngles.x > 60.0)
					pEdict->v.button |= IN_DUCK;
				else if(vecMoveAngles.x < -60.0)
					pEdict->v.button |= IN_JUMP;
			}
		}
	}

	// Are we allowed to check blocking Terrain (and react to it) ?
	if(pBot->bCheckTerrain)
	{

#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotThink","bCheckTerrain");
#endif
		edict_t *pent;
		bool bBackOffPlayer = FALSE;
		bool bBotIsStuck = FALSE;

		Vector v_angles = UTIL_VecToAngles(v_direction);
		
		// Test if there's a shootable breakable
		// in our way
		v_src = pEdict->v.origin + pEdict->v.view_ofs;  // EyePosition()
		v_dest = v_src + vecDirection * 50;
		UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters,
			pEdict->v.pContainingEntity, &tr);
		if (tr.flFraction != 1.0)
		{
			if(strcmp("func_breakable",STRING(tr.pHit->v.classname))==0)
			{
				pent=tr.pHit;
				if(IsShootableBreakable(pent))
				{
					pBot->pShootBreakable=pent;
					pBot->iCampButtons=pEdict->v.button & IN_DUCK;
					bottask_t TempTask = {NULL,NULL,TASK_SHOOTBREAKABLE,TASKPRI_SHOOTBREAKABLE,
						-1,0.0,FALSE};
					BotPushTask(pBot,&TempTask);
				}
			}
		}
		else
		{
			v_src = pEdict->v.origin;
			v_dest = v_src + vecDirection * 50;
			UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters,dont_ignore_glass,
				pEdict->v.pContainingEntity, &tr);
			if (tr.flFraction != 1.0)
			{
				if(strcmp("func_breakable",STRING(tr.pHit->v.classname))==0)
				{
					pent=tr.pHit;
					// Check if this isn't a triggered (bomb) breakable and
					// if it takes damage. If true, shoot the crap!
					if(IsShootableBreakable(pent))
					{
						pBot->pShootBreakable=pent;
						pBot->iCampButtons=pEdict->v.button & IN_DUCK;
						bottask_t TempTask = {NULL,NULL,TASK_SHOOTBREAKABLE,TASKPRI_SHOOTBREAKABLE,
							-1,0.0,FALSE};
						BotPushTask(pBot,&TempTask);
					}
				}
			}
		}

		// No Breakable blocking ?
		if(pBot->pShootBreakable == NULL)
		{
			// Standing still, no need to check ?
			// FIXME: Doesn't care for ladder movement (handled separately) 
			// should be included in some way
			if(pBot->f_move_speed!=0 || pBot->f_sidemove_speed!=0)
			{
				pent = NULL;
				edict_t *pNearestPlayer = NULL;
				float f_nearestdistance = 256.0;
				float f_distance_now;
				float f_distance_moved;

				// Find nearest player to Bot
				while ((pent = UTIL_FindEntityInSphere( pent, pEdict->v.origin, pBot->f_max_speed)) != NULL)
				{
					// Spectator or not drawn ?
					if (pent->v.effects & EF_NODRAW)
						continue;
					// Player Entity ?
					if(pent->v.flags & (FL_CLIENT|FL_FAKECLIENT))
					{
						// Our Team, alive and not myself ?
						if((UTIL_GetTeam(pent)!=UTIL_GetTeam(pEdict)) ||
							(!IsAlive(pent)) || (pent==pEdict))
							continue;
						f_distance_now = (pent->v.origin-pEdict->v.origin).Length();
						if(f_distance_now<f_nearestdistance)
						{
							pNearestPlayer = pent;
							f_nearestdistance = f_distance_now;
						}
					}
				}


				// Found somebody ?
				if(pNearestPlayer!=NULL)
				{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotThink","PlayerAvoidance");
#endif
					Vector vecOther = pNearestPlayer->v.origin;
					// Use our Movement angles
					UTIL_MakeVectors (vecMoveAngles );
					
					// Cache it for optimisation
					float fTimeRange = pBot->fTimeFrameInterval;
					// Try to predict where we should be next frame
					Vector vecMoved = pEdict->v.origin+((gpGlobals->v_forward*pBot->f_move_speed) * fTimeRange);
					vecMoved=vecMoved+((gpGlobals->v_right*pBot->f_sidemove_speed) * fTimeRange);
					vecMoved=vecMoved+(pEdict->v.velocity * fTimeRange);
					f_distance_moved = (vecOther - vecMoved).Length();
					vecOther = vecOther + pNearestPlayer->v.velocity * fTimeRange;
					float fDistanceNextFrame = (vecOther - pEdict->v.origin).Length();
					// Is Player that near now or in future that we need to steer away ?
					if(f_distance_moved<=48.0 || f_nearestdistance<=56.0)
					{
						bBackOffPlayer = TRUE;

						BotResetCollideState(pBot);
						Vector2D	vec2DirToPoint; 
						Vector2D	vec2RightSide;
						
						// to start strafing, we have to first figure out if the target is on the left side or right side
						vec2DirToPoint = ( pEdict->v.origin - pNearestPlayer->v.origin ).Make2D().Normalize();
						vec2RightSide = gpGlobals->v_right.Make2D().Normalize();
						
						if ( DotProduct ( vec2DirToPoint, vec2RightSide ) > 0 )
						{
							if (f_nearestdistance <= 56.0)
								pBot->f_move_speed = -pBot->f_max_speed;
							BotSetStrafeSpeed(pBot,vecDirectionNormal,pBot->f_max_speed);
						}
						else
						{
							if (f_nearestdistance <= 56.0)
								pBot->f_move_speed = -pBot->f_max_speed;
							BotSetStrafeSpeed(pBot,vecDirectionNormal,-pBot->f_max_speed);
						}
						if(moved_distance < 2.0)
							pEdict->v.button |= IN_DUCK;
					}
				}
			}


			// No Player collision ?
			if(!bBackOffPlayer
			&& pBot->fNoCollTime < gpGlobals->time && BotGetSafeTask(pBot)->iTask != TASK_ATTACK)
			{
				// Didn't we move enough previously ?
				if(moved_distance < 2.0 && pBot->prev_speed >= 1.0)
				{
					// Then consider being stuck
					pBot->prev_time = gpGlobals->time;
					bBotIsStuck = TRUE;
					if(pBot->f_firstcollide_time == 0.0)
						pBot->f_firstcollide_time = gpGlobals->time + 0.2;
				}
				// Not stuck yet
				else
				{
					// Test if there's something ahead blocking the way
					if(BotCantMoveForward(pBot,vecDirectionNormal,&tr) && !pBot->bOnLadder)
					{
						if(pBot->f_firstcollide_time == 0.0)
							pBot->f_firstcollide_time = gpGlobals->time+0.2;
						else if(pBot->f_firstcollide_time <= gpGlobals->time)
							bBotIsStuck = TRUE;
					}
					else
						pBot->f_firstcollide_time = 0.0;
				}

				// Not stuck ?
				if(!bBotIsStuck)
				{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotThink","!bBotIsStuck");
#endif
					if(pBot->f_probe_time + 0.5 < gpGlobals->time)
					{
						// Reset Collision Memory if not being stuck
						// for 0.5 secs
						BotResetCollideState(pBot);
					}
					else
					{
						// Remember to keep pressing duck if it was necessary ago
						if(pBot->cCollideMoves[pBot->cCollStateIndex] == COLLISION_DUCK)
							pEdict->v.button |= IN_DUCK;
					}
				}
				// Bot is stuck !!
				else
				{
#ifdef DEBUG
	report_log (LOG_BOTNAV, "BotThink","bBotIsStuck");
#endif
					// Not yet decided what to do ?
					if(pBot->cCollisionState == COLLISION_NOTDECIDED)
					{
						char cBits=0;

						if(pBot->bInWater)
						{
							cBits |= PROBE_JUMP;
							cBits |= PROBE_STRAFE;
						}
						else
						{
							cBits |= PROBE_JUMP;
							cBits |= PROBE_DUCK;
							cBits |= PROBE_STRAFE;
						}

						// Collision check allowed if not flying
						// through the air
						if(pEdict->v.flags & FL_ONGROUND)
//							|| bInWater || !bOnLadder)
						{
							char cState[8];
							int i=0;

							// First 4 Entries hold the possible
							// Collision States
							cState[i]=COLLISION_JUMP;
							i++;
							cState[i]=COLLISION_DUCK;
							i++;
							cState[i]=COLLISION_STRAFELEFT;
							i++;
							cState[i]=COLLISION_STRAFERIGHT;
							i++;


							// Now weight all possible States
							if(cBits & PROBE_JUMP)
							{
								cState[i]=0;
								if(BotCanJumpUp( pBot,vecDirectionNormal))
									cState[i]+=10;
								if(pBot->dest_origin.z>=pEdict->v.origin.z+18.0)
									cState[i]+=5;
								if(BotEntityIsVisible(pBot, pBot->dest_origin))
								{
									UTIL_MakeVectors ( vecMoveAngles);
									v_src=pEdict->v.origin+pEdict->v.view_ofs;
									v_src=v_src+gpGlobals->v_right*15;
									UTIL_TraceLine( v_src, pBot->dest_origin, ignore_monsters,ignore_glass,
										pEdict->v.pContainingEntity, &tr);
									if (tr.flFraction >= 1.0)
									{
										v_src=pEdict->v.origin+pEdict->v.view_ofs;
										v_src=v_src + (-gpGlobals->v_right*15);
										UTIL_TraceLine( v_src, pBot->dest_origin, ignore_monsters,ignore_glass,
											pEdict->v.pContainingEntity, &tr);
										if (tr.flFraction >= 1.0)
											cState[i]+=5;
									}
								}
								if (pEdict->v.flags & FL_DUCKING)
									v_src = pEdict->v.origin;
								else
									v_src = pEdict->v.origin + Vector(0, 0, -17);
								v_dest=v_src + vecDirectionNormal*30;
								UTIL_TraceLine( v_src, v_dest, ignore_monsters,ignore_glass,
									pEdict->v.pContainingEntity, &tr);
								if (tr.flFraction != 1.0)
									cState[i]+=10;
							}
							else
								cState[i]=0;
							i++;
							if(cBits & PROBE_DUCK)
							{
								cState[i]=0;
								if(BotCanDuckUnder( pBot,vecDirectionNormal))
									cState[i]+=10;
								if(pBot->dest_origin.z+36.0<=pEdict->v.origin.z &&
									BotEntityIsVisible(pBot, pBot->dest_origin))
								{
									cState[i]+=5;
								}
							}
							else
								cState[i]=0;
							i++;
							if(cBits & PROBE_STRAFE)
							{
								cState[i]=0;
								cState[i+1]=0;
								Vector2D	vec2DirToPoint; 
								Vector2D	vec2RightSide;
								
								// to start strafing, we have to first figure out if the target is on the left side or right side
								UTIL_MakeVectors (vecMoveAngles);
								
								vec2DirToPoint = ( pEdict->v.origin - pBot->dest_origin).Make2D().Normalize();
								vec2RightSide = gpGlobals->v_right.Make2D().Normalize();
								bool bDirRight=FALSE;
								bool bDirLeft=FALSE;
								bool bBlockedLeft=FALSE;
								bool bBlockedRight=FALSE;
								
								if(pBot->f_move_speed>0)
									vecDirection=gpGlobals->v_forward;
								else
									vecDirection=-gpGlobals->v_forward;
								if ( DotProduct ( vec2DirToPoint, vec2RightSide ) > 0 )
									bDirRight=TRUE;
								else
									bDirLeft=TRUE;
								// Now check which side is blocked
								v_src = pEdict->v.origin+gpGlobals->v_right*32;
								v_dest = v_src + vecDirection * 32;
								UTIL_TraceLine(v_src, v_dest, ignore_monsters,
									pEdict->v.pContainingEntity, &tr);
								if (tr.flFraction != 1.0)
									bBlockedRight=TRUE;
								v_src = pEdict->v.origin+-gpGlobals->v_right*32;
								v_dest = v_src + vecDirection * 32;
								UTIL_TraceLine(v_src, v_dest, ignore_monsters,
									pEdict->v.pContainingEntity, &tr);
								if (tr.flFraction != 1.0)
									bBlockedLeft=TRUE;
								if(bDirLeft)
									cState[i]+=5;
								else
									cState[i]-=5;
								if(bBlockedLeft)
									cState[i]-=5;
								i++;
								if(bDirRight)
									cState[i]+=5;
								else
									cState[i]-=5;
								if(bBlockedRight)
									cState[i]-=5;
							}
							else
							{
								cState[i]=0;
								i++;
								cState[i]=0;
							}

							// Weighted all possible Moves, now
							// sort them to start with most probable
							char cTemp;
							bool bSorting;
							do
							{
								bSorting=FALSE;
								for(i=0;i<3;i++)
								{
									if(cState[i+4]<cState[i+1+4])
									{
										cTemp=cState[i];
										cState[i]=cState[i+1];
										cState[i+1]=cTemp;
										cTemp=cState[i+4];
										cState[i+4]=cState[i+1+4];
										cState[i+1+4]=cTemp;
										bSorting=TRUE;
									}
								}
							} while(bSorting);
							for(i=0;i<4;i++)
								pBot->cCollideMoves[i]=cState[i];
							pBot->f_collide_time=gpGlobals->time;
							pBot->f_probe_time=gpGlobals->time+0.5;
							pBot->cCollisionProbeBits=cBits;
							pBot->cCollisionState=COLLISION_PROBING;
							pBot->cCollStateIndex=0;
						}
					}

					if(pBot->cCollisionState==COLLISION_PROBING)
					{
						if(pBot->f_probe_time<gpGlobals->time)
						{
							pBot->cCollStateIndex++;
							pBot->f_probe_time=gpGlobals->time+0.5;
							if(pBot->cCollStateIndex>4)
							{
								pBot->f_wpt_timeset=gpGlobals->time-5.0;
								BotResetCollideState(pBot);
							}
						}
						if(pBot->cCollStateIndex<=4)
						{
							switch(pBot->cCollideMoves[pBot->cCollStateIndex])
							{
							case COLLISION_JUMP:
								if((pEdict->v.flags & FL_ONGROUND)
									|| pBot->bInWater || !pBot->bOnLadder)
								{
									pEdict->v.button |= IN_JUMP;
								}
								break;

							case COLLISION_DUCK:
								pEdict->v.button |= IN_DUCK;
								break;

							case COLLISION_STRAFELEFT:
								BotSetStrafeSpeed(pBot,vecDirectionNormal,-pBot->f_max_speed);
								break;

							case COLLISION_STRAFERIGHT:
								BotSetStrafeSpeed(pBot,vecDirectionNormal,pBot->f_max_speed);
								break;
							}
						}
					}
				}

			}
		}
	}

	// Must avoid a Grenade ?
	if(pBot->cAvoidGrenade!=0)
	{
#ifdef DEBUG
	report_log (LOG_BOTNAV, "BotThink","AvoidGrenade");
#endif
		// Don't duck to get away faster
		pEdict->v.button &= ~IN_DUCK;
		pBot->f_move_speed=-pBot->f_max_speed;
		pBot->f_sidemove_speed = pBot->f_max_speed * pBot->cAvoidGrenade;
	}

	// FIXME: time to reach waypoint should be calculated when getting this waypoint
	// depending on maxspeed and movetype instead of being hardcoded
	if((pBot->f_wpt_timeset+5.0<gpGlobals->time) && (pBot->pBotEnemy==NULL))
	{
#ifdef DEBUG
	report_log (LOG_BOTNAV, "BotThink","Didn't reach Waypoint in Time !");
#endif
		GetValidWaypoint(pBot);

		// Clear these pointers, Bot might be stuck getting
		// to them
		if(pBot->pBotPickupItem != NULL)
		{
			pBot->pItemIgnore = pBot->pBotPickupItem;
			pBot->pBotPickupItem = NULL;
			pBot->iPickupType = PICKUP_NONE;
			pBot->f_itemcheck_time = gpGlobals->time + 5.0;
		}
		pBot->pShootBreakable = NULL;
	}

	if(pEdict->v.button & IN_JUMP)
		pBot->f_jumptime = gpGlobals->time;

	if(pBot->f_jumptime+1.0>gpGlobals->time)
	{
		if(!(pEdict->v.flags & FL_ONGROUND) && !pBot->bInWater)
			pEdict->v.button|=IN_DUCK;
	}

	if(g_cFunMode != 0)
		DoFunStuff(pBot);

#ifdef DEBUG
	if(!FNullEnt(pHostEdict))
	{
		int iSpecIndex = pHostEdict->v.iuser2;
		if(iSpecIndex == ENTINDEX(pEdict))
		{
			static int iIndex, iGoal, iTask;
			
			if(pBot->pTasks != NULL)
			{
				if(iTask != pBot->pTasks->iTask
					|| iIndex != pBot->curr_wpt_index
					|| iGoal != pBot->pTasks->iData
					|| g_fTimeDebugUpdate < gpGlobals->time)
				{
					iTask = pBot->pTasks->iTask;
					iIndex = pBot->curr_wpt_index;
					iGoal = pBot->pTasks->iData;
					char szTaskname[80];
					
					switch(iTask)
					{
					case TASK_NORMAL:
						sprintf(szTaskname,"TASK_NORMAL");
						break;
					case TASK_PAUSE:
						sprintf(szTaskname,"TASK_PAUSE");
						break;
					case TASK_MOVETOPOSITION:
						sprintf(szTaskname,"TASK_MOVETOPOSITION");
						break;
					case TASK_FOLLOWUSER:
						sprintf(szTaskname,"TASK_FOLLOWUSER");
						break;
					case TASK_PICKUPITEM:
						sprintf(szTaskname,"TASK_PICKUPITEM");
						break;
					case TASK_CAMP:
						sprintf(szTaskname,"TASK_CAMP");
						break;
					case TASK_PLANTBOMB:
						sprintf(szTaskname,"TASK_PLANTBOMB");
						break;
					case TASK_DEFUSEBOMB:
						sprintf(szTaskname,"TASK_DEFUSEBOMB");
						break;
					case TASK_ATTACK:
						sprintf(szTaskname,"TASK_ATTACK");
						break;
					case TASK_ENEMYHUNT:
						sprintf(szTaskname,"TASK_ENEMYHUNT");
						break;
					case TASK_SEEKCOVER:
						sprintf(szTaskname,"TASK_SEEKCOVER");
						break;
					case TASK_THROWHEGRENADE:
						sprintf(szTaskname,"TASK_THROWHEGRENADE");
						break;
					case TASK_THROWFLASHBANG:
						sprintf(szTaskname,"TASK_THROWFLASHBANG");
						break;
					case TASK_THROWSMOKEGRENADE:
						sprintf(szTaskname,"TASK_THROWSMOKEGRENADE");
						break;
					case TASK_SHOOTBREAKABLE:
						sprintf(szTaskname,"TASK_SHOOTBREAKABLE");
						break;
					case TASK_HIDE:
						sprintf(szTaskname,"TASK_HIDE");
						break;
					case TASK_BLINDED:
						sprintf(szTaskname,"TASK_BLINDED");
						break;
					case TASK_SPRAYLOGO:
						sprintf(szTaskname,"TASK_SPRAYLOGO");
						break;
					}
					
					char szEnemyName[80];
					
					if(pBot->pBotEnemy != NULL)
						strcpy(szEnemyName,STRING(pBot->pBotEnemy->v.netname));
					else if(pBot->pLastEnemy != NULL)
					{
						strcpy(szEnemyName,"(L)");
						strcat(szEnemyName,STRING(pBot->pLastEnemy->v.netname));
					}
					else
						strcpy(szEnemyName,"(null)");
					
					char szPickupName[80];
					
					if(pBot->pBotPickupItem != NULL)
						strcpy(szPickupName,STRING(pBot->pBotPickupItem->v.classname));
					else
						strcpy(szPickupName,"(null)");
					
					bot_weapon_select_t *pSelect = &cs_weapon_select[0];
					char cWeaponCount = 0;
					while(pBot->current_weapon.iId != pSelect->iId
						&& cWeaponCount < NUM_WEAPONS)
					{
						pSelect++;
						cWeaponCount++;
					}
					
					char szWeaponName[80];
					
					if(cWeaponCount >= NUM_WEAPONS)
						sprintf(szWeaponName,"UNKNOWN! (%d)",pBot->current_weapon.iId);
					else
						sprintf(szWeaponName,pSelect->weapon_name);
					
					
					char szOutput[512];
					
					sprintf(szOutput,
						"%s - Task: %d=%s Desire:%.02f\n"
						"Item:%s Clip:%d Ammo:%d AimFlags:%X\n"
						"FSP=%.02f SSP=%.02f I=%d PG=%d G=%d T:%.02f MT:%d\n"
						"Enemy=%s Pickup=%s\n",
						STRING(pEdict->v.netname),iTask,szTaskname,pBot->pTasks->fDesire,
						szWeaponName,pBot->current_weapon.iClip,pBot->current_weapon.iAmmo1,
						pBot->iAimFlags,pBot->f_move_speed,pBot->f_sidemove_speed,
						iIndex,pBot->prev_goal_index,iGoal,pBot->f_wpt_timeset - gpGlobals->time,
						pEdict->v.movetype,szEnemyName,szPickupName);
					// Hacked together Version of HUD_DrawString
					MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY,NULL,pHostEdict);
					WRITE_BYTE(TE_TEXTMESSAGE);
					WRITE_BYTE(1);
					WRITE_SHORT(FixedSigned16(-1, 1<<13));
					WRITE_SHORT(FixedSigned16(0, 1<<13));
					WRITE_BYTE(0);
					WRITE_BYTE(255);
					WRITE_BYTE(255);
					WRITE_BYTE(255);
					WRITE_BYTE(0);
					WRITE_BYTE(255);
					WRITE_BYTE(255);
					WRITE_BYTE(255);
					WRITE_BYTE(0);
					WRITE_SHORT( FixedUnsigned16(0, 1<<8 ) );
					WRITE_SHORT( FixedUnsigned16(0, 1<<8 ) );
					WRITE_SHORT( FixedUnsigned16(1.0, 1<<8 ) );
					WRITE_STRING((const char*)&szOutput[0]);
					MESSAGE_END();
					g_fTimeDebugUpdate = gpGlobals->time + 1.0;
			}
			// Show Destination
			DebugShowBeam(pEdict->v.origin,pBot->dest_origin,255,0,0);
		}
		}
	}


#endif
	if(!(pEdict->v.button & (IN_FORWARD | IN_BACK)))
	{
		if(pBot->f_move_speed > 0)
			pEdict->v.button |= IN_FORWARD;
		else
			pEdict->v.button |= IN_BACK;
	}
	if(!(pEdict->v.button & (IN_LEFT | IN_RIGHT)))
	{
		if(pBot->f_sidemove_speed > 0)
			pEdict->v.button |= IN_RIGHT;
		else
			pEdict->v.button |= IN_LEFT;
	}


	// save the previous speed (for checking if stuck)
	pBot->prev_speed = abs(pBot->f_move_speed);

	// Reset Damage
	pBot->iLastDamageType = -1;

	pEdict->v.flags |= FL_FAKECLIENT;
	UTIL_ClampVector(&pEdict->v.angles);
	UTIL_ClampVector(&pEdict->v.v_angle);

	assert(pEdict->v.v_angle.x > -181.0 && pEdict->v.v_angle.x < 181.0
	&& pEdict->v.v_angle.y > -181.0 && pEdict->v.v_angle.y < 181.0
	&& pEdict->v.angles.x > -181.0 && pEdict->v.angles.x < 181.0
	&& pEdict->v.angles.y > -181.0 && pEdict->v.angles.y < 181.0
	&& pEdict->v.ideal_yaw > -181.0 && pEdict->v.ideal_yaw < 181.0
	&& pEdict->v.idealpitch > -181.0 && pEdict->v.idealpitch < 181.0);


	g_engfuncs.pfnRunPlayerMove( pEdict, vecMoveAngles, pBot->f_move_speed,
		pBot->f_sidemove_speed, 0, pEdict->v.button, 0, pBot->msecval);
	// Set Client to frozen if Borg
	if(g_bIsBorg)
		pEdict->v.flags |= FL_FROZEN;
}


bool BotHasHostage(bot_t *pBot)
{
	for(int i=0;i<MAX_HOSTAGES;i++)
	{
		if(pBot->pHostages[i]!=NULL)
			return TRUE;
	}
	return FALSE;
}


void BotResetCollideState(bot_t *pBot)
{
	pBot->f_collide_time = 0.0;
	pBot->f_probe_time = 0.0;
	pBot->cCollisionProbeBits = 0;
	pBot->cCollisionState = COLLISION_NOTDECIDED;
	pBot->cCollStateIndex = 0;
}


// Returns if given location would hurt Bot with
// falling damage
bool IsDeadlyDrop(bot_t *pBot,Vector vecTargetPos)
{
#ifdef DEBUG
	report_log (LOG_BOTMOVE, "BotCheckEdge",NULL);
#endif

	edict_t *pEdict = pBot->pEdict;
	Vector vecBot = pEdict->v.origin;

	TraceResult tr;

	Vector vecMove = UTIL_VecToAngles(vecTargetPos - vecBot);
	vecMove.x = 0;  // reset pitch to 0 (level horizontally)
	vecMove.z = 0;  // reset roll to 0 (straight up and down)
	UTIL_MakeVectors( vecMove );

	Vector v_direction = (vecTargetPos - vecBot).Normalize();  // 1 unit long
	Vector v_check = vecBot;
	Vector v_down = vecBot;
	
	v_down.z = v_down.z - 1000.0;  // straight down 1000 units
	
	UTIL_TraceLine(v_check, v_down, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	// We're not on ground anymore ?
	if(tr.flFraction > 0.036)
		tr.flFraction = 0.036;
	
	float height;
	float last_height = tr.flFraction * 1000.0;  // height from ground
	
	float distance = (vecTargetPos - v_check).Length();  // distance from goal
	
	while (distance > 16.0)
	{
		// move 10 units closer to the goal...
		v_check = v_check + (v_direction * 16.0);
		
		v_down = v_check;
		v_down.z = v_down.z - 1000.0;  // straight down 1000 units
		
		UTIL_TraceLine(v_check, v_down, ignore_monsters,
			pEdict->v.pContainingEntity, &tr);

		// Wall blocking ?
		if(tr.fStartSolid)
			return FALSE;
		
		height = tr.flFraction * 1000.0;  // height from ground

		// Drops more than 100 Units ?
		if (last_height < height - 100 )
			return TRUE; 
		
		last_height = height;
		
		distance = (vecTargetPos - v_check).Length();  // distance from goal
	}
	return FALSE;
}
	

void BotFreeAllMemory()
{
	// Frees waypoint data
	WaypointInit();

	if(pBotExperienceData != NULL)
		delete []pBotExperienceData;
	if(g_pFloydDistanceMatrix != NULL)
		delete []g_pFloydDistanceMatrix;
	if(g_pFloydPathMatrix != NULL)
		delete []g_pFloydPathMatrix;
	if(g_pWithHostageDistMatrix != NULL)
		delete []g_pWithHostageDistMatrix;
	if(g_pWithHostagePathMatrix != NULL)
		delete []g_pWithHostagePathMatrix;



	// Delete all Textnodes/strings

	STRINGNODE* pTempNode = pBotNames;
	STRINGNODE* pNextNode;
	
	while(pTempNode != NULL)
	{
		pNextNode = pTempNode->Next;
		if(pTempNode != NULL)
		{
			if(pTempNode->pszString != NULL)
				delete []pTempNode->pszString;
			delete pTempNode;
		}
		pTempNode = pNextNode;
	}
	
	pTempNode = pBombChat;
	while(pTempNode != NULL)
	{
		pNextNode = pTempNode->Next;
		if(pTempNode != NULL)
		{
			if(pTempNode->pszString != NULL)
				delete []pTempNode->pszString;
			delete pTempNode;
		}
		pTempNode = pNextNode;
	}
	
	pTempNode = pDeadChat;
	while(pTempNode != NULL)
	{
		pNextNode = pTempNode->Next;
		if(pTempNode != NULL)
		{
			if(pTempNode->pszString != NULL)
				delete []pTempNode->pszString;
			delete pTempNode;
		}
		pTempNode = pNextNode;
	}
	
	
	pTempNode = pKillChat;
	while(pTempNode != NULL)
	{
		pNextNode = pTempNode->Next;
		if(pTempNode != NULL)
		{
			if(pTempNode->pszString != NULL)
				delete []pTempNode->pszString;
			delete pTempNode;
		}
		pTempNode = pNextNode;
	}
	
	replynode_t *pTempReply = pChatReplies;
	replynode_t *pNextReply;
	while(pTempReply != NULL)
	{
		pTempNode = pTempReply->pReplies;
		while(pTempNode != NULL)
		{
			pNextNode = pTempNode->Next;
			if(pTempNode != NULL)
			{
				if(pTempNode->pszString != NULL)
					delete []pTempNode->pszString;
				delete pTempNode;
			}
			pTempNode = pNextNode;
		}
		pNextReply = pTempReply->pNextReplyNode;
		delete pTempReply;
		pTempReply = pNextReply;
	}
	
	pTempNode = pChatNoKeyword;
	while(pTempNode != NULL)
	{
		pNextNode = pTempNode->Next;
		if(pTempNode != NULL)
		{
			if(pTempNode->pszString != NULL)
				delete []pTempNode->pszString;
			delete pTempNode;
		}
		pTempNode = pNextNode;
	}

}

// Numerous Debug/Test Routines from here

#ifdef _DEBUG


void DebugShowBeam(Vector vecStartpos,Vector vecEndpos,int r,int g,int b)
{
	edict_t *pEdict=INDEXENT(1);

    WaypointDrawBeam(pEdict, vecStartpos, vecEndpos, 30, 0,r, g, b, 250, 5);
}

void DebugShowAiming(edict_t *pPlayer)
{
	static Vector vecVel = Vector(0,0,0);

	float fDistance;
	edict_t *pEnemy = NULL;

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		if((ThreatTab[i].IsUsed == FALSE) || (ThreatTab[i].IsAlive == FALSE) || (ThreatTab[i].pEdict == pPlayer))
			continue;
		pEnemy = ThreatTab[i].pEdict;
		break;
	}

	if(pEnemy)
	{
		fDistance = (pEnemy->v.origin - pPlayer->v.origin).Length();
		fDistance /= 512.0;
		vecVel = vecVel * 0.1;
		vecVel = vecVel + (vecVel * fDistance);
		Vector vecAim = pEnemy->v.origin + vecVel;
		WaypointDrawBeam(pPlayer, pPlayer->v.origin, vecAim, 30, 0,255, 0, 0, 250, 5);
		WaypointDrawBeam(pEnemy, pPlayer->v.origin, vecAim, 30, 0,255, 0, 0, 250, 5);
		vecVel = -pEnemy->v.velocity;
	}
}

void DebugShowEntities(edict_t *pEdict)
{
	CVAR_SET_FLOAT("developer",1);
	ALERT(at_console,"\n");
	edict_t *pent = NULL;

	while ((pent = UTIL_FindEntityInSphere( pent, pEdict->v.origin, 128.0 )) != NULL)
	{
		if (pent->v.effects & EF_NODRAW)
			ALERT(at_console,"%s (Invisible) IDX=%d\n",STRING(pent->v.classname),ENTINDEX(pent));
		else
			ALERT(at_console,"%s IDX=%d\n",STRING(pent->v.classname),ENTINDEX(pent));
	}
	CVAR_SET_FLOAT("developer",0);
}

void DebugShowEntInfo(int iNum)
{
	CVAR_SET_FLOAT("developer",1);
	ALERT(at_console,"\n");
	edict_t *pEdict = INDEXENT(iNum);
	if(!FNullEnt(pEdict))
	{
		ALERT(at_console,"classname = %s\n",STRING(pEdict->v.classname));
		ALERT(at_console,"movedir = (%f,%f,%f)\n",pEdict->v.movedir.x,pEdict->v.movedir.y,
			pEdict->v.movedir.z);
		ALERT(at_console,"angles = (%f,%f,%f)\n",pEdict->v.angles.x,pEdict->v.angles.y,
			pEdict->v.angles.z);
		ALERT(at_console,"endpos = (%f,%f,%f)\n",pEdict->v.endpos.x,pEdict->v.endpos.y,
			pEdict->v.endpos.z);
		ALERT(at_console,"startpos = (%f,%f,%f)\n",pEdict->v.startpos.x,pEdict->v.startpos.y,
			pEdict->v.startpos.z);
		ALERT(at_console,"model = %s\n",STRING(pEdict->v.model));
		ALERT(at_console,"health = %f\n",pEdict->v.health);
		ALERT(at_console,"takedamage = %f\n",pEdict->v.takedamage);
		ALERT(at_console,"spawnflags = %d\n",pEdict->v.spawnflags);
		ALERT(at_console,"flags = %d\n",pEdict->v.flags);
		ALERT(at_console,"target = %s",STRING(pEdict->v.target));
		edict_t *pTarget = NULL;
		pTarget = UTIL_FindEntityByTargetname(pTarget,STRING(pEdict->v.target));
		if(pTarget != NULL)
		{
			ALERT(at_console," - Target classname = %s\n",STRING(pTarget->v.classname));
		}
		else
			ALERT(at_console,"\n");
		ALERT(at_console,"targetname = %s\n",STRING(pEdict->v.targetname));
	}
	CVAR_SET_FLOAT("developer",0);
}

void DebugTraceEntities(edict_t *pEdict)
{
	CVAR_SET_FLOAT("developer",1);
	ALERT(at_console,"\n");
	TraceResult tr;

	UTIL_MakeVectors( pEdict->v.v_angle );
	Vector vecSource = pEdict->v.origin + pEdict->v.view_ofs;
	Vector vecDest = vecSource + gpGlobals->v_forward * 500.0;
	
	UTIL_TraceLine( vecSource, vecDest, dont_ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	if (tr.flFraction != 1.0)
		ALERT(at_console,"%s\n",STRING(tr.pHit->v.classname));
	else
		ALERT(at_console,"Nothing...\n");
	CVAR_SET_FLOAT("developer",0);
}



void DebugShowGrens(edict_t *pEdict)
{
	TraceResult tr;

	CVAR_SET_FLOAT("developer",1);
	ALERT(at_console,"\n");
	UTIL_MakeVectors( pEdict->v.v_angle );
	Vector vecSource = pEdict->v.origin + pEdict->v.view_ofs;
	Vector vecDest = vecSource + gpGlobals->v_forward * 1024.0;
	UTIL_TraceLine( vecSource, vecDest, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	if (tr.flFraction != 1.0)
	{
		Vector vecDir = gpGlobals->v_forward;
		float n = -DotProduct(tr.vecPlaneNormal, vecDir);
//		if (n < 0.5) // 60 degrees
			ALERT(at_console,"%f\n",n);
	}
	else
		ALERT(at_console,"Frei!\n");
}

void ShowPlayerView(edict_t *pPlayer)
{
//	DebugShowDot(pPlayer);
//	DebugShowAiming(pPlayer);
//	DebugShowGrens(pPlayer);
}

#endif		// DEBUG


STRINGNODE *GetNodeString(STRINGNODE* pNode,int NodeNum)
{
	STRINGNODE* pTempNode=pNode;
	int i=0;
	while(i<NodeNum)
	{
		pTempNode=pTempNode->Next;
		assert(pTempNode!=NULL);
		if(pTempNode==NULL)
			break;
		i++;
	}
	return pTempNode;
}

// Duplicate of the previous to prevent overlapping calls
STRINGNODE *GetNodeStringNames(STRINGNODE* pNode,int NodeNum)
{
	STRINGNODE* pTempNode=pNode;
	int i=0;
	while(i<NodeNum)
	{
		pTempNode=pTempNode->Next;
		assert(pTempNode!=NULL);
		if(pTempNode==NULL)
			break;
		i++;
	}
	return pTempNode;
}
