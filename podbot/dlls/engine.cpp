//
// engine.cpp
//
// Does the major work of calling the original
// Engine Functions
// 

#include "extdll.h"
#include "util.h"

#include "bot.h"
#include "bot_client.h"
#include "bot_sounds.h"
#include "log.h"
#include "waypoint.h"
#include "bot_globals.h"


void (*botMsgFunction)(void *, int) = NULL;

extern bot_t bots[MAXBOTSNUMBER];   // max of 32 bots in a game
extern Vector g_vecBomb;


int botMsgIndex;


// messages created in RegUserMsg which will be "caught"
int message_VGUIMenu = 0;
int message_ShowMenu = 0;
int message_WeaponList = 0;
int message_CurWeapon = 0;
int message_AmmoX = 0;
int message_WeapPickup = 0;
int message_AmmoPickup = 0;
int message_ItemPickup = 0;
int message_Health = 0;
int message_Battery = 0;  // Armor
int message_Damage = 0;
int message_Money = 0;
int message_StatusIcon = 0;	// for buyzone,rescue,bombzone  
int message_SayText = 0;  
int message_MOTD = 0;  
int message_DeathMsg = 0;
int message_ScreenFade = 0;  
int message_TeamScore = 0;  

// Holds the Bot Index for MessageEnd
//int iMessageLoopIndex;


static FILE *fp;


int pfnPrecacheModel(char* s)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "PrecacheModel","model=%s",s);
#endif
   return (*g_engfuncs.pfnPrecacheModel)(s);
}
int pfnPrecacheSound(char* s)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "PrecacheSound","name=%s",s);
#endif
   return (*g_engfuncs.pfnPrecacheSound)(s);
}
void pfnSetModel(edict_t *e, const char *m)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "SetModel","edict=%x model=%s",e,m);
#endif
   (*g_engfuncs.pfnSetModel)(e, m);
}
int pfnModelIndex(const char *m)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "ModelIndex","model=%s",m);
#endif
   return (*g_engfuncs.pfnModelIndex)(m);
}
int pfnModelFrames(int modelIndex)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "ModelFrames","index=%d",modelIndex);
#endif
   return (*g_engfuncs.pfnModelFrames)(modelIndex);
}

void pfnSetSize(edict_t *e, const float *rgflMin, const float *rgflMax)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "SetSize","ent=%x min=%f max=%f",e,rgflMin,rgflMax);
#endif
   (*g_engfuncs.pfnSetSize)(e, rgflMin, rgflMax);
}
void pfnChangeLevel(char* s1, char* s2)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "ChangeLevel","%s %s",s1,s2);
#endif

   // kick any bot off of the server after time/frag limit...
   for (int index = 0; index <MAXBOTSNUMBER; index++)
   {
      if (bots[index].is_used)  // is this slot used?
      {
         char cmd[40];

         sprintf(cmd, "kick \"%s\"\n", bots[index].name);
         SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
         bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
      }
   }
   
   // Save collected Experience on Map Change
   SaveExperienceTab(NULL);

   (*g_engfuncs.pfnChangeLevel)(s1, s2);
}
void pfnGetSpawnParms(edict_t *ent)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "GetSpawnParams","ent=%x",ent);
#endif
   (*g_engfuncs.pfnGetSpawnParms)(ent);
}
void pfnSaveSpawnParms(edict_t *ent)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "SaveSpawnParams","ent=%x",ent);
#endif
   (*g_engfuncs.pfnSaveSpawnParms)(ent);
}
float pfnVecToYaw(const float *rgflVector)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "VecToYaw",NULL);
#endif
   return (*g_engfuncs.pfnVecToYaw)(rgflVector);
}
void pfnVecToAngles(const float *rgflVectorIn, float *rgflVectorOut)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "VecToAngles",NULL);
#endif
   (*g_engfuncs.pfnVecToAngles)(rgflVectorIn, rgflVectorOut);
}
void pfnMoveToOrigin(edict_t *ent, const float *pflGoal, float dist, int iMoveType)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "MoveToOrigin",NULL);
#endif
   (*g_engfuncs.pfnMoveToOrigin)(ent, pflGoal, dist, iMoveType);
}

void pfnChangeYaw(edict_t* ent)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "ChangeYaw",NULL);
#endif
   (*g_engfuncs.pfnChangeYaw)(ent);
}

