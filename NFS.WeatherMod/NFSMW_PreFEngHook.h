#pragma once
namespace MW
{
    // NFS MW - ReShade Pre FENg Hook
#ifndef HAS_COPS
#define HAS_COPS
#endif
#ifndef HAS_FOG_CTRL
#define HAS_FOG_CTRL
#endif
constexpr uintptr_t  FEMANAGER_RENDER_HOOKADDR1  = 0x006E71D0;
constexpr uintptr_t  FEMANAGER_RENDER_HOOKADDR2  = 0x00573130;
constexpr uintptr_t  FEMANAGER_RENDER_ADDRESS  = 0x00516F70;
constexpr uintptr_t  NFS_D3D9_DEVICE_ADDRESS  = 0x00982BDC;

constexpr uintptr_t  DRAW_FENG_BOOL_ADDR  = 0x008F374C;

constexpr uintptr_t  SIM_SETSTREAM_ADDR  = 0x006F1170;
constexpr uintptr_t  WCOLMGR_GETWORLDHEIGHT_ADDR  = 0x00789870;
constexpr uintptr_t  PLAYER_LISTABLESET_ADDR  = 0x0092D84C;
constexpr uintptr_t  SAVEHOTPOS_ADDR  = 0x009B0908;
constexpr uintptr_t  LOADHOTPOS_ADDR  = 0x009B090C;

constexpr uintptr_t  CHANGEPLAYERVEHICLE_ADDR  = 0x009B08FD;

constexpr uintptr_t  EXITGAMEFLAG_ADDR  = 0x009257EC;
constexpr uintptr_t  GAMEFLOWMGR_ADDR  = 0x00925E70;
constexpr uintptr_t  GAMEFLOWMGR_STATUS_ADDR  = 0x00925E90;
constexpr uintptr_t  GAMEFLOWMGR_UNLOADFE_ADDR  = 0x006596E0;
constexpr uintptr_t  GAMEFLOWMGR_UNLOADTRACK_ADDR  = 0x00667340;
constexpr uintptr_t  GAMEFLOWMGR_LOADREGION_ADDR  = 0x006672D0;
constexpr uintptr_t  SKIPFE_ADDR  = 0x00926064;
constexpr uintptr_t  SKIPFETRACKNUM_ADDR  = 0x00904FB8;
constexpr uintptr_t  SKIPFETRACKNUM_ADDR2  = 0x008F86A4;
constexpr uintptr_t  DEFAULT_TRACK_NUM  = 2000;
constexpr uintptr_t  STARTSKIPFERACE_ADDR  = 0x0057ECD0;
constexpr uintptr_t  ONLINENABLED_ADDR  = 0x00926110;

constexpr uintptr_t  GAMEFLOW_UNLOADTRACK_FIX  = 0x006618C3;
constexpr uintptr_t  BOOTFLOWMGR_INIT_ADDR  = 0x00597C10;

constexpr uintptr_t  SKIPFE_NUMAICARS_ADDR  = 0x00926080;
constexpr uintptr_t  SKIPFE_TRACKDIRECTION_ADDR  = 0x00926084;
constexpr uintptr_t  SKIPFE_TRAFFICDENSITY_ADDR  = 0x00926090;
constexpr uintptr_t  SKIPFE_TRAFFICONCOMING_ADDR  = 0x008F86C8;
constexpr uintptr_t  SKIPFE_DISABLETRAFFIC_ADDR  = 0x00926094;
constexpr uintptr_t SKIPFE_DRAGRACE_ADDR = 0x00864FC0;
constexpr uintptr_t SKIPFE_DRIFTRACE_ADDR = 0x00864FC4;
constexpr uintptr_t  SKIPFE_P2P_ADDR  = 0x0092609C;
constexpr uintptr_t  SKIPFE_NUMPLAYERCARS_ADDR  = 0x008F86B8;
constexpr uintptr_t  SKIPFE_NUMLAPS_ADDR  = 0x008F86BC;
constexpr uintptr_t  SKIPFE_RACETYPE_ADDR  = 0x00926098;
constexpr uintptr_t  SKIPFE_DIFFICULTY_ADDR  = 0x008F86CC;

constexpr uintptr_t  SKIPFE_PLAYERPERFORMANCE_ADDR  = 0x00926078;
constexpr uintptr_t  SKIPFE_SPLITSCREEN_ADDR  = 0x0092607C;
constexpr uintptr_t  SKIPFE_RACEID_ADDR  = 0x00926068;
constexpr uintptr_t  SKIPFE_PLAYERCAR_ADDR  = 0x008F86A8;
constexpr uintptr_t  SKIPFE_PLAYERCAR2_ADDR  = 0x008F86AC;
constexpr uintptr_t  SKIPFE_OPPONENTPRESETRIDE_ADDR  = 0x008F86B0;
constexpr uintptr_t  SKIPFE_DISABLECOPS_ADDR  = 0x008F86C0;
constexpr uintptr_t  SKIPFE_MAXCOPS_ADDR  = 0x00926088;
constexpr uintptr_t  SKIPFE_HELICOPTER_ADDR  = 0x0092608C;

inline const char*  DEFAULT_PLAYERCAR  = "bmwm3gtre46";
inline const char*  DEFAULT_PLAYER2CAR  = "911turbo";

constexpr uintptr_t  UNLOCKALLTHINGS_ADDR  = 0x00926124;
constexpr uintptr_t  SKIPCAREERINTRO_ADDR  = 0x00926125;
constexpr uintptr_t  SKIPDDAYRACES_ADDR  = 0x00926126;
constexpr uintptr_t  SHOWALLCARSINFE_ADDR  = 0x00926127;
constexpr uintptr_t  CARGUYSCAMERA_ADDR  = 0x00926134;

constexpr uintptr_t  PRECIPITATION_ENABLE_ADDR  = 0x008F86E4;
constexpr uintptr_t  PRECIPITATION_DEBUG_ADDR  = 0x009B0A30;
constexpr uintptr_t  PRECIPITATION_RENDER_ADDR  = 0x00904AD0;
constexpr uintptr_t  PRECIPITATION_PERCENT_ADDR  = 0x00904A14;
constexpr uintptr_t  PRECIP_RAINX_ADDR  = 0x00904A20;
constexpr uintptr_t  PRECIP_RAINY_ADDR  = 0x00904A24;
constexpr uintptr_t  PRECIP_RAINZ_ADDR  = 0x00904A28;
constexpr uintptr_t  PRECIP_RAINZCONSTANT_ADDR  = 0x00904A2C;
constexpr uintptr_t  PRECIP_BOUNDX_ADDR  = 0x00904A60;
constexpr uintptr_t  PRECIP_BOUNDY_ADDR  = 0x00904A64;
constexpr uintptr_t  PRECIP_BOUNDZ_ADDR  = 0x00904A68;
constexpr uintptr_t  PRECIP_AHEADX_ADDR  = 0x00904A6C;
constexpr uintptr_t  PRECIP_AHEADY_ADDR  = 0x009B0A34;
constexpr uintptr_t  PRECIP_AHEADZ_ADDR  = 0x009B0A38;
constexpr uintptr_t  PRECIP_RAINWINDEFF_ADDR  = 0x00904A80;
constexpr uintptr_t  PRECIP_RAINRADIUSX_ADDR  = 0x00904A90;
constexpr uintptr_t  PRECIP_RAINRADIUSY_ADDR  = 0x00904A94;
constexpr uintptr_t  PRECIP_RAINRADIUSZ_ADDR  = 0x00904A98;
constexpr uintptr_t  PRECIP_DRIVEFACTOR_ADDR  = 0x00904ACC;
constexpr uintptr_t  PRECIP_RAINPERCENT_ADDR  = 0x00904AD8;
constexpr uintptr_t  PRECIP_FOGPERCENT_ADDR  = 0x009B0A40;
constexpr uintptr_t  PRECIP_RAININTHEHEADLIGHTS_ADDR  = 0x008F2924;
constexpr uintptr_t  PRECIP_WINDANG_ADDR  = 0x009B0A50;
constexpr uintptr_t  PRECIP_SWAYMAX_ADDR  = 0x00904AE8;
constexpr uintptr_t  PRECIP_MAXWINDEFF_ADDR  = 0x00904AF0;
constexpr uintptr_t  PRECIP_PREVAILINGMULT_ADDR  = 0x00904AF4;
constexpr uintptr_t  PRECIP_ONSCREEN_DRIPSPEED_ADDR  = 0x00904B2C;
constexpr uintptr_t  PRECIP_ONSCREEN_SPEEDMOD_ADDR  = 0x00904B30;
constexpr uintptr_t  PRECIP_ONSCREEN_DROPSHAPESPEEDCHANGE_ADDR  = 0x00904B34;
constexpr uintptr_t  PRECIP_BASEDAMPNESS_ADDR  = 0x00904B38;
constexpr uintptr_t  PRECIP_RAINRATEOFCHANGE_ADDR  = 0x00904AC4;
constexpr uintptr_t  PRECIP_CLOUDSRATEOFCHANGE_ADDR  = 0x00904AC8;
constexpr uintptr_t  PRECIP_CAMERAMOD_ADDR  = 0x00904AE0;
constexpr uintptr_t  FOG_CTRLOVERRIDE_ADDR  = 0x009B0A70;

constexpr uintptr_t  DISABLECOPS_ADDR  = 0x0090D5EC;
constexpr uintptr_t  CAMERADEBUGWATCHCAR_ADDR  = 0x00911038;
constexpr uintptr_t  MTOGGLECAR_ADDR  = 0x00911058;
constexpr uintptr_t  MTOGGLECARLIST_ADDR  = 0x0091105C;
constexpr uintptr_t  CAMERA_SETACTION_ADDR  = 0x00479EB0;

constexpr uintptr_t  CARLIST_TYPE_AIRACER  = 3;
constexpr uintptr_t  CARLIST_TYPE_COP  = 4;
constexpr uintptr_t  CARLIST_TYPE_TRAFFIC  = 5;

constexpr uintptr_t  TOGGLEAICONTROL_ADDR  = 0x0090D5FA;
constexpr uintptr_t  MINIMAP_SHOWNONPURSUITCOPS_ADDR  = 0x0091CF00;
constexpr uintptr_t  MINIMAP_SHOWPURSUITCOPS_ADDR  = 0x008F3AAC;

// heat control stuff
constexpr uintptr_t  ADJUSTSTABLEHEAT_EVENTWIN_ADDR  = 0x00590260;
constexpr uintptr_t  HEATONEVENTWIN_HOOK_ADDR  = 0x005902C1;
constexpr uintptr_t  HEATONEVENTWIN_ADDR  = 0x00581F70;
constexpr uintptr_t  CAREERHEAT_OFFSET  = 0xC;
constexpr uintptr_t  GETSIMABLE_OFFSET  = 0x4;
constexpr uintptr_t  UTL_ILIST_FIND_ADDR  = 0x005D59F0;
constexpr uintptr_t  IPERPETRATOR_HANDLE_ADDR  = 0x004037E0;
constexpr uintptr_t  PERP_SETHEAT_OFFSET  = 0x8;

constexpr uintptr_t  AI_RANDOMTURNS_ADDR  = 0x009B3900;

// car flip stuff
constexpr uintptr_t  VEHICLE_LISTABLESET_ADDR  = 0x0092CD7C;
constexpr uintptr_t  IRIGIDBODY_HANDLE_ADDR  = 0x004039F0;
constexpr uintptr_t  RB_GETMATRIX4_OFFSET  = 0x40;
constexpr uintptr_t  RB_SETORIENTATION_OFFSET  = 0x70;

constexpr uintptr_t  INFINITENOS_ADDR  = 0x00937804;
constexpr uintptr_t  INFINITERACEBREAKER_ADDR  = 0x00988E1C;

constexpr uintptr_t  PRECULLERMODE_ADDR  = 0x00903160;

constexpr uintptr_t  FEDATABASE_ADDR  = 0x0091CF90;

#define PLAYERCASH_POINTER (((*(int*)((*(int*)FEDATABASE_ADDR) + 0x10)) + 0xA8) + 0xC)
#define CURRENTBIN_POINTER ((*(int*)((*(int*)FEDATABASE_ADDR) + 0x10)) + 0xB0)

constexpr uintptr_t  CURRENTLANG_ADDR  = 0x008F41C0;
constexpr uintptr_t  SETCURRLANG_ADDR  = 0x0057E6F0;

constexpr uintptr_t  FENG_MINSTANCE_ADDR  = 0x0091CADC;
constexpr uintptr_t  FENG_QUEUEPACKAGEPOP_ADDR  = 0x005258B0;
constexpr uintptr_t  FENG_QUEUEPACKAGESWITCH_ADDR  = 0x00525940;

constexpr uintptr_t  TESTCAREERCUSTOMIZATION_ADDR  = 0x009B9E8D;

constexpr uintptr_t  DOSCREENPRINTF_ADDR  = 0x00926140;

constexpr uintptr_t  GAMENOTIFYRACEFINISHED_ADDR  = 0x006119F0;
constexpr uintptr_t  GAMENOTIFYLAPFINISHED_ADDR  = 0x00611850;
constexpr uintptr_t  GAMEENTERPOSTRACEFLOW_ADDR  = 0x00611F20;

constexpr uintptr_t  GRACESTATUS_ADDR  = 0x0091E000;
constexpr uintptr_t  EVENTALLOC_ADDR  = 0x00627400;
constexpr uintptr_t  EENTERBIN_ADDR  = 0x0062C2D0;

constexpr uintptr_t  APPLYVISUALLOOK_ADDR  = 0x008F9B5C;
constexpr uintptr_t  DRAWCARS_ADDR  = 0x00903320;
constexpr uintptr_t  DRAWCARSHADOWS_ADDR  = 0x00903328;
constexpr uintptr_t  GAMESPEED_ADDR  = 0x00901B1C;

constexpr uintptr_t  BUILDVERSIONCLNAME_ADDR  = 0x008F8690;
constexpr uintptr_t  BUILDVERSIONMACHINE_ADDR  = 0x008F868C;
constexpr uintptr_t  BUILDVERSIONCLNUMBER_ADDR  = 0x008F8694;
constexpr uintptr_t  BUILDVERSIONDATE_ADDR  = 0x008F8698;
constexpr uintptr_t  BUILDVERSIONPLAT_ADDR  = 0x008A87A4;
constexpr uintptr_t  BUILDVERSIONNAME_ADDR  = 0x008A8EAC;
constexpr uintptr_t  BUILDVERSIONOPTNAME_ADDR  = 0x008A8EA8;

// FMV playback stuff
constexpr uintptr_t  MOVIEFILENAME_ADDR  = 0x008F3750;
constexpr uintptr_t  INGAMEMOVIEFILENAME_ADDR  = 0x008F3F40;

constexpr uintptr_t  BASEFOG_FALLOFF_ADDR  = 0x00903190;
constexpr uintptr_t  BASEFOG_FALLOFFX_ADDR  = 0x00903194;
constexpr uintptr_t  BASEFOG_FALLOFFY_ADDR  = 0x00903198;
constexpr uintptr_t  BASEWEATHER_FOG_ADDR  = 0x0090319C;
constexpr uintptr_t  BASEWEATHER_FOG_START_ADDR  = 0x009031A0;
constexpr uintptr_t  BASEWEATHER_FOG_COLOUR_R_ADDR  = 0x009031AC;
constexpr uintptr_t  BASEWEATHER_FOG_COLOUR_G_ADDR  = 0x009031A8;
constexpr uintptr_t  BASEWEATHER_FOG_COLOUR_B_ADDR  = 0x009031A4;

// Visual Treatment stuff
constexpr uintptr_t VISUALTREATMENT_INSTANCE_ADDR = 0x982AF0;


