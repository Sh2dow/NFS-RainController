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
        bool enableOnStartup;
        bool enable2DRain;
        bool enable3DRain;

        bool use_raindrop_dds;
        std::string raindropTexturePath;

        float rainIntensity;
        float fogIntensity;
    } g_precipitationConfig{};
}