void pfnChangePitch(edict_t* ent)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "ChangePitch",NULL);
#endif
   (*g_engfuncs.pfnChangePitch)(ent);
}
edict_t* pfnFindEntityByString(edict_t *pEdictStartSearchAfter, const char *pszField, const char *pszValue)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "FindEntityByString","name=%s",pszValue);
#endif
	// info_map_paramters signals the beginning of a round
	// Credits go to Stefan Hendriks for pointing this out !
	if(FStrEq(pszValue, "info_map_parameters"))
	{
#ifdef DEBUG
		report_log (LOG_BOTBASE, "FindEntityByString","New Round detected ! Resetting Variables..." );
#endif
		
		// Clear the Array of Bots
		int i;
		bot_t *pBot;
		for(i=0;i<MAXBOTSNUMBER;i++)
		{
			pBot = &bots[i];
			if(pBot->is_used==TRUE)
			{
				pBot->need_to_initialize = TRUE;
			}
			iRadioSelect[i]=0;
		}

		g_bBombPlanted = FALSE;
		g_bBombSayString = FALSE;
		g_fTimeBombPlanted = 0.0;
		g_vecBomb = Vector(0,0,0);
		// Clear Waypoint Indices of visited Bomb Spots
		for(i=0;i<MAXNUMBOMBSPOTS;i++)
			g_rgiBombSpotsVisited[i] = -1;
		g_iLastBombPoint = -1;
		g_fTimeNextBombUpdate = 0.0;
		
		g_bLeaderChosenT = FALSE;
		g_bLeaderChosenCT = FALSE;

		g_bHostageRescued = FALSE;
		g_rgfLastRadioTime[0] = 0.0;
		g_rgfLastRadioTime[1] = 0.0;
		g_bBotsCanPause = FALSE;

		// Clear Array of Player Stats
		for (i = 0; i < gpGlobals->maxClients;i++)
		{
/*			ThreatTab[i].IsUsed = FALSE;
			ThreatTab[i].IsAlive = FALSE;
			ThreatTab[i].pEdict = NULL;*/
			ThreatTab[i].vecSoundPosition = Vector(0,0,0);
			ThreatTab[i].fHearingDistance = 0.0;
			ThreatTab[i].fTimeSoundLasting = 0.0;
		}

		// Update Experience Data on Round Start
		UpdateGlobalExperienceData();
		// Calculate the Round Mid/End in World Time
		g_fTimeRoundEnd = CVAR_GET_FLOAT("mp_roundtime");
		g_fTimeRoundEnd*=60;
		g_fTimeRoundEnd+= CVAR_GET_FLOAT("mp_freezetime");
		g_fTimeRoundMid = g_fTimeRoundEnd/2;
		g_fTimeRoundEnd+=gpGlobals->time;
		g_fTimeRoundMid+=gpGlobals->time;
	}
	return (*g_engfuncs.pfnFindEntityByString)(pEdictStartSearchAfter, pszField, pszValue);
}


