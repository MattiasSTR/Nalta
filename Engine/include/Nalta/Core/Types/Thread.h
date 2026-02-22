#pragma once
#include "Nalta/Core/Config.h"

#if NALTA_USE_STD_THREAD
#include <thread>
namespace Nalta
{
    using Thread = std::thread;
}
#else

#endif
