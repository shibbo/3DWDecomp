#include "MapObj/Stamp.hpp"

#include <agl/driver/aglGraphicsDriverMgr.h>
#include <g3d/aglNW4FToNN.h>
#include <g3d/aglShaderUtilG3D.h>
#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>
#include <nn/g3d/g3d_ModelObj.h>
#include <nn/g3d/g3d_ResFile.h>
#include <nn/g3d/g3d_ResShader.h>
#include <nn/g3d/g3d_Resources.h>
#include <nn/gfx/detail/gfx_MemoryPool-api.nvn.8.h>
#include <nn/gfx/gfx_MemoryPoolInfo.h>
#include <nn/gfx/gfx_ResTexture.h>
#include <nn/gfx/gfx_ResTextureData.h>
#include <nn/gfx/gfx_Texture.h>
#include <nn/util/util_ResDic.h>
#include <nvn/nvn_FuncPtrInline.h>
#include <prim/seadSafeString.h>

#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/Collision/CollisionPolygonUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Layout/LayoutResource.hpp"
#include "Library/Execute/ActorExecuteInfo.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Model/ModelDrawerDeferredExt.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Model/alModelCafe.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Resource/Resource.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shader/ForwardRendering/ShaderHolder.hpp"
#include "Library/Texture/TextureUtil.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/DrcTouchAssistInfo.hpp"
#include "MapObj/StampDirector.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Collision/HitDb.hpp"
#include "Project/Model/SimpleModelG3D.hpp"

namespace {
using TextureViewImpl = nn::gfx::detail::TextureViewImpl<nn::gfx::ApiVariationNvn8>;
using MemoryPoolImpl = nn::gfx::detail::MemoryPoolImpl<nn::gfx::ApiVariationNvn8>;
using rc::Stamp;

NERVE_DECL(Stamp, Appear);
NERVE_DECL(Stamp, Erase);
NERVE_DECL(Stamp, Sleep);
NERVE_DECL(Stamp, Held);
NERVE_DECL(Stamp, Hovered);
NERVE_DECL(Stamp, Placed);
NERVES_MAKE_NOSTRUCT(Stamp, Appear, Sleep, Held, Placed, Erase, Hovered)

/**
 * @brief Wraps an angle in degrees back into [0, 360) after a small step.
 * @param pDegree angle to wrap
 */
inline void wrapDegree(f32* pDegree) {
    if (*pDegree < 0.0f) {
        *pDegree += 360.0f;
    } else if (*pDegree >= 360.0f) {
        *pDegree -= 360.0f;
    }
}

/**
 * @brief Name dictionary of the textures in a texture file.
 * @param pTextureFile texture file
 * @return the dictionary
 */
inline const nn::util::ResDic* getTextureDic(const nn::gfx::ResTextureFile* pTextureFile) {
    return pTextureFile->ToData().textureContainerData.pTextureDic.Get();
}

/**
 * @brief Scale of a texture edge relative to the default stamp width.
 * @param size edge length in texels
 * @return the scale, at most 1
 */
inline f32 calcTextureEdgeScale(u32 size) {
    f64 width = rc::Stamp::getDefaultWidth();
    return std::fmin(static_cast<f64>(size), width) / width;
}

const sead::Vector3f cGravityDir(0.0f, -1.0f, 0.0f);
}  // namespace

