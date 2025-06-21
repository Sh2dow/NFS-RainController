#pragma once

class helpers
{
public:
};

template<typename T>
const char* ConvertToCString(T value)
{
    static_assert(std::is_arithmetic<T>::value, "ConvertToCString requires an arithmetic type");

    static thread_local char buffer[32]; // thread-safe and persistent per thread

    if constexpr (std::is_integral<T>::value)
        std::snprintf(buffer, sizeof(buffer), "%d", static_cast<int>(value));
    else if constexpr (std::is_floating_point<T>::value)
        std::snprintf(buffer, sizeof(buffer), "%.3f", static_cast<float>(value)); // adjust precision

    return buffer;
}
