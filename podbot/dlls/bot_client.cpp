//
// bot_client.cpp
//
// Handles Messages sent from the Server to a
// Client Bot
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_client.h"
#include "log.h"
#include "bot_weapons.h"
#include "waypoint.h"
#include "bot_globals.h"

// types of damage to ignore...
// Not used for CS because they seem to be ignored...
/*#define IGNORE_DAMAGE (DMG_CRUSH | DMG_FREEZE | DMG_SHOCK | DMG_DROWN | \
                       DMG_NERVEGAS | DMG_RADIATION | DMG_DROWNRECOVER | \
                       DMG_ACID | DMG_SLOWBURN | DMG_SLOWFREEZE)*/

bot_weapon_t weapon_defs[MAX_WEAPONS]; // array of weapon definitions

static FILE *fp;

extern bot_t bots[MAXBOTSNUMBER];   // max of 32 bots in a game

// This message is sent when the VGUI menu is displayed.
void BotClient_CS_VGUI(void *p, int bot_index)
{
   if(state == 0)
   {

#ifdef DEBUG
	report_log (LOG_MESSAGE, "BotClient_CS_VGUI","Botname=%s Param=%d",bots[bot_index].name,*(int *)p);
#endif
	   
	   if ((*(int *)p) == 2)  // is it a team select menu?
		   bots[bot_index].start_action = MSG_CS_TEAM_SELECT;
	   else if ((*(int *)p) == 26)  // is it a Terrorist class selection menu?
		   bots[bot_index].start_action = MSG_CS_T_SELECT;
	   else if ((*(int *)p) == 27)  // is it a CT class selection menu?
		   bots[bot_index].start_action = MSG_CS_CT_SELECT;
	   
	   else if ((*(int *)p) == 28)  // is it a Buy selection menu?
	   {
		   if(bots[bot_index].iBuyCount==1)
				bots[bot_index].BuyAction=MSG_CS_BUYWEAPON_SELECT;
		   else if(bots[bot_index].iBuyCount==3)
				bots[bot_index].BuyAction=MSG_CS_BUYAMMO;
		   else if((bots[bot_index].iBuyCount==4) || (bots[bot_index].iBuyCount==6))
				bots[bot_index].BuyAction=MSG_CS_BUYITEM;
	   }
	   else if ((*(int *)p) == 29)  // is it a Buy Pistol selection menu?
		   bots[bot_index].BuyAction=MSG_CS_WEAPONBUY;
	   else if ((*(int *)p) == 30)  // is it a Buy Shotgun selection menu?
		   bots[bot_index].BuyAction=MSG_CS_WEAPONBUY;
	   else if ((*(int *)p) == 32)  // is it a Buy Submachine selection menu?
		   bots[bot_index].BuyAction=MSG_CS_WEAPONBUY;
	   else if ((*(int *)p) == 31)  // is it a Buy Rifle selection menu?
		   bots[bot_index].BuyAction=MSG_CS_WEAPONBUY;
	   else if ((*(int *)p) == 33)  // is it a Buy Machinegun selection menu?
		   bots[bot_index].BuyAction=MSG_CS_WEAPONBUY;
	   else if ((*(int *)p) == 34)  // is it a Buy Item selection menu?
		   bots[bot_index].BuyAction=MSG_CS_BUYITEM_SELECT;
   }
   state++;
}

