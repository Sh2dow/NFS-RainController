#pragma once
namespace CB
{
    // NFS CB - ReShade Pre FENg Hook
#ifndef HAS_COPS
#define HAS_COPS
#endif
#ifndef HAS_FOG_CTRL
#define HAS_FOG_CTRL
#endif
#ifndef HAS_DAL
#define HAS_DAL
#endif
    constexpr uintptr_t FEMANAGER_RENDER_HOOKADDR1 = 0x00731138;
    constexpr uintptr_t FEMANAGER_RENDER_HOOKADDR2 = 0x00731138;
    constexpr uintptr_t FEMANAGER_RENDER_ADDRESS = 0x005915D0;
    constexpr uintptr_t NFS_D3D9_DEVICE_ADDRESS = 0x00AB0ABC;

    constexpr uintptr_t DRAW_FENG_BOOL_ADDR = 0x00A5E20C;

    constexpr uintptr_t SIM_SETSTREAM_ADDR = 0x00764D40;
    constexpr uintptr_t WCOLMGR_GETWORLDHEIGHT_ADDR = 0x00816DF0;
    constexpr uintptr_t PLAYER_LISTABLESET_ADDR = 0x00A9FF2C;
    constexpr uintptr_t SAVEHOTPOS_ADDR = 0x00B74BF8;
    constexpr uintptr_t LOADHOTPOS_ADDR = 0x00B74BFC;

    constexpr uintptr_t CHANGEPLAYERVEHICLE_ADDR = 0x00B74BED;

    constexpr uintptr_t EXITGAMEFLAG_ADDR = 0x00A99560;
    constexpr uintptr_t GAMEFLOWMGR_ADDR = 0x00A99B9C;
    constexpr uintptr_t GAMEFLOWMGR_STATUS_ADDR = 0x00A99BBC;
    constexpr uintptr_t GAMEFLOWMGR_UNLOADFE_ADDR = 0x006A8BD0;
    constexpr uintptr_t GAMEFLOWMGR_UNLOADTRACK_ADDR = 0x006BC600;
    constexpr uintptr_t GAMEFLOWMGR_LOADREGION_ADDR = 0x006BC5C0;
    constexpr uintptr_t SKIPFE_ADDR = 0x00A9E620;
    constexpr uintptr_t SKIPFETRACKNUM_ADDR = 0x00A7A1A8;
    constexpr uintptr_t SKIPFETRACKNUM_ADDR2 = 0x00A6313C;
    constexpr uintptr_t DEFAULT_TRACK_NUM = 5000;
    constexpr uintptr_t STARTSKIPFERACE_ADDR = 0x007A9B90;


    constexpr uintptr_t SKIPFE_NUMAICARS_ADDR = 0x00A9E630;
    constexpr uintptr_t SKIPFE_TRACKDIRECTION_ADDR = 0x00A9E634;
    constexpr uintptr_t SKIPFE_TRAFFICDENSITY_ADDR = 0x00A9E640;
    constexpr uintptr_t SKIPFE_TRAFFICONCOMING_ADDR = 0x00A63160;
    constexpr uintptr_t SKIPFE_DISABLETRAFFIC_ADDR = 0x00A9E644;
    //#define SKIPFE_DRAGRACE_ADDR 0x00864FC0
    //#define SKIPFE_DRIFTRACE_ADDR 0x00864FC4
    constexpr uintptr_t SKIPFE_P2P_ADDR = 0x00A9E64C;
    constexpr uintptr_t SKIPFE_NUMPLAYERCARS_ADDR = 0x00A63150;
    constexpr uintptr_t SKIPFE_NUMLAPS_ADDR = 0x00A63154;
    constexpr uintptr_t SKIPFE_RACETYPE_ADDR = 0x00A9E648;
    constexpr uintptr_t SKIPFE_DIFFICULTY_ADDR = 0x00A63164;

    constexpr uintptr_t SKIPFE_PLAYERPERFORMANCE_ADDR = 0x00A9E628;
    constexpr uintptr_t SKIPFE_SPLITSCREEN_ADDR = 0x00A9E62C;
    constexpr uintptr_t SKIPFE_RACEID_ADDR = 0x00A63140;
    constexpr uintptr_t SKIPFE_PLAYERCAR_ADDR = 0x00A63144;
    constexpr uintptr_t SKIPFE_PLAYERCAR2_ADDR = 0x00A63148;
    constexpr uintptr_t SKIPFE_OPPONENTPRESETRIDE_ADDR = 0x008F86B0;
    constexpr uintptr_t SKIPFE_DISABLECOPS_ADDR = 0x00A63158;
    constexpr uintptr_t SKIPFE_MAXCOPS_ADDR = 0x00A9E638;
    constexpr uintptr_t SKIPFE_HELICOPTER_ADDR = 0x00A9E63C;

