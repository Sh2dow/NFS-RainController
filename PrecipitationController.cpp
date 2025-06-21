#include "PrecipitationController.h"
#include "RainConfigController.h"
#include <algorithm>
#include <d3d9.h>

// #ifdef GAME_PS
// #include "NFSPS_PreFEngHook.h"
// #elif GAME_UC
// #include "NFSUC_PreFEngHook.h"
// #endif

using namespace ngg::common;

IDirect3DDevice9* m_device = nullptr; // Add to class

bool PrecipitationController::IsActive() const
{
    return m_active;
}

void PrecipitationController::enable()
{
    if (m_active)
        return;

    RainConfigController::Load();

    m_active = true;

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

    // m_rainSettings[0] = {20, 2.0f, 1.5f, 0.2f, true}; // Near: small, slow, few
    // m_rainSettings[1] = {40, 3.5f, 2.0f, 0.3f, false}; // Mid: moderate
    // m_rainSettings[2] = {60, 1.5f, 2.5f, 0.5f, false}; // Far: tiny, fast, dense

    ScaleSettingsForIntensity(RainConfigController::g_precipitationConfig.rainIntensity);

    m_drops.clear();
    for (int group = 0; group < 3; ++group)
    {
        const RainGroupSettings& settings = m_rainSettings[group];
        for (int i = 0; i < settings.dropCount; ++i)
            m_drops.push_back(RespawnDrop(settings, 0.0f));
    }

    if (!m_registered)
    {
        OutputDebugStringA("[PrecipitationController::enable] Registering DX9 loop\n");
        m_callbackId = core::AddDirectX9Loop(std::bind(&PrecipitationController::Update, this, std::placeholders::_1));
        m_registered = true;
    }
}

void PrecipitationController::disable()
{
    RainConfigController::g_precipitationConfig = RainConfigController::PrecipitationData();
    // resets all members to their default values

    m_active = false;
    m_drops.clear();

    if (m_callbackId != size_t(-1))
    {
        core::RemoveDirectX9Loop(m_callbackId);
        m_callbackId = -1;
    }
}

PrecipitationController::Drop PrecipitationController::RespawnDrop(const RainGroupSettings& settings, float camY)
{
    Drop drop;
    drop.length = settings.dropSize;
    drop.velocity = D3DXVECTOR3(settings.windSway, -settings.speed, 0);
    drop.position = D3DXVECTOR3(
        static_cast<float>((rand() % 200) - 100),
        camY + 100.0f,
        static_cast<float>((rand() % 200) + 50)
    );
    return drop;
}

const PrecipitationController::RainGroupSettings* PrecipitationController::ChooseGroupByY(float y)
{
    float relY = y - m_cameraY;
    if (relY > 85.0f) return &m_rainSettings[0]; // near
    if (relY > 55.0f) return &m_rainSettings[1]; // mid
    return &m_rainSettings[2]; // far
}

void PrecipitationController::ScaleSettingsForIntensity(float intensity)
{
    m_rainSettings[0].dropCount = static_cast<int>(10 + 30 * intensity);
    m_rainSettings[1].dropCount = static_cast<int>(20 + 50 * intensity);
    m_rainSettings[2].dropCount = static_cast<int>(30 + 70 * intensity);

    m_rainSettings[0].speed = 1.5f + intensity * 1.0f;
    m_rainSettings[1].speed = 2.0f + intensity * 1.0f;
    m_rainSettings[2].speed = 2.5f + intensity * 1.0f;

    m_rainSettings[0].dropSize = 2.0f;
    m_rainSettings[1].dropSize = 3.5f;
    m_rainSettings[2].dropSize = 1.5f;

    m_rainSettings[0].windSway = 0.2f;
    m_rainSettings[1].windSway = 0.3f;
    m_rainSettings[2].windSway = 0.5f;
}

bool IsFormatSupported(IDirect3DDevice9* device, D3DFORMAT targetFormat)
{
    D3DDEVICE_CREATION_PARAMETERS params;
    if (FAILED(device->GetCreationParameters(&params)))
        return false;

    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
        return false;

    HRESULT hr = d3d->CheckDeviceFormat(
        params.AdapterOrdinal,
        params.DeviceType,
        D3DFMT_X8R8G8B8, // assumed backbuffer format
        0, // no usage flags (standard texture)
        D3DRTYPE_TEXTURE,
        targetFormat);

    d3d->Release();
    return SUCCEEDED(hr);
}

