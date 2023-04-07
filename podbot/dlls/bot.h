//
// bot.h
//

#ifndef BOT_H
#define BOT_H

// stuff for Win32 vs. Linux builds

#ifndef __linux__

typedef int (FAR *GETENTITYAPI)(DLL_FUNCTIONS *, int);
typedef int (FAR *GETNEWDLLFUNCTIONS)(NEW_DLL_FUNCTIONS *, int *);
typedef void (DLLEXPORT *GIVEFNPTRSTODLL)(enginefuncs_t *, globalvars_t *);
typedef void (FAR *LINK_ENTITY_FUNC)(entvars_t *);

#ifdef _MSC_VER
#define EXPORT _declspec( dllexport )
#else
#define EXPORT __declspec( dllexport )
#endif

#else

#include <dlfcn.h>
#define GetProcAddress dlsym

typedef int BOOL;

typedef int (*GETENTITYAPI)(DLL_FUNCTIONS *, int);
typedef void (*GIVEFNPTRSTODLL)(enginefuncs_t *, globalvars_t *);
typedef void (*LINK_ENTITY_FUNC)(entvars_t *);

#define EXPORT

#endif

#include <assert.h>


typedef struct bottask_s
{
	bottask_s *pPreviousTask;
	bottask_s *pNextTask;
	int iTask;			// Major Task/Action carried out
	float fDesire;		// Desire (filled in) for this Task
	int iData;			// Additional Data (Waypoint Index)
	float fTime;		// Time Task expires
	bool bCanContinue;	// If Task can be continued if interrupted
} bottask_t;


// Task Filter functions

inline struct bottask_s* clampdesire(bottask_t* t1,float fMin,float fMax)
{
	if(t1->fDesire<fMin)
		t1->fDesire=fMin;
	else if(t1->fDesire>fMax)
		t1->fDesire=fMax;
	return t1;
}

inline struct bottask_s* maxdesire(bottask_t* t1,bottask_t *t2)
{
	if(t1->fDesire > t2->fDesire)
		return t1;
	else
		return t2;
}

inline bottask_s* subsumedesire(bottask_t* t1,bottask_t *t2)
{
	if(t1->fDesire > 0)
		return t1;
	else
		return t2;
}

inline bottask_s* thresholddesire(bottask_t* t1,float t, float d)
{
	if(t1->fDesire >= t)
		return t1;
	else
	{
		t1->fDesire=d;
		return t1;
	}
}
 
inline float hysteresisdesire(float x, float min, float max, float oldval)
{
	if(x <= min || x >= max)
		oldval=x;
	return oldval;
}


// Some more function prototypes
BOOL ClientConnect( edict_t *pEntity, const char *pszName,
                    const char *pszAddress, char szRejectReason[ 128 ] );
void ClientPutInServer( edict_t *pEntity );
void ClientCommand( edict_t *pEntity );
void ClientKill( edict_t *pEntity );

void FakeClientCommand(edict_t *pBot, char *arg1, char *arg2, char *arg3);


const char *Cmd_Args( void );
const char *Cmd_Argv( int argc );
int Cmd_Argc( void );


// define Console/CFG Commands
#define CONSOLE_CMD_ADDBOT			"addbot\0"
#define CONSOLE_CMD_ADDHUNT			"addhunt\0"
#define CONSOLE_CMD_KILLALLBOTS		"killbots\0"
#define CONSOLE_CMD_WAYPOINT		"waypoint\0"
#define CONSOLE_CMD_WAYZONE			"wayzone\0"
#define CONSOLE_CMD_AUTOWAYPOINT	"autowaypoint\0"
#define CONSOLE_CMD_PATHWAYPOINT	"pathwaypoint\0"
#define CONSOLE_CMD_REMOVEALLBOTS	"removebots\0"
#define CONSOLE_CMD_MINBOTS			"min_bots\0"
#define CONSOLE_CMD_MAXBOTS			"max_bots\0"
#define CONSOLE_CMD_MINBOTSKILL		"minbotskill\0"
#define CONSOLE_CMD_MAXBOTSKILL		"maxbotskill\0"
#define CONSOLE_CMD_PAUSE			"pause\0"
#define CONSOLE_CMD_BOTCHAT			"botchat\0"
#define CONSOLE_CMD_NEWMAP			"newmap\0"
#define CONSOLE_CMD_JASONMODE		"jasonmode\0"
#define CONSOLE_CMD_PODMENU			"podbotmenu\0"
#define CONSOLE_CMD_BORG			"imaborg\0"
#define CONSOLE_CMD_WPTFOLDER		"wptfolder\0"
#define CONSOLE_CMD_DETAILNAMES		"detailnames\0"
#define CONSOLE_CMD_INHUMANTURNS	"inhumanturns\0"
#define CONSOLE_CMD_MAXBOTSFOLLOW	"botsfollowuser\0"
#define CONSOLE_CMD_MAXWEAPONPICKUP	"maxweaponpickup\0"
#define CONSOLE_CMD_EXPERIENCE		"experience\0"
#define CONSOLE_CMD_COLLECTEXP		"collectexperience\0"
#define CONSOLE_CMD_SHOOTTHRU		"shootthruwalls\0"
#define CONSOLE_CMD_TIMESOUND		"timer_sound\0"
#define CONSOLE_CMD_TIMEPICKUP		"timer_pickup\0"
#define CONSOLE_CMD_TIMEGRENADE		"timer_grenade\0"
#define CONSOLE_CMD_SPEECH			"usespeech\0"
#define CONSOLE_CMD_VOTEKICK		"botvotekick\0"
#define CONSOLE_CMD_BOTVOTEMAP		"botsvotemap\0"
#define CONSOLE_CMD_FUN				"omg\0"
#define CONSOLE_CMD_ALLOWSPRAY		"botspray\0"


