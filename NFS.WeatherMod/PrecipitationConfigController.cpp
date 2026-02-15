#include "stdafx.h"
#include "PrecipitationConfigController.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include "IniReader.h"

static PrecipitationConfigController::PrecipitationData::NativePreset MakePreset(const std::string& name)
{
    using Preset = PrecipitationConfigController::PrecipitationData::NativePreset;
    Preset p{};
    
    if (name == "Rain")
    {
        // Default Rain preset (based on MW defaults).
        p.rainCrossing = 0.02f;
        p.rainFallSpeed = 0.03f;
        p.rainGravity = 0.35f;
        p.rainWindEff = 0.30f;
        p.rainRadiusX = 0.01f;
        p.rainRadiusY = 0.45f;
        p.rainRadiusZ = 0.05f;
        p.boundX = 20.0f;
        p.boundY = 7.0f;
        p.boundZ = 6.0f;
        p.aheadX = 8.0f;
        p.aheadY = 0.0f;
        p.aheadZ = 0.0f;
        p.driveFactor = -0.03f;
        p.rainRateOfChange = 1.0f;
        p.cloudsRateOfChange = 10.0f;
        p.windAngle = 0.0f;
        p.swayMax = 1.5f;
        p.maxWindEff = 25.0f;
        p.prevailingMult = 0.01f;
        p.onScreenDripSpeed = 0.2f;
        p.onScreenSpeedMod = 0.0001f;
        p.onScreenDropShapeSpeedChange = 0.0025f;
        p.baseDampness = 0.0f;
        p.rainInHeadlights = 1.0f;
        p.roadReflectionEnable = 1.0f;
    }
    if (name == "SnowLight")
    {
        p.rainCrossing = 0.005f;
        p.rainFallSpeed = 0.008f;
        p.rainGravity = 0.08f;
        p.rainWindEff = 0.15f;
        p.rainRadiusX = 0.02f;
        p.rainRadiusY = 0.25f;
        p.rainRadiusZ = 0.08f;
        p.boundX = 26.0f;
        p.boundY = 10.0f;
        p.boundZ = 9.0f;
        p.aheadX = 8.0f;
        p.aheadY = 0.0f;
        p.aheadZ = 0.0f;
        p.driveFactor = -0.02f;
        p.rainRateOfChange = 0.6f;
        p.cloudsRateOfChange = 8.0f;
        p.windAngle = 0.0f;
        p.swayMax = 2.5f;
        p.maxWindEff = 10.0f;
        p.prevailingMult = 0.004f;
        p.onScreenDripSpeed = 0.0f;
        p.onScreenSpeedMod = 0.0f;
        p.onScreenDropShapeSpeedChange = 0.0f;
        p.baseDampness = 0.0f;
        p.rainInHeadlights = 0.25f;
        p.roadReflectionEnable = 0.0f;
    }
    if (name == "SnowHeavy")
    {
        p.rainCrossing = 0.008f;
        p.rainFallSpeed = 0.012f;
        p.rainGravity = 0.12f;
        p.rainWindEff = 0.20f;
        p.rainRadiusX = 0.025f;
        p.rainRadiusY = 0.35f;
        p.rainRadiusZ = 0.10f;
        p.boundX = 30.0f;
        p.boundY = 12.0f;
        p.boundZ = 10.0f;
        p.aheadX = 10.0f;
        p.aheadY = 0.0f;
        p.aheadZ = 0.0f;
        p.driveFactor = -0.025f;
        p.rainRateOfChange = 0.5f;
        p.cloudsRateOfChange = 10.0f;
        p.windAngle = 0.0f;
        p.swayMax = 3.0f;
        p.maxWindEff = 12.0f;
        p.prevailingMult = 0.006f;
        p.onScreenDripSpeed = 0.0f;
        p.onScreenSpeedMod = 0.0f;
        p.onScreenDropShapeSpeedChange = 0.0f;
        p.baseDampness = 0.0f;
        p.rainInHeadlights = 0.35f;
        p.roadReflectionEnable = 0.0f;
    }
    if (name == "Blizzard")
    {
        p.rainCrossing = 0.01f;
        p.rainFallSpeed = 0.015f;
        p.rainGravity = 0.14f;
        p.rainWindEff = 0.35f;
        p.rainRadiusX = 0.03f;
        p.rainRadiusY = 0.45f;
        p.rainRadiusZ = 0.12f;
        p.boundX = 34.0f;
        p.boundY = 14.0f;
        p.boundZ = 12.0f;
        p.aheadX = 12.0f;
        p.aheadY = 0.0f;
        p.aheadZ = 0.0f;
        p.driveFactor = -0.03f;
        p.rainRateOfChange = 0.4f;
        p.cloudsRateOfChange = 12.0f;
        p.windAngle = 0.0f;
        p.swayMax = 3.5f;
        p.maxWindEff = 18.0f;
        p.prevailingMult = 0.01f;
        p.onScreenDripSpeed = 0.0f;
        p.onScreenSpeedMod = 0.0f;
        p.onScreenDropShapeSpeedChange = 0.0f;
        p.baseDampness = 0.0f;
        p.rainInHeadlights = 0.45f;
        p.roadReflectionEnable = 0.0f;
    }

    return p;
}

