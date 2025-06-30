#include "stdafx.h"
#include "PrecipitationController.h"
#include "RainConfigController.h"
#include <algorithm>
#include <d3d9.h>
#include "Hooking.Patterns.h"
#include "PerlinNoise.h"

#ifdef GAME_PS
#include "NFSPS_PreFEngHook.h"
#elif GAME_UC
#include "NFSUC_PreFEngHook.h"
#endif

using namespace ngg::common;

static auto& g_precipitationConfig = RainConfigController::precipitationConfig;

bool PrecipitationController::IsActive() const
{
    return m_active;
}

void PrecipitationController::DebugEVIEWListPtr()
{
#ifdef GAME_PS
    auto match = hook::pattern("A1 20 BE A8 00 50 E8 ?? ??");
#elif GAME_UC
    auto match = hook::pattern("D9 C9 D9 5C 24 1C D9 44 24 1C DE D9 DF E0 F6 C4 41 75");    // unique fld/fstp/fcompp/fnstsw/test/loop combo
#endif

    if (match.empty())
    {
        OutputDebugStringA("[DebugEVIEWListPtr] ❌ Pattern not found.\n");
        return;
    }

    // char logBuf[128];
    // sprintf_s(logBuf, "[DebugEVIEWListPtr] Matches found: %zu\n", match.size());
    // OutputDebugStringA(logBuf);

#ifdef GAME_PS
    // for (int i = 0; i < match.size(); ++i)
    // {
    //     uintptr_t addr = reinterpret_cast<uintptr_t>(match.get(i).get<void>(0));
    //     char buffer[128];
    //     sprintf(buffer, "[DebugEVIEWListPtr] Match %d at 0x%p\n", i, (void*)addr);
    //     OutputDebugStringA(buffer);
    // }

    uintptr_t ptrAddr = *match.get(0).get<uintptr_t>(1); // +1 skips A1 opcode
    g_EVIEW_LIST_PTR = reinterpret_cast<EViewNode**>(ptrAddr);
#elif GAME_UC
    uintptr_t addr = reinterpret_cast<uintptr_t>(match.get_first());
    uintptr_t targetAddr = *reinterpret_cast<uintptr_t*>(addr + 1);
    g_EVIEW_LIST_PTR = reinterpret_cast<EViewNode**>(targetAddr);
#endif

    char buf[256];
    sprintf_s(buf, "[DebugEVIEWListPtr] ✅ g_EVIEW_LIST_PTR = 0x%08X -> 0x%08X\n",
              (uintptr_t)g_EVIEW_LIST_PTR, (g_EVIEW_LIST_PTR ? (uintptr_t)*g_EVIEW_LIST_PTR : 0));
    OutputDebugStringA(buf);
}

bool isLikelyValidPtr(void* p)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(p, &mbi, sizeof(mbi)))
    {
        DWORD protect = mbi.Protect;

        // Only treat these as safe to read
        bool readable = (protect & PAGE_READONLY) ||
            (protect & PAGE_READWRITE) ||
            (protect & PAGE_EXECUTE_READ) ||
            (protect & PAGE_EXECUTE_READWRITE);

        // char buf[256];
        // sprintf_s(buf, "[isLikelyValidPtr] ✅ EVIEW_PTR state = 0x%X, protect = 0x%X, base = 0x%p\n",
        //           mbi.State, mbi.Protect, mbi.BaseAddress);
        // OutputDebugStringA(buf);

        if (mbi.State != MEM_COMMIT || !readable)
        {
            OutputDebugStringA("[isLikelyValidPtr] 🚫 EVIEW ptr is NOT readable.\n");
            return false;
        }

        return true;
    }

    OutputDebugStringA("[isLikelyValidPtr] ❌ VirtualQuery failed — invalid pointer.\n");
    return false;
}

// Used by rain renderer or world logic
D3DXVECTOR3 PrecipitationController::GetCameraPositionSafe()
{
    if (g_EVIEW_LIST_PTR == nullptr)
    {
        DebugEVIEWListPtr();
    }

    auto head = *(EViewNode**)g_EVIEW_LIST_PTR;

    if (!head
        // || !isLikelyValidPtr(head)
    )
    {
        OutputDebugStringA("[GetCameraPositionSafe] 🚫 EVIEW ptr is NOT valid memory.\n");
        return D3DXVECTOR3(0, 0, 0);
    }

    // Also optionally validate the matrix if needed
    auto mat = (D3DXMATRIX*)((uintptr_t)head + NODE_MATRIX_OFFSET);
    // if (!isLikelyValidPtr(mat))
    // {
    //     OutputDebugStringA("[GetCameraPositionSafe] 🚫 Matrix is NOT readable.\n");
    //     return D3DXVECTOR3(0, 0, 0);
    // }

    return D3DXVECTOR3(mat->_41, mat->_42, mat->_43); // camera position
}

