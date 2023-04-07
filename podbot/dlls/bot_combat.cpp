//
// bot_combat.cpp
//
// Does Enemy Sensing (spotting), combat movement and
// firing weapons
// 

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "log.h"
#include "bot_weapons.h"
#include "waypoint.h"
#include "bot_globals.h"


extern threat_t ThreatTab[32];
extern bot_weapon_t weapon_defs[MAX_WEAPONS]; // array of weapon definitions
extern skilldelay_t BotSkillDelays[6];
extern bot_t bots[MAXBOTSNUMBER];


static FILE *fp;

// Weapons and their specifications
bot_weapon_select_t cs_weapon_select[NUM_WEAPONS+1] = {
	// Knife
   {CS_WEAPON_KNIFE, "weapon_knife","models/w_knife.mdl",0.0, 50.0, 0.0, 0.0,
    TRUE, TRUE, 0.0,0,
   0,-1,-1,0,0,FALSE},
	// Pistols
	// 1280
   {CS_WEAPON_USP, "weapon_usp","models/w_usp.mdl", 0.0, 4096.0, 0.0, 1200.0,
    TRUE, FALSE, 2.8,500,
   1,-1,-1,1,1,FALSE},
   // 1536
   {CS_WEAPON_GLOCK18, "weapon_glock18","models/w_glock18.mdl",0.0, 2048.0, 0.0, 1200.0,
    TRUE, FALSE, 2.3,400,
   1,-1,-1,1,2,FALSE},
	// 1024
   {CS_WEAPON_DEAGLE, "weapon_deagle","models/w_deagle.mdl",0.0, 4096.0, 0.0, 1200.0,
    TRUE, FALSE, 2.3,650,
   1,2,2,1,3,TRUE},
	// 1024
   {CS_WEAPON_P228, "weapon_p228","models/w_p228.mdl",0.0, 4096.0, 0.0, 1200.0,
    TRUE, FALSE, 2.8,600,
   1,2,2,1,4,FALSE},
   // new for CS 7.0
   // 1280
   {CS_WEAPON_ELITE, "weapon_elite","models/w_elite.mdl",0.0, 4096.0, 0.0, 1200.0,
    TRUE, FALSE, 4.6,1000,
   1,0,0,1,5,FALSE},
   // new for CS 1.0
   {CS_WEAPON_FIVESEVEN, "weapon_fiveseven","models/w_fiveseven.mdl", 0.0, 4096.0, 0.0, 1200.0,
    TRUE, FALSE, 3.17,750,
   1,1,1,1,6,FALSE},
	// Shotguns
	// 1024
   {CS_WEAPON_M3, "weapon_m3","models/w_m3.mdl",0.0,2048.0, 0.0, 1200.0,
    TRUE, FALSE, 1.1,1700,
   1,2,-1,2,1,FALSE},
   // 1024
   {CS_WEAPON_XM1014, "weapon_xm1014","models/w_xm1014.mdl",0.0,2048.0, 0.0, 1200.0,
    TRUE, FALSE, 0.9,3000,
   1,2,-1,2,2,FALSE},
	// Sub Machine Guns
	// 1536
   {CS_WEAPON_MP5NAVY, "weapon_mp5navy","models/w_mp5.mdl",0.0,2048.0, 0.0, 1200.0,
    TRUE, TRUE, 2.74,1500,
   1,2,1,3,1,FALSE},
   {CS_WEAPON_TMP, "weapon_tmp","models/w_tmp.mdl",0.0,2048.0, 0.0, 1200.0,
    TRUE, TRUE, 2.2,1250,
   1,1,1,3,2,FALSE},
   // 1536
   {CS_WEAPON_P90, "weapon_p90","models/w_p90.mdl", 0.0,2048.0, 0.0, 1200.0,
    TRUE, TRUE, 3.5,2350,
   1,2,1,3,3,FALSE},
   // 1536
   {CS_WEAPON_MAC10, "weapon_mac10","models/w_mac10.mdl", 0.0,2048.0, 0.0, 1200.0,
    TRUE, TRUE, 3.3,1400,
   1,0,0,3,4,FALSE},
   // new for CS 1.0
   {CS_WEAPON_UMP45, "weapon_ump45","models/w_ump45.mdl",0.0,2048.0, 0.0, 1200.0,
    TRUE, TRUE, 4.15,1700,
   1,2,2,3,5,FALSE},
	// Rifles
	// 2048
   {CS_WEAPON_AK47, "weapon_ak47","models/w_ak47.mdl", 0.0, 4096.0, 0.0, 1200.0,
    TRUE, TRUE, 2.6,2500,
   1,0,0,4,1,TRUE},
   {CS_WEAPON_SG552, "weapon_sg552","models/w_sg552.mdl",0.0, 4096.0, 0.0, 1200.0,
    TRUE, TRUE, 4.0,3500,
   1,0,-1,4,2,TRUE},
   {CS_WEAPON_M4A1, "weapon_m4a1","models/w_m4a1.mdl", 0.0, 4096.0, 0.0, 1200.0,
    TRUE, TRUE, 3.2,3100,
   1,1,1,4,3,TRUE},
   {CS_WEAPON_AUG, "weapon_aug","models/w_aug.mdl",0.0, 4096.0, 0.0, 1200.0,
    TRUE, TRUE, 3.5,3500,
   1,1,1,4,4,TRUE},
	// Sniper Rifles
	// 1024
   // 1024
   {CS_WEAPON_SCOUT, "weapon_scout","models/w_scout.mdl", 300.0, 2048.0, 0.0, 1200.0,
    TRUE, FALSE, 2.2,2750,
   1,2,0,4,5,FALSE},
   // 1280
   {CS_WEAPON_AWP, "weapon_awp","models/w_awp.mdl",0.0, 4096.0, 0.0, 1200.0,
    TRUE, FALSE, 2.6,4750,
   1,2,0,4,6,TRUE},
   // 1024
   {CS_WEAPON_G3SG1, "weapon_g3sg1","models/w_g3sg1.mdl",300.0, 4096.0, 0.0, 1200.0,
    TRUE, FALSE, 4.9,5000,
   1,0,2,4,7,TRUE},
   {CS_WEAPON_SG550, "weapon_sg550","models/w_sg550.mdl",300.0, 4096.0, 0.0, 1200.0,
    TRUE, FALSE, 4.182,4200,
   1,1,1,4,8,TRUE},
	// Machine Guns
   {CS_WEAPON_M249, "weapon_m249","models/w_m249.mdl",0.0,2048.0, 0.0, 1200.0,
    TRUE, TRUE, 4.85,5750,
   1,2,1,5,1,TRUE},
	 //terminator 
   {0, "","", 0.0, 0.0, 0.0, 0.0, TRUE, FALSE, 0.0,0,0,0,0,0}
};

// weapon firing delay based on skill (min and max delay for each weapon)
// THESE MUST MATCH THE SAME ORDER AS THE WEAPON SELECT ARRAY ABOVE!!!
// Last 2 values are Burstfire Bullet Count & Pause Times

