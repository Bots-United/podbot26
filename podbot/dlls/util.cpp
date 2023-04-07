/***
*
*  Copyright (c) 1999, Valve LLC. All rights reserved.
*
*  This product contains software technology licensed from Id 
*  Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*  All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

//
// HPB_bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// util.cpp
//

#include "extdll.h"
#include "util.h"

#include "bot.h"
#include "bot_globals.h"

extern int message_SayText;
extern threat_t ThreatTab[32];
extern bot_t bots[MAXBOTSNUMBER];   // max of 32 bots in a game
// Ptr to Hosting Edict
extern edict_t *pHostEdict;

int gmsgTextMsg = 0;

int gmsgShowMenu = 0;
int gmsgShake = 0;

static FILE *fp;

Vector UTIL_VecToAngles( const Vector &vec )
{
   float rgflVecOut[3];
   VEC_TO_ANGLES(vec, rgflVecOut);
   return Vector(rgflVecOut);
}


// Overloaded to add IGNORE_GLASS
void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr )
{
	TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE) | (ignoreGlass?0x100:0), pentIgnore, ptr );
}


void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
   TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), pentIgnore, ptr );
}

void UTIL_TraceHull( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr )
{
	TRACE_HULL( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), hullNumber, pentIgnore, ptr );
}


void UTIL_MakeVectors( const Vector &vecAngles )
{
   MAKE_VECTORS( vecAngles );
}


edict_t *UTIL_FindEntityInSphere( edict_t *pentStart, const Vector &vecCenter, float flRadius )
{
   edict_t  *pentEntity;

   pentEntity = FIND_ENTITY_IN_SPHERE( pentStart, vecCenter, flRadius);

   if (!FNullEnt(pentEntity))
      return pentEntity;

   return NULL;
}

edict_t *UTIL_FindEntityByString( edict_t *pentStart, const char *szKeyword, const char *szValue )
{
	edict_t	*pentEntity;

	pentEntity = FIND_ENTITY_BY_STRING( pentStart, szKeyword, szValue );

	if (!FNullEnt(pentEntity))
		return pentEntity;
	return NULL;
}

edict_t *UTIL_FindEntityByClassname( edict_t *pentStart, const char *szName )
{
	return UTIL_FindEntityByString( pentStart, "classname", szName );
}

edict_t *UTIL_FindEntityByTarget( edict_t *pentStart, const char *szName )
{
	return UTIL_FindEntityByString( pentStart, "target", szName );
}


edict_t *UTIL_FindEntityByTargetname( edict_t *pentStart, const char *szName )
{
	return UTIL_FindEntityByString( pentStart, "targetname", szName );
}


int UTIL_PointContents( const Vector &vec )
{
   return POINT_CONTENTS(vec);
}

unsigned short FixedUnsigned16( float value, float scale )
{
	int output;

	output = value * scale;
	if ( output < 0 )
		output = 0;
	if ( output > 0xFFFF )
		output = 0xFFFF;

	return (unsigned short)output;
}


short FixedSigned16( float value, float scale )
{
	int output;

	output = value * scale;

	if ( output > 32767 )
		output = 32767;

	if ( output < -32768 )
		output = -32768;

	return (short)output;
}



void UTIL_SetSize( entvars_t *pev, const Vector &vecMin, const Vector &vecMax )
{
   SET_SIZE( ENT(pev), vecMin, vecMax );
}


void UTIL_SetOrigin( entvars_t *pev, const Vector &vecOrigin )
{
   SET_ORIGIN(ENT(pev), vecOrigin );
}


void ClientPrint( entvars_t *client, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
   if (gmsgTextMsg == 0)
      gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );

   MESSAGE_BEGIN( MSG_ONE, gmsgTextMsg, NULL, client );

   WRITE_BYTE( msg_dest );
   WRITE_STRING( msg_name );

   if ( param1 )
      WRITE_STRING( param1 );
   if ( param2 )
      WRITE_STRING( param2 );
   if ( param3 )
      WRITE_STRING( param3 );
   if ( param4 )
      WRITE_STRING( param4 );

   MESSAGE_END();
}


void UTIL_ClientPrintAll( int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
   if (gmsgTextMsg == 0)
      gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );

   MESSAGE_BEGIN( MSG_ALL, gmsgTextMsg );
      WRITE_BYTE( msg_dest );
      WRITE_STRING( msg_name );

      if ( param1 )
         WRITE_STRING( param1 );
      if ( param2 )
         WRITE_STRING( param2 );
      if ( param3 )
         WRITE_STRING( param3 );
      if ( param4 )
         WRITE_STRING( param4 );

   MESSAGE_END();
}

// Modified SDK ScreenShake
void UTIL_ScreenShake( const Vector &center, float amplitude, float frequency, float duration, float radius )
{
// This structure is sent over the net to describe a screen shake event
typedef struct
{
	unsigned short	amplitude;		// FIXED 4.12 amount of shake
	unsigned short 	duration;		// FIXED 4.12 seconds duration
	unsigned short	frequency;		// FIXED 8.8 noise frequency (low frequency is a jerk,high frequency is a rumble)
} ScreenShake;

	int			i;
	float		localAmplitude;
	ScreenShake	shake;

   if (gmsgShake == 0)
      gmsgShake = REG_USER_MSG( "ScreenShake", sizeof(ScreenShake));

    shake.duration = FixedUnsigned16( duration, 1<<12 );		// 4.12 fixed
	shake.frequency = FixedUnsigned16( frequency, 1<<8 );	// 8.8 fixed

	for ( i = 0; i <= gpGlobals->maxClients+1; i++ )
	{
		if((ThreatTab[i].IsAlive==FALSE) || (ThreatTab[i].IsUsed==FALSE))
			continue;
		edict_t *pPlayer = ThreatTab[i].pEdict;

		if (!(pPlayer->v.flags & FL_ONGROUND) )	// Don't shake if not onground
			continue;

		localAmplitude = 0;

		if ( radius <= 0 )
			localAmplitude = amplitude;
		else
		{
			Vector delta = center - pPlayer->v.origin;
			float distance = delta.Length();
	
			// Had to get rid of this falloff - it didn't work well
			if ( distance < radius )
				localAmplitude = amplitude;//radius - distance;
		}
		if ( localAmplitude )
		{
			shake.amplitude = FixedUnsigned16( localAmplitude, 1<<12 );		// 4.12 fixed
			
			MESSAGE_BEGIN( MSG_ONE, gmsgShake, NULL, pPlayer);		// use the magic #1 for "one client"
				
				WRITE_SHORT( shake.amplitude );				// shake amount
				WRITE_SHORT( shake.duration );				// shake lasts this long
				WRITE_SHORT( shake.frequency );				// shake noise frequency

			MESSAGE_END();
		}
	}
}

void UTIL_SayText( const char *pText,bot_t *pBot)
{
	if (message_SayText == 0)
		message_SayText = REG_USER_MSG( "SayText", -1 );
	char szTemp[160];
	char szName[80];
	int i;
	edict_t *pPlayer;
	szTemp[0]=2;
	int entind=ENTINDEX(pBot->pEdict);
	if(IsAlive(pBot->pEdict))
	{
		strcpy(szName,pBot->name);
		for (i = 0; i < gpGlobals->maxClients; i++)
		{
			if((ThreatTab[i].IsAlive==FALSE) || (ThreatTab[i].IsUsed==FALSE))
				continue;
			pPlayer = ThreatTab[i].pEdict;
			MESSAGE_BEGIN( MSG_ONE,message_SayText,NULL,pPlayer);
			WRITE_BYTE(entind);
			sprintf(&szTemp[1],"%s :    %s",szName,pText);  
			WRITE_STRING(&szTemp[0]);
			MESSAGE_END();
		}
	}
	else
	{
		strcpy(szName,pBot->name);
		for (i = 0; i < gpGlobals->maxClients; i++)
		{
			if((ThreatTab[i].IsAlive!=FALSE) || (ThreatTab[i].IsUsed==FALSE))
				continue;
			pPlayer = ThreatTab[i].pEdict;
			MESSAGE_BEGIN( MSG_ONE,message_SayText,NULL,pPlayer);
			WRITE_BYTE(entind);
			sprintf(&szTemp[1],"*DEAD*%s :    %s",szName,pText);  
			WRITE_STRING(&szTemp[0]);
			MESSAGE_END();
		}
	}
}


void UTIL_TeamSayText( const char *pText,bot_t *pBot)
{
	if (message_SayText == 0)
		message_SayText = REG_USER_MSG( "SayText", -1 );
	char szTemp[180];
	char szName[80];
	char szTeam[]="(Counter-Terrorist)";
	int i;

	edict_t *pPlayer;
	szTemp[0]=2;
	int entind=ENTINDEX(pBot->pEdict);

	int BotTeam=UTIL_GetTeam(pBot->pEdict);
	if(BotTeam==0)
		strcpy(szTeam,"(Terrorist)");
	else
		strcpy(szTeam,"(Counter-Terrorist)");

	if(IsAlive(pBot->pEdict))
	{
		strcpy(szName,pBot->name);
		for (i = 0; i < gpGlobals->maxClients; i++)
		{
			if((ThreatTab[i].IsAlive==FALSE) || (ThreatTab[i].IsUsed==FALSE) ||
				(ThreatTab[i].iTeam!=BotTeam))
				continue;
			pPlayer = ThreatTab[i].pEdict;
			MESSAGE_BEGIN( MSG_ONE,message_SayText,NULL,pPlayer);
			WRITE_BYTE(entind);
			sprintf(&szTemp[1],"%s %s :    %s",szTeam,szName,pText);  
			WRITE_STRING(&szTemp[0]);
			MESSAGE_END();
		}
	}
	else
	{
		strcpy(szName,pBot->name);
		for (i = 0; i < gpGlobals->maxClients; i++)
		{
			if((ThreatTab[i].IsAlive!=FALSE) || (ThreatTab[i].IsUsed==FALSE) ||
				(ThreatTab[i].iTeam!=BotTeam))
				continue;
			pPlayer = ThreatTab[i].pEdict;
			MESSAGE_BEGIN( MSG_ONE,message_SayText,NULL,pPlayer);
			WRITE_BYTE(entind);
			sprintf(&szTemp[1],"*DEAD*%s %s :    %s",szTeam,szName,pText);  
			WRITE_STRING(&szTemp[0]);
			MESSAGE_END();
		}
	}
}


#ifdef	DEBUG
edict_t *DBG_EntOfVars( const entvars_t *pev )
{
	if (pev->pContainingEntity != NULL)
		return pev->pContainingEntity;
	ALERT(at_console, "entvars_t pContainingEntity is NULL, calling into engine");
	edict_t* pent = (*g_engfuncs.pfnFindEntityByVars)((entvars_t*)pev);
	if (pent == NULL)
		ALERT(at_console, "DAMN!  Even the engine couldn't FindEntityByVars!");
	((entvars_t *)pev)->pContainingEntity = pent;
	return pent;
}
#endif //DEBUG


// return team number 0 through 3 based what MOD uses for team numbers
int UTIL_GetTeam(edict_t *pEntity)
{
	char *infobuffer;
	char model_name[32];
	
	infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)( pEntity );
	strcpy(model_name, (g_engfuncs.pfnInfoKeyValue(infobuffer, "model")));
	
	if ((strcmp(model_name, "terror") == 0) ||  // Phoenix Connektion
		(strcmp(model_name, "arab") == 0) ||    // L337 Krew
		(strcmp(model_name, "arctic") == 0) ||   // Arctic Avenger
		(strcmp(model_name, "guerilla") == 0))  // Gorilla Warfare
	{
		return 0;
	}
	else if ((strcmp(model_name, "urban") == 0) ||  // Seal Team 6
		(strcmp(model_name, "gsg9") == 0) ||   // German GSG-9
		(strcmp(model_name, "sas") == 0) ||    // UK SAS
		(strcmp(model_name, "gign") == 0) ||   // French GIGN
		(strcmp(model_name, "vip") == 0))      // VIP
	{
		return 1;
	}
	
	return 0;  // return zero if team is unknown
}


int UTIL_GetBotIndex(edict_t *pEdict)
{
	int index;
	
	for (index=0; index <MAXBOTSNUMBER; index++)
	{
		if (bots[index].pEdict == pEdict)
		{
			break;
		}
	}
	
	if (index <MAXBOTSNUMBER)
		return index;
	
	return -1;  // return -1 if edict is not a bot
}


bot_t *UTIL_GetBotPointer(edict_t *pEdict)
{
	int index;
	
	for (index=0; index <MAXBOTSNUMBER; index++)
	{
		if (bots[index].pEdict == pEdict)
		{
			break;
		}
	}
	
	if (index <MAXBOTSNUMBER)
		return (&bots[index]);
	
	return NULL;  // return NULL if edict is not a bot
}


bool IsAlive(edict_t *pEdict)
{
	return ((pEdict->v.deadflag == DEAD_NO) &&
		(pEdict->v.health > 0) && (pEdict->v.movetype != MOVETYPE_NOCLIP));
}


bool FInViewCone(Vector *pOrigin, edict_t *pEdict)
{
	Vector2D vec2LOS;
	float    flDot;
	
	UTIL_MakeVectors ( pEdict->v.v_angle );
	
	vec2LOS = ( *pOrigin - pEdict->v.origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();
	
	flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );
		
	if ( flDot >= cos((pEdict->v.fov/2) * M_PI / 180))
		return TRUE;
	else
		return FALSE;
}

float GetShootingConeDeviation(edict_t *pEdict,Vector *pvecPosition)
{
	Vector vecDir = (*pvecPosition - GetGunPosition(pEdict));
//	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector vecAngle = pEdict->v.v_angle;
//	vecAngle.x = 0;
	UTIL_MakeVectors(vecAngle );
	// He's facing it, he meant it
	return(DotProduct(gpGlobals->v_forward, vecDir ));
}

bool IsShootableBreakable(edict_t *pent)
{
	return(pent->v.impulse == 0
		&& pent->v.takedamage > 0
		&& pent->v.health < 1000.0 
		&& !FBitSet( pent->v.spawnflags, SF_BREAK_TRIGGER_ONLY ) );
}

bool FBoxVisible (edict_t *pEdict, edict_t *pTargetEdict,Vector* pvHit, unsigned char* ucBodyPart)
{
	*ucBodyPart = 0;
	entvars_t *pevLooker=VARS(pEdict);
	entvars_t *pevTarget=VARS(pTargetEdict);

	// don't look through water
	if ((pevLooker->waterlevel != 3 && pevTarget->waterlevel == 3) 
		|| (pevLooker->waterlevel == 3 && pevTarget->waterlevel == 0))
		return FALSE;

	TraceResult tr;
	Vector	vecLookerOrigin = pevLooker->origin + pevLooker->view_ofs;
	Vector vecTarget = pevTarget->origin;
	// Check direct Line to waist
	UTIL_TraceLine(vecLookerOrigin, vecTarget, ignore_monsters, ignore_glass, pEdict, &tr);
	if (tr.flFraction == 1.0)
	{
		*pvHit=tr.vecEndPos;
		*ucBodyPart |= WAIST_VISIBLE;
	}

	// Check direct Line to head
	vecTarget = vecTarget + pevTarget->view_ofs;
	UTIL_TraceLine(vecLookerOrigin, vecTarget, ignore_monsters, ignore_glass, pEdict, &tr);
	if (tr.flFraction == 1.0)
	{
		*pvHit=tr.vecEndPos;
		*ucBodyPart |= HEAD_VISIBLE; 
	}

	if(*ucBodyPart != 0)
		return TRUE;

	// Nothing visible - check randomly other Parts of Body
	for (int i = 0; i < 5; i++)
	{
		Vector vecTarget = pevTarget->origin;
		vecTarget.x += RANDOM_FLOAT( pevTarget->mins.x, pevTarget->maxs.x);
		vecTarget.y += RANDOM_FLOAT( pevTarget->mins.y, pevTarget->maxs.y);
		vecTarget.z += RANDOM_FLOAT( pevTarget->mins.z, pevTarget->maxs.z);

		UTIL_TraceLine(vecLookerOrigin, vecTarget, ignore_monsters, ignore_glass, pEdict, &tr);
		
		if (tr.flFraction == 1.0)
		{
			// Return seen position
			*pvHit=tr.vecEndPos;
			*ucBodyPart |= CUSTOM_VISIBLE; 
			return TRUE;
		}
	}
	return FALSE;
}


bool FVisible( const Vector &vecOrigin, edict_t *pEdict )
{
	TraceResult tr;
	Vector      vecLookerOrigin;
	
	// look through caller's eyes
	vecLookerOrigin = pEdict->v.origin + pEdict->v.view_ofs;
	
	int bInWater = (UTIL_PointContents (vecOrigin) == CONTENTS_WATER);
	int bLookerInWater = (UTIL_PointContents (vecLookerOrigin) == CONTENTS_WATER);
	
	// don't look through water
	if (bInWater != bLookerInWater)
		return FALSE;
	
	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, pEdict, &tr);
	
	if (tr.flFraction != 1.0)
	{
		return FALSE;  // Line of sight is not established
	}
	else
	{
		return TRUE;  // line of sight is valid.
	}
}


Vector GetGunPosition(edict_t *pEdict)
{
	return (pEdict->v.origin + pEdict->v.view_ofs);
}


int UTIL_GetNearestPlayerIndex(Vector vecOrigin)
{
	float fDistance;
	float fMinDistance = 9999.0;
	int index = 0;
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		if(ThreatTab[i].IsAlive==FALSE
		|| ThreatTab[i].IsUsed==FALSE)
			continue;

		fDistance = (ThreatTab[i].pEdict->v.origin - vecOrigin).Length();
		
		if (fDistance < fMinDistance)
		{
			index = i;
			fMinDistance = fDistance;
		}

	}
	return(index);
}


void UTIL_SelectItem(edict_t *pEdict, char *item_name)
{
   FakeClientCommand(pEdict, item_name, NULL, NULL);
}


Vector VecBModelOrigin(edict_t *pEdict)
{
   return pEdict->v.absmin + (pEdict->v.size * 0.5);
}




void UTIL_ShowMenu( edict_t *pEdict, int slots, int displaytime, bool needmore, char *pText )
{
   if (gmsgShowMenu == 0)
      gmsgShowMenu = REG_USER_MSG( "ShowMenu", -1 );

   MESSAGE_BEGIN( MSG_ONE, gmsgShowMenu, NULL, pEdict );

   WRITE_SHORT( slots );
   WRITE_CHAR( displaytime );
   WRITE_BYTE( needmore );
   WRITE_STRING( pText );

   MESSAGE_END();
}

void UTIL_DecalTrace( TraceResult *pTrace, char* pszDecalName )
{
	short entityIndex;
	int index;
	int message;

	index=DECAL_INDEX(pszDecalName);
	if(index<0)
		index = 0;

	if (pTrace->flFraction == 1.0)
		return;

	// Only decal BSP models
	if ( pTrace->pHit )
	{
		edict_t *pHit=pTrace->pHit;

		if(pHit->v.solid == SOLID_BSP || pHit->v.movetype == MOVETYPE_PUSHSTEP)
			entityIndex = ENTINDEX(pHit );
		else
			return;
	}
	else 
		entityIndex = 0;

	message = TE_DECAL;
	if ( entityIndex != 0 )
	{
		if ( index > 255 )
		{
			message = TE_DECALHIGH;
			index -= 256;
		}
	}
	else
	{
		message = TE_WORLDDECAL;
		if ( index > 255 )
		{
			message = TE_WORLDDECALHIGH;
			index -= 256;
		}
	}
	
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( message );
		WRITE_COORD( pTrace->vecEndPos.x );
		WRITE_COORD( pTrace->vecEndPos.y );
		WRITE_COORD( pTrace->vecEndPos.z );
		WRITE_BYTE( index );
		if ( entityIndex )
			WRITE_SHORT( entityIndex );
	MESSAGE_END();
}

void UTIL_SpeechSynth(edict_t *pEdict,char* szMessage)
{
	if(!g_bUseSpeech)
		return;

	char szSpeak[128];

	sprintf(szSpeak,"speak \"%s\"\n",szMessage);
	SERVER_COMMAND(szSpeak);
}



void UTIL_HostPrint(int msg_dest,char *pszMessage)
{
	if(pHostEdict)
		ClientPrint(VARS(pHostEdict), msg_dest, pszMessage);
	else
	{
		if (IS_DEDICATED_SERVER())
			printf(pszMessage);
		else
			ALERT( at_console, pszMessage );
	}
}

void UTIL_DebugMSG(const char *fmt, ...)
{
	va_list va_alist;
	char msg[256];

	va_start (va_alist, fmt);
	_vsnprintf (msg, sizeof(msg), fmt, va_alist);
	va_end (va_alist);

	UTIL_HostPrint(HUD_PRINTNOTIFY,msg);
}

void UTIL_ClampAngle(float* fAngle)
{
/*	char cSign;
	int iAngle = static_cast<int>(*fAngle);
	if(iAngle != 0)
	{
		if(iAngle < 0)
			cSign = -1;
		else
			cSign = 1;
		
		iAngle %= 179;
		if(iAngle == 0)
			iAngle = 179 * -cSign;
	}
	*fAngle = static_cast<float>(iAngle);*/
	if(*fAngle > 180.0)
		*fAngle -= 360.0;
	if(*fAngle < -180.0)
		*fAngle += 360.0;
}

