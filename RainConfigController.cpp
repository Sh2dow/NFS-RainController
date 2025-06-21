#include "RainConfigController.h"
#include <stdafx.h>
#include <windows.h>
#include <string>
#include "IniReader.h"

using namespace ngg::common;


void RainConfigController::LoadOnStartup()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::string iniPath = std::string(buffer);
    iniPath = iniPath.substr(0, iniPath.find_last_of("\\/")) + "\\scripts\\NFS-RainController.ini";

    CIniReader iniReader(iniPath.c_str());
    g_precipitationConfig.enableOnStartup = iniReader.ReadInteger("Precipitation", "EnableOnStartup", 0) != 0;
}

void RainConfigController::Load()
{

    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::string iniPath = std::string(buffer);
    iniPath = iniPath.substr(0, iniPath.find_last_of("\\/")) + "\\scripts\\NFS-RainController.ini";

    CIniReader iniReader(iniPath.c_str());
    g_precipitationConfig.enable3DRain = iniReader.ReadInteger("Precipitation", "Enable3DRain", 0) != 0;
    g_precipitationConfig.enable2DRain = iniReader.ReadInteger("Precipitation", "Enable2DRain", 0) != 0;
    g_precipitationConfig.rainIntensity = iniReader.ReadFloat("Precipitation", "RainIntensity", 0.5f);
    g_precipitationConfig.fogIntensity = iniReader.ReadFloat("Precipitation", "FogIntensity", 0.25f);

    char debugBuffer[512];
    sprintf_s(debugBuffer,
        "[RainConfigController] enableOnStartup=%d, enable3DRain=%d, enable2DRain=%d, rainIntensity=%.2f, fogIntensity=%.2f\n",
        g_precipitationConfig.enableOnStartup ? 1 : 0,
        g_precipitationConfig.enable3DRain ? 1 : 0,
        g_precipitationConfig.enable2DRain ? 1 : 0,
        g_precipitationConfig.rainIntensity,
        g_precipitationConfig.fogIntensity
    );
    OutputDebugStringA(debugBuffer);

    // Handle Use_raindrop_dds as optional relative path
    std::string szRaindropTexturePath = iniReader.ReadString("Precipitation", "Use_raindrop_dds", "0");
    std::filesystem::path fullPath;

    // Normalize default value
    if (szRaindropTexturePath.empty() || szRaindropTexturePath == "0")
    {
        g_precipitationConfig.use_raindrop_dds = false;
        g_precipitationConfig.raindropTexturePath.clear();
    }
    else
    {
        fullPath = GetExeModulePath<std::filesystem::path>();
        fullPath.append(szRaindropTexturePath);

        if (!std::filesystem::exists(fullPath))
        {
            OutputDebugStringA("[RainConfig] Warning: raindropTexturePath does not exist\n");
        }

        g_precipitationConfig.use_raindrop_dds = true;
        g_precipitationConfig.raindropTexturePath = fullPath.string(); // or keep path if std::filesystem::path
    }

    OutputDebugStringA("[RainConfigController::Load] finished\n");
}