int pfnGetEntityIllum(edict_t* pEnt)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "GetEntityIllum",NULL);
#endif
   return (*g_engfuncs.pfnGetEntityIllum)(pEnt);
}
edict_t* pfnFindEntityInSphere(edict_t *pEdictStartSearchAfter, const float *org, float rad)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "FindEntityInSphere",NULL);
#endif
   return (*g_engfuncs.pfnFindEntityInSphere)(pEdictStartSearchAfter, org, rad);
}
edict_t* pfnFindClientInPVS(edict_t *pEdict)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "FindClientInPVS",NULL);
#endif
   return (*g_engfuncs.pfnFindClientInPVS)(pEdict);
}
edict_t* pfnEntitiesInPVS(edict_t *pplayer)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "EntitiesInPVS",NULL);
#endif
   return (*g_engfuncs.pfnEntitiesInPVS)(pplayer);
}
void pfnMakeVectors(const float *rgflVector)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "MakeVectors",NULL);
#endif
   (*g_engfuncs.pfnMakeVectors)(rgflVector);
}
void pfnAngleVectors(const float *rgflVector, float *forward, float *right, float *up)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "AngleVectors",NULL);
#endif
   (*g_engfuncs.pfnAngleVectors)(rgflVector, forward, right, up);
}
edict_t* pfnCreateEntity(void)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CreateEntity",NULL);
#endif
   return (*g_engfuncs.pfnCreateEntity)();
}
void pfnRemoveEntity(edict_t* e)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "RemoveEntity",NULL);
#endif
   (*g_engfuncs.pfnRemoveEntity)(e);
}
edict_t* pfnCreateNamedEntity(int className)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CreateNamedEntity",NULL);
#endif
   return (*g_engfuncs.pfnCreateNamedEntity)(className);
}
void pfnMakeStatic(edict_t *ent)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "MakeStatic",NULL);
#endif
   (*g_engfuncs.pfnMakeStatic)(ent);
}
int pfnEntIsOnFloor(edict_t *e)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "EntIsOnFloor",NULL);
#endif
   return (*g_engfuncs.pfnEntIsOnFloor)(e);
}
int pfnDropToFloor(edict_t* e)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "DropToFloor",NULL);
#endif
   return (*g_engfuncs.pfnDropToFloor)(e);
}
int pfnWalkMove(edict_t *ent, float yaw, float dist, int iMode)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "WalkMove",NULL);
#endif
   return (*g_engfuncs.pfnWalkMove)(ent, yaw, dist, iMode);
}
void pfnSetOrigin(edict_t *e, const float *rgflOrigin)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "SetOrigin",NULL);
#endif
   (*g_engfuncs.pfnSetOrigin)(e, rgflOrigin);
}
void pfnEmitSound(edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "EmitSound","ent=%x name=%s volume=%f",entity,sample,volume);
#endif
	SoundAttachToThreat(entity,sample,volume);
   (*g_engfuncs.pfnEmitSound)(entity, channel, sample, volume, attenuation, fFlags, pitch);
}
void pfnEmitAmbientSound(edict_t *entity, float *pos, const char *samp, float vol, float attenuation, int fFlags, int pitch)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "EmitAmbientSound","ent=%x name=%s volume=%f",entity,samp,vol);
#endif
   (*g_engfuncs.pfnEmitAmbientSound)(entity, pos, samp, vol, attenuation, fFlags, pitch);
}
void pfnTraceLine(const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr)
{
   (*g_engfuncs.pfnTraceLine)(v1, v2, fNoMonsters, pentToSkip, ptr);
}
void pfnTraceToss(edict_t* pent, edict_t* pentToIgnore, TraceResult *ptr)
{
   (*g_engfuncs.pfnTraceToss)(pent, pentToIgnore, ptr);
}
int pfnTraceMonsterHull(edict_t *pEdict, const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr)
{
   return (*g_engfuncs.pfnTraceMonsterHull)(pEdict, v1, v2, fNoMonsters, pentToSkip, ptr);
}
void pfnTraceHull(const float *v1, const float *v2, int fNoMonsters, int hullNumber, edict_t *pentToSkip, TraceResult *ptr)
{
   (*g_engfuncs.pfnTraceHull)(v1, v2, fNoMonsters, hullNumber, pentToSkip, ptr);
}
void pfnTraceModel(const float *v1, const float *v2, int hullNumber, edict_t *pent, TraceResult *ptr)
{
   (*g_engfuncs.pfnTraceModel)(v1, v2, hullNumber, pent, ptr);
}
const char *pfnTraceTexture(edict_t *pTextureEntity, const float *v1, const float *v2 )
{
   return (*g_engfuncs.pfnTraceTexture)(pTextureEntity, v1, v2);
}
void pfnTraceSphere(const float *v1, const float *v2, int fNoMonsters, float radius, edict_t *pentToSkip, TraceResult *ptr)
{
   (*g_engfuncs.pfnTraceSphere)(v1, v2, fNoMonsters, radius, pentToSkip, ptr);
}
void pfnGetAimVector(edict_t* ent, float speed, float *rgflReturn)
{
   (*g_engfuncs.pfnGetAimVector)(ent, speed, rgflReturn);
}
void pfnServerCommand(char* str)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "ServerCommand","cmd=%s",str);
#endif
   (*g_engfuncs.pfnServerCommand)(str);
}
void pfnServerExecute(void)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "ServerExecute",NULL);
#endif
   (*g_engfuncs.pfnServerExecute)();
}
void pfnClientCommand(edict_t* pEdict, char* szFmt, ...)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "ClientCommand","szFmt=%s",szFmt);
#endif
   return;
}

void pfnParticleEffect(const float *org, const float *dir, float color, float count)
{
   (*g_engfuncs.pfnParticleEffect)(org, dir, color, count);
}
void pfnLightStyle(int style, char* val)
{
   (*g_engfuncs.pfnLightStyle)(style, val);
}
int pfnDecalIndex(const char *name)
{
   return (*g_engfuncs.pfnDecalIndex)(name);
}
int pfnPointContents(const float *rgflVector)
{
   return (*g_engfuncs.pfnPointContents)(rgflVector);
}

