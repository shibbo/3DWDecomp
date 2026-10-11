#include "Player/Normal/PlayerAnimator.hpp"

#include <math/seadVector.h>

#include "Library/Anim/AnimPlayerSkl.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Project/Play/Actor/ActorAlphaCtrl.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/SubActorKeeper.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Model/alModelCafe.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Player/Normal/PlayerAnimFrameCtrl.hpp"
#include "Player/Normal/PlayerRetargettingSelector.hpp"

namespace {
/// Sub actor sync flag of parts that never follow the player's animations.
constexpr u32 cSyncFlag_IgnoreAnim = 8;

/**
 * @brief Tests whether a parts actor is one of the hands (driven by upper body animations).
 * @param pParts Parts actor.
 * @return True if it is the left or right hand.
 */
bool isHandParts(const al::LiveActor* pParts) {
    const char* name = pParts->getName();
    return al::isEqualString(name, "左手") || al::isEqualString(name, "右手");
}

/**
 * @brief Tests whether a sub actor follows the player's animations.
 * @param pInfo Sub actor info.
 * @return True if it does.
 */
bool isSyncAnimParts(const al::SubActorInfo* pInfo) {
    return (pInfo->mSyncType & cSyncFlag_IgnoreAnim) == 0;
}

void startUpperBodyAnimImpl(PlayerModel* pModel, const sead::SafeString& rName, bool isReverse,
                            IUsePlayerRetargettingSelector* pSelector, s32 index);
void clearUpperBodyAnimImpl(PlayerModel* pModel);
}  // namespace

/**
 * @brief Constructs the animator.
 * @param pActor Player actor.
 * @param pModelHolder Holder of the figure models.
 * @param pRetargettingSelector Selector of the skeletal retargetting info.
 */
PlayerAnimator::PlayerAnimator(al::LiveActor* pActor, PlayerModelHolder* pModelHolder,
                               IUsePlayerRetargettingSelector* pRetargettingSelector)
    : mActor(pActor), mModelHolder(pModelHolder), mRetargettingSelector(pRetargettingSelector),
      mFrameCtrl(new PlayerAnimFrameCtrl()), mIsSubAnimBinding(false), mIsUpperBodyAnim(false),
      mIsMaterialAnim(false), mIsSideFacingCamera(true), mIsWeightSet(false),
      mAnimRetargettingType(PlayerAnimFunc::cRetargettingType_Default),
      mSubAnimRetargettingType(PlayerAnimFunc::cRetargettingType_Default), mPrevModel(nullptr),
      mIsAnimReverse(false), mIsSubAnimReverse(false), mIsClimbMoveAsWalk(false) {}

/**
 * @brief Remembers the current model.
 */
void PlayerAnimator::init() {
    mPrevModel = getModel();
}

/**
 * @brief Advances the animations and carries them over when the figure model changed.
 * @param isStopFrame Whether the sub animation frame is kept.
 */
void PlayerAnimator::update(bool isStopFrame) {
    PlayerModel* model = getModel();

    if (mIsSubAnimBinding) {
        if (!isStopFrame) {
            mFrameCtrl->update();
        }
    } else {
        mFrameCtrl->updateSync(model);
    }

    sead::Vector3f sideDir;
    al::calcSideDir(&sideDir, model);
    sead::Vector3f lookDir;
    al::calcCameraLookDir(&lookDir, model);
    mIsSideFacingCamera = sideDir.dot(lookDir) >= 0.5736f;

    if (mPrevModel != model) {
        copyAnim();
    }

    applyReplaceAnim();
    applyPartsSpecialRule();
    mPrevModel = model;
}

/**
 * @brief Copies the animations of the previous model to the current one.
 */
