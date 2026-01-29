#pragma once

namespace ngg::common
{
    class Feature
    {
    public:
        virtual ~Feature() = default;
        virtual const char* name() const = 0;
        virtual void enable() = 0;
        virtual void disable() = 0;
    };
}