namespace rc {

/**
 * @brief Rejects triangles with the "NoCode" material.
 * @param rTriangle triangle to check
 * @return true if the triangle is ignored
 */
bool TriangleWallFilter::isInvalidTriangle(const al::Triangle& rTriangle) const {
    return al::isMaterialCode("NoCode", rTriangle);
}

/**
 * @brief Rejects walls with the "NoAction" wall code and triangles with the "NoCode" material.
 * @param rTriangle triangle to check
 * @return true if the triangle is ignored
 */
bool TriangleWallNoCodeFilter::isInvalidTriangle(const al::Triangle& rTriangle) const {
    if (al::isWallPolygon(*rTriangle.getFaceNormal(), cGravityDir) &&
        al::isWallCode("NoAction", rTriangle)) {
        return true;
    }

    return al::isMaterialCode("NoCode", rTriangle);
}

nn::gfx::ResTextureFile* Stamp::spTextureFile[100];

/**
 * @brief Name every stamp actor is created with.
 * @return the actor name
 */
const char* Stamp::getStampActorName() {
    return "StampActor";
}

/**
 * @brief Checks whether an actor is a stamp.
 * @param pActor actor to check, may be nullptr
 * @return true if the actor is a stamp
 */
bool Stamp::isStampActor(const al::LiveActor* pActor) {
    if (pActor != nullptr && al::isEqualString(pActor->getName(), getStampActorName())) {
        return true;
    }

    return false;
}

/**
 * @brief Creates a dead stamp.
 * @param rInfo actor init info
 * @param pArchiveName model archive name
 * @param pTouchInfo touch screen state
 * @param pDirector owning stamp director
 * @param depth offset of the stamp along the hit normal (stacking order)
 */
Stamp::Stamp(const al::ActorInitInfo& rInfo, const char* pArchiveName,
             const DrcTouchAssistInfo* pTouchInfo, StampDirector* pDirector, s32 depth)
    : al::LiveActor(getStampActorName()), mDrcTouchAssistInfo(pTouchInfo),
      mStampDirector(pDirector), mDepth(depth) {
    mCameraRotate = 0.0f;
    mOffsetRate = 0.0f;
    mOffsetDir = sead::Vector3f::zero;
    mPlacedRotation = 0.0f;
    mRotation = 0.0f;
    al::initActorWithArchiveNameNoPlacementInfo(this, rInfo, pArchiveName, nullptr);
    al::initNerve(this, &NrvStampAppear, 0);
    mTextureReplacer = new al::TextureReplacer();
    makeActorDead();
}

/**
 * @brief Replaces the touch screen state.
 * @param pTouchInfo touch screen state
 */
void Stamp::setDrcTouchAssistInfo(const DrcTouchAssistInfo* pTouchInfo) {
    mDrcTouchAssistInfo = pTouchInfo;
}

/**
 * @brief Looks up the stamp texture sampler and hooks the deferred draw callback.
 */
void Stamp::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();
    const nn::g3d::ResShadingModel* shadingModel =
        al::ShaderHolder::instance()->getShadingModel("RenderScreenSpaceDecal");
    const nn::g3d::ResShaderProgram* program = shadingModel->GetProgram(0);
    agl::g3d::ShaderUtilG3D::search(&mSamplerLocation, shadingModel, program, "_a0");
    auto* drawer = static_cast<al::ModelDrawerDeferredExt*>(getExecuteInfo()->getDrawer(0));

    if (drawer != nullptr) {
        drawer->setDrawCallback(&Stamp::draw, this);
    }
}

/**
 * @brief Deferred draw callback: draws the stamp owning the drawn model.
 * @param pUserData the stamp that registered the callback
 * @param pModel model being drawn
 */
void Stamp::draw(void* pUserData, void* pModel) {
    Stamp* stamp = static_cast<Stamp*>(pUserData)->mStampDirector->getStampFromModel(
        static_cast<alModelCafe*>(pModel));

    if (stamp != nullptr) {
        stamp->draw();
    }
}

/**
 * @brief Shows the stamp at the touch position.
 * @param stampId stamp to switch to (resetting its rotation), or a negative value to keep the
 *        current one
 */
void Stamp::appear(s32 stampId) {
    mOffsetRate = 0.0f;
    mOffsetDir = sead::Vector3f::zero;
    mIsFirstUpdate = true;
    al::LiveActor::appear();
    al::setNerve(this, &NrvStampAppear);

    if (stampId >= 0) {
        mStampId = stampId;
        mPlacedRotation = 0.0f;
        mRotation = 0.0f;
        mCameraRotate = calculateCameraRotate();
        al::getQuatPtr(this)->set(sead::Quatf::unit);
    }

    setStamp();
    updatePos(true);
    al::invalidateClipping(this);
}

/**
 * @brief Calculates the yaw of the camera.
 * @return the camera yaw in degrees
 */
f32 Stamp::calculateCameraRotate() {
    const sead::LookAtCamera* camera = al::getCameraLookAtCamera(this);
    sead::Vector3f dir = camera->getAt() - camera->getPos();
    dir.normalize();
    return sead::Mathf::rad2deg(atan2f(dir.x, dir.z));
}

/**
 * @brief Applies the texture and touch radius of the current stamp from the stamp list.
 */
