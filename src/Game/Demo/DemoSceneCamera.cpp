#include "Demo/DemoSceneCamera.hpp"
#include "Demo/DemoActionList.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/File/FileUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"

namespace DemoSceneActorFunction {
void calcPlacementBaseMtx(sead::Matrix34f* pOut, const al::ActorInitInfo& rInfo,
    const al::ActorInitInfo& rDemoInfo, const sead::Matrix34f* pBaseMtx, sead::Matrix34f* pLocalMtx);
}

/** @brief Creates a demo camera with identity transforms. @param pHolder Game data used for player-count variants. */
DemoSceneCamera::DemoSceneCamera(const GameDataHolder* pHolder) : mGameDataHolder(pHolder) {
    mPlacementMtx.makeIdentity();
    mDemoMtx.makeIdentity();
    mLocalMtx.makeIdentity();
}

/**
 * @brief Loads camera animation resources and initializes the available camera system.
 * @param rInfo Camera placement and scene services.
 * @param rCameraInfo Parent camera placement data.
 * @param pMtx Optional parent placement transform.
 */
void DemoSceneCamera::initDemoSceneActor(const al::ActorInitInfo& rInfo,
    const al::ActorInitInfo& rCameraInfo, const sead::Matrix34f* pMtx) {
    mActions = new DemoActionList(rInfo, "Action");
    DemoSceneActorFunction::calcPlacementBaseMtx(&mPlacementMtx, rInfo, rCameraInfo, pMtx, &mLocalMtx);
    if (rInfo.mActorSceneInfo.cameraDirector)
        mCameraDirector = rInfo.mActorSceneInfo.cameraDirector;
    else
        mSceneCameraInfo = rInfo.mActorSceneInfo.sceneCameraInfo;
    mActionCount = mActions->getActionCount();
    mDemoMtx = mPlacementMtx;
    const char* archive = "DemoCamera";
    al::tryGetStringArg(&archive, rInfo, "ArcName");
    al::tryGetArg(&mUseFovAnim, rInfo, "UseFOVAnim");
    if (al::isExistFile(al::StringTmp<64>("ObjectData/%s.szs", archive).cstr())) {
        float nearClip = 50.0f;
        al::tryGetArg(&nearClip, rInfo, "NearClip");
        float farClip = 55000.0f;
        al::tryGetArg(&farClip, rInfo, "FarClip");
        al::Resource* resource = al::findOrCreateResource(al::StringTmp<256>("ObjectData/%s", archive), nullptr);
        if (al::isPlaced(rCameraInfo)) {
            if (mCameraDirector)
                mCameraTicket = al::initDemoAnimCamera(this, rCameraInfo, resource, &mDemoMtx, mActions->getActionName(0), mUseFovAnim);
            else
                mCameraInfo = al::initAnimCamera(this, rCameraInfo, resource, mActions->getActionName(0), mUseFovAnim);
        } else if (mCameraDirector) {
            mCameraTicket = al::initDemoAnimCamera(this, rCameraInfo, resource, &mDemoMtx, mActions->getActionName(0), mUseFovAnim);
        } else {
            mCameraInfo = al::initAnimCamera(this, rInfo, resource, mActions->getActionName(0), mUseFovAnim);
        }
    }
}

/** @brief Recomputes camera placement. @param pMtx Parent transform. */
void DemoSceneCamera::setPlacementBaseMtx(const sead::Matrix34f* pMtx) {
    mPlacementMtx.setMul(*pMtx, mLocalMtx);
    mDemoMtx = mPlacementMtx;
}

/** @brief Gets legacy camera services. @return Scene camera information. */
al::SceneCameraInfo* DemoSceneCamera::getSceneCameraInfo() const { return mSceneCameraInfo; }
/** @brief Gets RS camera services. @return Camera director. */
al::CameraDirector_RS* DemoSceneCamera::getCameraDirector_RS() const { return mCameraDirector; }

/** @brief Starts the indexed camera action. @param index Action index. */
void DemoSceneCamera::startAction(int index) { startCamera(index); }

/** @brief Starts a camera animation with the configured interpolation. @param index Action index. */
void DemoSceneCamera::startCamera(int index) {
    if (mCameraInfo) {
        if (mFovyDegree > 0.0f)
            al::setCameraFovyDegree(mCameraInfo, mFovyDegree);
        const char* action = mActions->getActionName(index);
        al::StringTmp<64> variant("%s%d", action, rc::getActiveControlUserNum(GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameDataHolder))));
        if (al::isExistAnimAnimCamera(mCameraInfo, variant.cstr()))
            al::startAnimCamera(this, mCameraInfo, variant.cstr(), &mPlacementMtx, mInterpolateFrames);
        else
            al::startAnimCamera(this, mCameraInfo, action, &mPlacementMtx, mInterpolateFrames);
    } else if (mCameraTicket) {
        const char* action = mActions->getActionName(index);
        if (al::isAnimCameraPlaying(mCameraTicket) || al::isActiveCamera(mCameraTicket))
            al::startAnimCameraAnim(mCameraTicket, action, -1, -1, -1);
        else
            al::startAnimCamera_RS(this, mCameraTicket, action, mInterpolateFrames);
    }
}

