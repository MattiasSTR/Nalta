#pragma once
#include "Nalta/Platform/IFileWatcher.h"
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <chrono>
#include <unordered_map>

namespace Nalta
{
    class Win32FileWatcher final : public IFileWatcher
    {
    public:
        Win32FileWatcher();
        ~Win32FileWatcher() override;

        void Initialize() override;
        void Shutdown() override;
        void Watch(const std::filesystem::path& aDirectory) override;
        void AddOnChangedCallback(OnFileChangedCallback aCallback) override;

    private:
        void WatchThread();
        void ProcessChanges(const std::filesystem::path& aDirectory, void* aBuffer);
        void FlushDebounced();

        struct WatchedDirectory
        {
            std::filesystem::path path;
            void* handle{ nullptr }; // HANDLE
        };

        struct PendingChange
        {
            std::filesystem::path path;
            std::chrono::steady_clock::time_point lastSeen;
        };

        std::vector<OnFileChangedCallback> myCallbacks;
        std::vector<WatchedDirectory>  myDirectories;
        std::unordered_map<std::string, PendingChange> myPendingChanges;

        std::mutex myMutex;
        std::thread myThread;
        std::atomic<bool> myStop{ false };
        void* myWakeEvent{ nullptr }; // HANDLE - wakes thread on new directory
    };
}