#include "stdafx.h"
#include "RainConfigController.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include "IniReader.h"

void RainConfigController::LoadOnStartup()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::string iniPath = std::string(buffer);
    std::string baseDir = iniPath.substr(0, iniPath.find_last_of("\\/"));
    iniPath = baseDir + "\\scripts\\NFS.WeatherMod.ini";
    if (!std::filesystem::exists(iniPath))
        iniPath = baseDir + "\\NFS.WeatherMod.ini";

    std::string section = "Precipitation";
    {
        std::ifstream file(iniPath);
        if (file)
        {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (content.find("[Precipitation]") == std::string::npos)
                section.clear();
        }
    }

    CIniReader iniReader(iniPath.c_str());
    precipitationConfig.enableOnStartup = iniReader.ReadInteger(section, "EnableOnStartup", 0) != 0;
    toggleKey = iniReader.ReadInteger(section, "ToggleKey", VK_F3);
}

void RainConfigController::Load()
{

    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::string iniPath = std::string(buffer);
    std::string baseDir = iniPath.substr(0, iniPath.find_last_of("\\/"));
    iniPath = baseDir + "\\scripts\\NFS.WeatherMod.ini";
    if (!std::filesystem::exists(iniPath))
        iniPath = baseDir + "\\NFS.WeatherMod.ini";

    CIniReader iniReader(iniPath.c_str());

    std::string section = "Precipitation";
    {
        std::ifstream file(iniPath);
        if (file)
        {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (content.find("[Precipitation]") == std::string::npos)
                section.clear();
        }
    }

    toggleKey = iniReader.ReadInteger(section, "ToggleKey", VK_F4);
    
    precipitationConfig.fpsOverride = iniReader.ReadFloat(section, "fpsOverride", 0.0f);

    precipitationConfig.enable2DRain = iniReader.ReadInteger(section, "Enable2DRain", 0);
    precipitationConfig.enable3DRain = iniReader.ReadInteger(section, "Enable3DRain", 0);
    precipitationConfig.enable3DSplatters = iniReader.ReadInteger(section, "Enable3DSplatters", 0);
    precipitationConfig.rainIntensity = iniReader.ReadFloat(section, "RainIntensity", 0.0f);
    precipitationConfig.fogIntensity = iniReader.ReadFloat(section, "FogIntensity", 0.0f);
    precipitationConfig.transitionSeconds = iniReader.ReadFloat(section, "TransitionSeconds", 5.0f);

    precipitationConfig.baseSpeed = iniReader.ReadFloat(section, "BaseSpeed", 0.0f);
    precipitationConfig.speedScale = iniReader.ReadFloat(section, "SpeedScale", 0.0f);
    precipitationConfig.baseLength = iniReader.ReadFloat(section, "BaseLength", 0.0f);
    precipitationConfig.lengthScale = iniReader.ReadFloat(section, "LengthScale", 0.0f);
    precipitationConfig.windStrength = iniReader.ReadFloat(section, "WindStrength", 0.0f);

    precipitationConfig.drop2DCount = iniReader.ReadInteger(section, "Drop2DCount", 0);
    
    precipitationConfig.alphaBlend2DRain = iniReader.ReadInteger(section, "AlphaBlend2DRain", 0);
    precipitationConfig.alpha2DRainMin = iniReader.ReadInteger(section, "Alpha2DRainMin", 0);
    precipitationConfig.alpha2DRainMax = iniReader.ReadInteger(section, "Alpha2DRainMax", 0);

    precipitationConfig.nearMinOffset = iniReader.ReadFloat(section, "NearMinOffset", 0.0f);
    precipitationConfig.nearMaxOffset = iniReader.ReadFloat(section, "NearMaxOffset", 0.0f);
    precipitationConfig.midMaxOffset  = iniReader.ReadFloat(section, "MidMaxOffset",   0.0f);
    precipitationConfig.farMaxOffset  = iniReader.ReadFloat(section, "FarMaxOffset", 0.0f);

    precipitationConfig.dropCountNear = iniReader.ReadInteger(section, "DropCountNear", 0);
    precipitationConfig.dropCountMid = iniReader.ReadInteger(section, "DropCountMid", 0);
    precipitationConfig.dropCountFar = iniReader.ReadInteger(section, "DropCountFar", 0);

    precipitationConfig.dropSizeNear = iniReader.ReadFloat(section, "DropSizeNear", 0.0f);
    precipitationConfig.dropSizeMid = iniReader.ReadFloat(section, "DropSizeMid", 0.0f);
    precipitationConfig.dropSizeFar = iniReader.ReadFloat(section, "DropSizeFar", 0.0f);

    precipitationConfig.speedNear = iniReader.ReadFloat(section, "SpeedNear", 0.0f);
    precipitationConfig.speedMid = iniReader.ReadFloat(section, "SpeedMid", 0.0f);
    precipitationConfig.speedFar = iniReader.ReadFloat(section, "SpeedFar", 0.0f);

    precipitationConfig.windSwayNear = iniReader.ReadFloat(section, "WindSwayNear", 0.0f);
    precipitationConfig.windSwayMid = iniReader.ReadFloat(section, "WindSwayMid", 0.0f);
    precipitationConfig.windSwayFar = iniReader.ReadFloat(section, "WindSwayFar", 0.0f);

    precipitationConfig.alphaBlend3DRainNear = iniReader.ReadInteger(section, "AlphaBlend3DRainNear", 0);
    precipitationConfig.alphaBlend3DRainMid = iniReader.ReadInteger(section, "AlphaBlend3DRainMid", 0);
    precipitationConfig.alphaBlend3DRainFar = iniReader.ReadInteger(section, "AlphaBlend3DRainFar", 0);

    precipitationConfig.alphaBlendNearValue = iniReader.ReadInteger(section, "AlphaBlendNearValue", 0);
    precipitationConfig.alphaBlendMidValue = iniReader.ReadInteger(section, "AlphaBlendMidValue", 0);
    precipitationConfig.alphaBlendFarValue = iniReader.ReadInteger(section, "AlphaBlendFarValue", 0);

    precipitationConfig.alphaBlendSplatters = iniReader.ReadInteger(section, "AlphaBlendSplatters", 0);

    
    precipitationConfig.occlusionZone_XMin = iniReader.ReadFloat("OcclusionZone", "XMin", 0.0f);
    precipitationConfig.occlusionZone_XMax = iniReader.ReadFloat("OcclusionZone", "XMax", 0.0f);
    precipitationConfig.occlusionZone_ZMin = iniReader.ReadFloat("OcclusionZone", "ZMin", 0.0f);
    precipitationConfig.occlusionZone_ZMax = iniReader.ReadFloat("OcclusionZone", "ZMax", 0.0f);

    precipitationConfig.rainMatrixMode = iniReader.ReadInteger(section, "RainMatrixMode", 0);
    precipitationConfig.useLookAtMatrix = iniReader.ReadInteger(section, "UseMWLookAtMatrix", 0);
    precipitationConfig.preferHookedView = iniReader.ReadInteger(section, "PreferHookedView", 0);

#ifdef _DEBUG
    char debugBuffer[512];
    sprintf_s(debugBuffer,
        "[RainConfigController] enableOnStartup=%d, enable2DRain=%d, enable3DRain=%d, enable3DSplatters=%d, rainIntensity=%.2f, fogIntensity=%.2f\n",
        precipitationConfig.enableOnStartup ? 1 : 0,
        precipitationConfig.enable2DRain ? 1 : 0,
        precipitationConfig.enable3DRain ? 1 : 0,
        precipitationConfig.enable3DSplatters ? 1 : 0,
        precipitationConfig.rainIntensity,
        precipitationConfig.fogIntensity
    );
    OutputDebugStringA(debugBuffer);
#endif
    
    // Handle Use_raindrop_dds as optional relative path
    std::string szRaindropTexturePath = iniReader.ReadString(section, "Use_raindrop_dds", "0");
    std::filesystem::path fullPath;

    // Normalize default value
    if (szRaindropTexturePath.empty() || szRaindropTexturePath == "0")
    {
        precipitationConfig.use_raindrop_dds = false;
        precipitationConfig.raindropTexturePath.clear();
    }
    else
    {
        fullPath = GetExeModulePath<std::filesystem::path>();
        fullPath.append(szRaindropTexturePath);

        if (!std::filesystem::exists(fullPath))
        {
            OutputDebugStringA("[RainConfig] Warning: raindropTexturePath does not exist\n");
        }

        precipitationConfig.use_raindrop_dds = true;
        precipitationConfig.raindropTexturePath = fullPath.string(); // or keep path if std::filesystem::path
    }

    OutputDebugStringA("[RainConfigController::Load] finished\n");
}
