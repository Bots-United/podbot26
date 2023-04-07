//
// waypoint.h
//

#ifndef WAYPOINT_H
#define WAYPOINT_H

#define MAP_AS		1
#define MAP_CS		2
#define MAP_DE		3


// defines for waypoint flags field (32 bits are available)
#define W_FL_USE_BUTTON  (1<<0)  /* use a nearby button (lifts, doors, etc.) */
#define W_FL_LIFT        (1<<1)  /* wait for lift to be down before approaching this waypoint */
#define W_FL_CROUCH      (1<<2)  /* must crouch to reach this waypoint */
#define W_FL_CROSSING	 (1<<3)	 /* a target waypoint */
#define W_FL_GOAL		 (1<<4)  /* mission goal point (bomb,hostage etc.) */
#define W_FL_LADDER		 (1<<5)  /* waypoint is on ladder */
#define W_FL_RESCUE		 (1<<6)  /* waypoint is a Hostage Rescue Point */
#define W_FL_CAMP		 (1<<7)  /* waypoint is a Camping Point */
#define W_FL_NOHOSTAGE	 (1<<8)	 /* only use this waypoint if no hostage */	

#define W_FL_TERRORIST	 (1<<29) /* It's a specific Terrorist Point */
#define W_FL_COUNTER	 (1<<30) /* It's a specific Counter Terrorist Point */

// defines for waypoint connection flags field (16 bits are available)
#define C_FL_JUMP  (1<<0)  /* Must Jump for this Connection */

#define WAYPOINT_VERSION	7
#define WAYPOINT_VERSION6	6
#define WAYPOINT_VERSION5	5

#define EXPERIENCE_VERSION	2

// define the waypoint file header structure...
typedef struct {
   char filetype[8];  // should be "PODWAY!\0"
   int  waypoint_file_version;
   int  number_of_waypoints;
   char mapname[32];  // name of map for these waypoints
   char creatorname[32];	// Name of Waypoint File Creator
} WAYPOINT_HDR;


// define the experience file header structure...
typedef struct {
   char filetype[8];  // should be "PODEXP!\0"
   int  experiencedata_file_version;
   int  number_of_waypoints;
} EXPERIENCE_HDR;


#define MAX_PATH_INDEX 8
#define OLDMAX_PATH_INDEX 4

// define the structure for waypoint paths (paths are connections between
// two waypoint nodes that indicates the bot can get from point A to point B.
// note that paths DON'T have to be two-way.  sometimes they are just one-way
// connections between two points.  There is an array called "paths" that
// contains head pointers to these structures for each waypoint index.
typedef struct path {
	int iPathNumber;
	int    flags;    // button, lift, flag, health, ammo, etc.
	Vector origin;   // location

	float Radius;	 // Maximum Distance WPT Origin can be varied

	float fcampstartx;
	float fcampstarty;
	float fcampendx;
	float fcampendy;
	short int index[MAX_PATH_INDEX];  // indexes of waypoints (index -1 means not used)
	unsigned short connectflag[MAX_PATH_INDEX];
	Vector vecConnectVel[MAX_PATH_INDEX];
	int distance[MAX_PATH_INDEX];
	struct path *next;   // link to next structure
} PATH;


typedef struct temp {
	int iPathNumber;
	int    flags;    // button, lift, flag, health, ammo, etc.
	Vector origin;   // location

	float Radius;	 // Maximum Distance WPT Origin can be varied

	float fcampstartx;
	float fcampstarty;
	float fcampendx;
	float fcampendy;
	short int index[MAX_PATH_INDEX];  // indexes of waypoints (index -1 means not used)
	unsigned short connectflag[MAX_PATH_INDEX];
	int distance[MAX_PATH_INDEX];
	struct temp *next;   // link to next structure
} TEMP;

// Path Structure used by Version 6
typedef struct path6 {
	int iPathNumber;
	int    flags;    // button, lift, flag, health, ammo, etc.
	Vector origin;   // location

	float Radius;	 // Maximum Distance WPT Origin can be varied

	float fcampstartx;
	float fcampstarty;
	float fcampendx;
	float fcampendy;
	short int index[MAX_PATH_INDEX];  // indexes of waypoints (index -1 means not used)
	int distance[MAX_PATH_INDEX];
	struct path6 *next;   // link to next structure
} PATH6;


