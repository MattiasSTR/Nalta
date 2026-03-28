#pragma once
#include "Nalta/Core/StringID.h"

#include <filesystem>

namespace Nalta
{
    class AssetPath
    {
    public:
        AssetPath() = default;
        explicit AssetPath(const char* aPath) : myID(aPath) {}
        explicit AssetPath(const std::string& aPath) : myID(aPath) {}
        explicit AssetPath(const std::filesystem::path& aPath) : myID(aPath.string()) {}

        [[nodiscard]] StringID GetID() const { return myID; }
        [[nodiscard]] const std::string& GetPath() const { return myID.GetString(); }
        [[nodiscard]] uint64_t GetHash() const { return myID.GetHash(); }
        [[nodiscard]] std::string GetExtension() const { return std::filesystem::path(GetPath()).extension().string(); }
        [[nodiscard]] std::string GetStem() const { return std::filesystem::path(GetPath()).stem().string(); }
        [[nodiscard]] bool IsEmpty() const { return !myID.IsValid(); }

        bool operator==(const AssetPath& aOther) const { return myID == aOther.myID; }
        bool operator!=(const AssetPath& aOther) const { return myID != aOther.myID; }

    private:
        StringID myID;
    };
}

template<>
struct std::hash<Nalta::AssetPath>
{
    size_t operator()(const Nalta::AssetPath& aPath) const noexcept
    {
        return static_cast<size_t>(aPath.GetHash());
    }
};