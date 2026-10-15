#include "Library/Camera/SnapShotCameraCtrl.hpp"

#include <math/seadMathCalcCommon.h>

#include "Library/Camera/CameraPoserFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Camera/ICameraInput.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"

namespace {
using namespace al;

NERVE_DECL(SnapShotCameraCtrl, Wait)
NERVE_DECL(SnapShotCameraCtrl, Reset)

NERVES_MAKE_NOSTRUCT(SnapShotCameraCtrl, Wait, Reset)
}  // namespace

namespace al {

SnapShotCameraCtrl::SnapShotCameraCtrl(const IUseAreaObj* pAreaObj,
                                       const SnapShotCameraSceneInfo* pSceneInfo,
                                       bool isLongRange)
    : NerveExecutor("スナップショットモード中のカメラ制御"), mAreaObj(pAreaObj),
      mSceneInfo(pSceneInfo), mIsLongRange(isLongRange) {
    initNerve(&NrvSnapShotCameraCtrlWait, 0);
    mParam = new SnapShotCameraParam;
}

/**
 * Starts snapshot mode from the current fovy, clearing offsets and roll.
 * @param fovyDegree Fovy of the camera when snapshot mode starts.
 */
void SnapShotCameraCtrl::start(f32 fovyDegree) {
    mLookAtOffset = {0.0f, 0.0f, 0.0f};
    mLookAtOffsetTarget = {0.0f, 0.0f, 0.0f};
    mSafeLookAtOffset = {0.0f, 0.0f, 0.0f};
    mSafeLookAtOffsetTarget = {0.0f, 0.0f, 0.0f};
    mFovyDegree = fovyDegree;
    mDefaultFovyDegree = fovyDegree;
    mFovyDegreeTarget = fovyDegree;
    mRollDegree = 0.0f;
    mRollTarget = 0.0f;
}

/**
 * Loads the optional fovy limits from the "SnapShotParam" block.
 * @param rIter Camera parameter iterator.
 */
void SnapShotCameraCtrl::load(const ByamlIter& rIter) {
    SnapShotCameraParam* param = mParam;
    ByamlIter paramIter;

    if (!tryGetByamlIterByKey(&paramIter, rIter, "SnapShotParam")) {
        return;
    }

    if (tryGetByamlF32(&param->minFovyDegree, paramIter, "MinFovyDegree")) {
        param->hasMinFovyDegree = true;
    }

    if (tryGetByamlF32(&param->maxFovyDegree, paramIter, "MaxFovyDegree")) {
        param->hasMaxFovyDegree = true;
    }
}

/**
 * Starts interpolating back to the default pose.
 * @param step Reset length in steps, or a negative value for the default of 15.
 */
void SnapShotCameraCtrl::startReset(s32 step) {
    mResetStep = step >= 0 ? step : 15;
    setNerve(this, &NrvSnapShotCameraCtrlReset);
}

void SnapShotCameraCtrl::update(const sead::LookAtCamera& rCamera,
                                const IUseCollision* pCollision, const ICameraInput* pInput) {
    updateNerve();
    sead::Vector3f cameraPos = rCamera.getPos();
    mIsInInk = isInInk(pCollision, cameraPos, mIsLongRange ? 3000.0f : 400.0f);
    mCollision = pCollision;

    if (!isNerve(this, &NrvSnapShotCameraCtrlWait)) {
        return;
    }

    if (mIsValidZoomFovy) {
        f32 fovyTarget = mFovyDegreeTarget;
        f32 prevFovy = mFovyDegree;
        f32 nextFovy;

        if (pInput->isHoldSnapShotZoomIn()) {
            nextFovy = fovyTarget - 2.0f;
        } else {
            nextFovy = pInput->isHoldSnapShotZoomOut() ? fovyTarget + 2.0f : fovyTarget;
        }

        f32 minFovy = mParam->hasMinFovyDegree ? mParam->minFovyDegree : 8.0f;
        f32 maxFovy;

        if (mParam->hasMaxFovyDegree) {
            maxFovy = mParam->maxFovyDegree;
        } else {
            maxFovy = mMaxZoomOutFovyDegree > 0.0f ? mMaxZoomOutFovyDegree : mDefaultFovyDegree;
        }

        mFovyDegreeTarget =
            lerpValue(0.3f, mFovyDegreeTarget, sead::Mathf::clamp(nextFovy, minFovy, maxFovy));
        mFovyDegree = lerpValue(0.3f, mFovyDegree, mFovyDegreeTarget);

        if (getAudioKeeper() != nullptr && sead::Mathf::abs(mFovyDegree - prevFovy) > 0.2f) {
            tryHoldSeWithParam(this, "PgZoom",
                               1.0f - (mFovyDegree - minFovy) / (maxFovy - minFovy), nullptr);
        }
    }

    if (mIsValidRoll && mIsEnableRoll) {
        f32 prevRoll = mRollDegree;
        f32 rollTarget = mRollTarget;
        f32 nextRoll;

        if (pInput->isHoldSnapShotRollLeft()) {
            nextRoll = rollTarget - 3.0f;
        } else {
            nextRoll = pInput->isHoldSnapShotRollRight() ? rollTarget + 3.0f : rollTarget;
        }

        mRollTarget = lerpValue(0.2f, mRollTarget, sead::Mathf::clamp(nextRoll, -90.0f, 90.0f));
        mRollDegree = lerpValue(0.15f, mRollDegree, mRollTarget);

        if (getAudioKeeper() != nullptr && sead::Mathf::abs(mRollDegree - prevRoll) > 0.2f) {
            tryHoldSeWithParam(this, "PgRoll", sead::Mathf::abs(mRollDegree), nullptr);
        }
    }

    if (mIsValidLookAtOffset) {
        f32 speed = mIsValidMove ? 400.0f : 50.0f;
        f32 limit = mIsValidMove ? 3500.0f : 500.0f;

        if (!isNearZero(mSafeLookAtOffsetTarget - mLookAtOffsetTarget, 0.5f)) {
            mSafeLookAtOffsetTarget.set(mLookAtOffsetTarget);
        }

        if (!isNearZero(mSafeLookAtOffset - mLookAtOffset, 0.5f)) {
            mSafeLookAtOffset = mLookAtOffset;
        }

        sead::Vector3f move = {0.0f, 0.0f, 0.0f};
        sead::Vector2f stick = {0.0f, 0.0f};

        if (pInput->tryCalcSnapShotMoveStick(&stick)) {
            sead::Vector3f front = rCamera.getAt() - rCamera.getPos();
            sead::Vector3f prevOffsetTarget = mLookAtOffsetTarget;
            normalize(&front);
            sead::Vector3f up = rCamera.getUp();
            rotateVectorDegree(&up, up, front, mRollDegree);
            normalize(&up);
            f32 moveSpeed = speed * getRotationScaler();

            if (!isNearZero(stick.x, 0.001f)) {
                sead::Vector3f side;
                side.setCross(front, up);
                tryNormalizeOrZero(&side);
                move += (stick.x * side) * moveSpeed;
            }

            if (!isNearZero(stick.y, 0.001f)) {
                move += (stick.y * up) * moveSpeed;
            }

            if (!isNearZero(move, 0.001f) && !mIsInInk) {
                sead::Vector3f target = mLookAtOffset + move;
                clampV3f(&target, {-limit, -limit, -limit}, {limit, limit, limit});
                sead::Vector3f prevTarget = mLookAtOffsetTarget;
                lerpVec(&mLookAtOffsetTarget, mLookAtOffsetTarget, target, 0.3f);
                sead::Vector3f checkPos = cameraPos + mLookAtOffsetTarget;

                if (isInInk(pCollision, checkPos, mIsLongRange ? 3000.0f : 400.0f)) {
                    mLookAtOffsetTarget.set(prevTarget);
                    mIsInInk = true;
                }
            }

            if (mSceneInfo->isValidLimitAtY && mLookAtOffsetTarget.y < 0.0f) {
                f32 atY = rCamera.getAt().y;
                f32 limitY = mSceneInfo->limitAtY;

                if (mLookAtOffsetTarget.y + atY < limitY) {
                    mLookAtOffsetTarget.y = sead::Mathf::min(limitY, atY) - atY;
                }
            }

            sead::Vector3f from = rCamera.getAt() + prevOffsetTarget;
            sead::Vector3f to = rCamera.getAt() + mLookAtOffsetTarget;
            alCameraPoserFunction::checkCameraCollisionMoveSphere(&to, pCollision, from, to,
                                                                  75.0f);
            mLookAtOffsetTarget = to - rCamera.getAt();
        }

        lerpVec(&mLookAtOffset, mLookAtOffset, mLookAtOffsetTarget, 0.3f);
    }

    if (mIsZoomAutoReset && pInput->isTriggerReset()) {
        mResetStep = 15;
        setNerve(this, &NrvSnapShotCameraCtrlReset);
    }
}

void SnapShotCameraCtrl::makeLookAtCameraPost(sead::LookAtCamera* pCamera) {
    if (!mIsValidLookAtOffset) {
        return;
    }

    f32 inkDist;
    sead::Vector3f pos = pCamera->getPos();
    sead::Vector3f lowerPos =
        pCamera->getPos() + mLookAtOffset - sead::Vector3f(0.0f, 101.0f, 0.0f);
    sead::Vector3f upperPos = pos;
    upperPos.y += 60.0f;
    sead::Vector3f hitPos;
    sead::Vector3f hitNormal;
    sead::Vector3f offsetPos = pos + mLookAtOffsetTarget;
    sead::Vector3f inkDir = {0.0f, mIsLongRange ? -3000.0f : -400.0f, 0.0f};

    if (mIsInInk || (mCollision != nullptr && isInInk(mCollision, offsetPos, inkDir, &inkDist))) {
        if (isNearZero(mLookAtOffset - mSafeLookAtOffset, 0.001f)) {
            if (mLookAtOffsetTarget.y < 100.0f) {
                mLookAtOffsetTarget.y += 10.0f;
                mSafeLookAtOffsetTarget.y = mLookAtOffsetTarget.y;
                mLookAtOffset.y += 10.0f;
                mSafeLookAtOffset.y = mLookAtOffset.y;
            }
        } else {
            mLookAtOffsetTarget.set(mSafeLookAtOffsetTarget);
            mLookAtOffset.set(mSafeLookAtOffset);
        }
    }

    if (isInWaterArea(mAreaObj, pos, lowerPos, &hitPos, &hitNormal)) {
        mLookAtOffsetTarget.y = hitPos.y - pCamera->getPos().y + 101.0f;
        mLookAtOffset.y = hitPos.y - pCamera->getPos().y + 101.0f;
    } else if (isInWaterArea(mAreaObj, upperPos)) {
        mLookAtOffsetTarget.set(mSafeLookAtOffsetTarget);
            mLookAtOffset.set(mSafeLookAtOffset);
    } else {
        sead::Vector3f at = pCamera->getAt();
        sead::Vector3f lowerAt =
            pCamera->getAt() + mLookAtOffset - sead::Vector3f(0.0f, 101.0f, 0.0f);
        sead::Vector3f atHitPos;
        sead::Vector3f atHitNormal;

        if (isInWaterArea(mAreaObj, at, lowerAt, &atHitPos, &atHitNormal)) {
            mLookAtOffsetTarget.y = atHitPos.y - pCamera->getAt().y + 101.0f;
            mLookAtOffset.y = atHitPos.y - pCamera->getAt().y + 101.0f;
        }
    }

    pCamera->setAt(pCamera->getAt() + mLookAtOffset);
    pCamera->setPos(pCamera->getPos() + mLookAtOffset);
}

/**
 * Applies the snapshot roll to the camera up vector.
 * @param pCamera Camera to modify.
 */
void SnapShotCameraCtrl::makeLookAtCameraLast(sead::LookAtCamera* pCamera) const {
    if (!mIsValidRoll) {
        return;
    }

    sead::Vector3f front = pCamera->getAt() - pCamera->getPos();
    normalize(&front);
    sead::Vector3f up = pCamera->getUp();
    rotateVectorDegree(&up, up, front, mRollDegree);
    normalize(&up);
    pCamera->setUp(up);
    pCamera->getUp().normalize();
}

/**
 * Gets the scale for stick movement based on the current zoom.
 * @return Scale between 0.25 and 1.
 */
f32 SnapShotCameraCtrl::getRotationScaler() const {
    return lerpValue((mFovyDegree - 8.0f) / (mDefaultFovyDegree - 8.0f), 0.25f, 1.0f);
}

/**
 * Waits for input.
 */
void SnapShotCameraCtrl::exeWait() {}

void SnapShotCameraCtrl::exeReset() {
    if (isFirstStep(this)) {
        mRollTarget = mRollDegree;
        mFovyDegreeTarget = mFovyDegree;
        mLookAtOffsetTarget.set(mLookAtOffset);
    }

    mFovyDegree = calcNerveValue(this, mResetStep, mFovyDegreeTarget, mDefaultFovyDegree);
    f32 rate = 1.0f - calcNerveRate(this, mResetStep);
    mRollDegree = mRollTarget * rate;
    mSafeLookAtOffsetTarget.set(mLookAtOffsetTarget);
    mLookAtOffset = rate * mLookAtOffsetTarget;
    mSafeLookAtOffset = mLookAtOffset;

    if (isGreaterEqualStep(this, mResetStep)) {
        mRollDegree = 0.0f;
        mRollTarget = 0.0f;
        mResetStep = -1;
        mFovyDegree = mDefaultFovyDegree;
        mFovyDegreeTarget = mDefaultFovyDegree;
        mLookAtOffset = {0.0f, 0.0f, 0.0f};
        mLookAtOffsetTarget = {0.0f, 0.0f, 0.0f};
        setNerve(this, &NrvSnapShotCameraCtrlWait);
    }
}

/**
 * Gets the audio keeper of the scene.
 * @return Audio keeper, or nullptr if there is none.
 */
AudioKeeper* SnapShotCameraCtrl::getAudioKeeper() const {
    if (mSceneInfo != nullptr && mSceneInfo->audioKeeper != nullptr) {
        return mSceneInfo->audioKeeper->getAudioKeeper();
    }

    return nullptr;
}

}  // namespace al