#define BOT_NAME_LEN 32	// Max Botname len

#define LADDER_UP    1
#define LADDER_DOWN  2

#define WANDER_LEFT  1
#define WANDER_RIGHT 2

#define BOT_YAW_SPEED 20
#define BOT_PITCH_SPEED  20  // degrees per frame for rotation

// Collision States
#define COLLISION_NOTDECIDED	0
#define COLLISION_PROBING		1
#define COLLISION_NOMOVE		2
#define COLLISION_JUMP			3
#define COLLISION_DUCK			4
#define COLLISION_STRAFELEFT	5
#define COLLISION_STRAFERIGHT	6

#define PROBE_JUMP		(1<<0)	// Probe Jump when colliding  
#define PROBE_DUCK		(1<<1)	// Probe Duck when colliding
#define PROBE_STRAFE	(1<<2)	// Probe Strafing when colliding


#define RESPAWN_IDLE             1
#define RESPAWN_NEED_TO_KICK     2
#define RESPAWN_NEED_TO_RESPAWN  3
#define RESPAWN_IS_RESPAWNING    4
#define RESPAWN_SERVER_KICKED    5


// game start messages for CS...
#define MSG_CS_IDLE         1
#define MSG_CS_TEAM_SELECT  2
#define MSG_CS_CT_SELECT    3
#define MSG_CS_T_SELECT     4
#define MSG_CS_MOTD			8

// Misc Message Queue Defines
#define MSG_CS_BUY						100
#define MSG_CS_BUYWEAPON_SELECT			101
#define MSG_CS_WEAPONBUY				102
#define MSG_CS_BUYAMMO					103
#define MSG_CS_BUYITEM					104
#define MSG_CS_BUYITEM_SELECT			105

#define MSG_CS_RADIO					200
#define MSG_CS_RADIO_SELECT				201

#define MSG_CS_SAY			10000
#define MSG_CS_TEAMSAY		10001


// Radio Messages
#define RADIO_COVERME		1
#define RADIO_YOUTAKEPOINT	2
#define RADIO_HOLDPOSITION	3
#define RADIO_REGROUPTEAM	4
#define RADIO_FOLLOWME		5
#define RADIO_TAKINGFIRE	6

#define RADIO_GOGOGO		11
#define RADIO_FALLBACK		12
#define RADIO_STICKTOGETHER	13
#define RADIO_GETINPOSITION	14
#define RADIO_STORMTHEFRONT	15
#define RADIO_REPORTTEAM	16

#define RADIO_AFFIRMATIVE	21
#define RADIO_ENEMYSPOTTED	22
#define RADIO_NEEDBACKUP	23
#define RADIO_SECTORCLEAR	24
#define RADIO_IMINPOSITION	25
#define RADIO_REPORTINGIN	26
#define RADIO_SHESGONNABLOW 27
#define RADIO_NEGATIVE		28
#define RADIO_ENEMYDOWN		29


// Sensing States
#define STATE_SEEINGENEMY		(1<<0)	// Seeing an Enemy  
#define STATE_HEARINGENEMY		(1<<1)	// Hearing an Enemy
#define STATE_PICKUPITEM		(1<<2)	// Pickup Item Nearby
#define STATE_THROWHEGREN		(1<<3)	// Could throw HE Grenade
#define STATE_THROWFLASHBANG	(1<<4)	// Could throw Flashbang
#define STATE_THROWSMOKEGREN	(1<<5)	// Could throw SmokeGrenade
#define STATE_SUSPECTENEMY		(1<<6)	// Suspect Enemy behind Obstacle 

// Positions to aim at
#define AIM_DEST				(1<<0)	// Aim at Nav Point
#define AIM_CAMP				(1<<1)	// Aim at Camp Vector
#define AIM_PREDICTPATH			(1<<2)	// Aim at predicted Path  
#define AIM_LASTENEMY			(1<<3)	// Aim at Last Enemy
#define AIM_ENTITY				(1<<4)	// Aim at Entity like Buttons,Hostages
#define AIM_ENEMY				(1<<5)	// Aim at Enemy
#define AIM_GRENADE				(1<<6)	// Aim for Grenade Throw
#define AIM_OVERRIDE			(1<<7)	// Overrides all others (blinded)

// Tasks to do
#define TASK_NONE				-1

#define TASK_NORMAL				0
#define TASK_PAUSE				1
#define TASK_MOVETOPOSITION		2
#define TASK_FOLLOWUSER			3
#define TASK_WAITFORGO			4
#define TASK_PICKUPITEM			5
#define TASK_CAMP				6
#define TASK_PLANTBOMB			7
#define TASK_DEFUSEBOMB			8
#define TASK_ATTACK				9
#define TASK_ENEMYHUNT			10
#define TASK_SEEKCOVER			11
#define TASK_THROWHEGRENADE		12
#define TASK_THROWFLASHBANG		13
#define TASK_THROWSMOKEGRENADE	14
#define TASK_SHOOTBREAKABLE		15
#define TASK_HIDE				16
#define TASK_BLINDED			17
#define TASK_SPRAYLOGO			18


// Some hardcoded Desire Defines used to override
// calculated ones
#define TASKPRI_NORMAL			35.0
#define TASKPRI_PAUSE			36.0
#define TASKPRI_CAMP			37.0
#define TASKPRI_SPRAYLOGO		38.0
#define TASKPRI_FOLLOWUSER		39.0
#define TASKPRI_MOVETOPOSITION	50.0
#define TASKPRI_DEFUSEBOMB		89.0
#define TASKPRI_PLANTBOMB		89.0
#define TASKPRI_ATTACK			90.0
#define TASKPRI_SEEKCOVER		91.0
#define TASKPRI_HIDE			92.0
#define TASKPRI_THROWGRENADE	99.0
#define TASKPRI_BLINDED			100.0
#define TASKPRI_SHOOTBREAKABLE	100.0