// Called each Time a Message is about to sent 
void pfnMessageBegin(int msg_dest, int msg_type, const float *pOrigin, edict_t *ed)
{
	state=0;
	if (gpGlobals->deathmatch)
	{
		int index = -1;
		
		// Bot involved ?
		if (ed && FBitSet(ed->v.flags, FL_FAKECLIENT))
		{
#ifdef DEBUG
			report_log (LOG_ENGINE, "MessageBegin","ent=%x dest=%d type=%d",ed,msg_dest,msg_type);
#endif
			index = UTIL_GetBotIndex(ed);
			//		 iMessageLoopIndex=index;
			
			// is this message for a bot?
			if (index != -1)
			{
				botMsgFunction = NULL;  // no msg function until known otherwise
				botMsgIndex = index;    // index of bot receiving message

				// Message handling is done in bot_client.cpp
				if (msg_type == message_VGUIMenu)
					botMsgFunction = BotClient_CS_VGUI;
				else if (msg_type == message_ShowMenu)
					botMsgFunction = BotClient_CS_ShowMenu;
				else if (msg_type == message_WeaponList)
					botMsgFunction = BotClient_CS_WeaponList;
				else if (msg_type == message_CurWeapon)
					botMsgFunction = BotClient_CS_CurrentWeapon;
				else if (msg_type == message_AmmoX)
					botMsgFunction = BotClient_CS_AmmoX;
				else if (msg_type == message_WeapPickup)
					botMsgFunction = BotClient_CS_WeaponPickup;
				else if (msg_type == message_AmmoPickup)
					botMsgFunction = BotClient_CS_AmmoPickup;
				else if (msg_type == message_WeapPickup)
					botMsgFunction = BotClient_CS_WeaponPickup;
				else if (msg_type == message_ItemPickup)
					botMsgFunction = BotClient_CS_ItemPickup;
				else if (msg_type == message_Health)
					botMsgFunction = BotClient_CS_Health;
				else if (msg_type == message_Battery)
					botMsgFunction = BotClient_CS_Battery;
				else if (msg_type == message_Damage)
					botMsgFunction = BotClient_CS_Damage;
				else if (msg_type == message_Money)
					botMsgFunction = BotClient_CS_Money;
				else if (msg_type == message_StatusIcon)
					botMsgFunction = BotClient_CS_StatusIcon;
				else if (msg_type == message_ScreenFade)
					botMsgFunction = BotClient_CS_ScreenFade;
				else if (msg_type == message_SayText)
					botMsgFunction = BotClient_CS_SayText;
				else if (msg_type == message_MOTD)
					botMsgFunction = BotClient_CS_MOTD;
/*				else if (msg_type == message_TeamScore)
					botMsgFunction = BotClient_CS_TeamScore;*/
			}

			
		}
		// Show the Waypoint copyright Message right at the MOTD
		else if(msg_type == message_MOTD)
		{
			g_bMapInitialised = TRUE;
			g_bNeedHUDDisplay = TRUE;
		}
		else if (msg_dest == MSG_ALL)
		{
			botMsgFunction = NULL;  // no msg function until known otherwise
			botMsgIndex = -1;       // index of bot receiving message (none)
			
			if (msg_type == message_DeathMsg)
				botMsgFunction = BotClient_CS_DeathMsg;
			else if(msg_type == SVC_INTERMISSION)
			{
				bot_t *pBot;
				
				for(int i=0;i<MAXBOTSNUMBER;i++)
				{
					pBot = &bots[i];
					if((pBot->is_used == TRUE))
						pBot->bDead = TRUE;
				}
			}
		}
		
	}
	
	(*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ed);
}

void pfnMessageEnd(void)
{
	if (gpGlobals->deathmatch)
	{
#ifdef DEBUG
	report_log (LOG_ENGINE, "MessageEnd",NULL);
#endif
		
//		if(iMessageLoopIndex!=-1)
//			bots[iMessageLoopIndex].bInMessageLoop=FALSE;
	
		state=0;
		botMsgFunction = NULL;
	}
	
	(*g_engfuncs.pfnMessageEnd)();
}

void pfnWriteByte(int iValue)
{
   if (gpGlobals->deathmatch)
   {
#ifdef DEBUG
	report_log (LOG_ENGINE, "WriteByte","iValue=%d",iValue);
#endif

      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteByte)(iValue);
}

void pfnWriteChar(int iValue)
{
   if (gpGlobals->deathmatch)
   {
#ifdef DEBUG
	report_log (LOG_ENGINE, "WriteChar","iValue=%d",iValue);
#endif

      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteChar)(iValue);
}

void pfnWriteShort(int iValue)
{
   if (gpGlobals->deathmatch)
   {
#ifdef DEBUG
	report_log (LOG_ENGINE, "WriteShort","iValue=%d",iValue);
#endif

      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteShort)(iValue);
}

void pfnWriteLong(int iValue)
{
   if (gpGlobals->deathmatch)
   {
#ifdef DEBUG
	report_log (LOG_ENGINE, "WriteLong","iValue=%d",iValue);
#endif

      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteLong)(iValue);
}

void pfnWriteAngle(float flValue)
{
   if (gpGlobals->deathmatch)
   {
#ifdef DEBUG
	report_log (LOG_ENGINE, "WriteAngle","flValue=%f",flValue);
#endif

      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&flValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteAngle)(flValue);
}

void pfnWriteCoord(float flValue)
{
   if (gpGlobals->deathmatch)
   {
#ifdef DEBUG
	report_log (LOG_ENGINE, "WriteCoord","flValue=%f",flValue);
#endif

      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&flValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteCoord)(flValue);
}

void pfnWriteString(const char *sz)
{
	if (gpGlobals->deathmatch)
	{
#ifdef DEBUG
	report_log (LOG_ENGINE, "WriteString","%s",sz);
#endif
	// Check if it's the "Bomb Planted" Message
		if (FStrEq(sz,"%!MRAD_BOMBPL"))
		{
			if(!g_bBombPlanted)
			{
				g_bBombPlanted = g_bBombSayString = TRUE;
				g_fTimeBombPlanted = gpGlobals->time;
			}
		}
		
		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)((void *)sz, botMsgIndex);
	}
	
	(*g_engfuncs.pfnWriteString)(sz);
}

