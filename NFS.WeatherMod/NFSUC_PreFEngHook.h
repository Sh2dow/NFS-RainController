#pragma once
#include <cstdint>   // REQUIRED for uintptr_t

namespace UC
{
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

    constexpr uintptr_t FEMANAGER_RENDER_HOOKADDR1 = 0x007AE5D8;
    constexpr uintptr_t NFSUC_EXIT1 = 0x007AE5DE;
    constexpr uintptr_t NFSUC_EXIT2 = 0x07AE635;

    constexpr uintptr_t NFS_D3D9_DEVICE_ADDRESS = 0x00EA0110;

    constexpr uintptr_t DRAW_FENG_BOOL_ADDR = 0x00D52ECA;

#define NFSUC_MOTIONBLUR_HOOK_ADDR 0x007B2978
#define NFSUC_MOTIONBLUR_EXIT_TRUE 0x007B297F
#define NFSUC_MOTIONBLUR_EXIT_FALSE 0x007B2AAC

    constexpr uintptr_t SIM_SETSTREAM_ADDR = 0x007C0C70;
    constexpr uintptr_t WCOLMGR_GETWORLDHEIGHT_ADDR = 0x0088FDC0;
    constexpr uintptr_t PLAYER_LISTABLESET_ADDR = 0x00DE99DC;
    constexpr uintptr_t MAINSERVICE_HOOK_ADDR = 0x00669BBD;
    constexpr uintptr_t SAVEHOTPOS_ADDR = 0x01336494;
    constexpr uintptr_t LOADHOTPOS_ADDR = 0x01336498;

    constexpr uintptr_t CHANGEPLAYERVEHICLE_ADDR = 0x0133648C;

    constexpr uintptr_t EXITGAMEFLAG_ADDR = 0x00DA5110;
    constexpr uintptr_t GAMEFLOWMGR_ADDR = 0x00DA57B0;
    constexpr uintptr_t GAMEFLOWMGR_STATUS_ADDR = 0x00DA57B8;
    constexpr uintptr_t GAMEFLOWMGR_UNLOADFE_ADDR = 0x006AE650;
    constexpr uintptr_t GAMEFLOWMGR_UNLOADTRACK_ADDR = 0x005F8270;
    constexpr uintptr_t GAMEFLOWMGR_RELOADTRACK_ADDR = 0x0069E3A0;
    constexpr uintptr_t GAMEFLOWMGR_LOADREGION_ADDR = 0x0069E330;
    constexpr uintptr_t SKIPFE_ADDR = 0x00DAA0A4;
    constexpr uintptr_t SKIPFETRACKNUM_ADDR = 0x00D5E8B0;
    constexpr uintptr_t SKIPFETRACKNUM_ADDR2 = 0x00D3CDF4;
    constexpr uintptr_t DEFAULT_TRACK_NUM = 8000;
    constexpr uintptr_t STARTSKIPFERACE_ADDR = 0x0084D7A0;

    constexpr uintptr_t SKIPFE_NUMAICARS_ADDR = 0x00DAA0B0;
    constexpr uintptr_t SKIPFE_TRACKDIRECTION_ADDR = 0x00DAA0AC;
    //#define SKIPFE_TRAFFICDENSITY_ADDR 0x00BFBC24
    //#define SKIPFE_TRAFFICONCOMING_ADDR 0x00A9D9B4
    constexpr uintptr_t SKIPFE_DISABLETRAFFIC_ADDR = 0x00D82102;
    //#define SKIPFE_DRAGRACE_ADDR 0x00864FC0
    //#define SKIPFE_DRIFTRACE_ADDR 0x00864FC4
    constexpr uintptr_t SKIPFE_P2P_ADDR = 0x00DAA0BA;
    constexpr uintptr_t SKIPFE_NUMPLAYERCARS_ADDR = 0x00D3CE28;
    constexpr uintptr_t SKIPFE_NUMLAPS_ADDR = 0x00D3CE2C;
    constexpr uintptr_t SKIPFE_RACETYPE_ADDR = 0x00DAA0A8;
    constexpr uintptr_t SKIPFE_DIFFICULTY_ADDR = 0x00D3CE34;