// Defines for User Menues
#define MENU_WAYPOINT_ADD			1
#define MENU_WAYPOINT_ADD2			2
#define MENU_WAYPOINT_ADDFLAG		3
#define MENU_WAYPOINT_DELETEFLAG	4
#define MENU_PODMAIN				5
#define MENU_SKILLSELECT			6
#define MENU_TEAMSELECT				7
#define MENU_SERVERTEAMSELECT		8
#define MENU_MODEL_SELECT			9
#define MENU_WEAPONMODESELECT		10

// Defines for Pickup Items
#define PICKUP_NONE				0
#define PICKUP_WEAPON			1
#define PICKUP_DROPPED_C4		2
#define PICKUP_PLANTED_C4		3
#define PICKUP_HOSTAGE			4
#define PICKUP_BUTTON			5

// Enemy Body Parts Seen
#define HEAD_VISIBLE		(1<<0)  
#define WAIST_VISIBLE		(1<<1)
#define CUSTOM_VISIBLE		(1<<2)


#define MAX_HOSTAGES	8
#define MAX_DAMAGE_VAL	2040
#define MAX_GOAL_VAL	2040
#define TIME_GRENPRIME	1.2
#define MAX_KILL_HIST	16

const float BOMBMAXHEARDISTANCE = 2048.0;


// This Structure links Waypoints returned from Pathfinder
typedef struct pathnode
{
	int  id;
	int iIndex;
	int  depth;
	int  state;   // {OPEN, NEW, CLOSED}
	double g;
	double h;
	double f;
	pathnode *parent;
	pathnode *NextNode;
	pathnode *prev;
} PATHNODE;


// This Structure holds String Messages
typedef struct stringnode
{
	char *pszString;
	struct stringnode *Next;
} STRINGNODE;


// Links Keywords and replies together
typedef struct replynode
{
	char pszKeywords[256];
	struct replynode *pNextReplyNode;
	char	cNumReplies;
	char	cLastReply;
	struct stringnode *pReplies;
} replynode_t;


typedef struct
{
   int  iId;     // weapon ID
   int  iClip;   // amount of ammo in the clip
   int  iAmmo1;  // amount of ammo in primary reserve
   int  iAmmo2;  // amount of ammo in secondary reserve
} bot_current_weapon_t;


typedef struct
{
   int iId;  // the weapon ID value
   char  weapon_name[64];  // name of the weapon when selecting it
   char  model_name[64];	// Model Name to separate CS Weapons
   float primary_min_distance;   // 0 = no minimum
   float primary_max_distance;   // 9999 = no maximum
   float secondary_min_distance; // 0 = no minimum
   float secondary_max_distance; // 9999 = no maximum
   bool  can_use_underwater;     // can use this weapon underwater
   bool  primary_fire_hold;      // hold down primary fire button to use?
   float primary_charge_delay;   // time to charge weapon
   int   iPrice;				 // Price when buying
   int	 min_primary_ammo;
   int   iTeamStandard;			 // Used by Team (Number) (standard map)
   int   iTeamAS;				 // Used by Team (AS map)
   int   iBuyGroup;				 // Group in Buy Menu (standard map)
   int   iBuySelect;			 // Select Item in Buy Menu (standard map)
   bool  bShootsThru;			 // Can shoot thru Walls 
} bot_weapon_select_t;


typedef struct
{
	float	fMinSurpriseDelay;	// time in secs
	float	fMaxSurpriseDelay;	// time in secs
	int		iPauseProbality;	// % Probability to pause after reaching a waypoint	
	float	fBotCampStartDelay;	// time in secs
	float	fBotCampEndDelay;	// time in secs
} skilldelay_t;



typedef struct
{
	float fAim_X;				// X Max Offset
	float fAim_Y;				// Y Max Offset
	float fAim_Z;				// Z Max Offset
	int iHeadShot_Frequency;	// % to aim at Haed
	int iHeardShootThruProb;	// % to shoot thru Wall when heard  
	int iSeenShootThruProb;		// % to shoot thru Wall when seen 
} botaim_t;


// Struct for Menus
typedef struct
{
	int ValidSlots;		// Ored together Bits for valid Keys
	char *szMenuText;	// Ptr to actual String
} menutext_t; 



// Not needed anymore since using ClientSetMaxspeed
typedef struct
{
	int iId;
	float fSpeed;
} weapon_movespeed_t;


// Records some Player Stats each Frame and holds sound events playing
typedef struct
{
	bool IsUsed;				// Player used in the Game
	bool IsAlive;				// Alive or Dead
	edict_t *pEdict;			// Ptr to actual Edict
	int iTeam;					// What Team
	Vector vOrigin;				// Position in the world
	Vector vecSoundPosition;	// Position Sound was played
	float fHearingDistance;		// Distance this Sound is heared 
	float fTimeSoundLasting;	// Time sound is played/heared
} threat_t; 


typedef struct
{
	edict_t *pEdict;			// ptr to Edict heard
	float distance;				// distance to Player
} heartab_t;

// Experience Data hold in memory while playing
typedef struct
{
	unsigned short uTeam0Damage;		// Amount of Damage
	unsigned short uTeam1Damage;		// "
	signed short iTeam0_danger_index;	// Index of Waypoint
	signed short iTeam1_danger_index;	// "
	signed short wTeam0Value;			// Goal Value
	signed short wTeam1Value;			// "
} experience_t;

