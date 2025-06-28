#pragma once
#include "features.h"
#include "core.h"
#include "PerlinNoise.h"

class PrecipitationController : public ngg::common::Feature
{
public:
    // Simplified view node
    struct EViewNode
    {
        EViewNode* prev; // 0x00
        EViewNode* next; // 0x04
        char padding[0x40]; // Up to 0x40
        D3DXMATRIX viewMatrix; // 0x40
    };

    EViewNode** g_EVIEW_LIST_PTR = nullptr;

    LPDIRECT3DTEXTURE9 m_rainTex{nullptr};
    LPDIRECT3DTEXTURE9 m_splatterTex{nullptr};
    inline static D3DFORMAT chosenFormat;

    static PrecipitationController* Get()
    {
        static PrecipitationController instance;
        return &instance;
    }

    const char* name() const override { return "PrecipitationController"; }

    void DebugEVIEWListPtr();
    D3DXVECTOR3 GetCameraPositionSafe();
    D3DXVECTOR3 GetCameraPositionSafe_Static_Addr();
    bool IsCameraCovered(const D3DXVECTOR3& camPos);
    void enable() override;
    void disable() override;

    void Render2DRainOverlay(const D3DVIEWPORT9& viewport, const D3DXVECTOR3& cam_pos);
    void Update(IDirect3DDevice9* device);
    bool IsActive() const;

private:
    size_t m_callbackId = static_cast<size_t>(-1);

    struct Drop
    {
        float length = 0.0f;
        bool initialized = false;
    };

    struct Drop2D
    {
        float x, y;
        float speed;
        float length;
        bool initialized;
        float noiseSeed; // for more diversity
    };

    struct Drop3D : Drop
    {
        D3DXVECTOR3 position; // World position
        D3DXVECTOR3 velocity;
        float life;
        float angle; // in radians
    };

    std::vector<Drop2D> m_drops2D;
    std::vector<Drop3D> m_drops3D;
    std::vector<Drop3D> m_splatters3D;

    bool m_active{false};

    bool m_registered{false};

    struct RHWVertex
    {
        float x, y, z, rhw;
        DWORD color;
    };

    struct Vertex
    {
        float x, y, z, rhw;
        DWORD color;
        float u, v;
    };

    struct RainGroupSettings
    {
        int dropCount;
        float dropSize;
        float speed;
        float windSway;
        bool alphaBlended;
    };

    D3DXVECTOR3 camPos;
    float m_cameraY = 0.0f; // updated each frame based on estimated or real camera Y

    Drop3D RespawnDrop(const RainGroupSettings& settings, float camY);
    const RainGroupSettings* ChooseGroupByY(float y);

    siv::PerlinNoise noise{std::random_device{}};
    RainGroupSettings m_rainSettings[3] = {};

    void ScaleSettingsForIntensity(float intensity);
    bool IsCreatedRainTexture();
    void Render2DRainOverlay(const D3DVIEWPORT9& viewport);
    void Render3DRainOverlay(const D3DVIEWPORT9& viewport);
    void Render3DSplattersOverlay(const D3DVIEWPORT9& viewport);
};