bot_fire_delay_t cs_fire_delay[NUM_WEAPONS+1] = {
	// Knife
   {CS_WEAPON_KNIFE,
    0.3, {0.0, 0.2, 0.3, 0.4, 0.6,0.8}, {0.1, 0.3, 0.5, 0.7, 1.0,1.2},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
	255,1.0},
	// Pistols
   {CS_WEAPON_USP,
    0.2, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	2,MIN_BURST_DISTANCE/0.3},
   {CS_WEAPON_GLOCK18,
    0.2, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	2,MIN_BURST_DISTANCE/0.3},
   {CS_WEAPON_DEAGLE,
    0.3, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	2,MIN_BURST_DISTANCE/0.5},
   {CS_WEAPON_P228,
    0.2, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	2,MIN_BURST_DISTANCE/0.3},
   {CS_WEAPON_ELITE,
    0.15, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	3,MIN_BURST_DISTANCE/0.5},
   {CS_WEAPON_FIVESEVEN,
    0.18, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	2,MIN_BURST_DISTANCE/0.5},
	// Shotguns
	// 7
   {CS_WEAPON_M3,
    0.86, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	2,MIN_BURST_DISTANCE/1.0},
	// 8
   {CS_WEAPON_XM1014,
    0.28, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	3,MIN_BURST_DISTANCE/1.0},
	// Sub Machine Guns
	// 9
   {CS_WEAPON_MP5NAVY,
    0.1, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	5,MIN_BURST_DISTANCE/0.5},
	// 10
   {CS_WEAPON_TMP,
    0.1, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	5,MIN_BURST_DISTANCE/0.5},
	// 11
   {CS_WEAPON_P90,
    0.06, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	5,MIN_BURST_DISTANCE/0.5},
	// 12
   {CS_WEAPON_MAC10,
    0.08, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	5,MIN_BURST_DISTANCE/0.5},
	// 13
   {CS_WEAPON_UMP45,
    0.2, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	3,MIN_BURST_DISTANCE/0.5},
	// Rifles
	// 14
   {CS_WEAPON_AK47,
    0.13, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	3,MIN_BURST_DISTANCE/0.5},
	// 15
   {CS_WEAPON_SG552,
    0.12, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	5,MIN_BURST_DISTANCE/0.5},
	// 16
   {CS_WEAPON_M4A1,
    0.13, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	5,MIN_BURST_DISTANCE/0.5},
	// 17
   {CS_WEAPON_AUG,
    0.12, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	3,MIN_BURST_DISTANCE/0.5},
	// Sniper Rifles
	// 18
   {CS_WEAPON_SCOUT,
    1.3, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	2,MIN_BURST_DISTANCE/1.0},
	// 19
   {CS_WEAPON_AWP,
    1.6, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	2,MIN_BURST_DISTANCE/1.5},
	// 20
   {CS_WEAPON_G3SG1,
    0.27, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	3,MIN_BURST_DISTANCE/1.0},
	// 21
   {CS_WEAPON_SG550,
    0.26, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	3,MIN_BURST_DISTANCE/1.0},
	// Machine Guns
	// 22
   {CS_WEAPON_M249,
    0.1, {0.0, 0.1, 0.2, 0.3, 0.4,0.6}, {0.1, 0.2, 0.3, 0.4, 0.5,0.7},
    0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4},
	6,MIN_BURST_DISTANCE/1.0},
	/* terminator */
   {0, 0.0, {0.0, 0.0, 0.0, 0.0, 0.0,0.0}, {0.0, 0.0, 0.0, 0.0, 0.0,0.0},
       0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
	0,MIN_BURST_DISTANCE/1.0}
};




// This Array stores the Aiming Offsets, Headshot Frequency and
// the ShootThruWalls Probability (worst to best skill)
// Overridden by botskill.cfg
botaim_t BotAimTab[6]={
	{40,40,50,0,0,0},{30,30,42,10,0,0},{20,20,32,30,0,50},
	{10,10,18,50,30,80},{5,5,10,80,50,100},{0,0,0,100,100,100}};





int NumTeammatesNearPos(bot_t *pBot,Vector vecPosition,int iRadius)
{
	int iCount = 0;
	float fDistance;
	edict_t *pEdict = pBot->pEdict;
	int iTeam = UTIL_GetTeam(pEdict);

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		if(ThreatTab[i].IsUsed == FALSE || ThreatTab[i].IsAlive == FALSE
		|| ThreatTab[i].iTeam != iTeam || ThreatTab[i].pEdict == pEdict)
			continue;
		fDistance = (ThreatTab[i].vOrigin - vecPosition).Length();
		if(fDistance < iRadius)
			iCount++;
	}
#ifdef DEBUG
	report_log (LOG_BOTCOMBAT, "NumTeammatesNearPos","Count=%d",iCount);
#endif
	return(iCount);
}

int NumEnemiesNearPos(bot_t *pBot,Vector vecPosition,int iRadius)
{
	int iCount = 0;
	float fDistance;
	edict_t *pEdict = pBot->pEdict;
	int iTeam = UTIL_GetTeam(pEdict);

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		if(ThreatTab[i].IsUsed == FALSE || ThreatTab[i].IsAlive == FALSE
		|| ThreatTab[i].iTeam == iTeam)
			continue;
		fDistance = (ThreatTab[i].vOrigin - vecPosition).Length();
		if(fDistance < iRadius)
			iCount++;
	}
#ifdef DEBUG
	report_log (LOG_BOTCOMBAT, "NumEnemiesNearPos","Count=%d",iCount);
#endif
	return(iCount);
}