// This message is sent when a menu is being displayed.
void BotClient_CS_ShowMenu(void *p, int bot_index)
{
   if (state < 3)
   {
      state++;  // ignore first 3 fields of message
      return;
   }

#ifdef DEBUG
	report_log (LOG_MESSAGE, "BotClient_CS_ShowMenu","Botname=%s Menu=%s",bots[bot_index].name,p);
#endif
   if (strcmp((char *)p, "#Team_Select") == 0)  // team select menu?
   {
      bots[bot_index].start_action = MSG_CS_TEAM_SELECT;
   }
   else if (strcmp((char *)p, "#CT_Select") == 0)  // CT model select menu?
   {
      bots[bot_index].start_action = MSG_CS_CT_SELECT;
   }
   else if (strcmp((char *)p, "#Terrorist_Select") == 0)  // T model select?
   {
      bots[bot_index].start_action = MSG_CS_T_SELECT;
   }

   // Buy Messages
   else if (strcmp((char *)p, "#Buy") == 0)  // Issued Buy Command
   {
		   if(bots[bot_index].iBuyCount==1)
				bots[bot_index].BuyAction=MSG_CS_BUYWEAPON_SELECT;
		   else if(bots[bot_index].iBuyCount==3)
				bots[bot_index].BuyAction=MSG_CS_BUYAMMO;
		   else if((bots[bot_index].iBuyCount==4) || (bots[bot_index].iBuyCount==6))
				bots[bot_index].BuyAction=MSG_CS_BUYITEM;
   }
   else if (strcmp((char *)p, "#BuyPistol") == 0)  // Bot wants a Pistol
   {
	   bots[bot_index].BuyAction=MSG_CS_WEAPONBUY;
   }
   else if (strcmp((char *)p, "#BuyShotgun") == 0)  // Bot wants a Shotgun
   {
	   bots[bot_index].BuyAction=MSG_CS_WEAPONBUY;
   }
   else if (strcmp((char *)p, "#T_BuySubMachineGun") == 0)  // Terrorist Bot wants a SMG
   {
	   bots[bot_index].BuyAction=MSG_CS_WEAPONBUY;
   }
   else if (strcmp((char *)p, "#CT_BuySubMachineGun") == 0)  // Counter Terrorist Bot wants a SMG
   {
	   bots[bot_index].BuyAction=MSG_CS_WEAPONBUY;
   }
   else if (strcmp((char *)p, "#T_BuyRifle") == 0)  // Terrorist Bot wants a Rifle
   {
	   bots[bot_index].BuyAction=MSG_CS_WEAPONBUY;
   }
   else if (strcmp((char *)p, "#CT_BuyRifle") == 0)  // Counter Terrorist Bot wants a Rifle
   {
	   bots[bot_index].BuyAction=MSG_CS_WEAPONBUY;
   }
   else if (strcmp((char *)p, "#BuyMachinegun") == 0)  // Counter Terrorist Bot wants a Rifle
   {
	   bots[bot_index].BuyAction=MSG_CS_WEAPONBUY;
   }
   else if (strcmp((char *)p, "#BuyItem") == 0)  // All wanted Items (Grenades)
   {
	   bots[bot_index].BuyAction=MSG_CS_BUYITEM_SELECT;
   }

	// Radio Messages
   else if (strcmp((char *)p, "#RadioA") == 0)
   {
      BotPushMessageQueue(&bots[bot_index],MSG_CS_RADIO_SELECT);
   }
   else if (strcmp((char *)p, "#RadioB") == 0)
   {
      BotPushMessageQueue(&bots[bot_index],MSG_CS_RADIO_SELECT);
   }
   else if (strcmp((char *)p, "#RadioC") == 0)
   {
      BotPushMessageQueue(&bots[bot_index],MSG_CS_RADIO_SELECT);
   }
   state++;
}

