//
// waypoint.cpp
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "log.h"
#include "waypoint.h"
#include "bot_globals.h"


extern int m_spriteTexture;
extern char g_szWPTDirname[256];


PATH *paths[MAX_WAYPOINTS];

// Declare the Array of Experience Data
experience_t* pBotExperienceData = NULL;

int *g_pFloydPathMatrix = NULL;

int *g_pWithHostageDistMatrix = NULL;
int *g_pWithHostagePathMatrix = NULL;

bool g_bWaypointPaths = FALSE;  // have any paths been allocated?
bool g_bEndJumpPoint = FALSE;
float g_fTimeJumpStarted = 0.0;
Vector vecLearnVelocity = Vector(0,0,0);
Vector vecLearnPos = Vector(0,0,0);

int g_iLastJumpWaypoint=-1;


Vector g_vecLastWaypoint;
float g_fPathTime = 0.0;
float f_danger_time = 0.0;
static FILE *fp;


// free the linked list of waypoint path nodes...
void WaypointFree(void)
{
	PATH *p = paths[0];
	PATH *p_next;
	
	while (p)  // free the linked list
	{
		p_next = p->next;  // save the link to next
		delete p;
		p = p_next;
		
		//count++;
		//               if (count > 1000) WaypointDebug();
	}

}


// initialize the waypoint structures...
void WaypointInit(void)
{
   // have any waypoint path nodes been allocated yet?
   if (g_bWaypointPaths)
      WaypointFree();  // must free previously allocated path memory
	
   g_iNumWaypoints = 0;

   g_vecLastWaypoint = Vector(0,0,0);
   paths[0]=NULL;
}


void WaypointAddPath(edict_t *pEntity, short int add_index, short int path_index,float fDistance)
{
   PATH *p;
   int i;
   int count = 0;
   char msg[80];

   p = paths[add_index];

   // Don't allow Paths get connected twice
   for(i=0;i< MAX_PATH_INDEX;i++)
   {
	   if (p->index[i] == path_index)
		   return;
   }

   // Check for free space in the Connection
   // indices
   i = 0;
   while (i < MAX_PATH_INDEX)
   {
	   if (p->index[i] == -1)
	   {
		   p->index[i] = path_index;
		   p->distance[i]=abs(fDistance);
		   if (pEntity)
		   {
			   sprintf(msg,"Path added from %d to %d\n",add_index,path_index);
			   ClientPrint(&pEntity->v, HUD_PRINTCONSOLE,msg);
		   }
		   return;
	   }
	   
	   i++;
   }

   
   // There wasn't any free space. Try exchanging it
   // with a long-distance path
   i = 0;
   while (i < MAX_PATH_INDEX)
   {
	   if (p->distance[i]>abs(fDistance))
	   {
		   if (pEntity)
		   {
			   sprintf(msg,"Path added from %d to %d\n",add_index,path_index);
			   ClientPrint(&pEntity->v, HUD_PRINTCONSOLE,msg);
		   }
		   p->index[i] = path_index;
		   p->distance[i]=abs(fDistance);
		   return;
	   }
	   
	   i++;
   }
	  
}




// find the nearest waypoint to the player and return the index (-1 if not found)
int WaypointFindNearest(edict_t *pEntity)
{
   int i, index;
   float distance;
   float min_distance;

   // find the nearest waypoint and remove it...

   min_distance = 9999.0;

   for (i=0; i < g_iNumWaypoints; i++)
   {

      distance = (paths[i]->origin - pEntity->v.origin).Length();

      if (distance < min_distance)
      {
         index = i;
         min_distance = distance;
      }
   }

   // if not close enough to a waypoint then just return
   if (min_distance > 50)
      return -1;

   return index;
}


// find the nearest waypoint to that Origin and return the index
int WaypointFindNearestToMove(Vector vOrigin)
{
   int i, index;
   float distance;
   float min_distance;

   min_distance = 9999.0;

   for (i=0; i < g_iNumWaypoints; i++)
   {

      distance = (paths[i]->origin - vOrigin).Length();

      if (distance < min_distance)
      {
         index = i;
         min_distance = distance;
      }
   }

   return index;
}


// Returns all Waypoints within Radius from Position
void WaypointFindInRadius(Vector vecPos,float fRadius,int *pTab,int *iCount)
{
	int i,iMaxCount;
	float distance;
	
	iMaxCount = *iCount;
	*iCount = 0;
	
	for (i=0; i < g_iNumWaypoints; i++)
	{
		
		distance = (paths[i]->origin - vecPos).Length();
		
		if (distance < fRadius)
		{
			*pTab++ = i;
			*iCount += 1;
			if(*iCount == iMaxCount)
				break;
		}
	}
	*iCount -= 1;
}

void WaypointDrawBeam(edict_t *pEntity, Vector start, Vector end, int width,
        int noise, int red, int green, int blue, int brightness, int speed)
{
   MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pEntity);
   WRITE_BYTE( TE_BEAMPOINTS);
   WRITE_COORD(start.x);
   WRITE_COORD(start.y);
   WRITE_COORD(start.z);
   WRITE_COORD(end.x);
   WRITE_COORD(end.y);
   WRITE_COORD(end.z);
   WRITE_SHORT( m_spriteTexture );
   WRITE_BYTE( 1 ); // framestart
   WRITE_BYTE( 10 ); // framerate
   WRITE_BYTE( 10 ); // life in 0.1's
   WRITE_BYTE( width ); // width
   WRITE_BYTE( noise );  // noise

   WRITE_BYTE( red );   // r, g, b
   WRITE_BYTE( green );   // r, g, b
   WRITE_BYTE( blue );   // r, g, b

   WRITE_BYTE( brightness );   // brightness
   WRITE_BYTE( speed );    // speed
   MESSAGE_END();
}


void WaypointAdd(edict_t *pEntity,int iFlags)
{
	int index,i;
	edict_t *pent = NULL;
	float radius = 40;
	char item_name[64];
	float distance;
	Vector v_forward=Vector(0,0,0);
	PATH *p,*prev;
	bool bPlaceNew=TRUE;
	Vector vecNewWaypoint=pEntity->v.origin;

	g_bWaypointsChanged=TRUE;
	
	switch(iFlags)
	{
	case 6:
		index=WaypointFindNearest(pEntity);
		if(index!=-1)
		{
			p = paths[index];
			if(!(p->flags & W_FL_CAMP))
			{
				ClientPrint(&pEntity->v, HUD_PRINTCONSOLE,"This is no Camping Waypoint !\n");
				return;
			}
			UTIL_MakeVectors( pEntity->v.v_angle );
			v_forward = pEntity->v.origin + pEntity->v.view_ofs + gpGlobals->v_forward * 640;
			p->fcampendx=v_forward.x;
			p->fcampendy=v_forward.y;
			// play "done" sound...
			EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0,
				ATTN_NORM, 0, 100);
		}
		return;
	case 9:
		index=WaypointFindNearest(pEntity);
		if(index!=-1)
		{
			distance = (paths[index]->origin - pEntity->v.origin).Length();
			if(distance<50)
			{
				bPlaceNew=FALSE;
				p=paths[index];
				if(iFlags==9)
					p->origin=(p->origin+vecLearnPos)/2;
			}
		}
		else
			vecNewWaypoint=vecLearnPos;
		break;
	case 10:
		index=WaypointFindNearest(pEntity);
		if(index!=-1)
		{
			distance = (paths[index]->origin - pEntity->v.origin).Length();
			if(distance<50)
			{
				bPlaceNew=FALSE;
				p=paths[index];
				int flags=0;
				for(i=0;i<MAX_PATH_INDEX;i++)
				{
					flags+=p->connectflag[i];
				}
				if(flags==0)
					p->origin=(p->origin+pEntity->v.origin)/2;
			}
		}
		break;
	}



	if(bPlaceNew)
	{
		if (g_iNumWaypoints >= MAX_WAYPOINTS)
			return;
		
		index = 0;
		
		// find the next available slot for the new waypoint...
		p = paths[0];
		prev = NULL;
		
		// find an empty slot for new path_index...
		while (p != NULL)
		{
			prev=p;
			p=p->next;
			index++;
		}
		
		paths[index]=new PATH;
		p = paths[index];
		if(prev)
			prev->next=p;
		if (p == NULL)
		{
			// ERROR ALLOCATING MEMORY!!!
			return;
		}
		// increment total number of waypoints
		g_iNumWaypoints++;
		p->iPathNumber=index;
		p->flags = 0;
		
		// store the origin (location) of this waypoint
		p->origin = vecNewWaypoint;
		p->fcampstartx=0;
		p->fcampstarty=0;
		p->fcampendx=0;
		p->fcampendy=0;
		
		
		for(i=0;i<MAX_PATH_INDEX;i++)
		{
			p->index[i] = -1;
			p->distance[i] = 0;
			p->connectflag[i]=0;
			p->vecConnectVel[i]=Vector(0,0,0);
		}
		
		p->next = NULL;
		// store the last used waypoint for the auto waypoint code...
		g_vecLastWaypoint = pEntity->v.origin;
	}
	
	// set the time that this waypoint was originally displayed...
	g_rgfWPDisplayTime[index] = gpGlobals->time;

	if(iFlags==9)
		g_iLastJumpWaypoint=index;
	else if(iFlags==10)
	{
		distance = (paths[g_iLastJumpWaypoint]->origin - pEntity->v.origin).Length();
		WaypointAddPath(pEntity,g_iLastJumpWaypoint,index,distance);
		i = 0;
		while (i < MAX_PATH_INDEX)
		{
			if(paths[g_iLastJumpWaypoint]->index[i]==index)
			{
				paths[g_iLastJumpWaypoint]->connectflag[i] |= C_FL_JUMP;
				paths[g_iLastJumpWaypoint]->vecConnectVel[i] = vecLearnVelocity;
				break;
			}
			i++;
		}
		CalculateWaypointWayzone(pEntity);
		return;
	}
	
	
	Vector start, end;
	
	if ((pEntity->v.flags & FL_DUCKING) == FL_DUCKING)
	{
		p->flags |= W_FL_CROUCH;  // set a crouch waypoint
		
		start = vecNewWaypoint - Vector(0, 0, 17);
		end = start + Vector(0, 0, 34);
	}
	else
	{
		start = vecNewWaypoint - Vector(0, 0, 34);
		end = start + Vector(0, 0, 68);
	}
	
	//********************************************************
	// look for buttons, lift, ammo, flag, health, armor, etc.
	//********************************************************
	
	while ((pent = UTIL_FindEntityInSphere( pent, pEntity->v.origin, radius )) != NULL)
	{
		strcpy(item_name, STRING(pent->v.classname));
		
		if (strcmp("func_button", item_name) == 0)
		{
			ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "Button detected. Setting Flag!\n");
			p->flags = W_FL_USE_BUTTON;
		}
		
	}
	switch(iFlags)
	{
	case 1:		// Terrorist Crossing
		p->flags|=W_FL_CROSSING;
		p->flags|=W_FL_TERRORIST;
		WaypointDrawBeam(pEntity, start, end, 30, 0, 255, 0, 0, 250, 5);
		break;
	case 2:				// CT Crossing
		p->flags|=W_FL_CROSSING;
		p->flags|=W_FL_COUNTER;
		WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);
		break;
	case 3:
		p->flags|=W_FL_LADDER;
		UTIL_MakeVectors( pEntity->v.v_angle );
		v_forward = pEntity->v.origin + pEntity->v.view_ofs + gpGlobals->v_forward * 640;
		p->fcampstarty=v_forward.y;
		WaypointDrawBeam(pEntity, start, end, 30, 0, 255, 0, 255, 250, 5);
		break;
	case 4:
		p->flags|=W_FL_RESCUE;
		WaypointDrawBeam(pEntity, start, end, 30, 0, 255, 255, 255, 250, 5);
		break;
	case 5:				// Camping Point
		p->flags|=W_FL_CROSSING;
		p->flags|=W_FL_CAMP;
		UTIL_MakeVectors( pEntity->v.v_angle );
		v_forward = pEntity->v.origin + pEntity->v.view_ofs + gpGlobals->v_forward * 640;
		p->fcampstartx=v_forward.x;
		p->fcampstarty=v_forward.y;
		WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 255, 255, 250, 5);
		break;
	case 100:
		p->flags|=W_FL_GOAL;
		WaypointDrawBeam(pEntity, start, end, 30, 0, 255, 0, 255, 250, 5);
		break;
	default:
		WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 255, 0, 250, 5);
		break;
	}
		
	EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/xbow_hit1.wav", 1.0,
		ATTN_NORM, 0, 100);

	// Ladder waypoints need careful connections 
	if(iFlags==3)
	{
		float min_distance = 9999.0;
		int iDestIndex=-1;
		// calculate all the paths to this new waypoint
		for (i=0; i < g_iNumWaypoints; i++)
		{
			if (i == index)
				continue;  // skip the waypoint that was just added
			// Other ladder waypoints should connect to this
			if(paths[i]->flags & W_FL_LADDER)
			{
				// check if the waypoint is reachable from the new one (one-way)
				if ( WaypointNodeReachable(vecNewWaypoint, paths[i]->origin, pEntity) )
				{
					distance = (paths[i]->origin - vecNewWaypoint).Length();
					
					WaypointAddPath(pEntity,index, i,distance);
				}
				
				// check if the new one is reachable from the waypoint (other way)
				if ( WaypointNodeReachable(paths[i]->origin, vecNewWaypoint, pEntity) )
				{
					distance = (paths[i]->origin - vecNewWaypoint).Length();
					WaypointAddPath(pEntity,i, index,distance);
				}
			}
			else
			{
				// check if the waypoint is reachable from the new one (one-way)
				if ( WaypointNodeReachable(vecNewWaypoint, paths[i]->origin, pEntity) )
				{
					distance = (paths[i]->origin - vecNewWaypoint).Length();
					if (distance < min_distance)
					{
						iDestIndex = i;
						min_distance = distance;
					}
				}
			}
		}
		if((iDestIndex>-1) && (iDestIndex<g_iNumWaypoints))
		{
			// check if the waypoint is reachable from the new one (one-way)
			if ( WaypointNodeReachable(vecNewWaypoint, paths[iDestIndex]->origin, pEntity) )
			{
				distance = (paths[iDestIndex]->origin - vecNewWaypoint).Length();
				
				WaypointAddPath(pEntity,index, iDestIndex,distance);
			}
			
			// check if the new one is reachable from the waypoint (other way)
			if ( WaypointNodeReachable(paths[iDestIndex]->origin, vecNewWaypoint, pEntity) )
			{
				distance = (paths[iDestIndex]->origin - vecNewWaypoint).Length();
				WaypointAddPath(pEntity,iDestIndex, index,distance);
			}
		}

	}
	else
	{
		// calculate all the paths to this new waypoint
		for (i=0; i < g_iNumWaypoints; i++)
		{
			if (i == index)
				continue;  // skip the waypoint that was just added
			
			// check if the waypoint is reachable from the new one (one-way)
			if ( WaypointNodeReachable(vecNewWaypoint, paths[i]->origin, pEntity) )
			{
				distance = (paths[i]->origin - vecNewWaypoint).Length();
				
				WaypointAddPath(pEntity,index, i,distance);
			}
			
			// check if the new one is reachable from the waypoint (other way)
			if ( WaypointNodeReachable(paths[i]->origin, vecNewWaypoint, pEntity) )
			{
				distance = (paths[i]->origin - vecNewWaypoint).Length();
				WaypointAddPath(pEntity,i, index,distance);
			}
		}
	}
	CalculateWaypointWayzone(pEntity);
}