// Returns if an Enemy can be seen
// FIXME: Bot should lock onto the best shoot position for
// a target instead of going through all of them everytime 
bool BotFindEnemy( bot_t *pBot )
{
	pBot->ucVisibility = 0;
	pBot->vecEnemy = Vector(0,0,0);
	// We're blind and can't see anything !!
	if(pBot->f_blind_time>gpGlobals->time)
		return FALSE;

	Vector vecEnd;
	static bool flag=TRUE;
	edict_t *pPlayer;
	
	edict_t *pEdict = pBot->pEdict;

	// Setup Potentially Visible Set for this Bot
	Vector vecOrg = pEdict->v.origin + pEdict->v.view_ofs;
	if ( pEdict->v.flags & FL_DUCKING )
	{
		vecOrg = vecOrg + ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
	}
	unsigned char *pvs = ENGINE_SET_PVS ( (float *)&vecOrg );

	// Clear suspected Flag
	pBot->iStates &= ~STATE_SUSPECTENEMY;

	int i;
	float nearestdistance = pBot->f_view_distance;
	edict_t *pNewEnemy = NULL;

	Vector vecVisible;
	unsigned char cHit;

	int bot_team = UTIL_GetTeam(pEdict);

	if(pBot->pBotEnemy && pBot->fEnemyUpdateTime > gpGlobals->time)
	{
		pPlayer = pBot->pBotEnemy;
		if(IsAlive(pPlayer))
		{
			vecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;
			if (FInViewCone( &vecEnd, pEdict) && FBoxVisible(pEdict,pPlayer,&vecVisible,&cHit)) 
			{
				pNewEnemy = pPlayer;
				pBot->vecEnemy = vecVisible;
				pBot->ucVisibility = cHit;
			}
		}

	}
	
	if(pNewEnemy == NULL)
	{
		pBot->fEnemyUpdateTime = gpGlobals->time + 0.5;
		// search the world for players...
		for (i = 0; i < gpGlobals->maxClients; i++)
		{
			if((ThreatTab[i].IsUsed==FALSE) || (ThreatTab[i].IsAlive==FALSE) ||
				(ThreatTab[i].iTeam==bot_team) || (ThreatTab[i].pEdict==pEdict))
				continue;
			pPlayer = ThreatTab[i].pEdict;
			// Let the Engine check if this Player is potentially
			// visible
			if(!ENGINE_CHECK_VISIBILITY(pPlayer, pvs ) )
				continue;
			vecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;
			
			// see if bot can see the player...
			if (FInViewCone( &vecEnd, pEdict) && FBoxVisible(pEdict,pPlayer,&vecVisible,&cHit)) 
			{
				float distance;
				distance = (pPlayer->v.origin - pEdict->v.origin).Length();
				if (distance < nearestdistance)
				{
					nearestdistance = distance;
					pNewEnemy = pPlayer;
					pBot->vecEnemy = vecVisible;
					pBot->ucVisibility = cHit;
					// On Assault Maps target VIP first !
					if(g_iMapType==MAP_AS)
					{
						char *infobuffer;
						char model_name[32];
						
						infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)( pEdict );
						strcpy(model_name, (g_engfuncs.pfnInfoKeyValue(infobuffer, "model")));
						if ((strcmp(model_name, "vip") == 0)) // Is VIP ?
							break;
					}
				}
			}
			
		}
	}
	
	if (pNewEnemy)
	{
		g_bBotsCanPause = TRUE;
		pBot->iAimFlags |= AIM_ENEMY;

		if(pNewEnemy == pBot->pBotEnemy)
		{
#ifdef DEBUG
	report_log (LOG_BOTCOMBAT, "BotFindEnemy","using Old Enemy %s",STRING(pNewEnemy->v.netname));
#endif
			// if enemy is still visible and in field of view, keep it
			// keep track of when we last saw an enemy
			pBot->f_bot_see_enemy_time = gpGlobals->time;
			// Zero out reaction time
			pBot->f_actual_reaction_time = 0.0;
			pBot->pLastEnemy = pNewEnemy;
			pBot->vecLastEnemyOrigin = pBot->vecEnemy;
			return TRUE;
		}
		else
		{
#ifdef DEBUG
	report_log (LOG_BOTCOMBAT, "BotFindEnemy","New Enemy %s",STRING(pNewEnemy->v.netname));
#endif
			if((pBot->f_bot_see_enemy_time+3.0<gpGlobals->time) && ((pEdict->v.weapons & (1<<CS_WEAPON_C4)) ||
				((BotHasHostage(pBot) || pBot->pBotUser!=NULL))))
				BotPlayRadioMessage(pBot,RADIO_ENEMYSPOTTED);

			pBot->f_enemy_surprise_time=gpGlobals->time+pBot->f_actual_reaction_time;
			// Zero out reaction time
			pBot->f_actual_reaction_time=0.0;
			pBot->pBotEnemy = pNewEnemy;
			pBot->pLastEnemy = pNewEnemy;
			pBot->vecLastEnemyOrigin = pBot->vecEnemy;
			pBot->vecEnemyVelocity = Vector(0,0,0);
			pBot->fEnemyReachableTimer = 0.0;
			// keep track of when we last saw an enemy
			pBot->f_bot_see_enemy_time = gpGlobals->time;
			// Now alarm all Teammates who see this Bot & 
			// don't have an actual Enemy of the Bots Enemy
			// Should simulate human players seeing a Teammate firing
			bot_t *pFriendlyBot;
			for (int j = 0; j < gpGlobals->maxClients; j++)
			{
				if((ThreatTab[j].IsUsed==FALSE) || (ThreatTab[j].IsAlive==FALSE) ||
					(ThreatTab[j].iTeam!=bot_team) || (ThreatTab[j].pEdict==pEdict))
					continue;
				pFriendlyBot = UTIL_GetBotPointer(ThreatTab[j].pEdict);
				if(pFriendlyBot != NULL)
				{
					if(pFriendlyBot->f_bot_see_enemy_time+3.0 < gpGlobals->time ||
						(pFriendlyBot->pLastEnemy == NULL))
					{
						if(FVisible(pEdict->v.origin,pFriendlyBot->pEdict))
						{
							pFriendlyBot->pLastEnemy = pNewEnemy;
							pFriendlyBot->vecLastEnemyOrigin = pBot->vecLastEnemyOrigin;
							pFriendlyBot->f_bot_see_enemy_time = gpGlobals->time;
						}
					}
				}

			}

			return TRUE;
		}
	}
	else if(pBot->pBotEnemy != NULL)
	{
		pNewEnemy = pBot->pBotEnemy;
		pBot->pLastEnemy = pNewEnemy;
		if(!IsAlive(pNewEnemy))
		{
			pBot->pBotEnemy = NULL;
			// Shoot at dying players if no new enemy to give some
			// more human-like illusion
/*			if(pBot->f_bot_see_enemy_time+0.1>gpGlobals->time)
			{
				if(!BotUsesSniper(pBot))
				{
					pBot->f_shootatdead_time = gpGlobals->time+(pBot->fAgressionLevel*1.5);
					pBot->f_actual_reaction_time = 0.0;
					// Set suspected Flags to prevent throwing Grens &
					// only shoot body
					pBot->iStates |= STATE_SUSPECTENEMY;
					return TRUE;
				}
				return FALSE;
			}
			else if(pBot->f_shootatdead_time > gpGlobals->time)
			{
				pBot->f_actual_reaction_time = 0.0;
				// Set suspected Flags to prevent throwing Grens &
				// only shoot body
				pBot->iStates |= STATE_SUSPECTENEMY;
				return TRUE;
			}*/
			return FALSE;
		}

		// If no Enemy visible check if last one shootable thru Wall 
		int iShootThruFreq = BotAimTab[pBot->bot_skill/20].iSeenShootThruProb;
		if(g_bShootThruWalls && RANDOM_LONG(1,100)<iShootThruFreq &&
			WeaponShootsThru(pBot->current_weapon.iId))
		{
			if(IsShootableThruObstacle(pEdict,pNewEnemy->v.origin))
			{
#ifdef DEBUG
				report_log (LOG_BOTCOMBAT, "BotFindEnemy","Through Wall - Enemy %s",STRING(pNewEnemy->v.netname));
#endif
				pBot->f_bot_see_enemy_time = gpGlobals->time;
				pBot->iStates |= STATE_SUSPECTENEMY;
				pBot->iAimFlags |= AIM_LASTENEMY;
				pBot->pLastEnemy = pNewEnemy;
				pBot->vecLastEnemyOrigin = pNewEnemy->v.origin;
				return TRUE;
			}
		}
		else
			return FALSE;
	}
	return FALSE;
}