void pfnWriteEntity(int iValue)
{
   if (gpGlobals->deathmatch)
   {
#ifdef DEBUG
	report_log (LOG_ENGINE, "WriteEntity","iValue=%x",iValue);
#endif

      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteEntity)(iValue);
}

void pfnCVarRegister(cvar_t *pCvar)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CVarRegister",NULL);
#endif
   (*g_engfuncs.pfnCVarRegister)(pCvar);
}
float pfnCVarGetFloat(const char *szVarName)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CVarGetFloat","CVAR=%s",szVarName);
#endif
   return (*g_engfuncs.pfnCVarGetFloat)(szVarName);
}
const char* pfnCVarGetString(const char *szVarName)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CVarGetString","CVAR=%s",szVarName);
#endif
   return (*g_engfuncs.pfnCVarGetString)(szVarName);
}
void pfnCVarSetFloat(const char *szVarName, float flValue)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CVarSetFloat","CVAR=%s Value=%f",szVarName,flValue);
#endif
   (*g_engfuncs.pfnCVarSetFloat)(szVarName, flValue);
}
void pfnCVarSetString(const char *szVarName, const char *szValue)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CVarSetString","CVAR=%s Value=%s",szVarName,szValue);
#endif
   (*g_engfuncs.pfnCVarSetString)(szVarName, szValue);
}
void* pfnPvAllocEntPrivateData(edict_t *pEdict, long cb)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "PvAllocEntPrivateData",NULL);
#endif
   return (*g_engfuncs.pfnPvAllocEntPrivateData)(pEdict, cb);
}
void* pfnPvEntPrivateData(edict_t *pEdict)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "PvEntPrivateData",NULL);
#endif
   return (*g_engfuncs.pfnPvEntPrivateData)(pEdict);
}
void pfnFreeEntPrivateData(edict_t *pEdict)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "FreeEntPrivateData",NULL);
#endif
   (*g_engfuncs.pfnFreeEntPrivateData)(pEdict);
}
const char* pfnSzFromIndex(int iString)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "SzFromIndex",NULL);
#endif
   return (*g_engfuncs.pfnSzFromIndex)(iString);
}
int pfnAllocString(const char *szValue)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "AllocString",NULL);
#endif
   return (*g_engfuncs.pfnAllocString)(szValue);
}
entvars_t* pfnGetVarsOfEnt(edict_t *pEdict)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "GetVarsOfEnt",NULL);
#endif
   return (*g_engfuncs.pfnGetVarsOfEnt)(pEdict);
}
edict_t* pfnPEntityOfEntOffset(int iEntOffset)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "PEntityOfEntOffset",NULL);
#endif
   return (*g_engfuncs.pfnPEntityOfEntOffset)(iEntOffset);
}
int pfnEntOffsetOfPEntity(const edict_t *pEdict)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "EntOffsetOfPEntity",NULL);
#endif
   return (*g_engfuncs.pfnEntOffsetOfPEntity)(pEdict);
}
int pfnIndexOfEdict(const edict_t *pEdict)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "IndexOfEdict",NULL);
#endif
   return (*g_engfuncs.pfnIndexOfEdict)(pEdict);
}
edict_t* pfnPEntityOfEntIndex(int iEntIndex)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "PEntityOfEntIndex",NULL);
#endif
   return (*g_engfuncs.pfnPEntityOfEntIndex)(iEntIndex);
}
edict_t* pfnFindEntityByVars(entvars_t* pvars)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "FindEntityByVars",NULL);
#endif
   return (*g_engfuncs.pfnFindEntityByVars)(pvars);
}
void* pfnGetModelPtr(edict_t* pEdict)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "GetModelPtr",NULL);
#endif
   return (*g_engfuncs.pfnGetModelPtr)(pEdict);
}

