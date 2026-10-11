#include "Player/Normal/PlayerModelHolder.hpp"

#include <cstring>

#include "Library/LiveActor/SubActorKeeper.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Player/Normal/PlayerModelHair.hpp"
#include "Player/Normal/PlayerModelIK.hpp"

/**
 * @brief Constructs the holder.
 * @param modelNum Number of model slots (one per figure).
 */
PlayerModelHolder::PlayerModelHolder(u32 modelNum) : mModelNum(modelNum) {
    mModels = new PlayerModel*[modelNum];
    memset(mModels, 0, sizeof(PlayerModel*) * modelNum);
}

/**
 * @brief Registers the model of a figure and initializes its joint controller.
 * @param index Figure index.
 * @param pModel Model of the figure.
 */
void PlayerModelHolder::registerModel(s32 index, PlayerModel* pModel) {
    mModels[index] = pModel;

    f32 mashLimit;
    switch (index) {
    case 3:
    case 8:
    case 9:
        mashLimit = 20.0f;
        break;
    default:
        mashLimit = 50.0f;
        break;
    }

    pModel->initJointController(sead::Vector2f(-10.0f, mashLimit), &mIsMash, &mJointQuat, 0.15f);
}

/**
 * @brief Makes a figure slot use the model of another slot.
 * @param index Figure index to set.
 * @param srcIndex Figure index whose model is reused.
 */
void PlayerModelHolder::reuseModel(s32 index, s32 srcIndex) {
    mModels[index] = mModels[srcIndex];
}

/**
 * @brief Sets the current figure and makes its model appear.
 * @param index Figure index.
 */
void PlayerModelHolder::initCurrentModel(s32 index) {
    mCurrentIndex = index;
    appear();
}

/**
 * @brief Applies every display state (visibility, shadow, dynamics...) to the current model.
 */
inline void PlayerModelHolder::updateAll() {
    updateModelShowHide();
    updateInvincible();
    updateShadow();
    updateSilhouette();
    updateIK();
    updateHairCtrl();
    updateSkirtDynamics();
    updateTailDynamics();
    updateHairDynamics();
    updateShadowLength();
}

/**
 * @brief Makes the current model appear (if the holder isn't alive yet).
 */
void PlayerModelHolder::appear() {
    if (mIsAlive) {
        return;
    }

    mIsAlive = true;
    getCurrentModel()->makeActorAppeared();
    updateAll();
}

/**
 * @brief Makes every model use a global alpha value.
 * @param pAlpha Pointer to the alpha value.
 */
void PlayerModelHolder::assignGlobalAlpha(f32* pAlpha) {
    for (u32 i = 0; i < mModelNum; i++) {
        if (mModels[i] != nullptr) {
            mModels[i]->setGlobalAlphaPtr(pAlpha);
        }
    }
}

/**
 * @brief Shows or hides the current model depending on the alive and hidden states.
 */
void PlayerModelHolder::updateModelShowHide() {
    if (mIsAlive && !mIsHidden) {
        if (al::isHideModel(getCurrentModel())) {
            al::showModel(getCurrentModel());
        }
    } else {
        if (!al::isHideModel(getCurrentModel())) {
            al::hideModel(getCurrentModel());
        }
    }
}

/**
 * @brief Shows or hides the invincible model of the current model.
 */
void PlayerModelHolder::updateInvincible() {
    if (mIsAlive && !mIsHidden && mIsInvincible) {
        al::showInvincibleModel(getCurrentModel());
    } else {
        al::hideInvincibleModel(getCurrentModel());
    }
}

/**
 * @brief Shows or hides the shadows (normal, circle and wall snap) of the current model.
 */
