#pragma once

#include <basis/seadTypes.h>
#include <prim/seadSafeString.h>

#include "Player/IUsePlayerAnimator.hpp"
#include "Player/Normal/PlayerAnimFunc.hpp"
#include "Player/Normal/PlayerModel.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"

namespace al {
class LiveActor;
}

class IUsePlayerRetargettingSelector;
class PlayerAnimFrameCtrl;

/// Plays the player's skeletal, sub and material animations.
class PlayerAnimator : public IUsePlayerAnimator {
public:
    PlayerAnimator(al::LiveActor* pActor, PlayerModelHolder* pModelHolder,
                   IUsePlayerRetargettingSelector* pRetargettingSelector);

    ~PlayerAnimator() override {}

    void enableAlphaCtrl(bool isEnable) override;
    void startAnim(const sead::SafeString& rName) override;
    void setAnimRate(f32 rate) override;
    void setSubAnimRate(f32 rate) override;
    void setAnimFrame(f32 frame) override;
    bool isAnimEnd() const override;
    bool isAnim(const sead::SafeString& rName) const override;

    /** @brief Tests whether the main animation is played mirrored. @return True if reversed. */
    bool isAnimReverse() const override { return mIsAnimReverse; }

    f32 getAnimFrame() const override;
    f32 getAnimFrameMax() const override;
    void clearInterpolation() override;

    /** @brief Gets the name of the main animation. @return Animation name. */
    const char* getAnimName() const override { return mAnimName.cstr(); }

    void startSubAnim(const sead::SafeString& rName) override;
    void endSubAnim() override;

    /** @brief Tests whether a sub animation overrides the main one. @return True if so. */
    bool isSubAnimBinding() const override { return mIsSubAnimBinding; }

    bool isSubAnimEnd() const override;
    bool isSubAnim(const sead::SafeString& rName) const override;

    /** @brief Tests whether the sub animation is played mirrored. @return True if reversed. */
    bool isSubAnimReverse() const override { return mIsSubAnimReverse; }

    f32 getSubAnimFrame() const override;
    f32 getAnimFrameMax(const sead::SafeString& rName) const override;
    void startMaterialAnim(const sead::SafeString& rName) override;
    bool isMaterialAnimEnd() const override;
    void setWeightSixfold(f32 weight0, f32 weight1, f32 weight2, f32 weight3, f32 weight4,
                          f32 weight5) override;

    /**
     * @brief Gets one of the six blend weights.
     * @param index Weight index.
     * @return Blend weight.
     */
    f32 getWeight(u32 index) const override { return mWeights[index]; }

    bool isUpperBodyAnimAttached() const override;

    void init();
    void update(bool isStopFrame);
    void copyAnim();
    void applyReplaceAnim();
    void applyPartsSpecialRule();
    void startAnimLocal(const sead::SafeString& rName, s32 retargettingType);
    void setAnimRateCommon(f32 rate);
    void setAnimFrameCommon(f32 frame);
    void startAnimCommon(const sead::SafeString& rName, s32 retargettingType);
    void resetWeight();
    void startUpperBodyAnim(const sead::SafeString& rName);
    void clearUpperBodyAnim();
    bool isUpperBodyAnimEnd() const;
    void copyAnimLocal(const sead::SafeString& rName);
    void tryStartPartsAnim(const sead::SafeString& rName);
    void setPartsRate(f32 rate);
    void setPartsFrame(f32 frame);
    void replaceAnim(PlayerModel* pModel, const sead::SafeString& rName, s32 retargettingType);
    void revertAnim();

    /** @brief Tests whether the cat's ClimbMove animation is played as a normal walk. @return True if so. */
    bool isClimbMoveAsWalk() const { return mIsClimbMoveAsWalk; }

private:
    /** @brief Gets the model of the current figure. @return Current model. */
    PlayerModel* getModel() const {
        return static_cast<PlayerModel*>(mModelHolder->getCurrentModel());
    }

    void restartMainAnim();

    al::LiveActor* mActor;                                 // 0x8
    PlayerModelHolder* mModelHolder;                       // 0x10
    IUsePlayerRetargettingSelector* mRetargettingSelector;  // 0x18
    PlayerAnimFrameCtrl* mFrameCtrl;                       // 0x20
    bool mIsSubAnimBinding;                                // 0x28
    bool mIsUpperBodyAnim;                                 // 0x29
    bool mIsMaterialAnim;                                  // 0x2a
    bool mIsSideFacingCamera;                              // 0x2b
    bool mIsWeightSet;                                     // 0x2c
    f32 mWeights[6];                                       // 0x30
    s32 mAnimRetargettingType;                             // 0x48
    s32 mSubAnimRetargettingType;                          // 0x4c
    PlayerModel* mPrevModel;                               // 0x50
    sead::FixedSafeString<64> mAnimName;                   // 0x58
    sead::FixedSafeString<64> mSubAnimName;                // 0xb0
    sead::FixedSafeString<64> mUpperBodyAnimName;          // 0x108
    bool mIsAnimReverse;                                   // 0x160
    bool mIsSubAnimReverse;                                // 0x161
    bool mIsClimbMoveAsWalk;                               // 0x162
};

static_assert(sizeof(PlayerAnimator) == 0x168);