    constexpr uintptr_t SKIPFE_PLAYERPERFORMANCE_ADDR = 0x00D3CE20;
    //#define SKIPFE_NUMPLAYERSCREENS_ADDR 0x00A9D9E4
    constexpr uintptr_t SKIPFE_RACEID_ADDR = 0x00D3CDF8;
    constexpr uintptr_t SKIPFE_PLAYERCAR_ADDR = 0x00D3CE00;
    //#define SKIPFE_PLAYERCAR_DEHARDCODE_PATCH_ADDR 0x004D4B41
    constexpr uintptr_t SKIPFE_PLAYERCAR2_ADDR = 0x00D3CE04;
    //#define SKIPFE_OPPONENTPRESETRIDE_ADDR 0x008F86B0
    constexpr uintptr_t SKIPFE_DISABLECOPS_ADDR = 0x00DAA0B8;
    constexpr uintptr_t SKIPFE_MAXCOPS_ADDR = 0x00DAA0B4;
    constexpr uintptr_t SKIPFE_HELICOPTER_ADDR = 0x00DAA0B9;

    // New ProStreet stuff (other than 4 player support)
    constexpr uintptr_t SKIPFE_TURBOSFX_ADDR = 0x00D3CE08;
    //#define SKIPFE_FORCEHUBSELECTIONSET_ADDR 0x00A9D998
    //#define SKIPFE_FORCERACESELECTIONSET_ADDR 0x00A9D998
    //#define SKIPFE_FORCENIS_ADDR 0x00A9D9A0
    //#define SKIPFE_FORCENISCONTEXT_ADDR 0x00A9D9A4
    constexpr uintptr_t SKIPFE_TRACTIONCONTROLLEVEL_ADDR = 0x00D3CE44;
    constexpr uintptr_t SKIPFE_STABILITYCONTROLLEVEL_ADDR = 0x00D3CE48;
    constexpr uintptr_t SKIPFE_ANTILOCKBRAKESLEVEL_ADDR = 0x00D3CE4C;
    constexpr uintptr_t SKIPFE_DRIFTASSISTLEVEL_ADDR = 0x00D3CE50;
    constexpr uintptr_t SKIPFE_RACELINEASSISTLEVEL_ADDR = 0x00D3CE54;
    constexpr uintptr_t SKIPFE_BRAKINGASSISTLEVEL_ADDR = 0x00D3CE40;
    //#define SKIPFE_TRANSMISSIONSETUP_ADDR 0x00A9D9E8
    //#define SKIPFE_ENABLEDEBUGACTIVITY_ADDR 0x00BFBC2C
    constexpr uintptr_t SKIPFE_PRACTICEMODE_ADDR = 0x00DAA0BB;
    //#define SKIPFE_DISABLESMOKE_ADDR 0x00BFBC38
    //#define SKIPFE_SLOTCARRACE_ADDR 0x00BFBC3A
    //#define SKIPFE_BOOTFLOW_ADDR 0x00BFBC41

    constexpr uintptr_t SMARTLOOKAHEADCAMERA_ADDR = 0x00DAA0EB;

    inline const char* DEFAULT_PLAYERCAR = "por_911_tur_06";
    inline const char* DEFAULT_PLAYER2CAR = "por_911_tur_06";

    constexpr uintptr_t UNLOCKALLTHINGS_ADDR = 0x00DAA15A;
    constexpr uintptr_t SKIPCAREERINTRO_ADDR = 0x00DAA15B;

    //#define SKIPDDAYRACES_ADDR 0x00A9E6C2

    constexpr uintptr_t SHOWALLCARSINFE_ADDR = 0x00DAA165;
    constexpr uintptr_t CARGUYSCAMERA_ADDR = 0x00DAA167;
    constexpr uintptr_t DOSCREENPRINTF_ADDR = 0x00DAA175;

    // Precipitation memory addresses (NFS ProStreet)
    // #define RAIN_CF_INTENSITY_ADDR        0x00FEB1D4
    // #define RAIN_DROP_OFFSET_ADDR         0x00FEB1C8
    // #define RAIN_DROP_ALPHA_ADDR          0x00FEB1D0
    // #define FOG_SKY_FALLOFF_ADDR          0x00FEB198
    // #define RAIN_PARAM_A_ADDR             0x00FEB190
    // #define FOG_BLEND_PARAM_ADDR          0x00FEB1A0

    constexpr uintptr_t ENABLECOPS_ADDR = 0x00D1F042;
    constexpr uintptr_t CAMERADEBUGWATCHCAR_ADDR = 0x00D89951;
    constexpr uintptr_t MTOGGLECAR_ADDR = 0x00D89990;
    constexpr uintptr_t MTOGGLECARLIST_ADDR = 0x00D89994;
    constexpr uintptr_t CAMERA_SETACTION_ADDR = 0x004D2480;
    constexpr uintptr_t CAMERA_UPDATE_ADDR = 0x004D2480; // ToDo: find address

