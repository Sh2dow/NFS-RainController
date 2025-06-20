#include "ForcePrecipitation.h"
#include <d3dx9.h>
#include "RainConfigController.h"

#ifdef GAME_PS
#include "NFSPS_PreFEngHook.h"
#elif GAME_UC
#include "NFSUC_PreFEngHook.h"
#endif

using namespace ngg::common;

void ForcePrecipitation::PatchRainSettings(DWORD rainEnable)
{
    // Always zero the registry (may not affect runtime)
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\EA Games\\Need for Speed ProStreet", 0, KEY_SET_VALUE, &hKey) ==
        ERROR_SUCCESS)
    {
        RegSetValueExA(hKey, "g_RainEnable", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&rainEnable), sizeof(DWORD));
        RegCloseKey(hKey);
    }

    DWORD oldProtect;
    auto patch = [&](DWORD addr, float value)
    {
        if (float* ptr = reinterpret_cast<float*>(addr);
            VirtualProtect(ptr, sizeof(float), PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            *ptr = value;
            VirtualProtect(ptr, sizeof(float), oldProtect, &oldProtect);
        }
    };

    // Probably a leftover stuff, doesn't work either
    // patch(RAIN_CF_INTENSITY_ADDR, rainEnable ? m_precip.rainPercent : 0.0f); // cfRainIntensity
    // patch(RAIN_DROP_OFFSET_ADDR, rainEnable ? -10.0f : 0.0f); // RAINDROPOFFSET
    // patch(RAIN_DROP_ALPHA_ADDR, rainEnable ? m_precip.rainPercent : 0.0f); // RAINDROPALPHA
    // patch(FOG_SKY_FALLOFF_ADDR, rainEnable ? m_precip.fogPercent * 0.5f : 0.0f); // cfSkyFogFalloff
    //
    // patch(RAIN_PARAM_A_ADDR, rainEnable ? 1.5f : 0.0f); // Additional rain param A
    // patch(FOG_BLEND_PARAM_ADDR, rainEnable ? 0.5f : 0.0f); // Maybe fog falloff / blend param

    OutputDebugStringA(
        rainEnable ? "[PatchRainSettings] Patching rain ON\n" : "[PatchRainSettings] Patching rain OFF\n");
}

void ForcePrecipitation::enable()
{
    if (m_active)
        return;

    RainConfigController::Load(); // Load from INI

    m_precip.active = true;
    m_precip.rainPercent = RainConfigController::rainIntensity;
    m_precip.fogPercent = RainConfigController::fogIntensity;

    m_active = true;
    PatchRainSettings(RainConfigController::enabled);

    m_drops.resize(250);
    for (auto& d : m_drops)
    {
        d.x = 0.0f;
        d.y = 0.0f;
        d.speed = 200.0f + static_cast<float>(rand() % 400);
    }

    if (!m_registered)
    {
        OutputDebugStringA("Registering DX9 loop\n");
        core::AddDirectX9Loop([this](IDirect3DDevice9* device) { ForcePrecipitation::Update(device); });
        m_registered = true;
    }
}

size_t m_callbackId = static_cast<size_t>(-1);

void ForcePrecipitation::disable()
{
    m_precip.active = false;
    m_active = false;
    m_drops.clear();
    RainConfigController::enabled = false;
    ForcePrecipitation::PatchRainSettings(0);

    if (m_callbackId != static_cast<size_t>(-1))
    {
        core::RemoveDirectX9Loop(m_callbackId);
        m_callbackId = static_cast<size_t>(-1);
        m_registered = false;
    }
}

void ForcePrecipitation::Update(IDirect3DDevice9* device)
{
    if (!m_active || !device)
        return;

    D3DVIEWPORT9 vp{};
    if (FAILED(device->GetViewport(&vp)))
    {
        OutputDebugStringA("Failed to get viewport\n");
        return;
    }

    float width = static_cast<float>(vp.Width);
    float height = static_cast<float>(vp.Height);
    float wind = sinf(timeGetTime() * 0.001f) * 30.0f; // oscillating wind

    // Initialize and update drop positions
    for (auto& d : m_drops)
    {
        if (!d.initialized)
        {
            d.x = static_cast<float>(rand() % static_cast<int>(width));
            d.y = static_cast<float>(rand() % static_cast<int>(height));
            d.speed = 200.0f + m_precip.rainPercent * 600.0f;
            d.length = 10.0f + m_precip.rainPercent * 20.0f;
            d.initialized = true;
        }

        d.y += d.speed * 0.016f;
        d.x += wind * 0.016f; // Apply wind every frame

        if (d.y > height)
        {
            d.x = static_cast<float>(rand() % static_cast<int>(width));
            d.y = -d.length;
        }
    }

    // Render rain
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetTexture(0, nullptr);

    ID3DXLine* line = nullptr;
    if (FAILED(D3DXCreateLine(device, &line)))
    {
        OutputDebugStringA("D3DXCreateLine failed\n");
        return;
    }

    line->SetWidth(1.5f);
    line->SetAntialias(TRUE);
    line->Begin();

    for (auto& d : m_drops)
    {
        D3DXVECTOR2 verts[2] = {{d.x, d.y}, {d.x, d.y + d.length}};
        HRESULT result = line->Draw(verts, 2, D3DCOLOR_ARGB(128, 255, 255, 255));
        if (FAILED(result))
            OutputDebugStringA("line->Draw failed\n");
    }

    line->End();
    line->Release();
}