void PlayerAnimator::copyAnim() {
    PlayerModel* model = getModel();

    if (mIsSubAnimBinding) {
        copyAnimLocal(mSubAnimName);
    } else {
        copyAnimLocal(mAnimName);
    }

    if (!mIsUpperBodyAnim) {
        return;
    }

    startUpperBodyAnimImpl(model, mUpperBodyAnimName, mIsSideFacingCamera, mRetargettingSelector,
                           model->getRetargettingIndex());
    al::setPartialSklAnimFrame(model, 0, al::getPartialSklAnimFrame(mPrevModel, 0));
    al::setPartialSklAnimFrameRate(model, 0, al::getPartialSklAnimFrameRate(mPrevModel, 0));
    clearUpperBodyAnimImpl(mPrevModel);
}

/**
 * @brief Plays the base figure's walking animations on the cat while it plays an upper body
 * animation.
 */
void PlayerAnimator::applyReplaceAnim() {
    if (mIsSubAnimBinding) {
        mIsClimbMoveAsWalk = false;
        return;
    }

    PlayerModel* model = getModel();
    bool isAttached = al::isPartialSklAnimAttached(model, 0);

    if (mPrevModel != model && isAttached) {
        model->getModelKeeper()->getModelCafe()->getAnimPlayerSkl()->mIsSkipInterp = true;
    }

    bool isClimb = al::isEqualString(model->getAnimSetName(), "Climb");

    if (!isAttached || !isClimb) {
        revertAnim();
        return;
    }

    if (model != mPrevModel && al::isEqualString(mPrevModel->getAnimSetName(), "Climb")) {
        mIsClimbMoveAsWalk = false;
    }

    if (isAnim("Move") || isAnim("GigaMove") || isAnim("Brake") || isAnim("Turn") ||
        isAnim("TurnPoint") || isAnim("Land") || isAnim("Wait")) {
        if (mIsClimbMoveAsWalk) {
            return;
        }

        auto* baseModel = static_cast<PlayerModel*>(mModelHolder->getModel(0));
        al::StringTmp<128> name("%s", mAnimName.cstr());

        if (mIsAnimReverse) {
            PlayerAnimFunc::tryConvertToReverseName(&name, baseModel);
        }

        replaceAnim(model, sead::SafeString(name.cstr()), PlayerAnimFunc::cRetargettingType_Default);
        mIsClimbMoveAsWalk = true;
    } else if (isAnim("JumpKeep") && !mIsClimbMoveAsWalk) {
        replaceAnim(model, "PeachClimbJumpKeepReplace", PlayerAnimFunc::cRetargettingType_None);
        mIsClimbMoveAsWalk = true;
    }
}

/**
 * @brief Switches finished one-time parts animations to their looping version.
 */
void PlayerAnimator::applyPartsSpecialRule() {
    al::SubActorKeeper* keeper = getModel()->getSubActorKeeper();

    if (keeper == nullptr) {
        return;
    }

    s32 num = keeper->getSubActorNum();
    for (s32 i = 0; i < num; i++) {
        al::SubActorInfo* info = keeper->getSubActorInfo(i);

        if (!isSyncAnimParts(info)) {
            continue;
        }

        al::LiveActor* parts = info->mSubActor;

        if (mIsUpperBodyAnim && isHandParts(parts)) {
            continue;
        }

        const char* actionName = al::getActionName(parts);

        if (al::isActionOneTime(parts, actionName) && al::isActionEnd(parts)) {
            al::StringTmp<128> loopName("%sLoop", actionName);

            if (al::isExistAction(parts, loopName.cstr())) {
                al::startAction(parts, loopName.cstr());
            }
        }
    }
}

/**
 * @brief Enables or disables the player's alpha control.
 * @param isEnable Whether to enable it.
 */
void PlayerAnimator::enableAlphaCtrl(bool isEnable) {
    al::ActorAlphaCtrl* alphaCtrl = mActor->getAlphaCtrl();

    if (alphaCtrl != nullptr) {
        alphaCtrl->setOn(isEnable);
    }
}