void WaypointDelete(edict_t *pEntity)
{
	g_bWaypointsChanged=TRUE;
	int index;
	int count = 0;
	
	if (g_iNumWaypoints < 1)
		return;
	
	index = WaypointFindNearest(pEntity);
	
	if (index == -1)
		return;
	
	PATH *p_previous=NULL;
	PATH *p = paths[index];
	
	if (paths[index] != NULL)
	{
		if(index>0)
			p_previous=paths[index-1];
	}
	
	int i;
	int ix;
	count = 0;
	
	// delete all references to Node
	for (i=0; i < g_iNumWaypoints; i++)
	{
		p=paths[i];
		ix=0;
		while (ix < MAX_PATH_INDEX)
		{
			if (p->index[ix] == index)
			{
				p->index[ix] = -1;  // unassign this path
				p->connectflag[ix] = 0;
				p->distance[ix]=0;
				p->vecConnectVel[ix] = Vector(0,0,0);
			}
			ix++;
		}
	}

	// delete all connections to Node
	for (i=0; i < g_iNumWaypoints; i++)
	{
		p=paths[i];
		// if Pathnumber bigger than deleted Node Number=Number -1
		if(p->iPathNumber>index)
			p->iPathNumber--;
		ix=0;
		while (ix < MAX_PATH_INDEX)
		{
			if(p->index[ix]>index)
				p->index[ix]--;
			ix++;
		}
	}
	// free deleted node
	delete paths[index];
	paths[index]=NULL;

	if(index==0 && g_iNumWaypoints==1)
		return;
	// Rotate Path Array down
	for (i=index; i < g_iNumWaypoints-1; i++)
		paths[i]=paths[i+1];

	if(p_previous)
		p_previous->next=paths[index];

	g_iNumWaypoints--;

	g_rgfWPDisplayTime[index] = 0.0;
	
	EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/mine_activate.wav", 1.0,
		ATTN_NORM, 0, 100);
	g_bWaypointsChanged=TRUE;
}

// Allow manually changing Flags
void WaypointChangeFlags(edict_t *pEntity, int iFlag,char iMode)
{
	int index;
	PATH *p;

	index=WaypointFindNearest(pEntity);
	if(index!=-1)
	{
		p = paths[index];
		// Delete Flag
		if(iMode==0)
			p->flags &= ~iFlag;
		else
			p->flags|=iFlag;
		// play "done" sound...
		EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0,
			ATTN_NORM, 0, 100);
	}
	return;
}


// Allow manually setting the Zone Radius
void WaypointSetRadius(edict_t *pEntity, int iRadius)
{
	int index;

	index=WaypointFindNearest(pEntity);
	if(index!=-1)
	{
		paths[index]->Radius=(float)iRadius;
		// play "done" sound...
		EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0,
			ATTN_NORM, 0, 100);
	}
	return;
}


// allow player to manually create a path from one waypoint to another
void WaypointCreatePath(edict_t *pEntity, int iNodeNum)
{
	int waypoint1 = WaypointFindNearest(pEntity);

	if (waypoint1 == -1)
		return;
	if((iNodeNum<0) || (iNodeNum>=g_iNumWaypoints))
	{
		ClientPrint(VARS(pEntity), HUD_PRINTCONSOLE, "Waypoint Number out of Range !\n");
		return;
	}
		
	float distance = (paths[iNodeNum]->origin - paths[waypoint1]->origin).Length();
	
	WaypointAddPath(pEntity,waypoint1, iNodeNum,distance);
	// play "done" sound...
	EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0,
		ATTN_NORM, 0, 100);
}


// allow player to manually remove a path from one waypoint to another
void WaypointRemovePath(edict_t *pEntity, int iNodeNum)
{
	int waypoint1 = WaypointFindNearest(pEntity);

	if (waypoint1 == -1)
		return;
	if((iNodeNum<0) || (iNodeNum>g_iNumWaypoints))
	{
		ClientPrint(VARS(pEntity), HUD_PRINTCONSOLE, "Waypoint Number out of Range !\n");
		return;
	}
	
	int i=0;
	while (i < MAX_PATH_INDEX)
	{
		if (paths[waypoint1]->index[i] == iNodeNum)
		{
			paths[waypoint1]->index[i] = -1;  // unassign this path
			paths[waypoint1]->connectflag[i] = 0;
			paths[waypoint1]->vecConnectVel[i] = Vector(0,0,0);
			paths[waypoint1]->distance[i] = 0;  
		}
		i++;
	}
		
	// play "done" sound...
	EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0,
		ATTN_NORM, 0, 100);
}

// Checks if Waypoint A has a Connection to Waypoint Nr. B
bool ConnectedToWaypoint(int a,int b)
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


void CalculateWaypointWayzone(edict_t *pEntity)
{
	PATH* p;
	Vector start;
	Vector vRadiusEnd;
	Vector v_direction;
	TraceResult tr;

	int index = WaypointFindNearest(pEntity);
	p=paths[index];
	
	if(p->flags & W_FL_LADDER)
	{
		p->Radius=0;
		return;
	}
	else if(p->flags & W_FL_GOAL)
	{
		p->Radius=0;
		return;
	}
	else if(p->flags & W_FL_CAMP)
	{
		p->Radius=0;
		return;
	}
	else if(p->flags & W_FL_RESCUE)
	{
		p->Radius=0;
		return;
	}
	else if(p->flags & W_FL_CROUCH)
	{
		p->Radius=0;
		return;
	}
	else if(g_bLearnJumpWaypoint)
	{
		p->Radius=0;
		return;
	}

	int x = 0;
	while (x < MAX_PATH_INDEX)
	{
		if (p->index[x] != -1)
		{
			if(paths[p->index[x]]->flags & W_FL_LADDER)
			{
				p->Radius=0;
				return;
			}
		}
		
		x++;
	}
	
	bool bWayBlocked=FALSE;
	for (int iScanDistance=32;iScanDistance<128;iScanDistance+=16)
	{
		start = p->origin;
		UTIL_MakeVectors(Vector (0,0,0));
		vRadiusEnd=start+(gpGlobals->v_forward*iScanDistance);
		v_direction = vRadiusEnd - start;
		v_direction = UTIL_VecToAngles(v_direction);
		p->Radius=iScanDistance;
		for(float fRadCircle=0.0;fRadCircle<360.0;fRadCircle+=20)
		{
			UTIL_MakeVectors(v_direction);
			vRadiusEnd=start+(gpGlobals->v_forward*iScanDistance);
			UTIL_TraceLine( start, vRadiusEnd, ignore_monsters,
				pEntity->v.pContainingEntity, &tr );
			if (tr.flFraction < 1.0)
			{
				if ((FStrEq("func_door",STRING(tr.pHit->v.classname))) ||
					(FStrEq("func_door_rotating",STRING(tr.pHit->v.classname))))
				{
					p->Radius=0;
					bWayBlocked=TRUE;
					break;
				}
				bWayBlocked=TRUE;
				p->Radius-=16;
				break;
			}
			vRadiusEnd.z+=34;
			UTIL_TraceLine( start, vRadiusEnd, ignore_monsters,
				pEntity->v.pContainingEntity, &tr );
			if (tr.flFraction < 1.0)
			{
				bWayBlocked=TRUE;
				p->Radius-=16;
				break;
			}
			v_direction.y += fRadCircle;
			UTIL_ClampAngle(&v_direction.y);
		}
		if(bWayBlocked)
			break;
	}
	p->Radius-=16;
	if(p->Radius<0)
		p->Radius=0;
}

// Calculate "Wayzones" for each waypoint (meaning a
// dynamic distance area to vary waypoint origin)
void CalculateWayzones(edict_t *pEntity)
{
	int i;
	PATH* p;
	Vector start;
	Vector vRadiusEnd;
	Vector v_direction;
	TraceResult tr;

    ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "Calculating Wayzones...please wait!\n");

	for (i=0; i < g_iNumWaypoints; i++)
	{
		p=paths[i];

		if(p->flags & W_FL_LADDER)
		{
			p->Radius=0;
			continue;
		}
		else if(p->flags & W_FL_GOAL)
		{
			p->Radius=0;
			continue;
		}
		else if(p->flags & W_FL_CAMP)
		{
			p->Radius=0;
			continue;
		}
		else if(p->flags & W_FL_RESCUE)
		{
			p->Radius=0;
			continue;
		}
		else if(p->flags & W_FL_CROUCH)
		{
			p->Radius=0;
			continue;
		}
		int x = 0;
		bool bLadderLink=FALSE;
		while (x < MAX_PATH_INDEX)
		{
			if (p->index[x] != -1)
			{
				if(paths[p->index[x]]->flags & W_FL_LADDER)
				{
					p->Radius=0;
					bLadderLink=TRUE;
					break;
				}
			}
			
			x++;
		}
		if(bLadderLink)
			continue;

		bool bWayBlocked=FALSE;
		for (int iScanDistance=32;iScanDistance<128;iScanDistance+=16)
		{
			start = paths[i]->origin;
			UTIL_MakeVectors(Vector (0,0,0));
			vRadiusEnd=start+(gpGlobals->v_forward*iScanDistance);
			v_direction = vRadiusEnd - start;
			v_direction = UTIL_VecToAngles(v_direction);
			p->Radius=iScanDistance;
			for(float fRadCircle=0.0;fRadCircle<360.0;fRadCircle+=20)
			{
				UTIL_MakeVectors(v_direction);
				vRadiusEnd=start+(gpGlobals->v_forward*iScanDistance);
				UTIL_TraceLine( start, vRadiusEnd, ignore_monsters,
					pEntity->v.pContainingEntity, &tr );
				if (tr.flFraction < 1.0)
				{
					if ((FStrEq("func_door",STRING(tr.pHit->v.classname))) ||
						(FStrEq("func_door_rotating",STRING(tr.pHit->v.classname))))
					{
						p->Radius=0;
						bWayBlocked=TRUE;
						break;
					}
					bWayBlocked=TRUE;
					p->Radius-=16;
					break;
				}
				vRadiusEnd.z+=34;
				UTIL_TraceLine( start, vRadiusEnd, ignore_monsters,
					pEntity->v.pContainingEntity, &tr );
				if (tr.flFraction < 1.0)
				{
					bWayBlocked=TRUE;
					p->Radius-=16;
					break;
				}
				v_direction.y+=fRadCircle;
				UTIL_ClampAngle(&v_direction.y);
			}
			if(bWayBlocked)
				break;
		}
		p->Radius-=16;
		if(p->Radius<0)
			p->Radius=0;
	}
}

