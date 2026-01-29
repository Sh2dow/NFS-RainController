#pragma once
#include <cstdint>

namespace WeatherGameAddresses
{
    // FE readiness pointers
    constexpr uintptr_t FeManagerInstance_PS = 0x00AB27D8;
    constexpr uintptr_t FeManagerInstance_UC = 0x00D992BC;
    constexpr uintptr_t FeManagerInstance_CB = 0x00A97A7C;
    constexpr uintptr_t FeManagerInstance_MW = 0x0091CADC; // FENG_MINSTANCE_ADDR

    // EVIEW list + matrix offsets (PS/UC)
    constexpr uintptr_t EViewListHeadPtr_PS = 0x00A79B44;
    constexpr uintptr_t EViewListHeadPtr_UC = 0x00A79B44;
    constexpr uintptr_t NodeMatrixOffset = 0x40;

    // MW view/camera chain
    constexpr uintptr_t EViewArrayBase_MW = 0x009195E0;   // eViews[22]
    constexpr uintptr_t EViewArrayCount_MW = 22;
    constexpr uintptr_t EViewSize_MW = 0x70;
    constexpr uintptr_t EViewActiveFlagOffset_MW = 0x08;
    constexpr uintptr_t EViewCameraOffset_MW = 0x38;      // eView::pCamera
    constexpr uintptr_t EViewCameraParamsOffset_MW = 0x40; // eView::CameraParams*
    constexpr uintptr_t EViewMatrixOffset0_MW = 0x40;     // bMatrix4
    constexpr uintptr_t EViewMatrixOffset1_MW = 0x80;     // bMatrix4
    constexpr uintptr_t EViewMatrixOffset2_MW = 0xC0;     // bMatrix4 (alt)
    constexpr uintptr_t EViewMatrixOffset3_MW = 0x100;    // bMatrix4 (alt)
    constexpr uintptr_t EViewAltMatrixFlagPtr_MW = 0x00982CB4; // dword_982CB4
    constexpr uintptr_t CameraPositionOffset_MW = 0x40;   // CameraParams::Position
    constexpr uintptr_t CameraMatrixV3Offset_MW = 0x30;   // CameraParams::Matrix.v3 (fallback)

    // MW Rain instance and matrices
    constexpr uintptr_t RainInstancePtr_MW = 0x009196B8;   // dword_9196B8
    constexpr uintptr_t RainViewMatrixOffset_MW = 0x3610;
    constexpr uintptr_t RainProjMatrixOffset_MW = 0x3650;
    constexpr uintptr_t RainRender3D_MW = 0x0074A5D0;
    constexpr uintptr_t RainRender_MW = 0x0074ACC0;
    constexpr uintptr_t RainTick_MW = 0x00758100;
    constexpr uintptr_t RainEnablePtr_MW = 0x00901810;          // g_RainEnable
    constexpr uintptr_t ParticleSystemEnablePtr_MW = 0x009017EC; // g_ParticleSystemEnable

    constexpr uintptr_t CreateLookAtAddr_MW = 0x006CF0A0;
    constexpr uintptr_t BuildViewMatrixAddr_MW = 0x006CF400;
    constexpr uintptr_t BuildRenderViewAddr_MW = 0x006CEEE0;
    constexpr uintptr_t BuildRenderMatrixAddr_MW = 0x006C8000;
    constexpr uintptr_t EViewCurrentPtr_MW = 0x009196B4; // eViews_0
}
