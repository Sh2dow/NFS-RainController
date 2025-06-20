#pragma once
#include "features.h"
#include "core.h"

class ForcePrecipitation : public ngg::common::Feature
{
public:
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
        float x{0.0f};
        float y{0.0f};
        float speed{0.0f};
    };

    std::vector<Drop> m_drops;
    bool m_active{false};

    void Update(IDirect3DDevice9* device);

    struct PrecipitationData
    {
        bool active{false};
        float rainPercent{1.0f};
        float fogPercent{0.5f};
    } m_precip{};

    bool m_registered{false};
};
