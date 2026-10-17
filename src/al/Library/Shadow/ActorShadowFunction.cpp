#include "Library/Shadow/Common/ShadowUtil.hpp"

#include <agl/shadow/aglDepthShadow.h>
#include <attributes.h>
#include <math/seadGeometry.h>

#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Execute/ActorExecuteInfo.hpp"
#include "Library/Light/DirectionParam.hpp"
#include "Library/Light/PlaneParam.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Model/ModelDrawerBase.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Shadow/DepthShadowClip.hpp"
#include "Library/Shadow/DepthShadowParam.hpp"
#include "Library/Shadow/ShadowKeeper.hpp"
#include "Library/Shadow/ShadowMaskBase.hpp"
#include "Library/Shadow/ShadowMaskCube.hpp"
#include "Library/Shadow/ShadowMaskDrawer.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaShape.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Draw/GraphicsAreaDirector.hpp"

namespace al {
bool tryGetArg(f32* pValue, const ActorInitInfo& rInfo, const char* pKey);

/**
 * Checks whether the actor has at least one shadow mask.
 * @param pActor Actor.
 * @return Whether the actor has a shadow mask.
 */
bool isExistShadow(LiveActor* pActor) {
    ShadowKeeper* keeper = pActor->getShadowKeeper();

    if (keeper == nullptr) {
        return false;
    }

    return keeper->getShadowMaskNum() > 0;
}

/**
 * Checks whether the actor has a shadow mask with the given name.
 * @param pActor Actor.
 * @param pName Shadow mask name.
 * @return Whether the shadow mask exists.
 */
bool isExistShadow(LiveActor* pActor, const char* pName) {
    ShadowKeeper* keeper = pActor->getShadowKeeper();

    if (keeper == nullptr) {
        return false;
    }

    return keeper->findShadowMask(pName) != nullptr;
}

/**
 * Checks whether all shadow masks of the actor are hidden.
 * @param pActor Actor.
 * @return Whether the shadow is hidden.
 */
bool isHideShadow(const LiveActor* pActor) {
    return pActor->getShadowKeeper()->isHide();
}

/**
 * Hides all shadow masks of the actor.
 * @param pActor Actor.
 */
void hideShadow(LiveActor* pActor) {
    pActor->getShadowKeeper()->hide();
}

/**
 * Shows all shadow masks of the actor.
 * @param pActor Actor.
 */
void showShadow(LiveActor* pActor) {
    pActor->getShadowKeeper()->show();
}

/**
 * Removes the actor's model from all depth shadow drawers.
 * @param pActor Actor.
 */
void hideShadowDepth(LiveActor* pActor) {
    ActorExecuteInfo* info = pActor->getExecuteInfo();
    s32 drawerNum = info->getDrawerCount();

    if (drawerNum < 1) {
        return;
    }

    alModelCafe* model = pActor->getModelKeeper()->getModelCafe();

    for (s32 i = 0; i < drawerNum; i++) {
        ModelDrawerBase* drawer = info->getDrawer(i);

        if (drawer->isDepthShadowDrawer()) {
            drawer->removeModel(model);
        }
    }
}

/**
 * Adds the actor's model to all depth shadow drawers.
 * @param pActor Actor.
 */
void showShadowDepth(LiveActor* pActor) {
    ActorExecuteInfo* info = pActor->getExecuteInfo();
    s32 drawerNum = info->getDrawerCount();

    if (drawerNum < 1) {
        return;
    }

    alModelCafe* model = pActor->getModelKeeper()->getModelCafe();

    for (s32 i = 0; i < drawerNum; i++) {
        ModelDrawerBase* drawer = info->getDrawer(i);

        if (drawer->isDepthShadowDrawer()) {
            drawer->addModel(model);
        }
    }
}

/**
 * Shows a shadow mask.
 * @param pActor Host actor.
 * @param pMask Shadow mask.
 */
void showShadow(LiveActor* pActor, ShadowMaskBase* pMask) {
    if (!pMask->isHide()) {
        return;
    }

    pMask->setHide(false);

    if (pMask->isValid()) {
        ShadowMaskFunction::getShadowMaskKeeper(pActor)->registerShadowMask(pMask);
    }
}

/**
 * Shows a shadow mask by name.
 * @param pActor Host actor.
 * @param pName Shadow mask name.
 */
void showShadow(LiveActor* pActor, const char* pName) {
    ShadowMaskBase* mask = pActor->getShadowKeeper()->findShadowMask(pName);

    if (mask != nullptr) {
        showShadow(pActor, mask);
    }
}

/**
 * Shows all shadow masks of a draw category.
 * @param pActor Host actor.
 * @param category Draw category.
 */
void showShadow(LiveActor* pActor, ShadowMaskDrawCategory category) {
    for (s32 i = 0; i < pActor->getShadowKeeper()->getShadowMaskNum(); i++) {
        ShadowMaskBase* mask = pActor->getShadowKeeper()->tryGetShadowMask(i);

        if (mask != nullptr && mask->getDrawCategory() == category) {
            showShadow(pActor, mask);
        }
    }
}

/**
 * Hides a shadow mask.
 * @param pActor Host actor.
 * @param pMask Shadow mask.
 */
void hideShadow(LiveActor* pActor, ShadowMaskBase* pMask) {
    if (pMask->isHide() || pMask->isIgnoreHide()) {
        return;
    }

    pMask->setHide(true);

    if (pMask->isValid()) {
        ShadowMaskFunction::getShadowMaskKeeper(pActor)->removeShadowMask(pMask);
    }
}

/**
 * Hides a shadow mask by name.
 * @param pActor Host actor.
 * @param pName Shadow mask name.
 */
void hideShadow(LiveActor* pActor, const char* pName) {
    ShadowMaskBase* mask = pActor->getShadowKeeper()->findShadowMask(pName);

    if (mask != nullptr) {
        hideShadow(pActor, mask);
    }
}

/**
 * Hides all shadow masks of a draw category.
 * @param pActor Host actor.
 * @param category Draw category.
 */
void hideShadow(LiveActor* pActor, ShadowMaskDrawCategory category) {
    for (s32 i = 0; i < pActor->getShadowKeeper()->getShadowMaskNum(); i++) {
        ShadowMaskBase* mask = pActor->getShadowKeeper()->tryGetShadowMask(i);

        if (mask != nullptr && mask->getDrawCategory() == category) {
            hideShadow(pActor, mask);
        }
    }
}

/**
 * Checks whether a shadow mask is hidden.
 * @param pActor Host actor.
 * @param pName Shadow mask name.
 * @return Whether the shadow mask is hidden.
 */
bool isHideShadow(LiveActor* pActor, const char* pName) {
    return pActor->getShadowKeeper()->findShadowMask(pName)->isHide();
}

/**
 * Validates a shadow mask.
 * @param pActor Host actor.
 * @param pMask Shadow mask.
 */
void validateShadow(LiveActor* pActor, ShadowMaskBase* pMask) {
    if (pMask->isValid()) {
        return;
    }

    pMask->setValid(true);

    if (!pMask->isHide()) {
        ShadowMaskFunction::getShadowMaskKeeper(pActor)->registerShadowMask(pMask);
    }
}

/**
 * Invalidates a shadow mask.
 * @param pActor Host actor.
 * @param pMask Shadow mask.
 */
void invalidateShadow(LiveActor* pActor, ShadowMaskBase* pMask) {
    if (!pMask->isValid()) {
        return;
    }

    pMask->setValid(false);

    if (!pMask->isHide()) {
        ShadowMaskFunction::getShadowMaskKeeper(pActor)->removeShadowMask(pMask);
    }
}

/**
 * Validates a shadow mask by name.
 * @param pActor Host actor.
 * @param pName Shadow mask name.
 */
void validateShadow(LiveActor* pActor, const char* pName) {
    ShadowMaskBase* mask = pActor->getShadowKeeper()->findShadowMask(pName);

    if (mask != nullptr) {
        validateShadow(pActor, mask);
    }
}

/**
 * Invalidates a shadow mask by name.
 * @param pActor Host actor.
 * @param pName Shadow mask name.
 */
void invalidateShadow(LiveActor* pActor, const char* pName) {
    ShadowMaskBase* mask = pActor->getShadowKeeper()->findShadowMask(pName);

    if (mask != nullptr) {
        invalidateShadow(pActor, mask);
    }
}

/**
 * Hides and invalidates all shadow masks of a draw category.
 * @param pActor Host actor.
 * @param category Draw category.
 */
void invalidateShadow(LiveActor* pActor, ShadowMaskDrawCategory category) {
    for (s32 i = 0; i < pActor->getShadowKeeper()->getShadowMaskNum(); i++) {
        ShadowMaskBase* mask = pActor->getShadowKeeper()->tryGetShadowMask(i);

        if (mask != nullptr && mask->getDrawCategory() == category) {
            hideShadow(pActor, mask);
            mask->setValid(false);
        }
    }
}

/**
 * Hides and invalidates all shadow masks of the shadow draw categories.
 * @param pActor Host actor.
 */
void invalidateShadowIntensityAll(LiveActor* pActor) {
    for (s32 i = 0; i < ShadowMaskDrawCategory::LightScale; i++) {
        invalidateShadow(pActor, ShadowMaskDrawCategory(i));
    }
}

/**
 * Sets whether the shadow matrices of all shadow masks are fixed.
 * @param pActor Host actor.
 * @param isFixed Whether the shadow is fixed.
 */
void setShadowFixed(LiveActor* pActor, bool isFixed) {
    ShadowKeeper* keeper = pActor->getShadowKeeper();

    if (keeper == nullptr) {
        return;
    }

    for (s32 i = 0; i < keeper->getShadowMaskNum(); i++) {
        ShadowMaskBase* mask = keeper->mMaskArray.at(i);

        if (mask != nullptr) {
            mask->mIsShadowFixed = isFixed;
        }
    }
}

/**
 * Sets the drop direction of all shadow masks.
 * @param pActor Host actor.
 * @param rDir Drop direction.
 */
void setShadowDropDir(LiveActor* pActor, const sead::Vector3f& rDir) {
    for (s32 i = 0; i < pActor->getShadowKeeper()->getShadowMaskNum(); i++) {
        ShadowMaskBase* mask = pActor->getShadowKeeper()->tryGetShadowMask(i);

        if (mask != nullptr) {
            mask->setDropDir(rDir);
        }
    }
}

/**
 * Sets the drop direction of a shadow mask.
 * @param pActor Host actor.
 * @param rDir Drop direction.
 * @param pName Shadow mask name.
 */
void setShadowDropDir(LiveActor* pActor, const sead::Vector3f& rDir, const char* pName) {
    ShadowMaskBase* mask = pActor->getShadowKeeper()->findShadowMask(pName);

    if (mask != nullptr) {
        mask->setDropDir(rDir);
    }
}

/**
 * Sets the drop direction of all shadow masks to the actor's down direction.
 * @param pActor Host actor.
 */
void setShadowDropDirActorDown(LiveActor* pActor) {
    sead::Vector3f up;
    calcUpDir(&up, pActor);
    setShadowDropDir(pActor, -up);
}

/**
 * Sets the size of a cube or cylinder shadow mask.
 * @param pActor Host actor.
 * @param pName Shadow mask name.
 * @param rSize Size.
 */
void setShadowMaskSize(LiveActor* pActor, const char* pName, const sead::Vector3f& rSize) {
    setShadowMaskSize(pActor, pName, rSize.x, rSize.y, rSize.z);
}

/**
 * Sets the size of a cube or cylinder shadow mask.
 * @param pActor Host actor.
 * @param pName Shadow mask name.
 * @param x Size in x.
 * @param y Size in y.
 * @param z Size in z.
 */
void setShadowMaskSize(LiveActor* pActor, const char* pName, f32 x, f32 y, f32 z) {
    ShadowMaskBase* mask = pActor->getShadowKeeper()->findShadowMask(pName);

    if (mask == nullptr) {
        return;
    }

    if (mask->getShadowMaskType() == ShadowMaskType::Cube) {
        auto* cube = static_cast<ShadowMaskCube*>(mask);
        cube->mScale.x = x;
        cube->mScale.z = z;
    } else if (mask->getShadowMaskType() == ShadowMaskType::Cylinder) {
        static_cast<ShadowMaskCube*>(mask)->mScale.x = x;
    }
}

/**
 * Calculates the size of a cube or cylinder shadow mask.
 * @param pOut Output size.
 * @param pActor Host actor.
 * @param pName Shadow mask name.
 */
void calcShadowMaskSize(sead::Vector3f* pOut, LiveActor* pActor, const char* pName) {
    ShadowMaskBase* mask = pActor->getShadowKeeper()->findShadowMask(pName);

    if (mask->getShadowMaskType() == ShadowMaskType::Cube) {
        auto* cube = static_cast<ShadowMaskCube*>(mask);
        pOut->x = cube->mScale.x;
        pOut->y = 0.0f;
        pOut->z = cube->mScale.z;
        return;
    }

    if (mask->getShadowMaskType() == ShadowMaskType::Cylinder) {
        f32 radius = static_cast<ShadowMaskCube*>(mask)->mScale.x;
        pOut->z = radius;
        pOut->y = 0.0f;
        pOut->x = radius;
    }
}

/**
 * Gets the drop length of a shadow mask.
 * @param pActor Host actor.
 * @param pName Shadow mask name.
 * @return Drop length.
 */
f32 getShadowDropLength(const LiveActor* pActor, const char* pName) {
    return pActor->getShadowKeeper()->findShadowMask(pName)->mDropLength;
}

/**
 * Sets the drop length of all shadow masks.
 * @param pActor Host actor.
 * @param length Drop length.
 */
void setShadowDropLength(LiveActor* pActor, f32 length) {
    for (s32 i = 0; i < pActor->getShadowKeeper()->getShadowMaskNum(); i++) {
        ShadowMaskBase* mask = pActor->getShadowKeeper()->tryGetShadowMask(i);

        if (mask != nullptr) {
            mask->setDropLength(length);
        }
    }
}

/**
 * Sets the drop length of a shadow mask.
 * @param pActor Host actor.
 * @param length Drop length.
 * @param pName Shadow mask name.
 */
void setShadowDropLength(LiveActor* pActor, f32 length, const char* pName) {
    ShadowMaskBase* mask = pActor->getShadowKeeper()->findShadowMask(pName);

    if (mask != nullptr) {
        mask->setDropLength(length);
    }
}

/**
 * Scales the drop length of all shadow masks of a draw category.
 * @param pActor Host actor.
 * @param scale Drop length scale.
 * @param category Draw category.
 */
void setShadowDropLengthScaleWithDrawCategory(LiveActor* pActor, f32 scale,
                                              ShadowMaskDrawCategory category) {
    for (s32 i = 0; i < pActor->getShadowKeeper()->getShadowMaskNum(); i++) {
        ShadowMaskBase* mask = pActor->getShadowKeeper()->tryGetShadowMask(i);

        if (mask != nullptr && mask->getDrawCategory() == category) {
            mask->mDropLength *= scale;
        }
    }
}

/**
 * Sets the drop length of all shadow masks of a draw category.
 * @param pActor Host actor.
 * @param length Drop length.
 * @param category Draw category.
 */
void setShadowDropLengthWithDrawCategory(LiveActor* pActor, f32 length,
                                         ShadowMaskDrawCategory category) {
    for (s32 i = 0; i < pActor->getShadowKeeper()->getShadowMaskNum(); i++) {
        ShadowMaskBase* mask = pActor->getShadowKeeper()->tryGetShadowMask(i);

        if (mask != nullptr && mask->getDrawCategory() == category) {
            mask->setDropLength(length);
        }
    }
}

/**
 * Sets the drop length of a shadow mask so that it ends at another shadow mask of the host.
 * @param pMask Shadow mask.
 * @param pTargetName Name of the target shadow mask.
 * @param rPos Plane normal.
 */
void setShadowDropLengthEvenWithTarget(ShadowMaskBase* pMask, const char* pTargetName,
                                       const sead::Vector3f& rPos) {
    ShadowMaskBase* target = pMask->getHost()->getShadowKeeper()->findShadowMask(pTargetName);
    setShadowDropLengthEvenWithTarget(pMask, target, rPos);
}

/**
 * Sets the drop length of a shadow mask so that it ends at another shadow mask.
 * @param pMask Shadow mask.
 * @param pTarget Target shadow mask.
 * @param rNormal Plane normal.
 */
void setShadowDropLengthEvenWithTarget(ShadowMaskBase* pMask, const ShadowMaskBase* pTarget,
                                       const sead::Vector3f& rNormal) {
    if (pTarget == nullptr || pTarget == pMask) {
        return;
    }

    sead::Vector3f trans;
    pMask->mShadowMtx.getTranslation(trans);
    const sead::Matrix34f& mtx = pMask->mShadowMtx;
    sead::Vector3f up(mtx.m[0][1], mtx.m[1][1], mtx.m[2][1]);

    if (isNearZero(up, 0.001f) || isNearZero(up.dot(rNormal), 0.001f)) {
        pMask->setDropLength(0.0f);
        return;
    }

    trans += up * 0.5f;

    sead::Vector3f targetUp;
    pTarget->mShadowMtx.getBase(targetUp, 1);
    sead::Vector3f targetTrans;
    pTarget->mShadowMtx.getTranslation(targetTrans);
    targetTrans -= targetUp * 0.5f;
    targetUp.length();

    f32 length = calcDistanceVecToPlane(-up, trans, rNormal, targetTrans);

    if (length < 0.0f) {
        return;
    }

    pMask->setDropLength(length);
}

/**
 * Sets the drop length of all shadow masks of a draw category so that they end at a shadow mask
 * of another actor.
 * @param pActor Host actor.
 * @param category Draw category.
 * @param pTargetActor Actor owning the target shadow mask.
 * @param pTargetName Name of the target shadow mask.
 */
void setShadowDropLengthEvenWithDrawCategory(LiveActor* pActor, ShadowMaskDrawCategory category,
                                             const LiveActor* pTargetActor,
                                             const char* pTargetName) {
    ShadowMaskBase* target = pTargetActor->getShadowKeeper()->findShadowMask(pTargetName);
    ShadowKeeper* keeper = pActor->getShadowKeeper();
    s32 maskNum = keeper->getShadowMaskNum();

    for (s32 i = 0; i < maskNum; i++) {
        ShadowMaskBase* mask = keeper->mMaskArray.at(i);

        if (mask != nullptr && mask->getDrawCategory() == category) {
            setShadowDropLengthEvenWithTarget(mask, target, sead::Vector3f::ey);
        }
    }
}

/**
 * Sets the plane normal used for the drop length of all shadow masks.
 * @param pActor Host actor.
 * @param rNormal Plane normal.
 */
void setShadowDropLengthEvenPlaneNormal(const LiveActor* pActor, const sead::Vector3f& rNormal) {
    for (s32 i = 0; i < pActor->getShadowKeeper()->getShadowMaskNum(); i++) {
        ShadowMaskBase* mask = pActor->getShadowKeeper()->tryGetShadowMask(i);

        if (mask != nullptr) {
            mask->mUp = rNormal;
        }
    }
}

/**
 * Gets the maximum drop length of all shadow masks.
 * @param pActor Host actor.
 * @return Maximum drop length.
 */
f32 getShadowDropLengthMax(const LiveActor* pActor) {
    ShadowKeeper* keeper = pActor->getShadowKeeper();
    f32 max = 0.0f;

    for (s32 i = 0; i < keeper->getShadowMaskNum(); i++) {
        f32 length = keeper->mMaskArray.unsafeAt(i)->mDropLength;

        if (max < length) {
            max = length;
        }
    }

    return max;
}

/**
 * Sets the user shadow intensity of a shadow mask.
 * @param pActor Host actor.
 * @param intensity Shadow intensity.
 * @param pName Shadow mask name.
 */
void setShadowIntensityUser(LiveActor* pActor, u8 intensity, const char* pName) {
    ShadowMaskBase* mask = pActor->getShadowKeeper()->findShadowMask(pName);

    if (mask != nullptr) {
        mask->mIsApplyShadowIntensityUser = true;
        mask->mShadowIntensityUser = intensity;
    }
}

/**
 * Gets the shadow intensity of a shadow mask.
 * @param pActor Host actor.
 * @param pName Shadow mask name.
 * @return Shadow intensity.
 */
f32 getShadowIntensity(LiveActor* pActor, const char* pName) {
    return pActor->getShadowKeeper()->findShadowMask(pName)->getShadowIntensity();
}

/**
 * Checks whether a shadow mask draws to an ambient occlusion category.
 * @param pMask Shadow mask.
 * @return Whether the category is an ambient occlusion category.
 */
bool isShadowMaskDrawCategoryAO(const ShadowMaskBase* pMask) {
    switch (pMask->getDrawCategory().value()) {
    case ShadowMaskDrawCategory::MapObjAO:
    case ShadowMaskDrawCategory::EnemyAO:
    case ShadowMaskDrawCategory::PlayerAO:
    case ShadowMaskDrawCategory::AllAO:
    case ShadowMaskDrawCategory::MapObjAOSO:
    case ShadowMaskDrawCategory::MapObjAndWaterAOSO:
    case ShadowMaskDrawCategory::EnemyAOSO:
    case ShadowMaskDrawCategory::PlayerAOSO:
    case ShadowMaskDrawCategory::AllAOSO:
        return true;
    default:
        return false;
    }
}

/**
 * Checks whether a shadow mask draws to a light scale category.
 * @param pMask Shadow mask.
 * @return Whether the category is a light scale category.
 */
bool isShadowMaskDrawCategoryLightScale(const ShadowMaskBase* pMask) {
    return pMask->getDrawCategory() == ShadowMaskDrawCategory::LightScale ||
           pMask->getDrawCategory() == ShadowMaskDrawCategory::LightScaleLight;
}

/**
 * Gets the fixed texture scale of a cube shadow mask.
 * @param pActor Host actor.
 * @param pName Shadow mask name.
 * @return Fixed texture scale.
 */
f32 getShadowTextureFixedScale(const LiveActor* pActor, const char* pName) {
    return static_cast<ShadowMaskCube*>(pActor->getShadowKeeper()->findShadowMask(pName))
        ->mTextureFixedScale;
}

/**
 * Sets the fixed texture scale of a cube shadow mask.
 * @param pActor Host actor.
 * @param pName Shadow mask name.
 * @param scale Fixed texture scale.
 */
void setShadowTextureFixedScale(const LiveActor* pActor, const char* pName, f32 scale) {
    static_cast<ShadowMaskCube*>(pActor->getShadowKeeper()->findShadowMask(pName))->mTextureFixedScale =
        scale;
}

/**
 * Gets the offset of a shadow mask.
 * @param pActor Host actor.
 * @param pName Shadow mask name.
 * @return Offset.
 */
const sead::Vector3f& getShadowMaskOffset(const LiveActor* pActor, const char* pName) {
    return pActor->getShadowKeeper()->findShadowMask(pName)->mOffset;
}

/**
 * Sets the offset of a shadow mask.
 * @param pActor Host actor.
 * @param rOffset Offset.
 * @param pName Shadow mask name.
 */
void setShadowMaskOffset(const LiveActor* pActor, const sead::Vector3f& rOffset,
                         const char* pName) {
    pActor->getShadowKeeper()->findShadowMask(pName)->mOffset.set(rOffset);
}

/**
 * Sets the drop length from the placement argument "ShadowLength" if it is positive.
 * @param pActor Host actor.
 * @param rInfo Actor init info.
 * @param pName Shadow mask name, or nullptr for all shadow masks.
 * @return Whether the drop length was set.
 */
bool trySetShadowLength(LiveActor* pActor, const ActorInitInfo& rInfo, const char* pName) {
    f32 length = -1.0f;
    tryGetArg(&length, rInfo, "ShadowLength");

    if (length > 0.0f) {
        if (pName != nullptr) {
            setShadowDropLength(pActor, length, pName);
        } else {
            setShadowDropLength(pActor, length);
        }

        return true;
    }

    return false;
}

/**
 * Registers the depth shadow parameters and sets their default values.
 */
void DepthShadowParam::init() {
    _270.init(100.0f, "Near", "Near", "Min=100,Max=100000", &mParamObj);
    _290.init(40000.0f, "Far", "Far", "Min=500,Max=40000", &mParamObj);
    mIntensity.init(1.0f, "Intensity", "Intensity", "Min=0,Max=1", &mParamObj);
    _30.init(false, "IsCalcClipVolume", "IsCalcClipVolume", &mParamObj);
    _50.init(false, "IsDrawPlayerShadow", "IsDrawPlayerShadow", &mParamObj);
    mIsDrawObjectShadow.init(false, "IsDrawObjectShadow", "IsDrawObjectShadow", &mParamObj);
    _90.init(true, "IsDrawCasterShadow", "IsDrawCasterShadow", &mParamObj);
    mIsDrawCasterShadowOnlyMap.init(false, "IsDrawCasterShadowOnlyMap",
                                    "IsDrawCasterShadowOnlyMap", &mParamObj);
    mIsDrawEffectShadow.init(false, "IsDrawEffectShadow", "IsDrawEffectShadow", &mParamObj);
    mIsShadowMap16UNorm.init(true, "IsShadowMap16UNorm", "IsShadowMap16UNorm", &mParamObj);
    mClipPlaneName.init(sead::FixedSafeString<64>("No Name"), "ClipPlaneName", "ClipPlaneName",
                        &mParamObj);
    mMatrixCalcType.init(0, "MatrixCalcType", "MatrixCalcType", "IsEnable=False", &mParamObj);
    mIsShadowMaskEnable.init(true, "IsShadowMaskEnable", "IsShadowMaskEnable", &mParamObj);
    _130.init(1, "DepthShadowType", "DepthShadowType", "Min=0, Max=10", &mParamObj);
    mPcfType.init(0, "PcfType", "PcfType", "Min=0, Max=2", &mParamObj);
    mPcfOffsetValue.init(0.0008f, "PcfOffsetValue", "PcfOffsetValue", "Min=0.0,Max=0.001",
                         &mParamObj);
    mIsEnableShadowDamp.init(false, "IsEnableShadowDamp", "IsEnableShadowDamp", &mParamObj);
    mShadowDampStart.init(4800.0f, "ShadowDampStart", "ShadowDampStart", "Min=0.f, Max=10000.f",
                          &mParamObj);
    mShadowDampEnd.init(5400.0f, "ShadowDampEnd", "ShadowDampEnd", "Min=0.f, Max=10000.f",
                        &mParamObj);
    mAddViewDepth.init(12.0f, "AddViewDepth", "AddViewDepth", "Min=-100.0f, Max=100.0f",
                       &mParamObj);
    mAddVariance.init(0.05f, "AddVariance", "AddVariance", "Min=-1.0f, Max=1.0f", &mParamObj);
    mPolygonOffset.init(0.0f, "PolygonOffset", "PolygonOffset", "Min=-100.0f, Max=100.0f",
                        &mParamObj);
    mPolygonSlopeScale.init(0.0f, "PolygonSlopeScale", "PolygonSlopeScale",
                            "Min=-100.0f, Max=100.0f", &mParamObj);
    mShadowMapSize.init(1024, "ShadowMapSize", "ShadowMapSize", "Min=64.0f, Max=1024.0f",
                        &mParamObj);
    mPrePassShadowMapSize.init(512, "PrePassShadowMapSize", "PrePassShadowMapSize",
                               "Min=64.0f, Max=1024.0f", &mParamObj);
    mShadowComposeType.init(0, "ShadowComposeType", "ShadowComposeType", "Min=0, Max=2",
                            &mParamObj);
    mDepthShadowFactorMin.init(0.5f, "DepthShadowFactorMin", "DepthShadowFactorMin",
                               "Min=0.0f, Max=1.0f", &mParamObj);
    mRepairIrradianceScale.init(0.5f, "RepairIrradianceScale", "RepairIrradianceScale",
                                "Min=0.0f, Max=1.0f", &mParamObj);

    mDirForLpp = new DirectionParam();
    mDirForLpp->initializeDir(-sead::Vector3f::ey, &mParamObj, "DirForLpp", "DirForLpp");

    mIsUsingShadowCamera.init(false, "IsUsingShadowCamera", "IsUsingShadowCamera", &mParamObj);
    mCameraPos.init(sead::Vector3f(0.0f, 0.0f, 10.0f), "CameraPos", "CameraPos",
                    "Min=-10000.0f, Max=10000.0f", &mParamObj);
    mCameraAt.init(sead::Vector3f(0.0f, 0.0f, 0.0f), "CameraAt", "CameraAt",
                   "Min=-10000.0f, Max=10000.0f", &mParamObj);
    mCameraUp.init(sead::Vector3f(0.0f, 1.0f, 0.0f), "CameraUp", "CameraUp",
                   "Min=-1.f, Max=1.0f", &mParamObj);
    mCameraNear.init(100.0f, "CameraNear", "CameraNear", "Min=0.0f, Max=1000.0f", &mParamObj);
    mCameraFar.init(10000.0f, "CameraFar", "CameraFar", "Min=0.0f, Max=100000.0f", &mParamObj);
    mCameraFovy.init(30.0f, "CameraFovy", "CameraFovy", "Min=0.0f, Max=90.0f", &mParamObj);
    mCameraAspect.init(16.0f / 9.0f, "CameraAspect", "CameraAspect", "Min=0.0f, Max=2.0f",
                       &mParamObj);
    mIsExpandSizeByDesign.init(false, "IsExpandSizeByDesign", "IsExpandSizeByDesign",
                               &mParamObj);
}

/**
 * Creates the name parameter and registers the depth shadow parameters.
 */
NamedDepthShadowParam::NamedDepthShadowParam() {
    mName.init(sead::FixedSafeString<64>("No Name"), "Name", "名前", &mParamObj);
    init();
}

/**
 * Compares two depth shadow parameters.
 * @param rOther Parameter to compare with.
 * @return Whether both parameters are (nearly) equal.
 */
bool DepthShadowParam::operator==(const DepthShadowParam& rOther) const {
    return isNearZero(*mAddViewDepth - *rOther.mAddViewDepth) &&
           isNearZero(*mAddVariance - *rOther.mAddVariance) &&
           isNearZero(*mPolygonOffset - *rOther.mPolygonOffset) &&
           isNearZero(*mPolygonSlopeScale - *rOther.mPolygonSlopeScale) &&
           isNearZero(*_270 - *rOther._270) && isNearZero(*_290 - *rOther._290) &&
           isNearZero(*mIntensity - *rOther.mIntensity) && *_30 == *rOther._30 &&
           *_50 == *rOther._50 && *mIsDrawObjectShadow == *rOther.mIsDrawObjectShadow &&
           *_90 == *rOther._90 &&
           *mIsDrawCasterShadowOnlyMap == *rOther.mIsDrawCasterShadowOnlyMap &&
           *mIsDrawEffectShadow == *rOther.mIsDrawEffectShadow &&
           *mIsShadowMap16UNorm == *rOther.mIsShadowMap16UNorm &&
           isEqualString(mClipPlaneName->cstr(), rOther.mClipPlaneName->cstr()) &&
           *mMatrixCalcType == *rOther.mMatrixCalcType &&
           *mIsShadowMaskEnable == *rOther.mIsShadowMaskEnable &&
           *mPcfType == *rOther.mPcfType &&
           isNearZero(*mPcfOffsetValue - *rOther.mPcfOffsetValue) &&
           *mShadowMapSize == *rOther.mShadowMapSize &&
           *mPrePassShadowMapSize == *rOther.mPrePassShadowMapSize && *_130 == *rOther._130 &&
           *mIsEnableShadowDamp == *rOther.mIsEnableShadowDamp &&
           isNearZero(*mShadowDampStart - *rOther.mShadowDampStart) &&
           isNearZero(*mShadowDampEnd - *rOther.mShadowDampEnd) &&
           *mShadowComposeType == *rOther.mShadowComposeType &&
           *mDepthShadowFactorMin == *rOther.mDepthShadowFactorMin &&
           *mRepairIrradianceScale == *rOther.mRepairIrradianceScale &&
           isNearDirection(mDirForLpp->getDirection(), rOther.mDirForLpp->getDirection()) &&
           *mIsUsingShadowCamera == *rOther.mIsUsingShadowCamera &&
           isNearDirection(*mCameraPos, *rOther.mCameraPos) &&
           isNearDirection(*mCameraAt, *rOther.mCameraAt) &&
           isNearDirection(*mCameraUp, *rOther.mCameraUp) &&
           isNearZero(*mCameraNear - *rOther.mCameraNear) &&
           isNearZero(*mCameraFar - *rOther.mCameraFar) &&
           isNearZero(*mCameraFovy - *rOther.mCameraFovy) &&
           isNearZero(*mCameraAspect - *rOther.mCameraAspect);
}

/**
 * Copies the parameter values.
 * @param rOther Parameter to copy from.
 * @return This parameter.
 */
DepthShadowParam& DepthShadowParam::operator=(const DepthShadowParam& rOther) {
    *mAddViewDepth = *rOther.mAddViewDepth;
    *mAddVariance = *rOther.mAddVariance;
    *mPolygonOffset = *rOther.mPolygonOffset;
    *mPolygonSlopeScale = *rOther.mPolygonSlopeScale;
    *_270 = *rOther._270;
    *_290 = *rOther._290;
    *mIntensity = *rOther.mIntensity;
    *_30 = *rOther._30;
    *_50 = *rOther._50;
    *mIsDrawObjectShadow = *rOther.mIsDrawObjectShadow;
    *_90 = *rOther._90;
    *mIsDrawCasterShadowOnlyMap = *rOther.mIsDrawCasterShadowOnlyMap;
    *mIsDrawEffectShadow = *rOther.mIsDrawEffectShadow;
    *mIsShadowMap16UNorm = *rOther.mIsShadowMap16UNorm;
    mClipPlaneName = rOther.mClipPlaneName;
    *mMatrixCalcType = *rOther.mMatrixCalcType;
    *mIsShadowMaskEnable = *rOther.mIsShadowMaskEnable;
    *mPcfType = *rOther.mPcfType;
    *mPcfOffsetValue = *rOther.mPcfOffsetValue;
    *mShadowMapSize = *rOther.mShadowMapSize;
    *mPrePassShadowMapSize = *rOther.mPrePassShadowMapSize;
    *_130 = *rOther._130;
    *mIsEnableShadowDamp = *rOther.mIsEnableShadowDamp;
    *mShadowDampStart = *rOther.mShadowDampStart;
    *mShadowDampEnd = *rOther.mShadowDampEnd;
    *mShadowComposeType = *rOther.mShadowComposeType;
    *mDepthShadowFactorMin = *rOther.mDepthShadowFactorMin;
    *mRepairIrradianceScale = *rOther.mRepairIrradianceScale;
    mDirForLpp->syncFromDirection(rOther.mDirForLpp->getDirection());
    *mIsUsingShadowCamera = *rOther.mIsUsingShadowCamera;
    *mCameraPos = *rOther.mCameraPos;
    *mCameraAt = *rOther.mCameraAt;
    *mCameraUp = *rOther.mCameraUp;
    *mCameraNear = *rOther.mCameraNear;
    *mCameraFar = *rOther.mCameraFar;
    *mCameraFovy = *rOther.mCameraFovy;
    *mCameraAspect = *rOther.mCameraAspect;
    return *this;
}

/**
 * Interpolates between two depth shadow parameters.
 * @param rA Start parameter.
 * @param rB End parameter.
 * @param rate Interpolation rate.
 */
void DepthShadowParam::interp(const DepthShadowParam& rA, const DepthShadowParam& rB,
                              f32 rate) {
    mAddViewDepth.copyLerp(rA.mAddViewDepth, rB.mAddViewDepth, rate);
    mAddVariance.copyLerp(rA.mAddVariance, rB.mAddVariance, rate);
    mPolygonOffset.copyLerp(rA.mPolygonOffset, rB.mPolygonOffset, rate);
    mPolygonSlopeScale.copyLerp(rA.mPolygonSlopeScale, rB.mPolygonSlopeScale, rate);
    _270.copyLerp(rA._270, rB._270, rate);
    _290.copyLerp(rA._290, rB._290, rate);
    mIntensity.copyLerp(rA.mIntensity, rB.mIntensity, rate);

    _30.copy(rate < 0.5f ? rA._30 : rB._30);
    _50.copy(rate < 0.5f ? rA._50 : rB._50);
    mIsDrawObjectShadow.copy(rate < 0.5f ? rA.mIsDrawObjectShadow : rB.mIsDrawObjectShadow);
    _90.copy(rate < 0.5f ? rA._90 : rB._90);
    mIsDrawCasterShadowOnlyMap.copy(rate < 0.5f ? rA.mIsDrawCasterShadowOnlyMap :
                                                  rB.mIsDrawCasterShadowOnlyMap);
    mIsDrawEffectShadow.copy(rate < 0.5f ? rA.mIsDrawEffectShadow : rB.mIsDrawEffectShadow);
    mIsShadowMap16UNorm.copy(rate < 0.5f ? rA.mIsShadowMap16UNorm : rB.mIsShadowMap16UNorm);
    mClipPlaneName = rate < 0.5f ? rA.mClipPlaneName : rB.mClipPlaneName;
    mMatrixCalcType.copy(rate < 0.5f ? rA.mMatrixCalcType : rB.mMatrixCalcType);
    mIsShadowMaskEnable.copy(rate < 0.5f ? rA.mIsShadowMaskEnable : rB.mIsShadowMaskEnable);
    mPcfType.copy(rate < 0.5f ? rA.mPcfType : rB.mPcfType);
    mPcfOffsetValue.copyLerp(rA.mPcfOffsetValue, rB.mPcfOffsetValue, rate);
    *mShadowMapSize = lerpValueNew(*rA.mShadowMapSize, *rB.mShadowMapSize, rate);
    *mPrePassShadowMapSize = lerpValueNew(*rA.mPrePassShadowMapSize, *rB.mPrePassShadowMapSize,
                                          rate);
    _130.copy(rate < 0.5f ? rA._130 : rB._130);
    mIsEnableShadowDamp.copy(rate < 0.5f ? rA.mIsEnableShadowDamp : rB.mIsEnableShadowDamp);
    mShadowDampStart.copyLerp(rA.mShadowDampStart, rB.mShadowDampStart, rate);
    mShadowDampEnd.copyLerp(rA.mShadowDampEnd, rB.mShadowDampEnd, rate);
    mShadowComposeType.copy(rate < 0.5f ? rA.mShadowComposeType : rB.mShadowComposeType);
    mDepthShadowFactorMin.copyLerp(rA.mDepthShadowFactorMin, rB.mDepthShadowFactorMin, rate);
    mRepairIrradianceScale.copyLerp(rA.mRepairIrradianceScale, rB.mRepairIrradianceScale, rate);
    mDirForLpp->lerp(*rA.mDirForLpp, *rB.mDirForLpp, rate);
    mIsUsingShadowCamera.copy(rate < 0.5f ? rA.mIsUsingShadowCamera : rB.mIsUsingShadowCamera);
    mCameraPos.copyLerp(rA.mCameraPos, rB.mCameraPos, rate);
    mCameraAt.copyLerp(rA.mCameraAt, rB.mCameraAt, rate);
    mCameraUp.copyLerp(rA.mCameraUp, rB.mCameraUp, rate);
    mCameraNear.copyLerp(rA.mCameraNear, rB.mCameraNear, rate);
    mCameraFar.copyLerp(rA.mCameraFar, rB.mCameraFar, rate);
    mCameraFovy.copyLerp(rA.mCameraFovy, rB.mCameraFovy, rate);
    mCameraAspect.copyLerp(rA.mCameraAspect, rB.mCameraAspect, rate);
}

/**
 * Creates the clip parameters and the user clip planes.
 * @param pInfo Graphics system info, used to look up the graphics areas.
 */
DepthShadowClip::DepthShadowClip(const GraphicsSystemInfo* pInfo) : mSystemInfo(pInfo) {
    mName.init(sead::FixedSafeString<64>("No Name"), "Name", "名前", &mParamObj);
    mIsUsingCurrentGraphicsAreaClip.init(false, "IsUsingCurrentGraphicsAreaClip",
                                         "グラフィックスエリアでクリップ", &mParamObj);
    mClipAreaName.init(sead::FixedSafeString<64>(sead::SafeString::cEmptyString), "ClipAreaName",
                       "クリップエリア名", &mParamObj);

    for (s32 i = 0; i < cClipPlaneNum; i++) {
        mClipPlanes[i].mPlane = new PlaneParam;
        mClipPlanes[i].mPlane->initialize(sead::Vector3f::ex, &mParamObj,
                                          StringTmp<64>("ClipPlane%02d", i).cstr());
        mClipPlanes[i].mIsUse.init(false, StringTmp<64>("IsUsePlane%02d", i).cstr(),
                                   "平面をクリップに使うか", &mParamObj);
    }
}

/**
 * Builds the plane through three points, facing (rB - rOrigin) x (rC - rOrigin).
 * @param pPlane Output plane.
 * @param rOrigin Point on the plane.
 * @param rB Second point.
 * @param rC Third point.
 */
ALWAYS_INLINE static void calcPlane(sead::Plane3f* pPlane, const sead::Vector3f& rOrigin,
                                    const sead::Vector3f& rB, const sead::Vector3f& rC) {
    sead::Vector3f normal;
    normal.setCross(rB - rOrigin, rC - rOrigin);
    normal.normalize();
    *pPlane = sead::Plane3f(normal, normal.dot(rOrigin));
}

/**
 * Clips the depth shadow with the enabled clip planes and the clip area box.
 * @param pDepthShadow Depth shadow to clip.
 */
void DepthShadowClip::applyClipPlane(agl::sdw::DepthShadow* pDepthShadow) const {
    bool isClipped = false;

    for (s32 i = 0; i < cClipPlaneNum; i++) {
        if (*mClipPlanes[i].mIsUse) {
            const PlaneParam* plane = mClipPlanes[i].mPlane;
            pDepthShadow->clipByPlane(
                sead::Plane3f(-plane->getDirection(), plane->getDistanceFromOrigin()));
            isClipped = true;
        }
    }

    if (*mIsUsingCurrentGraphicsAreaClip) {
        const AreaObj* areaObj = getClipAreaObj();

        if (areaObj != nullptr) {
            const AreaShape* shape = areaObj->getAreaShape();
            sead::BoundBox3f localBox;

            if (shape->calcLocalBoundingBox(&localBox)) {
                const sead::Vector3f& min = localBox.getMin();
                const sead::Vector3f& max = localBox.getMax();
                sead::Vector3f points[8] = {
                    {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, min.y, max.z},
                    {min.x, min.y, max.z}, {min.x, max.y, min.z}, {max.x, max.y, min.z},
                    {max.x, max.y, max.z}, {min.x, max.y, max.z},
                };

                for (s32 i = 0; i < 8; i++) {
                    sead::Vector3f& point = points[i];
                    point.x *= shape->mScale.x;
                    point.y *= shape->mScale.y;
                    point.z *= shape->mScale.z;
                    point.setMul(areaObj->_28, point);
                }

                sead::Plane3f plane(sead::Vector3f::ex, 0.0f);
                calcPlane(&plane, points[0], points[2], points[1]);
                pDepthShadow->clipByPlane(plane);
                calcPlane(&plane, points[6], points[4], points[5]);
                pDepthShadow->clipByPlane(plane);
                calcPlane(&plane, points[2], points[5], points[1]);
                pDepthShadow->clipByPlane(plane);
                calcPlane(&plane, points[3], points[4], points[7]);
                pDepthShadow->clipByPlane(plane);
                calcPlane(&plane, points[2], points[7], points[6]);
                pDepthShadow->clipByPlane(plane);
                calcPlane(&plane, points[0], points[5], points[4]);
                pDepthShadow->clipByPlane(plane);
                isClipped = true;
            }
        }
    }

    if (isClipped) {
        pDepthShadow->updatePlaneClipInfo();
    }
}

/**
 * Finds the area the clip box is taken from: the named graphics area, or the current one.
 * @return Clip area, or nullptr if there is none.
 */
const AreaObj* DepthShadowClip::getClipAreaObj() const {
    GraphicsAreaDirector* director = mSystemInfo->getGraphicsAreaDirector();

    if (director == nullptr) {
        return nullptr;
    }

    if (mClipAreaName->isEmpty()) {
        return director->getCurrentArea();
    }

    s32 areaNum = director->getGraphicsAreaNum();
    for (s32 i = 0; i < areaNum; i++) {
        if (isEqualString(director->getGraphicsAreaInfoByIndex(i)->getName(),
                          mClipAreaName->cstr())) {
            return director->getGraphicsAreaInfoByIndex(i)->getAreaObj();
        }
    }

    return nullptr;
}

}  // namespace al
