#pragma once
#include "Nalta/Util/DynamicBitset.h"
#include "Nalta/Util/SlotKey.h"

#include <vector>

namespace Nalta
{
    template<typename TKey, typename TValue>
    class SlotMap
    {
        static_assert(std::is_base_of_v<SlotKey, TKey>, "TKey must derive from SlotKey");

    public:
        [[nodiscard]] TKey Insert(TValue aValue)
        {
            uint32_t index{};

            if (!myFreeList.empty())
            {
                index = myFreeList.back();
                myFreeList.pop_back();
                myValues[index] = std::move(aValue);
                myOccupied.Set(index, true);
            }
            else
            {
                index = static_cast<uint32_t>(myValues.size());
                myValues.push_back(std::move(aValue));
                myGenerations.push_back(0u);
                myOccupied.PushBack(true);
            }

            return TKey{ SlotKey::Make(index, myGenerations[index]).myRaw };
        }
        
        [[nodiscard]] TValue* Get(TKey aKey)
        {
            if (!IsKeyValid(aKey))
            {
                return nullptr;
            }
            return &myValues[aKey.GetIndex()];
        }
        
        [[nodiscard]] const TValue* Get(TKey aKey) const
        {
            if (!IsKeyValid(aKey))
            {
                return nullptr;
            }
            return &myValues[aKey.GetIndex()];
        }
        
        // Returns a reference — asserts in debug if key is invalid or stale
        // Use when you are certain the key is valid and want to avoid pointer indirection at call sites
        [[nodiscard]] TValue& GetRef(TKey aKey)
        {
            N_CORE_ASSERT(IsKeyValid(aKey), "SlotMap::GetRef called with invalid or stale key");
            return myValues[aKey.GetIndex()];
        }
 
        [[nodiscard]] const TValue& GetRef(TKey aKey) const
        {
            N_CORE_ASSERT(IsKeyValid(aKey), "SlotMap::GetRef called with invalid or stale key");
            return myValues[aKey.GetIndex()];
        }
        
        void Remove(TKey aKey)
        {
            if (!IsKeyValid(aKey))
            {
                return;
            }

            const uint32_t index{ aKey.GetIndex() };
            myOccupied.Set(index, false);
            ++myGenerations[index];
            myFreeList.push_back(index);
        }
        
        [[nodiscard]] bool Contains(TKey aKey) const
        {
            return IsKeyValid(aKey);
        }

        [[nodiscard]] uint32_t Count() const
        {
            return static_cast<uint32_t>(myValues.size() - myFreeList.size());
        }
        
        template<typename TFunc>
        void ForEach(TFunc&& aFunc)
        {
            for (uint32_t i{ 0 }; i < static_cast<uint32_t>(myValues.size()); ++i)
            {
                if (myOccupied.Get(i))
                {
                    std::forward<TFunc>(aFunc)(myValues[i]);
                }
            }
        }

        template<typename TFunc>
        void ForEach(TFunc&& aFunc) const
        {
            for (uint32_t i{ 0 }; i < static_cast<uint32_t>(myValues.size()); ++i)
            {
                if (myOccupied.Get(i))
                {
                    std::forward<TFunc>(aFunc)(myValues[i]);
                }
            }
        }
        
        // Iterates all live values and passes the reconstructed key alongside the value.
        // Use when you need to identify which slot you're in
        template<typename TFunc>
        void ForEachWithKey(TFunc&& aFunc)
        {
            for (uint32_t i{ 0 }; i < static_cast<uint32_t>(myValues.size()); ++i)
            {
                if (myOccupied.Get(i))
                {
                    TKey key{ SlotKey::Make(i, myGenerations[i]).myRaw };
                    std::forward<TFunc>(aFunc)(key, myValues[i]);
                }
            }
        }
 
        template<typename TFunc>
        void ForEachWithKey(TFunc&& aFunc) const
        {
            for (uint32_t i{ 0 }; i < static_cast<uint32_t>(myValues.size()); ++i)
            {
                if (myOccupied.Get(i))
                {
                    TKey key{ SlotKey::Make(i, myGenerations[i]).myRaw };
                    std::forward<TFunc>(aFunc)(key, myValues[i]);
                }
            }
        }
        
    private:
        [[nodiscard]] bool IsKeyValid(TKey aKey) const
        {
            if (!aKey.IsValid())
            {
                return false;
            }

            const uint32_t index{ aKey.GetIndex() };
            if (index >= static_cast<uint32_t>(myValues.size()))
            {
                return false;
            }

            return myOccupied.Get(index) && myGenerations[index] == aKey.GetGeneration();
        }

        std::vector<TValue> myValues{};
        std::vector<uint32_t> myGenerations{};
        DynamicBitset myOccupied{};
        std::vector<uint32_t> myFreeList{};
    };
}