// Sends the Index of a Message
int pfnRegUserMsg(const char *pszName, int iSize)
{
   int msg;

   msg = (*g_engfuncs.pfnRegUserMsg)(pszName, iSize);

   if (gpGlobals->deathmatch)
   {
#ifdef DEBUG
	report_log (LOG_ENGINE, "RegUserMsg","name=%s msg=%d",pszName,msg);
#endif

         if (strcmp(pszName, "ShowMenu") == 0)
            message_ShowMenu = msg;
         else if (strcmp(pszName, "VGUIMenu") == 0)
            message_VGUIMenu = msg;
         else if (strcmp(pszName, "WeaponList") == 0)
            message_WeaponList = msg;
         else if (strcmp(pszName, "CurWeapon") == 0)
            message_CurWeapon = msg;
         else if (strcmp(pszName, "AmmoX") == 0)
            message_AmmoX = msg;
         else if (strcmp(pszName, "AmmoPickup") == 0)
            message_AmmoPickup = msg;
         else if (strcmp(pszName, "WeapPickup") == 0)
            message_WeapPickup = msg;
         else if (strcmp(pszName, "ItemPickup") == 0)
            message_ItemPickup = msg;
         else if (strcmp(pszName, "Health") == 0)
            message_Health = msg;
         else if (strcmp(pszName, "Battery") == 0)
            message_Battery = msg;
         else if (strcmp(pszName, "Damage") == 0)
            message_Damage = msg;
         else if (strcmp(pszName, "Money") == 0)
            message_Money = msg;
         else if (strcmp(pszName, "StatusIcon") == 0)
            message_StatusIcon = msg;
		 else if(strcmp(pszName, "SayText") == 0)
            message_SayText = msg;
		 else if(strcmp(pszName, "MOTD") == 0)
            message_MOTD = msg;
         else if (strcmp(pszName, "DeathMsg") == 0)
            message_DeathMsg = msg;
		 else if(strcmp(pszName, "ScreenFade") == 0)
            message_ScreenFade = msg;
		 else if(strcmp(pszName, "TeamScore") == 0)
            message_ScreenFade = msg;
   }

   return msg;
}

void pfnAnimationAutomove(const edict_t* pEdict, float flTime)
{
   (*g_engfuncs.pfnAnimationAutomove)(pEdict, flTime);
}
void pfnGetBonePosition(const edict_t* pEdict, int iBone, float *rgflOrigin, float *rgflAngles )
{
   (*g_engfuncs.pfnGetBonePosition)(pEdict, iBone, rgflOrigin, rgflAngles);
}
unsigned long pfnFunctionFromName( const char *pName )
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "FunctionFromName",NULL);
#endif
   return (*g_engfuncs.pfnFunctionFromName)(pName);
}
const char *pfnNameForFunction( unsigned long function )
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "NameForFunction",NULL);
#endif
   return (*g_engfuncs.pfnNameForFunction)(function);
}
void pfnClientPrintf( edict_t* pEdict, PRINT_TYPE ptype, const char *szMsg )
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "ClientPrintf",NULL);
#endif
   (*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
}
void pfnServerPrint( const char *szMsg )
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "ServerPrint",NULL);
#endif
   (*g_engfuncs.pfnServerPrint)(szMsg);
}
void pfnGetAttachment(const edict_t *pEdict, int iAttachment, float *rgflOrigin, float *rgflAngles )
{
   (*g_engfuncs.pfnGetAttachment)(pEdict, iAttachment, rgflOrigin, rgflAngles);
}
void pfnCRC32_Init(CRC32_t *pulCRC)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CRC32_Init",NULL);
#endif
   (*g_engfuncs.pfnCRC32_Init)(pulCRC);
}
void pfnCRC32_ProcessBuffer(CRC32_t *pulCRC, void *p, int len)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CRC32_ProcessBuffer",NULL);
#endif
   (*g_engfuncs.pfnCRC32_ProcessBuffer)(pulCRC, p, len);
}
void pfnCRC32_ProcessByte(CRC32_t *pulCRC, unsigned char ch)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CRC32_ProcessByte",NULL);
#endif
   (*g_engfuncs.pfnCRC32_ProcessByte)(pulCRC, ch);
}
CRC32_t pfnCRC32_Final(CRC32_t pulCRC)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CRC32_Final",NULL);
#endif
   return (*g_engfuncs.pfnCRC32_Final)(pulCRC);
}
long pfnRandomLong(long lLow, long lHigh)
{
   return (*g_engfuncs.pfnRandomLong)(lLow, lHigh);
}
float pfnRandomFloat(float flLow, float flHigh)
{
   return (*g_engfuncs.pfnRandomFloat)(flLow, flHigh);
}
void pfnSetView(const edict_t *pClient, const edict_t *pViewent )
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "SetView",NULL);
#endif
   (*g_engfuncs.pfnSetView)(pClient, pViewent);
}
float pfnTime( void )
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "Time",NULL);
#endif
   return (*g_engfuncs.pfnTime)();
}
void pfnCrosshairAngle(const edict_t *pClient, float pitch, float yaw)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CrosshairAngle",NULL);
#endif
   (*g_engfuncs.pfnCrosshairAngle)(pClient, pitch, yaw);
}
byte *pfnLoadFileForMe(char *filename, int *pLength)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "LoadFileForMe","filename=%s",filename);
#endif
   return (*g_engfuncs.pfnLoadFileForMe)(filename, pLength);
}
void pfnFreeFile(void *buffer)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "FreeFile",NULL);
#endif
   (*g_engfuncs.pfnFreeFile)(buffer);
}
void pfnEndSection(const char *pszSectionName)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "EndSection","SectionName=%s",pszSectionName);
#endif
   (*g_engfuncs.pfnEndSection)(pszSectionName);
}
int pfnCompareFileTime(char *filename1, char *filename2, int *iCompare)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CompareFileTime","name1=%s name2=%s",filename1,filename2);
#endif
   return (*g_engfuncs.pfnCompareFileTime)(filename1, filename2, iCompare);
}
void pfnGetGameDir(char *szGetGameDir)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "GetGameDir",NULL);
#endif
   (*g_engfuncs.pfnGetGameDir)(szGetGameDir);
}
void pfnCvar_RegisterVariable(cvar_t *variable)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "RegisterVariable",NULL);
#endif
   (*g_engfuncs.pfnCvar_RegisterVariable)(variable);
}
void pfnFadeClientVolume(const edict_t *pEdict, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "FadeClientVolume",NULL);
#endif
   (*g_engfuncs.pfnFadeClientVolume)(pEdict, fadePercent, fadeOutSeconds, holdTime, fadeInSeconds);
}
void pfnSetClientMaxspeed(const edict_t *pEdict, float fNewMaxspeed)
{
	bot_t *pBot=UTIL_GetBotPointer((edict_t*)pEdict);
	if(pBot!=NULL)
		pBot->f_max_speed=fNewMaxspeed;


#ifdef DEBUG
	report_log (LOG_ENGINE, "SetClientMaxspeed","ent=%x speed=%f",pEdict,fNewMaxspeed);
#endif
   (*g_engfuncs.pfnSetClientMaxspeed)(pEdict, fNewMaxspeed);
}
edict_t * pfnCreateFakeClient(const char *netname)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CreateFakeClient","name=%s",netname);
#endif
   return (*g_engfuncs.pfnCreateFakeClient)(netname);
}
void pfnRunPlayerMove(edict_t *fakeclient, const float *viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, byte impulse, byte msec )
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "RunPlayerMove",NULL);
#endif
   (*g_engfuncs.pfnRunPlayerMove)(fakeclient, viewangles, forwardmove, sidemove, upmove, buttons, impulse, msec);
}
int pfnNumberOfEntities(void)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "NumberOfEntities",NULL);
#endif
   return (*g_engfuncs.pfnNumberOfEntities)();
}
char* pfnGetInfoKeyBuffer(edict_t *e)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "GetInfoKeyBuffer",NULL);
#endif
   return (*g_engfuncs.pfnGetInfoKeyBuffer)(e);
}
char* pfnInfoKeyValue(char *infobuffer, char *key)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "InfoKeyValue","Buffer=%s key=%s",infobuffer,key);
#endif
   return (*g_engfuncs.pfnInfoKeyValue)(infobuffer, key);
}
void pfnSetKeyValue(char *infobuffer, char *key, char *value)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "SetKeyValue","buffer=%s key=%s value=%s",infobuffer,key,value);
#endif
   (*g_engfuncs.pfnSetKeyValue)(infobuffer, key, value);
}
void pfnSetClientKeyValue(int clientIndex, char *infobuffer, char *key, char *value)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "SetClientKeyValue","index=%d buffer=%s key=%s value=%s",
		clientIndex,infobuffer,key,value);
