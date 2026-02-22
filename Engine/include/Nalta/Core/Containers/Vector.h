#pragma once
#include "Nalta/Core/Config.h"

#if NALTA_USE_STD_VECTOR
#include <vector>
namespace Nalta
{
    template <typename T>
    using Vector = std::vector<T>;
}
#else

#endif
