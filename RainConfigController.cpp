#include "RainConfigController.h"
#include <windows.h>
#include <string>
#include "GameAddresses.h"

using namespace ngg::common;

bool RainConfigController::enabled = false;
float RainConfigController::rainIntensity = 1.0f;
float RainConfigController::fogIntensity = 0.5f;

void RainConfigController::Load()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::string path = std::string(buffer);
    path = path.substr(0, path.find_last_of("\\/")) + "\\scripts\\RainController.ini";

    char temp[64];

    // Enabled (bool as string)
    GetPrivateProfileStringA("Precipitation", "Enabled", "true", temp, sizeof(temp), path.c_str());
    RainConfigController::enabled = (_stricmp(temp, "true") == 0 || _stricmp(temp, "1") == 0);

    // RainIntensity (float)
    GetPrivateProfileStringA("Precipitation", "RainIntensity", "1.0", temp, sizeof(temp), path.c_str());
    RainConfigController::rainIntensity = std::strtof(temp, nullptr);

    // FogIntensity (float)
    GetPrivateProfileStringA("Precipitation", "FogIntensity", "0.5", temp, sizeof(temp), path.c_str());
    RainConfigController::fogIntensity = std::strtof(temp, nullptr);
}
