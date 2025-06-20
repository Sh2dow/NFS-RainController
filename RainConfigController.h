#pragma once
#include "CPatch.h"

namespace ngg::common::RainConfigController
{
    inline int toggleKey = VK_F3; // Default to F3

    void Load();
    extern bool enabled;
    extern float rainIntensity;
    extern float fogIntensity;
}
