#pragma once
#include "features.h"
#include "core.h"

class PrecipitationController : public ngg::common::Feature
{
public:
    LPDIRECT3DTEXTURE9 m_rainTex{nullptr};

    static PrecipitationController* Get()
    {
        static PrecipitationController instance;
        return &instance;
    }

    const char* name() const override { return "PrecipitationController"; }

    D3DXVECTOR3 GetCameraPositionSafe();
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

    struct Drop2D : Drop
    {
        float speed = 0.0f;
        float x = 0.0f;
        float y = 0.0f;
    };

    struct Drop3D : Drop
    {
        D3DXVECTOR3 position; // World position
        D3DXVECTOR3 velocity;
    };

    std::vector<Drop2D> m_drops2D;
    std::vector<Drop3D> m_drops3D;
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

    RainGroupSettings m_rainSettings[3] = {};

    void ScaleSettingsForIntensity(float intensity);
    bool IsCreatedRainTexture();
    void Render2DRainOverlay(const D3DVIEWPORT9& viewport);
    void Render3DRainOverlay(const D3DVIEWPORT9& viewport);
    void Render3DSplattersOverlay(const D3DVIEWPORT9& viewport);
};