void UTIL_ClampVector(Vector *vecAngles)
{
	/*
	char cSign;
	int iAngle = static_cast<int>(vecAngles->x);
	if(iAngle != 0)
	{
		if(iAngle < 0)
			cSign = -1;
		else
			cSign = 1;
		
		iAngle %= 179;
		if(iAngle == 0)
			iAngle = 179 * -cSign;
	}
	vecAngles->x = static_cast<float>(iAngle);

	iAngle = static_cast<int>(vecAngles->y);
	if(iAngle != 0)
	{
		if(iAngle < 0)
			cSign = -1;
		else
			cSign = 1;
		
		iAngle %= 179;
		if(iAngle == 0)
			iAngle = 179 * -cSign;
	}
	vecAngles->y = static_cast<float>(iAngle);

	iAngle = static_cast<int>(vecAngles->z);
	if(iAngle != 0)
	{
		if(iAngle < 0)
			cSign = -1;
		else
			cSign = 1;
		
		iAngle %= 179;
		if(iAngle == 0)
			iAngle = 179 * -cSign;
	}
	vecAngles->z = static_cast<float>(iAngle);*/

	if(vecAngles->x > 180.0)
		vecAngles->x -= 360.0;
	if(vecAngles->x < -180.0)
		vecAngles->x += 360.0;
	if(vecAngles->y > 180.0)
		vecAngles->y -= 360.0;
	if(vecAngles->y < -180.0)
		vecAngles->y += 360.0;
	vecAngles->z = 0.0;
}