    void __stdcall FEManager_Render_Hook();
    void __stdcall FECareerRecord_AdjustHeatOnEventWin_Hook();
    
    // ------------------------------------------------------------
    // Executable validation
    // ------------------------------------------------------------
    constexpr uintptr_t Entry = 0x007C4040;

    inline const char* Name  =
        "NFSMW - Camera Mod";

    inline const char* Error =
        "This .exe is not supported.\n"
        "Please use v1.3 Reloaded speed.exe (5,75 MB (6.029.312 bytes).";

    // ------------------------------------------------------------
    // Engine globals
    // ------------------------------------------------------------
    inline float* DeltaTime = reinterpret_cast<float*>(0x009259BC);
    inline bool*  IsPaused  = nullptr;

    // ------------------------------------------------------------
    // Functions / addresses
    // ------------------------------------------------------------
    using CreateLookAtFn =
        int(__cdecl*)(void*, void*, void*, void*);
	
    inline CreateLookAtFn eCreateLookAtMatrix =
        reinterpret_cast<CreateLookAtFn>(0x006CF0A0);

    // CALL site (not used by MinHook, but kept for reference)
    constexpr uintptr_t HookAddr = 0X0047DCBC;

    // Sim::GetTime (ADDRESS, not float!)
    constexpr uintptr_t SimGetTimeAddr = 0x006E8DE0;

    // ------------------------------------------------------------
    // Camera internals (v1.3 EN)
    // ------------------------------------------------------------
    constexpr uintptr_t CameraWrapperAddr = 0x0047DBF6;   // your wrapper start
    constexpr uintptr_t CreateLookAtAddr  = 0x006CF0A0;
    constexpr uintptr_t DisableTiltsAddr  = 0x0047D3C0;
}
