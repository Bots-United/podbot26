//
// bot_globals.h
//

#ifndef BOT_GLOBALSH
#define BOT_GLOBALSH

#define NUM_WEAPONS 23
#define MAX_WAYPOINTS 1024
#define MAXBOTSNUMBER	32
#define MAXNUMBOMBSPOTS	16	
#define NUM_SPRAYPAINTS	16


extern int g_iMapType;
extern bool g_bWaypointOn;
extern bool g_bWaypointsChanged;	// Waypoints changed
extern bool g_bAutoWaypoint;
extern bool g_bPathWaypoint;
extern bool g_bDangerDirection;
extern bool g_bLearnJumpWaypoint;
extern int g_iFindWPIndex;
extern int g_iCacheWaypointIndex;
extern float g_fTimeNextBombUpdate;
extern int g_iNumWaypoints;  // number of waypoints currently in use

extern bool g_bLeaderChosenT;
extern bool g_bLeaderChosenCT;

extern char g_cKillHistory;

extern int g_rgiTerrorWaypoints[MAX_WAYPOINTS];
extern int g_iNumTerrorPoints;
extern int g_rgiCTWaypoints[MAX_WAYPOINTS];
extern int g_iNumCTPoints;
extern int g_rgiGoalWaypoints[MAX_WAYPOINTS];
extern int g_iNumGoalPoints;
extern int g_rgiCampWaypoints[MAX_WAYPOINTS];
extern int g_iNumCampPoints;
extern float g_rgfWPDisplayTime[MAX_WAYPOINTS];
extern int *g_pFloydDistanceMatrix;	// Distance Table

extern bool g_bMapInitialised;
extern bool g_bRecalcVis;
extern int g_iCurrVisIndex;
extern unsigned char g_rgbyVisLUT[MAX_WAYPOINTS][MAX_WAYPOINTS/4];

extern int NormalWeaponPrefs[NUM_WEAPONS];
extern int AgressiveWeaponPrefs[NUM_WEAPONS];
extern int DefensiveWeaponPrefs[NUM_WEAPONS];
extern int* ptrWeaponPrefs[];

extern int iNumBotnames;
extern int iNumKillChats;
extern int iNumBombChats;
extern int iNumDeadChats;
extern int iNumNoKeyStrings;

extern int min_bots;
extern int max_bots;
extern float botcreation_time;
extern bool g_bBusyCreatingBot;
extern int g_iMinBotSkill;
extern int g_iMaxBotSkill;
extern bool g_bBotChat;
extern bool g_bJasonMode;
extern bool g_bDetailNames;
extern bool g_bInstantTurns;
extern int g_iMaxNumFollow;
extern int g_iMaxWeaponPickup;
extern bool g_bShootThruWalls;
extern float g_fLastChatTime;
extern float g_fTimeRoundEnd;
extern float g_fTimeRoundMid;
extern float g_fTimeSoundUpdate;
extern float g_fTimePickupUpdate;
extern float g_fTimeGrenadeUpdate;
extern float g_fTimeNextBombUpdate;
extern int g_iLastBombPoint;
extern bool g_bBombPlanted;
extern float g_fTimeBombPlanted;
extern bool g_bBombSayString;			
extern int g_rgiBombSpotsVisited[MAXNUMBOMBSPOTS];
extern bool	g_bUseSpeech;
extern bool g_bAllowVotes;
extern bool g_bUseExperience;
extern bool g_bBotSpray;
extern bool g_bBotsCanPause; 
extern bool g_bHostageRescued;
extern int iRadioSelect[32];
extern int g_rgfLastRadio[2];
extern float g_rgfLastRadioTime[2];

extern bool g_bNeedHUDDisplay;
extern bool g_bCreatorShown;
extern float g_fTimeHUDDisplay;
extern char g_szWaypointMessage[512];

extern char g_cFunMode;
extern float g_fTimeFunUpdate;
extern int g_iNumLogos;
extern char szSprayNames[NUM_SPRAYPAINTS][20];

extern bool g_bIsBorg;
extern int state;

extern int g_iDebugGoalIndex;
extern int g_iSearchGoalIndex;


#ifdef DEBUG
extern int g_iDebugBotIndex;
extern int g_iDebugTaskNumber;
extern float g_fTimeDebugUpdate;
extern bool g_bKillMe;
#endif


#endif