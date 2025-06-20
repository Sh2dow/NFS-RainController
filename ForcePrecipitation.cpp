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

bool ForcePrecipitation::IsActive() const
{
    return m_active && m_precip.active;
}

void ForcePrecipitation::enable()
{
    if (m_active)
        return;

    RainConfigController::Load();

    m_precip.active = true;
    m_precip.rainPercent = RainConfigController::rainIntensity;
    m_precip.fogPercent = RainConfigController::fogIntensity;

    m_active = true;
    PatchRainSettings(1);

    m_drops.resize(250);

    // Default camera-relative origin if actual camera can't be read
    D3DXVECTOR3 camPos(0, 0, 0);

    for (auto& drop : m_drops)
    {
        drop.position = camPos + D3DXVECTOR3(
            static_cast<float>((rand() % 200) - 100),
            static_cast<float>((rand() % 100)),
            static_cast<float>((rand() % 200) + 50)
        );
        drop.velocity = D3DXVECTOR3(0.0f, -1.0f, 0.0f);
        drop.length = 8.0f + (rand() % 8);
    }

    if (!m_registered)
    {
        OutputDebugStringA("[RainController] Registering DX9 loop\n");
        m_callbackId = core::AddDirectX9Loop(std::bind(&ForcePrecipitation::Update, this, std::placeholders::_1));
        m_registered = true;
    }
}

void ForcePrecipitation::disable()
{
    m_precip.active = false;
    m_active = false;
    m_drops.clear();
    RainConfigController::enabled = false;
    PatchRainSettings(0);

    if (m_callbackId != size_t(-1))
    {
        core::RemoveDirectX9Loop(m_callbackId);
        m_callbackId = -1;
    }
}

void ForcePrecipitation::Update(IDirect3DDevice9* device)
{
    if (!m_active || !m_precip.active || !device)
        return;

    device->BeginScene(); // 👈 Add this

    // Initialize rain texture if needed
    if (!m_rainTex)
    {
        device->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_rainTex, nullptr);
        D3DLOCKED_RECT rect;
        if (SUCCEEDED(m_rainTex->LockRect(0, &rect, nullptr, 0)))
        {
            DWORD* pixels = static_cast<DWORD*>(rect.pBits);
            for (int i = 0; i < 16; ++i)
                pixels[i] = 0x80FFFFFF; // semi-transparent white
            m_rainTex->UnlockRect(0);
            OutputDebugStringA("[Rain] Procedural fallback rain texture created\n");
        }
    }

    D3DVIEWPORT9 viewport{};
    if (FAILED(device->GetViewport(&viewport)))
        return;

    D3DXMATRIX matProj, matView, matWorld;
    D3DXMatrixIdentity(&matWorld);
    device->GetTransform(D3DTS_VIEW, &matView);

    // Fallback perspective matrix (60° FOV, 16:9 aspect, near/far = 0.1, 1000)
    float aspect = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
    D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(60.0f), aspect, 0.1f, 1000.0f);

    char buf[256];
    sprintf(buf, "Proj: %f %f %f %f\n", matProj._11, matProj._22, matProj._33, matProj._43);
    OutputDebugStringA(buf);

    // Estimated camera position (replace this with actual camera lookup if possible)
    D3DXVECTOR3 camPos(0, 0, 0); // TODO: Replace with real camera pos if found

    // Drop logic
    for (auto& drop : m_drops)
    {
        drop.position += drop.velocity;

        if (drop.position.y < camPos.y - 100.0f)
        {
            // Reset above camera
            drop.position = camPos + D3DXVECTOR3(
                static_cast<float>((rand() % 200) - 100),
                100.0f,
                static_cast<float>((rand() % 200) + 50)
            );
        }
    }

    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetTexture(0, m_rainTex);

    struct Vertex
    {
        float x, y, z, rhw;
        DWORD color;
        float u, v;
    };

    for (const auto& drop : m_drops)
    {
        D3DXVECTOR3 screen{};
        D3DXMATRIX identity;
        D3DXMatrixIdentity(&identity);
        D3DXVec3Project(&screen, &drop.position, &viewport, &matProj, &matView, &identity);

        if (screen.z < 0.0f || screen.z > 1.0f)
            continue;

        float size = drop.length;
        float half = size * 0.5f;

        Vertex quad[4] = {
            {screen.x - half, screen.y - half, screen.z, 1.0f, D3DCOLOR_ARGB(128, 255, 255, 255), 0.0f, 0.0f},
            {screen.x + half, screen.y - half, screen.z, 1.0f, D3DCOLOR_ARGB(128, 255, 255, 255), 1.0f, 0.0f},
            {screen.x + half, screen.y + half, screen.z, 1.0f, D3DCOLOR_ARGB(128, 255, 255, 255), 1.0f, 1.0f},
            {screen.x - half, screen.y + half, screen.z, 1.0f, D3DCOLOR_ARGB(128, 255, 255, 255), 0.0f, 1.0f},
        };

        device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
        device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(Vertex));
    }

    // Clean up render state
    device->SetTexture(0, nullptr);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    device->EndScene(); // 👈 And this
}
