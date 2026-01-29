#include "PrecipitationController.h"
#include "RainConfigController.h"
#include <algorithm>
#include <d3d9.h>
#include <d3dx9.h>

#include "core.h"
#include "Hooking.Patterns.h"
#include "PerlinNoise.h"
#include "Game.h"
#include "WeatherGameAddresses.h"

#if WORKING_CAMERA
using namespace ngg::common;

static auto& g_precipitationConfig = RainConfigController::precipitationConfig;

static D3DXMATRIX g_mwViewMatrix{};
static bool g_mwViewValid = false;
static D3DXMATRIX g_d3dViewMatrix{};
static D3DXMATRIX g_d3dProjMatrix{};
static bool g_d3dViewValid = false;
static bool g_d3dProjValid = false;
static void* g_mwActiveViewPtr = nullptr;

void PrecipitationController::UpdateMWViewMatrix(const D3DXMATRIX& view)
{
    g_mwViewMatrix = view;
    g_mwViewValid = true;
}

void PrecipitationController::ResetMWViewMatrix()
{
    g_mwViewValid = false;
    g_mwViewMatrix = D3DXMATRIX{};
}

bool PrecipitationController::GetMWViewMatrix(D3DXMATRIX& outView)
{
    if (!g_mwViewValid)
        return false;
    outView = g_mwViewMatrix;
    return true;
}

void PrecipitationController::UpdateMWActiveViewPtr(void* viewPtr)
{
    if (viewPtr && core::IsReadable(viewPtr, WeatherGameAddresses::EViewMatrixOffset1_MW + sizeof(D3DXMATRIX)))
        g_mwActiveViewPtr = viewPtr;
}

void PrecipitationController::UpdateD3DTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* mat)
{
    if (!mat)
        return;

    const D3DXMATRIX& src = *reinterpret_cast<const D3DXMATRIX*>(mat);
    if (state == D3DTS_VIEW)
    {
        g_d3dViewMatrix = src;
        g_d3dViewValid = true;
    }
    else if (state == D3DTS_PROJECTION)
    {
        g_d3dProjMatrix = src;
        g_d3dProjValid = true;
    }
}

bool PrecipitationController::GetD3DViewProj(D3DXMATRIX& view, D3DXMATRIX& proj)
{
    if (!g_d3dViewValid || !g_d3dProjValid)
        return false;
    view = g_d3dViewMatrix;
    proj = g_d3dProjMatrix;
    return true;
}

bool PrecipitationController::GetD3DProj(D3DXMATRIX& proj)
{
    if (!g_d3dProjValid)
        return false;
    proj = g_d3dProjMatrix;
    return true;
}

static bool GetRainMatricesMW(D3DXMATRIX& outView, D3DXMATRIX& outProj)
{
    auto* rain = *reinterpret_cast<void**>(WeatherGameAddresses::RainInstancePtr_MW);
    if (!rain ||
        !core::IsReadable(rain, WeatherGameAddresses::RainProjMatrixOffset_MW + sizeof(D3DXMATRIX)))
        return false;

    D3DXMATRIX viewRaw = *reinterpret_cast<D3DXMATRIX*>(
        reinterpret_cast<uintptr_t>(rain) + WeatherGameAddresses::RainViewMatrixOffset_MW);
    D3DXMATRIX projRaw = *reinterpret_cast<D3DXMATRIX*>(
        reinterpret_cast<uintptr_t>(rain) + WeatherGameAddresses::RainProjMatrixOffset_MW);

    switch (g_precipitationConfig.rainMatrixMode)
    {
    case 1: // swap
        outView = projRaw;
        outProj = viewRaw;
        break;
    case 2: // transpose
        outView = viewRaw;
        outProj = projRaw;
        D3DXMatrixTranspose(&outView, &outView);
        D3DXMatrixTranspose(&outProj, &outProj);
        break;
    case 3: // swap + transpose
        outView = projRaw;
        outProj = viewRaw;
        D3DXMatrixTranspose(&outView, &outView);
        D3DXMatrixTranspose(&outProj, &outProj);
        break;
    case 0: // raw
    default:
        outView = viewRaw;
        outProj = projRaw;
        break;
    }

    return true;
}

static bool GetEViewMatrixMW(D3DXMATRIX& outView)
{
    void* view = g_mwActiveViewPtr;
    if (!view)
        view = *reinterpret_cast<void**>(WeatherGameAddresses::EViewCurrentPtr_MW);
    if (!view || !core::IsReadable(view, WeatherGameAddresses::EViewMatrixOffset0_MW + sizeof(D3DXMATRIX)))
        return false;
    const auto tryOffset = [&](uintptr_t offset, D3DXMATRIX& candidate) -> bool
    {
        auto* ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(view) + offset);
        if (!core::IsReadable(ptr, sizeof(D3DXMATRIX)))
            return false;
        std::memcpy(&candidate, ptr, sizeof(D3DXMATRIX));
        D3DXMATRIX inv{};
        if (!D3DXMatrixInverse(&inv, nullptr, &candidate))
            return false;
        return D3DXVECTOR3(inv._41, inv._42, inv._43) != D3DXVECTOR3(0, 0, 0);
    };

    D3DXMATRIX m0{}, m1{};
    if (tryOffset(WeatherGameAddresses::EViewMatrixOffset0_MW, m0))
    {
        outView = m0;
        return true;
    }
    if (tryOffset(WeatherGameAddresses::EViewMatrixOffset1_MW, m1))
    {
        outView = m1;
        return true;
    }
    return false;
}