/**
 * @brief Starts a main animation.
 * @param rName Animation name.
 */
void PlayerAnimator::startAnim(const sead::SafeString& rName) {
    sead::FixedSafeString<128> name;
    mAnimRetargettingType = PlayerAnimFunc::convertToRegularName(&name, getModel(), rName);
    mIsAnimReverse = false;

    if (mIsSideFacingCamera) {
        mIsAnimReverse = PlayerAnimFunc::tryConvertToReverseName(&name, getModel());
    }

    startAnimLocal(name, mAnimRetargettingType);
    setAnimRate(1.0f);
    mAnimName = rName;
    mIsClimbMoveAsWalk = false;
}

/**
 * @brief Starts a main animation on the frame control and, without a sub animation, on the models.
 * @param rName Regular animation name.
 * @param retargettingType Retargetting info to use.
 */
void PlayerAnimator::startAnimLocal(const sead::SafeString& rName, s32 retargettingType) {
    mFrameCtrl->startAction(getModel(), rName);
    mIsWeightSet = false;

    if (mIsSubAnimBinding) {
        return;
    }

    startAnimCommon(rName, retargettingType);
}

/**
 * @brief Sets the play rate of the main animation.
 * @param rate Frame rate.
 */
void PlayerAnimator::setAnimRate(f32 rate) {
    mFrameCtrl->setRate(rate);

    if (mIsSubAnimBinding) {
        return;
    }

    setAnimRateCommon(rate);
}

/**
 * @brief Sets the play rate of the player, its model and its parts.
 * @param rate Frame rate.
 */
void PlayerAnimator::setAnimRateCommon(f32 rate) {
    al::setSklAnimBlendFrameRateAll(mActor, rate, true);
    al::setSklAnimBlendFrameRateAll(getModel(), rate, true);
    setPartsRate(rate);
}

/**
 * @brief Sets the play rate of the sub animation.
 * @param rate Frame rate.
 */
void PlayerAnimator::setSubAnimRate(f32 rate) {
    mFrameCtrl->setRate(rate);
    setAnimRateCommon(rate);
}

/**
 * @brief Sets the frame of the main animation.
 * @param frame Frame.
 */
void PlayerAnimator::setAnimFrame(f32 frame) {
    mFrameCtrl->setFrame(frame);

    if (mIsSubAnimBinding) {
        return;
    }

    setAnimFrameCommon(frame);
}

/**
 * @brief Sets the animation frame of the player, its model and its parts.
 * @param frame Frame.
 */
void PlayerAnimator::setAnimFrameCommon(f32 frame) {
    al::setSklAnimBlendFrameAll(mActor, frame, true);
    al::setSklAnimBlendFrameAll(getModel(), frame, true);
    setPartsFrame(frame);
}

/**
 * @brief Tests whether the main animation ended.
 * @return True if ended.
 */
bool PlayerAnimator::isAnimEnd() const {
    return mFrameCtrl->isActionEnd();
}

/**
 * @brief Tests whether a main animation is playing.
 * @param rName Animation name.
 * @return True if playing.
 */
bool PlayerAnimator::isAnim(const sead::SafeString& rName) const {
    if (al::isEqualString(rName.cstr(), mAnimName.cstr())) {
        return true;
    }

    return al::isEqualString(rName.cstr(), mFrameCtrl->getActionName());
}

/**
 * @brief Gets the frame of the main animation.
 * @return Frame.
 */
f32 PlayerAnimator::getAnimFrame() const {
    return mFrameCtrl->getCurrentFrame();
}

/**
 * @brief Gets the last frame of the main animation.
 * @return Max frame.
 */
f32 PlayerAnimator::getAnimFrameMax() const {
    return mFrameCtrl->getFrameMax();
}

/**
 * @brief Cancels the interpolation of the model's skeletal animation.
 */