// This message is sent when a client joins the game.  All of the weapons
// are sent with the weapon ID and information about what ammo is used.
void BotClient_CS_WeaponList(void *p, int bot_index)
{
   static bot_weapon_t bot_weapon;

   if (state == 0)
   {
      strcpy(bot_weapon.szClassname, (char *)p);
   }
   else if (state == 1)
   {
      bot_weapon.iAmmo1 = *(int *)p;  // ammo index 1
   }
   else if (state == 2)
   {
      bot_weapon.iAmmo1Max = *(int *)p;  // max ammo1
   }
   else if (state == 3)
   {
      bot_weapon.iAmmo2 = *(int *)p;  // ammo index 2
   }
   else if (state == 4)
   {
      bot_weapon.iAmmo2Max = *(int *)p;  // max ammo2
   }
   else if (state == 5)
   {
      bot_weapon.iSlot = *(int *)p;  // slot for this weapon
   }
   else if (state == 6)
   {
      bot_weapon.iPosition = *(int *)p;  // position in slot
   }
   else if (state == 7)
   {
      bot_weapon.iId = *(int *)p;  // weapon ID
   }
   else if (state == 8)
   {
      bot_weapon.iFlags = *(int *)p;  // flags for weapon (WTF???)

      // store away this weapon with it's ammo information...
#ifdef DEBUG
	report_log (LOG_MESSAGE, "BotClient_CS_WeaponList","Botname=%s\n WeaponName=%s WeaponiId=%d",bots[bot_index].name,
		bot_weapon.szClassname,bot_weapon.iId);
#endif
      weapon_defs[bot_weapon.iId] = bot_weapon;

   }
   state++;
}



// This message is sent when a weapon is selected (either by the bot chosing
// a weapon or by the server auto assigning the bot a weapon).
// In CS it's also called when Ammo is increased/decreased
void BotClient_CS_CurrentWeapon(void *p, int bot_index)
{
	static int iState;
	static int iId;
	static int iClip;
	
	bot_t *pBot = &bots[bot_index];

	if (state == 0)
	{
		iState = *(int *)p;  // state of the current weapon (WTF???)
	}
	else if (state == 1)
	{
		iId = *(int *)p;  // weapon ID of current weapon
	}
	else if (state == 2)
	{
		iClip = *(int *)p;  // ammo currently in the clip for this weapon
		
		if (iId <= 31)
		{
#ifdef DEBUG
			report_log (LOG_MESSAGE, "BotClient_CS_CurrentWeapon","Botname=%s iId=%d iClip=%d",pBot->name,iId,iClip);
#endif
			
			// set this weapon bit
			// not needed for CS
			//bots[bot_index].bot_weapons |= (1<<iId);
			if (iState == 1)
			{
				
				// Ammo amount decreased ? Must have fired a bullet...
				if(iId == pBot->current_weapon.iId
				&& pBot->current_weapon.iClip >	iClip)
				{
					// Time fired withing burst firing time ?
					if(pBot->fTimeLastFired + 1.0 > gpGlobals->time)
						pBot->iBurstShotsFired++;
					// Remember the last bullet time 
					pBot->fTimeLastFired = gpGlobals->time;
				}
				pBot->current_weapon.iId = iId;
				pBot->current_weapon.iClip = iClip;
				
				// update the ammo counts for this weapon...
				pBot->current_weapon.iAmmo1 =
					pBot->m_rgAmmo[weapon_defs[iId].iAmmo1];
				pBot->current_weapon.iAmmo2 =
					pBot->m_rgAmmo[weapon_defs[iId].iAmmo2];
			}
		}
	}
	state++;
}



// This message is sent whenever ammo ammounts are adjusted (up or down).
// NOTE: Logging reveals that CS uses it very unreliable !
void BotClient_CS_AmmoX(void *p, int bot_index)
{
	static int index;
	static int ammount;
	int ammo_index;
	
	bot_t *pBot = &bots[bot_index];
	
	if (state == 0)
	{
		index = *(int *)p;  // ammo index (for type of ammo)
	}
	else if (state == 1)
	{
		ammount = *(int *)p;  // the ammount of ammo currently available
		
		pBot->m_rgAmmo[index] = ammount;  // store it away
		if(pBot->current_weapon.iId > CS_WEAPON_INSWITCH)
		{
			ammo_index = pBot->current_weapon.iId;
			
#ifdef DEBUG
			report_log (LOG_MESSAGE, "BotClient_CS_AmmoX","Botname=%s iId=%d Amount=%d",pBot->name,ammo_index,ammount);
#endif
			
			// update the ammo counts for this weapon...
			pBot->current_weapon.iAmmo1 =
				pBot->m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
			pBot->current_weapon.iAmmo2 =
				pBot->m_rgAmmo[weapon_defs[ammo_index].iAmmo2];
		}
		
	}
	state++;
}



