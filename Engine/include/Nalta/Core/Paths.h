#pragma once
#include <filesystem>

namespace Nalta
{
    class Paths
    {
    public:
        [[nodiscard]] static std::filesystem::path ToRelative(const std::filesystem::path& aAbsolute);
        [[nodiscard]] static std::filesystem::path ToAbsolute(const std::filesystem::path& aRelative);
        
        [[nodiscard]] static std::filesystem::path RootDir();
        [[nodiscard]] static std::filesystem::path EngineAssetDir();
        [[nodiscard]] static std::filesystem::path ExeDir();
        [[nodiscard]] static std::filesystem::path CookedDir();
    };
}