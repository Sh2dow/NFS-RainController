#pragma once

namespace PrecipitationFlowMW
{
    void Tick();
    void Disable();
    void EnforceState(bool enable);
    float GetSmoothedRain();
    float GetSmoothedFog();
    void SetUseGameSkyFlow(bool useGameSkyFlow);
    void SetTargets(float rain, float fog);
    
}
