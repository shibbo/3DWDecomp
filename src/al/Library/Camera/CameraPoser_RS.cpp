#include "Library/Camera/CameraPoser_RS.hpp"

#include <gfx/seadCamera.h>
#include <math/seadMathCalcCommon.h>

#include "Library/Camera/CameraGyroCtrl.hpp"
#include "Library/Camera/CameraParamMoveLimit.hpp"
#include "Library/Camera/CameraPoserFlag.hpp"
#include "Library/Camera/CameraPoserFunction.hpp"
#include "Library/Camera/CameraPoserSceneInfo_RS.hpp"
#include "Library/Camera/CameraStartInfo.hpp"
#include "Library/Camera/SnapShotCameraCtrl.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveKeeper.hpp"
#include "Library/Play/Camera/CameraArrowCollider.hpp"
#include "Library/Play/Camera/CameraVerticalAbsorber.hpp"
#include "Library/Rail/RailKeeper.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Camera/CameraAngleCtrlInfo.hpp"
#include "Project/Camera/CameraAngleSwingInfo.hpp"
#include "Project/Camera/CameraObjectRequestInfo.hpp"
#include "Project/Camera/CameraOffsetCtrlPreset.hpp"
#include "Project/Camera/Holder/CameraTargetAreaLimitter.hpp"