// Experience Data when saving/loading
typedef struct
{
	unsigned char uTeam0Damage;
	unsigned char uTeam1Damage;
	signed char cTeam0Value;
	signed char cTeam1Value;
} experiencesave_t;


// Array to hold Params for creation of a Bot 
typedef struct
{
	bool bNeedsCreation;
	edict_t *pCallingEnt;
	char name[BOT_NAME_LEN+1];
	char skill[5];
	char team[5];
	char model[5];
} createbot_t;

// Array to hold Clients for welcoming
typedef struct
{
	bool bNeedWelcome;
	float welcome_time;
	edict_t *pEntity;
} client_t;


typedef struct
{
	char  cChatProbability;
	float fChatDelay;
	float fTimeNextChat;
	int iEntityIndex;
	char szSayText[512];
} saytext_t;


// Main Bot Structure	
typedef struct
{
	bool is_used;				// Bot used in the Game
	int respawn_state;			// Needs respawn of what type
	edict_t *pEdict;			// ptr to actual Player edict
	bool need_to_initialize;	// needs to be initialized
	int not_started;			// team/class not chosen yet

#ifdef DEBUG
	bool bDebug;				// if true break with assert
	int iTask;
#endif

	int start_action;			// Team/Class selection state
	bool bDead;					// dead or alive
//	bool bInMessageLoop;
	char name[BOT_NAME_LEN+1];	// botname
	int bot_skill;				// skill

	// TheFatal - START
	int msecnum;
	float msecdel;
	float msecval;
	// TheFatal - END

	float fBaseAgressionLevel;	// Base Levels used when initializing
	float fBaseFearLevel;
	float fAgressionLevel;		// Dynamic Levels used ingame
	float fFearLevel;
	float fNextEmotionUpdate;	// Next time to sanitize emotions

	float oldcombatdesire;		// holds old desire for filtering

	unsigned char bot_personality;	// Personality 0-2
	int iSprayLogo;				// Index of Logo to use

	
	unsigned int iStates;		// Sensing BitStates
	bottask_t *pTasks;			// Ptr to active Tasks/Schedules


	float fTimePrevThink;		// Last time BotThink was called
	float fTimeFrameInterval;	// Interval Time between BotThink calls
	
	// things from pev in CBasePlayer...
	int bot_team;
	int bot_class;
	int bot_health;
	int bot_armor;
// bit map of weapons the bot is carrying
// not needed for CS
//	int bot_weapons;
	int bot_money;			// money in Counter-Strike

	bool bIsVIP;			// Bot is VIP
	bool bIsLeader;			// Bot is leader of his Team;
	float fTimeTeamOrder;	// Time of Last Radio command
	bool IsDefendingTeam;	// Bot in defending Team on this map
	
	float f_max_speed;		// Max Speed with this Weapon
	float f_move_speed;		// Current Speed forward/backward 
	float f_sidemove_speed;	// Current Speed sideways
	float fMinSpeed;		// Minimum Speed in Normal Mode
	bool bCheckTerrain;

	bool bOnLadder;
	bool bInWater;
	bool bNearDoor;
	bool bCheckMyTeam;
	int iTeam;

	float prev_time;		// Time previously checked movement speed
	float prev_speed;		// Speed some frames before 
	Vector v_prev_origin;	// Origin "   "      "


	bool bCanJump;			// Bot can jump over obstacle
	bool bCanDuck;			// Bot can duck under obstacle

	float f_view_distance;		// Current View distance
	float f_maxview_distance;	// Maximum View distance
	
	int iActMessageIndex;		// Current processed Message
	int iPushMessageIndex;		// Offset for next pushed Message
	
	int aMessageQueue[32];		// Stack for Messages
	char szMiscStrings[160];	// Space for Strings (SayText...)	
	int iRadioSelect;			// Radio Entry

	// Holds the Index & the actual Message of the last
	// unprocessed Text Message of a Player
	saytext_t SaytextBuffer;

	float f_itemcheck_time;		// Time next Search for Items needs to be done
	edict_t *pBotPickupItem;	// Ptr to Entity of Item to use/pickup
	edict_t *pItemIgnore;		// Ptr to Entity to ignore for pickup
	int iPickupType;			// Type of Entity which needs to be used/picked up

	edict_t *pShootBreakable;	// Ptr to Breakable Entity
	Vector vecBreakable;		// Origin of Breakable

	float f_use_button_time;	// Time to next Button Use Check

	int iNumWeaponPickups;		// Counter of Pickups done
	bool bDroppedWeapon;
	
	float f_spawn_time;			// Time this Bot spawned
	float f_kick_time;			// "    "    "   was kicked
	
	float f_lastchattime;		// Time Bot last chatted

	bool bLogoSprayed;			// Logo sprayed this round
	bool bDefendedBomb;			// Defend Action issued

	int ladder_dir;				// Direction for Ladder movement
	float f_use_ladder_time;	// Last Time Bot used Ladder
	Vector ladder_targetpos; 
	
	// Not used at the moment
	float f_wall_check_time;
	float f_wall_on_right;
	float f_wall_on_left;
	float f_dont_avoid_wall_time;
	// end not used
	
	float f_collide_time;		// Time last collision
	float f_firstcollide_time;	// Time of first collision
	float f_probe_time;			// Time of probing different moves
	float fNoCollTime;			// Time until next collision check 
	float f_CollisionSidemove;	// Amount to move sideways
	char  cCollisionState;		// Collision State 
	char  cCollisionProbeBits;	// Bits of possible Collision Moves
	char cCollideMoves[4];		// Sorted Array of Movements
	char cCollStateIndex;		// Index into cCollideMoves

	Vector wpt_origin;			// Origin of Waypoint
	Vector dest_origin;			// Origin of move destination
	Vector vecLadderOrigin;		// Origin of last Ladder Entity
	Vector vecDirWaypoint;		// Direction to next Waypoint
	
	PATHNODE* pWaypointNodes;	// Ptr to current Node from Path
	PATHNODE* pWayNodesStart;	// Ptr to start of Pathfinding Nodes
	unsigned char byPathType;	// Which Pathfinder to use
	int prev_goal_index;		// Holds destination Goal wpt
	int chosengoal_index;		// Used for experience, same as above
	float f_goal_value;			// Ranking Value for this waypoint
	int curr_wpt_index;			// Current wpt index
	int prev_wpt_index[5];		// Previous wpt indices from waypointfind 
	int iWPTFlags;
	unsigned short curr_travel_flags;	// Connection Flags like jumping
	unsigned short curr_move_flags;	// Move Flags (copy of travel flags)
	Vector vecDesiredVelocity;	// Desired Velocity for jump waypoints
	float f_wpt_timeset;		// Time waypoint chosen by Bot
	
	edict_t *pBotEnemy;			// ptr to Enemy Entity
	float fEnemyUpdateTime;		// Time to check for new enemies
	float fEnemyReachableTimer;	// Time to recheck if Enemy reachable
	bool bEnemyReachable;		// Direct Line to Enemy

	float f_bot_see_enemy_time;		// Time Bot sees Enemy
	float f_enemy_surprise_time;	// Time of surprise
	float f_ideal_reaction_time;	// Time of base reaction
	float f_actual_reaction_time;	// Time of current reaction time

	float rgfYawHistory[2];
	float rgfPitchHistory[2];


	edict_t *pLastEnemy;		// ptr to last Enemy Entity
	Vector vecLastEnemyOrigin;	// Origin
	Vector vecEnemyVelocity;	// Velocity of Enemy 1 Frame before
	unsigned char ucVisibility;	// Which Parts are visible
	edict_t *pLastVictim;		// ptr to killed Entity
	edict_t	*pTrackingEdict;	// ptr to last tracked Player when camping/hiding
	float fTimeNextTracking;	// Time Waypoint Index for tracking Player is recalculated

	unsigned int iAimFlags;		// Aiming Conditions
	Vector vecLookAt;			// Vector Bot should look at
	Vector vecThrow;			// Origin of Waypoint to Throw Grens

	Vector vecEnemy;			// Target Origin chosen for shooting
	Vector vecGrenade;			// Calculated Vector for Grenades
	Vector vecEntity;			// Origin of Entities like Buttons etc.
	Vector vecCamp;				// Aiming Vector when camping.

	float fSideSpeed;			// Side Speed from last MoveToGoal Movement
	float fForwardSpeed;		// Forward Speed from last MoveToGoal Movement
	float fTimeWaypointMove;	// Last Time Bot followed Waypoints
//	float fTimeNextYawUpdate;

	bool bWantsToFire;			// Bot needs consider firing
	float f_shootatdead_time;	// Time to shoot at dying players
	edict_t *pAvoidGrenade;		// ptr to Grenade Entity to avoid
	char cAvoidGrenade;			// which direction to strafe away

	int iVoteKickIndex;			// Index of Player to vote against
	int iLastVoteKick;			// Last Index
	int iVoteMap;				// Number of map to vote for

	Vector vecPosition;			// Position to Move To (TASK_MOVETOPOSITION)
	

	edict_t *pBotUser;			// ptr to User Entity (using a Bot)
	float f_bot_use_time;		// Time last seen User

	edict_t *pRadioEntity;		// ptr to Entity issuing a Radio Command
	int iRadioOrder;			// actual Command

	edict_t *pHostages[MAX_HOSTAGES];	// ptr to used Hostage Entities

	float f_hostage_check_time;	// Next time to check if Hostage should be used
								// (Terrorists)

	bool  bIsReloading;			// Bot is reloading a gun

	float f_zoomchecktime;		// Time to check Zoom again

	int	  iBurstShotsFired;		// Shots fired in 1 interval
	float fTimeLastFired;		
	float fTimeFirePause;
	float f_shoot_time;

	bool  bCheckWeaponSwitch;
	float fTimeWeaponSwitch;

	int   charging_weapon_id;
	float f_primary_charging;
	float f_secondary_charging;
	// end not used
	
	float f_grenade_check_time;	// Time to check Grenade Usage
	bool bUsingGrenade;


	unsigned char byCombatStrafeDir;	// Direction to strafe
	unsigned char byFightStyle;			// Combat Style to use
	float f_lastfightstylecheck;		// Time checked style
	float f_StrafeSetTime;				// Time strafe direction was set

	float f_blind_time;				// Time Bot is blind
	float f_blindmovespeed_forward;	// Mad speeds when Bot is blind
	float f_blindmovespeed_side;

	int iLastDamageType;			// Stores Last Damage
	
	float f_sound_update_time;		// Time next sound next soundcheck
	float f_heard_sound_time;		// Time enemy has been heard
	float fTimeCamping;				// Time to Camp
	int  iCampDirection;			// Camp Facing direction
	int  iCampButtons;				// Buttons to press while camping

	float f_jumptime;				// Time last jump happened
	bool  bUseDuckJump;				// Use duck jump (not used)

	float f_bomb_time;				// Time to hold Button for planting

	bool  b_lift_moving;			// not currently used
	
	int iBuyCount;					// Current Count in Buying
	int BuyAction;					// Current Buy Action
	bool bInBuyzone;				// not used now
	bool bBuyingFinished;			// Done with buying
	
	bot_weapon_select_t *pSelected_weapon; // Weapon chosen to be selected
	bot_current_weapon_t current_weapon;  // one current weapon for each bot
	int m_rgAmmo[MAX_AMMO_SLOTS];  // total ammo amounts (1 array for each bot)

} bot_t;