// This message is sent when the bot picks up some ammo (AmmoX messages are
// also sent so this message is probably not really necessary except it
// allows the HUD to draw pictures of ammo that have been picked up.  The
// bots don't really need pictures since they don't have any eyes anyway.
void BotClient_CS_AmmoPickup(void *p, int bot_index)
{
   static int index;
   static int ammount;
   int ammo_index;

   if (state == 0)
   {
      index = *(int *)p;
   }
   else if (state == 1)
   {
      ammount = *(int *)p;

      bots[bot_index].m_rgAmmo[index] = ammount;

      ammo_index = bots[bot_index].current_weapon.iId;

#ifdef DEBUG
	report_log (LOG_MESSAGE, "BotClient_CS_AmmoPickup","Botname=%s iId=%d Amount=%d",bots[bot_index].name,ammo_index,ammount);
#endif
      // update the ammo counts for this weapon...
      bots[bot_index].current_weapon.iAmmo1 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
      bots[bot_index].current_weapon.iAmmo2 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo2];

   }
   state++;
}



// This message gets sent when the bot picks up a weapon.
void BotClient_CS_WeaponPickup(void *p, int bot_index)
{
   int index;

   index = *(int *)p;

#ifdef DEBUG
	report_log (LOG_MESSAGE, "BotClient_CS_WeaponPickup","Botname=%s iId=%d",bots[bot_index].name,index);
#endif
   // set this weapon bit to indicate that we are carrying this weapon
//   bots[bot_index].bot_weapons |= (1<<index);
}


void BotClient_CS_ItemPickup(void *p, int bot_index)
{
}


// This message gets sent when the bots health changes.
void BotClient_CS_Health(void *p, int bot_index)
{
   bots[bot_index].bot_health = *(int *)p;  // health ammount
   if(bots[bot_index].bot_health == 100)
	   bots[bot_index].bDead = FALSE;
#ifdef DEBUG
	report_log (LOG_MESSAGE, "BotClient_CS_Health","Botname=%s health=%d",bots[bot_index].name,*(int *)p);
#endif
}



// This message gets sent when the bots armor changes.
void BotClient_CS_Battery(void *p, int bot_index)
{
   bots[bot_index].bot_armor = *(int *)p;  // armor ammount
#ifdef DEBUG
	report_log (LOG_MESSAGE, "BotClient_CS_Battery","Botname=%s Armor=%d",bots[bot_index].name,*(int *)p);
#endif
}