void PlayerAnimator::clearInterpolation() {
    if (mIsSubAnimBinding) {
        return;
    }

    al::clearSklAnimInterpole(getModel());
}

/**
 * @brief Starts a sub animation that overrides the main one.
 * @param rName Animation name.
 */
void PlayerAnimator::startSubAnim(const sead::SafeString& rName) {
    sead::FixedSafeString<128> name;
    mSubAnimRetargettingType = PlayerAnimFunc::convertToRegularName(&name, getModel(), rName);
    mIsSubAnimReverse = false;

    if (mIsSideFacingCamera) {
        mIsSubAnimReverse = PlayerAnimFunc::tryConvertToReverseName(&name, getModel());
    }

    mIsSubAnimBinding = true;
    startAnimCommon(name, mSubAnimRetargettingType);
    mSubAnimName = rName;
}

/**
 * @brief Starts an animation on the player, its model and its parts.
 * @param rName Regular animation name.
 * @param retargettingType Retargetting info to use.
 */
void PlayerAnimator::startAnimCommon(const sead::SafeString& rName, s32 retargettingType) {
    al::startAction(mActor, rName.cstr());
    PlayerModel* model = getModel();
    al::startAction(model, rName.cstr());
    PlayerAnimFunc::controlRetargetting(model, mRetargettingSelector, retargettingType);
    tryStartPartsAnim(rName);
}

/**
 * @brief Restarts the main animation kept by the frame control on the models.
 */
inline void PlayerAnimator::restartMainAnim() {
    startAnimCommon(sead::SafeString(mFrameCtrl->getActionName()), mAnimRetargettingType);
    setAnimRate(mFrameCtrl->getRate());
    setAnimFrame(mFrameCtrl->getCurrentFrame());
    resetWeight();
}

/**
 * @brief Ends the sub animation and goes back to the main one.
 */
void PlayerAnimator::endSubAnim() {
    mIsSubAnimBinding = false;
    restartMainAnim();
}

/**
 * @brief Applies the blend weights to the model and the parts' animations.
 */
void PlayerAnimator::resetWeight() {
    if (!mIsWeightSet) {
        return;
    }

    al::setSklAnimBlendWeightSixfold(getModel(), mWeights[0], mWeights[1], mWeights[2],
                                     mWeights[3], mWeights[4], mWeights[5]);

    PlayerModel* model = getModel();
    al::SubActorKeeper* keeper = model->getSubActorKeeper();

    if (keeper == nullptr) {
        return;
    }

    s32 mainIndex = -1;
    f32 mainWeight = 0.8f;
    for (s32 i = 0; i < 6; i++) {
        if (mWeights[i] > mainWeight) {
            mainWeight = mWeights[i];
            mainIndex = i;
        }
    }

    s32 num = keeper->getSubActorNum();
    for (s32 i = 0; i < num; i++) {
        al::SubActorInfo* info = keeper->getSubActorInfo(i);

        if (!isSyncAnimParts(info)) {
            continue;
        }

        if (mIsUpperBodyAnim && isHandParts(info->mSubActor)) {
            continue;
        }

        if (al::isEqualString(al::getActionName(model), al::getActionName(info->mSubActor)) &&
            al::isSklAnimExist(info->mSubActor)) {
            al::setSklAnimBlendWeightSixfold(info->mSubActor, mWeights[0], mWeights[1],
                                             mWeights[2], mWeights[3], mWeights[4], mWeights[5]);
        }

        if (mainIndex < 0) {
            continue;
        }

        const char* animName = al::getPlayingSklAnimName(model, mainIndex);
        f32 frame = al::getSklAnimFrame(model, mainIndex);
        f32 rate = al::getSklAnimFrameRate(model, mainIndex);

        if (al::isVisAnimExist(info->mSubActor)) {
            if (!al::isVisAnimExist(info->mSubActor, animName)) {
                al::startVisAnim(info->mSubActor, "Wait");
            } else if (!al::isVisAnimPlaying(info->mSubActor, animName)) {
                al::startVisAnim(info->mSubActor, animName);
                al::setVisAnimFrame(info->mSubActor, frame);
                al::setVisAnimFrameRate(info->mSubActor, rate);
            }
        }

        if (al::isMtpAnimExist(info->mSubActor)) {
            if (!al::isMtpAnimExist(info->mSubActor, animName)) {
                al::startMtpAnim(info->mSubActor, "Wait");
            } else if (!al::isMtpAnimPlaying(info->mSubActor, animName)) {
                al::startMtpAnim(info->mSubActor, animName);
                al::setMtpAnimFrame(info->mSubActor, frame);
                al::setMtpAnimFrameRate(info->mSubActor, rate);
            }
        }
    }
}

