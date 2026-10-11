#pragma once
#include <nn/os.h>
#include <nn/util/util_IntrusiveList.h>

namespace nn::atk {
class AddonSoundArchive;
class SoundDataManager;
namespace detail {
class AddonSoundArchiveContainer {
  public:
    AddonSoundArchiveContainer();
    ~AddonSoundArchiveContainer();
    bool Initialize(const char* name, const AddonSoundArchive* archive, const SoundDataManager* manager);
    void Finalize();
    bool IsSameName(const char* name) const;
    void SetAddTick(const nn::os::Tick& tick);
    /** @brief Gets the registered archive. @return Archive pointer, or nullptr before initialization. */
    const AddonSoundArchive* GetSoundArchive() const { return mArchive; }
    /** @brief Gets the archive resource manager. @return Manager pointer supplied at initialization. */
    const SoundDataManager* GetSoundDataManager() const { return mDataManager; }
    /** @brief Checks whether the container holds an archive. @return True once initialized. */
    bool IsActive() const { return mInitialized; }
    /** @brief Gets the registered archive name. @return Name passed to Initialize. */
    const char* GetSoundArchiveName() const { return mName; }
    /** @brief Gets when the archive was added. @return Tick passed to SetAddTick. */
    const nn::os::Tick& GetAddTick() const { return mAddTick; }

  private:
    friend class SoundArchiveManager;
    nn::util::IntrusiveListNode mNode;
    bool mInitialized;
    const AddonSoundArchive* mArchive;
    const SoundDataManager* mDataManager;
    char mName[64];
    nn::os::Tick mAddTick;
};
static_assert(sizeof(AddonSoundArchiveContainer) == 0x70, "AddonSoundArchiveContainer size");
} // namespace detail
} // namespace nn::atk
