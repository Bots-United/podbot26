//
// linkfunc.cpp
//
// Links Entities to Functions
//

#include "extdll.h"

#ifndef __linux__
extern HINSTANCE h_Library;
#else
extern void *h_Library;
#endif

typedef void (FAR *LINK_ENTITY_FUNC)(entvars_t *);


#define LINK_ENTITY_TO_FUNC(mapClassName, mapClassString) \
 extern "C" _declspec( dllexport ) void mapClassName( entvars_t *pev ); \
 void mapClassName( entvars_t *pev ) { \
      static LINK_ENTITY_FUNC otherClassName = NULL; \
      static int skip_this = 0; \
      if (skip_this) return; \
      if (otherClassName == NULL) \
         otherClassName = (LINK_ENTITY_FUNC)GetProcAddress(h_Library, mapClassString); \
      if (otherClassName == NULL) { \
         skip_this = 1; return; \
      } \
      (*otherClassName)(pev); }


// Entities for CounterStrike (from CS 6.5 to CS 1.3 beta)
LINK_ENTITY_TO_FUNC(ambient_generic, "ambient_generic");
LINK_ENTITY_TO_FUNC(ammo_338magnum, "ammo_338magnum");
LINK_ENTITY_TO_FUNC(ammo_357sig, "ammo_357sig");
LINK_ENTITY_TO_FUNC(ammo_45acp, "ammo_45acp");
LINK_ENTITY_TO_FUNC(ammo_50ae, "ammo_50ae");
LINK_ENTITY_TO_FUNC(ammo_556nato, "ammo_556nato");
LINK_ENTITY_TO_FUNC(ammo_556natobox, "ammo_556natobox");
LINK_ENTITY_TO_FUNC(ammo_57mm, "ammo_57mm");
LINK_ENTITY_TO_FUNC(ammo_762nato, "ammo_762nato");
LINK_ENTITY_TO_FUNC(ammo_9mm, "ammo_9mm");
LINK_ENTITY_TO_FUNC(ammo_buckshot, "ammo_buckshot");
LINK_ENTITY_TO_FUNC(armoury_entity, "armoury_entity");
LINK_ENTITY_TO_FUNC(beam, "beam");
LINK_ENTITY_TO_FUNC(bodyque, "bodyque");
LINK_ENTITY_TO_FUNC(button_target, "button_target");
LINK_ENTITY_TO_FUNC(cycler, "cycler");
LINK_ENTITY_TO_FUNC(cycler_prdroid, "cycler_prdroid");
LINK_ENTITY_TO_FUNC(cycler_sprite, "cycler_sprite");
LINK_ENTITY_TO_FUNC(cycler_weapon, "cycler_weapon");
LINK_ENTITY_TO_FUNC(cycler_wreckage, "cycler_wreckage");
LINK_ENTITY_TO_FUNC(DelayedUse, "DelayedUse");
LINK_ENTITY_TO_FUNC(env_beam, "env_beam");
LINK_ENTITY_TO_FUNC(env_beverage, "env_beverage");
LINK_ENTITY_TO_FUNC(env_blood, "env_blood");
LINK_ENTITY_TO_FUNC(env_bombglow, "env_bombglow");
LINK_ENTITY_TO_FUNC(env_bubbles, "env_bubbles");
LINK_ENTITY_TO_FUNC(env_debris, "env_debris");
LINK_ENTITY_TO_FUNC(env_explosion, "env_explosion");
LINK_ENTITY_TO_FUNC(env_fade, "env_fade");
LINK_ENTITY_TO_FUNC(env_funnel, "env_funnel");
LINK_ENTITY_TO_FUNC(env_global, "env_global");
LINK_ENTITY_TO_FUNC(env_glow, "env_glow");
LINK_ENTITY_TO_FUNC(env_laser, "env_laser");
LINK_ENTITY_TO_FUNC(env_lighting, "env_lighting");
LINK_ENTITY_TO_FUNC(env_message, "env_message");
LINK_ENTITY_TO_FUNC(env_render, "env_render");
LINK_ENTITY_TO_FUNC(env_shake, "env_shake");
LINK_ENTITY_TO_FUNC(env_shooter, "env_shooter");
LINK_ENTITY_TO_FUNC(env_sound, "env_sound");
LINK_ENTITY_TO_FUNC(env_spark, "env_spark");
LINK_ENTITY_TO_FUNC(env_sprite, "env_sprite");
LINK_ENTITY_TO_FUNC(fireanddie, "fireanddie");
LINK_ENTITY_TO_FUNC(func_bomb_target, "func_bomb_target");
LINK_ENTITY_TO_FUNC(func_breakable, "func_breakable");
LINK_ENTITY_TO_FUNC(func_button, "func_button");
LINK_ENTITY_TO_FUNC(func_buyzone, "func_buyzone");
LINK_ENTITY_TO_FUNC(func_conveyor, "func_conveyor");
LINK_ENTITY_TO_FUNC(func_door, "func_door");
LINK_ENTITY_TO_FUNC(func_door_rotating, "func_door_rotating");
LINK_ENTITY_TO_FUNC(func_escapezone, "func_escapezone");
LINK_ENTITY_TO_FUNC(func_friction, "func_friction");
LINK_ENTITY_TO_FUNC(func_grencatch, "func_grencatch");
LINK_ENTITY_TO_FUNC(func_guntarget, "func_guntarget");
LINK_ENTITY_TO_FUNC(func_healthcharger, "func_healthcharger");
LINK_ENTITY_TO_FUNC(func_hostage_rescue, "func_hostage_rescue");
LINK_ENTITY_TO_FUNC(func_illusionary, "func_illusionary");
LINK_ENTITY_TO_FUNC(func_ladder, "func_ladder");
LINK_ENTITY_TO_FUNC(func_monsterclip, "func_monsterclip");
LINK_ENTITY_TO_FUNC(func_mortar_field, "func_mortar_field");
LINK_ENTITY_TO_FUNC(func_pendulum, "func_pendulum");
LINK_ENTITY_TO_FUNC(func_plat, "func_plat");
LINK_ENTITY_TO_FUNC(func_platrot, "func_platrot");
LINK_ENTITY_TO_FUNC(func_pushable, "func_pushable");
LINK_ENTITY_TO_FUNC(func_recharge, "func_recharge");
LINK_ENTITY_TO_FUNC(func_rot_button, "func_rot_button");
LINK_ENTITY_TO_FUNC(func_rotating, "func_rotating");
LINK_ENTITY_TO_FUNC(func_tank, "func_tank");
LINK_ENTITY_TO_FUNC(func_tankcontrols, "func_tankcontrols");
LINK_ENTITY_TO_FUNC(func_tanklaser, "func_tanklaser");
LINK_ENTITY_TO_FUNC(func_tankmortar, "func_tankmortar");
LINK_ENTITY_TO_FUNC(func_tankrocket, "func_tankrocket");
LINK_ENTITY_TO_FUNC(func_trackautochange, "func_trackautochange");
LINK_ENTITY_TO_FUNC(func_trackchange, "func_trackchange");
LINK_ENTITY_TO_FUNC(func_tracktrain, "func_tracktrain");
LINK_ENTITY_TO_FUNC(func_train, "func_train");
LINK_ENTITY_TO_FUNC(func_traincontrols, "func_traincontrols");
LINK_ENTITY_TO_FUNC(func_vehicle, "func_vehicle");
LINK_ENTITY_TO_FUNC(func_vehiclecontrols, "func_vehiclecontrols");
LINK_ENTITY_TO_FUNC(func_vip_safetyzone, "func_vip_safetyzone");
LINK_ENTITY_TO_FUNC(func_wall, "func_wall");
LINK_ENTITY_TO_FUNC(func_wall_toggle, "func_wall_toggle");
LINK_ENTITY_TO_FUNC(func_water, "func_water");
LINK_ENTITY_TO_FUNC(func_weaponcheck, "func_weaponcheck");
LINK_ENTITY_TO_FUNC(game_counter, "game_counter");
LINK_ENTITY_TO_FUNC(game_counter_set, "game_counter_set");
LINK_ENTITY_TO_FUNC(game_end, "game_end");
LINK_ENTITY_TO_FUNC(game_player_equip, "game_player_equip");
LINK_ENTITY_TO_FUNC(game_player_hurt, "game_player_hurt");
LINK_ENTITY_TO_FUNC(game_player_team, "game_player_team");
LINK_ENTITY_TO_FUNC(game_score, "game_score");
LINK_ENTITY_TO_FUNC(game_team_master, "game_team_master");
LINK_ENTITY_TO_FUNC(game_team_set, "game_team_set");
LINK_ENTITY_TO_FUNC(game_text, "game_text");
LINK_ENTITY_TO_FUNC(game_zone_player, "game_zone_player");
LINK_ENTITY_TO_FUNC(gibshooter, "gibshooter");
LINK_ENTITY_TO_FUNC(grenade, "grenade");
LINK_ENTITY_TO_FUNC(hostage_entity, "hostage_entity");
LINK_ENTITY_TO_FUNC(info_bomb_target, "info_bomb_target");
LINK_ENTITY_TO_FUNC(info_hostage_rescue, "info_hostage_rescue");
LINK_ENTITY_TO_FUNC(info_intermission, "info_intermission");
LINK_ENTITY_TO_FUNC(info_landmark, "info_landmark");
LINK_ENTITY_TO_FUNC(info_map_parameters, "info_map_parameters");
LINK_ENTITY_TO_FUNC(info_null, "info_null");
LINK_ENTITY_TO_FUNC(info_player_deathmatch, "info_player_deathmatch");
LINK_ENTITY_TO_FUNC(info_player_start, "info_player_start");
LINK_ENTITY_TO_FUNC(info_target, "info_target");
LINK_ENTITY_TO_FUNC(info_teleport_destination, "info_teleport_destination");
LINK_ENTITY_TO_FUNC(info_vip_start, "info_vip_start");
LINK_ENTITY_TO_FUNC(infodecal, "infodecal");
LINK_ENTITY_TO_FUNC(item_airtank, "item_airtank");
LINK_ENTITY_TO_FUNC(item_antidote, "item_antidote");
LINK_ENTITY_TO_FUNC(item_assaultsuit, "item_assaultsuit");
LINK_ENTITY_TO_FUNC(item_battery, "item_battery");
LINK_ENTITY_TO_FUNC(item_healthkit, "item_healthkit");
LINK_ENTITY_TO_FUNC(item_kevlar, "item_kevlar");
LINK_ENTITY_TO_FUNC(item_longjump, "item_longjump");
LINK_ENTITY_TO_FUNC(item_security, "item_security");
LINK_ENTITY_TO_FUNC(item_sodacan, "item_sodacan");
LINK_ENTITY_TO_FUNC(item_suit, "item_suit");
LINK_ENTITY_TO_FUNC(item_thighpack, "item_thighpack");
LINK_ENTITY_TO_FUNC(light, "light");
LINK_ENTITY_TO_FUNC(light_environment, "light_environment");
LINK_ENTITY_TO_FUNC(light_spot, "light_spot");
LINK_ENTITY_TO_FUNC(momentary_door, "momentary_door");
LINK_ENTITY_TO_FUNC(momentary_rot_button, "momentary_rot_button");
LINK_ENTITY_TO_FUNC(monster_hevsuit_dead, "monster_hevsuit_dead");
LINK_ENTITY_TO_FUNC(monster_mortar, "monster_mortar");
LINK_ENTITY_TO_FUNC(monster_scientist, "monster_scientist");
LINK_ENTITY_TO_FUNC(multi_manager, "multi_manager");
LINK_ENTITY_TO_FUNC(multisource, "multisource");
LINK_ENTITY_TO_FUNC(path_corner, "path_corner");
LINK_ENTITY_TO_FUNC(path_track, "path_track");
LINK_ENTITY_TO_FUNC(player, "player");
LINK_ENTITY_TO_FUNC(player_loadsaved, "player_loadsaved");
LINK_ENTITY_TO_FUNC(player_weaponstrip, "player_weaponstrip");
LINK_ENTITY_TO_FUNC(soundent, "soundent");
LINK_ENTITY_TO_FUNC(spark_shower, "spark_shower");
LINK_ENTITY_TO_FUNC(speaker, "speaker");
LINK_ENTITY_TO_FUNC(target_cdaudio, "target_cdaudio");
LINK_ENTITY_TO_FUNC(test_effect, "test_effect");
LINK_ENTITY_TO_FUNC(trigger, "trigger");
LINK_ENTITY_TO_FUNC(trigger_auto, "trigger_auto");
LINK_ENTITY_TO_FUNC(trigger_autosave, "trigger_autosave");
LINK_ENTITY_TO_FUNC(trigger_camera, "trigger_camera");
LINK_ENTITY_TO_FUNC(trigger_cdaudio, "trigger_cdaudio");
LINK_ENTITY_TO_FUNC(trigger_changelevel, "trigger_changelevel");
LINK_ENTITY_TO_FUNC(trigger_changetarget, "trigger_changetarget");
LINK_ENTITY_TO_FUNC(trigger_counter, "trigger_counter");
LINK_ENTITY_TO_FUNC(trigger_endsection, "trigger_endsection");
LINK_ENTITY_TO_FUNC(trigger_gravity, "trigger_gravity");
LINK_ENTITY_TO_FUNC(trigger_hurt, "trigger_hurt");
LINK_ENTITY_TO_FUNC(trigger_monsterjump, "trigger_monsterjump");
LINK_ENTITY_TO_FUNC(trigger_multiple, "trigger_multiple");
LINK_ENTITY_TO_FUNC(trigger_once, "trigger_once");
LINK_ENTITY_TO_FUNC(trigger_push, "trigger_push");
LINK_ENTITY_TO_FUNC(trigger_relay, "trigger_relay");
LINK_ENTITY_TO_FUNC(trigger_teleport, "trigger_teleport");
LINK_ENTITY_TO_FUNC(trigger_transition, "trigger_transition");
LINK_ENTITY_TO_FUNC(weapon_ak47, "weapon_ak47");
LINK_ENTITY_TO_FUNC(weapon_aug, "weapon_aug");
LINK_ENTITY_TO_FUNC(weapon_awp, "weapon_awp");
LINK_ENTITY_TO_FUNC(weapon_c4, "weapon_c4");
LINK_ENTITY_TO_FUNC(weapon_deagle, "weapon_deagle");
LINK_ENTITY_TO_FUNC(weapon_elite, "weapon_elite");
LINK_ENTITY_TO_FUNC(weapon_fiveseven, "weapon_fiveseven");
LINK_ENTITY_TO_FUNC(weapon_flashbang, "weapon_flashbang");
LINK_ENTITY_TO_FUNC(weapon_g3sg1, "weapon_g3sg1");
LINK_ENTITY_TO_FUNC(weapon_glock18, "weapon_glock18");
LINK_ENTITY_TO_FUNC(weapon_hegrenade, "weapon_hegrenade");
LINK_ENTITY_TO_FUNC(weapon_knife, "weapon_knife");
LINK_ENTITY_TO_FUNC(weapon_m249, "weapon_m249");
LINK_ENTITY_TO_FUNC(weapon_m3, "weapon_m3");
LINK_ENTITY_TO_FUNC(weapon_m4a1, "weapon_m4a1");
LINK_ENTITY_TO_FUNC(weapon_mac10, "weapon_mac10");
LINK_ENTITY_TO_FUNC(weapon_mp5navy, "weapon_mp5navy");
LINK_ENTITY_TO_FUNC(weapon_p228, "weapon_p228");
LINK_ENTITY_TO_FUNC(weapon_p90, "weapon_p90");
LINK_ENTITY_TO_FUNC(weapon_scout, "weapon_scout");
LINK_ENTITY_TO_FUNC(weapon_sg550, "weapon_sg550");
LINK_ENTITY_TO_FUNC(weapon_sg552, "weapon_sg552");
LINK_ENTITY_TO_FUNC(weapon_smokegrenade, "weapon_smokegrenade");
LINK_ENTITY_TO_FUNC(weapon_tmp, "weapon_tmp");
LINK_ENTITY_TO_FUNC(weapon_ump45, "weapon_ump45");
LINK_ENTITY_TO_FUNC(weapon_usp, "weapon_usp");
LINK_ENTITY_TO_FUNC(weapon_xm1014, "weapon_xm1014");
LINK_ENTITY_TO_FUNC(weaponbox, "weaponbox");
LINK_ENTITY_TO_FUNC(world_items, "world_items");
LINK_ENTITY_TO_FUNC(worldspawn, "worldspawn");
LINK_ENTITY_TO_FUNC(xen_hair, "xen_hair");
LINK_ENTITY_TO_FUNC(xen_hull, "xen_hull");
LINK_ENTITY_TO_FUNC(xen_plantlight, "xen_plantlight");
LINK_ENTITY_TO_FUNC(xen_spore_large, "xen_spore_large");
LINK_ENTITY_TO_FUNC(xen_spore_medium, "xen_spore_medium");
LINK_ENTITY_TO_FUNC(xen_spore_small, "xen_spore_small");
LINK_ENTITY_TO_FUNC(xen_tree, "xen_tree");
LINK_ENTITY_TO_FUNC(xen_ttrigger, "xen_ttrigger");