void SaveExperienceTab(edict_t *pEntity)
{
	char dirname[32];
	char filename[256];
	char msg[80];
	EXPERIENCE_HDR header;
	experiencesave_t *pExperienceSave;

	if(g_iNumWaypoints<=0 || !g_bUseExperience || g_bWaypointsChanged)
		return;
	
#ifdef DEBUG
	report_log (LOG_BOTBASE, "SaveExperienceTab",NULL);
#endif

	strcpy(header.filetype, "PODEXP!");
	
	header.experiencedata_file_version = EXPERIENCE_VERSION;
	
	header.number_of_waypoints = g_iNumWaypoints;
	
#ifndef __linux__
    strcpy(dirname, "cstrike\\PODBot\\");
#else
    strcpy(dirname, "cstrike/PODBot/");
#endif
	strcat(dirname,g_szWPTDirname);

#ifndef __linux__
	strcat(dirname,"\\");
#else
	strcat(dirname,"/");
#endif
	
	strcpy(filename, dirname);
	strcat(filename, STRING(gpGlobals->mapname));
	strcat(filename, ".pxp");

	pExperienceSave=new experiencesave_t[g_iNumWaypoints*g_iNumWaypoints];
	if(pExperienceSave==NULL)
	{
		sprintf(msg, "ERROR: Couldn't allocate Memory for saving Experience Data!\n");
		if (pEntity)
			ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, msg);
		ALERT( at_console, msg );
#ifdef __linux__
		printf(msg);
#endif
		return;
	}
	sprintf(msg, "Compressing & saving Experience Data...this may take a while!\n");
	if (pEntity)
		ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, msg);
	ALERT( at_console, msg );
#ifdef __linux__
	printf(msg);
#endif
	int test=0;
	int index=0;
	int result=0;
	for(int i=0;i<g_iNumWaypoints;i++)
	{
		for(int j=0;j<g_iNumWaypoints;j++)
		{
			(pExperienceSave+(i*g_iNumWaypoints)+j)->uTeam0Damage = (pBotExperienceData+(i*g_iNumWaypoints)+j)->uTeam0Damage>>3;
			(pExperienceSave+(i*g_iNumWaypoints)+j)->uTeam1Damage = (pBotExperienceData+(i*g_iNumWaypoints)+j)->uTeam1Damage>>3;
			(pExperienceSave+(i*g_iNumWaypoints)+j)->cTeam0Value = (pBotExperienceData+(i*g_iNumWaypoints)+j)->wTeam0Value/8;
			(pExperienceSave+(i*g_iNumWaypoints)+j)->cTeam1Value = (pBotExperienceData+(i*g_iNumWaypoints)+j)->wTeam1Value/8;

			if((pBotExperienceData+(i*g_iNumWaypoints)+j)->wTeam0Value>test)
			{
				test=(pBotExperienceData+(i*g_iNumWaypoints)+j)->wTeam0Value;
				index=i;
				result=(pExperienceSave+(i*g_iNumWaypoints)+j)->cTeam0Value;
			}
		}
	}
#ifdef _DEBUG
	sprintf(msg, "Index:%d - 1:%d - 2:%d\n",index,test,result);
	ALERT( at_console, msg );
#endif

	int iResult=Encode(filename,(unsigned char*)&header,sizeof(EXPERIENCE_HDR),(unsigned char*)pExperienceSave,g_iNumWaypoints*g_iNumWaypoints*sizeof(experiencesave_t));

	delete []pExperienceSave;
	
	if(iResult == -1)
	{
		sprintf(msg, "ERROR: Couldn't save Experience Data!\n");
		if (pEntity)
			ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, msg);
		ALERT( at_console, msg );
#ifdef __linux__
		printf(msg);
#endif
		return;
	}

	sprintf(msg, "Experience Data saved...\n");
	if (pEntity)
		ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, msg);
	ALERT( at_console, msg );
#ifdef __linux__
	printf(msg);
#endif
}



void InitExperienceTab(edict_t *pEntity)
{
	if(!g_bUseExperience)
		return;
#ifdef DEBUG
	report_log (LOG_BOTBASE, "InitExperienceTab",NULL);
#endif

	EXPERIENCE_HDR header;
	char dirname[256];
	char filename[256];
	char wptfilename[256];
	char msg[128];

	if(pBotExperienceData!=NULL)
	{
		delete []pBotExperienceData;
		pBotExperienceData = NULL;
	}

	strcpy(filename, STRING(gpGlobals->mapname));

#ifndef __linux__
    strcpy(dirname, "PODBot\\");
#else
    strcpy(dirname, "PODBot/");
#endif
	strcat(dirname,g_szWPTDirname);
#ifndef __linux__
	strcat(dirname,"\\");
#else
	strcat(dirname,"/");
#endif	
	strcpy(filename, dirname);
	strcat(filename, STRING(gpGlobals->mapname));
	strcpy(wptfilename,filename);
	strcat(filename, ".pxp");
	strcat(wptfilename, ".pwf");
	
	pBotExperienceData = new experience_t[g_iNumWaypoints*g_iNumWaypoints];
	if(pBotExperienceData == NULL)
	{
		sprintf(msg, "ERROR: Couldn't allocate Memory for Experience Data !\n");
		if (pEntity)
			ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, msg);
		ALERT( at_console, msg );
#ifdef __linux__
		printf(msg);
#endif
		return;
	}


	int iCompare;
	bool bDataLoaded=FALSE;
	bool bExperienceExists=FALSE;


	// Does the Experience File exist & is newer than waypoint file ?
	if (COMPARE_FILE_TIME(filename, wptfilename, &iCompare))
	{
		if ( iCompare > 0 )
			bExperienceExists=TRUE;
	}

	if(bExperienceExists)
	{
		// Now build the real filename
		strcpy (wptfilename,filename);
#ifndef __linux__
		strcpy(filename,"cstrike\\");
#else
		strcpy(filename,"cstrike/");
#endif
		strcat(filename,wptfilename);
		
		FILE *bfp = fopen(filename, "rb");
		
		// if file exists, read the experience Data from it
		if (bfp != NULL)
		{
			fread(&header, sizeof(EXPERIENCE_HDR), 1, bfp);
			fclose(bfp);
			
			header.filetype[7] = 0;
			if (strcmp(header.filetype, "PODEXP!") == 0)
			{
				if (header.experiencedata_file_version == EXPERIENCE_VERSION
				&& header.number_of_waypoints == g_iNumWaypoints)
				{
					sprintf(msg, "Loading & decompressing Experience Data\n");
					if (pEntity)
						ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, msg);
					ALERT( at_console, msg );
#ifdef __linux__
					printf(msg);
#endif
					experiencesave_t *pExperienceLoad;
					
					pExperienceLoad=new experiencesave_t[g_iNumWaypoints*g_iNumWaypoints];
					if(pExperienceLoad==NULL)
					{
						sprintf(msg, "ERROR: Couldn't allocate Memory for Experience Data !\n");
						if (pEntity)
							ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, msg);
						ALERT( at_console, msg );
#ifdef __linux__
						printf(msg);
#endif
						// Turn Experience off to be safe
						g_bUseExperience = FALSE;
						return;
					}
					
					Decode(filename,sizeof(EXPERIENCE_HDR),(unsigned char*)pExperienceLoad,g_iNumWaypoints*g_iNumWaypoints*sizeof(experiencesave_t));
					unsigned short test=0;
					int index=0;
					for(int i=0;i<g_iNumWaypoints;i++)
					{
						for(int j=0;j<g_iNumWaypoints;j++)
						{
							if(i == j)
							{
								(pBotExperienceData+(i*g_iNumWaypoints)+j)->uTeam0Damage=(unsigned short)((pExperienceLoad+(i*g_iNumWaypoints)+j)->uTeam0Damage);
								(pBotExperienceData+(i*g_iNumWaypoints)+j)->uTeam1Damage=(unsigned short)((pExperienceLoad+(i*g_iNumWaypoints)+j)->uTeam1Damage);
							}
							else
							{
								(pBotExperienceData+(i*g_iNumWaypoints)+j)->uTeam0Damage=(unsigned short)((pExperienceLoad+(i*g_iNumWaypoints)+j)->uTeam0Damage)<<3;
								(pBotExperienceData+(i*g_iNumWaypoints)+j)->uTeam1Damage=(unsigned short)((pExperienceLoad+(i*g_iNumWaypoints)+j)->uTeam1Damage)<<3;
							}
							(pBotExperienceData+(i*g_iNumWaypoints)+j)->wTeam0Value=(signed short)((pExperienceLoad+(i*g_iNumWaypoints)+j)->cTeam0Value)*8;
							(pBotExperienceData+(i*g_iNumWaypoints)+j)->wTeam1Value=(signed short)((pExperienceLoad+(i*g_iNumWaypoints)+j)->cTeam1Value)*8;
							if((pBotExperienceData+(i*g_iNumWaypoints)+j)->wTeam0Value>test)
							{
								test=(pBotExperienceData+(i*g_iNumWaypoints)+j)->wTeam0Value;
								index=i;
							}
						}
					}
					delete []pExperienceLoad;
#ifdef _DEBUG
					sprintf(msg, "Index:%d - Damage:%d\n",index,test);
					ALERT( at_console, msg );
#endif
					bDataLoaded=TRUE;
				}
			}
		}
	}


	if(!bDataLoaded)
	{
		memset(pBotExperienceData, 0, sizeof(experience_t)*g_iNumWaypoints*g_iNumWaypoints);
		sprintf(msg, "No Experience Data File or old one - starting new !\n");
		if (pEntity)
			ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, msg);
		ALERT( at_console, msg );
#ifdef __linux__
		printf(msg);
#endif
	}
	else
	{
		sprintf(msg, "Experience Data loaded from File...\n");
		if (pEntity)
			ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, msg);
		ALERT( at_console, msg );
#ifdef __linux__
		printf(msg);
#endif
	}
}



void InitWaypointTypes()
{
	g_iNumTerrorPoints = 0;
	g_iNumCTPoints = 0;
	g_iNumGoalPoints = 0;
	g_iNumCampPoints = 0;

	memset(&g_rgiTerrorWaypoints[0], 0, sizeof(g_rgiTerrorWaypoints));
	memset(&g_rgiCTWaypoints[0], 0, sizeof(g_rgiCTWaypoints));
	memset(&g_rgiGoalWaypoints[0], 0, sizeof(g_rgiGoalWaypoints));
	memset(&g_rgiCampWaypoints[0], 0, sizeof(g_rgiCampWaypoints));
	
	for (int index=0; index < g_iNumWaypoints; index++)
	{
		if(paths[index]->flags & W_FL_TERRORIST)
		{
			g_rgiTerrorWaypoints[g_iNumTerrorPoints]=index;
			g_iNumTerrorPoints++;
		}
		else if(paths[index]->flags & W_FL_COUNTER)
		{
			g_rgiCTWaypoints[g_iNumCTPoints]=index;
			g_iNumCTPoints++;
		}
		else if(paths[index]->flags & W_FL_GOAL)
		{
			g_rgiGoalWaypoints[g_iNumGoalPoints]=index;
			g_iNumGoalPoints++;
		}
		else if(paths[index]->flags & W_FL_CAMP)
		{
			g_rgiCampWaypoints[g_iNumCampPoints]=index;
			g_iNumCampPoints++;
		}
	}

	if(g_iNumTerrorPoints>0)
		g_iNumTerrorPoints--;
	if(g_iNumCTPoints>0)
		g_iNumCTPoints--;
	if(g_iNumGoalPoints>0)
		g_iNumGoalPoints--;
	if(g_iNumCampPoints>0)
		g_iNumCampPoints--;
}