/**
 * @brief Tests whether the sub animation ended.
 * @return True if ended or if there is no sub animation.
 */
bool PlayerAnimator::isSubAnimEnd() const {
    if (!mIsSubAnimBinding) {
        return true;
    }

    return al::isActionEnd(getModel());
}

/**
 * @brief Tests whether a sub animation is playing.
 * @param rName Animation name.
 * @return True if playing.
 */
bool PlayerAnimator::isSubAnim(const sead::SafeString& rName) const {
    if (!mIsSubAnimBinding) {
        return false;
    }

    if (al::isEqualString(rName.cstr(), mSubAnimName.cstr())) {
        return true;
    }

    return al::isEqualString(rName.cstr(), al::getPlayingSklAnimName(getModel(), 0));
}

/**
 * @brief Gets the frame of the sub animation.
 * @return Frame.
 */
f32 PlayerAnimator::getSubAnimFrame() const {
    return al::getActionFrame(getModel());
}

/**
 * @brief Gets the last frame of a skeletal animation of the model.
 * @param rName Animation name.
 * @return Max frame.
 */
f32 PlayerAnimator::getAnimFrameMax(const sead::SafeString& rName) const {
    return al::getSklAnimFrameMax(getModel(), rName.cstr());
}

/**
 * @brief Starts a material animation on the model if it has it.
 * @param rName Animation name.
 */
void PlayerAnimator::startMaterialAnim(const sead::SafeString& rName) {
    mIsMaterialAnim = al::tryStartMclAnimIfExist(getModel(), rName.cstr());
}

/**
 * @brief Tests whether the material animation ended.
 * @return True if ended or if none is playing.
 */
bool PlayerAnimator::isMaterialAnimEnd() const {
    if (!mIsMaterialAnim) {
        return true;
    }

    return al::isMclAnimEnd(getModel());
}

/**
 * @brief Sets the six blend weights.
 * @param weight0 Weight of the first animation.
 * @param weight1 Weight of the second animation.
 * @param weight2 Weight of the third animation.
 * @param weight3 Weight of the fourth animation.
 * @param weight4 Weight of the fifth animation.
 * @param weight5 Weight of the sixth animation.
 */
void PlayerAnimator::setWeightSixfold(f32 weight0, f32 weight1, f32 weight2, f32 weight3,
                                      f32 weight4, f32 weight5) {
    mIsWeightSet = true;
    mWeights[0] = weight0;
    mWeights[1] = weight1;
    mWeights[2] = weight2;
    mWeights[3] = weight3;
    mWeights[4] = weight4;
    mWeights[5] = weight5;

    if (mIsSubAnimBinding) {
        return;
    }

    resetWeight();
}

/**
 * @brief Tests whether an upper body animation is attached to the model.
 * @return True if attached.
 */
bool PlayerAnimator::isUpperBodyAnimAttached() const {
    return al::isPartialSklAnimAttached(getModel(), 0);
}

/**
 * @brief Starts an upper body animation.
 * @param rName Animation name.
 */
