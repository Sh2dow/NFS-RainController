#include "PrecipitationController.h"
#include "RainConfigController.h"
#include <algorithm>
#include <d3d9.h>
#include <format>
#include "Hooking.Patterns.h"

#ifdef GAME_PS
#include "NFSPS_PreFEngHook.h"
#elif GAME_UC
#include "NFSUC_PreFEngHook.h"
#endif

using namespace ngg::common;

IDirect3DDevice9* m_device = nullptr; // Add to class
D3DXVECTOR3 camPos = D3DXVECTOR3(0, 0, 0);

static auto& g_precipitationConfig = RainConfigController::precipitationConfig;

bool PrecipitationController::IsActive() const
{
    return m_active;
}

// Simplified view node
struct EViewNode
{
    EViewNode* prev; // 0x00
    EViewNode* next; // 0x04
    char padding[0x40]; // Up to 0x40
    D3DXMATRIX viewMatrix; // 0x40
};

// Used by rain renderer or world logic
D3DXVECTOR3 PrecipitationController::GetCameraPositionSafe()
{
    EViewNode* head = *(EViewNode**)EVIEW_LIST_HEAD_PTR;
    // OutputDebugStringA("[RainController] [GetCameraPositionFromVisibleView] 🧭 Starting camera search at head: 0x%p\n",
    //                    head);
    //
    // if (!head)
    // {
    //     OutputDebugStringA("[RainController] ❌ View list is null\n");
    //     return D3DXVECTOR3(0, 0, 0);
    // }

    EViewNode* current = head;

    // do
    // {
    //     __try
    //     {
    //         D3DXMATRIX* mat = (D3DXMATRIX*)((uintptr_t)current + NODE_MATRIX_OFFSET);
    //         D3DXVECTOR3 position(mat->_41, mat->_42, mat->_43);
    //
    //         char buf[160];
    //         sprintf_s(buf, "[RainController] ✅ Using camera matrix at node 0x%p: %.2f, %.2f, %.2f\n",
    //                   current, position.x, position.y, position.z);
    //         OutputDebugStringA(buf);
    //
    //         return position;
    //     }
    //     __except (EXCEPTION_EXECUTE_HANDLER)
    //     {
    //         OutputDebugStringA("[RainController] ❌ Exception reading matrix from view node\n");
    //     }
    //
    //     current = current->next;
    // }
    // while (current && current != head);

    D3DXMATRIX* mat = (D3DXMATRIX*)((uintptr_t)current + NODE_MATRIX_OFFSET);
    D3DXVECTOR3 position(mat->_41, mat->_42, mat->_43);

    // char buf[160];
    // sprintf_s(buf, "[RainController] ✅ Using camera matrix at node 0x%p: %.2f, %.2f, %.2f\n",
    //           current, position.x, position.y, position.z);
    // OutputDebugStringA(buf);

    return position;

    // OutputDebugStringA("[RainController] ❌ No readable view node found\n");
    // return D3DXVECTOR3(0, 0, 0);
}