    constexpr uintptr_t EVIEW_LIST_HEAD_PTR = 0x0A79B44; // Example: global pointer to head of EViewNode list
    constexpr uintptr_t NODE_MATRIX_OFFSET = 0x40; // Offset of viewMatrix within EViewNode
    // #define GET_VISIBLE_STATE_SB_ADDR 0xXXXXXXX

    constexpr uintptr_t CARLIST_TYPE_AIRACER = 3;
    constexpr uintptr_t CARLIST_TYPE_COP = 4;
    constexpr uintptr_t CARLIST_TYPE_TRAFFIC = 5;

    constexpr uintptr_t GETSIMABLE_OFFSET = 0x4;
    constexpr uintptr_t UTL_ILIST_FIND_ADDR = 0x005F6700;
    constexpr uintptr_t IPERPETRATOR_HANDLE_ADDR = 0x00402240;
    constexpr uintptr_t PERP_SETHEAT_OFFSET = 0x8;

    constexpr uintptr_t AI_RANDOMTURNS_ADDR = 0x01339F78;

    // car flip stuff
    constexpr uintptr_t VEHICLE_LISTABLESET_ADDR = 0x00DE93D4;
    constexpr uintptr_t IRIGIDBODY_HANDLE_ADDR = 0x00401A80;
    constexpr uintptr_t RB_GETMATRIX4_OFFSET = 0x40;
    constexpr uintptr_t RB_SETORIENTATION_OFFSET = 0x6C;

    // infinite NOS
    constexpr uintptr_t INFINITENOS_CAVE_ADDR = 0x00700F1F;
    constexpr uintptr_t INFINITENOS_CAVE_EXIT_TRUE = 0x700F69;
    constexpr uintptr_t INFINITENOS_CAVE_EXIT_FALSE = 0x700F25;
    constexpr uintptr_t INFINITENOS_CAVE_EXIT_BE = 0x701055;

    constexpr uintptr_t INFINITERACEBREAKER_ADDR = 0x012798A4;

    // AI Control toggle
    constexpr uintptr_t AICONTROL_CAVE_ADDR = 0x43C40C;
    constexpr uintptr_t AICONTROL_CAVE_EXIT = 0x0043C418;
    constexpr uintptr_t AICONTROL_CAVE_EXIT2 = 0x43C426;
    //#define UPDATEWRONGWAY_ADDR 0x0041CEC0

    constexpr uintptr_t PRECULLERMODE_ADDR = 0x00D3CE8C;

    // TODO - find FEngine package command control
    //#define FENG_QUEUEPACKAGEPOP_ADDR 0x0059F160
    //#define FENG_QUEUEPACKAGEPUSH_ADDR 0x005AC340
    //#define FENG_QUEUEPACKAGESWITCH_ADDR FENG_QUEUEPACKAGEPUSH_ADDR

    constexpr uintptr_t GAMENOTIFYRACEFINISHED_ADDR = 0x006623D0;
    constexpr uintptr_t GAMEENTERPOSTRACEFLOW_ADDR = 0x00662D40;

    constexpr uintptr_t PC_PLAT_STUFF_ADDR = 0xDF1DE0;

    constexpr uintptr_t DRAWWORLD_ADDR = 0x00D52F5C;
    constexpr uintptr_t DRAWCARS_ADDR = 0x00D5A71C;
    constexpr uintptr_t DRAWCARSHADOWS_ADDR = 0x00D5A724;
    constexpr uintptr_t GAMESPEED_ADDR = 0x00D54610;
    #define APPLYVISUALLOOK_ADDR  ((*(int*)(PC_PLAT_STUFF_ADDR + 0x54)) + 0x54)

    // player cash & DAL stuff
    constexpr uintptr_t FEMANAGER_INSTANCE_ADDR = 0x00D992BC;
    #define USERPROFILE_POINTER (*(int*)0x00D8E1A4)
    // since Undercover uses SQL databases for user profiles... we won't read the actual value from memory but adjust it with a function
    constexpr uintptr_t GMW2GAME_AWARDCASH_ADDR = 0x005FED00;
    constexpr uintptr_t GMW2GAME_OBJ_ADDR = 0x00D9B940;


    constexpr uintptr_t EViewListHeadPtr = 0x00A79B44;

    void ReShade_EntryPoint();
    void __stdcall MainService_Hook();
    void MotionBlur_EntryPoint();
    void InfiniteNOSCave();
    void ToggleAIControlCave();
}
