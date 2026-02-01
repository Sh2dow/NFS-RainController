#pragma once

#include "NFSMW_PreFEngHook.h"
#include "NFSC_PreFEngHook.h"
#include "NFSPS_PreFEngHook.h"
#include "NFSUC_PreFEngHook.h"

enum class GameType
{
    MW,
    CB,
    UG2,
    UG,
    PS,
    UC,
    Unknown
};

extern GameType detected_game;

namespace Game
{
    inline const char* Name = "NFS - Weather Mod";
    inline const char* Error = "This .exe is not supported.";
    
    // Global addresses inferred from IDA
    inline uintptr_t GAMEFLOWMGR_STATUS_ADDR = 0;
    inline uintptr_t kWorldTimeElapsed = 0;
    inline uintptr_t PRECIP_RAINRATEOFCHANGE_ADDR = 0;
    inline uintptr_t PRECIP_CLOUDSRATEOFCHANGE_ADDR = 0;
    inline uintptr_t kParamMapLayerRain = 0;
    inline uintptr_t kParamMapLayerClouds = 0;
    inline uintptr_t kParamDataRain = 0;
    inline uintptr_t kParamDataClouds = 0;
    inline uintptr_t kOnlineFlag = 0;
    inline uintptr_t kCloudBase = 0;
    
    
    inline uintptr_t PRECIPITATION_DEBUG_ADDR = 0;
    inline uintptr_t PRECIPITATION_ENABLE_ADDR = 0;
    inline uintptr_t PRECIPITATION_RENDER_ADDR = 0;
    inline uintptr_t PRECIP_RAINPERCENT_ADDR = 0;
    inline uintptr_t PRECIP_FOGPERCENT_ADDR = 0;
    inline uintptr_t PRECIPITATION_PERCENT_ADDR = 0;
    inline uintptr_t PRECIP_RAINOVERRIDE_ADDR = 0;
    inline uintptr_t WEATHER_SKY_BLEND_ADDR = 0;
    inline uintptr_t StuffSkyLayerBlendAddr = 0;
    inline uintptr_t StuffSkyLayerBlendCallsite1 = 0;
    inline uintptr_t StuffSkyLayerAddr = 0;
    inline uintptr_t StuffSkyLayerCallsite5 = 0;
    inline uintptr_t StuffSkyLayerCallsite6 = 0;
    inline uintptr_t FXWeatherUpdateAddr = 0;
    inline uintptr_t g_pEAXSound_0_ADDR = 0;
    inline uintptr_t WeatherSkyBlendVarAddr = 0;
    inline uintptr_t ReplaceSkyTexturesAddr = 0;
    inline uintptr_t ReplaceSkyTexturesCallsite = 0;
    inline uintptr_t WeatherBlendAccumAddr = 0;
    inline uintptr_t WeatherBlendAccumWrite1 = 0;
    inline uintptr_t WeatherBlendAccumWrite2 = 0;
    inline uintptr_t WeatherSkyBlendWrite = 0;
    inline uintptr_t AttachReplacementTextureTableAddr = 0;
    inline uintptr_t SkyReplacementTableAddr = 0;
    inline uintptr_t SkyLayerComputeAddr = 0;
    inline uintptr_t SkyLayerComputeCallsite = 0;
    inline uintptr_t SkyRenderCtxPtrAddr = 0;
    inline uintptr_t TimeOfDayUpdateAddr = 0;
    inline uintptr_t FX_WEATHER_UPDATE_ADDR = 0;
    inline uintptr_t BaseFogFalloff_ADDR = 0;
    inline uintptr_t BaseFogFalloffX_ADDR = 0;
    inline uintptr_t BaseFogFalloffY_ADDR = 0;
    inline uintptr_t BaseWeatherFog_ADDR = 0;
    inline uintptr_t BaseWeatherFogStart_ADDR = 0;
    inline uintptr_t BaseWeatherFogColourR_ADDR = 0;
    inline uintptr_t BaseWeatherFogColourG_ADDR = 0;
    inline uintptr_t BaseWeatherFogColourB_ADDR = 0;
    inline uintptr_t HorizFogFalloff_ADDR = 0;
    inline uintptr_t HorizFogFalloffY_ADDR = 0;
    inline uintptr_t HorizWeatherFog_ADDR = 0;
    inline uintptr_t HorizWeatherFogStart_ADDR = 0;
    inline uintptr_t FOG_CTRLOVERRIDE_ADDR = 0;
    