void PlayerAnimator::startUpperBodyAnim(const sead::SafeString& rName) {
    PlayerModel* model = getModel();
    startUpperBodyAnimImpl(model, rName, mIsSideFacingCamera, mRetargettingSelector,
                           model->getRetargettingIndex());
    mUpperBodyAnimName = rName;
    mIsUpperBodyAnim = true;
}

namespace {
/**
 * @brief Starts an upper body animation on a model and its hands.
 * @param pModel Model.
 * @param rName Animation name.
 * @param isReverse Whether to play the mirrored animation.
 * @param pSelector Selector of the retargetting info.
 * @param index Retargetting info index of the model.
 */
void startUpperBodyAnimImpl(PlayerModel* pModel, const sead::SafeString& rName, bool isReverse,
                            IUsePlayerRetargettingSelector* pSelector, s32 index) {
    sead::FixedSafeString<128> name;
    PlayerAnimFunc::RetargettingType type =
        PlayerAnimFunc::convertToRegularName(&name, pModel, rName);

    if (isReverse) {
        PlayerAnimFunc::tryConvertToReverseName(&name, pModel);
    }

    const al::SklAnimRetargettingInfo* retargettingInfo;
    switch (type) {
    case PlayerAnimFunc::cRetargettingType_Default:
        retargettingInfo = pSelector->getDefaultRetargettingInfo(index);
        break;
    case PlayerAnimFunc::cRetargettingType_Chara:
        retargettingInfo = pSelector->getCharaRetargettingInfo(index);
        break;
    case PlayerAnimFunc::cRetargettingType_Figure:
        retargettingInfo = pSelector->getFigureRetargettingInfo(index);
        break;
    default:
        retargettingInfo = nullptr;
        break;
    }

    al::startPartialSklAnim(pModel, name.cstr(), 0, 0, retargettingInfo);

    al::SubActorKeeper* keeper = pModel->getSubActorKeeper();

    if (keeper == nullptr) {
        return;
    }

    s32 num = keeper->getSubActorNum();
    for (s32 i = 0; i < num; i++) {
        al::SubActorInfo* info = keeper->getSubActorInfo(i);

        if (!isSyncAnimParts(info) || !isHandParts(info->mSubActor)) {
            continue;
        }

        if (al::isExistAction(info->mSubActor, name.cstr())) {
            al::startAction(info->mSubActor, name.cstr());
        } else {
            al::startAction(info->mSubActor, "Wait");
        }
    }
}
}  // namespace

/**
 * @brief Clears the upper body animation.
 */
void PlayerAnimator::clearUpperBodyAnim() {
    clearUpperBodyAnimImpl(getModel());
    mIsUpperBodyAnim = false;
}

namespace {
/**
 * @brief Clears the upper body animation of a model and syncs its hands back to the main
 * animation.
 * @param pModel Model.
 */
void clearUpperBodyAnimImpl(PlayerModel* pModel) {
    al::clearPartialSklAnim(pModel, 0);

    const char* actionName = al::getActionName(pModel);
    f32 frame = al::getActionFrame(pModel);
    f32 frameMax = al::getActionFrameMax(pModel, actionName);
    f32 rate = al::getActionFrameRate(pModel);
    al::getActionFrameMax(pModel, actionName);

    al::SubActorKeeper* keeper = pModel->getSubActorKeeper();

    if (keeper == nullptr) {
        return;
    }

    s32 num = keeper->getSubActorNum();
    f32 frameRatio = frame / frameMax;
    for (s32 i = 0; i < num; i++) {
        al::SubActorInfo* info = keeper->getSubActorInfo(i);

        if (!isSyncAnimParts(info)) {
            continue;
        }

        al::LiveActor* parts = info->mSubActor;

        if (!isHandParts(parts)) {
            continue;
        }

        if (!al::isExistAction(parts, actionName)) {
            al::startAction(info->mSubActor, "Wait");
            continue;
        }

        al::startAction(parts, actionName);

        if (al::isSklAnimExist(parts) && al::isSklAnimPlaying(parts, 0)) {
            al::setSklAnimBlendFrameAll(parts, frame, true);
            al::setSklAnimBlendFrameRateAll(parts, rate, true);
        }

        if (al::isMtpAnimExist(parts) && al::isMtpAnimPlaying(parts)) {
            al::setMtpAnimFrame(parts, frameRatio * al::getMtpAnimFrameMax(parts));
        }

        if (al::isVisAnimExist(parts) && al::isVisAnimPlaying(parts)) {
            al::setVisAnimFrame(parts, frameRatio * al::getVisAnimFrameMax(parts));
        }
    }
}
}  // namespace