D3DFORMAT GetSupportedRainTextureFormat(IDirect3DDevice9* device)
{
    D3DDEVICE_CREATION_PARAMETERS params;
    if (FAILED(device->GetCreationParameters(&params)))
        return D3DFMT_UNKNOWN;

    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
        return D3DFMT_UNKNOWN;

    // Adapter format (usually backbuffer format)
    D3DFORMAT adapterFormat = D3DFMT_X8R8G8B8;

    // List of candidate formats in priority order
    D3DFORMAT formatsToCheck[] = {
        D3DFMT_A8R8G8B8, // Most compatible
        D3DFMT_X8R8G8B8, // No alpha fallback
        D3DFMT_R5G6B5, // 16-bit fallback
        D3DFMT_A4R4G4B4 // 16-bit with alpha
    };

    D3DFORMAT supportedFormat = D3DFMT_UNKNOWN;

    for (D3DFORMAT format : formatsToCheck)
    {
        HRESULT hr = d3d->CheckDeviceFormat(
            params.AdapterOrdinal,
            params.DeviceType,
            adapterFormat,
            0, // No special usage
            D3DRTYPE_TEXTURE,
            format);

        if (SUCCEEDED(hr))
        {
            supportedFormat = format;
            break;
        }
    }

    d3d->Release();
    return supportedFormat;
}

bool PrecipitationController::IsCreatedRainTexture(IDirect3DDevice9* device)
{
    D3DFORMAT chosenFormat = D3DFMT_A8R8G8B8;
#ifdef GAME_UC
    // chosenFormat = GetSupportedRainTextureFormat(device);
    // if (chosenFormat == D3DFMT_UNKNOWN)
    // {
    //     OutputDebugStringA("[RainTex] No supported texture format found\n");
    // return false;
    // }
    chosenFormat = D3DFMT_A8B8G8R8;
#else

#endif

    device->CreateTexture(16, 512, 1, 0, chosenFormat, D3DPOOL_MANAGED, &m_rainTex, nullptr);

    D3DLOCKED_RECT rect;
    bool result = SUCCEEDED(m_rainTex->LockRect(0, &rect, nullptr, 0));
    if (result)
    {
        // First pass: thin rain lines (near group with fewer & smaller streaks)
        for (int x = 6; x < 26; x += 16)
        {
            int startY = rand() % (256 - 12);
            for (int y = 0; y < 12; ++y)
            {
                DWORD* row = reinterpret_cast<DWORD*>((BYTE*)rect.pBits + (startY + y) * rect.Pitch);
                BYTE alpha = static_cast<BYTE>(min(255, 192 - y * 6));
                row[x] = (alpha << 24) | 0xFFFFFF;
            }
        }

        // Second pass: thick core streaks (mid group)
        for (int x = 10; x < 22; x += 10)
        {
            int startY = rand() % (256 - 32);
            for (int y = 0; y < 32; ++y)
            {
                DWORD* row = reinterpret_cast<DWORD*>((BYTE*)rect.pBits + (startY + y) * rect.Pitch);
                float normY = static_cast<float>(startY + y) / 256.0f;
                float fade = 1.0f - std::clamp(normY / (2.0f / 3.0f), 0.0f, 1.0f);
                BYTE alpha = static_cast<BYTE>(min(255.0f, (224 - y * 3) * fade * 2.0f));

                if (x > 0) row[x - 1] = (alpha / 2 << 24) | 0xFFFFFF;
                row[x] = (alpha << 24) | 0xFFFFFF;
                if (x + 1 < 32) row[x + 1] = (alpha / 2 << 24) | 0xFFFFFF;
            }
        }

        // Third pass: high-alpha streaks (far group, smaller & faint)
        for (int x = 0; x < 32; x += 6)
        {
            int startY = rand() % (256 - 8);
            for (int y = 0; y < 8; ++y)
            {
                DWORD* row = reinterpret_cast<DWORD*>((BYTE*)rect.pBits + (startY + y) * rect.Pitch);
                float normY = static_cast<float>(startY + y) / 256.0f;
                float fade = 1.0f - std::clamp(normY / 0.5f, 0.0f, 1.0f);
                BYTE alpha = static_cast<BYTE>((160 - y * 2) * fade);

                row[x] = (alpha << 24) | 0xFFFFFF;
                if (x > 0) row[x - 1] = ((alpha / 2) << 24) | 0xFFFFFF;
                if (x + 1 < 32) row[x + 1] = ((alpha / 2) << 24) | 0xFFFFFF;
            }
        }

        m_rainTex->UnlockRect(0);
        OutputDebugStringA("[IsCreatedRainTexture] Vertical rain streaks texture created\n");
    }
    else
        OutputDebugStringA("[IsCreatedRainTexture] Failed Procedural fallback rain texture creation\n");

    return result;
}

