#include "npch.h"
#include "Nalta/Assets/AssetRegistry.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Nalta
{
    void AssetRegistry::Initialize(const std::filesystem::path& aRegistryPath)
    {
        NL_SCOPE_CORE("AssetRegistry");
        myRegistryPath = aRegistryPath;
        Load();
        NL_INFO(GCoreLogger, "initialized ({} entries)", myEntries.size());
    }

    void AssetRegistry::Shutdown()
    {
        NL_SCOPE_CORE("AssetRegistry");
        Save();
        myEntries.clear();
        myLoaded = false;
        NL_INFO(GCoreLogger, "shutdown");
    }
    
    const AssetRegistryEntry* AssetRegistry::Lookup(const std::string& aSourcePath) const
    {
        const auto it{ myEntries.find(aSourcePath) };
        return it != myEntries.end() ? &it->second : nullptr;
    }

    void AssetRegistry::Register(const AssetRegistryEntry& aEntry)
    {
        myEntries[aEntry.sourcePath] = aEntry;
    }

    void AssetRegistry::Unregister(const std::string& aSourcePath)
    {
        myEntries.erase(aSourcePath);
    }

    std::vector<std::string> AssetRegistry::FindDependents(const std::string& aSourcePath) const
    {
        std::vector<std::string> dependents;
        for (const auto& [path, entry] : myEntries)
        {
            for (const auto& dep : entry.dependencies)
            {
                NL_TRACE(GCoreLogger, "comparing dep '{}' vs '{}'", dep, aSourcePath);
                if (dep == aSourcePath)
                    dependents.push_back(path);
            }
        }
        return dependents;
    }

    bool AssetRegistry::NeedsRecook(const std::string& aSourcePath) const
    {
        const auto* entry{ Lookup(aSourcePath) };
        if (entry == nullptr) // not in registry — needs cook
        {
            return true;
        } 

        const std::filesystem::path sourcePath{ aSourcePath };
        if (!std::filesystem::exists(sourcePath)) // source gone — use cooked
        {
            return false;
        }

        const auto currentModified{ std::filesystem::last_write_time(sourcePath) };
        return currentModified > entry->lastModified;
    }
    
    void AssetRegistry::Save() const
    {
#ifndef N_SHIPPING
        NL_SCOPE_CORE("AssetRegistry");

        json root;
        json entriesArray = json::array();

        for (const auto& entry : myEntries | std::views::values)
        {
            json entryJson;
            entryJson["sourcePath"] = Paths::ToRelative(entry.sourcePath).string();
            entryJson["cookedFile"] = entry.cookedFile;
            entryJson["assetType"]  = entry.assetType;

            // Store last modified as duration since epoch
            const auto duration{ entry.lastModified.time_since_epoch() };
            entryJson["lastModified"] = duration.count();
            
            json depsArray = json::array();
            for (const auto& dep : entry.dependencies)
            {
                depsArray.push_back(Paths::ToRelative(dep).string());
            }
            entryJson["dependencies"] = depsArray;

            entriesArray.push_back(entryJson);
        }
        
        root["entries"] = entriesArray;
        root["version"] = 1;

        std::filesystem::create_directories(myRegistryPath.parent_path());
        std::ofstream file{ myRegistryPath };
        if (!file.is_open())
        {
            NL_ERROR(GCoreLogger, "failed to save registry to '{}'", myRegistryPath.string());
            return;
        }

        file << root.dump(4); // pretty print with 4 space indent
        NL_INFO(GCoreLogger, "saved {} entries to '{}'", myEntries.size(), myRegistryPath.string());
#endif
    }
    
    void AssetRegistry::Load()
    {
#ifndef N_SHIPPING
        if (!std::filesystem::exists(myRegistryPath))
        {
            NL_TRACE(GCoreLogger, "no registry found at '{}' — starting fresh", myRegistryPath.string());
            myLoaded = true;
            return;
        }

        std::ifstream file{ myRegistryPath };
        if (!file.is_open())
        {
            NL_WARN(GCoreLogger, "failed to open registry '{}'", myRegistryPath.string());
            myLoaded = true;
            return;
        }

        try
        {
            json root{ json::parse(file) };

            for (const auto& entryJson : root["entries"])
            {
                AssetRegistryEntry entry;
                entry.sourcePath = Paths::ToAbsolute(std::filesystem::path(entryJson["sourcePath"].get<std::string>())).string();
                entry.cookedFile = entryJson["cookedFile"].get<std::string>();
                entry.assetType  = entryJson["assetType"].get<std::string>();

                const auto count{ entryJson["lastModified"].get<int64_t>() };
                entry.lastModified = std::filesystem::file_time_type(std::filesystem::file_time_type::duration(count));
                
                if (entryJson.contains("dependencies"))
                {
                    for (const auto& dep : entryJson["dependencies"])
                    {
                        auto absPath{ Paths::ToAbsolute(dep.get<std::string>()) };
                        std::error_code ec;
                        auto canonical{ std::filesystem::weakly_canonical(absPath, ec) };
                        std::string normalized{ ec ? absPath.string() : canonical.string() };
                        std::ranges::replace(normalized, '\\', '/');
                        entry.dependencies.push_back(normalized);
                    }
                }

                myEntries[entry.sourcePath] = entry;
            }
        }
        catch (const json::exception& e)
        {
            NL_ERROR(GCoreLogger, "failed to parse registry: {}", e.what());
        }

        myLoaded = true;
#endif
    }
}