    constexpr uintptr_t SKIPFE_NOWINGMAN_ADDR = 0x00A9E655;
    constexpr uintptr_t SKIPFE_PLAYERPRESETRIDE_ADDR = 0x00A63178;
    constexpr uintptr_t SKIPFE_WINGMANPRESETRIDE_ADDR = 0x00A6317C;
    constexpr uintptr_t SKIPFE_OPPONENTPRESETRIDE0_ADDR = 0x00A63180;
    constexpr uintptr_t SKIPFE_OPPONENTPRESETRIDE1_ADDR = 0x00A63184;
    constexpr uintptr_t SKIPFE_OPPONENTPRESETRIDE2_ADDR = 0x00A63188;
    constexpr uintptr_t SKIPFE_OPPONENTPRESETRIDE3_ADDR = 0x00A6318C;
    constexpr uintptr_t SKIPFE_OPPONENTPRESETRIDE4_ADDR = 0x00A63190;
    constexpr uintptr_t SKIPFE_OPPONENTPRESETRIDE5_ADDR = 0x00A63194;
    constexpr uintptr_t SKIPFE_OPPONENTPRESETRIDE6_ADDR = 0x00A63198;
    constexpr uintptr_t SKIPFE_OPPONENTPRESETRIDE7_ADDR = 0x00A6319C;


    constexpr uintptr_t AUGMENTEDDRIFT_ADDR = 0x00A9E65B;
    constexpr uintptr_t SMARTLOOKAHEADCAMERA_ADDR = 0x00A9E65C;
    constexpr uintptr_t FORCEFAKEBOSS_ADDR = 0x00A9E66C;

    inline const char* DEFAULT_PLAYERCAR = "viper";
    inline const char* DEFAULT_PLAYER2CAR = "viper";

    constexpr uintptr_t UNLOCKALLTHINGS_ADDR = 0x00A9E6C0;
    constexpr uintptr_t SKIPCAREERINTRO_ADDR = 0x00A9E6C1;
    constexpr uintptr_t SKIPDDAYRACES_ADDR = 0x00A9E6C2;
    constexpr uintptr_t SHOWALLCARSINFE_ADDR = 0x00A9E6C3;
    constexpr uintptr_t CARGUYSCAMERA_ADDR = 0x00A9E6C8;

    constexpr uintptr_t DOSCREENPRINTF_ADDR = 0x00A9E6D4;
    constexpr uintptr_t ENABLEDCC_ADDR = 0x00A9E680;