// Returns the aiming Vector for an Enemy
// FIXME: Doesn't take the spotted part of the enemy  
Vector BotBodyTarget( edict_t *pBotEnemy, bot_t *pBot )
{
	Vector target;
	unsigned char ucVis = pBot->ucVisibility;	
	edict_t *pEdict = pBot->pEdict;
	
	Vector vecVel = pBot->vecEnemyVelocity;
	// No Up/Down Compensation
	vecVel.z = 0.0;
	float fSkillMult = 0.1 - ((float)pBot->bot_skill / 1000.0);
	vecVel = vecVel * fSkillMult;

	// More Precision if using sniper weapon
	if(BotUsesSniper(pBot))
	{
		vecVel = vecVel / 2;
	}
	// Waist Visible ?
	else if(ucVis & WAIST_VISIBLE)
	{
		// Use Waist as Target for big distances
		float fDistance = (pBotEnemy->v.origin - pEdict->v.origin).Length();
		if(fDistance > 1500)
			ucVis &= ~HEAD_VISIBLE;
	}

	int TabOffs = pBot->bot_skill/20;

	// If we only suspect an Enemy behind a Wall take
	// the worst Skill
	if(pBot->iStates & STATE_SUSPECTENEMY)
	{
		target = pBotEnemy->v.origin;
		target.x = target.x + RANDOM_FLOAT(-32.0,32.0);
		target.y = target.y + RANDOM_FLOAT(-32.0,32.0);
		target.z = target.z + RANDOM_FLOAT(-32.0,32.0);
	}
	else
	{
		if((ucVis & HEAD_VISIBLE)
		&& (ucVis & WAIST_VISIBLE))
		{
			if(RANDOM_LONG(1,100)<BotAimTab[TabOffs].iHeadShot_Frequency)
				target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;  // aim for the head
			else
				target = pBotEnemy->v.origin;  // aim for body
		}
		else if(ucVis & HEAD_VISIBLE)
		{
			target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;  // aim for the head
			target.z -= 8.0;
		}
		else if(ucVis & WAIST_VISIBLE)
			target = pBotEnemy->v.origin;  // aim for body
		else if(ucVis & CUSTOM_VISIBLE)
			target = pBot->vecEnemy;  // aim for custom part
		// Something went wrong - use last enemy origin
		else
		{
			assert(pBot == NULL);
			target = pBot->vecLastEnemyOrigin;
		}
	}

	pBot->vecEnemyVelocity = -pBotEnemy->v.velocity;

	pBot->vecEnemy = target;
	return target;
}


// Returns if Weapon can pierce thru a wall
bool WeaponShootsThru(int iId)
{
	int i=0;

	while (cs_weapon_select[i].iId)
	{
		if(cs_weapon_select[i].iId == iId)
		{
			if(cs_weapon_select[i].bShootsThru)
				return TRUE;
			else
				return FALSE;
		}
		i++;
	}
	return FALSE;
}


bool WeaponIsSniper(int iId)
{
	if(iId == CS_WEAPON_AWP || iId == CS_WEAPON_G3SG1
	|| iId==CS_WEAPON_SCOUT || iId==CS_WEAPON_SG550)
	{
		return TRUE;
	}
	return FALSE;
}

bool FireHurtsFriend(bot_t *pBot,float fDistance)
{
	edict_t *pEdict = pBot->pEdict;
	edict_t *pPlayer;
	int iTeam = UTIL_GetTeam(pEdict);

	// search the world for players...
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		if(ThreatTab[i].IsUsed == FALSE
			|| ThreatTab[i].IsAlive == FALSE
			|| ThreatTab[i].iTeam != iTeam
			|| ThreatTab[i].pEdict == pEdict)
			continue;
		pPlayer = ThreatTab[i].pEdict;
		if(GetShootingConeDeviation(pEdict,&pPlayer->v.origin) > 0.9)
		{
			if((pPlayer->v.origin - pEdict->v.origin).Length() <= fDistance)
				return TRUE;
		}
	}

	return FALSE;
}


// Returns if enemy can be shoot through some obstacle
// TODO: After seeing the disassembled CS Routine it could
// be speedup and simplified a lot

bool IsShootableThruObstacle(edict_t *pEdict,Vector vecDest)
{
	Vector vecSrc = pEdict->v.origin+pEdict->v.view_ofs;
	Vector vecDir = (vecDest - vecSrc).Normalize();  // 1 unit long
	Vector vecPoint;
	int iThickness=0;
	int iHits=0;
	
	edict_t	*pentIgnore=pEdict->v.pContainingEntity;
	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecDest, ignore_monsters,ignore_glass, pentIgnore, &tr);
	while(tr.flFraction != 1.0 && iHits<3)
	{
		iHits++;
		iThickness++;
		vecPoint=tr.vecEndPos+vecDir;
		while((UTIL_PointContents(vecPoint) == CONTENTS_SOLID) && (iThickness<64))
		{
			vecPoint=vecPoint+vecDir;
			iThickness++;
		}
		UTIL_TraceLine(vecPoint, vecDest, ignore_monsters,ignore_glass, pentIgnore, &tr);
	}
	if(iHits<3 && iThickness<64)
	{
		float f_distance=(vecDest - vecPoint).Length();
		if(f_distance<112)
			return TRUE;
	}
	return FALSE;
}


// Returns true if Bot needs to pause between firing to compensate for
// punchangle & weapon spread
bool BotDoFirePause(bot_t *pBot, float fDistance, bot_fire_delay_t* pDelay)
{
	if(fDistance < MIN_BURST_DISTANCE)
		return FALSE;
	else if(pBot->fTimeFirePause>gpGlobals->time)
	{
		pBot->f_shoot_time=gpGlobals->time;
		return TRUE;
	}
	else if(pDelay->iMaxFireBullets + RANDOM_LONG(0,1)<=pBot->iBurstShotsFired)
	{
		pBot->fTimeFirePause = gpGlobals->time + (fDistance/pDelay->fMinBurstPauseFactor);
		pBot->f_shoot_time=gpGlobals->time;
		pBot->iBurstShotsFired = 0;
		return TRUE;
	}
	return FALSE;
}