void Stamp::setStamp() {
    al::ByamlIter list(
        al::findResourceYaml(mStampDirector->getStampResource(), "StampList", nullptr));
    list.getSize();
    al::ByamlIter entry;

    if (!list.tryGetIterByIndex(&entry, mStampId)) {
        return;
    }

    const char* itemType = nullptr;
    entry.tryGetStringByKey(&itemType, "IllustItemType");
    // Course stamps also carry a course id, which is read but not used here.
    s32 courseId;

    if (al::isEqualString(itemType, "Course")) {
        courseId = 0;
        entry.tryGetIntByKey(&courseId, "CourseId");
    }

    {
        const char* itemName = nullptr;
        entry.tryGetStringByKey(&itemName, "IllustItemName");
        trySetTexture(itemName);
    }

    f32 radius = 200.0f;

    if (entry.tryGetFloatByKey(&radius, "Radius")) {
        al::setScreenPointTargetRadius(this, "Body", radius);
    }
}

/**
 * @brief Moves the stamp onto the surface under the touch position, or hides it.
 * @param isForce check even without a touch
 */
void Stamp::updatePos(bool isForce) {
    mIsHidden = !calcHitPosAndNormal(isForce);

    if (!mIsHidden) {
        f32 depth = mDepth + 7.5f;
        mOffsetRate += (0.0f - mOffsetRate) * 0.25f;
        sead::Vector3f trans = mHitPos + mHitNormal * depth + mOffsetDir * mOffsetRate;
        f32 rate = mIsFirstUpdate ? 1.0f : 0.25f;
        al::setTrans(this, trans);
        sead::Quatf* quat = al::getQuatPtr(this);
        al::turnQuatYDirRate(quat, *quat, mHitNormal, rate);
        mIsFirstUpdate = false;
        al::showModelIfHide(this);
    } else {
        al::hideModelIfShow(this);
    }
}

/**
 * @brief Sensor messages are ignored.
 * @param pMsg message
 * @param pSelf own sensor
 * @param pOther other sensor
 * @return always false
 */
bool Stamp::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) {
    return false;
}

/**
 * @brief Picks the stamp up or drops it on touch messages.
 * @param pMsg message
 * @param pPointer screen pointer
 * @param pTarget own screen point target
 * @return false while erasing, true otherwise
 */
bool Stamp::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                  al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvStampErase)) {
        return false;
    }

    if (mStampDirector != nullptr) {
        if (al::isMsgTouchCarryItem(pMsg)) {
            mStampDirector->activateStamp(this, -1);
            return true;
        }

        if (al::isMsgTouchReleaseItem(pMsg)) {
            mStampDirector->releaseStamp(this);
        }
    }

    return true;
}

/**
 * @brief Checks whether the stamp can be picked up.
 * @return true unless the stamp is being erased
 */
bool Stamp::canGrab() const {
    return !al::isNerve(this, &NrvStampErase);
}

/**
 * @brief Does nothing.
 */
void Stamp::forcePlace() {
}

/**
 * @brief Puts the stamp to sleep.
 */
void Stamp::sleep() {
    al::setNerve(this, &NrvStampSleep);
}

/**
 * @brief Starts dragging the stamp.
 * @param isKeepRotation keep the current rotation and skip the grab sound
 */
void Stamp::startGrab(bool isKeepRotation) {
    if (!isKeepRotation) {
        mCameraRotate = calculateCameraRotate();
        sead::Vector3f front;
        al::calcFrontDir(&front, this);
        f32 angle = sead::Mathf::rad2deg(atan2f(front.x, front.z));
        mRotation = angle > 0.0f ? 360.0f - angle : -angle;
        al::startSe(mStampDirector->getAudioKeeperUser(), "PgStampGrab");
    }

    al::setNerve(this, &NrvStampHeld);
}

/**
 * @brief Starts hovering, unless the stamp is being erased.
 */
void Stamp::startHover() {
    if (al::isNerve(this, &NrvStampErase)) {
        return;
    }

    al::setNerve(this, &NrvStampHovered);
}

/**
 * @brief Sticks the stamp onto the surface.
 */
void Stamp::startPlaced() {
    al::setNerve(this, &NrvStampPlaced);
}

/**
 * @brief Starts erasing the stamp.
 * @param rVelocity velocity the stamp moves away with
 */
void Stamp::startErase(const sead::Vector3f& rVelocity) {
    mEraseVelocity = rVelocity;
    al::setNerve(this, &NrvStampErase);
}

/**
 * @brief Rotates the stamp around its normal.
 * @param degree rotation step in degrees
 */
