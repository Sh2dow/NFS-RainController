#pragma once
#include <string>

#include "CPatch.h"

namespace ngg::common::RainConfigController
{
    inline int toggleKey = VK_F3; // Default to F3

    void Load();
    void LoadOnStartup();

    inline struct PrecipitationData
    {
        std::string raindropTexturePath;

        float fpsOverride;
        
        bool enableOnStartup;
        bool enable2DRain;
        bool enable3DRain;
        bool enable3DSplatters;

        bool use_raindrop_dds;

        float rainIntensity;
        float fogIntensity;

        // 2D overlay config
        bool alphaBlend2DRain = true;
        int alpha2DRainMin = 32;
        int alpha2DRainMax = 255;
        
        float baseSpeed = 200.0f;
        float speedScale = 600.0f;
        float baseLength = 10.0f;
        float lengthScale = 30.0f;
        float windStrength = 30.0f;

        // 3D group Y-offsets
        float nearMinOffset = -9999.0f;
        float nearMaxOffset = -100.0f;
        float midMaxOffset = 45.0f;
        float farMaxOffset = 9999.0f;

        // 3D group drop counts
        int dropCountNear = 200;
        int dropCountMid = 100;
        int dropCountFar = 200;

        // Drop sizes
        float dropSizeNear = 2.5f;
        float dropSizeMid = 2.0f;
        float dropSizeFar = 1.0f;

        // Speeds
        float speedNear = 2.0f;
        float speedMid = 1.5f;
        float speedFar = 0.5f;

        // Wind sway
        float windSwayNear = 0.15f;
        float windSwayMid = 0.25f;
        float windSwayFar = 0.25f;

        // Alpha blending flags
        bool alphaBlend3DRainNear = true;
        bool alphaBlend3DRainMid = true;
        bool alphaBlend3DRainFar = true;

        // Alpha blending values
        int alphaBlendNearValue = 192;
        int alphaBlendMidValue = 220;
        int alphaBlendFarValue = 189;

        // 3D Splatters
        bool alphaBlendSplatters = true;
        
    } precipitationConfig{};
}
