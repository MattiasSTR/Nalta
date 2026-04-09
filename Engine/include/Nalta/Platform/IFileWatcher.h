#pragma once
#include <filesystem>
#include <functional>
#include <string>

namespace Nalta
{
    using OnFileChangedCallback = std::function<void(const std::filesystem::path&)>;

    class IFileWatcher
    {
    public:
        virtual ~IFileWatcher() = default;

        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;

        virtual void Watch(const std::filesystem::path& aDirectory) = 0;
        virtual void AddOnChangedCallback(OnFileChangedCallback aCallback) = 0;
    };
}