static void DebugLogMatrixOnce(const char* tag, const D3DXMATRIX& mat)
{
    static bool logged = false;
    if (logged)
        return;
    logged = true;

    char buf[512];
    sprintf_s(buf,
              "[RainDebug MW] %s view m00=%.3f m01=%.3f m02=%.3f m03=%.3f | m10=%.3f m11=%.3f m12=%.3f m13=%.3f\n",
              tag,
              mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
              mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3]);
    OutputDebugStringA(buf);
    sprintf_s(buf,
              "[RainDebug MW] %s view m20=%.3f m21=%.3f m22=%.3f m23=%.3f | m30=%.3f m31=%.3f m32=%.3f m33=%.3f\n",
              tag,
              mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
              mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
    OutputDebugStringA(buf);
}

static void DebugLogMatrixNoGuard(const char* tag, const D3DXMATRIX& mat)
{
    char buf[512];
    sprintf_s(buf,
              "[RainDebug MW] %s m00=%.3f m01=%.3f m02=%.3f m03=%.3f | m10=%.3f m11=%.3f m12=%.3f m13=%.3f\n",
              tag,
              mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
              mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3]);
    OutputDebugStringA(buf);
    sprintf_s(buf,
              "[RainDebug MW] %s m20=%.3f m21=%.3f m22=%.3f m23=%.3f | m30=%.3f m31=%.3f m32=%.3f m33=%.3f\n",
              tag,
              mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
              mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
    OutputDebugStringA(buf);
}

static void DebugLogMatrixSourceOnce(const char* source)
{
    static bool logged = false;
    if (logged)
        return;
    logged = true;
    char buf[256];
    sprintf_s(buf, "[RainDebug MW] using view source: %s\n", source);
    OutputDebugStringA(buf);
}

static bool IsIdentityMatrix(const D3DXMATRIX& m)
{
    return m.m[0][0] == 1.0f && m.m[1][1] == 1.0f && m.m[2][2] == 1.0f && m.m[3][3] == 1.0f &&
        m.m[0][1] == 0.0f && m.m[0][2] == 0.0f && m.m[0][3] == 0.0f &&
        m.m[1][0] == 0.0f && m.m[1][2] == 0.0f && m.m[1][3] == 0.0f &&
        m.m[2][0] == 0.0f && m.m[2][1] == 0.0f && m.m[2][3] == 0.0f &&
        m.m[3][0] == 0.0f && m.m[3][1] == 0.0f && m.m[3][2] == 0.0f;
}

static bool IsProjectionLike(const D3DXMATRIX& m)
{
    return m.m[3][3] == 0.0f && m.m[2][3] != 0.0f;
}

static bool GetViewFromCameraPtr(void* cameraPtr, D3DXMATRIX& outView)
{
    if (!cameraPtr || !core::IsReadable(cameraPtr, sizeof(D3DXMATRIX)))
        return false;

    auto* camMat = reinterpret_cast<D3DXMATRIX*>(cameraPtr);
    D3DXMATRIX inv{};
    if (!D3DXMatrixInverse(&inv, nullptr, camMat))
        return false;
    outView = inv;
    return !IsIdentityMatrix(outView);
}

static bool IsLikelyPointer(void* p)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (!p || !VirtualQuery(p, &mbi, sizeof(mbi)))
        return false;
    if (mbi.State != MEM_COMMIT)
        return false;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))
        return false;
    return true;
}

static bool GetCameraMatrixFromPtr(void* cameraPtr, D3DXMATRIX& outMat)
{
    if (!cameraPtr || !IsLikelyPointer(cameraPtr))
        return false;

    const uintptr_t base = reinterpret_cast<uintptr_t>(cameraPtr);
    constexpr uintptr_t offsets[] = {0x0, 0x10, 0x20, 0x30, 0x40};
    for (uintptr_t off : offsets)
    {
        auto* mat = reinterpret_cast<D3DXMATRIX*>(base + off);
        if (!core::IsReadable(mat, sizeof(D3DXMATRIX)))
            continue;
        outMat = *mat;
        if (!IsIdentityMatrix(outMat))
            return true;
    }
    return false;
}