// Path Structure used by Version 5
typedef struct oldpath {
	int iPathNumber;
	int    flags;    // button, lift, flag, health, ammo, etc.
	Vector origin;   // location

	float Radius;	 // Maximum Distance WPT Origin can be varied

	float fcampstartx;
	float fcampstarty;
	float fcampendx;
	float fcampendy;
	short int index[OLDMAX_PATH_INDEX];  // indexes of waypoints (index -1 means not used)
	int distance[OLDMAX_PATH_INDEX];
	struct oldpath *next;   // link to next structure
} OLDPATH;


// waypoint function prototypes...
void WaypointInit(void);
int WaypointFindNearest(edict_t *pEntity);
int WaypointFindNearestToMove(Vector vOrigin);
void WaypointFindInRadius(Vector vecPos,float fRadius,int *pTab,int *iCount);
void WaypointAdd(edict_t *pEntity,int iFlags);
void WaypointDelete(edict_t *pEntity);
void WaypointChangeFlags(edict_t *pEntity, int iFlag,char iMode);
void WaypointSetRadius(edict_t *pEntity, int iRadius);
void WaypointCreatePath(edict_t *pEntity, int cmd);
void WaypointRemovePath(edict_t *pEntity, int cmd);
void WaypointCalcVisibility(edict_t *pPlayer);
bool WaypointIsVisible(int iSourceIndex,int iDestIndex);
bool ConnectedToWaypoint(int a,int b);
void CalculateWaypointWayzone(edict_t *pEntity);
void CalculateWayzones(edict_t *pEntity);
bool WaypointLoad(edict_t *pEntity);
void WaypointSave(edict_t *pEntity);
bool WaypointReachable(Vector v_srv, Vector v_dest, edict_t *pEntity);
void WaypointThink(edict_t *pEntity);
void WaypointDrawBeam(edict_t *pEntity, Vector start, Vector end, int width,
        int noise, int red, int green, int blue, int brightness, int speed);
bool WaypointNodesValid(edict_t *pEntity);
bool WaypointNodeReachable(Vector v_src, Vector v_dest, edict_t *pEntity);
void SaveExperienceTab(edict_t *pEntity);
int GetAimingWaypoint(bot_t *pBot,Vector vecTargetPos, int iCount);
float GetTravelTime(float fMaxSpeed, Vector vecSource,Vector vecPosition);

// Floyd Search Prototypes
void DeleteSearchNodes(bot_t *pBot);
void InitPathMatrix();
int GetPathDistance(int iSourceWaypoint,int iDestWaypoint);
PATHNODE* BasicFindPath(int iSourceIndex, int iDestIndex, bool* bValid);
PATHNODE* FindPath(bot_t* pBot,int iSourceIndex,int iDestIndex,unsigned char byPathType);

// A* Stuff
#define MAXNODES  30000

#define OPEN 1
#define NEW 0
#define CLOSED 2


//typedef Node *NodePtr;

int gfunctionKillsT(PATHNODE *p);
int gfunctionKillsCT(PATHNODE *p);
int gfunctionKillsDistT(PATHNODE *p);
int gfunctionKillsDistCT(PATHNODE *p);
int hfunctionNone(PATHNODE *p);
int goal(PATHNODE *p);
PATHNODE *makeChildren(PATHNODE *parent);
void freeNode(PATHNODE *p);
int nodeEqual(PATHNODE *a, PATHNODE *b);
void TestAPath(edict_t *pPlayer,int iSourceIndex,int iDestIndex,unsigned char byPathType);
PATHNODE* FindLeastCostPath(edict_t *pPlayer,int iSourceIndex,int iDestIndex,unsigned char byPathType);


// function prototypes
PATHNODE  *AStarSearch(PATHNODE * root, 
		   int (*gcalc) (PATHNODE *), 
		   int (*hcalc) (PATHNODE *), 
		   int (*goalNode) (PATHNODE *), 
		   PATHNODE * (*children) (PATHNODE *), 
		   void (*freeNode) (PATHNODE *), 
		   int (*nodeEqual) (PATHNODE *, PATHNODE *)); 


#ifdef DEBUG
void ShowPath(int iSourceIndex,int iDestIndex);
#endif

int Encode(char *filename, unsigned char* header, int headersize, unsigned char *buffer, int bufsize);
int Decode(char *filename, int headersize, unsigned char *buffer, int bufsize);

#endif // WAYPOINT_H
