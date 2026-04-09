#include "npch.h"
#include "Nalta/Platform/Win32/Win32FileWatcher.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace Nalta
{
    constexpr auto DEBOUNCE_DURATION{ std::chrono::milliseconds(300) };
    constexpr DWORD WATCH_FLAGS{FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME };
    
    Win32FileWatcher::Win32FileWatcher() = default;

    Win32FileWatcher::~Win32FileWatcher()
    {
        if (myWakeEvent != nullptr)
        {
            Shutdown();
        }
    }
    
    void Win32FileWatcher::Initialize()
    {
        NL_SCOPE_CORE("Win32FileWatcher");
        myStop = false;
        myWakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        myThread = std::thread(&Win32FileWatcher::WatchThread, this);
        NL_INFO(GCoreLogger, "initialized");
    }
    
    void Win32FileWatcher::Shutdown()
    {
        NL_SCOPE_CORE("Win32FileWatcher");
        myStop = true;
        
        myCallbacks.clear();

        if (myWakeEvent != nullptr)
        {
            SetEvent(static_cast<HANDLE>(myWakeEvent));
        }

        if (myThread.joinable())
        {
            myThread.join();
        }

        // Close directory handles
        {
            std::lock_guard lock{ myMutex };
            for (auto& dir : myDirectories)
            {
                if (dir.handle && dir.handle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(static_cast<HANDLE>(dir.handle));
                }
            }
            myDirectories.clear();
        }

        if (myWakeEvent != nullptr)
        {
            CloseHandle(static_cast<HANDLE>(myWakeEvent));
            myWakeEvent = nullptr;
        }

        NL_INFO(GCoreLogger, "shutdown");
    }
    
    void Win32FileWatcher::Watch(const std::filesystem::path& aDirectory)
    {
        NL_SCOPE_CORE("Win32FileWatcher");
        
        HANDLE handle{ CreateFileW(
            aDirectory.wstring().c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr) };
        
        if (handle == INVALID_HANDLE_VALUE)
        {
            NL_ERROR(GCoreLogger, "failed to open directory '{}'", aDirectory.string());
            return;
        }

        {
            std::lock_guard lock{ myMutex };
            myDirectories.push_back({ aDirectory, handle });
        }

        // Wake thread to pick up new directory
        if (myWakeEvent != nullptr)
        {
            SetEvent(static_cast<HANDLE>(myWakeEvent));
        }

        NL_INFO(GCoreLogger, "watching '{}'", aDirectory.string());
    }
    
    void Win32FileWatcher::AddOnChangedCallback(OnFileChangedCallback aCallback)
    {
        std::lock_guard lock{ myMutex };
        myCallbacks.push_back(std::move(aCallback));
    }
    
    void Win32FileWatcher::WatchThread()
    {
        NL_SCOPE_CORE("FileWatcher");
        NL_INFO(GCoreLogger, "watch thread started");

        const std::string threadName{ "FileWatcher" };
        const int size{ MultiByteToWideChar(CP_UTF8, 0, threadName.c_str(), -1, nullptr, 0) };
        std::wstring wide(size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, threadName.c_str(), -1, wide.data(), size);
        SetThreadDescription(GetCurrentThread(), wide.c_str());
        
        constexpr size_t BUFFER_SIZE{ 65536 };
        std::vector<uint8_t> buffer(BUFFER_SIZE);
        
        // Each directory needs its own OVERLAPPED
        struct DirContext
        {
            OVERLAPPED overlapped{};
            HANDLE handle{ INVALID_HANDLE_VALUE };
            std::filesystem::path path;
            std::vector<uint8_t> buffer;
            bool pending{ false };
        };

        std::vector<DirContext> contexts;
        
        auto refreshContexts = [&]()
        {
            std::lock_guard lock{ myMutex };
            // Add new directories
            for (const auto& dir : myDirectories)
            {
                const bool alreadyTracked{ std::ranges::any_of(contexts, [&](const DirContext& ctx)
                {
                    return ctx.handle == static_cast<HANDLE>(dir.handle);
                })};

                if (!alreadyTracked)
                {
                    DirContext ctx;
                    ctx.handle = static_cast<HANDLE>(dir.handle);
                    ctx.path   = dir.path;
                    ctx.buffer.resize(BUFFER_SIZE);
                    ctx.overlapped.hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
                    contexts.push_back(std::move(ctx));
                }
            }
        };
        
        auto issueRead = [&](DirContext& ctx)
        {
            if (ctx.pending)
            {
                return;
            }

            DWORD bytesReturned{ 0 };
            const BOOL result{ ReadDirectoryChangesW(
                ctx.handle,
                ctx.buffer.data(),
                static_cast<DWORD>(ctx.buffer.size()),
                TRUE, // watch subtree
                WATCH_FLAGS,
                &bytesReturned,
                &ctx.overlapped,
                nullptr) };

            if (result || GetLastError() == ERROR_IO_PENDING)
            {
                ctx.pending = true;
            }
        };

        refreshContexts();
        for (auto& ctx : contexts)
        {
            issueRead(ctx);
        }
        
        while (!myStop)
        {
            // Build wait list - one event per dir + wake event
            std::vector<HANDLE> waitHandles;
            waitHandles.push_back(static_cast<HANDLE>(myWakeEvent));
            for (const auto& ctx : contexts)
            {
                waitHandles.push_back(ctx.overlapped.hEvent);
            }
            
            const DWORD waitResult{ WaitForMultipleObjects(
                static_cast<DWORD>(waitHandles.size()),
                waitHandles.data(),
                FALSE,
                300) }; // 300ms timeout for debounce flush
            
            if (myStop)
            {
                break;
            }

            if (waitResult == WAIT_TIMEOUT)
            {
                // Flush debounced changes
                FlushDebounced();
                continue;
            }

            if (waitResult == WAIT_OBJECT_0)
            {
                // Wake event - new directory added
                refreshContexts();
                for (auto& ctx : contexts)
                {
                    issueRead(ctx);
                }
                continue;
            }
            
            // Directory change event
            const DWORD idx{ waitResult - WAIT_OBJECT_0 - 1 };
            if (idx < contexts.size())
            {
                auto& ctx{ contexts[idx] };
                ctx.pending = false;

                DWORD bytesTransferred{ 0 };
                if (GetOverlappedResult(ctx.handle, &ctx.overlapped, &bytesTransferred, FALSE) && bytesTransferred > 0)
                {
                    ProcessChanges(ctx.path, ctx.buffer.data());
                }

                issueRead(ctx);
                FlushDebounced();
            }
        }
        
        // Cleanup overlapped events
        for (auto& ctx : contexts)
        {
            if (ctx.overlapped.hEvent)
            {
                CloseHandle(ctx.overlapped.hEvent);
            }
        }

        NL_INFO(GCoreLogger, "watch thread stopped");
    }

    void Win32FileWatcher::ProcessChanges(const std::filesystem::path& aDirectory, void* aBuffer)
    {
        const auto* info{ static_cast<FILE_NOTIFY_INFORMATION*>(aBuffer) };

        while (true)
        {
            // Convert wide filename to path
            const std::wstring wideName(info->FileName, info->FileNameLength / sizeof(wchar_t));
            const auto changedPath{ aDirectory / wideName };

            // Normalize
            std::string pathStr{ changedPath.string() };
            std::ranges::replace(pathStr, '\\', '/');

            // Add to debounce map
            {
                std::lock_guard lock{ myMutex };
                myPendingChanges[pathStr] =
                {
                    .path = pathStr,
                    .lastSeen = std::chrono::steady_clock::now()
                };
            }

            if (info->NextEntryOffset == 0)
            {
                break;
            }
            info = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(reinterpret_cast<const uint8_t*>(info) + info->NextEntryOffset);
        }
    }

    void Win32FileWatcher::FlushDebounced()
    {
        const auto now{ std::chrono::steady_clock::now() };
        std::vector<OnFileChangedCallback> callbacks;
        std::vector<std::filesystem::path> toFire;

        {
            std::lock_guard lock{ myMutex };
            callbacks = myCallbacks;

            for (auto it{ myPendingChanges.begin() }; it != myPendingChanges.end();)
            {
                if (now - it->second.lastSeen >= DEBOUNCE_DURATION)
                {
                    toFire.push_back(it->second.path);
                    it = myPendingChanges.erase(it);
                }
                else { ++it; }
            }
        }

        for (const auto& path : toFire)
        {
            NL_TRACE(GCoreLogger, "changed '{}'", path.string());
            for (const auto& cb : callbacks)
            {
                cb(path);
            }
        }
    }
}