static bool GetViewFromCameraParams(void* cameraPtr, D3DXMATRIX& outView)
{
    if (!cameraPtr || !IsLikelyPointer(cameraPtr))
        return false;

    const uintptr_t base = reinterpret_cast<uintptr_t>(cameraPtr);
    const auto readVec3 = [&](uintptr_t off, D3DXVECTOR3& v) -> bool
    {
        auto* p = reinterpret_cast<const float*>(base + off);
        if (!core::IsReadable(const_cast<float*>(p), sizeof(float) * 3))
            return false;
        v = D3DXVECTOR3(p[0], p[1], p[2]);
        return _finite(v.x) && _finite(v.y) && _finite(v.z);
    };

    D3DXVECTOR3 right{}, up{}, back{}, pos{};
    if (!readVec3(0x00, right) ||
        !readVec3(0x10, up) ||
        !readVec3(0x20, back) ||
        !readVec3(WeatherGameAddresses::CameraPositionOffset_MW, pos))
        return false;

    if (D3DXVec3LengthSq(&right) < 0.0001f ||
        D3DXVec3LengthSq(&up) < 0.0001f ||
        D3DXVec3LengthSq(&back) < 0.0001f)
        return false;

    // MW uses column basis (right, up, back)
    D3DXMatrixIdentity(&outView);
    outView._11 = right.x; outView._21 = right.y; outView._31 = right.z;
    outView._12 = up.x;    outView._22 = up.y;    outView._32 = up.z;
    outView._13 = back.x;  outView._23 = back.y;  outView._33 = back.z;
    outView._41 = -D3DXVec3Dot(&right, &pos);
    outView._42 = -D3DXVec3Dot(&up, &pos);
    outView._43 = -D3DXVec3Dot(&back, &pos);

    return !IsIdentityMatrix(outView);
}

static bool GetViewFromCameraStruct(void* cameraPtr, D3DXMATRIX& outView)
{
    if (!cameraPtr || !IsLikelyPointer(cameraPtr))
        return false;

    // Try CameraParams.Matrix at 0x0 (CameraParams) and at 0xD4 (Camera)
    constexpr uintptr_t kCameraParamOffsets[] = {0x0, 0xD4};
    for (uintptr_t off : kCameraParamOffsets)
    {
        auto* mat = reinterpret_cast<D3DXMATRIX*>(reinterpret_cast<uintptr_t>(cameraPtr) + off);
        if (!core::IsReadable(mat, sizeof(D3DXMATRIX)))
            continue;

        D3DXMATRIX m = *mat;
        if (!IsIdentityMatrix(m) && !IsProjectionLike(m))
        {
            outView = m;
            return true;
        }

        D3DXMATRIX inv{};
        if (D3DXMatrixInverse(&inv, nullptr, &m))
        {
            if (!IsIdentityMatrix(inv))
            {
                outView = inv;
                return true;
            }
        }
    }

    return false;
}

static bool ProjectWithViewProj(const D3DXVECTOR3& world, const D3DVIEWPORT9& vp,
                                const D3DXMATRIX& viewProj, D3DXVECTOR3& outScreen)
{
    D3DXVECTOR4 v(world.x, world.y, world.z, 1.0f);
    D3DXVECTOR4 clip{};
    D3DXVec4Transform(&clip, &v, &viewProj);
    if (clip.w == 0.0f)
        return false;
    float ndcX = clip.x / clip.w;
    float ndcY = clip.y / clip.w;
    float ndcZ = clip.z / clip.w;
    outScreen.x = vp.X + (ndcX + 1.0f) * 0.5f * vp.Width;
    outScreen.y = vp.Y + (1.0f - ndcY) * 0.5f * vp.Height;
    outScreen.z = (ndcZ + 1.0f) * 0.5f;
    return true;
}

static void* GetCameraPtrFromEView()
{
    static bool logged = false;
    void* view = g_mwActiveViewPtr;
    if (!view)
        view = *reinterpret_cast<void**>(WeatherGameAddresses::EViewCurrentPtr_MW);
    if (!view)
        return nullptr;
    if (!core::IsReadable(view, WeatherGameAddresses::EViewCameraOffset_MW + sizeof(void*)))
        return nullptr;
    void* cam = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(view) + WeatherGameAddresses::EViewCameraOffset_MW);
    void* camNorm = cam;
    if (reinterpret_cast<uintptr_t>(camNorm) & 0x80000000u)
        camNorm = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(camNorm) & 0x7FFFFFFFu);
    if (!logged)
    {
        char buf[256];
        sprintf_s(buf, "[RainDebug MW] eViewPtr=0x%08X pCamera=0x%08X pCameraNorm=0x%08X\n",
                  (unsigned)(uintptr_t)view, (unsigned)(uintptr_t)cam, (unsigned)(uintptr_t)camNorm);
        OutputDebugStringA(buf);
        logged = true;
    }
    if (core::IsReadable(camNorm, sizeof(void*)))
        return camNorm;
    return cam;
}

static void DebugLogCameraPtrOnce(const char* tag, void* camPtr)
{
    static bool logged = false;
    if (logged)
        return;
    logged = true;
    char buf[256];
    sprintf_s(buf, "[RainDebug MW] %s cameraPtr=0x%08X\n", tag, (unsigned)(uintptr_t)camPtr);
    OutputDebugStringA(buf);
}

static void DebugLogCameraPtrDetailsOnce(void* camPtr)
{
    static bool logged = false;
    if (logged)
        return;
    logged = true;
    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(camPtr, &mbi, sizeof(mbi)))
    {
        char buf[256];
        sprintf_s(buf, "[RainDebug MW] cameraPtr mbi Base=0x%08X Protect=0x%X State=0x%X\n",
                  (unsigned)(uintptr_t)mbi.BaseAddress, (unsigned)mbi.Protect, (unsigned)mbi.State);
        OutputDebugStringA(buf);
    }
}

