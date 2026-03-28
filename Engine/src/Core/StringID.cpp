#include "npch.h"
#include "Nalta/Core/StringID.h"

#include <mutex>
#include <unordered_map>

namespace Nalta
{
    namespace
    {
        uint64_t FNV1a64(const std::string& aStr)
        {
            constexpr uint64_t FNV_OFFSET{ 14695981039346656037ULL };
            constexpr uint64_t FNV_PRIME{ 1099511628211ULL };

            uint64_t hash{ FNV_OFFSET };
            for (const char c : aStr)
            {
                hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
                hash *= FNV_PRIME;
            }
            return hash;
        }
        
        class StringTable
        {
        public:
            static StringTable& Get()
            {
                static StringTable instance;
                return instance;
            }
            
            const std::string* Intern(const std::string& aString, const uint64_t aHash)
            {
                std::lock_guard lock{ myMutex };

                const auto it{ myTable.find(aHash) };
                if (it != myTable.end())
                {
                    return &it->second;
                }

                auto [inserted, _]{ myTable.emplace(aHash, aString) };
                return &inserted->second;
            }
            
        private:
            StringTable() = default;

            std::mutex myMutex;
            std::unordered_map<uint64_t, std::string> myTable;
        };
    }
    
    StringID::StringID(const char* aString)
        : StringID(std::string(aString))
    {}
    
    StringID::StringID(const std::string& aString)
        : myHash(Hash(aString))
    {
#ifndef N_SHIPPING
        myString = Intern(aString, myHash);
#else
        Intern(aString, myHash); // still intern in non-shipping for consistency
#endif
    }
    
    const std::string& StringID::GetString() const
    {
        NL_SCOPE_CORE("StringID");
        
#ifndef N_SHIPPING
        N_CORE_ASSERT(myString, "null string pointer");
        return *myString;
#else
        static const std::string empty{};
        return empty;
#endif
    }
    
    uint64_t StringID::Hash(const std::string& aString)
    {
        return FNV1a64(aString);
    }
    
    const std::string* StringID::Intern(const std::string& aString, const uint64_t aHash)
    {
        return StringTable::Get().Intern(aString, aHash);
    }
}