extern createbot_t BotCreateTab[32];
extern experience_t* pBotExperienceData;
extern botaim_t BotAimTab[6];
// ptr to Botnames are stored here
extern STRINGNODE *pBotNames;
// ptr to already used Names
extern STRINGNODE *pUsedBotNames[8];
// ptr to Kill Messages go here
extern STRINGNODE *pKillChat;
// ptr to BombPlant Messages go here
extern STRINGNODE *pBombChat;
// ptr to Deadchats go here
extern STRINGNODE *pDeadChat;
extern STRINGNODE *pUsedDeadStrings[8];
// ptr to Strings when no keyword was found
extern STRINGNODE *pChatNoKeyword;
// ptr to Keywords & Replies for interactive Chat
extern replynode_t *pChatReplies;


// bot.cpp functions...
void ShowPlayerView(edict_t *pPlayer);
void BotCreate( edict_t *pPlayer, const char *arg1, const char *arg2, const char *arg3, const char *arg4);
void CreateBorgBot(edict_t *pPlayer);
void UpdateGlobalExperienceData();
void BotCollectGoalExperience(bot_t *pBot,int iDamage, int iTeam);
void BotCollectExperienceData(edict_t *pVictimEdict,edict_t *pAttackerEdict, int iDamage);
void BotPushTask(bot_t *pBot,bottask_t *pTask);
void BotTaskComplete(bot_t *pBot);
void BotResetTasks(bot_t *pBot);
bottask_t *BotGetSafeTask(bot_t *pBot);
void BotRemoveCertainTask(bot_t *pBot,int iTaskNum);
void TurnOffFunStuff(edict_t *pPlayer);
void BotThink(bot_t *pBot);
bool BotEnemyIsThreat(bot_t *pBot);
bool BotReactOnEnemy(bot_t *pBot);
void BotResetCollideState(bot_t *pBot);
void UserRemoveAllBots(edict_t *pPlayer);
void UserKillAllBots(edict_t *pPlayer);
void UserListBots(edict_t *pPlayer);
void UserNewroundAll(edict_t *pEntity);
void UserSelectWeaponMode(edict_t *pEntity,int iSelection);
bool BotCheckWallOnLeft( bot_t *pBot );
bool BotCheckWallOnRight( bot_t *pBot );
bool BotHasHostage(bot_t *pBot);
int GetBestWeaponCarried(bot_t *pBot);
Vector GetBombPosition();
int BotFindDefendWaypoint(bot_t *pBot,Vector vecPosition);
bool BotFindWaypoint( bot_t *pBot );
bool BotEntityIsVisible( bot_t *pBot, Vector dest );
int BotGetMessageQueue(bot_t *pBot);
void BotPushMessageQueue(bot_t *pBot,int iMessage);
void BotPlayRadioMessage(bot_t *pBot,int iMessage);
bool IsDeadlyDrop(bot_t *pBot,Vector vecTargetPos);
bool IsInWater( edict_t *pEdict);
bool IsOnLadder( edict_t *pEdict );
void BotFreeAllMemory();

