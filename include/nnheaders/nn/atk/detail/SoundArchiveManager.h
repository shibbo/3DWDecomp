#pragma once

#include <nn/atk/atk_AddonSoundArchiveContainer.h>

namespace nn::atk {
class SoundArchive;
class SoundDataManager;
namespace detail {
class SoundArchiveParametersHook;

class SoundArchiveManager {
  public:
    /** @brief Copy of the main and current archives taken when a sound is set up. */
    class SnapShot {
      public:
        /**
         * @brief Captures a set of archives and their data managers.
         * @param pMainArchive Main sound archive.
         * @param pMainDataManager Data manager of the main sound archive.
         * @param pCurrentArchive Archive that sounds are currently started from.
         * @param pCurrentDataManager Data manager of the current archive.
         */
        SnapShot(const SoundArchive* pMainArchive, const SoundDataManager* pMainDataManager,
                 const SoundArchive* pCurrentArchive, const SoundDataManager* pCurrentDataManager)
            : mMainArchive(pMainArchive), mMainDataManager(pMainDataManager),
              mCurrentArchive(pCurrentArchive), mCurrentDataManager(pCurrentDataManager) {}

        /** @brief Gets the main archive. @return Main sound archive. */
        const SoundArchive* GetMainSoundArchive() const { return mMainArchive; }
        /** @brief Gets the current archive. @return Archive sounds are started from. */
        const SoundArchive* GetCurrentSoundArchive() const { return mCurrentArchive; }
        /** @brief Gets the current data manager. @return Data manager of the current archive. */
        const SoundDataManager* GetCurrentSoundDataManager() const { return mCurrentDataManager; }

      private:
        const SoundArchive* mMainArchive;
        const SoundDataManager* mMainDataManager;
        const SoundArchive* mCurrentArchive;
        const SoundDataManager* mCurrentDataManager;
    };

    SoundArchiveManager();
    ~SoundArchiveManager();
    void Initialize(const SoundArchive* pArchive, const SoundDataManager* pDataManager);
    void Finalize();
    void Add(AddonSoundArchiveContainer& rContainer);
    void Remove(AddonSoundArchiveContainer& rContainer);
    void ChangeTargetArchive(const char* pName);
    bool IsAvailable() const;
    const AddonSoundArchive* GetAddonSoundArchive(const char* pName) const;
    const SoundDataManager* GetAddonSoundDataManager(const char* pName) const;
    const AddonSoundArchiveContainer* GetAddonSoundArchiveContainer(int index) const;
    AddonSoundArchiveContainer* GetAddonSoundArchiveContainer(int index);
    void SetParametersHook(SoundArchiveParametersHook* pHook);
    SoundArchiveParametersHook* GetParametersHook() const;

    /** @brief Gets the main archive. @return Archive passed to Initialize. */
    const SoundArchive* GetMainSoundArchive() const { return mMainArchive; }
    /** @brief Gets the main data manager. @return Data manager passed to Initialize. */
    const SoundDataManager* GetMainSoundDataManager() const { return mMainDataManager; }
    /** @brief Gets the archive selected by ChangeTargetArchive. @return Current archive. */
    const SoundArchive* GetCurrentSoundArchive() const { return mTargetArchive; }
    /** @brief Captures the main and current archives. @return Snapshot of the archives. */
    SnapShot GetSnapShot() const {
        return SnapShot(mMainArchive, mMainDataManager, mTargetArchive, mTargetDataManager);
    }

    /** @brief Counts registered addon archives. @return Number of linked containers. */
    int GetAddonSoundArchiveCount() const { return mContainerList.size(); }

  private:
    using ContainerTraits = nn::util::IntrusiveListMemberNodeTraits<AddonSoundArchiveContainer,
                                                                    &AddonSoundArchiveContainer::mNode>;
    using ContainerList = nn::util::IntrusiveList<AddonSoundArchiveContainer, ContainerTraits>;
    template <class T> using ContainerGetter = const T* (AddonSoundArchiveContainer::*)() const;

    /**
     * @brief Looks up one resource of an addon archive by name.
     * @tparam T Resource type returned by the container getter.
     * @param pName Null-terminated archive name, or nullptr for no addon.
     * @param getter Typed accessor for the resource to retrieve from a matching container.
     * @return Selected resource, or nullptr if no container matches.
     */
    template <class T> const T* FindAddon(const char* pName, ContainerGetter<T> getter) const {
        if (pName == nullptr) {
            return nullptr;
        }
        for (const auto& rContainer : mContainerList) {
            if (rContainer.IsSameName(pName)) {
                return (rContainer.*getter)();
            }
        }
        return nullptr;
    }

    const SoundArchive* mMainArchive;
    const SoundDataManager* mMainDataManager;
    ContainerList mContainerList;
    const SoundArchive* mTargetArchive;
    const SoundDataManager* mTargetDataManager;
    SoundArchiveParametersHook* mParametersHook;
};
static_assert(sizeof(SoundArchiveManager) == 0x38, "SoundArchiveManager size");
} // namespace detail
} // namespace nn::atk
