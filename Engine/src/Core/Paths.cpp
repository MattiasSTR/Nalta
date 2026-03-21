#include "npch.h"
#include "Nalta/Core/Paths.h"

namespace Nalta
{
    std::filesystem::path Paths::RootDir()
    {
        return { N_ROOT_DIR };
    }
}