static void DebugLogCameraPtrDetailsOnce2(const char* tag, void* camPtr)
{
    static bool logged = false;
    if (logged)
        return;
    logged = true;
    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(camPtr, &mbi, sizeof(mbi)))
    {
        char buf[256];
        sprintf_s(buf, "[RainDebug MW] %s ptr=0x%08X Base=0x%08X Protect=0x%X State=0x%X\n",
                  tag, (unsigned)(uintptr_t)camPtr, (unsigned)(uintptr_t)mbi.BaseAddress,
                  (unsigned)mbi.Protect, (unsigned)mbi.State);
        OutputDebugStringA(buf);
    }
}

static inline float GetUpCoord(const D3DXVECTOR3& v)
{
    return v.y;
}

static inline void SetUpCoord(D3DXVECTOR3& v, float up)
{
    v.y = up;
}

bool PrecipitationController::IsActive() const
{
    return m_active;
}

void PrecipitationController::DebugEVIEWListPtr()
{
    if (detected_game != GameType::PS && detected_game != GameType::UC)
    {
        OutputDebugStringA("[DebugEVIEWListPtr] EVIEW scan not supported for this game.\n");
        return;
    }

    auto match = hook::pattern(
        (detected_game == GameType::PS)
            ? "A1 20 BE A8 00 50 E8 ?? ??"
            : "D9 C9 D9 5C 24 1C D9 44 24 1C DE D9 DF E0 F6 C4 41 75");
    // UC pattern: unique fld/fstp/fcompp/fnstsw/test/loop combo

    if (match.empty())
    {
        OutputDebugStringA("[DebugEVIEWListPtr] ❌ Pattern not found.\n");
        return;
    }

    // char logBuf[128];
    // sprintf_s(logBuf, "[DebugEVIEWListPtr] Matches found: %zu\n", match.size());
    // OutputDebugStringA(logBuf);

    if (detected_game == GameType::PS)
    {
        uintptr_t ptrAddr = *match.get(0).get<uintptr_t>(1); // +1 skips A1 opcode
        g_EVIEW_LIST_PTR = reinterpret_cast<EViewNode**>(ptrAddr);
    }
    else if (detected_game == GameType::UC)
    {
        uintptr_t addr = reinterpret_cast<uintptr_t>(match.get_first());
        uintptr_t targetAddr = *reinterpret_cast<uintptr_t*>(addr + 1);
        g_EVIEW_LIST_PTR = reinterpret_cast<EViewNode**>(targetAddr);
    }

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
    if (detected_game == GameType::MW)
    {
        if (m_device)
        {
            D3DXMATRIX view{};
            if (SUCCEEDED(m_device->GetTransform(D3DTS_VIEW, &view)))
            {
                D3DXMATRIX invView{};
                if (D3DXMatrixInverse(&invView, nullptr, &view))
                    return D3DXVECTOR3(invView._41, invView._42, invView._43);
            }
        }

        uintptr_t base = WeatherGameAddresses::EViewArrayBase_MW;
        D3DXVECTOR3 fallbackPos(0, 0, 0);
        bool hasFallback = false;
        static bool loggedPtr = false;
        for (size_t i = 0; i < WeatherGameAddresses::EViewArrayCount_MW; ++i)
        {
            auto* view = reinterpret_cast<void*>(base + i * WeatherGameAddresses::EViewSize_MW);
            if (!view || !core::IsReadable(view, WeatherGameAddresses::EViewCameraOffset_MW + sizeof(void*)))
                continue;

            auto active = *reinterpret_cast<unsigned char*>(
                reinterpret_cast<uintptr_t>(view) + WeatherGameAddresses::EViewActiveFlagOffset_MW);
            auto* camera = *reinterpret_cast<void**>(
                reinterpret_cast<uintptr_t>(view) + WeatherGameAddresses::EViewCameraOffset_MW);
            if (reinterpret_cast<uintptr_t>(camera) & 0x80000000u)
                camera = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(camera) & 0x7FFFFFFFu);
            if (!loggedPtr)
            {
                char buf[256];
                sprintf_s(buf, "[RainDebug MW] eView entry=0x%08X active=%u camera=0x%08X\n",
                          (unsigned)(uintptr_t)view, (unsigned)active,
                          (unsigned)(uintptr_t)camera);
                OutputDebugStringA(buf);
                loggedPtr = true;
            }
            if (!camera)
                continue;

            D3DXVECTOR3 candidate(0, 0, 0);
            bool hasCandidate = false;

            if (core::IsReadable(camera, WeatherGameAddresses::CameraMatrixV3Offset_MW + sizeof(float) * 16))
            {
                auto* camMat = reinterpret_cast<D3DXMATRIX*>(camera);
                D3DXMATRIX invView{};
                if (D3DXMatrixInverse(&invView, nullptr, camMat))
                {
                    candidate = D3DXVECTOR3(invView._41, invView._42, invView._43);
                    hasCandidate = (candidate != D3DXVECTOR3(0, 0, 0));
                }
            }

            if (!hasCandidate &&
                core::IsReadable(camera, WeatherGameAddresses::CameraPositionOffset_MW + sizeof(float) * 3))
            {
                auto* pos = reinterpret_cast<float*>(
                    reinterpret_cast<uintptr_t>(camera) + WeatherGameAddresses::CameraPositionOffset_MW);
                candidate = D3DXVECTOR3(pos[0], pos[1], pos[2]);
                hasCandidate = (candidate != D3DXVECTOR3(0, 0, 0));
            }

            if (hasCandidate)
            {
                m_cameraPtr = camera;
                if (active)
                    return candidate;
                fallbackPos = candidate;
                hasFallback = true;
            }
        }

        if (hasFallback)
            return fallbackPos;
    }
    else if (detected_game == GameType::UC)
    {
        auto head = reinterpret_cast<EViewNode*>(WeatherGameAddresses::EViewListHeadPtr_UC);
        if (head)
        {
            auto mat = reinterpret_cast<D3DXMATRIX*>(
                reinterpret_cast<uintptr_t>(head) + WeatherGameAddresses::NodeMatrixOffset);
            return D3DXVECTOR3(mat->_41, mat->_42, mat->_43);
        }
    }
    else if (detected_game == GameType::PS)
    {
        if (g_EVIEW_LIST_PTR == nullptr)
        {
            DebugEVIEWListPtr();
        }

        if (g_EVIEW_LIST_PTR)
        {
            auto head = *g_EVIEW_LIST_PTR;
            if (head)
            {
                auto mat = reinterpret_cast<D3DXMATRIX*>(
                    reinterpret_cast<uintptr_t>(head) + WeatherGameAddresses::NodeMatrixOffset);
                return D3DXVECTOR3(mat->_41, mat->_42, mat->_43);
            }
        }
    }

    if (m_device)
    {
        D3DXMATRIX view;
        if (SUCCEEDED(m_device->GetTransform(D3DTS_VIEW, &view)))
        {
            D3DXMATRIX invView;
            if (D3DXMatrixInverse(&invView, nullptr, &view))
                return D3DXVECTOR3(invView._41, invView._42, invView._43);
        }
    }

    OutputDebugStringA("[GetCameraPositionSafe] 🚫 No valid camera source yet.\n");
    return D3DXVECTOR3(0, 0, 0);
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

    OutputDebugStringA("[PrecipitationController::enable] enabling\n");
    RainConfigController::Load();
    m_active = true;

    if (g_precipitationConfig.fpsOverride > 0.0f)
        core::fpsDeltaTime = 1.0f / g_precipitationConfig.fpsOverride;
    else
        m_lastTime = timeGetTime(); // where m_lastTime is a DWORD member

    // Select rain texture format
    chosenFormat = D3DFMT_A8R8G8B8;
    if (detected_game == GameType::UC)
    {
        // chosenFormat = GetSupportedRainTextureFormat(m_device);
        // if (chosenFormat == D3DFMT_UNKNOWN)
        // {
        //     OutputDebugStringA("[RainTex] No supported texture format found\n");
        //     return false;
        // }
        // chosenFormat = D3DFMT_A8B8G8R8;
    }

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
        m_cameraY = GetUpCoord(camPos);
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
    SetUpCoord(splatter.position, m_cameraY - 1.0f);

        // splatter.velocity = D3DXVECTOR3(0, 0, 0);
        splatter.velocity = D3DXVECTOR3(0, 0, 0);
        SetUpCoord(splatter.velocity, -3.0f); // or adjustable by config

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
    OutputDebugStringA("[PrecipitationController::disable] disabling\n");
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
    drop.velocity = D3DXVECTOR3(settings.windSway, 0.0f, 0.0f);
    SetUpCoord(drop.velocity, -settings.speed);
    drop.position = D3DXVECTOR3(
        camPos.x + static_cast<float>((rand() % 400) - 200), // wider spread around camera
        0.0f,
        camPos.z + static_cast<float>((rand() % 1000) - 200) // depth around camera
    );
    SetUpCoord(drop.position, camY + 25.0f);
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
    if (!m_device)
        return false;

    if (m_rainTex)
    {
        m_rainTex->Release();
        m_rainTex = nullptr;
    }

    HRESULT hr = m_device->CreateTexture(16, 512, 1, 0, chosenFormat, D3DPOOL_MANAGED, &m_rainTex, nullptr);
    if (FAILED(hr) && core::useDXVKFix)
    {
        hr = m_device->CreateTexture(16, 512, 1, 0, chosenFormat, D3DPOOL_DEFAULT, &m_rainTex, nullptr);
    }

    if (FAILED(hr) || !m_rainTex)
    {
        char buf[128];
        sprintf_s(buf, "[IsCreatedRainTexture] CreateTexture failed hr=0x%08X\n", static_cast<unsigned>(hr));
        OutputDebugStringA(buf);
        return false;
    }
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
            if (FAILED(
                D3DXCreateTextureFromFileA(m_device, g_precipitationConfig.raindropTexturePath.c_str(), &m_rainTex)))
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
    bool usedRainMatrices = false;
    bool hasProj = false;
    bool useViewProjOnly = false;
    D3DXMATRIX matViewProj{};
    if (detected_game == GameType::MW)
    {
        bool selected = false;
        if (g_precipitationConfig.preferHookedView)
        {
            if (PrecipitationController::GetMWViewMatrix(matView) && !IsIdentityMatrix(matView))
            {
                DebugLogMatrixSourceOnce("preferHookedView (MWView)");
                DebugLogMatrixOnce("preferHookedView", matView);
                selected = true;
            }
            else if (PrecipitationController::GetD3DViewProj(matView, matProj))
            {
                usedRainMatrices = true;
                hasProj = true;
                DebugLogMatrixSourceOnce("preferHookedView (D3D)");
                DebugLogMatrixOnce("preferHookedView", matView);
                selected = true;
            }
        }
        if (!selected && PrecipitationController::GetD3DViewProj(matView, matProj))
        {
            usedRainMatrices = true;
            hasProj = true;
            DebugLogMatrixSourceOnce("D3D view/proj");
            DebugLogMatrixOnce("D3D", matView);
        }
        else if (!selected && GetEViewMatrixMW(matView))
        {
            DebugLogMatrixSourceOnce("eView matrix");
            DebugLogMatrixOnce("eView", matView);
            if (IsProjectionLike(matView))
            {
                void* camPtr = PrecipitationController::Get()->m_cameraPtr;
                if (!camPtr)
                    camPtr = GetCameraPtrFromEView();
                DebugLogCameraPtrOnce("Render3D", camPtr);
                D3DXMATRIX camMat{};
                if (GetCameraMatrixFromPtr(camPtr, camMat))
                {
                    D3DXMATRIX inv{};
                    if (D3DXMatrixInverse(&inv, nullptr, &camMat))
                        matView = inv;
                }
                else
                {
                    D3DXMATRIX viewFromParams{};
                    if (GetViewFromCameraParams(camPtr, viewFromParams))
                        matView = viewFromParams;
                }
                if (GetViewFromCameraStruct(camPtr, matView))
                {
                    DebugLogMatrixSourceOnce("cameraPtr struct view");
                    DebugLogMatrixOnce("cameraPtr struct", matView);
                }
                if (GetViewFromCameraPtr(camPtr, matView))
                {
                    DebugLogMatrixSourceOnce("cameraPtr view");
                    DebugLogMatrixOnce("cameraPtr", matView);
                }
            }
            if (IsProjectionLike(matView))
            {
                useViewProjOnly = true;
                matViewProj = matView;
            }
            else
            {
                hasProj = PrecipitationController::GetD3DProj(matProj);
            }
            // projection will come from device
        }
        else if (!selected && g_precipitationConfig.useMWLookAtMatrix && PrecipitationController::GetMWViewMatrix(matView))
        {
            DebugLogMatrixSourceOnce("MW LookAt hook");
            DebugLogMatrixOnce("MWLookAt", matView);
            hasProj = PrecipitationController::GetD3DProj(matProj);
            // projection will come from device
        }
        else if (!selected && GetRainMatricesMW(matView, matProj))
        {
            usedRainMatrices = true;
            hasProj = true;
            DebugLogMatrixSourceOnce("Rain matrices");
            DebugLogMatrixOnce("Rain", matView);
        }
    }

    if (!usedRainMatrices &&
        !GetEViewMatrixMW(matView) &&
        !(g_precipitationConfig.useMWLookAtMatrix && PrecipitationController::GetMWViewMatrix(matView)))
    {
        m_device->GetTransform(D3DTS_VIEW, &matView);
        DebugLogMatrixSourceOnce("D3DTS_VIEW fallback");
        DebugLogMatrixOnce("D3DTS_VIEW", matView);
    }

    if (!hasProj &&
        FAILED(m_device->GetTransform(D3DTS_PROJECTION, &matProj)) &&
        !usedRainMatrices)
    {
        float aspect = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
        D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(60.0f), aspect, 1.0f, 500.0f);
    }
    D3DXMatrixIdentity(&matIdentity);


    // ✅ Ensure correct alpha blending and texture state before drawing
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_ALPHATESTENABLE, detected_game == GameType::MW ? FALSE : TRUE);
    m_device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    m_device->SetRenderState(D3DRS_ALPHAREF, 8);
    m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    if (detected_game == GameType::UC)
    {
        m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    }

    m_device->SetTexture(0, m_rainTex);

    // Move drops
    for (auto& drop : m_drops3D)
    {
        const RainGroupSettings* group = ChooseGroupByY(GetUpCoord(drop.position));

        float up = GetUpCoord(drop.position);
        float wind = (noise.noise3D(up * 0.05f, core::CurrentTime * 0.0005f, 0.0f) - 0.5f) * 2.0f * group->windSway;
        drop.position.x += wind;
        SetUpCoord(drop.position, up - group->speed);

        if (GetUpCoord(drop.position) < m_cameraY - 10.0f)
        {
            Drop3D splatter;

            // Compute correct forward vector from view matrix
            D3DXMATRIX invView;
            D3DXMatrixInverse(&invView, nullptr, &matView); // Invert the view matrix
            D3DXVECTOR3 camForward(invView._31, invView._32, invView._33); // World-space forward

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
            SetUpCoord(splatterPos, m_cameraY - 1.0f);

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
            if (GetUpCoord(drop.position) < minY || GetUpCoord(drop.position) >= maxY)
                continue;

        D3DXVECTOR3 screen;
        if (useViewProjOnly)
        {
            if (!ProjectWithViewProj(drop.position, viewport, matViewProj, screen))
                continue;
        }
        else
        {
            D3DXVec3Project(&screen, &drop.position, &viewport, &matProj, &matView, &matIdentity);
        }

            if (screen.z < 0.0f || screen.z > 1.0f)
                continue;



            float size = drop.length * 4.0f;
            float half = size * 0.5f;
            DWORD color = D3DCOLOR_ARGB(alpha, 255, 255, 255);

            float cosA = cosf(drop.angle);
            float sinA = sinf(drop.angle);

            D3DXVECTOR2 center(screen.x, screen.y);
            D3DXVECTOR2 offsets[4] = {{-half, -half}, {half, -half}, {half, half}, {-half, half}};
            RHWVertex quad[4];

            for (int i = 0; i < 4; ++i)
            {
                float x = offsets[i].x * cosA - offsets[i].y * sinA;
                float y = offsets[i].x * sinA + offsets[i].y * cosA;
                quad[i] = {
                    center.x + x, center.y + y, screen.z, 1.0f, color, (i == 1 || i == 2) ? 1.0f : 0.0f,
                    (i >= 2) ? 1.0f : 0.0f
                };
            }

            m_device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(RHWVertex));
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
    // ✅ Ensure correct alpha blending and texture state before drawing
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    m_device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    m_device->SetRenderState(D3DRS_ALPHAREF, 8);
    m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    if (detected_game == GameType::UC)
    {
        m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    }

    // Create splatter texture if not available
    if (!m_splatterTex)
    {
        m_device->CreateTexture(16, 16, 1, 0, chosenFormat, core::useDXVKFix ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED,
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
    bool usedRainMatrices = false;
    bool hasProj = false;
    bool useViewProjOnly = false;
    D3DXMATRIX matViewProj{};
    if (detected_game == GameType::MW)
    {
        bool selected = false;
        if (g_precipitationConfig.preferHookedView)
        {
            if (PrecipitationController::GetMWViewMatrix(matView) && !IsIdentityMatrix(matView))
            {
                DebugLogMatrixSourceOnce("Splatters preferHookedView (MWView)");
                DebugLogMatrixOnce("Splatters preferHookedView", matView);
                selected = true;
            }
            else if (PrecipitationController::GetD3DViewProj(matView, matProj))
            {
                usedRainMatrices = true;
                hasProj = true;
                DebugLogMatrixSourceOnce("Splatters preferHookedView (D3D)");
                DebugLogMatrixOnce("Splatters preferHookedView", matView);
                selected = true;
            }
        }
        if (!selected && PrecipitationController::GetD3DViewProj(matView, matProj))
        {
            usedRainMatrices = true;
            hasProj = true;
            DebugLogMatrixSourceOnce("Splatters D3D view/proj");
            DebugLogMatrixOnce("Splatters D3D", matView);
        }
        else if (!selected && GetEViewMatrixMW(matView))
        {
            DebugLogMatrixSourceOnce("Splatters eView matrix");
            DebugLogMatrixOnce("Splatters eView", matView);
            if (IsProjectionLike(matView))
            {
                void* camPtr = PrecipitationController::Get()->m_cameraPtr;
                if (!camPtr)
                    camPtr = GetCameraPtrFromEView();
                DebugLogCameraPtrOnce("Splatters", camPtr);
                D3DXMATRIX camMat{};
                if (GetCameraMatrixFromPtr(camPtr, camMat))
                {
                    D3DXMATRIX inv{};
                    if (D3DXMatrixInverse(&inv, nullptr, &camMat))
                        matView = inv;
                }
                else
                {
                    D3DXMATRIX viewFromParams{};
                    if (GetViewFromCameraParams(camPtr, viewFromParams))
                        matView = viewFromParams;
                }
                if (GetViewFromCameraStruct(camPtr, matView))
                {
                    DebugLogMatrixSourceOnce("Splatters cameraPtr struct view");
                    DebugLogMatrixOnce("Splatters cameraPtr struct", matView);
                }
                if (GetViewFromCameraPtr(camPtr, matView))
                {
                    DebugLogMatrixSourceOnce("Splatters cameraPtr view");
                    DebugLogMatrixOnce("Splatters cameraPtr", matView);
                }
            }
            if (IsProjectionLike(matView))
            {
                useViewProjOnly = true;
                matViewProj = matView;
            }
            else
            {
                hasProj = PrecipitationController::GetD3DProj(matProj);
            }
            // projection will come from device
        }
        else if (!selected && g_precipitationConfig.useMWLookAtMatrix && PrecipitationController::GetMWViewMatrix(matView))
        {
            DebugLogMatrixSourceOnce("Splatters MW LookAt hook");
            DebugLogMatrixOnce("Splatters MWLookAt", matView);
            hasProj = PrecipitationController::GetD3DProj(matProj);
            // projection will come from device
        }
        else if (!selected && GetRainMatricesMW(matView, matProj))
        {
            usedRainMatrices = true;
            hasProj = true;
            DebugLogMatrixSourceOnce("Splatters Rain matrices");
            DebugLogMatrixOnce("Splatters Rain", matView);
        }
    }

    if (!usedRainMatrices &&
        !GetEViewMatrixMW(matView) &&
        !(g_precipitationConfig.useMWLookAtMatrix && PrecipitationController::GetMWViewMatrix(matView)))
    {
        m_device->GetTransform(D3DTS_VIEW, &matView);
        DebugLogMatrixSourceOnce("Splatters D3DTS_VIEW fallback");
        DebugLogMatrixOnce("Splatters D3DTS_VIEW", matView);
    }

    if (!hasProj &&
        FAILED(m_device->GetTransform(D3DTS_PROJECTION, &matProj)) &&
        !usedRainMatrices)
    {
        float aspect = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
        D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(60.0f), aspect, 0.1f, 1000.0f);
    }

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
        if (useViewProjOnly)
        {
            if (!ProjectWithViewProj(drop.position, viewport, matViewProj, screen))
                continue;
        }
        else
        {
            D3DXVec3Project(&screen, &drop.position, &viewport, &matProj, &matView, &matWorld);
        }

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

        RHWVertex quad[4];
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

        m_device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(RHWVertex));
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
    m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
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

        Vertex verts[2] = {
            {drop.x, drop.y, 0.0f, 1.0f, color},
            {drop.x, drop.y + drop.length, 0.0f, 1.0f, color}
        };

        m_device->DrawPrimitiveUP(D3DPT_LINELIST, 1, verts, sizeof(Vertex));
    }
}