void PlayerModelHolder::updateShadow() {
    if (mIsCircleShadow) {
        al::hideShadow(getCurrentModel());
        hideSubActorShadow();

        if (al::isExistShadow(getCurrentModel(), "Circle") && mIsAlive && !mIsHidden &&
            !mIsShadowHidden) {
            al::showShadow(getCurrentModel(), "Circle");
        }

        return;
    }

    if (mCurrentIndex == 3 || mCurrentIndex == 7) {
        if (mIsAlive && !mIsHidden && !mIsShadowHidden) {
            if (mIsWallSnap) {
                al::hideShadow(getCurrentModel());
                hideSubActorShadow();
                al::showShadow(getCurrentModel(), "WallSnap");
            } else {
                al::showShadow(getCurrentModel());
                showSubActorShadow();
                al::hideShadow(getCurrentModel(), "WallSnap");
                al::hideShadow(getCurrentModel(), "WallSnap");
            }
        } else {
            al::hideShadow(getCurrentModel());
            hideSubActorShadow();
        }
    } else {
        if (al::isExistShadow(getCurrentModel(), "Circle")) {
            al::hideShadow(getCurrentModel(), "Circle");
        }

        if (mIsAlive && !mIsHidden && !mIsShadowHidden) {
            if (al::isHideShadow(getCurrentModel())) {
                al::showShadow(getCurrentModel());
            }

            showSubActorShadow();
        } else {
            if (!al::isHideShadow(getCurrentModel())) {
                al::hideShadow(getCurrentModel());
            }

            hideSubActorShadow();
        }
    }

    if (al::isExistShadow(getCurrentModel(), "Circle")) {
        al::hideShadow(getCurrentModel(), "Circle");
    }
}

/**
 * @brief Shows or hides the silhouette of the current model.
 */
void PlayerModelHolder::updateSilhouette() {
    bool isHidden = mIsSilhouetteHidden;
    bool isModelHidden = al::isSilhouetteModelHidden(getCurrentModel());

    if (isHidden) {
        if (!isModelHidden) {
            al::hideSilhouetteModel(getCurrentModel());
        }
    } else {
        if (isModelHidden) {
            al::showSilhouetteModel(getCurrentModel());
        }
    }
}

/**
 * @brief Enables or disables the foot IK of the current model.
 */
void PlayerModelHolder::updateIK() {
    PlayerModelIK* ik = getCurrentModel()->getIK();
    if (ik == nullptr) {
        return;
    }

    if (mIsValidIK) {
        ik->validate();
    } else {
        ik->invalidate();
    }
}

/**
 * @brief Enables or disables the hair control of the current model.
 */
void PlayerModelHolder::updateHairCtrl() {
    PlayerModelHair* hairCtrl = getCurrentModel()->getHairCtrl();
    if (hairCtrl == nullptr) {
        return;
    }

    if (mIsValidHairCtrl) {
        hairCtrl->validate();
    } else {
        hairCtrl->invalidate();
    }
}

/**
 * @brief Enables or disables the skirt dynamics of the current model.
 */
void PlayerModelHolder::updateSkirtDynamics() {
    PlayerModel* model = getCurrentModel();
    if (!model->isExistSkirtDynamics()) {
        return;
    }

    if (mIsValidSkirtDynamics) {
        if (!model->isValidSkirtDynamics()) {
            model->validateSkirtDynamics();
        }
    } else {
        if (model->isValidSkirtDynamics()) {
            model->invalidateSkirtDynamics();
        }
    }
}

/**
 * @brief Enables or disables the tail dynamics of the current model.
 */
void PlayerModelHolder::updateTailDynamics() {
    PlayerModel* model = getCurrentModel();
    if (!model->isExistTailDynamics()) {
        return;
    }

    if (mIsValidTailDynamics) {
        if (!model->isValidTailDynamics()) {
            model->validateTailDynamics();
        }
    } else {
        if (model->isValidTailDynamics()) {
            model->invalidateTailDynamics();
        }
    }
}

/**
 * @brief Enables or disables the hair dynamics of the current model.
 */
void PlayerModelHolder::updateHairDynamics() {
    PlayerModel* model = getCurrentModel();
    if (!model->isExistHairDynamics()) {
        return;
    }

    if (mIsValidHairDynamics) {
        if (!model->isValidHairDynamics()) {
            model->validateHairDynamics();
        }
    } else {
        if (model->isValidHairDynamics()) {
            model->invalidateHairDynamics();
        }
    }
}

/**
 * @brief Applies the shadow length to the current model (once one has been set).
 */
void PlayerModelHolder::updateShadowLength() {
    if (mIsSetShadowLength) {
        getCurrentModel()->setShadowLength(mShadowLength);
    }
}

/**
 * @brief Kills the current model (if the holder is alive).
 */
void PlayerModelHolder::kill() {
    if (!mIsAlive) {
        return;
    }

    mIsAlive = false;
    getCurrentModel()->makeActorDead();
    updateAll();
}

/**
 * @brief Switches to the model of another figure, carrying over its pose.
 * @param index Figure index to switch to.
 */