bool WaypointLoad(edict_t *pEntity)
{
	char dirname[256];
	char filename[256];
	WAYPOINT_HDR header;
	char msg[128];
	int index;
	bool bOldWaypointFormat=FALSE;
	
	g_bMapInitialised = FALSE;
	g_bRecalcVis = FALSE;
	g_bNeedHUDDisplay = TRUE;
	g_bCreatorShown = FALSE;
	strcpy(filename, STRING(gpGlobals->mapname));
	// Assume Defusion Map
	g_iMapType=MAP_DE;
	if(filename[0]=='a')
		g_iMapType=MAP_AS;
	else if(filename[0]=='c')
		g_iMapType=MAP_CS;
	else if(filename[0]=='d')
		g_iMapType=MAP_DE;

#ifndef __linux__
    strcpy(dirname, "cstrike\\PODBot\\");
#else
    strcpy(dirname, "cstrike/PODBot/");
#endif
	strcat(dirname,g_szWPTDirname);
#ifndef __linux__
	strcat(dirname,"\\");
#else
	strcat(dirname,"/");
#endif	
	strcpy(filename, dirname);
	strcat(filename, STRING(gpGlobals->mapname));
	strcat(filename, ".pwf");
	

	FILE *bfp = fopen(filename, "rb");
	
	// if file exists, read the waypoint structure from it
	if (bfp != NULL)
	{
		fread(&header, sizeof(header), 1, bfp);
		
		header.filetype[7] = 0;
		if (strcmp(header.filetype, "PODWAY!") == 0)
		{
//			bOldWaypointFormat=TRUE;
			if (header.waypoint_file_version != WAYPOINT_VERSION)
			{
				if (header.waypoint_file_version == WAYPOINT_VERSION5 ||
					header.waypoint_file_version == WAYPOINT_VERSION6)
				{
					sprintf(msg, "Old POD Bot waypoint file version (V%d)!\nTrying to convert...\n",header.waypoint_file_version);
					ALERT( at_console, msg );
#ifdef __linux__
					printf(msg);
#endif
					bOldWaypointFormat=TRUE;
				}
				else
				{
					sprintf(msg, "%s Incompatible POD Bot waypoint file version!\nWaypoints not loaded!\n",filename);
					sprintf(g_szWaypointMessage, "%s Incompatible POD Bot waypoint file version!\nWaypoints not loaded!\n",filename);
					ALERT( at_console, msg );
#ifdef __linux__
					printf(msg);
#endif
					fclose(bfp);
					fp=fopen("PODERROR.txt","a"); fprintf(fp, msg);
					fclose(fp);
					return FALSE;
				}
			}
			
			header.mapname[31] = 0;
			
			if (strcmp(header.mapname, STRING(gpGlobals->mapname)) == 0)
			{
				WaypointInit();  // remove any existing waypoints
				g_iNumWaypoints = header.number_of_waypoints;
				
				// read and add waypoint paths...
				for (index=0; index < g_iNumWaypoints; index++)
				{
					// It's an old format - convert...
					if(bOldWaypointFormat)
					{
						// Oldest Format to convert
						if(header.waypoint_file_version == WAYPOINT_VERSION5)
						{
							OLDPATH convpath;
							
							paths[index]=new PATH;
							// read 1 oldpath
							fread(&convpath, sizeof(OLDPATH), 1, bfp);
							// Convert old to new
							paths[index]->iPathNumber=convpath.iPathNumber;
							paths[index]->flags=convpath.flags;
							paths[index]->origin=convpath.origin;
							paths[index]->Radius=convpath.Radius;
							paths[index]->fcampstartx=convpath.fcampstartx;
							paths[index]->fcampstarty=convpath.fcampstarty;
							paths[index]->fcampendx=convpath.fcampendx;
							paths[index]->fcampendy=convpath.fcampendy;
							paths[index]->index[0]=convpath.index[0];
							paths[index]->index[1]=convpath.index[1];
							paths[index]->index[2]=convpath.index[2];
							paths[index]->index[3]=convpath.index[3];
							paths[index]->index[4]=-1;
							paths[index]->index[5]=-1;
							paths[index]->index[6]=-1;
							paths[index]->index[7]=-1;
							paths[index]->distance[0]=convpath.distance[0];
							paths[index]->distance[1]=convpath.distance[1];
							paths[index]->distance[2]=convpath.distance[2];
							paths[index]->distance[3]=convpath.distance[3];
							paths[index]->distance[4]=0;
							paths[index]->distance[5]=0;
							paths[index]->distance[6]=0;
							paths[index]->distance[7]=0;
							paths[index]->connectflag[0]=0;
							paths[index]->connectflag[1]=0;
							paths[index]->connectflag[2]=0;
							paths[index]->connectflag[3]=0;
							paths[index]->connectflag[4]=0;
							paths[index]->connectflag[5]=0;
							paths[index]->connectflag[6]=0;
							paths[index]->connectflag[7]=0;
							paths[index]->vecConnectVel[0]=Vector(0,0,0);
							paths[index]->vecConnectVel[1]=Vector(0,0,0);
							paths[index]->vecConnectVel[2]=Vector(0,0,0);
							paths[index]->vecConnectVel[3]=Vector(0,0,0);
							paths[index]->vecConnectVel[4]=Vector(0,0,0);
							paths[index]->vecConnectVel[5]=Vector(0,0,0);
							paths[index]->vecConnectVel[6]=Vector(0,0,0);
							paths[index]->vecConnectVel[7]=Vector(0,0,0);
						}
/*						else if(header.waypoint_file_version == WAYPOINT_VERSION)
						{
							TEMP temppath;
							
							paths[index]=new PATH;
							// read 1 oldpath
							fread(&temppath, sizeof(TEMP), 1, bfp);
							// Convert old to new
							paths[index]->iPathNumber=temppath.iPathNumber;
							paths[index]->flags=temppath.flags;
							paths[index]->origin=temppath.origin;
							paths[index]->Radius=temppath.Radius;
							paths[index]->fcampstartx=temppath.fcampstartx;
							paths[index]->fcampstarty=temppath.fcampstarty;
							paths[index]->fcampendx=temppath.fcampendx;
							paths[index]->fcampendy=temppath.fcampendy;
							paths[index]->index[0]=temppath.index[0];
							paths[index]->index[1]=temppath.index[1];
							paths[index]->index[2]=temppath.index[2];
							paths[index]->index[3]=temppath.index[3];
							paths[index]->index[4]=temppath.index[4];
							paths[index]->index[5]=temppath.index[5];
							paths[index]->index[6]=temppath.index[6];
							paths[index]->index[7]=temppath.index[7];
							paths[index]->distance[0]=temppath.distance[0];
							paths[index]->distance[1]=temppath.distance[1];
							paths[index]->distance[2]=temppath.distance[2];
							paths[index]->distance[3]=temppath.distance[3];
							paths[index]->distance[4]=temppath.distance[4];
							paths[index]->distance[5]=temppath.distance[5];
							paths[index]->distance[6]=temppath.distance[6];
							paths[index]->distance[7]=temppath.distance[7];
							paths[index]->connectflag[0]=temppath.connectflag[0];
							paths[index]->connectflag[1]=temppath.connectflag[1];
							paths[index]->connectflag[2]=temppath.connectflag[2];
							paths[index]->connectflag[3]=temppath.connectflag[3];
							paths[index]->connectflag[4]=temppath.connectflag[4];
							paths[index]->connectflag[5]=temppath.connectflag[5];
							paths[index]->connectflag[6]=temppath.connectflag[6];
							paths[index]->connectflag[7]=temppath.connectflag[7];
							paths[index]->vecConnectVel[0]=Vector(0,0,0);
							paths[index]->vecConnectVel[1]=Vector(0,0,0);
							paths[index]->vecConnectVel[2]=Vector(0,0,0);
							paths[index]->vecConnectVel[3]=Vector(0,0,0);
							paths[index]->vecConnectVel[4]=Vector(0,0,0);
							paths[index]->vecConnectVel[5]=Vector(0,0,0);
							paths[index]->vecConnectVel[6]=Vector(0,0,0);
							paths[index]->vecConnectVel[7]=Vector(0,0,0);
						}*/
						else
						{
							PATH6 convpath;
							
							paths[index]=new PATH;
							// read 1 oldpath
							fread(&convpath, sizeof(PATH6), 1, bfp);
							// Convert old to new
							paths[index]->iPathNumber=convpath.iPathNumber;
							paths[index]->flags=convpath.flags;
							paths[index]->origin=convpath.origin;
							paths[index]->Radius=convpath.Radius;
							paths[index]->fcampstartx=convpath.fcampstartx;
							paths[index]->fcampstarty=convpath.fcampstarty;
							paths[index]->fcampendx=convpath.fcampendx;
							paths[index]->fcampendy=convpath.fcampendy;
							paths[index]->index[0]=convpath.index[0];
							paths[index]->index[1]=convpath.index[1];
							paths[index]->index[2]=convpath.index[2];
							paths[index]->index[3]=convpath.index[3];
							paths[index]->index[4]=convpath.index[4];
							paths[index]->index[5]=convpath.index[5];
							paths[index]->index[6]=convpath.index[6];
							paths[index]->index[7]=convpath.index[7];
							paths[index]->distance[0]=convpath.distance[0];
							paths[index]->distance[1]=convpath.distance[1];
							paths[index]->distance[2]=convpath.distance[2];
							paths[index]->distance[3]=convpath.distance[3];
							paths[index]->distance[4]=convpath.distance[4];
							paths[index]->distance[5]=convpath.distance[5];
							paths[index]->distance[6]=convpath.distance[6];
							paths[index]->distance[7]=convpath.distance[7];
							paths[index]->connectflag[0]=0;
							paths[index]->connectflag[1]=0;
							paths[index]->connectflag[2]=0;
							paths[index]->connectflag[3]=0;
							paths[index]->connectflag[4]=0;
							paths[index]->connectflag[5]=0;
							paths[index]->connectflag[6]=0;
							paths[index]->connectflag[7]=0;
							paths[index]->vecConnectVel[0]=Vector(0,0,0);
							paths[index]->vecConnectVel[1]=Vector(0,0,0);
							paths[index]->vecConnectVel[2]=Vector(0,0,0);
							paths[index]->vecConnectVel[3]=Vector(0,0,0);
							paths[index]->vecConnectVel[4]=Vector(0,0,0);
							paths[index]->vecConnectVel[5]=Vector(0,0,0);
							paths[index]->vecConnectVel[6]=Vector(0,0,0);
							paths[index]->vecConnectVel[7]=Vector(0,0,0);
						}
					}
					else
					{
						paths[index]=new PATH;
						// read the number of paths from this node...
						fread(paths[index], sizeof(PATH), 1, bfp);
					}
				}
				for (index=0; index < g_iNumWaypoints; index++)
				{
					paths[index]->next=paths[index+1];
				}
				paths[index-1]->next=NULL;
				
				g_bWaypointPaths = TRUE;  // keep track so path can be freed
			}
			else
			{
				sprintf(msg, "%s POD Bot waypoints are not for this map!\n", filename);
				sprintf(g_szWaypointMessage, "%s POD Bot waypoints are not for this map!\n", filename);
				ALERT( at_console, msg );
#ifdef __linux__
				printf(msg);
#endif
				fclose(bfp);
				fp=fopen("PODERROR.txt","a"); fprintf(fp, msg);
				fclose(fp);
				return FALSE;
			}
		}
		else
		{
			sprintf(msg, "%s is not a POD Bot waypoint file!\n", filename);
			sprintf(g_szWaypointMessage, "Waypoint file %s does not exist!\n", filename);
			ALERT( at_console, msg );
#ifdef __linux__
			printf(msg);
#endif
			
			fclose(bfp);
			fp=fopen("PODERROR.txt","a"); fprintf(fp, msg);
			fclose(fp);
			return FALSE;
		}
		
		fclose(bfp);
	}
	else
	{
		sprintf(msg, "Waypoint file %s does not exist!\n", filename);
		ALERT( at_console, msg );
		sprintf(g_szWaypointMessage, "Waypoint file %s does not exist!\n"
									 "(you can't add Bots!)\n", filename);
		
#ifdef __linux__
		printf(msg);
#endif
		fp=fopen("PODERROR.txt","a"); fprintf(fp, msg);
		fclose(fp);
		return FALSE;
	}
	sprintf(g_szWaypointMessage, "Waypoints created by:\n"
		"%s", header.creatorname);

	// Reset Display Times for Waypoint Editing
	for (index=0; index < g_iNumWaypoints; index++)
		g_rgfWPDisplayTime[index] = 0.0;
	g_fPathTime=0.0;
	InitWaypointTypes();
	InitPathMatrix();
	InitExperienceTab(pEntity);
	g_iCurrVisIndex = 0;
	g_bRecalcVis = TRUE;
	return TRUE;
}


void WaypointSave(edict_t *pEntity)
{
	char dirname[32];
	char filename[256];
	WAYPOINT_HDR header;
	int i;
	PATH *p;
	
	strcpy(header.filetype, "PODWAY!");
	
	header.waypoint_file_version = WAYPOINT_VERSION;
	
	
	header.number_of_waypoints = g_iNumWaypoints;
	
	memset(header.mapname, 0, sizeof(header.mapname));
	memset(header.creatorname, 0, sizeof(header.creatorname));
	strncpy(header.mapname, STRING(gpGlobals->mapname), 31);
	header.mapname[31] = 0;
	strcpy(header.creatorname,STRING(pEntity->v.netname));
	
#ifndef __linux__
    strcpy(dirname, "cstrike\\PODBot\\");
#else
    strcpy(dirname, "cstrike/PODBot/");
#endif
	strcat(dirname,g_szWPTDirname);

#ifndef __linux__
	strcat(dirname,"\\");
#else
	strcat(dirname,"/");
#endif
	
	strcpy(filename, dirname);
	strcat(filename, header.mapname);
	strcat(filename, ".pwf");
	
	FILE *bfp = fopen(filename, "wb");
	
	// write the waypoint header to the file...
	fwrite(&header, sizeof(header), 1, bfp);
	
	p = paths[0];
	
	// save the waypoint paths...
	for (i=0; i < g_iNumWaypoints; i++)
	{
		fwrite(p, sizeof(PATH), 1, bfp);
		p=p->next;
	}
	fclose(bfp);
}

// Returns 2D Traveltime to a Position
float GetTravelTime(float fMaxSpeed, Vector vecSource,Vector vecPosition)
{
   float fDistance = (vecPosition - vecSource).Length2D();
   return(fDistance / fMaxSpeed);
}

bool WaypointReachable(Vector v_src, Vector v_dest, edict_t *pEntity)
{
   TraceResult tr;

   float distance = (v_dest - v_src).Length();

   // is the destination close enough?
   if (distance < 200)
   {
      // check if this waypoint is "visible"...

      UTIL_TraceLine( v_src, v_dest, ignore_monsters,
                      pEntity->v.pContainingEntity, &tr );

      // if waypoint is visible from current position (even behind head)...
      if (tr.flFraction >= 1.0)
      {
		  if(!IsInWater(pEntity))
		  {

			  // is dest waypoint higher than src? (62 is max jump height)
			  if (v_dest.z > (v_src.z + 62.0))
				  return FALSE;  // can't reach this one
			  // is dest waypoint lower than src?
			  if (v_dest.z < (v_src.z - 100.0))
				  return FALSE;  // can't reach this one
		  }
         return TRUE;
      }
   }

   return FALSE;
}


