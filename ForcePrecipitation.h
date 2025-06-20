#pragma once
#include "features.h"
#include "core.h"

class ForcePrecipitation : public ngg::common::Feature
{
public:
    LPDIRECT3DTEXTURE9 m_rainTex{nullptr};

    static ForcePrecipitation* Get()
    {
        static ForcePrecipitation instance;
        return &instance;
    }

    const char* name() const override { return "ForcePrecipitation"; }

    void enable() override;
    void disable() override;

    void Update(IDirect3DDevice9* device);
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

    struct PrecipitationData
    {
        bool active{false};
        float rainPercent{1.0f};
        float fogPercent{0.5f};
    } m_precip{};

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

    void RenderRainDrop(IDirect3DDevice9* device, const Drop& drop,
                        const D3DVIEWPORT9& viewport,
                        const D3DXMATRIX& matProj, const D3DXMATRIX& matView,
                        const D3DXVECTOR3& camPos, BYTE alpha);
};
