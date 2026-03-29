#include "npch.h"
#include "Nalta/Core/Paths.h"

#ifdef N_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace Nalta
{
    std::filesystem::path Paths::ToRelative(const std::filesystem::path& aAbsolute)
    {
        const auto relative{ std::filesystem::relative(aAbsolute, RootDir()) };
        std::string str{ relative.string() };
        std::ranges::replace(str, '\\', '/');
        return str;
    }

    std::filesystem::path Paths::ToAbsolute(const std::filesystem::path& aRelative)
    {
        const auto absolute{ RootDir() / aRelative };
        std::string str{ absolute.string() };
        std::ranges::replace(str, '\\', '/');
        return str;
    }

    std::filesystem::path Paths::RootDir()
    {
        return { N_ROOT_DIR };
    }

    std::filesystem::path Paths::EngineAssetDir()
    {
        return RootDir() / "Engine" / "Assets";
    }

    std::filesystem::path Paths::ExeDir()
    {
#ifdef N_PLATFORM_WINDOWS
        wchar_t exePath[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        return std::filesystem::path(exePath).parent_path();
#else
        return std::filesystem::current_path();
#endif
    }

    std::filesystem::path Paths::CookedDir()
    {
        return ExeDir() / "Cooked";
    }
}
