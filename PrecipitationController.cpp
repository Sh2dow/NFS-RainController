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

D3DXVECTOR3 GetCameraPositionFromVisibleView1()
{
    EViewNode** headPtr = (EViewNode**)EVIEW_LIST_HEAD_PTR;

    if (!headPtr || IsBadReadPtr(headPtr, sizeof(void*)) || *headPtr == nullptr)
    {
        OutputDebugStringA("[GetCameraPositionFromVisibleView] ❌ EVIEW_LIST_HEAD_PTR is null or invalid\n");
        return D3DXVECTOR3(0, 0, 0);
    }

    EViewNode* current = *headPtr;

    char buf[256];
    sprintf_s(buf, "[GetCameraPositionFromVisibleView] 🧭 Starting camera search at head: 0x%08X\n",
              (uintptr_t)current);
    OutputDebugStringA(buf);

    using GetVisibleStateSB_t = int(__thiscall*)(void*, const D3DXVECTOR3*, const D3DXVECTOR3*, D3DXMATRIX*);
    static GetVisibleStateSB_t GetVisibleStateSB = (GetVisibleStateSB_t)GET_VISIBLE_STATE_SB_ADDR;

    if (!GetVisibleStateSB || IsBadCodePtr((FARPROC)GetVisibleStateSB))
    {
        OutputDebugStringA("[GetCameraPositionFromVisibleView] ❌ GetVisibleStateSB function is invalid\n");
        return D3DXVECTOR3(0, 0, 0);
    }

    int nodeCount = 0;
    while (current && current != *headPtr)
    {
        __try
        {
            nodeCount++;

            sprintf_s(buf, "[GetCameraPositionFromVisibleView] 🔍 Node %d at 0x%08X\n", nodeCount, (uintptr_t)current);
            OutputDebugStringA(buf);

            D3DXVECTOR3 dummy1 = {0, 0, 0};
            D3DXVECTOR3 dummy2 = {0, 0, 0};
            D3DXMATRIX dummyMatrix = {};

            int state = GetVisibleStateSB(current, &dummy1, &dummy2, &dummyMatrix);
            sprintf_s(buf, "[GetCameraPositionFromVisibleView] ↪️ VisibleStateSB = %d\n", state);
            OutputDebugStringA(buf);

            if (state != 0) // visible
            {
                D3DXMATRIX* mat = (D3DXMATRIX*)((uintptr_t)current + NODE_MATRIX_OFFSET);
                if (IsBadReadPtr(mat, sizeof(D3DXMATRIX)))
                {
                    OutputDebugStringA("[GetCameraPositionFromVisibleView] ⚠️ View matrix pointer invalid\n");
                    break;
                }

                D3DXVECTOR3 position(mat->_41, mat->_42, mat->_43);
                sprintf_s(buf, "[GetCameraPositionFromVisibleView] ✅ Active camera found: %.2f, %.2f, %.2f\n",
                          position.x, position.y, position.z);
                OutputDebugStringA(buf);
                return position;
            }

            current = current->next;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            OutputDebugStringA("[GetCameraPositionFromVisibleView] ❌ Exception in view list traversal\n");
            break;
        }
    }

    OutputDebugStringA("[GetCameraPositionFromVisibleView] ❌ No visible camera found\n");
    return D3DXVECTOR3(0, 0, 0);
}

// Used by rain renderer or world logic
D3DXVECTOR3 PrecipitationController::GetCameraPositionSafe()
{
    EViewNode* head = *(EViewNode**)EVIEW_LIST_HEAD_PTR;
    OutputDebugStringA("[RainController] [GetCameraPositionFromVisibleView] 🧭 Starting camera search at head: 0x%p\n",
                       head);

    if (!head)
    {
        OutputDebugStringA("[RainController] ❌ View list is null\n");
        return D3DXVECTOR3(0, 0, 0);
    }

    EViewNode* current = head;

    do
    {
        __try
        {
            D3DXMATRIX* mat = (D3DXMATRIX*)((uintptr_t)current + NODE_MATRIX_OFFSET);
            D3DXVECTOR3 position(mat->_41, mat->_42, mat->_43);

            char buf[160];
            sprintf_s(buf, "[RainController] ✅ Using camera matrix at node 0x%p: %.2f, %.2f, %.2f\n",
                      current, position.x, position.y, position.z);
            OutputDebugStringA(buf);

            return position;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            OutputDebugStringA("[RainController] ❌ Exception reading matrix from view node\n");
        }

        current = current->next;
    }
    while (current && current != head);

    OutputDebugStringA("[RainController] ❌ No readable view node found\n");
    return D3DXVECTOR3(0, 0, 0);
}