static PrecipitationConfigController::PrecipitationData::RenderPreset MakeRenderPreset(const std::string& name)
{
    using RenderPreset = PrecipitationConfigController::PrecipitationData::RenderPreset;
    RenderPreset p{};
    // Defaults: keep current config (set to -1 to indicate "no override").
    p.enable2DRain = -1;
    p.enable3DRain = -1;
    p.enable3DSplatters = -1;
    p.drop2DCount = -1;
    p.dropSizeNear = -1.0f;
    p.dropSizeMid = -1.0f;
    p.dropSizeFar = -1.0f;
    p.speedNear = -1.0f;
    p.speedMid = -1.0f;
    p.speedFar = -1.0f;
    p.windSwayNear = -1.0f;
    p.windSwayMid = -1.0f;
    p.windSwayFar = -1.0f;
    p.dropCountNear = -1;
    p.dropCountMid = -1;
    p.dropCountFar = -1;

    if (name == "SnowLight")
    {
        p.enable2DRain = 0;
        p.enable3DSplatters = 0;
        p.drop2DCount = 0;
        p.dropSizeNear = 4.0f;
        p.dropSizeMid = 3.0f;
        p.dropSizeFar = 2.0f;
        p.speedNear = 0.35f;
        p.speedMid = 0.30f;
        p.speedFar = 0.25f;
        p.windSwayNear = 0.6f;
        p.windSwayMid = 0.7f;
        p.windSwayFar = 0.8f;
        p.dropCountNear = 70;
        p.dropCountMid = 60;
        p.dropCountFar = 50;
    }
    if (name == "SnowHeavy")
    {
        p.enable2DRain = 0;
        p.enable3DSplatters = 0;
        p.drop2DCount = 0;
        p.dropSizeNear = 5.0f;
        p.dropSizeMid = 4.0f;
        p.dropSizeFar = 3.0f;
        p.speedNear = 0.30f;
        p.speedMid = 0.26f;
        p.speedFar = 0.22f;
        p.windSwayNear = 0.7f;
        p.windSwayMid = 0.8f;
        p.windSwayFar = 0.9f;
        p.dropCountNear = 120;
        p.dropCountMid = 100;
        p.dropCountFar = 80;
    }
    if (name == "Blizzard")
    {
        p.enable2DRain = 0;
        p.enable3DSplatters = 0;
        p.drop2DCount = 0;
        p.dropSizeNear = 5.5f;
        p.dropSizeMid = 4.5f;
        p.dropSizeFar = 3.5f;
        p.speedNear = 0.25f;
        p.speedMid = 0.22f;
        p.speedFar = 0.20f;
        p.windSwayNear = 1.0f;
        p.windSwayMid = 1.1f;
        p.windSwayFar = 1.2f;
        p.dropCountNear = 160;
        p.dropCountMid = 140;
        p.dropCountFar = 120;
    }

    return p;
}

static void ReadRenderPresetOverrides(CIniReader& iniReader,
                                      const std::string& section,
                                      PrecipitationConfigController::PrecipitationData::RenderPreset& p)
{
    p.enable2DRain = iniReader.ReadInteger(section, "Enable2DRain", p.enable2DRain);
    p.enable3DRain = iniReader.ReadInteger(section, "Enable3DRain", p.enable3DRain);
    p.enable3DSplatters = iniReader.ReadInteger(section, "Enable3DSplatters", p.enable3DSplatters);
    p.drop2DCount = iniReader.ReadInteger(section, "Drop2DCount", p.drop2DCount);
    p.dropSizeNear = iniReader.ReadFloat(section, "DropSizeNear", p.dropSizeNear);
    p.dropSizeMid = iniReader.ReadFloat(section, "DropSizeMid", p.dropSizeMid);
    p.dropSizeFar = iniReader.ReadFloat(section, "DropSizeFar", p.dropSizeFar);
    p.speedNear = iniReader.ReadFloat(section, "SpeedNear", p.speedNear);
    p.speedMid = iniReader.ReadFloat(section, "SpeedMid", p.speedMid);
    p.speedFar = iniReader.ReadFloat(section, "SpeedFar", p.speedFar);
    p.windSwayNear = iniReader.ReadFloat(section, "WindSwayNear", p.windSwayNear);
    p.windSwayMid = iniReader.ReadFloat(section, "WindSwayMid", p.windSwayMid);
    p.windSwayFar = iniReader.ReadFloat(section, "WindSwayFar", p.windSwayFar);
    p.dropCountNear = iniReader.ReadInteger(section, "DropCountNear", p.dropCountNear);
    p.dropCountMid = iniReader.ReadInteger(section, "DropCountMid", p.dropCountMid);
    p.dropCountFar = iniReader.ReadInteger(section, "DropCountFar", p.dropCountFar);
}