void PrecipitationController::Render2DRainOverlay(IDirect3DDevice9* m_device, const D3DVIEWPORT9& viewport)
{
    if (RainConfigController::g_precipitationConfig.rainIntensity == 0.0f)
        return;

    ID3DXLine* line = nullptr;
    if (FAILED(D3DXCreateLine(m_device, &line)))
    {
        OutputDebugStringA("D3DXCreateLine failed\n");
        return;
    }

    line->SetWidth(1.5f);
    line->SetAntialias(TRUE);
    line->Begin();

    float wind = sinf(timeGetTime() * 0.001f) * 30.0f;
    float width = static_cast<float>(viewport.Width);
    float height = static_cast<float>(viewport.Height);
    BYTE alpha = static_cast<BYTE>(std::clamp(
        64.0f + RainConfigController::g_precipitationConfig.rainIntensity * 192.0f, 0.0f, 255.0f));

    for (auto& drop : m_drops)
    {
        // If velocity is zero, initialize it
        if (drop.velocity.y == 0.0f)
        {
            drop.position.x = static_cast<float>(rand() % static_cast<int>(width));
            drop.position.y = static_cast<float>(rand() % static_cast<int>(height));
            drop.velocity.y = 200.0f + RainConfigController::g_precipitationConfig.rainIntensity * 600.0f;
            drop.length = 10.0f + RainConfigController::g_precipitationConfig.rainIntensity * 20.0f;
        }

        drop.position.y += drop.velocity.y * 0.016f;
        drop.position.x += wind * 0.016f;

        if (drop.position.y > height)
        {
            drop.position.x = static_cast<float>(rand() % static_cast<int>(width));
            drop.position.y = -drop.length;
        }

        D3DXVECTOR2 verts[2] = {
            {drop.position.x, drop.position.y},
            {drop.position.x, drop.position.y + drop.length}
        };
        if (FAILED(line->Draw(verts, 2, D3DCOLOR_ARGB(alpha, 255, 255, 255))))
            OutputDebugStringA("line->Draw failed\n");
    }

    line->End();
    line->Release();
}