void Stamp::doRotate(f32 degree) {
    mRotation += degree;
    wrapDegree(&mRotation);
    sead::Vector3f front = sead::Vector3f::ex;
    al::calcFrontDir(&front, this);
    sead::Quatf* quat = al::getQuatPtr(this);
    sead::Vector3f axis = degree < 0.0f ? -front : front;
    al::turnQuatXDirRadian(quat, *quat, axis, sead::Mathf::deg2rad(std::fabs(degree)));
}

/**
 * @brief Rotates the stamp by a user input amount.
 * @param amount input amount
 */
void Stamp::rotateStamp(f32 amount) {
    doRotate(amount * 2.5f);
}

/**
 * @brief Casts a ray from the camera through the touch position onto the stage.
 * @param isForce check even without a touch
 * @return true if a surface was hit; mHitPos and mHitNormal are updated then
 */
bool Stamp::calcHitPosAndNormal(bool isForce) {
    if (!mDrcTouchAssistInfo->isTouch() && !isForce) {
        return false;
    }

    sead::Vector3f cameraPos = al::getCameraPosSub(this);
    sead::Vector3f dir;
    sead::Vector3f target;

    if (mDrcTouchAssistInfo->isUseScreenPos()) {
        sead::Vector2f screenPos = mDrcTouchAssistInfo->getScreenPos();
        screenPos.y -= 30.0f;
        al::calcWorldPosFromScreenPosSub(&target, this, screenPos, -1.0f);
    } else {
        target = mDrcTouchAssistInfo->getTouchPos();
    }

    al::normalizeOrZero(&dir, target - cameraPos);
    al::CollisionPartsFilterSpecialPurpose partsFilter("DrcAssist");
    TriangleWallFilter wallFilter;
    TriangleWallNoCodeFilter noCodeFilter;
    sead::Vector3f arrow = dir * mCheckLength;
    sead::Vector3f hitPos = cameraPos + arrow;
    const al::TriangleFilterBase* triFilter = &wallFilter;

    if (mStampDirector->isUseNoCodeWallFilter()) {
        triFilter = &noCodeFilter;
    }

    u32 hitNum = alCollisionUtil::checkStrikeArrow(this, cameraPos + dir * 500.0f, arrow,
                                                   &partsFilter, triFilter);

    if (hitNum == 0) {
        return false;
    }

    f32 minDist = sead::Mathf::maxNumber();
    const al::ArrowHitInfo* nearest = nullptr;

    for (u32 i = 0; i < hitNum; i++) {
        const al::ArrowHitInfo* hitInfo = alCollisionUtil::getStrikeArrowInfo(this, i);

        if (hitInfo->mTriangle.isValid() && al::isFloorCode("IgnoreTouch", hitInfo->mTriangle)) {
            continue;
        }

        hitInfo->mTriangle.getSensor();

        if (minDist > hitInfo->_70) {
            minDist = hitInfo->_70;
            hitPos = hitInfo->mPos;
            nearest = hitInfo;
        }
    }

    if (nearest == nullptr) {
        return false;
    }

    mHitPos = hitPos;
    mHitNormal.set(*nearest->mTriangle.getFaceNormal());
    al::normalizeOrZero(&mHitNormal);
    return true;
}

/**
 * @brief Applies the stamp and follows the touch position.
 */
void Stamp::exeAppear() {
    if (al::isFirstStep(this)) {
        setStamp();
    }

    updatePos(false);
}

/**
 * @brief Does nothing.
 */
void Stamp::exeDisappear() {
}

/**
 * @brief Waits.
 */
void Stamp::exeSleep() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
}

/**
 * @brief Follows the touch position and turns the stamp along with the camera.
 */
void Stamp::exeHeld() {
    if (al::isFirstStep(this)) {
        mIsFirstUpdate = true;
        al::startAction(this, "GrabIdle");
    }

    f32 cameraRotate = calculateCameraRotate();
    f32 diff = cameraRotate - mCameraRotate;

    if (diff > 180.0f) {
        diff -= 360.0f;
    } else if (diff < -180.0f) {
        diff += 360.0f;
    }

    if (!al::isNearZero(std::fabs(diff), 0.001f)) {
        doRotate(-diff);
        mCameraRotate = cameraRotate;
    }

    updatePos(false);
}

/**
 * @brief Plays the hover animation.
 */
void Stamp::exeHovered() {
    if (al::isFirstStep(this)) {
        if (!al::isActionPlaying(this, "Hover")) {
            al::startAction(this, "Hover");
        }
    } else {
        al::startAction(this, "Wait");
    }
}