namespace al {

/**
 * Creates a camera poser with default pose and interpolation parameters.
 * @param pName Camera poser name.
 */
CameraPoser_RS::CameraPoser_RS(const char* pName) : mPoserName(pName) {
    mPoserFlag = new CameraPoserFlag();
    mActiveInterpoleParam = new CameraInterpoleParam();
    mEndInterpoleParam = new CameraInterpoleStep();
}

/**
 * Returns the orthographic projection info of this camera if it is set.
 * @param pInfo Receives the projection info.
 * @return Whether the camera uses an orthographic projection.
 */
bool CameraPoser_RS::tryCalcOrthoProjectionInfo(OrthoProjectionInfo* pInfo) const {
    OrthoProjectionParam* param = mOrthoProjectionParam;

    if (!param) {
        return false;
    }

    if (!param->isSetInfo) {
        return false;
    }

    f32 width = param->info.nearClipWidth;

    if (!(width > 0.1f)) {
        return false;
    }

    f32 height = param->info.nearClipHeight;

    if (!(height > 0.1f)) {
        return false;
    }

    *pInfo = {width, height};
    return true;
}

/**
 * Returns whether the camera can be rotated with the stick.
 * @return Whether rotating by pad is enabled.
 */
bool CameraPoser_RS::isEnableRotateByPad() const {
    if (mAngleCtrlInfo) {
        return !mAngleCtrlInfo->isFixByRangeHV();
    }

    if (mAngleSwingInfo) {
        return !mAngleSwingInfo->isInvalidSwing;
    }

    return false;
}

/**
 * Returns the field of view of the camera, taking the snapshot mode into account.
 * @return Field of view in degrees.
 */
f32 CameraPoser_RS::getFovyDegree() const {
    if (alCameraPoserFunction::isSnapShotMode(this) && mSnapShotCtrl) {
        return mSnapShotCtrl->getFovyDegree();
    }

    return mFovyDegree;
}

/**
 * Returns the field of view of the scene.
 * @return Field of view in degrees.
 */
f32 CameraPoser_RS::getSceneFovyDegree() const {
    return mSceneInfo->sceneFovyDegree;
}

/**
 * Returns the area object director of the scene.
 * @return Area object director.
 */
AreaObjDirector* CameraPoser_RS::getAreaObjDirector() const {
    return mSceneInfo->areaObjDirector;
}

/**
 * Returns the collision director of the scene.
 * @return Collision director.
 */
CollisionDirector* CameraPoser_RS::getCollisionDirector() const {
    return mSceneInfo->collisionDirector;
}

/**
 * Returns the camera input holder of the scene.
 * @return Camera input holder.
 */
CameraInputHolder* CameraPoser_RS::getInputHolder() const {
    return mSceneInfo->inputHolder;
}

/**
 * Returns the camera target holder of the scene.
 * @return Camera target holder.
 */
CameraTargetHolder* CameraPoser_RS::getTargetHolder() const {
    return mSceneInfo->targetHolder;
}

/**
 * Returns the camera flags of the scene.
 * @return Camera flags.
 */
CameraFlagCtrl* CameraPoser_RS::getFlagCtrl() const {
    return mSceneInfo->flagCtrl;
}

/**
 * Returns the rail rider of the camera rail.
 * @return Rail rider, or null without a rail.
 */
RailRider* CameraPoser_RS::getRailRider() const {
    return mRailKeeper ? mRailKeeper->getRailRider() : nullptr;
}

/**
 * Returns the height absorbed by the vertical absorber.
 * @return Absorbed height.
 */
f32 CameraPoser_RS::getAbsorbHeight() const {
    return mVerticalAbsorber ? mVerticalAbsorber->getAbsorbHeight() : 0.0f;
}

/**
 * Returns whether the interpolation length depends on the camera distance.
 * @return Whether the interpolation is by camera distance.
 */
bool CameraPoser_RS::isInterpoleByCameraDistance() const {
    return mActiveInterpoleParam->stepType == CameraInterpoleStepType::ByCameraDistance;
}

/**
 * Returns the interpolation length to this camera.
 * @return Interpolation frames.
 */
s32 CameraPoser_RS::getInterpoleStep() const {
    return mActiveInterpoleParam->stepNum < 0 ? 60 : mActiveInterpoleParam->stepNum;
}

/**
 * Sets the interpolation length to this camera.
 * @param step Interpolation frames.
 */
void CameraPoser_RS::setInterpoleStep(s32 step) {
    mActiveInterpoleParam->set(CameraInterpoleStepType::ByStep, step, true);
}

/**
 * Sets the interpolation length when this camera ends.
 * @param step Interpolation frames.
 */
void CameraPoser_RS::setEndInterpoleStep(s32 step) {
    mEndInterpoleParam->stepNum = step;
    mEndInterpoleParam->stepType = CameraInterpoleStepType::ByStep;
}

/**
 * Resets the interpolation length to this camera.
 */
void CameraPoser_RS::resetInterpoleStep() {
    mActiveInterpoleParam->set(CameraInterpoleStepType::ByCameraDistance, -1, false);
}

/**
 * Returns whether the interpolation to this camera eases out.
 * @return Whether the interpolation eases out.
 */
bool CameraPoser_RS::isInterpoleEaseOut() const {
    return mActiveInterpoleParam->isEaseOut;
}

/**
 * Makes the interpolation to this camera ease out.
 */
void CameraPoser_RS::setInterpoleEaseOut() {
    mActiveInterpoleParam->isEaseOut = true;
}

/**
 * Returns whether the interpolation length at the end is given in frames.
 * @return Whether the end interpolation is by step.
 */
bool CameraPoser_RS::isEndInterpoleByStep() const {
    return mEndInterpoleParam->stepType == CameraInterpoleStepType::ByStep;
}

/**
 * Returns the interpolation length when this camera ends.
 * @return Interpolation frames.
 */
s32 CameraPoser_RS::getEndInterpoleStep() const {
    return mEndInterpoleParam->stepNum;
}

/**
 * Creates the nerve keeper of the camera.
 * @param pNerve First nerve.
 * @param stateNum Maximum number of nerve states.
 */
void CameraPoser_RS::initNerve(const Nerve* pNerve, s32 stateNum) {
    mNerveKeeper = new NerveKeeper(this, pNerve, stateNum);
}

/**
 * Sets the arrow collider and enables collision.
 * @param pCollider Arrow collider.
 */
void CameraPoser_RS::initArrowCollider(CameraArrowCollider* pCollider) {
    mArrowCollider = pCollider;
    mPoserFlag->isInvalidCollider = false;
}

/**
 * Does nothing in this version.
 * @param pName Audio keeper name.
 */
void CameraPoser_RS::initAudioKeeper(const char* pName) {}

/**
 * Creates the rail of the camera.
 * @param rInfo Placement info of the rail.
 */
void CameraPoser_RS::initRail(const PlacementInfo& rInfo) {
    mRailKeeper = new RailKeeper(rInfo);
}

/**
 * Creates the local interpolation.
 */
void CameraPoser_RS::initLocalInterpole() {
    mLocalInterpole = new LocalInterpole();
}

/**
 * Creates the look at interpolation.
 * @param rate Interpolation rate.
 */
void CameraPoser_RS::initLookAtInterpole(f32 rate) {
    mLookAtInterpole = new LookAtInterpole(rate);
}

/**
 * Creates the orthographic projection parameter.
 */
void CameraPoser_RS::initOrthoProjectionParam() {
    mOrthoProjectionParam = new OrthoProjectionParam();
}

/**
 * Creates the target area limitter if a limit area is linked.
 * @param rInfo Placement info.
 */
void CameraPoser_RS::tryInitAreaLimitter(const PlacementInfo& rInfo) {
    mTargetAreaLimitter = CameraTargetAreaLimitter::tryCreate(rInfo);
}

static void loadInterpoleParam(CameraPoser_RS::CameraInterpoleParam* pParam,
                               const ByamlIter& rIter) {
    tryGetByamlS32(reinterpret_cast<s32*>(&pParam->stepType), rIter, "InterpoleStepType");
    const char* curveType = nullptr;

    if (tryGetByamlString(&curveType, rIter, "InterpoleCurveType") && curveType &&
        isEqualString(curveType, "EaseOut")) {
        pParam->isEaseOut = true;
    }

    pParam->isInterpolateByStep = tryGetByamlS32(&pParam->stepNum, rIter, "InterpoleStep");

    if (pParam->isInterpolateByStep) {
        pParam->stepType = CameraPoser_RS::CameraInterpoleStepType::ByStep;
    }
}

static void loadEndInterpoleParam(CameraPoser_RS::CameraInterpoleStep* pParam,
                                  const ByamlIter& rIter) {
    ByamlIter iter;

    if (tryGetByamlIterByKey(&iter, rIter, "EndInterpoleParam")) {
        if (isEqualString(getByamlKeyString(iter, "Type"), "Step")) {
            pParam->stepType = CameraPoser_RS::CameraInterpoleStepType::ByStep;
        }

        if (pParam->stepType == CameraPoser_RS::CameraInterpoleStepType::ByStep) {
            pParam->stepNum = getByamlKeyInt(iter, "Step");
        }
    }
}

static void loadOrthoProjectionParam(CameraPoser_RS::OrthoProjectionParam* pParam,
                                     const ByamlIter& rIter) {
    if (tryGetByamlBool(&pParam->isSetInfo, rIter, "IsSetOrthoProjectionInfo") &&
        pParam->isSetInfo) {
        tryGetByamlF32(&pParam->info.nearClipWidth, rIter, "OrthoProjectionNearClipWidth");
        tryGetByamlF32(&pParam->info.nearClipHeight, rIter, "OrthoProjectionNearClipHeight");
    }
}

/**
 * Loads the camera parameters.
 * @param rIter Camera parameter iterator.
 */
void CameraPoser_RS::load(const ByamlIter& rIter) {
    loadParam(rIter);
    tryGetByamlF32(&mFovyDegree, rIter, "FovyDegree");
    tryGetByamlF32(&mFarClipDistance, rIter, "FarClipDistance");
    mFovyDegree = 45.0f;
    mPoserFlag->load(rIter);
    loadInterpoleParam(mActiveInterpoleParam, rIter);
    loadEndInterpoleParam(mEndInterpoleParam, rIter);

    if (mVerticalAbsorber) {
        mVerticalAbsorber->load(rIter);
    }

    if (mAngleCtrlInfo) {
        mAngleCtrlInfo->load(rIter);
    }

    if (mAngleSwingInfo) {
        mAngleSwingInfo->load(rIter);
    }

    if (mOffsetCtrlPreset) {
        mOffsetCtrlPreset->load(rIter);
    }

    if (mParamMoveLimit) {
        mParamMoveLimit->load(rIter);
    }

    if (mSnapShotCtrl) {
        mSnapShotCtrl->load(rIter);
    }

    if (mOrthoProjectionParam) {
        loadOrthoProjectionParam(mOrthoProjectionParam, rIter);
    }

    _141 = false;
}

/**
 * Returns whether the camera is calculated for the first time.
 * @return Whether this is the first calculation.
 */
bool CameraPoser_RS::isFirstCalc() const {
    return mPoserFlag->isFirstCalc;
}

/**
 * Activates the camera.
 * @param rInfo Camera start info.
 */
void CameraPoser_RS::appear(const CameraStartInfo& rInfo) {
    mActiveState = ActiveState::Active;

    if (mAngleCtrlInfo) {
        sead::Vector3f dir = {0.0f, 0.0f, 0.0f};
        alCameraPoserFunction::calcPreCameraDir(&dir, this);
        mAngleCtrlInfo->start(sead::Mathf::rad2deg(sead::Mathf::asin(dir.y)));
    }

    if (mAngleSwingInfo) {
        mAngleSwingInfo->currentAngle = {0.0f, 0.0f};
    }

    start(rInfo);

    if (mArrowCollider && !mPoserFlag->isInvalidCollider) {
        mArrowCollider->start();
    }

    if (mVerticalAbsorber && !mPoserFlag->isOffVerticalAbsorb) {
        mVerticalAbsorber->start(mAt, rInfo);
    }

    if (mLookAtInterpole) {
        mLookAtInterpole->target.set(mAt);
    }
}

static void updateLookAtInterpole(CameraPoser_RS::LookAtInterpole* pInterpole,
                                  CameraPoser_RS* pPoser, sead::Vector3f prevAt) {
    lerpVec(pPoser->getAtPtr(), pInterpole->target, pPoser->getAt(), pInterpole->lerp);
    pInterpole->target.set(pPoser->getAt());
    pPoser->addEyeOffset(pPoser->getAt() - prevAt);
}

static void updateLookAtInterpoleWithVerticalAbsorb(CameraPoser_RS::LookAtInterpole* pInterpole,
                                                    CameraPoser_RS* pPoser,
                                                    const sead::Vector3f& rGravity,
                                                    sead::Vector3f prevAt) {
    sead::Vector3f offset = pPoser->getAt() - pInterpole->target;
    sead::Vector3f offsetV = {0.0f, 0.0f, 0.0f};
    parallelizeVec(&offsetV, rGravity, offset);
    sead::Vector3f offsetH = offset - offsetV;
    pPoser->setAt(pInterpole->target + offsetV + offsetH * pInterpole->lerp);
    pInterpole->target.set(pPoser->getAt());
    pPoser->addEyeOffset(pPoser->getAt() - prevAt);
}

static void updateLocalInterpole(CameraPoser_RS::LocalInterpole* pInterpole,
                                 const CameraPoser_RS* pPoser) {
    if (alCameraPoserFunction::isChangeTarget(pPoser)) {
        pInterpole->step = 0;
        pInterpole->end = 30;
        pInterpole->prevCameraPos.set(alCameraPoserFunction::getPreCameraPos(pPoser));
        pInterpole->prevLookAtPos.set(alCameraPoserFunction::getPreLookAtPos(pPoser));
    }
}

/**
 * Updates the camera input, the camera and its helpers.
 */
void CameraPoser_RS::movement() {
    if (mNerveKeeper) {
        mNerveKeeper->update();
    }

    if (mAngleCtrlInfo || mAngleSwingInfo) {
        sead::Vector2f stick = {0.0f, 0.0f};
        alCameraPoserFunction::calcCameraRolledRotateStick(&stick, this);

        if (mAngleCtrlInfo) {
            bool isTriggerReset = alCameraPoserFunction::isTriggerCameraResetRotate(this);
            mAngleCtrlInfo->update(stick, alCameraPoserFunction::getStickSensitivityScale(this),
                                   isTriggerReset);
            if (alCameraPoserFunction::isSnapShotMode(this) && isTriggerReset) {
                s32 step = -1;

                if (mAngleCtrlInfo->isResetStartTiming()) {
                    step = mAngleCtrlInfo->getMaxResetStep();
                }

                alCameraPoserFunction::startResetSnapShotCameraCtrl(this, step);
            }
        }

        if (mAngleSwingInfo) {
            mAngleSwingInfo->update(stick, alCameraPoserFunction::getStickSensitivityScale(this));
        }
    }

    if (mGyroCtrl && !alCameraPoserFunction::isStopUpdateGyro(this)) {
        sead::Vector3f side;
        sead::Vector3f up;
        sead::Vector3f front;
        alCameraPoserFunction::calcCameraGyroPose(this, &side, &up, &front);
        mGyroCtrl->setIsValidGyro(alCameraPoserFunction::isValidGyro(this));
        mGyroCtrl->setSensitivityScale(alCameraPoserFunction::getGyroSensitivityScale(this));
        mGyroCtrl->update(side, up, front);
    }

    update();

    if (mLookAtInterpole) {
        if (mVerticalAbsorber && !mPoserFlag->isOffVerticalAbsorb) {
            sead::Vector3f gravity = {0.0f, 0.0f, 0.0f};
            alCameraPoserFunction::calcTargetGravity(&gravity, this);
            updateLookAtInterpoleWithVerticalAbsorb(mLookAtInterpole, this, gravity, mAt);
        } else {
            updateLookAtInterpole(mLookAtInterpole, this, mAt);
        }
    }

    if (mVerticalAbsorber && !mPoserFlag->isOffVerticalAbsorb) {
        mVerticalAbsorber->update();
    }

    if (alCameraPoserFunction::isSnapShotMode(this) && mSnapShotCtrl) {
        alCameraPoserFunction::updateSnapShotCameraCtrl(this);
    }

    if (mLocalInterpole) {
        updateLocalInterpole(mLocalInterpole, this);

        if (mLocalInterpole->step >= 0) {
            s32 nextStep = mLocalInterpole->step + 1;
            mLocalInterpole->step = mLocalInterpole->end > nextStep ? nextStep : -1;
        }
    }

    if (mArrowCollider && !mPoserFlag->isInvalidCollider) {
        sead::LookAtCamera camera;
        makeLookAtCameraPrev(&camera);
        makeLookAtCamera(&camera);

        if (alCameraPoserFunction::isSnapShotMode(this) && mSnapShotCtrl) {
            mSnapShotCtrl->makeLookAtCameraPost(&camera);
        }

        if (mParamMoveLimit) {
            mParamMoveLimit->apply(&camera);
        }

        if (alCameraPoserFunction::isSnapShotMode(this) && mSnapShotCtrl) {
            mSnapShotCtrl->makeLookAtCameraLast(&camera);
        }

        mArrowCollider->update(camera.getPos(), camera.getAt(), camera.getUp());
    }

    mPoserFlag->isFirstCalc = false;
}

static void interpolateLocal(const CameraPoser_RS::LocalInterpole* pInterpole,
                             sead::LookAtCamera* pCamera) {
    if (pInterpole->step < 0) {
        return;
    }

    f32 rate = hermiteRate(normalize(static_cast<f32>(pInterpole->step), 0.0f,
                                     static_cast<f32>(pInterpole->end)),
                           1.5f, 0.0f);
    sead::Vector3f pos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f at = {0.0f, 0.0f, 0.0f};
    lerpVec(&pos, pInterpole->prevCameraPos, pCamera->getPos(), rate);
    lerpVec(&at, pInterpole->prevLookAtPos, pCamera->getAt(), rate);
    pCamera->setPos(pos);
    pCamera->setAt(at);
}

/**
 * Makes the camera pose from the poser, the vertical absorber and the local helpers.
 * @param pCamera Receives the camera pose.
 */
void CameraPoser_RS::makeLookAtCameraPrev(sead::LookAtCamera* pCamera) const {
    pCamera->setPos(mEye);
    pCamera->setAt(mAt);
    pCamera->setUp(mUp);
    pCamera->normalizeUp();

    if (mVerticalAbsorber && !mPoserFlag->isOffVerticalAbsorb) {
        mVerticalAbsorber->makeLookAtCamera(pCamera);
    }

    if (mLocalInterpole) {
        interpolateLocal(mLocalInterpole, pCamera);
    }

    if (mAngleSwingInfo) {
        mAngleSwingInfo->makeLookAtCamera(pCamera);
    }

    if (mTargetAreaLimitter) {
        sead::Vector3f at = pCamera->getAt();

        if (mTargetAreaLimitter->applyAreaLimit(&at, at)) {
            sead::Vector3f offset = at - pCamera->getAt();
            pCamera->setPos(offset + pCamera->getPos());
            pCamera->setAt(offset + pCamera->getAt());
        }
    }
}

/**
 * Applies the snapshot and move limit adjustments to the camera pose.
 * @param pCamera Camera pose.
 */
void CameraPoser_RS::makeLookAtCameraPost(sead::LookAtCamera* pCamera) const {
    if (alCameraPoserFunction::isSnapShotMode(this) && mSnapShotCtrl) {
        mSnapShotCtrl->makeLookAtCameraPost(pCamera);
    }

    if (mParamMoveLimit) {
        mParamMoveLimit->apply(pCamera);
    }
}

/**
 * Applies the final snapshot adjustments to the camera pose.
 * @param pCamera Camera pose.
 */
void CameraPoser_RS::makeLookAtCameraLast(sead::LookAtCamera* pCamera) const {
    if (alCameraPoserFunction::isSnapShotMode(this) && mSnapShotCtrl) {
        mSnapShotCtrl->makeLookAtCameraLast(pCamera);
    }
}

/**
 * Updates the camera and calculates its pose for ending after an interpolation.
 * @param pCamera Receives the camera pose.
 */
void CameraPoser_RS::movementAndCalcCameraPoseForEndAfterInterpole(sead::LookAtCamera* pCamera) {
    _9c = true;
    movement();
    calcCameraPose(pCamera);
    _9c = false;
}

/**
 * Pauses or resumes the move limit.
 * @param isPause Whether the move limit is paused.
 */
void CameraPoser_RS::setPauseMoveLimit(bool isPause) {
    if (mParamMoveLimit) {
        mParamMoveLimit->setPauseApply(isPause);
    }
}

/**
 * Sets the water height of the move limit.
 * @param height Water height.
 */
void CameraPoser_RS::setWaterHeight(f32 height) {
    if (mParamMoveLimit) {
        mParamMoveLimit->setWaterHeight(height);
    }
}

/**
 * Applies the collision of the arrow collider to the camera pose.
 * @param pCamera Camera pose.
 */
void CameraPoser_RS::makeLookAtCameraCollide(sead::LookAtCamera* pCamera) const {
    if (!mPoserFlag->isInvalidCollider && mArrowCollider) {
        mArrowCollider->makeLookAtCamera(pCamera);
    }
}

/**
 * Calculates the full camera pose.
 * @param pCamera Receives the camera pose.
 */
void CameraPoser_RS::calcCameraPose(sead::LookAtCamera* pCamera) const {
    makeLookAtCameraPrev(pCamera);
    makeLookAtCamera(pCamera);
    makeLookAtCameraPost(pCamera);
    makeLookAtCameraCollide(pCamera);
    makeLookAtCameraLast(pCamera);
}

/**
 * Passes an object request to the camera and its helpers.
 * @param rInfo Request info.
 * @return Whether the request was accepted.
 */
bool CameraPoser_RS::receiveRequestFromObjectCore(const CameraObjectRequestInfo& rInfo) {
    if (receiveRequestFromObject(rInfo)) {
        return true;
    }

    if (mVerticalAbsorber && rInfo.isStopVerticalAbsorb) {
        mVerticalAbsorber->liberateAbsorb();
        return true;
    }

    if (mAngleCtrlInfo && mAngleCtrlInfo->receiveRequestFromObject(rInfo)) {
        return true;
    }

    return false;
}

/**
 * Starts the snapshot mode of the camera.
 */
void CameraPoser_RS::startSnapShotModeCore() {
    if (mSnapShotCtrl) {
        mSnapShotCtrl->start(mFovyDegree);
    }

    startSnapShotMode();
}

/**
 * Enables or disables rolling in snapshot mode.
 * @param isEnable Whether rolling is enabled.
 */
void CameraPoser_RS::enableSnapShotRoll(bool isEnable) {
    if (mSnapShotCtrl) {
        mSnapShotCtrl->setIsEnableRoll(isEnable);
    }
}

/**
 * Ends the snapshot mode of the camera.
 */
void CameraPoser_RS::endSnapShotModeCore() {
    endSnapShotMode();
}

}  // namespace al