// This message gets sent when the bots are getting damaged.
void BotClient_CS_Damage(void *p, int bot_index)
{
   static int damage_armor;
   static int damage_taken;
   static int damage_bits;
   static Vector damage_origin;

   if (state == 0)
   {
      damage_armor = *(int *)p;
   }
   else if (state == 1)
   {
      damage_taken = *(int *)p;
   }
   else if (state == 2)
   {
      damage_bits = *(int *)p;
   }
   else if (state == 3)
   {
      damage_origin.x = *(float *)p;
   }
   else if (state == 4)
   {
      damage_origin.y = *(float *)p;
   }
   else if (state == 5)
   {
      damage_origin.z = *(float *)p;

#ifdef DEBUG
	report_log (LOG_MESSAGE, "BotClient_CS_Damage","Botname=%s damage_taken=%d damage_armor=%d",bots[bot_index].name,damage_taken,damage_armor);
#endif
      if ((damage_armor > 0) || (damage_taken > 0))
      {
		  bot_t *pBot=&bots[bot_index];
		  pBot->iLastDamageType = damage_bits;
		  edict_t *pEdict=pBot->pEdict;

		  int iTeam=UTIL_GetTeam(pEdict);

		  BotCollectGoalExperience(pBot,damage_taken,iTeam);

		  edict_t *pEnt = pEdict->v.dmg_inflictor;
          if(pEnt->v.flags & (FL_CLIENT | FL_FAKECLIENT))
		  {
			  if(UTIL_GetTeam(pEnt) == iTeam)
			  {
				  if(RANDOM_LONG(1,100)<10)
				  {
//					  sprintf(pBot->szMiscStrings,"%s is a Teamkiller ! Admin kick him!",STRING(pEnt->v.netname));
//					  BotPrepareChatMessage(pBot,"%t is a Teamkiller ! Admin kick him!");
//					  BotPushMessageQueue(pBot,MSG_CS_SAY);
					  if (pBot->pBotEnemy == NULL && pBot->f_bot_see_enemy_time+2.0<gpGlobals->time)
					  {
						  pBot->f_bot_see_enemy_time = gpGlobals->time;
						  pBot->pBotEnemy=pEnt;
						  pBot->pLastEnemy=pEnt;
						  pBot->vecLastEnemyOrigin = pEnt->v.origin;
					  }
				  }
			  }
			  else
			  {
				  if(pBot->bot_health>70)
				  {
					  pBot->fAgressionLevel+=0.1;
					  if(pBot->fAgressionLevel>1.0)
						  pBot->fAgressionLevel=1.0;
				  }
				  else
				  {
					  pBot->fFearLevel+=0.05;
					  if(pBot->fFearLevel>1.0)
						  pBot->fFearLevel=1.0;
				  }
				  // Stop Bot from Hiding
	              BotRemoveCertainTask(pBot,TASK_HIDE);

				  if (pBot->pBotEnemy == NULL)
				  {
					  pBot->pLastEnemy = pEnt;
					  pBot->vecLastEnemyOrigin = pEnt->v.origin;
					  // FIXME - Bot doesn't necessary sees this enemy
					  pBot->f_bot_see_enemy_time = gpGlobals->time;
				  }
				  BotCollectExperienceData(pEdict,pEnt,damage_armor+damage_taken);
			  }
		  }
		  // Check old waypoint
		  else
		  {
			  if(!WaypointReachable(pEdict->v.origin, pBot->dest_origin, pBot->pEdict))
			  {
				  DeleteSearchNodes(pBot);
				  BotFindWaypoint(pBot);
			  }
		  }
      }
   }
   state++;
}



// This message gets sent when the bots money ammount changes
void BotClient_CS_Money(void *p, int bot_index)
{
   if (state == 0)
   {
      bots[bot_index].bot_money = *(int *)p;  // amount of money
#ifdef DEBUG
	report_log (LOG_MESSAGE, "BotClient_CS_Money","Botname=%s Money=%d",bots[bot_index].name,*(int *)p);
#endif
   }
   state++;
}


// This message gets sent when the HUD Status Icon changes
void BotClient_CS_StatusIcon(void *p, int bot_index)
{
	static unsigned char byEnable;

	if(state == 0)
		byEnable = *(unsigned char *)p;
	else if(state == 1)
	{
#ifdef DEBUG
		report_log (LOG_MESSAGE, "BotClient_CS_StatusIcon","Botname=%s Name=%s byEnable=%d",bots[bot_index].name,p,byEnable);
#endif
		if(strcmp((char *)p,"buyzone") == 0)
		{
			bots[bot_index].bInBuyzone=TRUE;
		}
	}
	state++;
}


// This message gets sent when someone got killed
void BotClient_CS_DeathMsg(void *p, int bot_index)
{
	static int killer_index;
	static int victim_index;
	static edict_t *killer_edict;
	static int index;
	
	if (state == 0)
		killer_index = *(int *)p;  // ENTINDEX() of killer
	else if (state == 1)
		victim_index = *(int *)p;  // ENTINDEX() of victim
	else if (state == 2)
	{
#ifdef DEBUG
	report_log (LOG_MESSAGE, "BotClient_CS_DeathMsg","Botname=%s Killer=%d Victim=%d",bots[bot_index].name,killer_index,victim_index);
#endif
		if ((killer_index != 0) && (killer_index != victim_index))
		{
			killer_edict = INDEXENT(killer_index);
			index = UTIL_GetBotIndex(killer_edict);
			// is this message about a bot who killed somebody ?
			if (index != -1)
				bots[index].pLastVictim=INDEXENT(victim_index);
			else // Did a human kill a Bot on his team ?
			{
				edict_t *victim_edict=INDEXENT(victim_index);
				index = UTIL_GetBotIndex(victim_edict);
				if(index != -1)
				{
					if(UTIL_GetTeam(killer_edict)==UTIL_GetTeam(victim_edict))
						  bots[index].iVoteKickIndex=ENTINDEX(killer_edict);
					bots[index].bDead = TRUE;
				}
			}
		}
	}
    state++;
}


