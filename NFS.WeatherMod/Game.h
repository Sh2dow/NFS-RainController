#pragma once
#include <cstdint>

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

    inline uintptr_t NSF_D3D9_DEVICE_ADDRESS = 0;
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

    inline uintptr_t eDisplayFrameAddr = 0;
    inline uintptr_t renderCtxAddr = 0;
    inline uintptr_t particleCtxAddr = 0;
    inline uintptr_t RainTickAddr = 0;
    inline uintptr_t InitViewsAddr = 0;
    
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
    
    inline void Init(GameType type)
    {
        switch (type)
        {
        case GameType::MW:
            Name = "NFSMW - Weather Mod";
            Error = "This .exe is not supported.";
            NSF_D3D9_DEVICE_ADDRESS = MW::NFS_D3D9_DEVICE_ADDRESS;
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
            
            eDisplayFrameAddr = MW::eDisplayFrameAddr;
            renderCtxAddr = MW::renderCtxAddr;
            particleCtxAddr = MW::particleCtxAddr;
            RainTickAddr = MW::RainTickAddr;
            InitViewsAddr = MW::epInitViewsAddr;
            
            g_originalRainTick   = reinterpret_cast<RainTick_t>(RainTick);
            g_originalRainUpdate = reinterpret_cast<RainUpdate_t>(RainUpdate);
            g_originalRainRender = reinterpret_cast<RainRender_t>(RainRender);
            g_originalRainRender3D = reinterpret_cast<RainRender3D_t>(RainRender3D);
            g_originalRainSetIntensity = reinterpret_cast<RainSetIntensity_t>(RainSetIntensityAddr);
            g_originalRainSetOverrideIntensity = reinterpret_cast<RainSetOverrideIntensity_t>(RainSetOverrideIntensityAddr);
            g_originalGameSetChanceOfRain = reinterpret_cast<GameSetChanceOfRain_t>(GameSetChanceOfRainAddr);
            g_originalInitViews = reinterpret_cast<InitViews_t>(InitViewsAddr);
            
            break;
        case GameType::CB:
            Name = "NFSC - Weather Mod";
            Error = "This .exe is not supported.";
            NSF_D3D9_DEVICE_ADDRESS = CB::NFS_D3D9_DEVICE_ADDRESS;
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
