#pragma once

#include <nn/atk/atk_SoundArchive.h>

namespace nn::atk::detail {
/** @brief Lets a tool override sound archive item parameters (for example while editing). */
class SoundArchiveParametersHook {
public:
    virtual ~SoundArchiveParametersHook() {}

    /** @brief Checks whether the hook currently overrides parameters. @return True when enabled. */
    bool GetIsEnable() const { return m_IsEnabled; }

    /**
     * @brief Enables or disables the hook.
     * @param isEnabled True to let the hook override parameters.
     */
    void SetIsEnable(bool isEnabled) { m_IsEnabled = isEnabled; }

    /**
     * @brief Resolves an item label through the hook.
     * @param pItemLabel Label of the archive item.
     * @return Item id provided by the hook.
     */
    SoundArchive::ItemId GetItemId(const char* pItemLabel) const { return GetItemIdImpl(pItemLabel); }

    /**
     * @brief Gets the sound type of an item through the hook.
     * @param pItemLabel Label of the archive item.
     * @return Sound type provided by the hook.
     */
    SoundArchive::SoundType GetSoundType(const char* pItemLabel) {
        return GetSoundTypeImpl(pItemLabel);
    }

protected:
    virtual bool IsTargetItemImpl(const char* pItemLabel) = 0;
    virtual const char* GetItemLabelImpl(SoundArchive::ItemId id) const = 0;
    virtual SoundArchive::ItemId GetItemIdImpl(const char* pItemLabel) const = 0;
    virtual SoundArchive::SoundType GetSoundTypeImpl(const char* pItemLabel) = 0;

private:
    bool m_IsEnabled;
};
}  // namespace nn::atk::detail