/** @brief Applies the demo scene transform. @param rMtx Demo transform. */
void DemoSceneCamera::startDemo(const sead::Matrix34f& rMtx) { mDemoMtx.setMul(rMtx, mPlacementMtx); }

/** @brief Ends animation playback and releases the camera. */
void DemoSceneCamera::endDemo() { endCameraAll(); }

/** @brief Ends camera playback through the active camera system. */
void DemoSceneCamera::endCameraAll() {
    if (mCameraInfo)
        al::endCamera(this, mCameraInfo, mInterpolateFrames);
    else if (mCameraTicket) {
        al::endAnimCamera_RS(this, mCameraTicket);
        al::endCamera_RS(this, mCameraTicket, mInterpolateFrames, false);
    }
}

/** @brief Attempts to play a named action. @param pName Camera animation name. */
void DemoSceneCamera::tryStartActionByName(const char* pName) {
    if (mCameraInfo) {
        if (mFovyDegree > 0.0f)
            al::setCameraFovyDegree(mCameraInfo, mFovyDegree);
        al::StringTmp<64> variant("%s%d", pName, rc::getActiveControlUserNum(GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameDataHolder))));
        if (al::isExistAnimAnimCamera(mCameraInfo, variant.cstr()))
            al::startAnimCamera(this, mCameraInfo, variant.cstr(), &mPlacementMtx, mInterpolateFrames);
        else
            al::startAnimCamera(this, mCameraInfo, pName, &mPlacementMtx, mInterpolateFrames);
    } else if (mCameraTicket && al::isExistAnimCameraData(mCameraTicket, pName)) {
        if (al::isAnimCameraPlaying(mCameraTicket) || al::isActiveCamera(mCameraTicket))
            al::startAnimCameraAnim(mCameraTicket, pName, -1, -1, -1);
        else
            al::startAnimCamera_RS(this, mCameraTicket, pName, mInterpolateFrames);
    }
}

/** @brief Releases the camera with the configured interpolation. @param index Unused action index. */
void DemoSceneCamera::endCamera(int index) {
    if (mCameraInfo)
        al::endCamera(this, mCameraInfo, mInterpolateFrames);
    else if (mCameraTicket)
        al::endCamera_RS(this, mCameraTicket, mInterpolateFrames, false);
}

/** @brief Tests animation completion. @param index Early completion window in frames. @return Whether the animation has ended. */
bool DemoSceneCamera::isEndAnimCamera(int index) const {
    if (mCameraInfo) {
        if (index > 0)
            return getMaxFrame() <= getCurrentFrame() + index;
        return al::isEndAnimCamera(mCameraInfo);
    }
    if (mCameraTicket) {
        if (index > 0)
            return getMaxFrame() <= getCurrentFrame() + index;
        return !al::isAnimCameraPlaying(mCameraTicket);
    }
    return true;
}

/** @brief Gets the current animation duration. @return Maximum frame. */
int DemoSceneCamera::getMaxFrame() const {
    if (mCameraInfo) return al::getMaxFrameAnimCamera(mCameraInfo);
    return al::getAnimCameraStepMax(mCameraTicket);
}

/** @brief Gets camera playback progress. @return Current frame. */
int DemoSceneCamera::getCurrentFrame() const {
    if (mCameraInfo) return al::getCurrentFrameAnimCamera(mCameraInfo);
    return al::getAnimCameraStep(mCameraTicket);
}

/** @brief Sets camera transition duration. @param frames Interpolation frames. */
void DemoSceneCamera::setInterpolateFrame(int frames) { mInterpolateFrames = frames; }

/** @brief Overrides the legacy camera field of view. @param degrees Vertical field of view in degrees. */
void DemoSceneCamera::setCameraFovyDegree(float degrees) { mFovyDegree = degrees; }

/** @brief Gets the duration of an indexed animation. @param index Action index. @return Maximum frame, or zero without a camera. */
int DemoSceneCamera::getMaxFrame(int index) const {
    if (mCameraInfo) {
        const char* action = mActions->getActionName(index);
        al::StringTmp<64> variant("%s%d", action, rc::getActiveControlUserNum(GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameDataHolder))));
        if (al::isExistAnimAnimCamera(mCameraInfo, variant.cstr()))
            return al::getCameraMaxFrame(mCameraInfo, variant.cstr());
        return al::getCameraMaxFrame(mCameraInfo, action);
    }
    if (mCameraTicket) {
        const char* action = mActions->getActionName(index);
        return al::calcAnimCameraAnimStepMax(mCameraTicket, action);
    }
    return 0;
}

/** @brief Seeks the RS camera animation. @param frame Target frame. */
void DemoSceneCamera::setCurrentFrame(int frame) {
    if (!mCameraInfo)
        al::setAnimCameraStep(mCameraTicket, frame);
}
