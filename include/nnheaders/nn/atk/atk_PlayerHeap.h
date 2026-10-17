#pragma once
#include <nn/atk/atk_SoundHeap.h>
#include <nn/util/util_IntrusiveListBaseNodeTraits.h>

namespace nn::atk {
class SoundPlayer;
} // namespace nn::atk

namespace nn::atk::detail {
class BasicSound;
class PlayerHeap : public SoundMemoryAllocatable {
  public:
    PlayerHeap();
    ~PlayerHeap() override;
    bool Create(void* pMemory, size_t size);
    void Destroy();
    void Clear();
    void* Allocate(size_t size) override;
    void* Allocate(size_t size, DisposeCallback callback, void* pCallbackArg) override;
    size_t GetAllocateSize(size_t size, bool needMemoryPool) override;
    size_t GetFreeSize() const;
    /** @brief Marks completion of an asynchronous data load, whether successful or failed. */
    void SetLoadFinished() { mState = State::LoadFinished; }
    /**
     * @brief Checks whether the asynchronous data load into this heap has finished.
     * @return True once SetLoadFinished has been called.
     */
    bool IsLoadFinished() const { return mState == State::LoadFinished; }
    /**
     * @brief Records the sound player that owns this heap.
     * @param pPlayer Owning sound player.
     */
    void AttachSoundPlayer(SoundPlayer* pPlayer) { mOwner = pPlayer; }

  private:
    struct CallbackRecord : util::IntrusiveListNode {
        /**
         * @brief Construct an unlinked disposal callback record.
         * @param callback Callback invoked on heap clear; null is allowed.
         * @param pArg Context passed unchanged to the callback.
         */
        CallbackRecord(DisposeCallback callback, void* pArg) : callback(callback), pArg(pArg) {}
        DisposeCallback callback;
        void* pArg;
    };
    using CallbackList =
        util::IntrusiveList<CallbackRecord, util::IntrusiveListBaseNodeTraits<CallbackRecord>>;
    SoundPlayer* mOwner;
    u8* mStart;
    u8* mEnd;
    u8* mCurrent;
    enum class State : u8 { Free, Allocated, LoadFinished };
    State mState;
    util::IntrusiveListNode mLink;
    CallbackList mCallbacks;

  public:
    /** @brief Node traits linking heaps into a sound player's heap lists. */
    using LinkNodeTraits =
        util::IntrusiveListMemberNodeTraits<PlayerHeap, &PlayerHeap::mLink, PlayerHeap, 0x50>;
};
static_assert(sizeof(PlayerHeap) == 0x50, "PlayerHeap size");
} // namespace nn::atk::detail