    inline uintptr_t PRECIP_BASEDAMPNESS_ADDR = 0;
    inline uintptr_t PRECIP_DRIVEFACTOR_ADDR = 0;
    inline uintptr_t PRECIP_RAINX_ADDR = 0;
    inline uintptr_t PRECIP_RAINY_ADDR = 0;
    inline uintptr_t PRECIP_RAINZ_ADDR = 0;
    inline uintptr_t PRECIP_RAINZCONSTANT_ADDR = 0;
    inline uintptr_t PRECIP_RAINRADIUSX_ADDR = 0;
    inline uintptr_t PRECIP_RAINRADIUSY_ADDR = 0;
    inline uintptr_t PRECIP_RAINRADIUSZ_ADDR = 0;
    inline uintptr_t PRECIP_RAINWINDEFF_ADDR = 0;
    inline uintptr_t PRECIP_BOUNDX_ADDR = 0;
    inline uintptr_t PRECIP_BOUNDY_ADDR = 0;
    inline uintptr_t PRECIP_BOUNDZ_ADDR = 0;
    inline uintptr_t PRECIP_AHEADX_ADDR = 0;
    inline uintptr_t PRECIP_AHEADY_ADDR = 0;
    inline uintptr_t PRECIP_AHEADZ_ADDR = 0;
    inline uintptr_t PRECIP_RAININTHEHEADLIGHTS_ADDR = 0;
    inline uintptr_t PRECIP_WINDANG_ADDR = 0;
    inline uintptr_t PRECIP_SWAYMAX_ADDR = 0;
    inline uintptr_t PRECIP_MAXWINDEFF_ADDR = 0;
    inline uintptr_t PRECIP_PREVAILINGMULT_ADDR = 0;
    inline uintptr_t PRECIP_ONSCREEN_DRIPSPEED_ADDR = 0;
    inline uintptr_t PRECIP_ONSCREEN_SPEEDMOD_ADDR = 0;
    inline uintptr_t PRECIP_ONSCREEN_DROPSHAPESPEEDCHANGE_ADDR = 0;
    
    inline uintptr_t RoadReflectionStateAddr = 0;
    inline uintptr_t RoadReflectionEnablePtr = 0;
    inline uintptr_t NFS_D3D9_DEVICE_ADDRESS = 0;
    inline uintptr_t NodeMatrixOffset = 0;
    
    inline uintptr_t EViewArrayBase = 0; // eViews[22]
    inline uintptr_t EViewArrayCount = 0;
    inline uintptr_t EViewSize = 0;
    inline uintptr_t EViewActiveFlagOffset = 0;
    inline uintptr_t EViewCameraOffset = 0; // eView::pCamera
    inline uintptr_t EViewCameraParamsOffset = 0; // eView::CameraParams*
    inline uintptr_t EViewMatrixOffset0 = 0; // bMatrix4
    inline uintptr_t EViewMatrixOffset1 = 0; // bMatrix4
    inline uintptr_t EViewMatrixOffset2 = 0; // bMatrix4 (alt)
    inline uintptr_t EViewMatrixOffset3 = 0; // bMatrix4 (alt)
    inline uintptr_t EViewAltMatrixFlagPtr = 0; // dword_982CB4
    inline uintptr_t CameraPositionOffset = 0; // CameraParams::Position
    inline uintptr_t CameraMatrixV3Offset = 0; // CameraParams::Matrix.v3 (fallback)

    // MW Rain instance and matrices
    inline uintptr_t RainInstancePtr = 0; // dword_9196B8
    inline uintptr_t RainViewMatrixOffset = 0;
    inline uintptr_t RainProjMatrixOffset = 0;
    inline uintptr_t RainRender3D = 0;
    inline uintptr_t RainRender = 0;
    inline uintptr_t RainUpdate = 0;
    inline uintptr_t RainTick = 0;
    inline uintptr_t RainUpdateCallsiteAddr = 0;
    inline uintptr_t RainRenderCallsiteAddr = 0;
    inline uintptr_t RainRenderCallsiteAddr2 = 0;
    inline uintptr_t RenderCtxCallsiteAddr = 0;
    inline uintptr_t RenderCtxCallsiteAddr2 = 0;
    inline uintptr_t RainSetIntensityAddr = 0;
    inline uintptr_t RainSetOverrideIntensityAddr = 0;
    inline uintptr_t GameSetChanceOfRainAddr = 0;
    inline uintptr_t RainEnablePtr = 0; // g_RainEnable
    inline uintptr_t ParticleSystemEnablePtr = 0; // g_ParticleSystemEnable

