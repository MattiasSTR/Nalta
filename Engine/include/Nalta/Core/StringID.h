#pragma once
#include <cstdint>
#include <string>

namespace Nalta
{
    class StringID
    {
    public:
        StringID() = default;
        explicit StringID(const char* aString);
        explicit StringID(const std::string& aString);

        [[nodiscard]] uint64_t GetHash() const { return myHash; }
        [[nodiscard]] const std::string& GetString() const;
        [[nodiscard]] bool IsValid() const { return myHash != 0; }

        bool operator==(const StringID& aOther) const { return myHash == aOther.myHash; }
        bool operator!=(const StringID& aOther) const { return myHash != aOther.myHash; }
        bool operator<(const StringID& aOther) const { return myHash < aOther.myHash; }

    private:
        static uint64_t Hash(const std::string& aString);
        static const std::string* Intern(const std::string& aString, uint64_t aHash);

        uint64_t myHash  { 0 };

#ifndef N_SHIPPING
        const std::string* myString{ nullptr };
#endif
    };
}

template<>
struct std::hash<Nalta::StringID>
{
    size_t operator()(const Nalta::StringID& aID) const noexcept
    {
        return static_cast<size_t>(aID.GetHash());
    }
};