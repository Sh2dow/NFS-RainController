#include "ForcePrecipitation.h"
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
    PatchRainSettings(1);

    m_drops.resize(250);

    if (!m_registered)
    {
        OutputDebugStringA("[RainController] Registering DX9 loop\n");

        // FIX: This lambda will now run every frame
        core::AddDirectX9Loop([](IDirect3DDevice9* device)
        {
            ForcePrecipitation::Get()->Update(device);
        });

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
    PatchRainSettings(0);

    // NOTE: We don't remove the loop callback anymore — just stop updating
}

void ForcePrecipitation::Update(IDirect3DDevice9* device)
{
    if (!m_active || !device)
        return;

    // Load raindrop texture if not already loaded
    if (!m_rainTex)
    {
        if (FAILED(D3DXCreateTextureFromFileA(device, "scripts\\raindrop.dds", &m_rainTex)))
        {
            OutputDebugStringA("[RainController] Failed to load raindrop.dds\n");
            return;
        }
    }

    // Get viewport and transforms
    D3DVIEWPORT9 vp{};
    device->GetViewport(&vp);

    D3DXMATRIX matView, matProj, matWorld;
    device->GetTransform(D3DTS_VIEW, &matView);
    device->GetTransform(D3DTS_PROJECTION, &matProj);
    device->GetTransform(D3DTS_WORLD, &matWorld); // usually identity

    // Initialize drops if empty
    if (m_drops.empty())
    {
        m_drops.resize(300);
        for (auto& d : m_drops)
        {
            float x = float(rand() % 200 - 100);
            float y = float(rand() % 50 + 10);
            float z = float(rand() % 200 + 10);
            d.position = D3DXVECTOR3(x, y, z);
            d.velocity = D3DXVECTOR3(0.0f, -200.0f - rand() % 100, 0.0f);
            d.length = 8.0f + rand() % 12;
        }
    }

    // Update drop physics
    float dt = 0.016f; // Approx 60 FPS fixed timestep
    for (auto& d : m_drops)
    {
        d.position += d.velocity * dt;

        if (d.position.y < -10.0f)
        {
            d.position.x = float(rand() % 200 - 100);
            d.position.y = float(rand() % 50 + 10);
            d.position.z = float(rand() % 200 + 10);
        }
    }

    // Setup rendering states
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

    device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1 | D3DFVF_DIFFUSE);
    device->SetTexture(0, m_rainTex);

    // Draw each drop
    for (const auto& d : m_drops)
    {
        D3DXVECTOR3 screenPos;
        if (FAILED(D3DXVec3Project(&screenPos, &d.position, &vp, &matProj, &matView, &matWorld)))
            continue;

        if (screenPos.z > 1.0f)
            continue; // behind camera

        float width = 2.0f;
        float height = d.length;
        int alpha = 160 + (rand() % 64);
        DWORD color = D3DCOLOR_ARGB(alpha, 255, 255, 255);

        Vertex quad[4] = {
            {0, 0, 0.5f, 1.0f, 0.0f, 0.0f, D3DCOLOR_ARGB(255, 255, 0, 0)},
            {800, 0, 0.5f, 1.0f, 1.0f, 0.0f, D3DCOLOR_ARGB(255, 255, 0, 0)},
            {800, 600, 0.5f, 1.0f, 1.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 0, 0)},
            {0, 600, 0.5f, 1.0f, 0.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 0, 0)},
        };

        device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        device->SetRenderState(D3DRS_ZENABLE, FALSE);
        device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1 | D3DFVF_DIFFUSE);
        device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(Vertex));
    }
}