bool WaypointNodeReachable(Vector v_src, Vector v_dest, edict_t *pEntity)
{
   TraceResult tr;
   float height, last_height;

   float distance = (v_dest - v_src).Length();

   // is the destination close enough?
   if (distance < 256)
   {
      // check if this waypoint is "visible"...

      UTIL_TraceLine( v_src, v_dest, ignore_monsters,
                      pEntity->v.pContainingEntity, &tr );

      // if waypoint is visible from current position (even behind head)...
      if ( (tr.flFraction >= 1.0) || (FStrEq("func_door",STRING(tr.pHit->v.classname))) ||
		   (FStrEq("func_door_rotating",STRING(tr.pHit->v.classname))) )
      {
		  // If it's a door check if nothing blocks behind
		  if((FStrEq("func_door",STRING(tr.pHit->v.classname))) ||
			  (FStrEq("func_door_rotating",STRING(tr.pHit->v.classname))))
		  {
			  Vector vDoorEnd=tr.vecEndPos;
			  UTIL_TraceLine( vDoorEnd, v_dest, ignore_monsters,
				  tr.pHit, &tr);
			  if(tr.flFraction < 1.0)
				  return FALSE;
		  }
			  
         // check for special case of waypoint being suspended in mid-air...

         // is dest waypoint higher than src? (45 is max jump height)
         if (v_dest.z > (v_src.z + 45.0))
         {
            Vector v_new_src = v_dest;
            Vector v_new_dest = v_dest;

            v_new_dest.z = v_new_dest.z - 50;  // straight down 50 units

            UTIL_TraceLine(v_new_src, v_new_dest, ignore_monsters,
                           pEntity->v.pContainingEntity, &tr);

            // check if we didn't hit anything, if not then it's in mid-air
            if (tr.flFraction >= 1.0)
            {
               return FALSE;  // can't reach this one
            }
         }

         // check if distance to ground drops more than step height at points
         // between source and destination...

         Vector v_direction = (v_dest - v_src).Normalize();  // 1 unit long
         Vector v_check = v_src;
         Vector v_down = v_src;

         v_down.z = v_down.z - 1000.0;  // straight down 1000 units

         UTIL_TraceLine(v_check, v_down, ignore_monsters,
                        pEntity->v.pContainingEntity, &tr);

         last_height = tr.flFraction * 1000.0;  // height from ground

         distance = (v_dest - v_check).Length();  // distance from goal

         while (distance > 10.0)
         {
            // move 10 units closer to the goal...
            v_check = v_check + (v_direction * 10.0);

            v_down = v_check;
            v_down.z = v_down.z - 1000.0;  // straight down 1000 units

            UTIL_TraceLine(v_check, v_down, ignore_monsters,
                           pEntity->v.pContainingEntity, &tr);

            height = tr.flFraction * 1000.0;  // height from ground

            // is the current height greater than the step height?
            if (height < (last_height - 18.0))
            {
               // can't get there without jumping...

               return FALSE;
            }

            last_height = height;

            distance = (v_dest - v_check).Length();  // distance from goal
         }

         return TRUE;
      }
   }

   return FALSE;
}




void WaypointCalcVisibility(edict_t *pPlayer)
{
	TraceResult tr;
	unsigned char byRes;
	unsigned char byShift;
	int i;


	Vector vecSourceDuck = paths[g_iCurrVisIndex]->origin;
	Vector vecSourceStand = paths[g_iCurrVisIndex]->origin;
	if(paths[g_iCurrVisIndex]->flags & W_FL_CROUCH)
	{
		vecSourceDuck.z += 12.0;
		vecSourceStand.z += 18.0 + 28.0;
	}
	else
	{
		vecSourceDuck.z += -18.0 + 12.0;
		vecSourceStand.z += 28.0;
	}
	
	Vector vecDest;
	for(i=0;i<g_iNumWaypoints;i++)
	{
		// First check ducked Visibility
		
		vecDest = paths[i]->origin;
		UTIL_TraceLine(vecSourceDuck,vecDest, ignore_monsters,
			NULL, &tr );
		// check if line of sight to object is not blocked (i.e. visible)
		if (tr.flFraction != 1.0)
			byRes = 1;
		else
			byRes = 0;
		byRes <<= 1;
		
		UTIL_TraceLine(vecSourceStand,vecDest, ignore_monsters,
			NULL, &tr );
		// check if line of sight to object is not blocked (i.e. visible)
		if (tr.flFraction != 1.0)
			byRes |= 1;
		
		
		if(byRes != 0)
		{
			vecDest = paths[i]->origin;
			// First check ducked Visibility
			if(paths[i]->flags & W_FL_CROUCH)				
				vecDest.z += 18.0 + 28.0;
			else
				vecDest.z += 28.0;
			UTIL_TraceLine(vecSourceDuck,vecDest, ignore_monsters,
				NULL, &tr );
			// check if line of sight to object is not blocked (i.e. visible)
			if (tr.flFraction != 1.0)
				byRes |= 2;
			else
				byRes &= 1;
			
			UTIL_TraceLine(vecSourceStand,vecDest, ignore_monsters,
				NULL, &tr );
			// check if line of sight to object is not blocked (i.e. visible)
			if (tr.flFraction != 1.0)
				byRes |= 1;
			else
				byRes &= 2;
			
		}
		byShift = (i % 4)<<1;
		g_rgbyVisLUT[g_iCurrVisIndex][i>>2] &= ~(3 << byShift);
		g_rgbyVisLUT[g_iCurrVisIndex][i>>2] |= byRes << byShift; 
	}
	g_iCurrVisIndex++;
	UTIL_DebugMSG("VisTable %d percent complete\n",(g_iCurrVisIndex * 100)/g_iNumWaypoints);
	if(g_iCurrVisIndex == g_iNumWaypoints)
		g_bRecalcVis = FALSE;
}

bool WaypointIsVisible(int iSourceIndex,int iDestIndex)
{
	unsigned char byRes = g_rgbyVisLUT[iSourceIndex][iDestIndex>>2];
	byRes >>= (iDestIndex%4)<<1;
	return(!((byRes & 3) == 3));
}

void WaypointThink(edict_t *pEntity)
{
	float distance, min_distance;
	Vector start, end;
	int i, index;
	char msg[80];

	if(g_iFindWPIndex!=-1)
		WaypointDrawBeam(pEntity, pEntity->v.origin,paths[g_iFindWPIndex]->origin, 10, 2, 0, 255, 0, 200, 10);
	
	if (g_bAutoWaypoint)  // is auto waypoint on?
	{
		// find the distance from the last used waypoint
		distance = (g_vecLastWaypoint - pEntity->v.origin).Length();
		
		if (distance > 128)
		{
			min_distance = 9999.0;
			
			// check that no other reachable waypoints are nearby...
			for (i=0; i < g_iNumWaypoints; i++)
			{
				
				if (WaypointReachable(pEntity->v.origin, paths[i]->origin, pEntity))
				{
					distance = (paths[i]->origin - pEntity->v.origin).Length();
					
					if (distance < min_distance)
						min_distance = distance;
				}
			}
			
			// make sure nearest waypoint is far enough away...
			if (min_distance >= 128)
				WaypointAdd(pEntity,0);  // place a waypoint here
		}
	}
	
	min_distance = 9999.0;
	
	
	for (i=0; i < g_iNumWaypoints; i++)
	{
		distance = (paths[i]->origin - pEntity->v.origin).Length();
		
		if (distance < 500)
		{
            if (distance < min_distance)
            {
				index = i; // store index of nearest waypoint
				min_distance = distance;
            }
			
            /*if ((g_rgfWPDisplayTime[i] + 1.0) < gpGlobals->time)
            {
				if (paths[i]->flags & W_FL_CROUCH)
				{
					start = paths[i]->origin - Vector(0, 0, 17);
					end = start + Vector(0, 0, 34);
				}
				else
				{
					start = paths[i]->origin - Vector(0, 0, 34);
					end = start + Vector(0, 0, 68);
				}
				
				if(paths[i]->flags & W_FL_CROSSING)
				{
					if(paths[i]->flags & W_FL_CAMP)
					{
						int iRGB[3]={0,255,255};

						if(paths[i]->flags & W_FL_TERRORIST)
						{
							iRGB[0]=255;
							iRGB[1]=160;
							iRGB[2]=160;
						}
						else if(paths[i]->flags & W_FL_COUNTER)
						{
							iRGB[1]=160;
							iRGB[0]=255;
						}
						WaypointDrawBeam(pEntity, start, end, 30, 0, iRGB[0], iRGB[1], iRGB[2], 250, 5);
					}
					else if(paths[i]->flags & W_FL_TERRORIST)
						WaypointDrawBeam(pEntity, start, end, 30, 0, 255, 0, 0, 250, 5);
					else if(paths[i]->flags & W_FL_COUNTER)
						WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);
				}
				else if(paths[i]->flags & W_FL_GOAL)
					WaypointDrawBeam(pEntity, start, end, 30, 0, 255, 0, 255, 250, 5);
				else if(paths[i]->flags & W_FL_LADDER)
					WaypointDrawBeam(pEntity, start, end, 30, 0, 255, 0, 255, 250, 5);
				else if(paths[i]->flags & W_FL_RESCUE)
					WaypointDrawBeam(pEntity, start, end, 30, 0, 255, 255, 255, 250, 5);
				else
					WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 255, 0, 250, 5);
				g_rgfWPDisplayTime[i] = gpGlobals->time;
            }*/
		}
	}
	
	if (g_bWaypointOn)
	{
		if(min_distance<=50)
		{
			if(g_fPathTime <= gpGlobals->time)
			{
				if(paths[index]->flags & W_FL_CAMP)
				{
					if (paths[index]->flags & W_FL_CROUCH)
					{
						start = paths[index]->origin - Vector(0, 0, 17) +
							Vector(0,0,34);
					}
					else
					{
						start = paths[index]->origin - Vector(0, 0, 34) + 
							Vector(0,0,68);
					}
					end.x=paths[index]->fcampstartx;
					end.y=paths[index]->fcampstarty;
					end.z=0.0;
					WaypointDrawBeam(pEntity, start, end, 30, 0, 255, 0, 0, 250, 5);
					end.x=paths[index]->fcampendx;
					end.y=paths[index]->fcampendy;
					end.z=0.0;
					WaypointDrawBeam(pEntity, start, end, 30, 0, 255, 0, 0, 250, 5);
				}
		
				// check if player is close enough to a waypoint and time to draw path...
				if (g_bPathWaypoint)
				{
					PATH *p;
					
					g_fPathTime = gpGlobals->time + 1.0;
					
					p = paths[index];
					if (pEntity)
					{
						sprintf(msg, "Index Nr.: %d - Path Node Nr.:%d - Radius: %d\n",index,p->iPathNumber,(int)p->Radius);
						ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, msg);
					}
					
					i = 0;
					
/*					while (i < MAX_PATH_INDEX)
					{
						if (p->index[i] != -1)
						{
							Vector v_src = p->origin;
							Vector v_dest = paths[p->index[i]]->origin;
							
							if(p->connectflag[i] & C_FL_JUMP)
								WaypointDrawBeam(pEntity, v_src, v_dest, 10, 2, 255, 0, 0, 200, 10);
							
							// If 2-way connection
							// draw a yellow line to this index's waypoint
							else if(ConnectedToWaypoint(p->index[i],p->iPathNumber))
								WaypointDrawBeam(pEntity, v_src, v_dest, 10, 2, 255, 255, 0, 200, 10);
							// draw a white line to this index's waypoint
							else
								WaypointDrawBeam(pEntity, v_src, v_dest, 10, 2, 250, 250, 250, 200, 10);
						}
						
						i++;
					}
					start = paths[index]->origin;
					UTIL_MakeVectors(Vector (0,0,0));
					int iDistance=paths[index]->Radius;
					Vector vRadiusEnd=start+(gpGlobals->v_forward*iDistance);
					Vector v_direction = vRadiusEnd - start;
					v_direction = UTIL_VecToAngles(v_direction);
					
					for(float fRadCircle=0.0;fRadCircle<360.0;fRadCircle+=20)
					{
						UTIL_MakeVectors(v_direction);
						vRadiusEnd=start+(gpGlobals->v_forward*iDistance);
						WaypointDrawBeam(pEntity, start, vRadiusEnd,30, 0, 0, 0, 255, 250, 5);
						v_direction.y+=fRadCircle;
						if (v_direction.y > 180)
							v_direction.y -=360;
					}*/
					start = paths[index]->origin - Vector(0, 0, 34);
					end = start + Vector(0, 0, 68);
					WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 255, 0, 250, 5);
					int iCount = 0;
					while(i < g_iNumWaypoints)
					{
						if(WaypointIsVisible(index,i))
						{
							if(iCount < 200)
							{
								WaypointDrawBeam(pEntity, paths[index]->origin, paths[i]->origin,10, 2, 255, 255, 255, 255, 10);
								iCount++;
							}
						}
						i++;
					}
				}
				g_fPathTime = gpGlobals->time + 1.0;
			}


			if ((f_danger_time <= gpGlobals->time) &&
				(g_bDangerDirection))
			{
				f_danger_time = gpGlobals->time + 1.0;
				
				if(UTIL_GetTeam(pEntity)==0)
				{
					if((pBotExperienceData+(index*g_iNumWaypoints)+index)->iTeam0_danger_index!=-1)
					{
						Vector v_src = paths[index]->origin;
						Vector v_dest = paths[(pBotExperienceData+(index*g_iNumWaypoints)+index)->iTeam0_danger_index]->origin;
						// draw a red line to this index's danger point
						WaypointDrawBeam(pEntity, v_src, v_dest, 10, 2, 255, 0, 0, 200, 10);
					}
				}
				else
				{
					if((pBotExperienceData+(index*g_iNumWaypoints)+index)->iTeam1_danger_index!=-1)
					{
						Vector v_src = paths[index]->origin;
						Vector v_dest = paths[(pBotExperienceData+(index*g_iNumWaypoints)+index)->iTeam1_danger_index]->origin;
						// draw a red line to this index's danger point
						WaypointDrawBeam(pEntity, v_src, v_dest, 10, 2, 255, 0, 0, 200, 10);
					}
				}
			}
		}
	}

	if(g_bLearnJumpWaypoint)
	{
		if(!(g_bEndJumpPoint))
		{
			if((pEntity->v.button & IN_JUMP) == IN_JUMP)
			{
				WaypointAdd(pEntity,9);
				g_fTimeJumpStarted=gpGlobals->time;
				g_bEndJumpPoint=TRUE;
			}
			else
			{
				vecLearnVelocity=pEntity->v.velocity;
				vecLearnPos=pEntity->v.origin;
			}
		}
		else if(((pEntity->v.flags & FL_ONGROUND) ||
				(pEntity->v.movetype == MOVETYPE_FLY)) &&
			(g_fTimeJumpStarted+0.1<gpGlobals->time) &&
			(g_bEndJumpPoint))
		{
			WaypointAdd(pEntity,10);
			g_bLearnJumpWaypoint=FALSE;
			g_bEndJumpPoint=FALSE;
			ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, "Observation Check off\n");
            UTIL_SpeechSynth(pEntity,"observation check,of");
		}
	}
}