// BotFireWeapon will return TRUE if weapon was fired, FALSE otherwise
bool BotFireWeapon( Vector v_enemy, bot_t *pBot)
{
	// Currently switching Weapon ?
	if(pBot->current_weapon.iId == CS_WEAPON_INSWITCH
	|| pBot->bUsingGrenade)
	{
		return FALSE;
	}


	bot_weapon_select_t *pSelect = NULL;
	bot_fire_delay_t *pDelay = NULL;
	int iId;
	int select_index = 0;
	int iChosenWeaponIndex = 0;
	
#ifdef DEBUG
	report_log (LOG_BOTCOMBAT, "BotFireWeapon",NULL);
#endif
	edict_t *pEdict = pBot->pEdict;

	edict_t *pEnemy=pBot->pBotEnemy;
	float distance = v_enemy.Length();  // how far away is the enemy?
	// Don't shoot through TeamMates !
	if(FireHurtsFriend(pBot,distance))
	{
#ifdef DEBUG
	report_log (LOG_BOTCOMBAT, "BotFireWeapon","Canceled (Hurts Friend)");
#endif
		return FALSE;
	}

	iId = pBot->current_weapon.iId;
	
	// TODO: Ammo Check doesn't always work in CS !! It seems to be because
	// AmmoX and CurrentWeapon Messages don't work very well together
	if((pBot->current_weapon.iClip==0) && (weapon_defs[iId].iAmmo1 != -1) &&
		(pBot->current_weapon.iAmmo1>0))
	{
#ifdef DEBUG
	report_log (LOG_BOTCOMBAT, "BotFireWeapon","Canceled (Reloading)");
#endif

		if(pBot->bIsReloading)
			return FALSE;
		pEdict->v.button |= IN_RELOAD;	// reload
		pBot->bIsReloading = TRUE;
		return FALSE;
	}

	pBot->bIsReloading = FALSE;

	pSelect = &cs_weapon_select[0];
	pDelay = &cs_fire_delay[0];
	
	if(pBot->pBotEnemy!=NULL)
	{
		bool bUseKnife=FALSE;

		// Use Knife if near and good skill (l33t dude!)
		if(g_bJasonMode)
			bUseKnife=TRUE;
		if(pBot->bot_skill>80)
		{
			if((distance<100) && (pBot->bot_health>80) &&
			(pBot->bot_health>pEnemy->v.health) &&
			(!IsGroupOfEnemies(pBot,pEdict->v.origin)))
			{
				bUseKnife=TRUE;
			}
		}
		if(bUseKnife)
			goto WeaponSelectEnd;
	}
	
	// loop through all the weapons until terminator is found...
	while (pSelect[select_index].iId)
	{
		
		// is the bot NOT carrying this weapon?
		if ( !(pEdict->v.weapons & (1 << pSelect[select_index].iId)) )
		{
            select_index++;  // skip to next weapon
            continue;
		}   

		iId = pSelect[select_index].iId;

		// Check if it's the currently used weapon because
		// otherwise Ammo in Clip will be ignored
		if(iId == pBot->current_weapon.iId)
		{
			if(pBot->current_weapon.iClip > 0 || weapon_defs[iId].iAmmo1 == -1)
				iChosenWeaponIndex = select_index;
		}
		
		// is primary percent less than weapon primary percent AND
		// no ammo required for this weapon OR
		// enough ammo available to fire AND
		// the bot is far enough away to use primary fire AND
		// the bot is close enough to the enemy to use primary fire

		
		else if (((weapon_defs[iId].iAmmo1 == -1) ||
			(pBot->m_rgAmmo[weapon_defs[iId].iAmmo1] >=
			pSelect[select_index].min_primary_ammo)) &&
			(distance >= pSelect[select_index].primary_min_distance) &&
			(distance <= pSelect[select_index].primary_max_distance))
		{
            iChosenWeaponIndex = select_index;
		}
		select_index++;
	}
	select_index = iChosenWeaponIndex;

WeaponSelectEnd:
	iId = pSelect[select_index].iId;
#ifdef DEBUG
	report_log (LOG_BOTCOMBAT, "BotFireWeapon","Using Weapon %s",pSelect[select_index].weapon_name);
#endif
	
	// select this weapon if it isn't already selected
	if (pBot->current_weapon.iId != iId)
	{
		if(pBot->current_weapon.iId != CS_WEAPON_INSWITCH)
		{
			SelectWeaponByName(pBot, pSelect[select_index].weapon_name);
#ifdef DEBUG
			report_log (LOG_BOTCOMBAT, "BotFireWeapon","Selecting Weapon %s",pSelect[select_index].weapon_name);
#endif
			
			// Reset Burst Fire Variables
			pBot->fTimeLastFired = 0.0;
			pBot->fTimeFirePause = 0.0;
			pBot->iBurstShotsFired = 0;
		}
		return FALSE;
	}

	bool bUsesSniper=BotUsesSniper(pBot);

	if(bUsesSniper && pBot->f_zoomchecktime<gpGlobals->time)
	{
		// Check Distance for correct Sniper Zooming
		int iZoomMagnification;

		if(distance < 1500)
			iZoomMagnification = 1;
		else
			iZoomMagnification = 2;

		bool bZoomChange = FALSE;

		switch(iZoomMagnification)
		{
		case 1:
			if(pEdict->v.fov != 40)
				bZoomChange = TRUE;
			break;
			
		case 2:
			if(pEdict->v.fov < 15)
				bZoomChange = TRUE;
		}

		if(bZoomChange)
		{
			pEdict->v.button |= IN_ATTACK2;

#ifdef DEBUG
		report_log (LOG_BOTCOMBAT, "BotFireWeapon","Changing FOV - Curr=%f",
		pEdict->v.fov);
#endif
		}
		pBot->f_zoomchecktime=gpGlobals->time + 2.0;
	}

	
	if (pDelay[select_index].iId != iId)
	{
#ifdef DEBUG
	report_log (LOG_BOTCOMBAT, "BotFireWeapon","Canceled (Fire Delay Mismatch for Id %d)",iId);
#endif
		return FALSE;
	}

	// Need to care for burst fire ?
	if(distance < MIN_BURST_DISTANCE || pBot->f_blind_time > gpGlobals->time)
	{
		if(iId==CS_WEAPON_KNIFE)
		{
			if(distance<64)
			{
				if(RANDOM_LONG(1,100)>60)
					pEdict->v.button |= IN_ATTACK;  // use primary attack
				else
					pEdict->v.button |= IN_ATTACK2;  // use secondary attack
			}
		}
		else
		{
			// If Automatic Weapon, just press attack
			if (pSelect[select_index].primary_fire_hold)
					pEdict->v.button |= IN_ATTACK;
			// if not, toggle buttons
			else
			{
				if((pEdict->v.oldbuttons & IN_ATTACK) == 0)
					pEdict->v.button |= IN_ATTACK;
			}
		}
		pBot->f_shoot_time=gpGlobals->time;
	}
	else
	{
		if(BotDoFirePause(pBot, distance, &pDelay[select_index]))
			return FALSE;

		// Don't attack with knife over long distance
		if(iId==CS_WEAPON_KNIFE)
		{
			pBot->f_shoot_time=gpGlobals->time;
			return FALSE;
		}

		if (pSelect[select_index].primary_fire_hold)
		{
			pBot->f_shoot_time = gpGlobals->time;
			pEdict->v.button |= IN_ATTACK;  // use primary attack
		}
		else
		{
			pEdict->v.button |= IN_ATTACK;  // use primary attack
			
			int skill = abs((pBot->bot_skill/20)-5);
			float base_delay, min_delay, max_delay;
			
			base_delay = pDelay[select_index].primary_base_delay;
			min_delay = pDelay[select_index].primary_min_delay[skill];
			max_delay = pDelay[select_index].primary_max_delay[skill];
			
			pBot->f_shoot_time = gpGlobals->time + base_delay +
				RANDOM_FLOAT(min_delay, max_delay);
		}
	}
	return TRUE;
}


