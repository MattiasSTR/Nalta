#pragma once
#include <filesystem>

namespace Nalta
{
    class Paths
    {
    public:
        [[nodiscard]] static std::filesystem::path RootDir();
        [[nodiscard]] static std::filesystem::path EngineAssetDir();
        [[nodiscard]] static std::filesystem::path ExeDir();
        [[nodiscard]] static std::filesystem::path CookedDir();
    };
}