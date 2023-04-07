// These are Hooks to let the Bot 'hear' otherwise
// unnoticed things. Base code & idea came from
// killaruna/ParaBot  


#include "extdll.h"
#include "util.h"
#include "bot.h"
#include "bot_sounds.h"


// Called by the Sound Hooking Code (in EMIT_SOUND)
// Enters the played Sound into the Array associated with the
// Entity
void SoundAttachToThreat(edict_t *pEdict,const char *pszSample,float fVolume)
{
	if(!pEdict)
		return;

	Vector vecPosition;
	int iIndex;

	
	// Hit/Fall Sound ?
	if ( strncmp("player/bhit_flesh", pszSample, 17) == 0
		|| strncmp("player/headshot", pszSample, 15) == 0)
	{
		iIndex=ENTINDEX(pEdict) - 1;
		ThreatTab[iIndex].fHearingDistance = 800.0 * fVolume;
		ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 0.5;
		ThreatTab[iIndex].vecSoundPosition = pEdict->v.origin;
	}
	// Weapon Pickup ?
	else if( strncmp("items/gunpickup", pszSample, 15) == 0) 
	{
		iIndex=ENTINDEX(pEdict) - 1;
		ThreatTab[iIndex].fHearingDistance = 800.0 * fVolume;
		ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 0.5;
		ThreatTab[iIndex].vecSoundPosition = pEdict->v.origin;
	}
	// Sniper zooming ?
	else if( strncmp("weapons/zoom", pszSample, 12) == 0)
	{
		iIndex=ENTINDEX(pEdict) - 1;
		ThreatTab[iIndex].fHearingDistance = 500.0 * fVolume;
		ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 0.1;
		ThreatTab[iIndex].vecSoundPosition = pEdict->v.origin;
	}
	// Reload ?
	else if ( strncmp("weapons/reload", pszSample, 14) == 0)
	{
		iIndex=ENTINDEX(pEdict) - 1;
		ThreatTab[iIndex].fHearingDistance = 500.0 * fVolume;
		ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 0.5;
		ThreatTab[iIndex].vecSoundPosition = pEdict->v.origin;
	}
	// The following Sounds don't have the Player Entity associated
	// so we need to search the nearest Player

	// Ammo Pickup ?
	else if( strncmp("items/9mmclip", pszSample, 13) == 0) 
	{
		vecPosition = pEdict->v.origin;
		iIndex=UTIL_GetNearestPlayerIndex(vecPosition);
		ThreatTab[iIndex].fHearingDistance = 500.0 * fVolume;
		ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 0.1;
		ThreatTab[iIndex].vecSoundPosition = pEdict->v.origin;
	}
	// CT used Hostage ?
	else if ( strncmp("hostage/hos", pszSample, 11) == 0)
	{
		vecPosition = VecBModelOrigin(pEdict);
		iIndex=UTIL_GetNearestPlayerIndex(vecPosition);
		ThreatTab[iIndex].fHearingDistance = 1024.0 * fVolume;
		ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 5.0;
		ThreatTab[iIndex].vecSoundPosition = vecPosition;
	}
	// Broke something ?
	else if ( strncmp("debris/bustmetal", pszSample, 16) == 0
			|| strncmp("debris/bustglass", pszSample, 16) == 0)
	{
		vecPosition = VecBModelOrigin(pEdict);
		iIndex=UTIL_GetNearestPlayerIndex(vecPosition);
		ThreatTab[iIndex].fHearingDistance = 1024.0 * fVolume;
		ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 2.0;
		ThreatTab[iIndex].vecSoundPosition = vecPosition;
	}
	// Someone opened a door
	else if ( strncmp("doors/doormove", pszSample, 14) == 0)
	{
		vecPosition = VecBModelOrigin(pEdict);
		iIndex=UTIL_GetNearestPlayerIndex(vecPosition);
		ThreatTab[iIndex].fHearingDistance = 1024.0 * fVolume;
		ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 3.0;
		ThreatTab[iIndex].vecSoundPosition = vecPosition;
	}
}


// Tries to simulate playing of Sounds to let the Bots hear
// sounds which aren't captured through Server Sound hooking
void SoundSimulateUpdate(int iPlayerIndex)
{
	edict_t *pPlayer = ThreatTab[iPlayerIndex].pEdict;
	float fVelocity = pPlayer->v.velocity.Length2D();
	float fHearDistance = 0.0;
	float fTimeSound;
	
	// Pressed Attack Button ?
	if(pPlayer->v.oldbuttons & IN_ATTACK)
	{
		fHearDistance = 4096.0;
		fTimeSound = gpGlobals->time + 0.5;
	}
	// Pressed Used Button ?
	else if(pPlayer->v.oldbuttons & IN_USE)
    {
		fHearDistance = 1024.0;
		fTimeSound = gpGlobals->time + 0.5;
    }
	// Uses Ladder ?
	else if(pPlayer->v.movetype == MOVETYPE_FLY)
	{
		if(abs(pPlayer->v.velocity.z)>50)
		{
			fHearDistance = 1024.0;
			fTimeSound = gpGlobals->time + 0.3;
		}
	}
	// Moves fast enough ?
	else
	{
		static float fMaxSpeed = 0.0;
		
		if(fMaxSpeed == 0.0)
			fMaxSpeed = CVAR_GET_FLOAT("sv_maxspeed");
		fHearDistance = 1024.0 * (fVelocity / fMaxSpeed);
		fTimeSound = gpGlobals->time + 0.3;
	}

	// Did issue Sound ?
	if(fHearDistance > 0.0)
	{
		// Some sound already associated ?
		if(ThreatTab[iPlayerIndex].fTimeSoundLasting > gpGlobals->time)
		{
			// New Sound louder (bigger range) than old sound ?
			if(ThreatTab[iPlayerIndex].fHearingDistance <= fHearDistance)
			{
				// Override it with new 
				ThreatTab[iPlayerIndex].fHearingDistance = fHearDistance;
				ThreatTab[iPlayerIndex].fTimeSoundLasting = fTimeSound;
				ThreatTab[iPlayerIndex].vecSoundPosition = pPlayer->v.origin;
			}
		}
		// New sound ?
		else
		{
			// Just remember it
			ThreatTab[iPlayerIndex].fHearingDistance = fHearDistance;
			ThreatTab[iPlayerIndex].fTimeSoundLasting = fTimeSound;
			ThreatTab[iPlayerIndex].vecSoundPosition = pPlayer->v.origin;
		}
	}
}