bool PrecipitationController::IsCameraCovered(const D3DXVECTOR3& camPos)
{
    // You can add more zones or load from config later
    if ((camPos.x > g_precipitationConfig.occlusionZone_XMin && camPos.x < g_precipitationConfig.occlusionZone_XMax) &&
        (camPos.z > g_precipitationConfig.occlusionZone_ZMin && camPos.z < g_precipitationConfig.occlusionZone_ZMax))
    {
        return true;
    }

    return false;
}

void PrecipitationController::enable()
{
    if (m_active)
        return;

    RainConfigController::Load();
    m_active = true;

    if (g_precipitationConfig.fpsOverride > 0.0f)
        core::fpsDeltaTime = 1.0f / g_precipitationConfig.fpsOverride;
    else
        m_lastTime = timeGetTime(); // where m_lastTime is a DWORD member

    // Select rain texture format
    chosenFormat = D3DFMT_A8R8G8B8;
#ifdef GAME_UC
    // chosenFormat = GetSupportedRainTextureFormat(m_device);
    // if (chosenFormat == D3DFMT_UNKNOWN)
    // {
    //     OutputDebugStringA("[RainTex] No supported texture format found\n");
    // return false;
    // }
    // chosenFormat = D3DFMT_A8B8G8R8;
#else

#endif

    // Resize drop containers if first-time

    if (m_drops2D.empty()) m_drops2D.resize(g_precipitationConfig.drop2DCount);
    // m_drops2D.clear(); // important

    int drops3dSize = g_precipitationConfig.dropCountNear + g_precipitationConfig.dropCountMid + g_precipitationConfig.
        dropCountFar;
    if (m_drops3D.empty()) m_drops3D.resize(drops3dSize);
    m_drops3D.clear(); // important

    if (m_splatters3D.empty()) m_splatters3D.resize(drops3dSize / 3);
    m_splatters3D.clear(); // important

    // Get camera position early for consistent reference
    static bool printedCameraReady = false;
    camPos = GetCameraPositionSafe();
    if (camPos != D3DXVECTOR3(0, 0, 0))
    {
        if (!printedCameraReady)
        {
            OutputDebugStringA("[PrecipitationController::enable] 🎥 Camera is now valid.\n");
            printedCameraReady = true;
        }
        m_cameraY = camPos.y;
    }

    // camPos = GetCameraPositionSafe();
    // m_cameraY = camPos.y;

    // Prepare rain group settings based on intensity
    ScaleSettingsForIntensity(g_precipitationConfig.rainIntensity);

    // Local lambda to spawn a splatter at a given drop position
    auto spawnSplatter = [&](const D3DXVECTOR3& pos)
    {
        Drop3D splatter;
        splatter.position = pos;
        splatter.position.y = m_cameraY - 1.0f;

        // splatter.velocity = D3DXVECTOR3(0, 0, 0);
        splatter.velocity = D3DXVECTOR3(0, -3.0f, 0); // or adjustable by config

        splatter.length = 4.0f + (rand() % 3);
        splatter.life = 3.5f + static_cast<float>(rand() % 100) / 100.0f;
        splatter.angle = static_cast<float>((rand() % 360)) * (D3DX_PI / 180.0f);
        return splatter;
    };

    // Preallocate the drops with separate logic per group
    // Generate initial 3D splatters (fewer than raindrops)
    for (int group = 0; group < 3; ++group)
    {
        const RainGroupSettings& settings = m_rainSettings[group];
        for (int i = 0; i < settings.dropCount; ++i)
            m_drops3D.push_back(RespawnDrop(settings, m_cameraY));

        for (int i = 0; i < settings.dropCount / 2; ++i)
            m_splatters3D.push_back(spawnSplatter(RespawnDrop(settings, m_cameraY).position));
    }

    // Register DX9 callback loop if not yet added
    if (!m_registered)
    {
        OutputDebugStringA("[PrecipitationController::enable] Registering DX9 loop\n");
        m_callbackId = core::AddDirectX9Loop([this](IDirect3DDevice9*)
        {
            this->Update();
        });
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
        static_cast<float>((rand() % 400) - 200), // wider spread
        camY + 25.0f,
        static_cast<float>((rand() % 1000) + 100) // deeper depth
    );
    drop.life = 3.5f + static_cast<float>(rand() % 100) / 100.0f;
    drop.angle = static_cast<float>((rand() % 360)) * (D3DX_PI / 180.0f);
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
    float intensityCurve = powf(intensity, 1.2f);
    m_rainSettings[0].dropCount = static_cast<int>(g_precipitationConfig.dropCountNear * intensity);
    m_rainSettings[1].dropCount = static_cast<int>(g_precipitationConfig.dropCountMid * intensity);
    m_rainSettings[2].dropCount = static_cast<int>(g_precipitationConfig.dropCountFar * intensity);

    m_rainSettings[0].dropSize = g_precipitationConfig.dropSizeNear;
    m_rainSettings[1].dropSize = g_precipitationConfig.dropSizeMid;
    m_rainSettings[2].dropSize = g_precipitationConfig.dropSizeFar;

    m_rainSettings[0].speed = g_precipitationConfig.speedNear + intensityCurve;
    m_rainSettings[1].speed = g_precipitationConfig.speedMid + intensityCurve;
    m_rainSettings[2].speed = g_precipitationConfig.speedFar + intensityCurve;

    m_rainSettings[0].windSway = g_precipitationConfig.windSwayNear;
    m_rainSettings[1].windSway = g_precipitationConfig.windSwayMid;
    m_rainSettings[2].windSway = g_precipitationConfig.windSwayFar;

    m_rainSettings[0].alphaBlended = g_precipitationConfig.alphaBlend3DRainNear;
    m_rainSettings[1].alphaBlended = g_precipitationConfig.alphaBlend3DRainMid;
    m_rainSettings[2].alphaBlended = g_precipitationConfig.alphaBlend3DRainFar;
}

