#include "stdafx.h"
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
    precipitationConfig.enableOnStartup = iniReader.ReadInteger("Precipitation", "EnableOnStartup", 0) != 0;
    toggleKey = iniReader.ReadInteger("Precipitation", "ToggleKey", VK_F3);
}

void RainConfigController::Load()
{

    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::string iniPath = std::string(buffer);
    iniPath = iniPath.substr(0, iniPath.find_last_of("\\/")) + "\\scripts\\NFS-RainController.ini";

    CIniReader iniReader(iniPath.c_str());

    toggleKey = iniReader.ReadInteger("Precipitation", "ToggleKey", VK_F3);
    
    precipitationConfig.fpsOverride = iniReader.ReadFloat("Precipitation", "fpsOverride", 0.0f);

    precipitationConfig.enable2DRain = iniReader.ReadInteger("Precipitation", "Enable2DRain", 0);
    precipitationConfig.enable3DRain = iniReader.ReadInteger("Precipitation", "Enable3DRain", 0);
    precipitationConfig.enable3DSplatters = iniReader.ReadInteger("Precipitation", "Enable3DSplatters", 0);
    precipitationConfig.rainIntensity = iniReader.ReadFloat("Precipitation", "RainIntensity", 0.0f);
    precipitationConfig.fogIntensity = iniReader.ReadFloat("Precipitation", "FogIntensity", 0.0f);

    precipitationConfig.baseSpeed = iniReader.ReadFloat("Precipitation", "BaseSpeed", 0.0f);
    precipitationConfig.speedScale = iniReader.ReadFloat("Precipitation", "SpeedScale", 0.0f);
    precipitationConfig.baseLength = iniReader.ReadFloat("Precipitation", "BaseLength", 0.0f);
    precipitationConfig.lengthScale = iniReader.ReadFloat("Precipitation", "LengthScale", 0.0f);
    precipitationConfig.windStrength = iniReader.ReadFloat("Precipitation", "WindStrength", 0.0f);

    precipitationConfig.drop2DCount = iniReader.ReadInteger("Precipitation", "Drop2DCount", 0);
    
    precipitationConfig.alphaBlend2DRain = iniReader.ReadInteger("Precipitation", "AlphaBlend2DRain", 0);
    precipitationConfig.alpha2DRainMin = iniReader.ReadInteger("Precipitation", "Alpha2DRainMin", 32);
    precipitationConfig.alpha2DRainMax = iniReader.ReadInteger("Precipitation", "Alpha2DRainMax", 255);

    precipitationConfig.nearMinOffset = iniReader.ReadFloat("Precipitation", "NearMinOffset", 0.0f);
    precipitationConfig.nearMaxOffset = iniReader.ReadFloat("Precipitation", "NearMaxOffset", 0.0f);
    precipitationConfig.midMaxOffset  = iniReader.ReadFloat("Precipitation", "MidMaxOffset",   0.0f);
    precipitationConfig.farMaxOffset  = iniReader.ReadFloat("Precipitation", "FarMaxOffset", 0.0f);

    precipitationConfig.dropCountNear = iniReader.ReadInteger("Precipitation", "DropCountNear", 0);
    precipitationConfig.dropCountMid = iniReader.ReadInteger("Precipitation", "DropCountMid", 0);
    precipitationConfig.dropCountFar = iniReader.ReadInteger("Precipitation", "DropCountFar", 0);

    precipitationConfig.dropSizeNear = iniReader.ReadFloat("Precipitation", "DropSizeNear", 0.0f);
    precipitationConfig.dropSizeMid = iniReader.ReadFloat("Precipitation", "DropSizeMid", 0.0f);
    precipitationConfig.dropSizeFar = iniReader.ReadFloat("Precipitation", "DropSizeFar", 0.0f);

    precipitationConfig.speedNear = iniReader.ReadFloat("Precipitation", "SpeedNear", 0.0f);
    precipitationConfig.speedMid = iniReader.ReadFloat("Precipitation", "SpeedMid", 0.0f);
    precipitationConfig.speedFar = iniReader.ReadFloat("Precipitation", "SpeedFar", 0.0f);

    precipitationConfig.windSwayNear = iniReader.ReadFloat("Precipitation", "WindSwayNear", 0.0f);
    precipitationConfig.windSwayMid = iniReader.ReadFloat("Precipitation", "WindSwayMid", 0.0f);
    precipitationConfig.windSwayFar = iniReader.ReadFloat("Precipitation", "WindSwayFar", 0.0f);

    precipitationConfig.alphaBlend3DRainNear = iniReader.ReadInteger("Precipitation", "AlphaBlend3DRainNear", 0);
    precipitationConfig.alphaBlend3DRainMid = iniReader.ReadInteger("Precipitation", "AlphaBlend3DRainMid", 0);
    precipitationConfig.alphaBlend3DRainFar = iniReader.ReadInteger("Precipitation", "AlphaBlend3DRainFar", 0);

    precipitationConfig.alphaBlendNearValue = iniReader.ReadInteger("Precipitation", "AlphaBlendNearValue", 255);
    precipitationConfig.alphaBlendMidValue = iniReader.ReadInteger("Precipitation", "AlphaBlendMidValue", 255);
    precipitationConfig.alphaBlendFarValue = iniReader.ReadInteger("Precipitation", "AlphaBlendFarValue", 255);

    precipitationConfig.alphaBlendSplatters = iniReader.ReadInteger("Precipitation", "AlphaBlendSplatters", 0);

    
    precipitationConfig.occlusionZone_XMin = iniReader.ReadFloat("OcclusionZone", "XMin", 100.0f);
    precipitationConfig.occlusionZone_XMax = iniReader.ReadFloat("OcclusionZone", "XMax", 300.0f);
    precipitationConfig.occlusionZone_ZMin = iniReader.ReadFloat("OcclusionZone", "ZMin", 500.0f);
    precipitationConfig.occlusionZone_ZMax = iniReader.ReadFloat("OcclusionZone", "ZMax", 700.0f);

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
    std::string szRaindropTexturePath = iniReader.ReadString("Precipitation", "Use_raindrop_dds", "0");
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