    inline uintptr_t CreateLookAtAddr = 0;
    inline uintptr_t BuildViewMatrixAddr = 0;
    inline uintptr_t BuildRenderViewAddr = 0;
    inline uintptr_t BuildRenderMatrixAddr = 0;
    inline uintptr_t EViewCurrentPtr = 0; // eViews_0

    inline uintptr_t EViewListHeadPtr = 0;

    inline uintptr_t FEMANAGER_INSTANCE_ADDR = 0;
    inline uintptr_t GAMEFLOWMGR_ADDR = 0;
    inline uintptr_t renderPlatAddr = 0;
    
    inline uintptr_t eDisplayFrameAddr = 0;
    inline uintptr_t renderCtxAddr = 0;
    inline uintptr_t particleCtxAddr = 0;
    inline uintptr_t RainTickAddr = 0;
    inline uintptr_t InitViewsAddr = 0;
    inline uintptr_t CurrentViewMode = 0;
    inline uintptr_t DripFreeze = 0;
    inline uintptr_t TunnelCameraRelative = 0;
    inline uintptr_t Normalize2DAddr = 0;
    inline uintptr_t FindBestFacingEdgeAddr = 0;
    inline uintptr_t TunnelBloom_SetParams = 0;
    
    inline uintptr_t PausedAddr = 0;
    inline uintptr_t AmIinATunnelSlowAddr = 0;
    
    using RainTick_t = void(__thiscall*)(void*);
    inline RainTick_t g_originalRainTick = nullptr;
    using RainUpdate_t = void(__thiscall*)(void*);
    inline RainUpdate_t g_originalRainUpdate = nullptr;
    using RainRender_t = void(__thiscall*)(void*);
    inline RainRender_t g_originalRainRender = nullptr;
    using RainRender3D_t = void(__thiscall*)(void*);
    inline RainRender3D_t g_originalRainRender3D = nullptr;
    using RainSetIntensity_t = void(__thiscall*)(void*, float);
    inline RainSetIntensity_t g_originalRainSetIntensity = nullptr;
    using RainSetOverrideIntensity_t = void(__cdecl*)(float);
    inline RainSetOverrideIntensity_t g_originalRainSetOverrideIntensity = nullptr;
    using GameSetChanceOfRain_t = void(__cdecl*)(float);
    inline GameSetChanceOfRain_t g_originalGameSetChanceOfRain = nullptr;
    using InitViews_t = int(__cdecl*)();
    inline InitViews_t g_originalInitViews = nullptr;
    using GetViewMode_t = int(__cdecl*)();
    inline GetViewMode_t eCurrentViewMode = nullptr;
    