static void ReadPresetOverrides(CIniReader& iniReader,
                                const std::string& section,
                                PrecipitationConfigController::PrecipitationData::NativePreset& p)
{
    p.rainCrossing = iniReader.ReadFloat(section, "RainCrossing", p.rainCrossing);
    p.rainFallSpeed = iniReader.ReadFloat(section, "RainFallSpeed", p.rainFallSpeed);
    p.rainGravity = iniReader.ReadFloat(section, "RainGravity", p.rainGravity);
    p.rainWindEff = iniReader.ReadFloat(section, "RainWindEff", p.rainWindEff);
    p.rainRadiusX = iniReader.ReadFloat(section, "RainRadiusX", p.rainRadiusX);
    p.rainRadiusY = iniReader.ReadFloat(section, "RainRadiusY", p.rainRadiusY);
    p.rainRadiusZ = iniReader.ReadFloat(section, "RainRadiusZ", p.rainRadiusZ);
    p.boundX = iniReader.ReadFloat(section, "BoundX", p.boundX);
    p.boundY = iniReader.ReadFloat(section, "BoundY", p.boundY);
    p.boundZ = iniReader.ReadFloat(section, "BoundZ", p.boundZ);
    p.aheadX = iniReader.ReadFloat(section, "AheadX", p.aheadX);
    p.aheadY = iniReader.ReadFloat(section, "AheadY", p.aheadY);
    p.aheadZ = iniReader.ReadFloat(section, "AheadZ", p.aheadZ);
    p.driveFactor = iniReader.ReadFloat(section, "DriveFactor", p.driveFactor);
    p.rainRateOfChange = iniReader.ReadFloat(section, "RainRateOfChange", p.rainRateOfChange);
    p.cloudsRateOfChange = iniReader.ReadFloat(section, "CloudsRateOfChange", p.cloudsRateOfChange);
    p.windAngle = iniReader.ReadFloat(section, "WindAngle", p.windAngle);
    p.swayMax = iniReader.ReadFloat(section, "SwayMax", p.swayMax);
    p.maxWindEff = iniReader.ReadFloat(section, "MaxWindEff", p.maxWindEff);
    p.prevailingMult = iniReader.ReadFloat(section, "PrevailingMult", p.prevailingMult);
    p.onScreenDripSpeed = iniReader.ReadFloat(section, "OnScreenDripSpeed", p.onScreenDripSpeed);
    p.onScreenSpeedMod = iniReader.ReadFloat(section, "OnScreenSpeedMod", p.onScreenSpeedMod);
    p.onScreenDropShapeSpeedChange = iniReader.ReadFloat(section, "OnScreenDropShapeSpeedChange", p.onScreenDropShapeSpeedChange);
    p.baseDampness = iniReader.ReadFloat(section, "BaseDampness", p.baseDampness);
    p.rainInHeadlights = iniReader.ReadFloat(section, "RainInHeadlights", p.rainInHeadlights);
    p.roadReflectionEnable = iniReader.ReadFloat(section, "RoadReflectionEnable", p.roadReflectionEnable);
}

void PrecipitationConfigController::LoadOnStartup()
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