bool PrecipitationController::IsCreatedRainTexture()
{
    m_device->CreateTexture(16, 512, 1, 0, chosenFormat, core::useDXVKFix ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED,
                            &m_rainTex, nullptr);
    // m_device->CreateTexture(16, 512, 1, 0, chosenFormat, D3DPOOL_MANAGED, &m_rainTex, nullptr);

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
            if (FAILED(D3DXCreateTextureFromFileA(m_device, g_precipitationConfig.raindropTexturePath.c_str(), &m_rainTex)))
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
    D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(60.0f), aspect, 1.0f, 500.0f);
    D3DXMatrixIdentity(&matIdentity);

    m_device->SetTexture(0, m_rainTex);

    // Move drops
    for (auto& drop : m_drops3D)
    {
        const RainGroupSettings* group = ChooseGroupByY(drop.position.y);
        drop.position.y -= group->speed;

        float wind = (noise.noise3D(drop.position.y * 0.05f, core::CurrentTime * 0.0005f, 0.0f) - 0.5f) * 2.0f * group->
            windSway;
        drop.position.x += wind;

        if (drop.position.y < m_cameraY - 10.0f)
        {
            Drop3D splatter;

            // Compute correct forward vector from view matrix
            D3DXMATRIX invView;
            D3DXMatrixInverse(&invView, nullptr, &matView);  // Invert the view matrix
            D3DXVECTOR3 camForward(invView._31, invView._32, invView._33);  // World-space forward

            D3DXVECTOR3 forwardXZ(camForward.x, 0.0f, camForward.z);
            if (D3DXVec3LengthSq(&forwardXZ) < 0.0001f)
            {
                forwardXZ = D3DXVECTOR3(0.0f, 0.0f, 1.0f); // fallback if facing directly up/down
            }
            else
            {
                D3DXVec3Normalize(&forwardXZ, &forwardXZ);
            }
            
            float randX = ((rand() % 100) - 50) * 0.05f;
            float randZ = ((rand() % 100) - 50) * 0.5f;
            D3DXVECTOR3 offset = forwardXZ * 6.0f + D3DXVECTOR3(randX, 0.0f, randZ);

            D3DXVECTOR3 splatterPos = camPos + offset;
            splatterPos.y = m_cameraY - 1.0f;

            splatter.position = splatterPos;
            splatter.velocity = D3DXVECTOR3(0, -3.0f, 0);
            splatter.length = 4.0f + (rand() % 3);
            splatter.life = 3.5f + static_cast<float>(rand() % 100) / 100.0f;
            splatter.angle = atan2f(forwardXZ.x, forwardXZ.z);

            // char dbg[128];
            // sprintf_s(dbg, "[Splatter] forwardXZ: %.2f %.2f %.2f\n", forwardXZ.x, forwardXZ.y, forwardXZ.z);
            // OutputDebugStringA(dbg);

            m_splatters3D.push_back(splatter);
            drop = RespawnDrop(*group, m_cameraY);
        }
    }

    auto RenderGroup = [&](float minY, float maxY, bool enableBlend, int alpha)
    {
        m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, enableBlend);

        for (const auto& drop : m_drops3D)
        {
            if (drop.position.y < minY || drop.position.y >= maxY)
                continue;

            D3DXVECTOR3 screen;
            D3DXVec3Project(&screen, &drop.position, &viewport, &matProj, &matView, &matIdentity);

            if (screen.z < -10.0f || screen.z > 100.0f)
                continue;

            float size = drop.length * 4.0f;
            float half = size * 0.5f;
            DWORD color = D3DCOLOR_ARGB(alpha, 255, 255, 255);

            float cosA = cosf(drop.angle);
            float sinA = sinf(drop.angle);

            D3DXVECTOR2 center(screen.x, screen.y);
            D3DXVECTOR2 offsets[4] = {{-half, -half}, {half, -half}, {half, half}, {-half, half}};
            Vertex quad[4];

            for (int i = 0; i < 4; ++i)
            {
                float x = offsets[i].x * cosA - offsets[i].y * sinA;
                float y = offsets[i].x * sinA + offsets[i].y * cosA;
                quad[i] = {
                    center.x + x, center.y + y, screen.z, 1.0f, color, (i == 1 || i == 2) ? 1.0f : 0.0f,
                    (i >= 2) ? 1.0f : 0.0f
                };
            }

            m_device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(Vertex));
        }
    };

    const float nearMin = m_cameraY + g_precipitationConfig.nearMinOffset;
    const float nearMax = m_cameraY + g_precipitationConfig.nearMaxOffset;
    const float midMax = m_cameraY + g_precipitationConfig.midMaxOffset;
    const float farMax = m_cameraY + g_precipitationConfig.farMaxOffset;

    RenderGroup(nearMin, nearMax, g_precipitationConfig.alphaBlend3DRainNear,
                g_precipitationConfig.alphaBlendNearValue);
    RenderGroup(nearMax, midMax, g_precipitationConfig.alphaBlend3DRainMid,
                g_precipitationConfig.alphaBlendMidValue);
    RenderGroup(midMax, farMax, g_precipitationConfig.alphaBlend3DRainFar,
                g_precipitationConfig.alphaBlendFarValue);

    // After moving, respawning, and generating splatters
    if (m_drops3D.size() > 600) // 200 per group × 3
        m_drops3D.erase(m_drops3D.begin(), m_drops3D.begin() + (m_drops3D.size() - 600));

    // ✅ After all spawning is done
    if (m_splatters3D.size() > 600)
        m_splatters3D.erase(m_splatters3D.begin(), m_splatters3D.begin() + (m_splatters3D.size() - 600));

    m_device->SetTexture(0, nullptr);
}