    constexpr uintptr_t PRECIPITATION_ENABLE_ADDR = 0x00A631B0;
    constexpr uintptr_t PRECIPITATION_DEBUG_ADDR = 0x00B74D20;
    constexpr uintptr_t PRECIPITATION_RENDER_ADDR = 0x00A7988C;
    constexpr uintptr_t PRECIPITATION_PERCENT_ADDR = 0x00A797D0;
    constexpr uintptr_t PRECIP_RAINX_ADDR = 0x00A797DC;
    constexpr uintptr_t PRECIP_RAINY_ADDR = 0x00A797E0;
    constexpr uintptr_t PRECIP_RAINZ_ADDR = 0x00A797E4;
    constexpr uintptr_t PRECIP_RAINZCONSTANT_ADDR = 0x00A797E8;
    constexpr uintptr_t PRECIP_BOUNDX_ADDR = 0x00A7981C;
    constexpr uintptr_t PRECIP_BOUNDY_ADDR = 0x00A79820;
    constexpr uintptr_t PRECIP_BOUNDZ_ADDR = 0x00A79824;
    constexpr uintptr_t PRECIP_AHEADX_ADDR = 0x00A79828;
    constexpr uintptr_t PRECIP_AHEADY_ADDR = 0x00B74D24;
    constexpr uintptr_t PRECIP_AHEADZ_ADDR = 0x00B74D28;
    constexpr uintptr_t PRECIP_RAINWINDEFF_ADDR = 0x00A7983C;
    constexpr uintptr_t PRECIP_RAINRADIUSX_ADDR = 0x00A7984C;
    constexpr uintptr_t PRECIP_RAINRADIUSY_ADDR = 0x00A79850;
    constexpr uintptr_t PRECIP_RAINRADIUSZ_ADDR = 0x00A79854;
    constexpr uintptr_t PRECIP_DRIVEFACTOR_ADDR = 0x00A79888;
    constexpr uintptr_t PRECIP_RAINPERCENT_ADDR = 0x00A79894;
    constexpr uintptr_t PRECIP_FOGPERCENT_ADDR = 0x00B74D30;
    constexpr uintptr_t PRECIP_RAININTHEHEADLIGHTS_ADDR = 0x00A6C098;
    constexpr uintptr_t PRECIP_WINDANG_ADDR = 0x00B74D48;
    constexpr uintptr_t PRECIP_SWAYMAX_ADDR = 0x00A798B0;
    constexpr uintptr_t PRECIP_MAXWINDEFF_ADDR = 0x00A798B8;
    constexpr uintptr_t PRECIP_PREVAILINGMULT_ADDR = 0x00A798BC;
    constexpr uintptr_t PRECIP_ONSCREEN_DRIPSPEED_ADDR = 0x00A798C8;
    constexpr uintptr_t PRECIP_ONSCREEN_SPEEDMOD_ADDR = 0x00A798CC;
    constexpr uintptr_t PRECIP_ONSCREEN_DROPSHAPESPEEDCHANGE_ADDR = 0x00A798D0;
    //#define PRECIP_BASEDAMPNESS_ADDR 0x00904B38 // unknown, address is from MW as example
    constexpr uintptr_t PRECIP_WETDAMPNESS_ADDR = 0x00A798D4;
    constexpr uintptr_t PRECIP_DRYDAMPNESS_ADDR = 0x00A798D8;
    constexpr uintptr_t PRECIP_RAINRATEOFCHANGE_ADDR = 0x00A79880;
    constexpr uintptr_t PRECIP_CLOUDSRATEOFCHANGE_ADDR = 0x00A79884;
    constexpr uintptr_t PRECIP_CAMERAMOD_ADDR = 0x00A798A8;
    constexpr uintptr_t FOG_CTRLOVERRIDE_ADDR = 0x00B74D64;

    constexpr uintptr_t DISABLECOPS_ADDR = 0x00A83A50;
    constexpr uintptr_t CAMERADEBUGWATCHCAR_ADDR = 0x00A888F1;
    constexpr uintptr_t MTOGGLECAR_ADDR = 0x00A88914;
    constexpr uintptr_t MTOGGLECARLIST_ADDR = 0x00A88918;
    constexpr uintptr_t CAMERA_SETACTION_ADDR = 0x0048D620;

    constexpr uintptr_t CARLIST_TYPE_AIRACER = 3;
    constexpr uintptr_t CARLIST_TYPE_COP = 4;
    constexpr uintptr_t CARLIST_TYPE_TRAFFIC = 5;

    constexpr uintptr_t TOGGLEAICONTROL_ADDR = 0x00A83A6C;
    //#define MINIMAP_SHOWNONPURSUITCOPS_ADDR 0x0091CF00
    //#define MINIMAP_SHOWPURSUITCOPS_ADDR 0x008F3AAC
    // heat control stuff
    constexpr uintptr_t ADJUSTSTABLEHEAT_EVENTWIN_ADDR = 0x004BA130;
    constexpr uintptr_t HEATONEVENTWIN_HOOK_ADDR = 0x004BA181;
    constexpr uintptr_t HEATONEVENTWIN_ADDR = 0x004AF220;
    constexpr uintptr_t CAREERHEAT_OFFSET = 0x8;
    constexpr uintptr_t GETSIMABLE_OFFSET = 0x4;
    constexpr uintptr_t UTL_ILIST_FIND_ADDR = 0x0060CB50;
    constexpr uintptr_t IPERPETRATOR_HANDLE_ADDR = 0x004061D0;
    constexpr uintptr_t PERP_SETHEAT_OFFSET = 0x8;

    constexpr uintptr_t AI_RANDOMTURNS_ADDR = 0x00B77F00;

    // car flip stuff
    constexpr uintptr_t VEHICLE_LISTABLESET_ADDR = 0x00A9F1E4;
    constexpr uintptr_t IRIGIDBODY_HANDLE_ADDR = 0x00403750;
    constexpr uintptr_t RB_GETMATRIX4_OFFSET = 0x40;
    constexpr uintptr_t RB_SETORIENTATION_OFFSET = 0x70;

