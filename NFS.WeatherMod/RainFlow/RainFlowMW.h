#pragma once
#include <cstdint>

namespace RainFlowMW
{
    void Tick();
    void Disable();
    void EnforceState(bool enable);
    float GetSmoothedRain();
    float GetSmoothedFog();
    void EnforceState(bool enable);
    void SetUseGameSkyFlow(bool useGameSkyFlow);
    void SetTargets(float rain, float fog);
    
}