void PrecipitationController::Render3DSplattersOverlay(const D3DVIEWPORT9& viewport)
{
    // Create splatter texture if not available
    if (!m_splatterTex)
    {
        m_device->CreateTexture(4, 4, 1, 0, chosenFormat, core::useDXVKFix ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED,
                                &m_splatterTex, nullptr);
        // m_device->CreateTexture(4, 4, 1, 0, chosenFormat, D3DPOOL_DEFAULT, &m_splatterTex, nullptr);
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
    D3DXMATRIX matProj, matView, matWorld;
    D3DXMatrixIdentity(&matWorld);
    m_device->GetTransform(D3DTS_VIEW, &matView);

    // Fallback perspective matrix (60° FOV, 16:9 aspect, near/far = 0.1, 1000)
    float aspect = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
    D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(60.0f), aspect, 0.1f, 1000.0f);

    // Estimated camera position (replace this with actual camera lookup if possible)
    // D3DXVECTOR3 camPos(0, 0, 0); // TODO: Replace with real camera pos if found
    D3DXVECTOR3 camPos = GetCameraPositionSafe();

    D3DXMatrixIdentity(&matWorld);

    // Set render state
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, g_precipitationConfig.alphaBlendSplatters);
    m_device->SetTexture(0, m_splatterTex);

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

    for (const auto& drop : m_splatters3D)
    {
        D3DXVECTOR3 screen{};
        D3DXVec3Project(&screen, &drop.position, &viewport, &matProj, &matView, &matWorld);

        if (screen.z < 0.0f || screen.z > 1.0f)
            continue;

        float flicker = (rand() % 100) / 500.0f; // up to ±0.2
        float size = std::clamp((drop.length + flicker) * 4.0f, 1.0f, 6.5f);
        float half = size * 0.5f;

        float cosA = cosf(drop.angle);
        float sinA = sinf(drop.angle);

        D3DXVECTOR2 center(screen.x, screen.y);
        D3DXVECTOR2 offsets[4] = {
            {-half, -half},
            {half, -half},
            {half, half},
            {-half, half}
        };

        // Fade alpha by life (optional: multiply with intensity later)
        BYTE alpha = static_cast<BYTE>(std::clamp(drop.life / 2.0f, 0.0f, 1.0f) * 255.0f);
        DWORD color = D3DCOLOR_ARGB(alpha, 255, 255, 255);

        Vertex quad[4];
        for (int i = 0; i < 4; ++i)
        {
            float x = offsets[i].x * cosA - offsets[i].y * sinA;
            float y = offsets[i].x * sinA + offsets[i].y * cosA;

            quad[i] = {
                center.x + x,
                center.y + y,
                screen.z,
                1.0f,
                color,
                (i == 1 || i == 2) ? 1.0f : 0.0f,
                (i >= 2) ? 1.0f : 0.0f
            };
        }

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

    BYTE alpha = static_cast<BYTE>(std::clamp(
        g_precipitationConfig.alpha2DRainMin + g_precipitationConfig.rainIntensity *
        (g_precipitationConfig.alpha2DRainMax - g_precipitationConfig.alpha2DRainMin), 0.0f, 255.0f));

    DWORD color = D3DCOLOR_ARGB(alpha, 255, 255, 255);

    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, g_precipitationConfig.alphaBlend2DRain);
    // m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
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
            drop.noiseSeed = static_cast<float>(rand()) / RAND_MAX * 100.0f;
            drop.initialized = true;
        }

        drop.y += drop.speed * 0.016f;
        float wind = (PrecipitationController::noise.noise3D(drop.y * 0.05f, core::CurrentTime * 0.0005f,
                                                             drop.noiseSeed) -
                0.5f) * 2.0f * g_precipitationConfig.
            windStrength;
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