#ifdef _DEBUG
void DebugShowBeam(Vector vecStartpos,Vector vecEndpos,int r,int g,int b);
void DebugShowEntities(edict_t *pEdict);
void DebugShowEntInfo(int iNum);
void DebugTraceEntities(edict_t *pEdict);
#endif

STRINGNODE *GetNodeString(STRINGNODE* pNode,int NodeNum);
STRINGNODE *GetNodeStringNames(STRINGNODE* pNode,int NodeNum);

void TestDecal(edict_t *pEdict,char* pszDecalName);

// bot_combat.cpp functions...
int NumTeammatesNearPos(bot_t *pBot,Vector vecPosition,int iRadius);
int NumEnemiesNearPos(bot_t *pBot,Vector vecPosition,int iRadius);
bool BotFindEnemy( bot_t *pBot );
bool IsShootableThruObstacle(edict_t *pEdict,Vector vecDest);
bool BotFireWeapon( Vector v_enemy, bot_t *pBot);
void BotFocusEnemy( bot_t *pBot );
void BotDoAttackMovement( bot_t *pBot);
bool BotHasPrimaryWeapon(bot_t *pBot);
bool WeaponShootsThru(int iId);
bool BotUsesRifle(bot_t *pBot);
bool BotUsesSniper(bot_t *pBot);
int BotCheckGrenades(bot_t *pBot);
int HighestWeaponOfEdict(edict_t *pEdict);
void BotSelectBestWeapon(bot_t *pBot);
void SelectWeaponByName(bot_t *pBot,char* pszName);
void SelectWeaponbyNumber(bot_t *pBot,int iNum);
void BotCommandTeam(bot_t *pBot);
bool IsGroupOfEnemies(bot_t *pBot,Vector vLocation);
Vector VecCheckToss ( edict_t *pEdict, const Vector &vecSpot1, Vector vecSpot2);
Vector VecCheckThrow ( edict_t *pEdict, const Vector &vecSpot1, Vector vecSpot2, float flSpeed);

