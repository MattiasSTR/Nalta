#pragma once
#include "Nalta/Core/Config.h"

#if NALTA_USE_STD_ATOMIC
#include <atomic>
namespace Nalta
{
    template <typename T>
    using Atomic = std::atomic<T>;
}
#else

#endif