void PrecipitationController::enable()
{
    if (m_active)
        return;

    RainConfigController::Load();
    m_active = true;

    // Select rain texture format

    chosenFormat = D3DFMT_A8R8G8B8;
#ifdef GAME_UC
    // chosenFormat = GetSupportedRainTextureFormat(m_device);
    // if (chosenFormat == D3DFMT_UNKNOWN)
    // {
    //     OutputDebugStringA("[RainTex] No supported texture format found\n");
    // return false;
    // }
    chosenFormat = D3DFMT_A8B8G8R8;
#else

#endif

    // Resize drop containers if first-time
    if (m_drops2D.empty()) m_drops2D.resize(250);
    if (m_drops3D.empty()) m_drops3D.resize(500);
    m_splatters3D.clear();

    // Get camera position early for consistent reference
    camPos = GetCameraPositionSafe();
    m_cameraY = camPos.y;

    // Prepare rain group settings based on intensity
    ScaleSettingsForIntensity(g_precipitationConfig.rainIntensity);

    // Local lambda to spawn a splatter at a given drop position
    auto spawnSplatter = [&](const D3DXVECTOR3& pos)
    {
        Drop3D splatter;
        // splatter.position = pos;
        // splatter.position.y = m_cameraY - 1.0f;

        float randX = ((rand() % 100) - 50) * 0.05f; // ±2.5
        float randZ = ((rand() % 100) - 50) * 0.5f; // ±25

        splatter.position = camPos + D3DXVECTOR3(
            randX, // lateral
            10.0f, // above camera (adjust as needed)
            randZ // depth
        );

        // splatter.velocity = D3DXVECTOR3(0, 0, 0);
        splatter.velocity = D3DXVECTOR3(0, -3.0f, 0); // or adjustable by config

        splatter.length = 4.0f + (rand() % 3);
        splatter.life = 1.5f + static_cast<float>(rand() % 100) / 100.0f;
        return splatter;
    };

    // Generate initial 3D splatters (fewer than raindrops)
    for (int group = 0; group < 3; ++group)
    {
        const RainGroupSettings& settings = m_rainSettings[group];
        for (int i = 0; i < settings.dropCount / 2; ++i)
        {
            m_splatters3D.push_back(spawnSplatter(RespawnDrop(settings, m_cameraY).position));
        }
    }

    // Animate existing drops and spawn splatters where appropriate
    for (auto& drop : m_drops3D)
    {
        const RainGroupSettings* group = ChooseGroupByY(drop.position.y);
        // drop.position.y -= group->speed;
        drop.position.x += sinf(drop.position.y * 0.05f) * group->windSway;

        if (drop.position.y < m_cameraY - 100.0f)
        {
            m_splatters3D.push_back(spawnSplatter(drop.position));
            drop = RespawnDrop(*group, m_cameraY);
        }
    }

    // Update splatters already in the list (if any)
    for (auto& drop : m_splatters3D)
    {
        const RainGroupSettings* group = ChooseGroupByY(drop.position.y);
        // drop.position.y -= group->speed;
        drop.position.x += sinf(drop.position.y * 0.05f) * group->windSway;

        if (drop.position.y < m_cameraY - 100.0f)
        {
            m_splatters3D.push_back(spawnSplatter(drop.position));
            drop = RespawnDrop(*group, m_cameraY);
        }
    }

    // Fully regenerate the main 3D rain field with updated intensity
    m_drops3D.clear();
    for (int group = 0; group < 3; ++group)
    {
        const RainGroupSettings& settings = m_rainSettings[group];
        for (int i = 0; i < settings.dropCount; ++i)
        {
            m_drops3D.push_back(RespawnDrop(settings, 0.0f));
        }
    }

    // Register DX9 callback loop if not yet added
    if (!m_registered)
    {
        OutputDebugStringA("[PrecipitationController::enable] Registering DX9 loop\n");
        m_callbackId = core::AddDirectX9Loop(std::bind(&PrecipitationController::Update, this, std::placeholders::_1));
        m_registered = true;
    }
}

void PrecipitationController::disable()
{
    g_precipitationConfig = RainConfigController::PrecipitationData();
    // resets all members to their default values

    m_active = false;
    m_drops2D.clear();
    m_drops3D.clear();
    m_splatters3D.clear();

    if (m_callbackId != size_t(-1))
    {
        core::RemoveDirectX9Loop(m_callbackId);
        m_callbackId = -1;
    }
}

PrecipitationController::Drop3D PrecipitationController::RespawnDrop(const RainGroupSettings& settings, float camY)
{
    Drop3D drop;
    drop.length = settings.dropSize;
    drop.velocity = D3DXVECTOR3(settings.windSway, -settings.speed, 0);
    drop.position = D3DXVECTOR3(
        static_cast<float>((rand() % 200) - 100),
        camY + 100.0f,
        static_cast<float>((rand() % 200) + 50)
    );
    drop.life = 1.5f + static_cast<float>(rand() % 100) / 100.0f;
    return drop;
}

const PrecipitationController::RainGroupSettings* PrecipitationController::ChooseGroupByY(float y)
{
    float relY = y - m_cameraY;
    if (relY > 85.0f)
        return &m_rainSettings[0]; // near
    if (relY > 55.0f)
        return &m_rainSettings[1]; // mid

    return &m_rainSettings[2]; // far
}