void BotFocusEnemy( bot_t *pBot )
{
	if(pBot->pBotEnemy == NULL)
		return;

	edict_t *pEdict = pBot->pEdict;

	// aim for the head and/or body
	Vector vecEnemy = BotBodyTarget( pBot->pBotEnemy, pBot );
	pBot->vecLookAt = vecEnemy;
	if(pBot->f_enemy_surprise_time > gpGlobals->time)
		return;

	vecEnemy = vecEnemy - GetGunPosition(pEdict);
	vecEnemy.z = 0;  // ignore z component (up & down)
	float f_distance = vecEnemy.Length();  // how far away is the enemy scum?

	if(f_distance < 128)
	{
		if(pBot->current_weapon.iId == CS_WEAPON_KNIFE)
		{
			if(f_distance < 80)
				pBot->bWantsToFire = TRUE;
		}
		else
			pBot->bWantsToFire = TRUE;
	}
	else
	{
		float flDot = GetShootingConeDeviation(pEdict,&pBot->vecEnemy);
		if(flDot < 0.90)
			pBot->bWantsToFire = FALSE;
		else
		{
			float flEnemyDot = GetShootingConeDeviation(pBot->pBotEnemy,&pEdict->v.origin);
			// Enemy faces Bot ?
			if(flEnemyDot >= 0.90)
				pBot->bWantsToFire = TRUE;
			else
			{
				if(flDot > 0.99)
					pBot->bWantsToFire = TRUE;
				else
					pBot->bWantsToFire = FALSE;
			}
		}
	}
}

// Does the (unwaypointed) attack movement
void BotDoAttackMovement( bot_t *pBot)
{
	// No enemy ? No need to do strafing
	if(pBot->pBotEnemy == NULL)
		return;

	pBot->dest_origin = pBot->pBotEnemy->v.origin;

	float f_distance;
	TraceResult tr;
	edict_t *pEdict = pBot->pEdict;
	Vector vecEnemy = pBot->vecLookAt;
	vecEnemy = vecEnemy - GetGunPosition(pEdict);
	vecEnemy.z = 0;  // ignore z component (up & down)
	f_distance = vecEnemy.Length();  // how far away is the enemy scum?

	if(pBot->fTimeWaypointMove + pBot->fTimeFrameInterval + 0.5 < gpGlobals->time)
	{
		int iId = pBot->current_weapon.iId;
		int iApproach;
		bool bUsesSniper = BotUsesSniper(pBot);
		
		// If suspecting Enemy stand still
		if(pBot->iStates & STATE_SUSPECTENEMY)
			iApproach = 49;
		// If reloading or VIP back off
		else if(pBot->bIsReloading || pBot->bIsVIP)
			iApproach = 29;
		else if(iId == CS_WEAPON_KNIFE) // Knife ?
			iApproach = 100;
		else 
		{
			iApproach = pBot->bot_health * pBot->fAgressionLevel;
			if(bUsesSniper && iApproach > 49)
				iApproach = 49;
		}
		
		if(iApproach<30)
		{
			pBot->f_move_speed = -pBot->f_max_speed;
			BotGetSafeTask(pBot)->iTask = TASK_SEEKCOVER;
			BotGetSafeTask(pBot)->bCanContinue = TRUE;
			BotGetSafeTask(pBot)->fDesire = TASKPRI_ATTACK + 1;
		}
		else if(iApproach<50)
			pBot->f_move_speed = 0.0;
		else
			pBot->f_move_speed = pBot->f_max_speed;
		
		if(f_distance < 96 && iId != CS_WEAPON_KNIFE)
			pBot->f_move_speed = -pBot->f_max_speed;
		
		bool bUsesRifle=BotUsesRifle(pBot);
		
		if(bUsesRifle)
		{
			if(pBot->f_lastfightstylecheck+3.0<gpGlobals->time)
			{
				int iRand=RANDOM_LONG(1,100);
				if(f_distance<500)
					pBot->byFightStyle=0;
				else if(f_distance<1024)
				{
					if(iRand<50)
						pBot->byFightStyle=0;
					else
						pBot->byFightStyle=1;
				}
				else
				{
					if(iRand<90)
						pBot->byFightStyle=1;
					else
						pBot->byFightStyle=0;
				}
				pBot->f_lastfightstylecheck=gpGlobals->time;
			}
		}
		else
			pBot->byFightStyle = 0;
		
		if(pBot->bot_skill>60 && pBot->byFightStyle == 0)
		{
			if(pBot->f_StrafeSetTime<gpGlobals->time)
			{
				Vector2D	vec2DirToPoint; 
				Vector2D	vec2RightSide;
				
				// to start strafing, we have to first figure out if the target is on the left side or right side
				UTIL_MakeVectors (pBot->pBotEnemy->v.v_angle );
				
				vec2DirToPoint = ( pEdict->v.origin - pBot->pBotEnemy->v.origin ).Make2D().Normalize();
				vec2RightSide = gpGlobals->v_right.Make2D().Normalize();
				
				if ( DotProduct ( vec2DirToPoint, vec2RightSide ) < 0 )
					pBot->byCombatStrafeDir=1;
				else
					pBot->byCombatStrafeDir=0;

				if(RANDOM_LONG(1,100) < 30)
					pBot->byCombatStrafeDir ^= 1;
				pBot->f_StrafeSetTime=gpGlobals->time+RANDOM_FLOAT(0.5,3.0);
			}

			if(pBot->byCombatStrafeDir == 0)
			{
				if(!BotCheckWallOnLeft(pBot))
					pBot->f_sidemove_speed=-pBot->f_max_speed;
				else
				{
					pBot->byCombatStrafeDir^=1;
					pBot->f_StrafeSetTime=gpGlobals->time+1.0;
				}
			}
			else
			{
				if(!BotCheckWallOnRight(pBot))
					pBot->f_sidemove_speed=pBot->f_max_speed;
				else
				{
					pBot->byCombatStrafeDir^=1;
					pBot->f_StrafeSetTime=gpGlobals->time+1.0;
				}
			}
			if(pBot->bot_skill>80)
			{
				if(pBot->f_jumptime+1.5 < gpGlobals->time && (pEdict->v.flags & FL_ONGROUND))
				{
					if(RANDOM_LONG(1,100)<5 && pEdict->v.velocity.Length2D() > 150)
						pEdict->v.button |= IN_JUMP;
				}
			}
		}
		else if(pBot->byFightStyle == 1)
		{
/*			Vector vecSource = pEdict->v.origin + pEdict->v.view_ofs;
			if (!(pEdict->v.flags & FL_DUCKING))
				vecSource.z -= 17.0;
			Vector vecDest = pBot->vecLookAt;
			// trace a line forward at duck height...
			UTIL_TraceLine( vecSource, vecDest, ignore_monsters,ignore_glass,
				pEdict->v.pContainingEntity, &tr);
			// if trace hit something, return FALSE
			if (tr.flFraction >= 1.0)
			{*/
				pEdict->v.button |= IN_DUCK;
				if(pBot->byCombatStrafeDir == 0)
				{
					if(!BotCheckWallOnLeft(pBot))
						pBot->f_sidemove_speed=-pBot->f_max_speed;
					else
						pBot->byCombatStrafeDir^=1;
				}
				else
				{
					if(!BotCheckWallOnRight(pBot))
						pBot->f_sidemove_speed=pBot->f_max_speed;
					else
						pBot->byCombatStrafeDir^=1;
				}
//			}
			// Don't run towards enemy
			if(pBot->f_move_speed>0)
				pBot->f_move_speed=0;
		}
	}
	else
	{
		if(pBot->bot_skill > 80)
		{
			if(pEdict->v.flags & FL_ONGROUND)
			{
				if(f_distance < 500)
				{
					if(RANDOM_LONG(1,100) < 5 && pEdict->v.velocity.Length2D() > 150)
						pEdict->v.button |= IN_JUMP;
				}
				else
					pEdict->v.button |= IN_DUCK;
			}
		}
	}

	if(pBot->bIsReloading)
		pBot->f_move_speed = -pBot->f_max_speed;


	if(!pBot->bInWater && !pBot->bOnLadder
		&& (pBot->f_move_speed != 0 || pBot->f_sidemove_speed != 0))
	{
		float fTimeRange = pBot->fTimeFrameInterval;
		Vector vecForward=(gpGlobals->v_forward*pBot->f_move_speed) * 0.2;
		Vector vecSide=(gpGlobals->v_right*pBot->f_sidemove_speed) * 0.2;
		Vector vecTargetPos=pEdict->v.origin+vecForward+vecSide+(pEdict->v.velocity * fTimeRange);
		if(IsDeadlyDrop(pBot,vecTargetPos))
		{
			pBot->f_sidemove_speed = -pBot->f_sidemove_speed;
			pBot->f_move_speed = -pBot->f_move_speed;
			pEdict->v.button &= ~IN_JUMP;
		}
	}
}