#endif
   (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, key, value);
}
int pfnIsMapValid(char *filename)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "IsMapValid","name=%s",filename);
#endif
   return (*g_engfuncs.pfnIsMapValid)(filename);
}
void pfnStaticDecal( const float *origin, int decalIndex, int entityIndex, int modelIndex )
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "StaticDecal",NULL);
#endif
   (*g_engfuncs.pfnStaticDecal)(origin, decalIndex, entityIndex, modelIndex);
}
int pfnPrecacheGeneric(char* s)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "PrecacheGeneric","%s",s);
#endif
   return (*g_engfuncs.pfnPrecacheGeneric)(s);
}
int pfnGetPlayerUserId(edict_t *e )
{
   if (gpGlobals->deathmatch)
   {
#ifdef DEBUG
	report_log (LOG_ENGINE, "GetPlayerUserId","ent=%x",e);
#endif
   }
   return (*g_engfuncs.pfnGetPlayerUserId)(e);
}
void pfnBuildSoundMsg(edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch, int msg_dest, int msg_type, const float *pOrigin, edict_t *ed)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "BuildSoundMsg",NULL);
#endif
   (*g_engfuncs.pfnBuildSoundMsg)(entity, channel, sample, volume, attenuation, fFlags, pitch, msg_dest, msg_type, pOrigin, ed);
}
int pfnIsDedicatedServer(void)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "IsDedicatedServer",NULL);
#endif
   return (*g_engfuncs.pfnIsDedicatedServer)();
}
cvar_t* pfnCVarGetPointer(const char *szVarName)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CVarGetPointer","CVAR=%s",szVarName);
#endif
   return (*g_engfuncs.pfnCVarGetPointer)(szVarName);
}
unsigned int pfnGetPlayerWONId(edict_t *e)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "GetPlayerWONId","ent=%x",e);
#endif
   return (*g_engfuncs.pfnGetPlayerWONId)(e);
}


// new stuff for SDK 2.0