// This message gets sent when the Screen fades (Flashbang)
void BotClient_CS_ScreenFade(void *p, int bot_index)
{
	static unsigned char r;
	static unsigned char g;
	static unsigned char b;
	
	if (state == 3)
		r = *(unsigned char *)p;
	else if (state == 4)
		g = *(unsigned char *)p;
	else if (state == 5)
		b = *(unsigned char *)p;
	else if (state == 6)
	{
		unsigned char alpha=*(unsigned char *)p;
#ifdef DEBUG
	report_log (LOG_MESSAGE, "BotClient_CS_ScreenFade","Botname=%s r=%d g=%d b=%d a=%d",bots[bot_index].name,r,g,b,alpha);
#endif
		if (r==255 && g==255 && b==255 && alpha>200)
		{
			bot_t *pBot=&bots[bot_index];
			pBot->pBotEnemy=NULL;
			pBot->f_view_distance=1;
			// About 3 seconds
			pBot->f_blind_time = gpGlobals->time + ((float)alpha - 200.0) / 15;
			if(pBot->bot_skill < 50)
			{
				pBot->f_blindmovespeed_forward = 0.0;
				pBot->f_blindmovespeed_side = 0.0;
			}
			else if(pBot->bot_skill < 80)
			{
				pBot->f_blindmovespeed_forward = -pBot->f_max_speed;
				pBot->f_blindmovespeed_side = 0.0;
			}
			else
			{
				if(RANDOM_LONG(1,100)<50)
				{
					if(RANDOM_LONG(1,100)<50)
						pBot->f_blindmovespeed_side = pBot->f_max_speed;
					else
						pBot->f_blindmovespeed_side = -pBot->f_max_speed;
				}
				else
				{
					if(pBot->bot_health>80)
						pBot->f_blindmovespeed_forward = pBot->f_max_speed;
					else
						pBot->f_blindmovespeed_forward = -pBot->f_max_speed;
				}
			}
		}
	}
	state++;
}


void BotClient_CS_SayText(void *p, int bot_index)
{
	static unsigned char ucEntIndex;

	if (state == 0)
	{
		ucEntIndex = *(unsigned char *)p;
	}
	else if (state == 1)
	{
		bot_t *pBot=&bots[bot_index];
		if(ENTINDEX(pBot->pEdict) != ucEntIndex)
		{
			pBot->SaytextBuffer.iEntityIndex = (int)ucEntIndex;
			strcpy(pBot->SaytextBuffer.szSayText,(char *)p);
			pBot->SaytextBuffer.fTimeNextChat = gpGlobals->time + pBot->SaytextBuffer.fChatDelay;
		}
	}
	state++;
}

// This message gets sent when the MOTD gets displayed
void BotClient_CS_MOTD(void *p, int bot_index)
{
    bots[bot_index].start_action = MSG_CS_MOTD;
}


	// This message gets sent when a round is won
/*void BotClient_CS_TeamScore(void *p, int bot_index)
{
	static unsigned short CTScore;
	static unsigned short TScore;
	static bool bIsCTScore;
	
	if (state == 0)
	{
		if(FStrEq((char *)p,"CT"))
			bIsCTScore=TRUE;
		else
			bIsCTScore=FALSE;
	}
	else if(state == 1)
	{
		if(bIsCTScore)
			CTScore=*(unsigned short *)p;
		else
			TScore=*(unsigned short *)p;
	}
    state++;
}*/