void PrecipitationController::ScaleSettingsForIntensity(float intensity)
{
    m_rainSettings[0].dropCount = static_cast<int>(g_precipitationConfig.dropCountNear * intensity);
    m_rainSettings[1].dropCount = static_cast<int>(g_precipitationConfig.dropCountMid * intensity);
    m_rainSettings[2].dropCount = static_cast<int>(g_precipitationConfig.dropCountFar * intensity);

    m_rainSettings[0].dropSize = g_precipitationConfig.dropSizeNear;
    m_rainSettings[1].dropSize = g_precipitationConfig.dropSizeMid;
    m_rainSettings[2].dropSize = g_precipitationConfig.dropSizeFar;

    m_rainSettings[0].speed = g_precipitationConfig.speedNear + intensity;
    m_rainSettings[1].speed = g_precipitationConfig.speedMid + intensity;
    m_rainSettings[2].speed = g_precipitationConfig.speedFar + intensity;

    m_rainSettings[0].windSway = g_precipitationConfig.windSwayNear;
    m_rainSettings[1].windSway = g_precipitationConfig.windSwayMid;
    m_rainSettings[2].windSway = g_precipitationConfig.windSwayFar;

    m_rainSettings[0].alphaBlended = g_precipitationConfig.alphaBlend3DRainNear;
    m_rainSettings[1].alphaBlended = g_precipitationConfig.alphaBlend3DRainMid;
    m_rainSettings[2].alphaBlended = g_precipitationConfig.alphaBlend3DRainFar;
}

