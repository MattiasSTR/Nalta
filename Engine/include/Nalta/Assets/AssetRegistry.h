#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace Nalta
{
    struct AssetRegistryEntry
    {
        std::string sourcePath;
        std::string cookedFile;   // filename only, e.g. "a3f8c2d1.nasset"
        std::string assetType;    // "Mesh", "Texture" etc.
        std::filesystem::file_time_type lastModified;
        std::vector<std::string> dependencies; // absolute paths this asset depends on
    };

    class AssetRegistry
    {
    public:
        AssetRegistry()  = default;
        ~AssetRegistry() = default;

        void Initialize(const std::filesystem::path& aRegistryPath);
        void Shutdown();

        // Find entry by source path - returns nullptr if not found
        [[nodiscard]] const AssetRegistryEntry* Lookup(const std::string& aSourcePath) const;

        // Add or update entry
        void Register(const AssetRegistryEntry& aEntry);

        // Remove entry
        void Unregister(const std::string& aSourcePath);
        
        [[nodiscard]] std::vector<std::string> FindDependents(const std::string& aSourcePath) const;

        // Check if source file has changed since last cook
        [[nodiscard]] bool NeedsRecook(const std::string& aSourcePath) const;

        // Persist to disk
        void Save() const;

        [[nodiscard]] bool IsLoaded() const { return myLoaded; }

    private:
        void Load();

        std::filesystem::path myRegistryPath;
        std::unordered_map<std::string, AssetRegistryEntry> myEntries;
        bool myLoaded{ false };
    };
}