// new UTIL.CPP functions...
edict_t *UTIL_FindEntityInSphere( edict_t *pentStart, const Vector &vecCenter, float flRadius );
edict_t *UTIL_FindEntityByClassname( edict_t *pentStart, const char *szName );
edict_t *UTIL_FindEntityByTargetname( edict_t *pentStart, const char *szName );
short FixedSigned16( float value, float scale );
void UTIL_SayText( const char *pText,bot_t *pBot);
void UTIL_TeamSayText( const char *pText,bot_t *pBot);
void UTIL_SpeechSynth(edict_t *pEdict,char* szMessage);
int UTIL_GetTeam(edict_t *pEntity);
int UTIL_GetBotIndex(edict_t *pEdict);
bot_t *UTIL_GetBotPointer(edict_t *pEdict);
bool IsAlive(edict_t *pEdict);
bool FInViewCone(Vector *pOrigin, edict_t *pEdict);
float GetShootingConeDeviation(edict_t *pEdict,Vector *pvecPosition);
bool IsShootableBreakable(edict_t *pent);
bool FBoxVisible (edict_t *pEdict, edict_t *pTargetEdict,Vector* pvHit, unsigned char* ucBodyPart);
bool FVisible( const Vector &vecOrigin, edict_t *pEdict );
Vector Center(edict_t *pEdict);
Vector GetGunPosition(edict_t *pEdict);
void UTIL_SelectItem(edict_t *pEdict, char *item_name);
Vector VecBModelOrigin(edict_t *pEdict);
void UTIL_ShowMenu( edict_t *pEdict, int slots, int displaytime, bool needmore, char *pText );
edict_t *UTIL_FindEntityByTarget( edict_t *pentStart, const char *szName );
void UTIL_DecalTrace( TraceResult *pTrace, char* pszDecalName );
int UTIL_GetNearestPlayerIndex(Vector vecOrigin);
void UTIL_HostPrint(int msg_dest,char *pszMessage);
void UTIL_DebugMSG(const char *fmt, ...);
void UTIL_ClampAngle(float* fAngle);
void UTIL_ClampVector(Vector *vecAngles);


// engine prototypes (from engine\eiface.h)...
int pfnPrecacheModel( char* s );
int pfnPrecacheSound( char* s );
void pfnSetModel( edict_t *e, const char *m );
int pfnModelIndex( const char *m );
int pfnModelFrames( int modelIndex );
void pfnSetSize( edict_t *e, const float *rgflMin, const float *rgflMax );
void pfnChangeLevel( char* s1, char* s2 );
void pfnGetSpawnParms( edict_t *ent );
void pfnSaveSpawnParms( edict_t *ent );
float pfnVecToYaw( const float *rgflVector );
void pfnVecToAngles( const float *rgflVectorIn, float *rgflVectorOut );
void pfnMoveToOrigin( edict_t *ent, const float *pflGoal, float dist, int iMoveType );
void pfnChangeYaw( edict_t* ent );
void pfnChangePitch( edict_t* ent );
edict_t* pfnFindEntityByString( edict_t *pEdictStartSearchAfter, const char *pszField, const char *pszValue );
int pfnGetEntityIllum( edict_t* pEnt );
edict_t* pfnFindEntityInSphere( edict_t *pEdictStartSearchAfter, const float *org, float rad );
edict_t* pfnFindClientInPVS( edict_t *pEdict );
edict_t* pfnEntitiesInPVS( edict_t *pplayer );
void pfnMakeVectors( const float *rgflVector );
void pfnAngleVectors( const float *rgflVector, float *forward, float *right, float *up );
edict_t* pfnCreateEntity( void );
void pfnRemoveEntity( edict_t* e );
edict_t* pfnCreateNamedEntity( int className );
void pfnMakeStatic( edict_t *ent );
int pfnEntIsOnFloor( edict_t *e );
int pfnDropToFloor( edict_t* e );
int pfnWalkMove( edict_t *ent, float yaw, float dist, int iMode );
void pfnSetOrigin( edict_t *e, const float *rgflOrigin );
void pfnEmitSound( edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch );
void pfnEmitAmbientSound( edict_t *entity, float *pos, const char *samp, float vol, float attenuation, int fFlags, int pitch );
void pfnTraceLine( const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr );
void pfnTraceToss( edict_t* pent, edict_t* pentToIgnore, TraceResult *ptr );
int pfnTraceMonsterHull( edict_t *pEdict, const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr );
void pfnTraceHull( const float *v1, const float *v2, int fNoMonsters, int hullNumber, edict_t *pentToSkip, TraceResult *ptr );
void pfnTraceModel( const float *v1, const float *v2, int hullNumber, edict_t *pent, TraceResult *ptr );
const char *pfnTraceTexture( edict_t *pTextureEntity, const float *v1, const float *v2 );
void pfnTraceSphere( const float *v1, const float *v2, int fNoMonsters, float radius, edict_t *pentToSkip, TraceResult *ptr );
void pfnGetAimVector( edict_t* ent, float speed, float *rgflReturn );
void pfnServerCommand( char* str );
void pfnServerExecute( void );
void pfnClientCommand( edict_t* pEdict, char* szFmt, ... );
void pfnParticleEffect( const float *org, const float *dir, float color, float count );
void pfnLightStyle( int style, char* val );
int pfnDecalIndex( const char *name );
int pfnPointContents( const float *rgflVector );
void pfnMessageBegin( int msg_dest, int msg_type, const float *pOrigin, edict_t *ed );
void pfnMessageEnd( void );
void pfnWriteByte( int iValue );
void pfnWriteChar( int iValue );
void pfnWriteShort( int iValue );
void pfnWriteLong( int iValue );
void pfnWriteAngle( float flValue );
void pfnWriteCoord( float flValue );
void pfnWriteString( const char *sz );
void pfnWriteEntity( int iValue );
void pfnCVarRegister( cvar_t *pCvar );
float pfnCVarGetFloat( const char *szVarName );
const char* pfnCVarGetString( const char *szVarName );
void pfnCVarSetFloat( const char *szVarName, float flValue );
void pfnCVarSetString( const char *szVarName, const char *szValue );
void pfnAlertMessage( ALERT_TYPE atype, char *szFmt, ... );
void pfnEngineFprintf( FILE *pfile, char *szFmt, ... );
void* pfnPvAllocEntPrivateData( edict_t *pEdict, long cb );
void* pfnPvEntPrivateData( edict_t *pEdict );
void pfnFreeEntPrivateData( edict_t *pEdict );
const char* pfnSzFromIndex( int iString );
int pfnAllocString( const char *szValue );
struct entvars_s* pfnGetVarsOfEnt( edict_t *pEdict );
edict_t* pfnPEntityOfEntOffset( int iEntOffset );
int pfnEntOffsetOfPEntity( const edict_t *pEdict );
int pfnIndexOfEdict( const edict_t *pEdict );
edict_t* pfnPEntityOfEntIndex( int iEntIndex );
edict_t* pfnFindEntityByVars( struct entvars_s* pvars );
void* pfnGetModelPtr( edict_t* pEdict );
int pfnRegUserMsg( const char *pszName, int iSize );
void pfnAnimationAutomove( const edict_t* pEdict, float flTime );
void pfnGetBonePosition( const edict_t* pEdict, int iBone, float *rgflOrigin, float *rgflAngles );
unsigned long pfnFunctionFromName( const char *pName );
const char *pfnNameForFunction( unsigned long function );
void pfnClientPrintf( edict_t* pEdict, PRINT_TYPE ptype, const char *szMsg );
void pfnServerPrint( const char *szMsg );
const char *pfnCmd_Args( void );
const char *pfnCmd_Argv( int argc );
int pfnCmd_Argc( void );
void pfnGetAttachment( const edict_t *pEdict, int iAttachment, float *rgflOrigin, float *rgflAngles );
void pfnCRC32_Init( CRC32_t *pulCRC );
void pfnCRC32_ProcessBuffer( CRC32_t *pulCRC, void *p, int len );
void pfnCRC32_ProcessByte( CRC32_t *pulCRC, unsigned char ch );
CRC32_t pfnCRC32_Final( CRC32_t pulCRC );
long pfnRandomLong( long lLow, long lHigh );
float pfnRandomFloat( float flLow, float flHigh );
void pfnSetView( const edict_t *pClient, const edict_t *pViewent );
float pfnTime( void );
void pfnCrosshairAngle( const edict_t *pClient, float pitch, float yaw );
byte * pfnLoadFileForMe( char *filename, int *pLength );
void pfnFreeFile( void *buffer );
void pfnEndSection( const char *pszSectionName );
int pfnCompareFileTime( char *filename1, char *filename2, int *iCompare );
void pfnGetGameDir( char *szGetGameDir );
void pfnCvar_RegisterVariable( cvar_t *variable );
void pfnFadeClientVolume( const edict_t *pEdict, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds );
void pfnSetClientMaxspeed( const edict_t *pEdict, float fNewMaxspeed );
edict_t * pfnCreateFakeClient( const char *netname );
void pfnRunPlayerMove( edict_t *fakeclient, const float *viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, byte impulse, byte msec );
int pfnNumberOfEntities( void );
char* pfnGetInfoKeyBuffer( edict_t *e );
char* pfnInfoKeyValue( char *infobuffer, char *key );
void pfnSetKeyValue( char *infobuffer, char *key, char *value );
void pfnSetClientKeyValue( int clientIndex, char *infobuffer, char *key, char *value );
int pfnIsMapValid( char *filename );
void pfnStaticDecal( const float *origin, int decalIndex, int entityIndex, int modelIndex );
int pfnPrecacheGeneric( char* s );
int pfnGetPlayerUserId( edict_t *e );
void pfnBuildSoundMsg( edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch, int msg_dest, int msg_type, const float *pOrigin, edict_t *ed );
int pfnIsDedicatedServer( void );
cvar_t *pfnCVarGetPointer( const char *szVarName );
unsigned int pfnGetPlayerWONId( edict_t *e );

