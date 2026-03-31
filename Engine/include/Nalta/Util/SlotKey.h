#pragma once
#include "Nalta/Core/Assert.h"

#include <cstdint>

namespace Nalta
{
    class SlotKey
    {
    public:
        using RawType = uint32_t;

        constexpr SlotKey() : myRaw(INVALID_RAW) {}

        [[nodiscard]] constexpr bool IsValid() const { return myRaw != INVALID_RAW; }
        [[nodiscard]] constexpr RawType GetRaw() const { return myRaw; }

        constexpr bool operator==(const SlotKey&) const = default;

    protected:
        explicit constexpr SlotKey(const RawType aRaw) : myRaw(aRaw) {}

    private:
        static constexpr RawType GENERATION_BITS{ 8u };
        static constexpr RawType INDEX_BITS{ sizeof(RawType) * 8u - GENERATION_BITS };
        static constexpr RawType INDEX_MASK{ (RawType{1} << INDEX_BITS) - 1u };
        static constexpr RawType GENERATION_MASK{ (RawType{1} << GENERATION_BITS) - 1u };
        static constexpr RawType INVALID_RAW{ RawType(-1) };

        static_assert(INDEX_BITS + GENERATION_BITS == sizeof(RawType) * 8u);

        RawType myRaw;

        template<typename TKey, typename TValue> friend class SlotMap;

        [[nodiscard]] constexpr RawType GetIndex() const
        {
            const RawType index{ myRaw & INDEX_MASK };
            N_CORE_ASSERT(index != INDEX_MASK, "SlotKey index is at the reserved sentinel value - key is likely uninitialized or corrupted");
            return index;
        }

        [[nodiscard]] constexpr RawType GetGeneration() const
        {
            return (myRaw >> INDEX_BITS) & GENERATION_MASK;
        }

        [[nodiscard]] constexpr SlotKey BumpGeneration() const
        {
            const RawType nextGen{ GetGeneration() + 1u };
            N_CORE_ASSERT(nextGen < (RawType{1} << GENERATION_BITS), "SlotKey generation overflow - slot has been recycled too many times");
            return SlotKey{ GetIndex() | (nextGen << INDEX_BITS) };
        }

        [[nodiscard]] static constexpr SlotKey Make(const RawType aIndex, const RawType aGeneration = 0u)
        {
            N_CORE_ASSERT(aIndex < INDEX_MASK, "SlotKey index out of range - exceeds maximum addressable slots");
            N_CORE_ASSERT(aGeneration <= GENERATION_MASK, "SlotKey generation out of range - value exceeds generation bit width");
            return SlotKey{ aIndex | (aGeneration << INDEX_BITS) };
        }
    };
}

template<typename TKey>
    requires std::is_base_of_v<Nalta::SlotKey, TKey>
struct std::hash<TKey>
{
    size_t operator()(const TKey& aKey) const noexcept
    {
        return std::hash<Nalta::SlotKey::RawType>{}(aKey.GetRaw());
    }
};