bool PrecipitationController::IsCreatedRainTexture()
{
    // m_device->CreateTexture(16, 512, 1, 0, chosenFormat,  core::useDXVKFix ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &m_rainTex, nullptr);
    m_device->CreateTexture(16, 512, 1, 0, chosenFormat, D3DPOOL_MANAGED, &m_rainTex, nullptr);

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
                row[x] = (alpha << 24) | 0x80FFFFFF; // semi-transparent white
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

void PrecipitationController::Render3DRainOverlay(const D3DVIEWPORT9& viewport)
{
    if (!m_rainTex)
    {
        if (g_precipitationConfig.use_raindrop_dds)
        {
            if (FAILED(D3DXCreateTextureFromFileA(m_device, "scripts\\raindrop.dds", &m_rainTex)))
            {
                OutputDebugStringA("[Render3DRainOverlay] Failed to load raindrop.dds\n");
                IsCreatedRainTexture();
            }
        }
        else
        {
            IsCreatedRainTexture();
        }

        if (!m_rainTex)
            return;
    }

    D3DXMATRIX matView, matProj, matIdentity;
    m_device->GetTransform(D3DTS_VIEW, &matView);
    float aspect = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
    // D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(60.0f), aspect, 0.1f, 1000.0f);
    D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(60.0f), aspect, 1.0f, 500.0f);

    D3DXMatrixIdentity(&matIdentity);

    m_device->SetTexture(0, m_rainTex);

    // Move drops
    for (auto& drop : m_drops3D)
    {
        const RainGroupSettings* group = ChooseGroupByY(drop.position.y);

        drop.position.y -= group->speed;
        drop.position.x += sinf(timeGetTime() * 0.002f + drop.position.y) * group->windSway;

        if (drop.position.y < m_cameraY - 100.0f)
        {
            // Spawn a splatter near the camera with slight X/Z randomness
            // float randX = ((rand() % 200) - 100) * 0.05f; // range: -5.0 to +5.0
            // float randZ = ((rand() % 200) - 100) * 0.05f; // range: -5.0 to +5.0

            float randX = ((rand() % 100) - 50) * 0.05f; // ±2.5
            float randZ = ((rand() % 100) - 50) * 0.5f; // ±25

            Drop3D splatter;
            splatter.position = camPos + D3DXVECTOR3(randX, -1.0f, randZ); // just below camera
            splatter.length = 4.0f + (rand() % 3);

            // splatter.velocity = D3DXVECTOR3(0, 0, 0);
            splatter.velocity = D3DXVECTOR3(0, -3.0f, 0); // or adjustable by config

            splatter.life = 1.5f + static_cast<float>(rand() % 100) / 100.0f;

            m_splatters3D.push_back(splatter);

            // Respawn the original drop above the camera
            drop = RespawnDrop(*group, m_cameraY);
        }
    }

    auto RenderGroup = [&](float minY, float maxY, bool enableBlend, BYTE alpha)
    {
        m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, enableBlend);

        for (const auto& drop : m_drops3D)
        {
            if (drop.position.y < minY || drop.position.y >= maxY)
                continue;

            D3DXVECTOR3 screen;
            D3DXVec3Project(&screen, &drop.position, &viewport, &matProj, &matView, &matIdentity);

            if (screen.z < 0.0f || screen.z > 1.0f)
                continue;

            float size = drop.length * 4.0f;
            float half = size * 0.5f;

            alpha = static_cast<BYTE>(std::clamp(
                g_precipitationConfig.alpha2DRainMin + g_precipitationConfig.rainIntensity *
                (g_precipitationConfig.alpha2DRainMax - g_precipitationConfig.alpha2DRainMin), 0.0f, 255.0f));

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

    const float nearMin = m_cameraY + g_precipitationConfig.nearMinOffset;
    const float nearMax = m_cameraY + g_precipitationConfig.nearMaxOffset;
    const float midMax = m_cameraY + g_precipitationConfig.midMaxOffset;
    const float farMax = m_cameraY + g_precipitationConfig.farMaxOffset;

    RenderGroup(nearMin, nearMax, g_precipitationConfig.alphaBlend3DRainNear,
                g_precipitationConfig.alphaBlendNearValue);
    RenderGroup(nearMax, midMax, g_precipitationConfig.alphaBlend3DRainMid, g_precipitationConfig.alphaBlendMidValue);
    RenderGroup(midMax, farMax, g_precipitationConfig.alphaBlend3DRainFar, g_precipitationConfig.alphaBlendFarValue);

    m_device->SetTexture(0, nullptr);
}

void PrecipitationController::Render3DSplattersOverlay(const D3DVIEWPORT9& viewport)
{
    // Create splatter texture if not available
    if (!m_splatterTex)
    {
        m_device->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_splatterTex, nullptr);
        D3DLOCKED_RECT rect;
        if (SUCCEEDED(m_splatterTex->LockRect(0, &rect, nullptr, 0)))
        {
            DWORD* pixels = static_cast<DWORD*>(rect.pBits);
            for (int i = 0; i < 16; ++i)
                pixels[i] = 0x80FFFFFF; // semi-transparent white
            m_splatterTex->UnlockRect(0);
            OutputDebugStringA("[Rain] Procedural fallback splatter texture created\n");
        }
    }

    // Setup matrices
    D3DXMATRIX matProj, matView, matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    m_device->GetTransform(D3DTS_VIEW, &matView);
    float aspect = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
    D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(60.0f), aspect, 0.1f, 1000.0f);

    // Update splatter positions and life
    for (auto it = m_splatters3D.begin(); it != m_splatters3D.end();)
    {
        it->position += it->velocity * core::fpsDeltaTime;
        it->life -= core::fpsDeltaTime;

        if (it->life <= 0.0f)
            it = m_splatters3D.erase(it);
        else
            ++it;
    }

    if (m_splatters3D.empty())
        return;

    // Set render state
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    m_device->SetTexture(0, m_splatterTex);

    for (const auto& drop : m_splatters3D)
    {
        D3DXVECTOR3 screen{};
        D3DXVec3Project(&screen, &drop.position, &viewport, &matProj, &matView, &matIdentity);

        if (screen.z < 0.0f || screen.z > 1.0f)
            continue;

        float size = std::clamp(drop.length * 4.0f, 1.0f, 6.0f);
        float half = size * 0.5f;

        // Fade alpha by life (optional: multiply with intensity later)
        BYTE alpha = static_cast<BYTE>(std::clamp(drop.life / 2.0f, 0.0f, 1.0f) * 255.0f);
        DWORD color = D3DCOLOR_ARGB(alpha, 255, 255, 255);

        Vertex quad[4] = {
            {screen.x - half, screen.y - half, screen.z, 1.0f, color, 0.0f, 0.0f},
            {screen.x + half, screen.y - half, screen.z, 1.0f, color, 1.0f, 0.0f},
            {screen.x + half, screen.y + half, screen.z, 1.0f, color, 1.0f, 1.0f},
            {screen.x - half, screen.y + half, screen.z, 1.0f, color, 0.0f, 1.0f}
        };

        m_device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(Vertex));
    }

    m_device->SetTexture(0, nullptr);
}