bool BotHasPrimaryWeapon(bot_t *pBot)
{
	bot_weapon_select_t *pSelect = &cs_weapon_select[7];
	int iWeapons=pBot->pEdict->v.weapons;
	// loop through all the weapons until terminator is found...
	while (pSelect->iId)
	{
		// is the bot carrying this weapon?
//		if ((pBot->bot_weapons & (1<<pSelect->iId)))
		if ((iWeapons & (1<<pSelect->iId)))
			return TRUE;
		pSelect++;
	}
	return FALSE;
}

bool BotUsesSniper(bot_t *pBot)
{
	int iId = pBot->current_weapon.iId;

	if(iId == CS_WEAPON_AWP
	|| iId == CS_WEAPON_G3SG1
	|| iId == CS_WEAPON_SCOUT
	|| iId == CS_WEAPON_SG550)
	{
		return TRUE;
	}
	else
		return FALSE;
}

bool BotUsesRifle(bot_t *pBot)
{
	int iId = pBot->current_weapon.iId;

	bot_weapon_select_t *pSelect = &cs_weapon_select[0];
	int iCount = 0;
	while (pSelect->iId)
	{
		if(iId == pSelect->iId)
			break;
		pSelect++;
		iCount++;
	}
	if((pSelect->iId) && (iCount>13))
		return TRUE;
	return FALSE;
}


int BotCheckGrenades(bot_t *pBot)
{
	int weapons = pBot->pEdict->v.weapons;
//	if(pBot->bot_weapons & (1<<CS_WEAPON_HEGRENADE))
	if(weapons & (1<<CS_WEAPON_HEGRENADE))
		return CS_WEAPON_HEGRENADE;
	else if (weapons & (1<<CS_WEAPON_FLASHBANG))
		return CS_WEAPON_FLASHBANG;
	else if (weapons & (1<<CS_WEAPON_SMOKEGRENADE))
		return CS_WEAPON_SMOKEGRENADE;
	return -1;
}

void BotSelectBestWeapon(bot_t *pBot)
{
	bot_weapon_select_t *pSelect = &cs_weapon_select[0];
	int select_index = 0;
	int iChosenWeaponIndex = 0;
	int iId;
	int iWeapons = pBot->pEdict->v.weapons;

	// loop through all the weapons until terminator is found...
	while (pSelect[select_index].iId)
	{
		
		// is the bot NOT carrying this weapon?
		if (!(iWeapons & (1<<pSelect[select_index].iId)))
		{
            select_index++;  // skip to next weapon
            continue;
		}   

		iId = pSelect[select_index].iId;
		
		// is primary percent less than weapon primary percent AND
		// no ammo required for this weapon OR
		// enough ammo available to fire AND
		// the bot is far enough away to use primary fire AND
		// the bot is close enough to the enemy to use primary fire
		
		if (((weapon_defs[iId].iAmmo1 == -1) ||
			(pBot->m_rgAmmo[weapon_defs[iId].iAmmo1] >=
			pSelect[select_index].min_primary_ammo)))
		{
            iChosenWeaponIndex=select_index;
		}
		select_index++;
	}
	iChosenWeaponIndex%=24;
	select_index=iChosenWeaponIndex;

	iId = pSelect[select_index].iId;
	
	// select this weapon if it isn't already selected
	if (pBot->current_weapon.iId != iId)
		SelectWeaponByName(pBot, pSelect[select_index].weapon_name);
}

int HighestWeaponOfEdict(edict_t *pEdict)
{
	bot_weapon_select_t *pSelect = &cs_weapon_select[0];
	int iWeapons = pEdict->v.weapons;
	int iNum = 0;
	int i = 0;
	// loop through all the weapons until terminator is found...
	while (pSelect->iId)
	{
		// is the bot carrying this weapon?
		if ((iWeapons & (1<<pSelect->iId)))
			iNum = i;
		i++;
		pSelect++;
	}
	return(iNum);
}

void SelectWeaponByName(bot_t *pBot,char* pszName)
{
	pBot->current_weapon.iId = CS_WEAPON_INSWITCH;
	pBot->fTimeWeaponSwitch = gpGlobals->time;
	UTIL_SelectItem(pBot->pEdict, pszName);
}

void SelectWeaponbyNumber(bot_t *pBot,int iNum)
{
	pBot->current_weapon.iId = CS_WEAPON_INSWITCH;
	pBot->fTimeWeaponSwitch = gpGlobals->time;
	UTIL_SelectItem(pBot->pEdict, (char*)&cs_weapon_select[iNum].weapon_name);
}


