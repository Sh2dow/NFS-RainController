#pragma once
#include "features.h"
#include "core.h"
#include "PerlinNoise.h"

class PrecipitationController : public ngg::common::Feature
{
public:
    IDirect3DDevice9* m_device; // Add to class
    D3DXVECTOR3 camPos = D3DXVECTOR3(0, 0, 0);

    inline static DWORD m_lastTime;

    // Simplified view node
    struct EViewNode
    {
        EViewNode* prev; // 0x00
        EViewNode* next; // 0x04
        char padding[0x40]; // Up to 0x40
        D3DXMATRIX viewMatrix; // 0x40
    };

    LPDIRECT3DTEXTURE9 m_rainTex{nullptr};
    LPDIRECT3DTEXTURE9 m_splatterTex{nullptr};
    inline static D3DFORMAT chosenFormat;

    static PrecipitationController* Get()
    {
        static PrecipitationController instance;
        return &instance;
    }

    const char* name() const override { return "PrecipitationController"; }

    EViewNode** g_EVIEW_LIST_PTR;
    void DebugEVIEWListPtr();
    D3DXVECTOR3 GetCameraPositionSafe();
    bool IsCameraCovered(const D3DXVECTOR3& camPos);

    void enable() override;
    void disable() override;

    bool IsCreatedRainTexture();
    void Update();
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
    std::vector<Drop3D> nearDrops, midDrops, farDrops;

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

    float m_cameraY = 0.0f; // updated each frame based on estimated or real camera Y

    Drop3D RespawnDrop(const RainGroupSettings& settings, float camY);
    const RainGroupSettings* ChooseGroupByY(float y);

    siv::PerlinNoise noise{std::random_device{}};
    RainGroupSettings m_rainSettings[3] = {};

    void ScaleSettingsForIntensity(float intensity);
    void Render2DRainOverlay(const D3DVIEWPORT9& viewport);
    void Render3DRainOverlay(const D3DVIEWPORT9& viewport);
    void Render3DSplattersOverlay(const D3DVIEWPORT9& viewport);
};