void DeleteSearchNodes(bot_t *pBot)
{
#ifdef DEBUG
	report_log (LOG_BOTNAV, "DeleteSearchNodes","pNodes=%x",pBot->pWayNodesStart);
#endif

	PATHNODE *Node,*DelNode;

	Node=pBot->pWayNodesStart;
	while(Node!=NULL)
	{
		DelNode=Node->NextNode;
		delete Node;
		Node=DelNode;
	}
	pBot->pWayNodesStart=NULL;
	pBot->pWaypointNodes=NULL;
}

bool WaypointIsConnected(int iNum)
{
	for (int i=0; i < g_iNumWaypoints; i++)
	{
		if(i!=iNum)
		{
			for(int n=0;n<MAX_PATH_INDEX;n++)
			{
				if(paths[i]->index[n]==iNum) 
					return TRUE;
			}
		}
	}
	return FALSE;
}

bool WaypointNodesValid(edict_t *pEntity)
{
	char msg[80];
	int iTPoints=0;
	int iCTPoints=0;
	int iGoalPoints=0;
	int iRescuePoints=0;
	int iConnections;
	int i;


	for (i=0; i < g_iNumWaypoints; i++)
	{
		iConnections=0;

		for(int n=0;n<MAX_PATH_INDEX;n++)
		{
			if(paths[i]->index[n]!=-1)
			{
				iConnections++;
				break;
			}
		}
		if(iConnections==0)
		{
			if(!WaypointIsConnected(i))
			{
				sprintf(msg,"Node %d isn't connected with any other Waypoint !\n",i);
				ClientPrint(VARS(pEntity), HUD_PRINTCONSOLE, msg);
				return FALSE;
			}
		}

		if(paths[i]->iPathNumber!=i)
		{
            sprintf(msg,"Node %d Pathnumber differs from Index !\n",i);
			ClientPrint(VARS(pEntity), HUD_PRINTCONSOLE, msg);
			return FALSE;
		}
		if((paths[i]->next==NULL) && (i!=g_iNumWaypoints-1))
		{
            sprintf(msg,"Node %d not connected !\n",i);
			ClientPrint(VARS(pEntity), HUD_PRINTCONSOLE, msg);
			return FALSE;
		}
		if(paths[i]->flags & W_FL_CAMP)
		{
			if((paths[i]->fcampendx == 0.0) && (paths[i]->fcampendy==0.0))
			{
				sprintf(msg,"Node %d Camp-Endposition not set !\n",i);
				ClientPrint(VARS(pEntity), HUD_PRINTCONSOLE, msg);
			}
		}
		else if(paths[i]->flags & W_FL_TERRORIST)
			iTPoints++;
		else if(paths[i]->flags & W_FL_COUNTER)
			iCTPoints++;
		else if(paths[i]->flags & W_FL_GOAL)
			iGoalPoints++;
		else if(paths[i]->flags & W_FL_RESCUE)
			iRescuePoints++;

		int x = 0;
		
		while (x < MAX_PATH_INDEX)
		{
			if (paths[i]->index[x] != -1)
			{
				if((paths[i]->index[x]>=g_iNumWaypoints) || (paths[i]->index[x]<-1))
				{
					sprintf(msg,"Node %d - Pathindex %d out of Range !\n",i,x);
					ClientPrint(VARS(pEntity), HUD_PRINTCONSOLE, msg);
					return FALSE;
				}
				else if(paths[i]->index[x]==paths[i]->iPathNumber)
				{
					sprintf(msg,"Node %d - Pathindex %d points to itself !\n",i,x);
					ClientPrint(VARS(pEntity), HUD_PRINTCONSOLE, msg);
					return FALSE;
				}
			}
			x++;
		}
	}
	if(g_iMapType==MAP_CS)
	{
		if(iRescuePoints==0)
		{
			ClientPrint(VARS(pEntity), HUD_PRINTCONSOLE,"You didn't set a Rescue Point !\n");
			return FALSE;
		}
	}
	if(iTPoints==0)
	{
		ClientPrint(VARS(pEntity), HUD_PRINTCONSOLE,"You didn't set any Terrorist Point !\n");
		return FALSE;
	}
	else if(iCTPoints==0)
	{
		ClientPrint(VARS(pEntity), HUD_PRINTCONSOLE,"You didn't set any Counter Point !\n");
		return FALSE;
	}
	else if(iGoalPoints==0)
	{
		ClientPrint(VARS(pEntity), HUD_PRINTCONSOLE,"You didn't set any Goal Points !\n");
		return FALSE;
	}

	// Init Floyd Matrix to use Floyd Pathfinder
	InitPathMatrix();

	// Now check if each and every Path is valid
	PATHNODE *pStartNode, *pDelNode;
	bool bPathValid;
	for (i=0; i < g_iNumWaypoints; i++)
	{
		for (int n=0; n < g_iNumWaypoints; n++)
		{
			if(i==n)
				continue;
			pStartNode=BasicFindPath(i,n,&bPathValid);
			while(pStartNode!=NULL)
			{
				pDelNode=pStartNode->NextNode;
				delete pStartNode;
				pStartNode=pDelNode;
			}
			pStartNode=NULL;
			pDelNode=NULL;

			// No path from A to B ?
			if(!bPathValid)
			{
				sprintf(msg,"Path broken from Nr. %d to %d ! One or both of them are unconnected !\n",
					i,n);
				ClientPrint(VARS(pEntity), HUD_PRINTCONSOLE, msg);
				return FALSE;
			}
		}
	}

	return TRUE;
}


void InitPathMatrix()
{
	int i,j,k;
	
	if(g_pFloydDistanceMatrix!=NULL)
	{
		delete []g_pFloydDistanceMatrix;
		g_pFloydDistanceMatrix=NULL;
	}
	if(g_pFloydPathMatrix!=NULL)
	{
		delete []g_pFloydPathMatrix;
		g_pFloydPathMatrix=NULL;
	}
	if(g_pWithHostageDistMatrix!=NULL)
	{
		delete []g_pWithHostageDistMatrix;
		g_pWithHostageDistMatrix=NULL;
	}
	if(g_pWithHostagePathMatrix!=NULL)
	{
		delete []g_pWithHostagePathMatrix;
		g_pWithHostagePathMatrix=NULL;
	}

	g_pFloydDistanceMatrix=new int[g_iNumWaypoints*g_iNumWaypoints];
	g_pFloydPathMatrix=new int[g_iNumWaypoints*g_iNumWaypoints];

	for(i=0;i<g_iNumWaypoints;i++)
	{
		for(j=0;j<g_iNumWaypoints;j++)
		{
			*(g_pFloydDistanceMatrix+(i*g_iNumWaypoints)+j)=999999;
			*(g_pFloydPathMatrix+(i*g_iNumWaypoints)+j)=-1;
		}
	}

	for(i=0;i<g_iNumWaypoints;i++)
	{
			if(paths[i]->index[0]!=-1)
			{
				*(g_pFloydDistanceMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[0]))=paths[i]->distance[0];
				*(g_pFloydPathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[0]))=paths[i]->index[0];
			}
			if(paths[i]->index[1]!=-1)
			{
				*(g_pFloydDistanceMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[1]))=paths[i]->distance[1];
				*(g_pFloydPathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[1]))=paths[i]->index[1];
			}
			if(paths[i]->index[2]!=-1)
			{
				*(g_pFloydDistanceMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[2]))=paths[i]->distance[2];
				*(g_pFloydPathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[2]))=paths[i]->index[2];
			}
			if(paths[i]->index[3]!=-1)
			{
				*(g_pFloydDistanceMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[3]))=paths[i]->distance[3];
				*(g_pFloydPathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[3]))=paths[i]->index[3];
			}
			if(paths[i]->index[4]!=-1)
			{
				*(g_pFloydDistanceMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[4]))=paths[i]->distance[4];
				*(g_pFloydPathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[4]))=paths[i]->index[4];
			}
			if(paths[i]->index[5]!=-1)
			{
				*(g_pFloydDistanceMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[5]))=paths[i]->distance[5];
				*(g_pFloydPathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[5]))=paths[i]->index[5];
			}
			if(paths[i]->index[6]!=-1)
			{
				*(g_pFloydDistanceMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[6]))=paths[i]->distance[6];
				*(g_pFloydPathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[6]))=paths[i]->index[6];
			}
			if(paths[i]->index[7]!=-1)
			{
				*(g_pFloydDistanceMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[7]))=paths[i]->distance[7];
				*(g_pFloydPathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[7]))=paths[i]->index[7];
			}
	}

	for(i=0;i<g_iNumWaypoints;i++)
	{
		*(g_pFloydDistanceMatrix+(i*g_iNumWaypoints)+i)=0;
	}

	for(k=0;k<g_iNumWaypoints;k++)
	{
		for(i=0;i<g_iNumWaypoints;i++)
		{
			for(j=0;j<g_iNumWaypoints;j++)
			{
				if (*(g_pFloydDistanceMatrix+(i*g_iNumWaypoints)+k) + *(g_pFloydDistanceMatrix+(k*g_iNumWaypoints)+j)
					<(*(g_pFloydDistanceMatrix+(i*g_iNumWaypoints)+j)) )
				{
					*(g_pFloydDistanceMatrix+(i*g_iNumWaypoints)+j) = *(g_pFloydDistanceMatrix+(i*g_iNumWaypoints)+k) + *(g_pFloydDistanceMatrix+(k*g_iNumWaypoints)+j);
					*(g_pFloydPathMatrix+(i*g_iNumWaypoints)+j)=*(g_pFloydPathMatrix+(i*g_iNumWaypoints)+k);
				}
			}
		}
	}

	if(g_iMapType==MAP_CS)
	{
		g_pWithHostageDistMatrix=new int[g_iNumWaypoints*g_iNumWaypoints];
		g_pWithHostagePathMatrix=new int[g_iNumWaypoints*g_iNumWaypoints];
		
		for(i=0;i<g_iNumWaypoints;i++)
		{
			for(j=0;j<g_iNumWaypoints;j++)
			{
				*(g_pWithHostageDistMatrix+(i*g_iNumWaypoints)+j)=999999;
				*(g_pWithHostagePathMatrix+(i*g_iNumWaypoints)+j)=-1;
			}
		}
		
		for(i=0;i<g_iNumWaypoints;i++)
		{
			PATH* p;
			if(paths[i]->index[0]!=-1)
			{
				p=paths[paths[i]->index[0]];
				if((p->flags & W_FL_NOHOSTAGE)==0)
				{
					*(g_pWithHostageDistMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[0]))=paths[i]->distance[0];
					*(g_pWithHostagePathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[0]))=paths[i]->index[0];
				}
			}
			if(paths[i]->index[1]!=-1)
			{
				p=paths[paths[i]->index[1]];
				if((p->flags & W_FL_NOHOSTAGE)==0)
				{
					*(g_pWithHostageDistMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[1]))=paths[i]->distance[1];
					*(g_pWithHostagePathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[1]))=paths[i]->index[1];
				}
			}
			if(paths[i]->index[2]!=-1)
			{
				p=paths[paths[i]->index[2]];
				if((p->flags & W_FL_NOHOSTAGE)==0)
				{
					*(g_pWithHostageDistMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[2]))=paths[i]->distance[2];
					*(g_pWithHostagePathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[2]))=paths[i]->index[2];
				}
			}
			if(paths[i]->index[3]!=-1)
			{
				p=paths[paths[i]->index[3]];
				if((p->flags & W_FL_NOHOSTAGE)==0)
				{
					*(g_pWithHostageDistMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[3]))=paths[i]->distance[3];
					*(g_pWithHostagePathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[3]))=paths[i]->index[3];
				}
			}
			if(paths[i]->index[4]!=-1)
			{
				p=paths[paths[i]->index[4]];
				if((p->flags & W_FL_NOHOSTAGE)==0)
				{
					*(g_pWithHostageDistMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[4]))=paths[i]->distance[4];
					*(g_pWithHostagePathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[4]))=paths[i]->index[4];
				}
			}
			if(paths[i]->index[5]!=-1)
			{
				p=paths[paths[i]->index[5]];
				if((p->flags & W_FL_NOHOSTAGE)==0)
				{
					*(g_pWithHostageDistMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[5]))=paths[i]->distance[5];
					*(g_pWithHostagePathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[5]))=paths[i]->index[5];
				}
			}
			if(paths[i]->index[6]!=-1)
			{
				p=paths[paths[i]->index[6]];
				if((p->flags & W_FL_NOHOSTAGE)==0)
				{
					*(g_pWithHostageDistMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[6]))=paths[i]->distance[6];
					*(g_pWithHostagePathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[6]))=paths[i]->index[6];
				}
			}
			if(paths[i]->index[7]!=-1)
			{
				p=paths[paths[i]->index[7]];
				if((p->flags & W_FL_NOHOSTAGE)==0)
				{
					*(g_pWithHostageDistMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[7]))=paths[i]->distance[7];
					*(g_pWithHostagePathMatrix+(paths[i]->iPathNumber*g_iNumWaypoints)+(paths[i]->index[7]))=paths[i]->index[7];
				}
			}
		}
		for(i=0;i<g_iNumWaypoints;i++)
		{
			*(g_pWithHostageDistMatrix+(i*g_iNumWaypoints)+i)=0;
		}
		
		for(k=0;k<g_iNumWaypoints;k++)
		{
			for(i=0;i<g_iNumWaypoints;i++)
			{
				for(j=0;j<g_iNumWaypoints;j++)
				{
					if (*(g_pWithHostageDistMatrix+(i*g_iNumWaypoints)+k) + *(g_pWithHostageDistMatrix+(k*g_iNumWaypoints)+j)
						<(*(g_pWithHostageDistMatrix+(i*g_iNumWaypoints)+j)) )
					{
						*(g_pWithHostageDistMatrix+(i*g_iNumWaypoints)+j) = *(g_pWithHostageDistMatrix+(i*g_iNumWaypoints)+k) + *(g_pWithHostageDistMatrix+(k*g_iNumWaypoints)+j);
						*(g_pWithHostagePathMatrix+(i*g_iNumWaypoints)+j)=*(g_pWithHostagePathMatrix+(i*g_iNumWaypoints)+k);
					}
				}
			}
		}

	}
	// Free up the hostage distance matrix
	delete []g_pWithHostageDistMatrix;
	g_pWithHostageDistMatrix=NULL;
	g_bWaypointsChanged=FALSE;

	g_cKillHistory = 0;
}