    // infinite cheats stuff
    constexpr uintptr_t EASTEREGG_CHECK_FUNC = 0x004AAE60;
    constexpr uintptr_t INFINITENOS_HOOK = 0x006E3F94;
    constexpr uintptr_t INFINITERB_HOOK = 0x00761DDF;
    constexpr uintptr_t INFINITERACEBREAKER_ADDR = 0x00B4D86C;

    constexpr uintptr_t PRECULLERMODE_ADDR = 0x00A72C28;

    constexpr uintptr_t FENG_MINSTANCE_ADDR = 0x00A97A78;
    constexpr uintptr_t FENG_QUEUEPACKAGEPOP_ADDR = 0x005840F0;
    constexpr uintptr_t FENG_QUEUEPACKAGESWITCH_ADDR = 0x005914D0;

    constexpr uintptr_t GAMENOTIFYRACEFINISHED_ADDR = 0x00660D20;
    constexpr uintptr_t GAMEENTERPOSTRACEFLOW_ADDR = 0x0065EF00;

    constexpr uintptr_t APPLYVISUALLOOK_ADDR = 0x00A65394;
    constexpr uintptr_t DRAWWORLD_ADDR = 0x00A63E0C;
    constexpr uintptr_t DRAWCARS_ADDR = 0x00A73008;
    constexpr uintptr_t DRAWCARSHADOWS_ADDR = 0x00A73010;
    constexpr uintptr_t GAMESPEED_ADDR = 0x00A712AC;

    constexpr uintptr_t GRACESTATUS_ADDR = 0x00A98284;

    constexpr uintptr_t PUSHMOVIE_ADDR = 0x005916E0;
    constexpr uintptr_t ISMOVIEPLAYING_ADDR = 0x00A97A80;

    constexpr uintptr_t BASEFOG_FALLOFF_ADDR = 0x00A72C58;
    constexpr uintptr_t BASEFOG_FALLOFFX_ADDR = 0x00A72C5C;
    constexpr uintptr_t BASEFOG_FALLOFFY_ADDR = 0x00A72C60;
    constexpr uintptr_t BASEWEATHER_FOG_ADDR = 0x00A72C64;
    constexpr uintptr_t BASEWEATHER_FOG_START_ADDR = 0x00A72C68;
    constexpr uintptr_t BASEWEATHER_FOG_COLOUR_R_ADDR = 0x00A72C78;
    constexpr uintptr_t BASEWEATHER_FOG_COLOUR_G_ADDR = 0x00A72C7C;
    constexpr uintptr_t BASEWEATHER_FOG_COLOUR_B_ADDR = 0x00A72C80;
    // carbon specifics
    constexpr uintptr_t BASEFOGEND_NONPS2_ADDR = 0x00A72C6C;
    constexpr uintptr_t BASEWEATHERFOG_NONPS2_ADDR = 0x00A72C70;
    constexpr uintptr_t BASEFOGEXPONENT_NONPS2_ADDR = 0x00A72C74;
    constexpr uintptr_t BASESKYFOGFALLOFF_ADDR = 0x00A72C84;
    constexpr uintptr_t BASESKYFOGOFFSET_ADDR = 0x00A72C88;

    // player cash & DAL stuff
    constexpr uintptr_t FEMANAGER_INSTANCE_ADDR = 0x00A97A7C;
    #define USERPROFILE_POINTER (*(int*)(*(int*)((*(int*)FEMANAGER_INSTANCE_ADDR) + 0xD4)))
    
    #define PLAYERCASH_POINTER (USERPROFILE_POINTER + 0x24330)
    #define CURRENTBIN_POINTER (USERPROFILE_POINTER + 0x2432C)
    
    #define PLAYERREP_POINTER  (USERPROFILE_POINTER + 0x24334)
    #define CREWNAME_POINTER  (USERPROFILE_POINTER + 0x24314)
    #define PROFILENAME_POINTER  (USERPROFILE_POINTER + 0x10)
    #define CAR_FEPOSITION_POINTER  (USERPROFILE_POINTER + 0x2946A)
    inline uintptr_t CAR_FEPOSITION_COUNT = 10;
    // DAL options
    #define MASTERVOL_POINTER  (USERPROFILE_POINTER + 0x24230)
    #define SPEECHVOL_POINTER  (USERPROFILE_POINTER + 0x24234)
    #define FEMUSICVOL_POINTER  (USERPROFILE_POINTER + 0x24238)
    #define IGMUSICVOL_POINTER  (USERPROFILE_POINTER + 0x2423C)
    #define SOUNDEFFECTSVOL_POINTER  (USERPROFILE_POINTER + 0x24240)
    #define ENGINEVOL_POINTER  (USERPROFILE_POINTER + 0x24244)
    #define CARVOL_POINTER  (USERPROFILE_POINTER + 0x24248)
    #define AMBIENTVOL_POINTER  (USERPROFILE_POINTER + 0x2424C)
    #define SPEEDVOL_POINTER  (USERPROFILE_POINTER + 0x24250)