/**
 * @brief Settles the stamp onto the surface, then puts it to sleep.
 */
void Stamp::exePlaced() {
    if (al::isFirstStep(this)) {
        mPlacedRotation = mRotation;
        al::startAction(this, "Place");
    }

    mOffsetRate += (0.0f - mOffsetRate) * 0.25f;
    sead::Vector3f trans = mHitPos + mHitNormal * (mDepth + 7.5f) + mOffsetDir * mOffsetRate;
    al::setTrans(this, trans);
    sead::Quatf* quat = al::getQuatPtr(this);
    al::turnQuatYDirRate(quat, *quat, mHitNormal, 0.25f);

    if (al::isActionEnd(this)) {
        trans = mHitPos + mHitNormal * (mDepth + 7.5f);
        al::setTrans(this, trans);
        quat = al::getQuatPtr(this);
        al::turnQuatYDirRate(quat, *quat, mHitNormal, 1.0f);
        al::setNerve(this, &NrvStampSleep);
    }
}

/**
 * @brief Shrinks and fades the stamp out, then kills it.
 */
void Stamp::exeErase() {
    if (al::isGreaterEqualStep(this, 10)) {
        al::setScale(this, sead::Vector3f::ones);
        mGlobalAlphaLastFrame = 1.0f;
        al::setNerve(this, &NrvStampSleep);
        kill();
        return;
    }

    sead::Vector3f scale(mTextureScale.x, 1.0f, mTextureScale.y);
    f32 rate = al::calcNerveRate(this, 10);
    al::lerpVec(&scale, scale, sead::Vector3f::zero, rate);
    al::setTrans(this, al::getTrans(this) + mEraseVelocity);
    al::setScale(this, scale);
    mGlobalAlphaLastFrame = 1.0f - rate;
}

/**
 * @brief Does nothing.
 */
void Stamp::control() {
}

/**
 * @brief Runs the default actor movement.
 */
void Stamp::movement() {
    al::LiveActor::movement();
}

/**
 * @brief Stamps are drawn through the deferred draw callback only.
 */
void Stamp::draw() const {
}

/**
 * @brief Sets the rotation around the normal and turns the model to match.
 * @param degree rotation in degrees
 */
void Stamp::setRotation(f32 degree) {
    mRotation = degree;

    while (degree >= 2.5f) {
        sead::Vector3f front = sead::Vector3f::ex;
        al::calcFrontDir(&front, this);
        sead::Quatf* quat = al::getQuatPtr(this);
        al::turnQuatXDirRadian(quat, *quat, front, sead::Mathf::deg2rad(2.5f));
        degree -= 2.5f;
    }

    if (degree > 0.0f) {
        sead::Vector3f front = sead::Vector3f::ex;
        al::calcFrontDir(&front, this);
        sead::Quatf* quat = al::getQuatPtr(this);
        al::turnQuatXDirRadian(quat, *quat, front, sead::Mathf::deg2rad(degree));
    }
}

/**
 * @brief Scale of the stamp texture relative to the default width.
 * @return the texture scale
 */
const sead::Vector2f& Stamp::getTextureScale() const {
    sead::Vector2f scale(calcTextureEdgeScale(mTextureData.getWidth(0)),
                         calcTextureEdgeScale(mTextureData.getHeight(0)));
    return scale;
}

/**
 * @brief Width in texels of a full size stamp texture.
 * @return the default width
 */
f32 Stamp::getDefaultWidth() {
    return 320.0f;
}

/**
 * @brief Screen position the stamp is attached to.
 * @return the touch position on the screen, slightly raised
 */
sead::Vector2f Stamp::get2DPos() const {
    sead::Vector2f pos = mDrcTouchAssistInfo->getScreenPos();
    pos.y -= 30.0f;
    return pos;
}

/**
 * @brief Rotation of the stamp as seen on the touch screen.
 * @return rotation as a fraction of a full turn
 */
f32 Stamp::getUIRotation() const {
    f32 rotation = mRotation + mCameraRotate - 180.0f;
    wrapDegree(&rotation);
    return rotation * (1.0f / 360.0f);
}

/**
 * @brief Binds a stamp texture to the model.
 * @param pTextureName texture name, looked up in the stamp layout first, then in the model
 */
