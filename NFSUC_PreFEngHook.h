#pragma once
// NFS Undercover - ReShade Pre FENg Hook
#ifndef HAS_COPS
#define HAS_COPS
#endif
//#ifndef HAS_DAL // it actually has DAL but not DALOptions... Other DAL classes haven't been implemented yet...
//#define HAS_DAL
//#endif
#ifndef NFS_MULTITHREAD
#define NFS_MULTITHREAD
#endif
#define FEMANAGER_RENDER_HOOKADDR1 0x007AE5D8
#define NFSUC_EXIT1 0x007AE5DE
#define NFSUC_EXIT2 0x07AE635
#define NFS_D3D9_DEVICE_ADDRESS 0x00EA0110
#define DRAW_FENG_BOOL_ADDR 0x00D52ECA

#define NFSUC_MOTIONBLUR_HOOK_ADDR 0x007B2978
#define NFSUC_MOTIONBLUR_EXIT_TRUE 0x007B297F
#define NFSUC_MOTIONBLUR_EXIT_FALSE 0x007B2AAC

#define SIM_SETSTREAM_ADDR 0x007C0C70
#define WCOLMGR_GETWORLDHEIGHT_ADDR 0x0088FDC0
#define PLAYER_LISTABLESET_ADDR 0x00DE99DC
#define MAINSERVICE_HOOK_ADDR 0x00669BBD
#define SAVEHOTPOS_ADDR 0x01336494
#define LOADHOTPOS_ADDR 0x01336498

#define CHANGEPLAYERVEHICLE_ADDR 0x0133648C

#define EXITGAMEFLAG_ADDR 0x00DA5110
#define GAMEFLOWMGR_ADDR 0x00DA57B0
#define GAMEFLOWMGR_STATUS_ADDR 0x00DA57B8
#define GAMEFLOWMGR_UNLOADFE_ADDR 0x006AE650
#define GAMEFLOWMGR_UNLOADTRACK_ADDR 0x005F8270
#define GAMEFLOWMGR_RELOADTRACK_ADDR 0x0069E3A0
#define GAMEFLOWMGR_LOADREGION_ADDR 0x0069E330
#define SKIPFE_ADDR 0x00DAA0A4
#define SKIPFETRACKNUM_ADDR 0x00D5E8B0
#define SKIPFETRACKNUM_ADDR2 0x00D3CDF4
#define DEFAULT_TRACK_NUM 8000
#define STARTSKIPFERACE_ADDR 0x0084D7A0

#define SKIPFE_NUMAICARS_ADDR 0x00DAA0B0
#define SKIPFE_TRACKDIRECTION_ADDR 0x00DAA0AC
//#define SKIPFE_TRAFFICDENSITY_ADDR 0x00BFBC24
//#define SKIPFE_TRAFFICONCOMING_ADDR 0x00A9D9B4
#define SKIPFE_DISABLETRAFFIC_ADDR 0x00D82102
//#define SKIPFE_DRAGRACE_ADDR 0x00864FC0
//#define SKIPFE_DRIFTRACE_ADDR 0x00864FC4
#define SKIPFE_P2P_ADDR 0x00DAA0BA
#define SKIPFE_NUMPLAYERCARS_ADDR 0x00D3CE28
#define SKIPFE_NUMLAPS_ADDR 0x00D3CE2C
#define SKIPFE_RACETYPE_ADDR 0x00DAA0A8
#define SKIPFE_DIFFICULTY_ADDR 0x00D3CE34

#define SKIPFE_PLAYERPERFORMANCE_ADDR 0x00D3CE20
//#define SKIPFE_NUMPLAYERSCREENS_ADDR 0x00A9D9E4
#define SKIPFE_RACEID_ADDR 0x00D3CDF8
#define SKIPFE_PLAYERCAR_ADDR 0x00D3CE00
//#define SKIPFE_PLAYERCAR_DEHARDCODE_PATCH_ADDR 0x004D4B41
#define SKIPFE_PLAYERCAR2_ADDR 0x00D3CE04
//#define SKIPFE_OPPONENTPRESETRIDE_ADDR 0x008F86B0
#define SKIPFE_DISABLECOPS_ADDR 0x00DAA0B8
#define SKIPFE_MAXCOPS_ADDR 0x00DAA0B4
#define SKIPFE_HELICOPTER_ADDR 0x00DAA0B9

// New ProStreet stuff (other than 4 player support)
#define SKIPFE_TURBOSFX_ADDR 0x00D3CE08
//#define SKIPFE_FORCEHUBSELECTIONSET_ADDR 0x00A9D998
//#define SKIPFE_FORCERACESELECTIONSET_ADDR 0x00A9D998
//#define SKIPFE_FORCENIS_ADDR 0x00A9D9A0
//#define SKIPFE_FORCENISCONTEXT_ADDR 0x00A9D9A4
#define SKIPFE_TRACTIONCONTROLLEVEL_ADDR 0x00D3CE44
#define SKIPFE_STABILITYCONTROLLEVEL_ADDR 0x00D3CE48
#define SKIPFE_ANTILOCKBRAKESLEVEL_ADDR 0x00D3CE4C
#define SKIPFE_DRIFTASSISTLEVEL_ADDR 0x00D3CE50
#define SKIPFE_RACELINEASSISTLEVEL_ADDR 0x00D3CE54
#define SKIPFE_BRAKINGASSISTLEVEL_ADDR 0x00D3CE40
//#define SKIPFE_TRANSMISSIONSETUP_ADDR 0x00A9D9E8
//#define SKIPFE_ENABLEDEBUGACTIVITY_ADDR 0x00BFBC2C
#define SKIPFE_PRACTICEMODE_ADDR 0x00DAA0BB
//#define SKIPFE_DISABLESMOKE_ADDR 0x00BFBC38
//#define SKIPFE_SLOTCARRACE_ADDR 0x00BFBC3A
//#define SKIPFE_BOOTFLOW_ADDR 0x00BFBC41