int GetPathDistance(int iSourceWaypoint,int iDestWaypoint)
{
	return(*(g_pFloydDistanceMatrix+(iSourceWaypoint*g_iNumWaypoints)+iDestWaypoint));
}


// Return the most distant waypoint which is seen from the Bot to
// the Target and is within iCount
int GetAimingWaypoint(bot_t *pBot,Vector vecTargetPos, int iCount)
{
	if(pBot->curr_wpt_index == -1)
		pBot->curr_wpt_index = WaypointFindNearestToMove(pBot->pEdict->v.origin);
	int iSourceIndex = pBot->curr_wpt_index;
	int iDestIndex = WaypointFindNearestToMove(vecTargetPos);

	int iCurrCount = 0;
	int iBestIndex = iSourceIndex;
	PATHNODE *pStartNode,*pNode;
	pNode=new PATHNODE;
	pNode->iIndex = iDestIndex;
	pNode->NextNode=NULL;
	pStartNode=pNode;
	while (iDestIndex != iSourceIndex)
	{
		iDestIndex = *(g_pFloydPathMatrix+(iDestIndex*g_iNumWaypoints)+iSourceIndex);
		if(iDestIndex < 0)
			break;
		pNode->NextNode=new PATHNODE;
		pNode=pNode->NextNode;
		pNode->iIndex = iDestIndex;
		pNode->NextNode=NULL;
		if(WaypointIsVisible(pBot->curr_wpt_index,iDestIndex))
		{
			iBestIndex = iDestIndex;
			break;
		}
	}
	while(pStartNode!=NULL)
	{
		pNode=pStartNode->NextNode;
		delete pStartNode;
		pStartNode=pNode;
	}

	return(iBestIndex);
}

#ifdef DEBUG

// Shows the Paths from Source to Dest using dotted Lines
// PATH MATRIX HAS TO BE INITIALIZED!!!
void ShowPath(int iSourceIndex,int iDestIndex)
{
	bool bValid;
	PATHNODE* pDelNode;
	PATHNODE* pPath = BasicFindPath(iSourceIndex,iDestIndex,&bValid);
	Vector vecSource,vecDest;
	while(pPath!=NULL)
	{
		pDelNode=pPath->NextNode;
		vecSource = paths[pPath->iIndex]->origin;
		delete pPath;
		pPath = pDelNode;
		if(pDelNode != NULL)
		{
			vecDest = paths[pDelNode->iIndex]->origin;
			MESSAGE_BEGIN(MSG_ALL, SVC_TEMPENTITY);
			WRITE_BYTE( TE_SHOWLINE);
			WRITE_COORD(vecSource.x);
			WRITE_COORD(vecSource.y);
			WRITE_COORD(vecSource.z);
			WRITE_COORD(vecDest.x);
			WRITE_COORD(vecDest.y);
			WRITE_COORD(vecDest.z);
			MESSAGE_END();
		}
	}
}
#endif


PATHNODE* BasicFindPath(int iSourceIndex, int iDestIndex, bool* bValid)
{
	PATHNODE *StartNode,*Node;
	Node=new PATHNODE;
	Node->iIndex=iSourceIndex;
	Node->NextNode=NULL;
	StartNode=Node;
	*bValid=FALSE;

	while (iSourceIndex != iDestIndex)
	{
		iSourceIndex = *(g_pFloydPathMatrix+(iSourceIndex*g_iNumWaypoints)+iDestIndex);
		if(iSourceIndex<0)
			return StartNode;
		Node->NextNode=new PATHNODE;
		Node=Node->NextNode;
		Node->iIndex=iSourceIndex;
		Node->NextNode=NULL;
	}
	*bValid=TRUE;
	return StartNode;
}


PATHNODE* FindPath(bot_t* pBot,int iSourceIndex,int iDestIndex,unsigned char byPathType)
{
	static int iLastFailedIndex;

	if((iDestIndex>g_iNumWaypoints-1) || (iDestIndex<0))
	{
		fp=fopen("PODERROR.txt","a"); fprintf(fp, "Map:%s Findpath Dest invalid->%d!\n",STRING(gpGlobals->mapname),iDestIndex); fclose(fp);
		return NULL;
	}
	else if((iSourceIndex>g_iNumWaypoints-1) || (iSourceIndex<0))
	{
		fp=fopen("PODERROR.txt","a"); fprintf(fp, "Map:%s Findpath Source invalid->%d!\n",STRING(gpGlobals->mapname),iSourceIndex); fclose(fp);
		return NULL;
	}

	if(byPathType)
		return(FindLeastCostPath(pBot->pEdict,iSourceIndex,iDestIndex,byPathType));

	PATHNODE *StartNode,*Node;
	int iStartIndex=iSourceIndex;
	pBot->chosengoal_index=iSourceIndex;
	pBot->f_goal_value=0.0;

	Node=new PATHNODE;
	Node->iIndex=iSourceIndex;
	Node->NextNode=NULL;
	StartNode=Node;

	if(g_iMapType!=MAP_CS)
	{
#ifdef DEBUG
	report_log (LOG_BOTNAV, "FindPath"," (MAP_CS) from %d to %d",iSourceIndex,iDestIndex);
#endif
		// don't care for hostages
		while (iSourceIndex != iDestIndex)
		{
			iSourceIndex = *(g_pFloydPathMatrix+(iSourceIndex*g_iNumWaypoints)+iDestIndex);
			if(iSourceIndex<0)
			{
				pBot->prev_goal_index=-1;
				BotGetSafeTask(pBot)->iData=-1;
				if(iLastFailedIndex != iSourceIndex)
				{
					fp=fopen("PODERROR.txt","a"); fprintf(fp, "Map:%s No Path from %d to %d!\n",STRING(gpGlobals->mapname),iStartIndex,iDestIndex);
					fclose(fp);
				}
				iLastFailedIndex=iSourceIndex;
				return StartNode;
			}
			Node->NextNode=new PATHNODE;
			Node=Node->NextNode;
			Node->iIndex=iSourceIndex;
			Node->NextNode=NULL;
		}
	}
	else
	{
		if(BotHasHostage(pBot))
		{
#ifdef DEBUG
	report_log (LOG_BOTNAV, "FindPath"," (with Hostage) from %d to %d",iSourceIndex,iDestIndex);
#endif
			while (iSourceIndex != iDestIndex)
			{
				iSourceIndex = *(g_pWithHostagePathMatrix+(iSourceIndex*g_iNumWaypoints)+iDestIndex);
				if(iSourceIndex<0)
				{
					pBot->prev_goal_index=-1;
					BotGetSafeTask(pBot)->iData=-1;
					if(iLastFailedIndex!=iSourceIndex)
					{
						fp=fopen("PODERROR.txt","a"); fprintf(fp, "Map:%s No Path from %d to %d! (with Hostage)\n",STRING(gpGlobals->mapname),iStartIndex,iDestIndex);
						fclose(fp);
					}
					iLastFailedIndex=iSourceIndex;
					return StartNode;
				}
				Node->NextNode=new PATHNODE;
				Node=Node->NextNode;
				Node->iIndex=iSourceIndex;
				Node->NextNode=NULL;
			}
		}
		else
		{
#ifdef DEBUG
	report_log (LOG_BOTNAV, "FindPath"," (without Hostage) from %d to %d",iSourceIndex,iDestIndex);
#endif
			while (iSourceIndex != iDestIndex)
			{
				iSourceIndex = *(g_pFloydPathMatrix+(iSourceIndex*g_iNumWaypoints)+iDestIndex);
				if(iSourceIndex<0)
				{
					pBot->prev_goal_index=-1;
					BotGetSafeTask(pBot)->iData=-1;
					if(iLastFailedIndex!=iSourceIndex)
					{
						fp=fopen("PODERROR.txt","a"); fprintf(fp, "Map:%s No Path from %d to %d!\n",STRING(gpGlobals->mapname),iStartIndex,iDestIndex);
						fclose(fp);
					}
					iLastFailedIndex=iSourceIndex;
					return StartNode;
				}

				Node->NextNode=new PATHNODE;
				Node=Node->NextNode;
				Node->iIndex=iSourceIndex;
				Node->NextNode=NULL;
			}
		}
	}
	return StartNode;
}



// Test function to view the calculated A* Path

void TestAPath(edict_t *pPlayer,int iSourceIndex,int iDestIndex,unsigned char byPathType)
{
	PATHNODE *root;
	PATHNODE *path, *p;

	g_iSearchGoalIndex = iDestIndex;
	// allocate and setup the root node
	root = new PATHNODE;
		
	root->iIndex = iSourceIndex;
	root->parent = NULL;
	root->NextNode = NULL;
	root->prev = NULL;
	root->g = 0;
	root->h = hfunctionNone(root);
	root->f = root->g + root->h;
	root->id = 0;
	root->depth = 0;
	root->state = OPEN;

	if(byPathType == 1)
	{
		if(UTIL_GetTeam(pPlayer) == 0)
			path = AStarSearch(root, gfunctionKillsDistT, hfunctionNone, goal, makeChildren, freeNode, nodeEqual);
		else
			path = AStarSearch(root, gfunctionKillsDistCT, hfunctionNone, goal, makeChildren, freeNode, nodeEqual);
	}
	else
	{
		if(UTIL_GetTeam(pPlayer) == 0)
			path = AStarSearch(root, gfunctionKillsT, hfunctionNone, goal, makeChildren, freeNode, nodeEqual);
		else
			path = AStarSearch(root, gfunctionKillsCT, hfunctionNone, goal, makeChildren, freeNode, nodeEqual);
	}
	
	// A* returned failure
	if(path == NULL)
		UTIL_HostPrint(HUD_PRINTCENTER,"NO PATH FOUND!\n");
	
	// otherwise, we had a successful search
	p = path;

	Vector vecSource;
	Vector vecDest;

	while(p != NULL)
	{
		vecSource = paths[p->iIndex]->origin;
		p = (PATHNODE *)p->NextNode;
		if(p != NULL)
		{
			vecDest = paths[p->iIndex]->origin;
			MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, NULL, pPlayer);
			WRITE_BYTE( TE_SHOWLINE);
			WRITE_COORD(vecSource.x);
			WRITE_COORD(vecSource.y);
			WRITE_COORD(vecSource.z);
			WRITE_COORD(vecDest.x);
			WRITE_COORD(vecDest.y);
			WRITE_COORD(vecDest.z);
			MESSAGE_END();
		}
	}
	
	// delete the nodes on the path (which should delete root as well)
	while(path != NULL)
	{
		p = (PATHNODE *)path->NextNode;
		freeNode(path);
		path = p;
	}
	
}