void Stamp::trySetTexture(const char* pTextureName) {
    mTextureName = pTextureName;
    nn::gfx::ResTextureFile* textureFile =
        mStampDirector->getLayoutResource()->getArchiveList().begin()->mTextureFile;

    if (textureFile == nullptr) {
        return;
    }

    al::StringTmp<128> layoutTextureName("%s^", pTextureName);
    s32 count = getTextureDic(textureFile)->GetCount();
    s32 index = -1;

    for (s32 i = 0; i < count; i++) {
        const char* name =
            getTextureDic(textureFile)->ToData().entries[i + 1].pKey.Get()->GetData();

        if (al::isEqualSubString(name, layoutTextureName.cstr())) {
            index = i;
            break;
        }
    }

    if (index < 0) {
        nn::g3d::ResFile* resFile = mStampDirector->getStampResource()->getResFile();

        if (resFile == nullptr) {
            return;
        }

        textureFile = agl::g3d::ResFile::getResTextureFile(resFile);

        if (textureFile == nullptr) {
            return;
        }

        index = getTextureDic(textureFile)->FindIndex(pTextureName);

        if (index < 0) {
            return;
        }
    }

    nn::gfx::ResTexture* texture =
        textureFile->ToData().textureContainerData.pTexturePtrArray.Get()[index].Get();

    if (texture == nullptr) {
        return;
    }

    auto* view = static_cast<TextureViewImpl*>(texture->ToData().pTextureView.Get());
    view->ToData()->userPtr = texture;
    auto* nvnTexture = static_cast<NVNtexture*>(view->ToData()->pNvnTexture.ptr);
    nvnTextureSetDebugLabel(nvnTexture, pTextureName);
    mTextureData.setDebugLabel(pTextureName);
    mTextureData.initializeFromNVNtexture(*nvnTexture);
    mTextureData.invalidateCPUCache();
    mTextureReplacer->setup(&mTextureData);
    mTextureData.getTexture().setReference_();
    mTextureScale.x = calcTextureEdgeScale(mTextureData.getWidth(0));
    mTextureScale.y = calcTextureEdgeScale(mTextureData.getHeight(0));
    al::setScale(this, {mTextureScale.x, 1.0f, mTextureScale.y});
    nn::g3d::ModelObj* modelObj =
        getModelKeeper()->getModelCafe()->getModelG3D()->getModelObj();

    for (s32 i = 0; i < modelObj->GetNumMaterials(); i++) {
        nn::g3d::MaterialObj* material = modelObj->GetMaterial(i);

        for (s32 j = 0; j < material->GetResource()->GetTextureCount(); j++) {
            material->SetTexture(j, *reinterpret_cast<const nn::g3d::TextureRef*>(
                                        mTextureReplacer->getTextureRef()));
        }

        material->ResetDirtyFlags();
    }
}

/**
 * @brief Switches to another stamp.
 * @param stampId stamp to show
 */
void Stamp::setStamp(s32 stampId) {
    mStampId = stampId;
    setStamp();
}

/**
 * @brief Sets up the memory pool of a stamp texture file once.
 * @param pTextureFile texture file
 * @return true if the file was set up now, false if it already was
 */
bool Stamp::initializeResTextureFile(nn::gfx::ResTextureFile* pTextureFile) {
    u32 count = 0;

    for (nn::gfx::ResTextureFile* textureFile : spTextureFile) {
        if (textureFile == pTextureFile) {
            return false;
        }

        if (textureFile != nullptr) {
            count++;
        }
    }

    auto* device = static_cast<nn::gfx::Device*>(
        agl::driver::GraphicsDriverMgr::instance()->getGfxDevice());
    nn::gfx::ResTextureContainerData& container = pTextureFile->ToData().textureContainerData;
    nn::gfx::MemoryPoolInfoData infoData = {};
    infoData.memoryPoolProperty = 0x21;
    nn::gfx::MemoryPoolInfo& info = nn::gfx::DataToAccessor(infoData);
    auto* header = static_cast<nn::util::BinaryBlockHeader*>(container.pTextureData.Get());
    info.SetPoolMemory(header + 1, header->GetBlockSize() - sizeof(nn::util::BinaryBlockHeader));
    static_cast<MemoryPoolImpl*>(container.pTextureMemoryPool.Get())->Initialize(device, info);
    container.pCurrentMemoryPool.Set(container.pTextureMemoryPool.Get());
    container.memoryPoolOffsetBase = 0;
    spTextureFile[count] = pTextureFile;
    return true;
}

}  // namespace rc