void PrecipitationController::Update()
{
    if (!m_active)
        return;

    if (!m_device)
    {
        OutputDebugStringA("[Update] 🚫 m_device still null, skipping update\n");
        return;
    }

    camPos = GetCameraPositionSafe();
    m_cameraY = camPos.y;

    if (IsCameraCovered(camPos))
        return;

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

    if (g_precipitationConfig.fpsOverride > 0.0f)
    {
        core::fpsDeltaTime = 1.0f / g_precipitationConfig.fpsOverride;
    }
    else
    {
        DWORD currentTime = core::CurrentTime;
        float rawDelta = (currentTime - m_lastTime) / 1000.0f;

        // Clamp to prevent stalling or oversimulation
        if (rawDelta < 0.001f)
            rawDelta = 0.001f; // Min cap (1000 FPS)
        else if (rawDelta > 0.1f)
            rawDelta = 0.016f; // Max cap (~60 FPS fallback)

        core::fpsDeltaTime = rawDelta;
        m_lastTime = currentTime;
    }

    if (g_precipitationConfig.enable3DRain)
    {
        m_device->SetTexture(0, m_rainTex);
        Render3DRainOverlay(viewport);
    }

    if (g_precipitationConfig.enable3DSplatters)
    {
        Render3DSplattersOverlay(viewport);
    }

    if (g_precipitationConfig.enable2DRain)
    {
        Render2DRainOverlay(viewport);
    }

    // m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
}