void PrecipitationController::Update()
{
    if (!m_active)
    {
        static bool loggedInactive = false;
        if (!loggedInactive)
        {
            OutputDebugStringA("[PrecipitationController::Update] inactive\n");
            loggedInactive = true;
        }
        return;
    }

    if (!m_device)
    {
        OutputDebugStringA("[Update] 🚫 m_device still null, skipping update\n");
        return;
    }

    if (detected_game == GameType::MW)
    {
        PrecipitationController::ResetMWViewMatrix();
        D3DXMATRIX view{};
        D3DXMATRIX proj{};
        bool hasView = false;
        bool hasProj = false;
        if (g_precipitationConfig.preferHookedView)
        {
            if (PrecipitationController::GetMWViewMatrix(view) && !IsIdentityMatrix(view))
                hasView = true;
            else if (PrecipitationController::GetD3DViewProj(view, proj))
            {
                hasView = true;
                hasProj = true;
            }
        }
        if (!hasView && PrecipitationController::GetD3DViewProj(view, proj))
        {
            hasView = true;
            hasProj = true;
        }
        if (!hasView && GetEViewMatrixMW(view))
        {
            hasView = true;
            hasProj = PrecipitationController::GetD3DProj(proj);
            if (IsProjectionLike(view))
            {
                void* camPtr = PrecipitationController::Get()->m_cameraPtr;
                if (!camPtr)
                    camPtr = GetCameraPtrFromEView();
                DebugLogCameraPtrOnce("Update", camPtr);
                DebugLogCameraPtrDetailsOnce(camPtr);
                if (reinterpret_cast<uintptr_t>(camPtr) & 0x80000000u)
                {
                    void* camNorm = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(camPtr) & 0x7FFFFFFFu);
                    DebugLogCameraPtrDetailsOnce2("cameraPtrNorm", camNorm);
                }
                D3DXMATRIX camMat{};
                if (GetCameraMatrixFromPtr(camPtr, camMat))
                {
                    D3DXMATRIX inv{};
                    if (D3DXMatrixInverse(&inv, nullptr, &camMat))
                        view = inv;
                }
                else
                {
                    D3DXMATRIX viewFromParams{};
                    if (GetViewFromCameraParams(camPtr, viewFromParams))
                        view = viewFromParams;
                }
                if (GetViewFromCameraStruct(camPtr, view))
                {
                    DebugLogMatrixSourceOnce("Update cameraPtr struct view");
                    DebugLogMatrixNoGuard("Update cameraPtr struct", view);
                }
                if (GetViewFromCameraPtr(camPtr, view))
                {
                    DebugLogMatrixSourceOnce("Update cameraPtr view");
                    DebugLogMatrixNoGuard("Update cameraPtr", view);
                }
            }
        }
        if (!hasView && g_precipitationConfig.useMWLookAtMatrix && PrecipitationController::GetMWViewMatrix(view))
        {
            hasView = true;
            hasProj = PrecipitationController::GetD3DProj(proj);
        }
        if (!hasView && GetRainMatricesMW(view, proj))
        {
            hasView = true;
            hasProj = true;
        }

        if (hasView)
        {
            static bool loggedUpdateMatrix = false;
            if (!loggedUpdateMatrix)
            {
                DebugLogMatrixSourceOnce("Update hasView");
                DebugLogMatrixNoGuard("Update view", view);
                if (hasProj)
                    DebugLogMatrixNoGuard("Update proj", proj);
                loggedUpdateMatrix = true;
            }
            D3DXMATRIX invView{};
            if (D3DXMatrixInverse(&invView, nullptr, &view))
                camPos = D3DXVECTOR3(invView._41, invView._42, invView._43);
        }
    }

    if (camPos == D3DXVECTOR3(0, 0, 0))
        camPos = GetCameraPositionSafe();

    m_cameraY = GetUpCoord(camPos);

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
        if (!m_rainTex && !g_precipitationConfig.use_raindrop_dds)
            IsCreatedRainTexture();

        m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
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
#endif