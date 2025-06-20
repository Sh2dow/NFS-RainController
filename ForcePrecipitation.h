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

    void PatchRainSettings(DWORD rainEnable);
    const char* name() const override { return "ForcePrecipitation"; }

    void enable() override;
    void disable() override;

private:
    struct Drop
    {
        D3DXVECTOR3 position;  // World position
        D3DXVECTOR3 velocity;
        float length;
    };

    std::vector<Drop> m_drops;
    bool m_active{false};

    
    struct Vertex
    {
        float x, y, z, rhw;
        float u, v;
        DWORD color;
    };


    void Update(IDirect3DDevice9* device);

    struct PrecipitationData
    {
        bool active{false};
        float rainPercent{2.0f};
        float fogPercent{0.5f};
    } m_precip{};

    bool m_registered{false};
};