void PrecipitationController::Render3DRainOverlay(IDirect3DDevice9* m_device, const D3DVIEWPORT9& viewport)
{
    if (!m_rainTex)
    {
        if (RainConfigController::g_precipitationConfig.use_raindrop_dds)
        {
            if (FAILED(D3DXCreateTextureFromFileA(m_device, "scripts\\raindrop.dds", &m_rainTex)))
            {
                OutputDebugStringA("[IsCreatedRainTexture] Failed to load raindrop.dds\n");
                IsCreatedRainTexture(m_device);
            }
        }
        else
        {
            IsCreatedRainTexture(m_device);
        }

        if (!m_rainTex)
            return;
    }

    D3DXMATRIX matProj, matView;
    m_device->GetTransform(D3DTS_VIEW, &matView);

    float aspect = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
    D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(60.0f), aspect, 0.1f, 1000.0f);

    D3DXVECTOR3 camPos(0, 0, 0); // replace with actual camera if possible

    // Move drops
    for (auto& drop : m_drops)
    {
        const RainGroupSettings* group = ChooseGroupByY(drop.position.y); // assign group based on vertical distance

        // Move down by speed and sway left/right
        drop.position.y -= group->speed;
        drop.position.x += sinf(drop.position.y * 0.05f) * group->windSway;

        // Respawn if too far below camera
        if (drop.position.y < camPos.y - 100.0f)
            drop = RespawnDrop(*group, camPos.y); // use group index
    }

    auto RenderGroup = [&](float minY, float maxY, bool enableBlend, BYTE alpha)
    {
        m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, enableBlend);

        for (const auto& drop : m_drops)
        {
            if (drop.position.y < minY || drop.position.y >= maxY)
                continue;

            D3DXVECTOR3 screen;
            D3DXMATRIX identity;
            D3DXMatrixIdentity(&identity);
            D3DXVec3Project(&screen, &drop.position, &viewport, &matProj, &matView, &identity);

            if (screen.z < 0.0f || screen.z > 1.0f)
                continue;

            float size = drop.length * 4.0f;
            float half = size * 0.5f;
            DWORD color = D3DCOLOR_ARGB(alpha, 255, 255, 255);

            Vertex quad[4] = {
                {screen.x - half, screen.y - half, screen.z, 1.0f, color, 0.0f, 0.0f},
                {screen.x + half, screen.y - half, screen.z, 1.0f, color, 1.0f, 0.0f},
                {screen.x + half, screen.y + half, screen.z, 1.0f, color, 1.0f, 1.0f},
                {screen.x - half, screen.y + half, screen.z, 1.0f, color, 0.0f, 1.0f},
            };

            m_device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(Vertex));
        }
    };

    // Group thresholds (relative to cam Y)
    const float nearMin = camPos.y - 9999.0f;
    const float nearMax = camPos.y - 100.0f;
    const float midMax = camPos.y + 45.0f;
    const float farMax = camPos.y + 9999.0f;

    // Render: near = fewer, smaller, blend
    RenderGroup(nearMin, nearMax, true, 180); // Enable alpha blending

    // Render: mid = moderate size/density, opaque
    RenderGroup(nearMax, midMax, false, 220);

    // Render: far = faint and small, opaque
    RenderGroup(midMax, farMax, false, 192);

    m_device->SetTexture(0, nullptr);
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
}

void PrecipitationController::Update(IDirect3DDevice9* device)
{
    if (!device || !m_active)
        return;

    // Cache actual device
    if (!m_device)
        m_device = device;

    if (core::useDXVKFix)
        device->BeginScene();

    D3DVIEWPORT9 viewport{};
    if (FAILED(m_device->GetViewport(&viewport)))
        return;
    
    // Setup shared render state
    m_device->SetRenderState(D3DRS_ZENABLE, FALSE);

    // device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

    m_device->SetRenderState(D3DRS_ALPHAREF, 32);
    m_device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetTexture(0, m_rainTex);
    m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    
    if (RainConfigController::g_precipitationConfig.enable3DRain)
        Render3DRainOverlay(m_device, viewport);

    // if (RainConfigController::g_precipitationConfig.enable2DRain)
    //     Render2DRainOverlay(m_device, viewport);
    
    if (core::useDXVKFix)
        device->EndScene();

    Update2(device);
}

void PrecipitationController::Update2(IDirect3DDevice9* device)
{
    D3DVIEWPORT9 viewport{};
    if (FAILED(m_device->GetViewport(&viewport)))
        return;

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
    for (auto& drop : m_drops)
    {
        // If velocity is zero, initialize it
        if (drop.velocity.y == 0.0f)
        {
            drop.position.x = static_cast<float>(rand() % static_cast<int>(width));
            drop.position.y = static_cast<float>(rand() % static_cast<int>(height));
            drop.velocity.y = 200.0f + RainConfigController::g_precipitationConfig.rainIntensity * 600.0f;
            drop.length = 10.0f + RainConfigController::g_precipitationConfig.rainIntensity * 20.0f;
        }

        drop.position.y += drop.velocity.y * 0.016f;
        drop.position.x += wind * 0.016f;

        if (drop.position.y > height)
        {
            drop.position.x = static_cast<float>(rand() % static_cast<int>(width));
            drop.position.y = -drop.length;
        }

        D3DXVECTOR2 verts[2] = {
            {drop.position.x, drop.position.y},
            {drop.position.x, drop.position.y + drop.length}
        };
        if (FAILED(line->Draw(verts, 2, D3DCOLOR_ARGB(alpha, 255, 255, 255))))
            OutputDebugStringA("line->Draw failed\n");
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
