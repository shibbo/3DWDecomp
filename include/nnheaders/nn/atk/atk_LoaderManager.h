#pragma once
#include <nn/atk/atk_SoundThread.h>

namespace nn::atk::detail {
/**
 * @brief Owns the storage and list roots for a sound runtime's loaders.
 * @tparam Loader Loader type managed by the runtime.
 */
template <typename Loader> class LoaderManager : public driver::SoundThread::SoundFrameCallback {
  public:
    /** @brief Constructs an empty loader manager. */
    LoaderManager() : mMemory(nullptr), mSize(0) {}
    /** @brief Destroys the manager after its owner releases the loaders. */
    ~LoaderManager() override = default;
    void OnBeginSoundFrame() override;

    /**
     * @brief Takes the first idle loader off the free list.
     * @return The loader, or nullptr when every loader is in use.
     */
    Loader* Alloc() {
        if (!mFreeLoaders.IsLinked()) {
            return nullptr;
        }

        using LinkTraits = util::IntrusiveListMemberNodeTraits<Loader, &Loader::mManagerLink>;
        util::IntrusiveListNode* pNode = mFreeLoaders.GetNext();
        pNode->Unlink();
        return &LinkTraits::GetItem(*pNode);
    }

    /**
     * @brief Returns a loader; one whose tasks still run is parked until they finish.
     * @param pLoader Loader obtained from Alloc().
     */
    void Free(Loader* pLoader) {
        if (pLoader->IsInUse()) {
            mActiveLoaders.LinkPrev(&pLoader->mManagerLink);
        } else {
            mFreeLoaders.LinkPrev(&pLoader->mManagerLink);
        }
    }

  private:
    void* mMemory;
    size_t mSize;
    util::IntrusiveListNode mFreeLoaders;
    util::IntrusiveListNode mActiveLoaders;
};
} // namespace nn::atk::detail