    inline void Init(GameType type)
    {
        switch (type)
        {
        case GameType::MW:
            Name = "NFSMW - Weather Mod";
            Error = "This .exe is not supported.";
            NFS_D3D9_DEVICE_ADDRESS = MW::NFS_D3D9_DEVICE_ADDRESS;
            NodeMatrixOffset = 0x40;
            EViewArrayBase = MW::EViewArrayBase;
            EViewArrayCount = MW::EViewArrayCount;
            EViewSize = MW::EViewSize;
            EViewActiveFlagOffset = MW::EViewActiveFlagOffset;
            EViewCameraOffset = MW::EViewCameraOffset;
            EViewCameraParamsOffset = MW::EViewCameraParamsOffset;
            EViewMatrixOffset0 = MW::EViewMatrixOffset0;
            EViewMatrixOffset1 = MW::EViewMatrixOffset1;
            EViewMatrixOffset2 = MW::EViewMatrixOffset2;
            EViewMatrixOffset3 = MW::EViewMatrixOffset3;
            EViewAltMatrixFlagPtr = MW::EViewAltMatrixFlagPtr;
            CameraPositionOffset = MW::CameraPositionOffset;
            CameraMatrixV3Offset = MW::CameraMatrixV3Offset;

            PRECIPITATION_DEBUG_ADDR = MW::PRECIPITATION_DEBUG_ADDR;
            PRECIP_RAINX_ADDR = MW::PRECIP_RAINX_ADDR;
            PRECIP_RAINY_ADDR = MW::PRECIP_RAINY_ADDR;
            PRECIP_RAINZ_ADDR = MW::PRECIP_RAINZ_ADDR;
            PRECIP_RAINZCONSTANT_ADDR = MW::PRECIP_RAINZCONSTANT_ADDR;
            PRECIP_RAINRADIUSX_ADDR = MW::PRECIP_RAINRADIUSX_ADDR;
            PRECIP_RAINRADIUSY_ADDR = MW::PRECIP_RAINRADIUSY_ADDR;
            PRECIP_RAINRADIUSZ_ADDR = MW::PRECIP_RAINRADIUSZ_ADDR;
            PRECIP_RAINWINDEFF_ADDR = MW::PRECIP_RAINWINDEFF_ADDR;
            PRECIP_BOUNDX_ADDR = MW::PRECIP_BOUNDX_ADDR;
            PRECIP_BOUNDY_ADDR = MW::PRECIP_BOUNDY_ADDR;
            PRECIP_BOUNDZ_ADDR = MW::PRECIP_BOUNDZ_ADDR;
            PRECIP_AHEADX_ADDR = MW::PRECIP_AHEADX_ADDR;
            PRECIP_AHEADY_ADDR = MW::PRECIP_AHEADY_ADDR;
            PRECIP_AHEADZ_ADDR = MW::PRECIP_AHEADZ_ADDR;
            PRECIP_RAININTHEHEADLIGHTS_ADDR = MW::PRECIP_RAININTHEHEADLIGHTS_ADDR;
            PRECIP_WINDANG_ADDR = MW::PRECIP_WINDANG_ADDR;
            PRECIP_SWAYMAX_ADDR = MW::PRECIP_SWAYMAX_ADDR;
            PRECIP_MAXWINDEFF_ADDR = MW::PRECIP_MAXWINDEFF_ADDR;
            PRECIP_PREVAILINGMULT_ADDR = MW::PRECIP_PREVAILINGMULT_ADDR;
            PRECIP_ONSCREEN_DRIPSPEED_ADDR = MW::PRECIP_ONSCREEN_DRIPSPEED_ADDR;
            PRECIP_ONSCREEN_SPEEDMOD_ADDR = MW::PRECIP_ONSCREEN_SPEEDMOD_ADDR;
            PRECIP_ONSCREEN_DROPSHAPESPEEDCHANGE_ADDR = MW::PRECIP_ONSCREEN_DROPSHAPESPEEDCHANGE_ADDR;
            RainInstancePtr = MW::RainInstancePtr;
            RainViewMatrixOffset = MW::RainViewMatrixOffset;
            RainProjMatrixOffset = MW::RainProjMatrixOffset;
            RainRender3D = MW::RainRender3D;
            RainRender = MW::RainRender;
            RainUpdate = MW::RainUpdate;
            RainTick = MW::RainTick;
            RainUpdateCallsiteAddr = MW::RainUpdateCallsiteAddr;
            RainRenderCallsiteAddr = MW::RainRenderCallsiteAddr;
            RainRenderCallsiteAddr2 = MW::RainRenderCallsiteAddr2;
            RenderCtxCallsiteAddr = MW::RenderCtxCallsiteAddr;
            RenderCtxCallsiteAddr2 = MW::RenderCtxCallsiteAddr2;
            RainSetIntensityAddr = MW::RainSetIntensityAddr;
            RainSetOverrideIntensityAddr = MW::RainSetOverrideIntensityAddr;
            GameSetChanceOfRainAddr = MW::GameSetChanceOfRainAddr;
            RainEnablePtr = MW::RainEnablePtr;
            ParticleSystemEnablePtr = MW::ParticleSystemEnablePtr;

            CreateLookAtAddr = MW::CreateLookAtAddr;
            BuildViewMatrixAddr = MW::BuildViewMatrixAddr;
            BuildRenderViewAddr = MW::BuildRenderViewAddr;
            BuildRenderMatrixAddr = MW::BuildRenderMatrixAddr;
            EViewCurrentPtr = MW::EViewCurrentPtr;
            
            FEMANAGER_INSTANCE_ADDR = MW::FEMANAGER_INSTANCE_ADDR;
            GAMEFLOWMGR_ADDR = MW::GAMEFLOWMGR_ADDR;
            renderPlatAddr = MW::renderPlatAddr;
            
            eDisplayFrameAddr = MW::eDisplayFrameAddr;
            renderCtxAddr = MW::renderCtxAddr;
            particleCtxAddr = MW::particleCtxAddr;
            RainTickAddr = MW::RainTickAddr;
            InitViewsAddr = MW::epInitViewsAddr;
            CurrentViewMode = MW::CurrentViewMode;
            DripFreeze = MW::DripFreeze;
            TunnelCameraRelative = MW::TunnelCameraRelative;
            Normalize2DAddr = MW::Normalize2DAddr;
            FindBestFacingEdgeAddr = MW::FindBestFacingEdgeAddr;
            TunnelBloom_SetParams = MW::TunnelBloom_SetParams;
            
            GAMEFLOWMGR_STATUS_ADDR = MW::GAMEFLOWMGR_STATUS_ADDR; // A1 905E9200
            kWorldTimeElapsed = MW::kWorldTimeElapsed;   // A1 70599200
            PRECIP_RAINRATEOFCHANGE_ADDR = MW::PRECIP_RAINRATEOFCHANGE_ADDR;   // 0x00904AC4
            PRECIP_CLOUDSRATEOFCHANGE_ADDR = MW::PRECIP_CLOUDSRATEOFCHANGE_ADDR; //0x00904AC8
            PRECIPITATION_ENABLE_ADDR = MW::PRECIPITATION_ENABLE_ADDR; //0x008F86E4
            PRECIPITATION_RENDER_ADDR = MW::PRECIPITATION_RENDER_ADDR; //0x00904AD0
            kParamMapLayerRain = MW::kParamMapLayerRain;
            kParamMapLayerClouds = MW::kParamMapLayerClouds;
            kParamDataRain = MW::kParamDataRain;
            kParamDataClouds = MW::kParamDataClouds;
            kOnlineFlag = MW::kOnlineFlag;
            kCloudBase = MW::kCloudBase;
            
            PRECIP_RAINPERCENT_ADDR = MW::PRECIP_RAINPERCENT_ADDR;
            PRECIP_FOGPERCENT_ADDR = MW::PRECIP_FOGPERCENT_ADDR;
            PRECIPITATION_PERCENT_ADDR = MW::PRECIPITATION_PERCENT_ADDR;
            PRECIP_RAINOVERRIDE_ADDR = MW::PRECIP_RAINOVERRIDE_ADDR;
            WEATHER_SKY_BLEND_ADDR = MW::WEATHER_SKY_BLEND_ADDR;
            StuffSkyLayerBlendAddr = MW::StuffSkyLayerBlendAddr;
            StuffSkyLayerBlendCallsite1 = MW::StuffSkyLayerBlendCallsite1;
            StuffSkyLayerAddr = MW::StuffSkyLayerAddr;
            StuffSkyLayerCallsite5 = MW::StuffSkyLayerCallsite5;
            StuffSkyLayerCallsite6 = MW::StuffSkyLayerCallsite6;
            FXWeatherUpdateAddr = MW::FXWeatherUpdateAddr;
            g_pEAXSound_0_ADDR = MW::g_pEAXSound_0_ADDR;
            WeatherSkyBlendVarAddr = MW::WeatherSkyBlendVarAddr;
            ReplaceSkyTexturesAddr = MW::ReplaceSkyTexturesAddr;
            ReplaceSkyTexturesCallsite = MW::ReplaceSkyTexturesCallsite;
            WeatherBlendAccumAddr = MW::WeatherBlendAccumAddr;
            WeatherBlendAccumWrite1 = MW::WeatherBlendAccumWrite1;
            WeatherBlendAccumWrite2 = MW::WeatherBlendAccumWrite2;
            WeatherSkyBlendWrite = MW::WeatherSkyBlendWrite;
            AttachReplacementTextureTableAddr = MW::AttachReplacementTextureTableAddr;
            SkyReplacementTableAddr = MW::SkyReplacementTableAddr;
            SkyLayerComputeAddr = MW::SkyLayerComputeAddr;
            SkyLayerComputeCallsite = MW::SkyLayerComputeCallsite;
            SkyRenderCtxPtrAddr = MW::SkyRenderCtxPtrAddr;
            TimeOfDayUpdateAddr = MW::TimeOfDayUpdateAddr;
            PRECIP_BASEDAMPNESS_ADDR = MW::PRECIP_BASEDAMPNESS_ADDR;
            PRECIP_DRIVEFACTOR_ADDR = MW::PRECIP_DRIVEFACTOR_ADDR;
            FX_WEATHER_UPDATE_ADDR = MW::FX_WEATHER_UPDATE_ADDR;
            BaseFogFalloff_ADDR = MW::BaseFogFalloff_ADDR;
            BaseFogFalloffX_ADDR = MW::BaseFogFalloffX_ADDR;
            BaseFogFalloffY_ADDR = MW::BaseFogFalloffY_ADDR;
            BaseWeatherFog_ADDR = MW::BaseWeatherFog_ADDR;
            BaseWeatherFogStart_ADDR = MW::BaseWeatherFogStart_ADDR;
            BaseWeatherFogColourR_ADDR = MW::BaseWeatherFogColourR_ADDR;
            BaseWeatherFogColourG_ADDR = MW::BaseWeatherFogColourG_ADDR;
            BaseWeatherFogColourB_ADDR = MW::BaseWeatherFogColourB_ADDR;
            HorizFogFalloff_ADDR = MW::HorizFogFalloff_ADDR;
            HorizFogFalloffY_ADDR = MW::HorizFogFalloffY_ADDR;
            HorizWeatherFog_ADDR = MW::HorizWeatherFog_ADDR;
            HorizWeatherFogStart_ADDR = MW::HorizWeatherFogStart_ADDR;
            FOG_CTRLOVERRIDE_ADDR = MW::FOG_CTRLOVERRIDE_ADDR;
            RoadReflectionStateAddr = MW::RoadReflectionStateAddr;
            RoadReflectionEnablePtr = MW::RoadReflectionEnablePtr;
            
            g_originalRainTick   = reinterpret_cast<RainTick_t>(RainTick);
            g_originalRainUpdate = reinterpret_cast<RainUpdate_t>(RainUpdate);
            g_originalRainRender = reinterpret_cast<RainRender_t>(RainRender);
            g_originalRainRender3D = reinterpret_cast<RainRender3D_t>(RainRender3D);
            g_originalRainSetIntensity = reinterpret_cast<RainSetIntensity_t>(RainSetIntensityAddr);
            g_originalRainSetOverrideIntensity = reinterpret_cast<RainSetOverrideIntensity_t>(RainSetOverrideIntensityAddr);
            g_originalGameSetChanceOfRain = reinterpret_cast<GameSetChanceOfRain_t>(GameSetChanceOfRainAddr);
            g_originalInitViews = reinterpret_cast<InitViews_t>(InitViewsAddr);
            eCurrentViewMode = reinterpret_cast<InitViews_t>(CurrentViewMode);
            
            AmIinATunnelSlowAddr = MW::AmIinATunnelSlowAddr;
            PausedAddr = MW::PausedAddr;
            
            break;
        case GameType::CB:
            Name = "NFSC - Weather Mod";
            Error = "This .exe is not supported.";
            NFS_D3D9_DEVICE_ADDRESS = CB::NFS_D3D9_DEVICE_ADDRESS;
            // NodeMatrixOffset = 0x40;
            // EViewArrayBase = CB::EViewArrayBase;
            // EViewArrayCount = CB::EViewArrayCount;
            // EViewSize = CB::EViewSize;
            // EViewActiveFlagOffset = CB::EViewActiveFlagOffset;
            // EViewCameraOffset = CB::EViewCameraOffset;
            // EViewCameraParamsOffset = CB::EViewCameraParamsOffset;
            // EViewMatrixOffset0 = CB::EViewMatrixOffset0;
            // EViewMatrixOffset1 = CB::EViewMatrixOffset1;
            // EViewMatrixOffset2 = CB::EViewMatrixOffset2;
            // EViewMatrixOffset3 = CB::EViewMatrixOffset3;
            // EViewAltMatrixFlagPtr = CB::EViewAltMatrixFlagPtr;
            // CameraPositionOffset = CB::CameraPositionOffset;
            // CameraMatrixV3Offset =  CB::CameraMatrixV3Offset;

            PRECIPITATION_DEBUG_ADDR = CB::PRECIPITATION_DEBUG_ADDR;
            FEMANAGER_INSTANCE_ADDR = CB::FEMANAGER_INSTANCE_ADDR;
            break;
        case GameType::PS:
            
            EViewListHeadPtr = PS::EViewListHeadPtr;
            
            FEMANAGER_INSTANCE_ADDR = PS::FEMANAGER_INSTANCE_ADDR;
            break;
        case GameType::UC:
            
            EViewListHeadPtr = UC::EViewListHeadPtr;
            
            FEMANAGER_INSTANCE_ADDR = UC::FEMANAGER_INSTANCE_ADDR;
            break;
        default:
            Name = "NFS - Weather Mod";
            Error = "This .exe is not supported.";
            break;
        }
    }
}