/**
 * @brief Tests whether the upper body animation ended.
 * @return True if ended.
 */
bool PlayerAnimator::isUpperBodyAnimEnd() const {
    return al::isPartialSklAnimEnd(getModel(), 0);
}

/**
 * @brief Carries an animation of the previous model over to the current one.
 * @param rName Animation name.
 */
void PlayerAnimator::copyAnimLocal(const sead::SafeString& rName) {
    al::StringTmp<128> prevActionName("%s", al::getActionName(mPrevModel));
    al::StringTmp<128> name;
    s32 type = PlayerAnimFunc::convertToRegularName(&name, getModel(), rName);
    bool isReverse = false;

    if (mIsSideFacingCamera) {
        isReverse = PlayerAnimFunc::tryConvertToReverseName(&name, getModel());
    }

    if (al::isEqualString(name, prevActionName)) {
        al::copyAction(getModel(), mPrevModel);
        PlayerAnimFunc::controlRetargetting(getModel(), mRetargettingSelector, type);
        tryStartPartsAnim(sead::SafeString(al::getActionName(getModel())));
        setPartsRate(al::getActionFrameRate(getModel()));
        setPartsFrame(al::getActionFrame(getModel()));
    } else {
        startAnimCommon(name, type);
        setAnimRateCommon(al::getActionFrameRate(mPrevModel));
        setAnimFrameCommon(al::getActionFrame(mPrevModel));
    }

    al::clearSklAnimInterpole(getModel());

    if (mIsSubAnimBinding) {
        al::StringTmp<128> animName;
        mAnimRetargettingType =
            PlayerAnimFunc::convertToRegularName(&animName, getModel(), mAnimName);
        mIsAnimReverse = false;

        if (mIsSideFacingCamera) {
            mIsAnimReverse = PlayerAnimFunc::tryConvertToReverseName(&animName, getModel());
        }

        mFrameCtrl->changeActionName(getModel(), animName);
        mSubAnimRetargettingType = type;
        mIsSubAnimReverse = isReverse;
    } else {
        mAnimRetargettingType = type;
        mIsAnimReverse = isReverse;
        mFrameCtrl->changeActionName(getModel(), name);
    }
}

/**
 * @brief Starts an animation on the parts, falling back to Wait.
 * @param rName Animation name.
 */
void PlayerAnimator::tryStartPartsAnim(const sead::SafeString& rName) {
    al::SubActorKeeper* keeper = getModel()->getSubActorKeeper();

    if (keeper == nullptr) {
        return;
    }

    s32 num = keeper->getSubActorNum();
    for (s32 i = 0; i < num; i++) {
        al::SubActorInfo* info = keeper->getSubActorInfo(i);

        if (!isSyncAnimParts(info)) {
            continue;
        }

        if (mIsUpperBodyAnim && isHandParts(info->mSubActor)) {
            continue;
        }

        if (!al::tryStartAction(info->mSubActor, rName.cstr())) {
            al::startAction(info->mSubActor, "Wait");
        }
    }
}

/**
 * @brief Sets the play rate of the parts playing the model's animation.
 * @param rate Frame rate.
 */