void pfnInfo_RemoveKey(char *s, const char *key)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "Info_RemoveKey",NULL);
#endif
   (*g_engfuncs.pfnInfo_RemoveKey)(s, key);
}
const char *pfnGetPhysicsKeyValue(const edict_t *pClient, const char *key)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "GetPhysicsKeyValue",NULL);
#endif
   return (*g_engfuncs.pfnGetPhysicsKeyValue)(pClient, key);
}
void pfnSetPhysicsKeyValue(const edict_t *pClient, const char *key, const char *value)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "SetPhysicsKeyValue",NULL);
#endif
   (*g_engfuncs.pfnSetPhysicsKeyValue)(pClient, key, value);
}
const char *pfnGetPhysicsInfoString(const edict_t *pClient)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "GetPhysicsInfoString",NULL);
#endif
   return (*g_engfuncs.pfnGetPhysicsInfoString)(pClient);
}
unsigned short pfnPrecacheEvent(int type, const char *psz)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "PrecacheEvent","name=%s",psz);
#endif
   return (*g_engfuncs.pfnPrecacheEvent)(type, psz);
}
void pfnPlaybackEvent(int flags, const edict_t *pInvoker, unsigned short eventindex, float delay,
   float *origin, float *angles, float fparam1,float fparam2, int iparam1, int iparam2, int bparam1, int bparam2)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "PlaybackEvent",NULL);
#endif
   (*g_engfuncs.pfnPlaybackEvent)(flags, pInvoker, eventindex, delay, origin, angles, fparam1, fparam2, iparam1, iparam2, bparam1, bparam2);
}
unsigned char *pfnSetFatPVS(float *org)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "SetFatPVS",NULL);
#endif
   return (*g_engfuncs.pfnSetFatPVS)(org);
}
unsigned char *pfnSetFatPAS(float *org)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "SetFatPAS",NULL);
#endif
   return (*g_engfuncs.pfnSetFatPAS)(org);
}
int pfnCheckVisibility(const edict_t *entity, unsigned char *pset)
{
   return (*g_engfuncs.pfnCheckVisibility)(entity, pset);
}
void pfnDeltaSetField(struct delta_s *pFields, const char *fieldname)
{
   (*g_engfuncs.pfnDeltaSetField)(pFields, fieldname);
}
void pfnDeltaUnsetField(struct delta_s *pFields, const char *fieldname)
{
   (*g_engfuncs.pfnDeltaUnsetField)(pFields, fieldname);
}
void pfnDeltaAddEncoder(char *name, void (*conditionalencode)( struct delta_s *pFields, const unsigned char *from, const unsigned char *to))
{
   (*g_engfuncs.pfnDeltaAddEncoder)(name, conditionalencode);
}
int pfnGetCurrentPlayer(void)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "GetCurrentPlayer",NULL);
#endif
   return (*g_engfuncs.pfnGetCurrentPlayer)();
}
int pfnCanSkipPlayer(const edict_t *player)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CanSkipPlayer",NULL);
#endif
   return (*g_engfuncs.pfnCanSkipPlayer)(player);
}
int pfnDeltaFindField(struct delta_s *pFields, const char *fieldname)
{
   return (*g_engfuncs.pfnDeltaFindField)(pFields, fieldname);
}
void pfnDeltaSetFieldByIndex(struct delta_s *pFields, int fieldNumber)
{
   (*g_engfuncs.pfnDeltaSetFieldByIndex)(pFields, fieldNumber);
}
void pfnDeltaUnsetFieldByIndex(struct delta_s *pFields, int fieldNumber)
{
   (*g_engfuncs.pfnDeltaUnsetFieldByIndex)(pFields, fieldNumber);
}
void pfnSetGroupMask(int mask, int op)
{
   (*g_engfuncs.pfnSetGroupMask)(mask, op);
}
int pfnCreateInstancedBaseline(int classname, struct entity_state_s *baseline)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CreateInstancedBaseline",NULL);
#endif
   return (*g_engfuncs.pfnCreateInstancedBaseline)(classname, baseline);
}
void pfnCvar_DirectSet(struct cvar_s *var, char *value)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "CVar_DirectSet",NULL);
#endif
   (*g_engfuncs.pfnCvar_DirectSet)(var, value);
}
void pfnForceUnmodified(FORCE_TYPE type, float *mins, float *maxs, const char *filename)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "ForceUnmodified","name=%s",filename);
#endif
   (*g_engfuncs.pfnForceUnmodified)(type, mins, maxs, filename);
}
void pfnGetPlayerStats(const edict_t *pClient, int *ping, int *packet_loss)
{
#ifdef DEBUG
	report_log (LOG_ENGINE, "GetPlayerStats",NULL);
#endif
   (*g_engfuncs.pfnGetPlayerStats)(pClient, ping, packet_loss);
}

/*void pfnAddServerCommand(char *cmd_name, void (*function) (void) )
{
   (*g_engfuncs.pfnAddServerCommand)(cmd_name, function);
}*/