void pfnInfo_RemoveKey( char *s, const char *key );
const char *pfnGetPhysicsKeyValue( const edict_t *pClient, const char *key );
void pfnSetPhysicsKeyValue( const edict_t *pClient, const char *key, const char *value );
const char *pfnGetPhysicsInfoString( const edict_t *pClient );
unsigned short pfnPrecacheEvent( int type, const char*psz );
void pfnPlaybackEvent( int flags, const edict_t *pInvoker, unsigned short eventindex, float delay, float *origin, float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 );
unsigned char *pfnSetFatPVS( float *org );
unsigned char *pfnSetFatPAS( float *org );
int pfnCheckVisibility ( const edict_t *entity, unsigned char *pset );
void pfnDeltaSetField( struct delta_s *pFields, const char *fieldname );
void pfnDeltaUnsetField( struct delta_s *pFields, const char *fieldname );
void pfnDeltaAddEncoder( char *name, void (*conditionalencode)( struct delta_s *pFields, const unsigned char *from, const unsigned char *to ) );
int pfnGetCurrentPlayer( void );
int pfnCanSkipPlayer( const edict_t *player );
int pfnDeltaFindField( struct delta_s *pFields, const char *fieldname );
void pfnDeltaSetFieldByIndex( struct delta_s *pFields, int fieldNumber );
void pfnDeltaUnsetFieldByIndex( struct delta_s *pFields, int fieldNumber );
void pfnSetGroupMask( int mask, int op );
int pfnCreateInstancedBaseline( int classname, struct entity_state_s *baseline );
void pfnCvar_DirectSet( struct cvar_s *var, char *value );
void pfnForceUnmodified( FORCE_TYPE type, float *mins, float *maxs, const char *filename );
void pfnGetPlayerStats( const edict_t *pClient, int *ping, int *packet_loss );
// new for 1.1.0.4.
void pfnAddServerCommand( char *cmd_name, void (*function) (void) );

#endif // BOT_H