void PlayerAnimator::setPartsRate(f32 rate) {
    const char* actionName = al::getActionName(getModel());
    f32 frameMax = al::getActionFrameMax(getModel(), actionName);
    al::SubActorKeeper* keeper = getModel()->getSubActorKeeper();

    if (keeper == nullptr) {
        return;
    }

    s32 num = keeper->getSubActorNum();
    f32 rateRatio = rate / frameMax;
    for (s32 i = 0; i < num; i++) {
        al::SubActorInfo* info = keeper->getSubActorInfo(i);

        if (!isSyncAnimParts(info)) {
            continue;
        }

        if (mIsUpperBodyAnim && isHandParts(info->mSubActor)) {
            continue;
        }

        al::LiveActor* parts = info->mSubActor;

        if (!al::isActionPlaying(parts, actionName)) {
            continue;
        }

        if (al::isSklAnimExist(parts) && al::isSklAnimPlaying(parts, 0)) {
            al::setSklAnimBlendFrameRateAll(parts, rate, true);
        }

        if (al::isMtpAnimExist(parts) && al::isMtpAnimPlaying(parts)) {
            al::setMtpAnimFrameRate(parts, rateRatio * al::getMtpAnimFrameMax(parts));
        }

        if (al::isVisAnimExist(parts) && al::isVisAnimPlaying(parts)) {
            al::setVisAnimFrameRate(parts, rateRatio * al::getVisAnimFrameMax(parts));
        }
    }
}

/**
 * @brief Sets the frame of the parts playing the model's animation.
 * @param frame Frame.
 */
void PlayerAnimator::setPartsFrame(f32 frame) {
    const char* actionName = al::getActionName(getModel());
    f32 frameMax = al::getActionFrameMax(getModel(), actionName);
    al::SubActorKeeper* keeper = getModel()->getSubActorKeeper();

    if (keeper == nullptr) {
        return;
    }

    s32 num = keeper->getSubActorNum();
    f32 frameRatio = frame / frameMax;
    for (s32 i = 0; i < num; i++) {
        al::SubActorInfo* info = keeper->getSubActorInfo(i);

        if (!isSyncAnimParts(info)) {
            continue;
        }

        if (mIsUpperBodyAnim && isHandParts(info->mSubActor)) {
            continue;
        }

        al::LiveActor* parts = info->mSubActor;

        if (!al::isActionPlaying(parts, actionName)) {
            continue;
        }

        if (al::isSklAnimExist(parts) && al::isSklAnimPlaying(parts, 0)) {
            al::setSklAnimBlendFrameAll(parts, frame, true);
        }

        if (al::isMtpAnimExist(parts) && al::isMtpAnimPlaying(parts)) {
            al::setMtpAnimFrame(parts, frameRatio * al::getMtpAnimFrameMax(parts));
        }

        if (al::isVisAnimExist(parts) && al::isVisAnimPlaying(parts)) {
            al::setVisAnimFrame(parts, frameRatio * al::getVisAnimFrameMax(parts));
        }
    }
}

/**
 * @brief Plays another animation in place of the main one, keeping its frame and rate.
 * @param pModel Model whose animation frame and rate are kept.
 * @param rName Replacing animation name.
 * @param retargettingType Retargetting info to use.
 */
void PlayerAnimator::replaceAnim(PlayerModel* pModel, const sead::SafeString& rName,
                                 s32 retargettingType) {
    f32 rate = al::getActionFrameRate(pModel);
    f32 frame = al::getActionFrame(pModel);

    if (mIsSubAnimBinding) {
        return;
    }

    startAnimCommon(rName, retargettingType);
    setAnimRate(rate);
    setAnimFrame(frame);
}

/**
 * @brief Goes back to the main animation after a replacement.
 */
void PlayerAnimator::revertAnim() {
    if (!mIsClimbMoveAsWalk) {
        return;
    }

    restartMainAnim();
    mIsClimbMoveAsWalk = false;
}
