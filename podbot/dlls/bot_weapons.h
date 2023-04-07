//
// bot_weapons.h
//

#ifndef BOT_WEAPONS_H
#define BOT_WEAPONS_H

// Set by me to flag in-between weapon switching
#define CS_WEAPON_INSWITCH		255

// weapon ID values for Counter-Strike
#define CS_WEAPON_P228           1
#define CS_WEAPON_UNKNOWN1       2
#define CS_WEAPON_SCOUT          3
#define CS_WEAPON_HEGRENADE      4
#define CS_WEAPON_XM1014         5
#define CS_WEAPON_C4             6
#define CS_WEAPON_MAC10          7
#define CS_WEAPON_AUG            8
#define CS_WEAPON_SMOKEGRENADE   9
// new for CS 7.0
#define CS_WEAPON_ELITE         10

#define CS_WEAPON_FIVESEVEN     11
#define CS_WEAPON_UMP45			12
#define CS_WEAPON_SG550			13
#define CS_WEAPON_UNKNOWN6      14
#define CS_WEAPON_UNKNOWN7      15
#define CS_WEAPON_USP           16
#define CS_WEAPON_GLOCK18       17
#define CS_WEAPON_AWP           18
#define CS_WEAPON_MP5NAVY       19
#define CS_WEAPON_M249          20
#define CS_WEAPON_M3            21
#define CS_WEAPON_M4A1          22
#define CS_WEAPON_TMP           23
#define CS_WEAPON_G3SG1         24
#define CS_WEAPON_FLASHBANG     25
#define CS_WEAPON_DEAGLE        26
#define CS_WEAPON_SG552         27
#define CS_WEAPON_AK47          28
#define CS_WEAPON_KNIFE         29
#define CS_WEAPON_P90           30

#define MIN_BURST_DISTANCE	512.0


typedef struct
{
   char szClassname[64];
   int  iAmmo1;     // ammo index for primary ammo
   int  iAmmo1Max;  // max primary ammo
   int  iAmmo2;     // ammo index for secondary ammo
   int  iAmmo2Max;  // max secondary ammo
   int  iSlot;      // HUD slot (0 based)
   int  iPosition;  // slot position
   int  iId;        // weapon ID
   int  iFlags;     // flags???
} bot_weapon_t;



typedef struct
{
   int iId;
   float primary_base_delay;
   float primary_min_delay[6];
   float primary_max_delay[6];
   float secondary_base_delay;
   float secondary_min_delay[5];
   float secondary_max_delay[5];
   int	 iMaxFireBullets;
   float fMinBurstPauseFactor;
   
} bot_fire_delay_t;

#endif // BOT_WEAPONS_H