#define SMARTLOOKAHEADCAMERA_ADDR 0x00DAA0EB

#define DEFAULT_PLAYERCAR "por_911_tur_06"
#define DEFAULT_PLAYER2CAR "por_911_tur_06"

#define UNLOCKALLTHINGS_ADDR 0x00DAA15A
#define SKIPCAREERINTRO_ADDR 0x00DAA15B
//#define SKIPDDAYRACES_ADDR 0x00A9E6C2
#define SHOWALLCARSINFE_ADDR 0x00DAA165
#define CARGUYSCAMERA_ADDR 0x00DAA167

#define DOSCREENPRINTF_ADDR 0x00DAA175

// Precipitation memory addresses (NFS ProStreet)
// #define RAIN_CF_INTENSITY_ADDR        0x00FEB1D4
// #define RAIN_DROP_OFFSET_ADDR         0x00FEB1C8
// #define RAIN_DROP_ALPHA_ADDR          0x00FEB1D0
// #define FOG_SKY_FALLOFF_ADDR          0x00FEB198
// #define RAIN_PARAM_A_ADDR             0x00FEB190
// #define FOG_BLEND_PARAM_ADDR          0x00FEB1A0

#define ENABLECOPS_ADDR 0x00D1F042
#define CAMERADEBUGWATCHCAR_ADDR 0x00D89951
#define MTOGGLECAR_ADDR 0x00D89990
#define MTOGGLECARLIST_ADDR 0x00D89994
#define CAMERA_SETACTION_ADDR 0x004D2480
#define CAMERA_UPDATE_ADDR 0x004D2480  // ToDo: find address

#define EVIEW_LIST_HEAD_PTR  0x0A79B44  // Example: global pointer to head of EViewNode list
#define NODE_MATRIX_OFFSET   0x40       // Offset of viewMatrix within EViewNode
// #define GET_VISIBLE_STATE_SB_ADDR 0xXXXXXXX

#define CARLIST_TYPE_AIRACER 3
#define CARLIST_TYPE_COP 4
#define CARLIST_TYPE_TRAFFIC 5

#define GETSIMABLE_OFFSET 0x4
#define UTL_ILIST_FIND_ADDR 0x005F6700
#define IPERPETRATOR_HANDLE_ADDR 0x00402240
#define PERP_SETHEAT_OFFSET 0x8

#define AI_RANDOMTURNS_ADDR 0x01339F78

// car flip stuff
#define VEHICLE_LISTABLESET_ADDR 0x00DE93D4
#define IRIGIDBODY_HANDLE_ADDR 0x00401A80
#define RB_GETMATRIX4_OFFSET 0x40
#define RB_SETORIENTATION_OFFSET 0x6C

// infinite NOS
#define INFINITENOS_CAVE_ADDR 0x00700F1F
#define INFINITENOS_CAVE_EXIT_TRUE 0x700F69
#define INFINITENOS_CAVE_EXIT_FALSE 0x700F25
#define INFINITENOS_CAVE_EXIT_BE 0x701055

#define INFINITERACEBREAKER_ADDR 0x012798A4

// AI Control toggle
#define AICONTROL_CAVE_ADDR 0x43C40C
#define AICONTROL_CAVE_EXIT 0x0043C418
#define AICONTROL_CAVE_EXIT2 0x43C426
//#define UPDATEWRONGWAY_ADDR 0x0041CEC0

#define PRECULLERMODE_ADDR 0x00D3CE8C

// TODO - find FEngine package command control
#define FENG_MINSTANCE_ADDR 0x00D992BC
//#define FENG_QUEUEPACKAGEPOP_ADDR 0x0059F160
//#define FENG_QUEUEPACKAGEPUSH_ADDR 0x005AC340
//#define FENG_QUEUEPACKAGESWITCH_ADDR FENG_QUEUEPACKAGEPUSH_ADDR

#define GAMENOTIFYRACEFINISHED_ADDR 0x006623D0
#define GAMEENTERPOSTRACEFLOW_ADDR 0x00662D40

#define PC_PLAT_STUFF_ADDR 0xDF1DE0

#define DRAWWORLD_ADDR 0x00D52F5C
#define DRAWCARS_ADDR 0x00D5A71C
#define DRAWCARSHADOWS_ADDR 0x00D5A724
#define GAMESPEED_ADDR 0x00D54610
#define APPLYVISUALLOOK_ADDR ((*(int*)(PC_PLAT_STUFF_ADDR + 0x54)) + 0x54)

// player cash & DAL stuff
#define FEMANAGER_INSTANCE_ADDR 0x00D992BC
#define USERPROFILE_POINTER (*(int*)0x00D8E1A4)
// since Undercover uses SQL databases for user profiles... we won't read the actual value from memory but adjust it with a function
#define GMW2GAME_AWARDCASH_ADDR 0x005FED00
#define GMW2GAME_OBJ_ADDR 0x00D9B940


void ReShade_EntryPoint();
void __stdcall MainService_Hook();
void MotionBlur_EntryPoint();
void InfiniteNOSCave();
void ToggleAIControlCave();
