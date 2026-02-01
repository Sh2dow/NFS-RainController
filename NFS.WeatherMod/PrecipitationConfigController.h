#pragma once
#include <IniReader.h>
#include <string>

#include "CPatch.h"

namespace PrecipitationConfigController
{
    inline int toggleKey = VK_F3; // Default to F3

    void Load();
    void LoadOnStartup();

    inline struct PrecipitationData
    {
        std::string raindropTexturePath;
        std::string presetName;

        float fpsOverride;
        
        bool enableOnStartup;
        bool enable2DRain;
        bool enable3DRain;
        bool enable3DSplatters;

        bool use_raindrop_dds;

        float rainIntensity;
        float fogIntensity;
        float transitionSeconds;

        // 2D overlay config
        int drop2DCount;

        bool alphaBlend2DRain;
        int alpha2DRainMin;
        int alpha2DRainMax;
        
        float baseSpeed;
        float speedScale;
        float baseLength;
        float lengthScale;
        float windStrength;

        // 3D group Y-offsets
        float nearMinOffset;
        float nearMaxOffset;
        float midMaxOffset;
        float farMaxOffset;

        // 3D group drop counts
        int dropCountNear;
        int dropCountMid;
        int dropCountFar;

        // Drop sizes
        float dropSizeNear;
        float dropSizeMid;
        float dropSizeFar;

        // Speeds
        float speedNear;
        float speedMid;
        float speedFar;

        // Wind sway
        float windSwayNear;
        float windSwayMid;
        float windSwayFar;

        // Alpha blending flags
        bool alphaBlend3DRainNear;
        bool alphaBlend3DRainMid;
        bool alphaBlend3DRainFar;

        // Alpha blending values
        int alphaBlendNearValue;
        int alphaBlendMidValue;
        int alphaBlendFarValue;

        // 3D Splatters
        bool alphaBlendSplatters;
        
        float occlusionZone_XMin;
        float occlusionZone_XMax;
        float occlusionZone_ZMin;
        float occlusionZone_ZMax;

        int rainMatrixMode;
        int useLookAtMatrix;
        int preferHookedView;

        bool applyPresetGlobals;
        bool applyPresetRendering;

        struct NativePreset
        {
            float rainCrossing;
            float rainFallSpeed;
            float rainGravity;
            float rainWindEff;
            float rainRadiusX;
            float rainRadiusY;
            float rainRadiusZ;
            float boundX;
            float boundY;
            float boundZ;
            float aheadX;
            float aheadY;
            float aheadZ;
            float driveFactor;
            float rainRateOfChange;
            float cloudsRateOfChange;
            float windAngle;
            float swayMax;
            float maxWindEff;
            float prevailingMult;
            float onScreenDripSpeed;
            float onScreenSpeedMod;
            float onScreenDropShapeSpeedChange;
            float baseDampness;
            float rainInHeadlights;
            float roadReflectionLevel;
        } nativePreset{};

        struct RenderPreset
        {
            int enable2DRain;
            int enable3DRain;
            int enable3DSplatters;
            int drop2DCount;
            float dropSizeNear;
            float dropSizeMid;
            float dropSizeFar;
            float speedNear;
            float speedMid;
            float speedFar;
            float windSwayNear;
            float windSwayMid;
            float windSwayFar;
            int dropCountNear;
            int dropCountMid;
            int dropCountFar;
        } renderPreset{};
    } precipitationConfig{};
}
