#include "ForcePrecipitation.h"
#include <algorithm>
#include "RainConfigController.h"

// #ifdef GAME_PS
// #include "NFSPS_PreFEngHook.h"
// #elif GAME_UC
// #include "NFSUC_PreFEngHook.h"
// #endif

using namespace ngg::common;

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

    m_rainSettings[0] = {20, 2.0f, 1.5f, 0.2f, true}; // Near: small, slow, few
    m_rainSettings[1] = {40, 3.5f, 2.0f, 0.3f, false}; // Mid: moderate
    m_rainSettings[2] = {60, 1.5f, 2.5f, 0.5f, false}; // Far: tiny, fast, dense

    m_drops.clear();
    for (int group = 0; group < 3; ++group)
    {
        const RainGroupSettings& settings = m_rainSettings[group];
        for (int i = 0; i < settings.dropCount; ++i)
            m_drops.push_back(RespawnDrop(settings, 0.0f));
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

    if (m_callbackId != size_t(-1))
    {
        core::RemoveDirectX9Loop(m_callbackId);
        m_callbackId = -1;
    }
}

ForcePrecipitation::Drop ForcePrecipitation::RespawnDrop(const RainGroupSettings& settings, float camY)
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

const ForcePrecipitation::RainGroupSettings* ForcePrecipitation::ChooseGroupByY(float y)
{
    float relY = y - m_cameraY;
    if (relY > 85.0f) return &m_rainSettings[0]; // near
    if (relY > 55.0f) return &m_rainSettings[1]; // mid
    return &m_rainSettings[2]; // far
}

void ForcePrecipitation::ScaleSettingsForIntensity(float intensity)
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

bool ForcePrecipitation::IsCreatedRainTexture(IDirect3DDevice9* device)
{
    device->CreateTexture(16, 512, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_rainTex, nullptr);
    // device->CreateTexture(64, 1024, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_rainTex, nullptr);
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

void ForcePrecipitation::Render2DRainOverlay(IDirect3DDevice9* device, const D3DVIEWPORT9& viewport)
{
    if (m_precip.rainPercent <= 0.05f)
        return;

    ID3DXLine* line = nullptr;
    if (FAILED(D3DXCreateLine(device, &line)))
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
    BYTE alpha = static_cast<BYTE>(std::clamp(64.0f + m_precip.rainPercent * 192.0f, 0.0f, 255.0f));

    for (auto& drop : m_drops)
    {
        // If velocity is zero, initialize it
        if (drop.velocity.y == 0.0f)
        {
            drop.position.x = static_cast<float>(rand() % static_cast<int>(width));
            drop.position.y = static_cast<float>(rand() % static_cast<int>(height));
            drop.velocity.y = 200.0f + m_precip.rainPercent * 600.0f;
            drop.length = 10.0f + m_precip.rainPercent * 20.0f;
        }

        drop.position.y += drop.velocity.y * 0.016f;
        drop.position.x += wind * 0.016f;

        if (drop.position.y > height)
        {
            drop.position.x = static_cast<float>(rand() % static_cast<int>(width));
            drop.position.y = -drop.length;
        }

        D3DXVECTOR2 verts[2] = {
            { drop.position.x, drop.position.y },
            { drop.position.x, drop.position.y + drop.length }
        };
        if (FAILED(line->Draw(verts, 2, D3DCOLOR_ARGB(alpha, 255, 255, 255))))
            OutputDebugStringA("line->Draw failed\n");
    }

    line->End();
    line->Release();
}

void ForcePrecipitation::Update(IDirect3DDevice9* device)
{
    if (!m_active || !m_precip.active || !device)
        return;

    device->BeginScene(); // DXVK workaround

    if (!m_rainTex && !IsCreatedRainTexture(device))
        return;

    D3DVIEWPORT9 viewport{};
    if (FAILED(device->GetViewport(&viewport)))
        return;

    Render2DRainOverlay(device, viewport);

    D3DXMATRIX matProj, matView;
    device->GetTransform(D3DTS_VIEW, &matView);

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

    // Setup shared render state
    device->SetRenderState(D3DRS_ZENABLE, FALSE);

    // device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

    device->SetRenderState(D3DRS_ALPHAREF, 32);
    device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetTexture(0, m_rainTex);
    device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);

    auto RenderGroup = [&](float minY, float maxY, bool enableBlend, BYTE alpha)
    {
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, enableBlend);

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

            device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(Vertex));
        }
    };

    // Group thresholds (relative to cam Y)
    const float nearMin = camPos.y - 9999.0f;
    const float nearMax = camPos.y - 100.0f;
    const float midMax  = camPos.y + 45.0f;
    const float farMax  = camPos.y + 9999.0f;

    // Render: near = fewer, smaller, blend
    RenderGroup(nearMin, nearMax, true, 180);  // Enable alpha blending

    // Render: mid = moderate size/density, opaque
    RenderGroup(nearMax, midMax, false, 220);

    // Render: far = faint and small, opaque
    RenderGroup(midMax, farMax, false, 192);

    device->SetTexture(0, nullptr);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->EndScene(); // DXVK workaround
}