void PlayerModelHolder::change(s32 index) {
    if (mCurrentIndex == index) {
        return;
    }

    al::setTrans(mModels[index], al::getTrans(getCurrentModel()));
    al::updatePoseRotate(mModels[index], al::getRotate(getCurrentModel()));
    al::resetPosition(mModels[index], false);
    al::setScale(mModels[index], al::getScale(getCurrentModel()));

    if (!mIsAlive) {
        mCurrentIndex = index;
        return;
    }

    if (al::isHideShadow(getCurrentModel())) {
        al::showShadow(getCurrentModel());
    }

    al::tryUpdateEffectMaterialCode(getCurrentModel(), "NoCode");
    getCurrentModel()->makeActorDead();
    al::hideInvincibleModel(getCurrentModel());

    mCurrentIndex = index;
    mModels[index]->makeActorAppeared();
    getCurrentModel()->calcAnim();
    updateAll();
}

/**
 * @brief Shows the current model (if it is hidden).
 */
void PlayerModelHolder::show() {
    if (!mIsHidden) {
        return;
    }

    mIsHidden = false;
    updateAll();
}

/**
 * @brief Hides the current model (if it is shown).
 */
void PlayerModelHolder::hide() {
    if (mIsHidden) {
        return;
    }

    mIsHidden = true;
    updateAll();
}

/**
 * @brief Checks if the model is hidden.
 * @return True if hidden.
 */
bool PlayerModelHolder::isHidden() const {
    return mIsHidden;
}

/**
 * @brief Shows the silhouette (if it is hidden).
 */
void PlayerModelHolder::showSilhouette() {
    if (!mIsSilhouetteHidden) {
        return;
    }

    mIsSilhouetteHidden = false;
    updateSilhouette();
}

/**
 * @brief Hides the silhouette (if it is shown).
 */
void PlayerModelHolder::hideSilhouette() {
    if (mIsSilhouetteHidden) {
        return;
    }

    mIsSilhouetteHidden = true;
    updateSilhouette();
}

/**
 * @brief Checks if the silhouette is hidden.
 * @return True if hidden.
 */
bool PlayerModelHolder::isSilhouetteHidden() const {
    return mIsSilhouetteHidden;
}

/**
 * @brief Shows the shadow.
 */
void PlayerModelHolder::showShadow() {
    mIsShadowHidden = false;
    updateShadow();
}

/**
 * @brief Hides the shadow.
 */
void PlayerModelHolder::hideShadow() {
    mIsShadowHidden = true;
    updateShadow();
}

/**
 * @brief Enables the mash joint control.
 */
void PlayerModelHolder::validateMash() {
    mIsMash = true;
}

/**
 * @brief Disables the mash joint control.
 */
void PlayerModelHolder::invalidateMash() {
    mIsMash = false;
}

/**
 * @brief Starts showing the invincible model.
 */
void PlayerModelHolder::startInvincible() {
    mIsInvincible = true;
    updateInvincible();
}

/**
 * @brief Sets the color of the current model's invincible effect.
 * @param rColor Color.
 */
void PlayerModelHolder::setInvincibleColor(const sead::Color4f& rColor) {
    getCurrentModel()->setInvincibleColor(rColor);
}

/**
 * @brief Stops showing the invincible model.
 */
void PlayerModelHolder::endInvincible() {
    mIsInvincible = false;
    updateInvincible();
}

/**
 * @brief Switches to the wall snap shadow.
 */
void PlayerModelHolder::startWallSnap() {
    mIsWallSnap = true;
    updateShadow();
}

/**
 * @brief Switches back from the wall snap shadow.
 */
void PlayerModelHolder::endWallSnap() {
    mIsWallSnap = false;
    updateShadow();
}

/**
 * @brief Enables the foot IK.
 */
void PlayerModelHolder::validateIK() {
    mIsValidIK = true;
    updateIK();
}

/**
 * @brief Disables the foot IK.
 */
void PlayerModelHolder::invalidateIK() {
    mIsValidIK = false;
    updateIK();
}

/**
 * @brief Shows the fur of every model.
 */
void PlayerModelHolder::showFur() {
    for (u32 i = 0; i < mModelNum; i++) {
        if (mModels[i] != nullptr) {
            mModels[i]->showFur();
        }
    }
}

/**
 * @brief Hides the fur of every model.
 */
void PlayerModelHolder::hideFur() {
    for (u32 i = 0; i < mModelNum; i++) {
        if (mModels[i] != nullptr) {
            mModels[i]->hideFur();
        }
    }
}