void BotCommandTeam(bot_t *pBot)
{
	// Prevent spamming
	if(pBot->fTimeTeamOrder + 5.0 < gpGlobals->time)
	{
		bool bMemberNear = FALSE;
		edict_t *pTeamEdict;
		edict_t *pEdict = pBot->pEdict;

		int iTeam = UTIL_GetTeam(pEdict);
		
		// Search Teammates seen by this Bot
		for (int ind = 0; ind < gpGlobals->maxClients; ind++)
		{
			if(ThreatTab[ind].IsUsed == FALSE
			|| ThreatTab[ind].IsAlive == FALSE
			|| ThreatTab[ind].iTeam != iTeam
			|| ThreatTab[ind].pEdict == pEdict)
				continue;
			pTeamEdict = ThreatTab[ind].pEdict;
			if(BotEntityIsVisible( pBot, pTeamEdict->v.origin))
			{
				bMemberNear = TRUE;
				break;
			}
		}
		
		// Has Teammates ?
		if(bMemberNear)
		{
			if(pBot->bot_personality == 1)
				BotPlayRadioMessage(pBot,RADIO_STORMTHEFRONT);
			else
				BotPlayRadioMessage(pBot,RADIO_FALLBACK);
		}
		else
			BotPlayRadioMessage(pBot,RADIO_TAKINGFIRE);
		pBot->fTimeTeamOrder = gpGlobals->time;
	}
}

bool IsGroupOfEnemies(bot_t *pBot,Vector vLocation)
{
	edict_t *pPlayer;
	edict_t *pEdict=pBot->pEdict;
	int iNumPlayers=0;
	float distance;

	int bot_team = UTIL_GetTeam(pEdict);

	// search the world for enemy players...
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if((ThreatTab[i].IsUsed==FALSE) || (ThreatTab[i].IsAlive==FALSE) ||
			(ThreatTab[i].pEdict=pEdict))
			continue;

		pPlayer=ThreatTab[i].pEdict;
		distance = (pPlayer->v.origin - vLocation).Length();
		if(distance<256)
		{
			// don't target our teammates...
			if(ThreatTab[i].iTeam==bot_team)
				return FALSE;
			iNumPlayers++;
		}
	}
#ifdef DEBUG
	report_log (LOG_BOTCOMBAT, "IsGroupOfEnemies","Enemies=%d",iNumPlayers);
#endif

	if(iNumPlayers>1)
		return TRUE;
	return FALSE;
}



//
// VecCheckToss - returns the velocity at which an object should be lobbed from vecspot1 to land near vecspot2.
// returns g_vecZero if toss is not feasible.
// 
Vector VecCheckToss ( edict_t *pEdict, const Vector &vecSpot1, Vector vecSpot2)
{
	float flGravityAdj = 0.5;
	TraceResult		tr;
	Vector			vecMidPoint;// halfway point between Spot1 and Spot2
	Vector			vecApex;// highest point 
	Vector			vecScale;
	Vector			vecGrenadeVel;
	Vector			vecTemp;
	float			flGravity = CVAR_GET_FLOAT( "sv_gravity" ) * flGravityAdj;

	vecSpot2 = vecSpot2 - pEdict->v.velocity;
	vecSpot2.z -= 15.0;
    if (vecSpot2.z - vecSpot1.z > 500)
	{
		// to high, fail
		return g_vecZero;
	}

	
	// calculate the midpoint and apex of the 'triangle'
	// UNDONE: normalize any Z position differences between spot1 and spot2 so that triangle is always RIGHT

	// How much time does it take to get there?

	// get a rough idea of how high it can be thrown
	vecMidPoint = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	UTIL_TraceLine( vecMidPoint, vecMidPoint + Vector(0,0,500), ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	if (tr.flFraction != 1.0)
	{
		vecMidPoint = tr.vecEndPos;
		vecMidPoint.z = tr.pHit->v.absmin.z;
		vecMidPoint.z--;
	}

	if (vecMidPoint.z < vecSpot2.z)
	{
		// to not enough space, fail
		return g_vecZero;
	}

	// How high should the grenade travel to reach the apex
	float distance1 = (vecMidPoint.z - vecSpot1.z);
	float distance2 = (vecMidPoint.z - vecSpot2.z);

	// How long will it take for the grenade to travel this distance
	float time1 = sqrt( distance1 / (0.5 * flGravity) );
	float time2 = sqrt( distance2 / (0.5 * flGravity) );

	if (time1 < 0.1)
	{
		// too close
		return g_vecZero;
	}

	// how hard to throw sideways to get there in time.
	vecGrenadeVel = (vecSpot2 - vecSpot1) / (time1 + time2);
	// how hard upwards to reach the apex at the right time.
	vecGrenadeVel.z = flGravity * time1;

	// find the apex
	vecApex  = vecSpot1 + vecGrenadeVel * time1;
	vecApex.z = vecMidPoint.z;

	UTIL_TraceLine( vecSpot1, vecApex, dont_ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	if (tr.flFraction != 1.0 || tr.fAllSolid)
	{
		// fail!
		return g_vecZero;
	}

	UTIL_TraceLine( vecSpot2, vecApex, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	if (tr.flFraction != 1.0)
	{
		Vector vecDir = (vecApex - vecSpot2).Normalize();
		float n = -DotProduct(tr.vecPlaneNormal, vecDir);
		if (n > 0.7 || tr.flFraction < 0.8) // 60 degrees
			return g_vecZero;
	}
	
	return vecGrenadeVel * 0.6667;
}


//
// VecCheckThrow - returns the velocity vector at which an object should be thrown from vecspot1 to hit vecspot2.
// returns g_vecZero if throw is not feasible.
// 
Vector VecCheckThrow ( edict_t *pEdict, const Vector &vecSpot1, Vector vecSpot2, float flSpeed)
{
	float flGravityAdj = 1.0;
	float flGravity = CVAR_GET_FLOAT( "sv_gravity" ) * flGravityAdj;

	vecSpot2 = vecSpot2 - pEdict->v.velocity;
	Vector vecGrenadeVel = (vecSpot2 - vecSpot1);

	// throw at a constant time
//	float time = vecGrenadeVel.Length( ) / flSpeed;
	float time = 1.0;
	vecGrenadeVel = vecGrenadeVel * (1.0 / time);

	// adjust upward toss to compensate for gravity loss
	vecGrenadeVel.z += flGravity * time * 0.5;

	Vector vecApex = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	vecApex.z += 0.5 * flGravity * (time * 0.5) * (time * 0.5);
	
	TraceResult tr;
	UTIL_TraceLine( vecSpot1, vecApex, dont_ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	if (tr.flFraction != 1.0 || tr.fAllSolid)
	{
		// fail!
		return g_vecZero;
	}

	UTIL_TraceLine( vecSpot2, vecApex, ignore_monsters,
		pEdict->v.pContainingEntity, &tr);
	if (tr.flFraction != 1.0)
	{
		Vector vecDir = (vecApex - vecSpot2).Normalize();
		float n = -DotProduct(tr.vecPlaneNormal, vecDir);
		if (n > 0.7 || tr.flFraction < 0.8) // 60 degrees
			return g_vecZero;
	}

	return vecGrenadeVel * 0.6667;
}



