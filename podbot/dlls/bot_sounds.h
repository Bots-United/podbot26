//
// bot_sounds.h
//

#ifndef BOTSOUNDS_H
#define BOTSOUNDS_H

extern threat_t ThreatTab[32];

void SoundAttachToThreat(edict_t *pEdict,const char *pszSample,float fVolume);
void SoundSimulateUpdate(int iPlayerIndex);

#endif