void PrecipitationController::enable()
{
    if (m_active)
        return;

    RainConfigController::Load();

    m_active = true;

    if (m_drops2D.empty())
    {
        m_drops2D.resize(250); // or 200+ for denser rain
    }
    if (m_drops3D.empty())
    {
        m_drops3D.resize(200); // or 200+ for denser rain
    }

    // Default camera-relative origin if actual camera can't be read
    camPos = GetCameraPositionSafe();

    for (auto& drop : m_drops3D)
    {
        drop.position = camPos + D3DXVECTOR3(
            static_cast<float>((rand() % 200) - 100),
            static_cast<float>((rand() % 100)),
            static_cast<float>((rand() % 200) + 50)
        );
        drop.velocity = D3DXVECTOR3(0.0f, -1.0f, 0.0f);
        drop.length = 8.0f + (rand() % 8);
    }

    ScaleSettingsForIntensity(g_precipitationConfig.rainIntensity);

    m_drops3D.clear();
    for (int group = 0; group < 3; ++group)
    {
        const RainGroupSettings& settings = m_rainSettings[group];
        for (int i = 0; i < settings.dropCount; ++i)
            m_drops3D.push_back(RespawnDrop(settings, 0.0f));
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
    g_precipitationConfig = RainConfigController::PrecipitationData();
    // resets all members to their default values

    m_active = false;
    m_drops2D.clear();
    m_drops3D.clear();

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

    m_rainSettings[0].alphaBlended = g_precipitationConfig.alphaBlendNear;
    m_rainSettings[1].alphaBlended = g_precipitationConfig.alphaBlendMid;
    m_rainSettings[2].alphaBlended = g_precipitationConfig.alphaBlendFar;
}

bool PrecipitationController::IsCreatedRainTexture()
{
    D3DFORMAT chosenFormat = D3DFMT_A8R8G8B8;
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

    m_device->CreateTexture(16, 512, 1, 0, chosenFormat, D3DPOOL_DEFAULT, &m_rainTex, nullptr);

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

    D3DXMATRIX matProj, matView;
    m_device->GetTransform(D3DTS_VIEW, &matView);

    float aspect = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
    D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(60.0f), aspect, 0.1f, 1000.0f);

    m_cameraY = camPos.y;

    // Move drops
    for (auto& drop : m_drops3D)
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

        for (const auto& drop : m_drops3D)
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
    const float nearMin = camPos.y + g_precipitationConfig.nearMinOffset;
    const float nearMax = camPos.y + g_precipitationConfig.nearMaxOffset;
    const float midMax = camPos.y + g_precipitationConfig.midMaxOffset;
    const float farMax = camPos.y + g_precipitationConfig.farMaxOffset;

    // Render: near = faint and small, opaque
    RenderGroup(nearMin, nearMax, g_precipitationConfig.alphaBlendNear, g_precipitationConfig.alphaBlendNearValue);

    // Render: mid = moderate size/density, opaque
    RenderGroup(nearMax, midMax, g_precipitationConfig.alphaBlendMid, g_precipitationConfig.alphaBlendMidValue);

    // Render: far = fewer, smaller, blend
    RenderGroup(midMax, farMax, g_precipitationConfig.alphaBlendFar, g_precipitationConfig.alphaBlendFarValue);
    // Enable alpha blending

    m_device->SetTexture(0, nullptr);
}

void PrecipitationController::Render3DSplattersOverlay(const D3DVIEWPORT9& viewport)
{
    // Initialize rain texture if needed
    if (!m_rainTex)
    {
        m_device->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_rainTex, nullptr);
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

    D3DXMATRIX matProj, matView, matWorld;
    D3DXMatrixIdentity(&matWorld);
    m_device->GetTransform(D3DTS_VIEW, &matView);

    // Fallback perspective matrix (60° FOV, 16:9 aspect, near/far = 0.1, 1000)
    float aspect = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
    D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(60.0f), aspect, 0.1f, 1000.0f);

    // Estimated camera position (replace this with actual camera lookup if possible)

    // Drop logic
    for (auto& drop : m_drops3D)
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

    m_device->SetTexture(0, m_rainTex);

    for (const auto& drop : m_drops3D)
    {
        D3DXVECTOR3 screen{};
        D3DXMATRIX identity;
        D3DXMatrixIdentity(&identity);
        D3DXVec3Project(&screen, &drop.position, &viewport, &matProj, &matView, &identity);

        if (screen.z < 0.0f || screen.z > 1.0f)
            continue;

        float size = drop.length * 4.0f;
        float half = size * 0.5f;

        DWORD color = D3DCOLOR_ARGB(128, 255, 255, 255);

        Vertex quad[4] = {
            {screen.x - half, screen.y - half, screen.z, 1.0f, color, 0.0f, 0.0f},
            {screen.x + half, screen.y - half, screen.z, 1.0f, color, 1.0f, 0.0f},
            {screen.x + half, screen.y + half, screen.z, 1.0f, color, 1.0f, 1.0f},
            {screen.x - half, screen.y + half, screen.z, 1.0f, color, 0.0f, 1.0f},
        };

        m_device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(Vertex));
    }

    // Clean up render state
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
        g_precipitationConfig.alphaMin + g_precipitationConfig.rainIntensity *
        (g_precipitationConfig.alphaMax - g_precipitationConfig.alphaMin), 0.0f, 255.0f));

    DWORD color = D3DCOLOR_ARGB(alpha, 255, 255, 255);

    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
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

    if (core::useDXVKFix)
        device->BeginScene();

    D3DVIEWPORT9 viewport{};
    if (FAILED(m_device->GetViewport(&viewport)))
        return;

    // Setup shared render state
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_device->SetRenderState(D3DRS_ALPHAREF, 32);
    m_device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetTexture(0, m_rainTex);
    m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);

    if (g_precipitationConfig.enable3DRain)
        Render3DRainOverlay(viewport);

    if (g_precipitationConfig.enable3DSplatters)
        Render3DSplattersOverlay(viewport);

    if (g_precipitationConfig.enable2DRain)
        Render2DRainOverlay(viewport);

    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    if (core::useDXVKFix)
        device->EndScene();
}