    #define HIGHLIGHTCAM_POINTER  (USERPROFILE_POINTER + 0x2421C)
    #define JUMPCAM_POINTER  (USERPROFILE_POINTER + 0x24219)
    #define TIMEOFDAY_POINTER  (USERPROFILE_POINTER + 0x241EC)
    #define DAMAGEON_POINTER  (USERPROFILE_POINTER + 0x2420B)
    #define AUTOSAVEON_POINTER  (USERPROFILE_POINTER + 0x24208)

    #define FESCALE_POINTER  (USERPROFILE_POINTER + 0x241E8)
    #define WIDESCREEN_POINTER  (USERPROFILE_POINTER + 0x241F4)

    #define FEPOSITION_POINTER  (USERPROFILE_POINTER + 0x24279)
    #define FELAPINFO_POINTER  (USERPROFILE_POINTER + 0x2427A)
    #define FESCORE_POINTER  (USERPROFILE_POINTER + 0x2427B)
    #define FELEADERBOARD_POINTER  (USERPROFILE_POINTER + 0x2427D)
    #define FECREWINFO_POINTER  (USERPROFILE_POINTER + 0x2427E)
    #define FETRANSMISSIONPROMPT_POINTER  (USERPROFILE_POINTER + 0x2427F)
    #define FERVM_POINTER  (USERPROFILE_POINTER + 0x24209)
    #define FEPIP_POINTER  (USERPROFILE_POINTER + 0x2420A)
    #define SPEEDOUNIT_POINTER  (USERPROFILE_POINTER + 0x2420C)

    #define DRIVEWITHANALOG_POINTER  (USERPROFILE_POINTER + 0x24280)
    #define INPUT_SENSITIVITYSETTING_POINTER  (USERPROFILE_POINTER + 0x24284)
    #define RUMBLEON_POINTER  (USERPROFILE_POINTER + 0x2427C)
    #define PCPADIDX_POINTER  (USERPROFILE_POINTER + 0x242D0)
    #define PCDEVTYPE_POINTER  (USERPROFILE_POINTER + 0x242D4)

    #define BRIGHTNESS_POINTER  (USERPROFILE_POINTER + 0x241F0)

    constexpr uintptr_t FAKEBOSS_COUNT = 6;

    void __stdcall FEManager_Render_Hook();
    void __stdcall FECareerRecord_AdjustHeatOnEventWin_Hook();
    bool __stdcall EasterEggCheck_Hook(int cheat);

    // ------------------------------------------------------------
    // Executable validation
    // ------------------------------------------------------------
    constexpr uintptr_t Entry = 0x0087E926;

    inline const char* Name =
        "NFSC - Camera Mod";

    inline const char* Error =
        "This .exe is not supported.\n"
        "Please use v1.4 English nfsc.exe (6.88 MB / 7,217,152 bytes).";

    // ------------------------------------------------------------
    // Engine globals
    // ------------------------------------------------------------
    inline float* DeltaTime = reinterpret_cast<float*>(0x00A99A5C);
    inline bool* IsPaused = nullptr;

    // ------------------------------------------------------------
    // Functions / addresses
    // ------------------------------------------------------------
    using CreateLookAtFn =
    int(__cdecl*)(void*, void*, void*, void*);

    inline CreateLookAtFn eCreateLookAtMatrix =
        reinterpret_cast<CreateLookAtFn>(0x0071B430);

    // CALL site (not used by MinHook, but kept for reference)
    constexpr uintptr_t HookAddr = 0x00492E5B;

    // Sim::GetTime (ADDRESS, not float!)
    constexpr uintptr_t SimGetTimeAddr = 0x0075CF60;

    // ------------------------------------------------------------
    // Camera internals (v1.4 EN)
    // ------------------------------------------------------------
    constexpr uintptr_t CameraWrapperAddr = 0x00492D80; // your wrapper start
    constexpr uintptr_t CreateLookAtAddr = 0x0071B430;
    constexpr uintptr_t DisableTiltsAddr = 0x00492353;
    constexpr uintptr_t CameraAnchor_Update = 0x0047968C;
}