PATHNODE* FindLeastCostPath(edict_t *pPlayer,int iSourceIndex,int iDestIndex,unsigned char byPathType)
{
	PATHNODE *root;
	PATHNODE *path;


	g_iSearchGoalIndex = iDestIndex;
	// allocate and setup the root node
	root = new PATHNODE;
		
	root->iIndex = iSourceIndex;
	root->parent = NULL;
	root->NextNode = NULL;
	root->prev = NULL;
	root->g = 0;
	root->h = hfunctionNone(root);
	root->f = root->g + root->h;
	root->id = 0;
	root->depth = 0;
	root->state = OPEN;

	if(byPathType == 1)
	{
		if(UTIL_GetTeam(pPlayer) == 0)
			path = AStarSearch(root, gfunctionKillsDistT, hfunctionNone, goal, makeChildren, freeNode, nodeEqual);
		else
			path = AStarSearch(root, gfunctionKillsDistCT, hfunctionNone, goal, makeChildren, freeNode, nodeEqual);
	}
	else
	{
		if(UTIL_GetTeam(pPlayer) == 0)
			path = AStarSearch(root, gfunctionKillsT, hfunctionNone, goal, makeChildren, freeNode, nodeEqual);
		else
			path = AStarSearch(root, gfunctionKillsCT, hfunctionNone, goal, makeChildren, freeNode, nodeEqual);
	}

	
	// A* returned failure
	if(path == NULL)
		UTIL_HostPrint(HUD_PRINTCENTER,"NO PATH FOUND!\n");
	
	// otherwise, we had a successful search
	return(path);
}

/* 

 * This is a general A* function, so the user defines all of the
 * necessary function calls for calculating path costs and generating
 * the children of a particular node.  

 * root: Node*, root node from which to begin the search.

 * gcalc: Takes a Node* as argument and returns a double indicating
 * the g value (cost from initial state).

 * hcalc: Takes a Node* as argument and returns a double indicating
 * the h value (estimated cost to the goal).

 * goalNode: Takes a Node* as argument and returns 1 if the node is
 * the goal node, 0 otherwise.

 * children: Takes a Node* as argument and returns a linked list of
 * the children of that node, or NULL if there are none.

 * freeNode: Takes a Node* as argument and frees the memory associated
 * with the node.

 * nodeEqual: Takes two Node* as arguments and returns a 1 if the
 * nodes are equivalent states, 0 otherwise.


 * The function returns a Node*, which is the root node of a linked
 * list (follow the "next" fields) that specifies the path from the
 * root node to the goal node.*/

PATHNODE *AStarSearch(PATHNODE * root, 
		   int (*gcalc) (PATHNODE *), 
		   int (*hcalc) (PATHNODE *), 
		   int (*goalNode) (PATHNODE *), 
		   PATHNODE * (*children) (PATHNODE *), 
		   void (*freeNode) (PATHNODE *), 
		   int (*nodeEqual) (PATHNODE *, PATHNODE *)) 
{	
	PATHNODE*	openList;
	PATHNODE*	closedList;
	PATHNODE*	current;
	PATHNODE*	childList;
	PATHNODE*	curChild;
	PATHNODE	*p, *q;
	PATHNODE*	path;
	static int      gblID = 1;
	static int      gblExpand = 0;
	
	
	// generate the open list
	openList = NULL;
	
	// generate the closed list
	closedList = NULL;
	
	// put the root node on the open list
	root->NextNode = NULL;
	root->prev = NULL;
	openList = root;
	
	while (openList != NULL)
	{
		// remove the first node from the open list, as it will always be sorted
		
		current = openList;
		openList = (PATHNODE *) openList->NextNode;
		if(openList != NULL)
			openList->prev = NULL;
		gblExpand++;
		
		// is the current node the goal node?
		if (goalNode(current))
		{
			
			// build the complete path to return
			current->NextNode = NULL;
			path = current;
			p = (PATHNODE *) current->parent;
			
			//     printf("Goal state reached with %d nodes created and %d nodes expanded\n", gblID, gblExpand);
			
			while (p != NULL)
			{
				
				// remove the parent node from the closed list (where it has to be)
				if (p->prev != NULL)
					((PATHNODE *) p->prev)->NextNode = p->NextNode;
				if (p->NextNode != NULL)
					((PATHNODE *) p->NextNode)->prev = p->prev;
				
				// check if we're romoving the top of the list
				if (p == closedList)
					closedList = (PATHNODE*)p->NextNode;
				
				// set it up in the path
				p->NextNode = path;
				path = p;
				
				p = (PATHNODE *) p->parent;
			}
			
			// now delete all nodes on OPEN
			while (openList != NULL)
			{
				p = (PATHNODE*) openList->NextNode;
				freeNode(openList);
				openList = p;
			}
			
			// now delete all nodes on CLOSED
			while (closedList != NULL)
			{
				p = (PATHNODE*) closedList->NextNode;
				freeNode(closedList);
				closedList = p;
			}
			
			// now return the path
			return (path);
		}
		
		// now expand the current node
		childList = children(current);
		
		// insert the children into the OPEN list according to their f values
		while (childList != NULL)
		{
			curChild = childList;
			childList = (PATHNODE*) childList->NextNode;
			
			// set up the rest of the child node
			curChild->parent = current;
			curChild->state = OPEN;
			curChild->depth = current->depth + 1;
			curChild->id = gblID++;
			curChild->NextNode = NULL;
			curChild->prev = NULL;
			
			// calculate the f value as f = g + h
			curChild->g = gcalc(curChild);
			curChild->h = hcalc(curChild);
			curChild->f = curChild->g + curChild->h;
			
			// test whether the child is in the closed list (already been there)
			if (closedList != NULL)
			{
				p = closedList;
				while (p != NULL)
				{
					if (nodeEqual(p, curChild))
					{	       // if so, check if the f value is lower
						
						if (p->f <= curChild->f)
						{	       // if the f value of the older node is lower, delete the new
							// child
							
							freeNode(curChild);
							curChild = NULL;
							break;
						}
						else
						{			       // the child is a shorter path to this point, delete p from
							// the closed list
							
							if (p->prev != NULL)	       // This works so long as the new child is put in the OPEN
								// list.
								
								((PATHNODE*) p->prev)->NextNode = p->NextNode;	// Another solution is to just update all of the
							
							if (p->NextNode != NULL)	       // descendents of this node with the new f values.
								
								((PATHNODE*) p->NextNode)->prev = p->prev;
							if (p == closedList)
								closedList = (PATHNODE*)p->NextNode;
							freeNode(p);
							break;
						}
					}
					p = (PATHNODE*)p->NextNode;
				}
			}
			
			if (curChild != NULL)
			{
				// check if the child is already on the open list
				p = openList;
				while (p != NULL) {
					if (nodeEqual(p, curChild))
					{	       // child is on the OPEN list
						
						if (p->f <= curChild->f)
						{	       // child is a longer path to the same place so delete it
							
							freeNode(curChild);
							curChild = NULL;
							break;
						}
						else
						{			       // child is a shorter path to the same place
							// remove the duplicate node
							
							if (p->prev != NULL)
								((PATHNODE*) p->prev)->NextNode = p->NextNode;
							if (p->NextNode != NULL)
								((PATHNODE*) p->NextNode)->prev = p->prev;
							if (p == openList)
								openList = (PATHNODE*)p->NextNode;
							break;
						}
					}
					p = (PATHNODE*)p->NextNode;
				}
				
				if (curChild != NULL)
				{
					// now insert the child into the list according to the f values
					p = openList;
					q = p;
					while (p != NULL) {
						if (p->f >= curChild->f)
						{	       // insert before p
							// test head case
							
							if (p == openList)
								openList = curChild;
							
							// insert the node
							curChild->NextNode = p;
							curChild->prev = p->prev;
							p->prev = curChild;
							if (curChild->prev != NULL)
								((PATHNODE*) curChild->prev)->NextNode = curChild;
							break;
						}
						q = p;
						p = (PATHNODE*)p->NextNode;
					}
					if (p == NULL)
					{		       // insert at the end
						
						if (q != NULL)
						{
							q->NextNode = curChild;
							curChild->prev = q;
						}
						else			       // insert at the beginning
							openList = curChild;
					}
				}				       // end if child is not NULL (better duplicate on OPEN list)
				
			}					       // end if child is not NULL (better duplicate on CLOSED list)
			
    }					       // end of child list loop
	
    // put the current node onto the closed list
    current->NextNode = closedList;
    if (closedList != NULL)
		closedList->prev = current;
    closedList = current;
	
    current->prev = NULL;
    current->state = CLOSED;
	
    // Test to see if we have expanded too many nodes without a solution
/*	if (current->id > MAXNODES)
	{
		   //     printf("Expanded more than the maximum allowable nodes. Terminating\n");
		   
		   // delete all nodes on OPEN
		   while (openList != NULL)
		   {
			   p = (PATHNODE*) openList->NextNode;
			   freeNode(openList);
			   openList = p;
		   }
		   
		   // delete all nodes on CLOSED
		   while (closedList != NULL)
		   {
			   p = (PATHNODE*) closedList->NextNode;
			   freeNode(closedList);
			   closedList = p;
		   }
		   
		   return (NULL);
	 }*/
  }					       // end of OPEN loop
  
  // if we got here, then there is no path to the goal
  
  // delete all nodes on CLOSED since OPEN is now empty
  while (closedList != NULL)
  {
	  p = (PATHNODE*) closedList->NextNode;
	  freeNode(closedList);
	  closedList = p;
  }
  
  return (NULL);
}



// Least Kills and Number of Nodes to Goal for a Team
int gfunctionKillsDistT(PATHNODE* p)
{
	if(p == NULL)
		return(65355);
	int iThisIndex = p->iIndex;
	int iCost = (pBotExperienceData+(iThisIndex*g_iNumWaypoints)+iThisIndex)->uTeam0Damage + g_cKillHistory;
	int iNeighbour;
	for(int i=0;i<MAX_PATH_INDEX;i++)
	{
		iNeighbour = paths[iThisIndex]->index[i];
		if(iNeighbour != -1)
			iCost += (pBotExperienceData+(iNeighbour*g_iNumWaypoints)+iNeighbour)->uTeam0Damage;
	}
	return iCost;
}

int gfunctionKillsDistCT(PATHNODE *p)
{
	if(p == NULL)
		return(65355);
	int iThisIndex = p->iIndex;
	int iCost = (pBotExperienceData+(iThisIndex*g_iNumWaypoints)+iThisIndex)->uTeam1Damage +  + g_cKillHistory;
	int iNeighbour;
	for(int i=0;i<MAX_PATH_INDEX;i++)
	{
		iNeighbour = paths[iThisIndex]->index[i];
		if(iNeighbour != -1)
			iCost += (pBotExperienceData+(iNeighbour*g_iNumWaypoints)+iNeighbour)->uTeam1Damage;
	}
	return iCost;
}

// Least Kills to Goal for a Team
int gfunctionKillsT(PATHNODE* p)
{
	if(p == NULL)
		return(65355);
	int iThisIndex = p->iIndex;
	int iCost = (pBotExperienceData+(iThisIndex*g_iNumWaypoints)+iThisIndex)->uTeam0Damage;
	int iNeighbour;
	for(int i=0;i<MAX_PATH_INDEX;i++)
	{
		iNeighbour = paths[iThisIndex]->index[i];
		if(iNeighbour != -1)
			iCost += (pBotExperienceData+(iNeighbour*g_iNumWaypoints)+iNeighbour)->uTeam0Damage;
	}
	return iCost;
}

int gfunctionKillsCT(PATHNODE *p)
{
	if(p == NULL)
		return(65355);
	int iThisIndex = p->iIndex;
	int iCost = (pBotExperienceData+(iThisIndex*g_iNumWaypoints)+iThisIndex)->uTeam1Damage;
	int iNeighbour;
	for(int i=0;i<MAX_PATH_INDEX;i++)
	{
		iNeighbour = paths[iThisIndex]->index[i];
		if(iNeighbour != -1)
			iCost += (pBotExperienceData+(iNeighbour*g_iNumWaypoints)+iNeighbour)->uTeam1Damage;
	}
	return iCost;
}

// No heurist (greedy) !!
int hfunctionNone(PATHNODE *p)
{
	if(p == NULL)
		return(65355);
	return(0);
}
	

// define the goalNode function
int goal(PATHNODE *p)
{
	if(p->iIndex == g_iSearchGoalIndex)
		return(1);
	
	return(0);
}


// define the children function
PATHNODE *makeChildren(PATHNODE *parent)
{
	PATHNODE *p, *q;
	
	// initialize the return list
	q = NULL;
	int iParentIndex = parent->iIndex;

	for(int i=0;i<MAX_PATH_INDEX;i++)
	{
		if(paths[iParentIndex]->index[i] != -1)
		{
			p = new PATHNODE;
			p->iIndex = paths[iParentIndex]->index[i];
			p->parent = parent;
			p->NextNode = q;
			q = p;
		}
	}
	
	return(q);
}

// Free node function
void freeNode(PATHNODE *p)
{
	delete(p);
}

// Test for node equality
int nodeEqual(PATHNODE *a, PATHNODE *b)
{
	if(a == NULL && b == NULL)
		return(1);
	else if(a == NULL)
		return(0);
	else if(b == NULL)
		return(0);

	return(a->iIndex == b->iIndex);
}
