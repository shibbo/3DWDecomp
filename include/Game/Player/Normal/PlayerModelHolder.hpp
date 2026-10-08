#pragma once

#include <attributes.h>
#include <basis/seadTypes.h>
#include <gfx/seadColor.h>
#include <math/seadQuat.h>

#include "Player/IUsePlayerMash.hpp"
#include "Player/IUsePlayerModelChange.hpp"
#include "Player/IUsePlayerModelVisibility.hpp"
#include "Player/IUsePlayerShadow.hpp"
#include "Player/IUsePlayerSilhouette.hpp"
#include "Player/Normal/PlayerModel.hpp"

/// Holds the models of a player's figures and switches between them.
class PlayerModelHolder : public IUsePlayerModelChange,
                          public IUsePlayerModelVisibility,
                          public IUsePlayerMash,
                          public IUsePlayerSilhouette,
                          public IUsePlayerShadow {
public:
    PlayerModelHolder(u32 modelNum);

    void registerModel(s32 index, PlayerModel* pModel);
    void reuseModel(s32 index, s32 srcIndex);
    void initCurrentModel(s32 index);
    void appear();
    void assignGlobalAlpha(f32* pAlpha);
    void updateModelShowHide();
    void updateInvincible();
    void updateShadow();
    void updateSilhouette();
    void updateIK();
    void updateHairCtrl();
    void updateSkirtDynamics();
    void updateTailDynamics();
    void updateHairDynamics();
    void updateShadowLength();
    void kill();

    void change(s32 index) override;
    void show() override;
    void hide() override;
    bool isHidden() const override;
    void showSilhouette() override;
    void hideSilhouette() override;
    bool isSilhouetteHidden() const override;
    void showShadow() override;
    void hideShadow() override;

    /** @brief Checks if the shadow is hidden. @return True if hidden. */
    bool isShadowHidden() const override { return mIsShadowHidden; }

    void validateMash() override;
    void invalidateMash() override;

    /** @brief Checks if the mash joint control is enabled. @return True if enabled. */
    bool isMash() const override { return mIsMash; }

    void startInvincible();
    void setInvincibleColor(const sead::Color4f& rColor);
    void endInvincible();
    void startWallSnap();
    void endWallSnap();
    void validateIK();
    void invalidateIK();
    void showFur();
    void hideFur();
    void validateHairCtrl();
    void invalidateHairCtrl();
    void validateSkirtDynamics();
    void invalidateSkirtDynamics();
    void resetSkirtDynamics();
    void validateTailDynamics();
    void invalidateTailDynamics();
    void resetTailDynamics();
    void validateHairDynamics();
    void invalidateHairDynamics();
    void resetHairDynamics();
    void setShadowLength(f32 length);
    void validateCircleShadow();
    void invalidateCircleShadow();
    void hideSubActorShadow();
    void showSubActorShadow();

    /** @brief Gets the model of the current figure. @return Model. */
    PlayerModel* getCurrentModel() const { return mModels[mCurrentIndex]; }

    /** @brief Gets the index of the current figure. @return Figure index. */
    s32 getCurrentIndex() const { return mCurrentIndex; }

    /** @brief Gets the model of a figure. @param index Figure index. @return Model. */
    PlayerModel* getModel(s32 index) const { return mModels[index]; }

    /** @brief Gets the shadow length last set with setShadowLength. @return Length. */
    f32 getShadowLength() const { return mShadowLength; }

private:
    /** @brief Applies every display state to the current model (always inlined, as in the game). */
    ALWAYS_INLINE inline void updateAll();

    PlayerModel** mModels;               // 0x28
    s32 mCurrentIndex = 0;               // 0x30
    bool mIsAlive = false;               // 0x34
    bool mIsHidden = false;              // 0x35
    bool mIsShadowHidden = false;        // 0x36
    bool mIsMash = false;                // 0x37
    bool mIsSilhouetteHidden = false;    // 0x38
    sead::Quatf mJointQuat = sead::Quatf::unit;  // 0x3c
    bool mIsInvincible = false;          // 0x4c
    bool mIsWallSnap = false;            // 0x4d
    bool mIsValidIK = false;             // 0x4e
    bool mIsValidHairCtrl = false;       // 0x4f
    bool mIsValidSkirtDynamics = true;   // 0x50
    bool mIsValidTailDynamics = true;    // 0x51
    bool mIsValidHairDynamics = true;    // 0x52
    bool _53 = false;                    // 0x53
    f32 mShadowLength = 1250.0f;         // 0x54
    bool mIsSetShadowLength = false;     // 0x58
    u32 mModelNum;                       // 0x5c
    bool mIsCircleShadow = false;        // 0x60
};