void PrecipitationController::Render2DRainOverlay(const D3DVIEWPORT9& viewport)
{
    if (g_precipitationConfig.rainIntensity == 0.0f)
        return;

    float width = static_cast<float>(viewport.Width);
    float height = static_cast<float>(viewport.Height);
    float wind = sinf(timeGetTime() * 0.001f) * g_precipitationConfig.windStrength;

    BYTE alpha = static_cast<BYTE>(std::clamp(
        g_precipitationConfig.alpha2DRainMin + g_precipitationConfig.rainIntensity *
        (g_precipitationConfig.alpha2DRainMax - g_precipitationConfig.alpha2DRainMin), 0.0f, 255.0f));

    DWORD color = D3DCOLOR_ARGB(alpha, 255, 255, 255);

    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, g_precipitationConfig.alphaBlend2DRain);
    m_device->SetTexture(0, nullptr);

    for (auto& drop : m_drops2D)
    {
        if (!drop.initialized)
        {
            drop.x = ((float)rand() / RAND_MAX) * width;
            drop.y = ((float)rand() / RAND_MAX) * height;
            drop.speed = g_precipitationConfig.baseSpeed + g_precipitationConfig.rainIntensity * g_precipitationConfig.
                speedScale;
            drop.length = g_precipitationConfig.baseLength + g_precipitationConfig.rainIntensity * g_precipitationConfig
                .lengthScale;
            drop.initialized = true;
        }

        drop.y += drop.speed * 0.016f;
        drop.x += wind * 0.016f;

        if (drop.y > height)
        {
            drop.x = ((float)rand() / RAND_MAX) * width;
            drop.y = -drop.length;
        }

        RHWVertex verts[2] = {
            {drop.x, drop.y, 0.0f, 1.0f, color},
            {drop.x, drop.y + drop.length, 0.0f, 1.0f, color}
        };

        m_device->DrawPrimitiveUP(D3DPT_LINELIST, 1, verts, sizeof(RHWVertex));
    }
}

void PrecipitationController::Update(IDirect3DDevice9* device)
{
    if (!device || !m_active)
        return;

    // Cache actual device
    if (!m_device)
        m_device = device;

    if (g_precipitationConfig.fpsOverride > 0.0f)
        core::fpsDeltaTime = 1.0f / g_precipitationConfig.fpsOverride;
    else
    {
        static DWORD lastTime = timeGetTime();
        DWORD currentTime = timeGetTime();
        core::fpsDeltaTime = (currentTime - lastTime) / 1000.0f; // seconds
        lastTime = currentTime;
    }

    if (core::useDXVKFix)
        device->BeginScene();

    camPos = GetCameraPositionSafe();
    m_cameraY = camPos.y;

    D3DVIEWPORT9 viewport{};
    if (FAILED(m_device->GetViewport(&viewport)))
        return;

    // Setup shared render state
    // m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_device->SetRenderState(D3DRS_ALPHAREF, 32);
    m_device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);

    if (g_precipitationConfig.enable3DRain)
    {
        m_device->SetTexture(0, m_rainTex);
        Render3DRainOverlay(viewport);
    }

    if (g_precipitationConfig.enable3DSplatters)
    {
        m_device->SetTexture(0, m_splatterTex);
        Render3DSplattersOverlay(viewport);
    }

    if (g_precipitationConfig.enable2DRain)
    {
        Render2DRainOverlay(viewport);
    }

    // m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    if (core::useDXVKFix)
        device->EndScene();
}
