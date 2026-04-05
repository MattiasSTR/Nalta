#pragma once
#include "Nalta/Assets/AssetState.h"
#include "Nalta/Util/SlotMap.h"

#include <mutex>
#include <unordered_map>

namespace Nalta
{
    template<typename TKey, typename TAsset>
    class AssetStore
    {
    public:
        // Insert a new asset slot for this hash, or return the existing key.
        // Returns {key, true} if newly inserted, {key, false} if already present.
        std::pair<TKey, bool> GetOrInsert(uint64_t aHash)
        {
            std::lock_guard lock{ myMutex };
            if (const auto it{ myIndex.find(aHash) }; it != myIndex.end())
            {
                return { it->second, false };
            }

            TKey key{ mySlots.Insert(TAsset{}) };
            myIndex[aHash] = key;
            return { key, true };
        }

        TKey GetKey(uint64_t aHash) const
        {
            std::lock_guard lock{ myMutex };
            const auto it{ myIndex.find(aHash) };
            N_CORE_ASSERT(it != myIndex.end(), "no asset key for hash");
            return it->second;
        }
        
        void SetFallback(const TAsset* aFallback)
        {
            myFallback = aFallback;
        }

        void SetState(TKey aKey, AssetState aState)
        {
            std::lock_guard lock{ myMutex };
            if (TAsset* asset{ mySlots.Get(aKey) })
            {
                asset->state = aState;
            }
        }

        AssetState GetState(TKey aKey) const
        {
            std::lock_guard lock{ myMutex };
            const TAsset* asset{ mySlots.Get(aKey) };
            return asset ? asset->state : AssetState::Failed;
        }

        // Runs fn(TAsset&) under the mutex. Use for writes.
        template<typename Fn>
        void Modify(TKey aKey, Fn&& aFn)
        {
            std::lock_guard lock{ myMutex };
            if (TAsset* asset{ mySlots.Get(aKey) })
            {
                std::forward<Fn>(aFn)(*asset);
            }
        }
        
        template<typename Fn>
        void Peek(TKey aKey, Fn&& aFn) const
        {
            std::lock_guard lock{ myMutex };
            if (const TAsset* asset{ mySlots.Get(aKey) })
            {
                std::forward<Fn>(aFn)(*asset);
            }
        }

        // Returns a pointer valid only while you hold the mutex externally.
        // Prefer GetReady() for read access from the game thread.
        const TAsset* GetReady(TKey aKey) const
        {
            N_CORE_ASSERT(myFallback, "fallback not set");
            std::lock_guard lock{ myMutex };
            const TAsset* asset{ mySlots.Get(aKey) };
            return (asset && asset->state == AssetState::Ready) ? asset : myFallback;
        }
        
        bool Contains(uint64_t aHash) const
        {
            std::lock_guard lock{ myMutex };
            return myIndex.contains(aHash);
        }

        template<typename Fn>
        void ForEach(Fn&& aFn)
        {
            std::lock_guard lock{ myMutex };
            mySlots.ForEach(std::forward<Fn>(aFn));
        }

        void Clear()
        {
            std::lock_guard lock{ myMutex };
            mySlots = {};
            myIndex.clear();
        }

    private:
        mutable std::mutex myMutex;
        SlotMap<TKey, TAsset> mySlots;
        std::unordered_map<uint64_t, TKey> myIndex;
        const TAsset* myFallback{ nullptr };
    };
}