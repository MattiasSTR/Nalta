#pragma once
#include "Nalta/Core/Assert.h"

#include <cstdint>
#include <vector>

namespace Nalta
{
    class DynamicBitset
    {
    public:
        void PushBack(const bool aValue)
        {
            if (mySize % BITS_PER_WORD == 0)
            {
                myWords.push_back(0u);
            }

            ++mySize;
            Set(mySize - 1u, aValue);
        }
        
        void Set(const uint32_t aIndex, const bool aValue)
        {
            N_CORE_ASSERT(aIndex < mySize, "DynamicBitset index out of range");
            if (aValue)
            {
                myWords[WordIndex(aIndex)] |= BitMask(aIndex);
            }
            else
            {
                myWords[WordIndex(aIndex)] &= ~BitMask(aIndex);
            }
        }
        
        [[nodiscard]] bool Get(const uint32_t aIndex) const
        {
            N_CORE_ASSERT(aIndex < mySize, "DynamicBitset index out of range");
            return (myWords[WordIndex(aIndex)] & BitMask(aIndex)) != 0u;
        }
        
        void Clear()
        {
            for (uint32_t& word : myWords)
            {
                word = 0u;
            }
        }
        
        void Reset()
        {
            myWords.clear();
            mySize = 0u;
        }
        
        // Returns the index of the first set bit, or mySize if none found
        [[nodiscard]] uint32_t FindFirstSet() const
        {
            for (uint32_t w{ 0 }; w < static_cast<uint32_t>(myWords.size()); ++w)
            {
                if (myWords[w] == 0u)
                {
                    continue;
                }
                return w * BITS_PER_WORD + CountTrailingZeros(myWords[w]);
            }
            return mySize;
        }
        
        // Returns the index of the first unset bit, or mySize if none found
        [[nodiscard]] uint32_t FindFirstUnset() const
        {
            for (uint32_t w{ 0 }; w < static_cast<uint32_t>(myWords.size()); ++w)
            {
                if (myWords[w] == FULL_WORD)
                {
                    continue;
                }
                return w * BITS_PER_WORD + CountTrailingZeros(~myWords[w]);
            }
            return mySize;
        }
        
        [[nodiscard]] uint32_t Count() const
        {
            uint32_t count{ 0u };
            for (const uint32_t word : myWords)
            {
                count += PopCount(word);
            }
            return count;
        }
        
        [[nodiscard]] uint32_t Size() const { return mySize; }
        [[nodiscard]] bool IsEmpty() const { return mySize == 0u; }
        
    private:
        static constexpr uint32_t BITS_PER_WORD{ sizeof(uint32_t) * 8u };
        static constexpr uint32_t FULL_WORD{ ~uint32_t{0} };
        
        [[nodiscard]] static constexpr uint32_t WordIndex(const uint32_t aIndex) { return aIndex / BITS_PER_WORD; }
        [[nodiscard]] static constexpr uint32_t BitMask(const uint32_t aIndex) { return 1u << (aIndex % BITS_PER_WORD); }

        [[nodiscard]] static uint32_t CountTrailingZeros(uint32_t aWord)
        {
            N_CORE_ASSERT(aWord != 0u, "CountTrailingZeros called with zero — no set bit to find");
#ifdef _MSC_VER
            unsigned long index{ 0 };
            _BitScanForward(&index, aWord);
            return static_cast<uint32_t>(index);
#else
            return static_cast<uint32_t>(__builtin_ctz(aWord));
#endif
        }
        
        [[nodiscard]] static uint32_t PopCount(uint32_t aWord)
        {
#ifdef _MSC_VER
            return __popcnt(aWord);
#else
            return static_cast<uint32_t>(__builtin_popcount(aWord));
#endif
        }
        
        std::vector<uint32_t> myWords{};
        uint32_t mySize{ 0u };
    };
}