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

    void enable() override;
    void disable() override;

    void Update(IDirect3DDevice9* device);
    void Update2(IDirect3DDevice9* device);
    bool IsActive() const;

private:
    size_t m_callbackId = static_cast<size_t>(-1);

    struct Drop
    {
        D3DXVECTOR3 position; // World position
        D3DXVECTOR3 velocity;
        float length;
    };

    std::vector<Drop> m_drops;
    bool m_active{false};

    bool m_registered{false};

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

    Drop RespawnDrop(const RainGroupSettings& settings, float camY);
    const RainGroupSettings* ChooseGroupByY(float y);
    
    RainGroupSettings m_rainSettings[3] = {
        // near
        {20, 2.0f, 1.5f, 0.2f, true},
        // mid
        {40, 3.5f, 2.0f, 0.3f, false},
        // far
        {60, 1.5f, 2.5f, 0.5f, false}
    };
    
    void ScaleSettingsForIntensity(float intensity);
    bool IsCreatedRainTexture(IDirect3DDevice9* device);
    void Render2DRainOverlay(IDirect3DDevice9* m_device, const D3DVIEWPORT9& viewport);
    void Render3DRainOverlay(IDirect3DDevice9* m_device, const D3DVIEWPORT9& viewport);
};