void PrecipitationConfigController::Load()
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
    precipitationConfig.presetName = iniReader.ReadString(section, "Preset", "Rain");
    precipitationConfig.applyPresetGlobals = iniReader.ReadInteger(section, "ApplyPresetGlobals", 1) != 0;
    precipitationConfig.applyPresetRendering = iniReader.ReadInteger(section, "ApplyPresetRendering", 1) != 0;

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
    precipitationConfig.forceOpaqueSnow = iniReader.ReadInteger(section, "ForceOpaqueSnow", 0) != 0;

    precipitationConfig.alphaBlendNearValue = iniReader.ReadInteger(section, "AlphaBlendNearValue", 0);
    precipitationConfig.alphaBlendMidValue = iniReader.ReadInteger(section, "AlphaBlendMidValue", 0);
    precipitationConfig.alphaBlendFarValue = iniReader.ReadInteger(section, "AlphaBlendFarValue", 0);
    precipitationConfig.alphaBoost3D = iniReader.ReadFloat(section, "AlphaBoost3D", 1.0f);

    precipitationConfig.alphaBlendSplatters = iniReader.ReadInteger(section, "AlphaBlendSplatters", 0);

    
    precipitationConfig.occlusionZone_XMin = iniReader.ReadFloat("OcclusionZone", "XMin", 0.0f);
    precipitationConfig.occlusionZone_XMax = iniReader.ReadFloat("OcclusionZone", "XMax", 0.0f);
    precipitationConfig.occlusionZone_ZMin = iniReader.ReadFloat("OcclusionZone", "ZMin", 0.0f);
    precipitationConfig.occlusionZone_ZMax = iniReader.ReadFloat("OcclusionZone", "ZMax", 0.0f);

    precipitationConfig.rainMatrixMode = iniReader.ReadInteger(section, "RainMatrixMode", 0);
    precipitationConfig.useLookAtMatrix = iniReader.ReadInteger(section, "UseMWLookAtMatrix", 0);
    precipitationConfig.preferHookedView = iniReader.ReadInteger(section, "PreferHookedView", 0);

    precipitationConfig.nativePreset = MakePreset(precipitationConfig.presetName);
    if (!precipitationConfig.presetName.empty())
    {
        std::string presetSection = "Preset." + precipitationConfig.presetName;
        ReadPresetOverrides(iniReader, presetSection, precipitationConfig.nativePreset);
    }

    precipitationConfig.renderPreset = MakeRenderPreset(precipitationConfig.presetName);
    if (!precipitationConfig.presetName.empty())
    {
        std::string presetRenderSection = "Preset." + precipitationConfig.presetName + ".Render";
        ReadRenderPresetOverrides(iniReader, presetRenderSection, precipitationConfig.renderPreset);
    }

    if (precipitationConfig.applyPresetRendering)
    {
        const auto& r = precipitationConfig.renderPreset;
        if (r.enable2DRain >= 0) precipitationConfig.enable2DRain = (r.enable2DRain != 0);
        if (r.enable3DRain >= 0) precipitationConfig.enable3DRain = (r.enable3DRain != 0);
        if (r.enable3DSplatters >= 0) precipitationConfig.enable3DSplatters = (r.enable3DSplatters != 0);
        if (r.drop2DCount >= 0) precipitationConfig.drop2DCount = r.drop2DCount;
        if (r.dropSizeNear >= 0.0f) precipitationConfig.dropSizeNear = r.dropSizeNear;
        if (r.dropSizeMid >= 0.0f) precipitationConfig.dropSizeMid = r.dropSizeMid;
        if (r.dropSizeFar >= 0.0f) precipitationConfig.dropSizeFar = r.dropSizeFar;
        if (r.speedNear >= 0.0f) precipitationConfig.speedNear = r.speedNear;
        if (r.speedMid >= 0.0f) precipitationConfig.speedMid = r.speedMid;
        if (r.speedFar >= 0.0f) precipitationConfig.speedFar = r.speedFar;
        if (r.windSwayNear >= 0.0f) precipitationConfig.windSwayNear = r.windSwayNear;
        if (r.windSwayMid >= 0.0f) precipitationConfig.windSwayMid = r.windSwayMid;
        if (r.windSwayFar >= 0.0f) precipitationConfig.windSwayFar = r.windSwayFar;
        if (r.dropCountNear >= 0) precipitationConfig.dropCountNear = r.dropCountNear;
        if (r.dropCountMid >= 0) precipitationConfig.dropCountMid = r.dropCountMid;
        if (r.dropCountFar >= 0) precipitationConfig.dropCountFar = r.dropCountFar;
    }

#ifdef _DEBUG
    char debugBuffer[512];
    sprintf_s(debugBuffer,
        "[WeatherMod] enableOnStartup=%d, enable2DRain=%d, enable3DRain=%d, enable3DSplatters=%d, rainIntensity=%.2f, fogIntensity=%.2f, preset=%s, applyPreset=%d\n",
        precipitationConfig.enableOnStartup ? 1 : 0,
        precipitationConfig.enable2DRain ? 1 : 0,
        precipitationConfig.enable3DRain ? 1 : 0,
        precipitationConfig.enable3DSplatters ? 1 : 0,
        precipitationConfig.rainIntensity,
        precipitationConfig.fogIntensity,
        precipitationConfig.presetName.c_str(),
        precipitationConfig.applyPresetGlobals ? 1 : 0
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
            OutputDebugStringA("[WeatherMod] Warning: raindropTexturePath does not exist\n");
        }

        precipitationConfig.use_raindrop_dds = true;
        precipitationConfig.raindropTexturePath = fullPath.string(); // or keep path if std::filesystem::path
    }

    OutputDebugStringA("[WeatherMod::Load] finished\n");
}
