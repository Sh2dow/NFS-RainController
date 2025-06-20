#pragma once

#if _DEBUG
#include "Log.h"
#define OutputDebugStringA(...) asi_log::Log(__VA_ARGS__)
#endif

namespace ngg
{
    namespace common
    {
        class Feature
        {
        public:
            virtual ~Feature() = default;
            virtual const char* name() const = 0;

            virtual void enable()
            {
            }

            virtual void disable()
            {
            }
        };
    }
}

namespace ngg
{
    namespace common
    {
        namespace features
        {
        }
    }
}