/**
 * @brief Enables the hair control.
 */
void PlayerModelHolder::validateHairCtrl() {
    mIsValidHairCtrl = true;
    updateHairCtrl();
}

/**
 * @brief Disables the hair control.
 */
void PlayerModelHolder::invalidateHairCtrl() {
    mIsValidHairCtrl = false;
    updateHairCtrl();
}

/**
 * @brief Enables the skirt dynamics.
 */
void PlayerModelHolder::validateSkirtDynamics() {
    mIsValidSkirtDynamics = true;
    updateSkirtDynamics();
}

/**
 * @brief Disables the skirt dynamics.
 */
void PlayerModelHolder::invalidateSkirtDynamics() {
    mIsValidSkirtDynamics = false;
    updateSkirtDynamics();
}

/**
 * @brief Resets the skirt dynamics of the current model (if it has any).
 */
void PlayerModelHolder::resetSkirtDynamics() {
    PlayerModel* model = getCurrentModel();
    if (model->isExistSkirtDynamics()) {
        model->resetSkirtDynamics();
    }
}

/**
 * @brief Enables the tail dynamics.
 */
void PlayerModelHolder::validateTailDynamics() {
    mIsValidTailDynamics = true;
    updateTailDynamics();
}

/**
 * @brief Disables the tail dynamics.
 */
void PlayerModelHolder::invalidateTailDynamics() {
    mIsValidTailDynamics = false;
    updateTailDynamics();
}

/**
 * @brief Resets the tail dynamics of the current model (if it has any).
 */
void PlayerModelHolder::resetTailDynamics() {
    PlayerModel* model = getCurrentModel();
    if (model->isExistTailDynamics()) {
        model->resetTailDynamics();
    }
}

/**
 * @brief Enables the hair dynamics.
 */
void PlayerModelHolder::validateHairDynamics() {
    mIsValidHairDynamics = true;
    updateHairDynamics();
}

/**
 * @brief Disables the hair dynamics.
 */
void PlayerModelHolder::invalidateHairDynamics() {
    mIsValidHairDynamics = false;
    updateHairDynamics();
}

/**
 * @brief Resets the hair dynamics of the current model (if it has any).
 */
void PlayerModelHolder::resetHairDynamics() {
    PlayerModel* model = getCurrentModel();
    if (model->isExistHairDynamics()) {
        model->resetHairDynamics();
    }
}

/**
 * @brief Sets the shadow length and applies it to the current model.
 * @param length Shadow length.
 */
void PlayerModelHolder::setShadowLength(f32 length) {
    mIsSetShadowLength = true;
    mShadowLength = length;
    getCurrentModel()->setShadowLength(length);
}

/**
 * @brief Switches to the circle shadow.
 */
void PlayerModelHolder::validateCircleShadow() {
    mIsCircleShadow = true;
    updateShadow();
}

/**
 * @brief Switches back from the circle shadow.
 */
void PlayerModelHolder::invalidateCircleShadow() {
    mIsCircleShadow = false;
    updateShadow();
}

/**
 * @brief Disables the shadows of the current model's sub actors.
 */
void PlayerModelHolder::hideSubActorShadow() {
    al::SubActorKeeper* keeper = getCurrentModel()->getSubActorKeeper();
    if (keeper == nullptr) {
        return;
    }

    s32 num = keeper->getSubActorNum();
    for (s32 i = 0; i < num; i++) {
        al::SubActorInfo* info = keeper->getSubActorInfo(i);
        if ((info->mSyncType & 8) != 0) {
            continue;
        }

        if (al::isExistShadow(info->mSubActor)) {
            al::invalidateShadow(info->mSubActor);
        }
    }
}

/**
 * @brief Enables the shadows of the current model's sub actors.
 */
void PlayerModelHolder::showSubActorShadow() {
    al::SubActorKeeper* keeper = getCurrentModel()->getSubActorKeeper();
    if (keeper == nullptr) {
        return;
    }

    s32 num = keeper->getSubActorNum();
    for (s32 i = 0; i < num; i++) {
        al::SubActorInfo* info = keeper->getSubActorInfo(i);
        if ((info->mSyncType & 8) != 0) {
            continue;
        }

        if (al::isExistShadow(info->mSubActor)) {
            al::validateShadow(info->mSubActor);
        }
    }
}
