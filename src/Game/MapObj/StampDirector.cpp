#include "MapObj/StampDirector.hpp"

#include <eui/euiUtility.h>
#include <gfx/nin/seadGraphicsNvn.h>
#include <prim/seadSafeString.h>
#include <cstring>

#include "Layout/Switch/SnapshotLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/HitSensor/HitSensor.hpp"
#include "Library/Layout/LayoutActor.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutResource.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Scene/SceneObjHolder.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/DrcAssistDirector.hpp"
#include "MapObj/DrcAssistDirectorList.hpp"
#include "MapObj/Stamp.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Collision/HitDb.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/Data/StampType.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"

namespace rc {

namespace {
/** @brief Stamp unlock condition, as returned by StampType::calcStampTypeID. */
enum StampUnlockType {
    StampUnlockType_Course = 0,
    StampUnlockType_Default = 1,
    StampUnlockType_MarioComplete = 2,
    StampUnlockType_LuigiComplete = 3,
    StampUnlockType_PeachComplete = 4,
    StampUnlockType_KinopioComplete = 5,
    StampUnlockType_RosettaComplete = 6,
};
}  // namespace

/**
 * @brief Resets the unlock list.
 * @param pFlags initial unlock bit set (128 bits)
 */
void StampDirector::StampUnlockInfo::init(u32* pFlags) {
    mCount = 0;

    if (mFlags != pFlags) {
        std::memcpy(mFlags, pFlags, sizeof(mFlags));
    }

    mStampIds.fill(0xFF);
}

/**
 * @brief Appends a stamp at the end of the unlock list.
 * @param stampId stamp to unlock
 */
void StampDirector::StampUnlockInfo::addStamp(s32 stampId) {
    setFlag(stampId);
    mStampIds[mCount] = stampId;
    mCount++;
}

/**
 * @brief Inserts a stamp in the sorted unlock list.
 * @param stampId stamp to unlock
 * @return index the stamp was inserted at, or -1 if it was already unlocked
 */
s32 StampDirector::StampUnlockInfo::insertStamp(s32 stampId) {
    for (s32 i = 0; i < mCount; i++) {
        if (mStampIds[i] == stampId) {
            return -1;
        }

        if (mStampIds[i] > stampId) {
            for (s32 j = mCount; j > i; j--) {
                mStampIds[j] = mStampIds[j - 1];
            }

            mStampIds[i] = stampId;
            setFlag(stampId);
            mCount++;
            return i;
        }
    }

    return -1;
}

/**
 * @brief Inserts a stamp at a given position of the unlock list.
 * @param index position to insert at
 * @param stampId stamp to unlock
 */
void StampDirector::StampUnlockInfo::insertStamp(s32 index, s32 stampId) {
    setFlag(stampId);

    for (s32 i = mCount - 1; i > index; i--) {
        mStampIds[i + 1] = mStampIds[i];
    }

    mStampIds[index] = stampId;
    mCount++;
}

/**
 * @brief Checks whether a stamp of the stamp list is unlocked.
 * @param rIter stamp list entry
 * @param type unlock condition (StampUnlockType)
 * @param accessor game data accessor
 * @param isCheckFile whether only one save file is checked
 * @param fileId save file to check
 * @param pCourseId receives the course of course stamps, or -1
 * @return whether the stamp is unlocked
 */
static bool isUnlockStamp(const al::ByamlIter& rIter, s32 type, GameDataHolderAccessor accessor,
                          bool isCheckFile, s32 fileId, s32* pCourseId) {
    *pCourseId = -1;

    switch (type) {
    case StampUnlockType_Course: {
        s32 courseId = 0;

        if (rIter.tryGetIntByKey(&courseId, "CourseId")) {
            *pCourseId = courseId;
            return CourseInfoFunction::isAcquireIllustItem(accessor, courseId, isCheckFile,
                                                           fileId);
        }

        break;
    }
    case StampUnlockType_Default:
        return true;
    case StampUnlockType_MarioComplete:
        return GameDataFunction::isAcquireCharacterCompleteIllustItem(accessor, 0, isCheckFile,
                                                                      fileId);
    case StampUnlockType_LuigiComplete:
        return GameDataFunction::isAcquireCharacterCompleteIllustItem(accessor, 1, isCheckFile,
                                                                      fileId);
    case StampUnlockType_PeachComplete:
        return GameDataFunction::isAcquireCharacterCompleteIllustItem(accessor, 2, isCheckFile,
                                                                      fileId);
    case StampUnlockType_KinopioComplete:
        return GameDataFunction::isAcquireCharacterCompleteIllustItem(accessor, 3, isCheckFile,
                                                                      fileId);
    case StampUnlockType_RosettaComplete:
        return GameDataFunction::isAcquireCharacterCompleteIllustItem(accessor, 4, isCheckFile,
                                                                      fileId);
    default:
        break;
    }

    return false;
}

/**
 * @brief Creates the stamps, the stamp layout and the unlock lists.
 * @param rInfo actor init info
 * @param pArchiveName stamp model archive
 * @param pTouchInfo touch assist info given to the stamps
 * @param stampNum number of stamp actors
 * @param courseId current course
 */
StampDirector::StampDirector(const al::ActorInitInfo& rInfo, const char* pArchiveName,
                             const DrcTouchAssistInfo* pTouchInfo, s32 stampNum, s32 courseId) {
    mCourseId = courseId;
    mCollectStampIndex = -1;
    mCollectStampId = -1;
    mRotation = 0.0f;
    mStampResource = al::findOrCreateResource("SystemData/StampData", nullptr);
    al::Resource* layoutResource = al::findOrCreateResource("LayoutData/ListStampParts", nullptr);

    {
        al::LayoutAllocatorInScope allocatorScope;
        mLayoutResource = new al::LayoutResource(
            layoutResource, rInfo.getLayoutInitInfo()->getLayoutSystem(), false);
        mLayoutResource->loadAllTextures(
            reinterpret_cast<nn::gfx::Device*>(sead::GraphicsNvn::instance()->getGfxDevice()));
        mLayoutResource->RegisterTextureViewToDescriptorPool(eui::RegisterSlotForTexture,
                                                             nullptr);
    }

    mStamps = new sead::PtrArray<Stamp>();
    mStamps->allocBuffer(stampNum, nullptr);

    for (s32 i = 0; i < mStamps->capacity(); i++) {
        mStamps->pushBack(new Stamp(rInfo, pArchiveName, pTouchInfo, this, i));
    }

    al::ByamlIter stampList(al::findResourceYaml(mStampResource, "StampList", nullptr));
    mStampNum = stampList.getSize();
    u32 flags[4] = {};
    mUnlockInfo.init(flags);

    for (s32 i = 0; i < cFileUnlockInfoNum; i++) {
        mFileUnlockInfos[i].init(flags);
    }

    bool isSingleMode = al::isSingleMode(rInfo);
    Stamp* firstStamp = mStamps->front();
    mTextureInfo = al::createTextureInfo();
    mLayoutActor = new al::LayoutActor("StampLayout");
    al::initLayoutActor(mLayoutActor, al::getLayoutInitInfo(rInfo), "StampIcon", nullptr);

    for (s32 i = 0; i < mStampNum; i++) {
        s32 courseId;
        al::ByamlIter stampIter;

        if (!stampList.tryGetIterByIndex(&stampIter, i)) {
            continue;
        }

        const char* typeName;
        s32 type;

        if (stampIter.tryGetStringByKey(&typeName, "IllustItemType")) {
            type = StampType::calcStampTypeID(typeName);
        } else {
            type = StampUnlockType_Course;
        }

        if (isUnlockStamp(stampIter, type, firstStamp, isSingleMode, -1, &courseId)) {
            mUnlockInfo.addStamp(i);
        } else if (mCourseId >= 0 && mCourseId == courseId) {
            mCollectStampId = i;
            mCollectStampIndex = mUnlockInfo.mCount;
        }

        for (s32 fileId = 0; fileId < cFileNum; fileId++) {
            if (isUnlockStamp(stampIter, type, firstStamp, true, fileId, &courseId)) {
                mFileUnlockInfos[fileId].addStamp(i);
            }
        }

        if (isUnlockStamp(stampIter, type, firstStamp, true, -1, &courseId)) {
            mFileUnlockInfos[cFileNum].addStamp(i);
        }
    }

    mStartingStampId = 87;
}

/**
 * @brief Releases the stamp layout resource.
 */
StampDirector::~StampDirector() {
    mLayoutResource->UnregisterTextureViewFromDescriptorPool(eui::UnregisterSlotForTexture,
                                                             nullptr);
    al::LayoutAllocatorInScope allocatorScope;
    mLayoutResource->Finalize(
        reinterpret_cast<nn::gfx::Device*>(sead::GraphicsNvn::instance()->getGfxDevice()));
}

/**
 * @brief Copies the stamps unlocked in a save file.
 * @param pOutStampIds receives the stamp ids
 * @param fileIndex save file unlock list index
 * @return number of stamps written
 */
s32 StampDirector::getAllUnlockedStamps(s32* pOutStampIds, s32 fileIndex) {
    StampUnlockInfo& info = mFileUnlockInfos[fileIndex];
    s32 i;

    for (i = 0; i < info.mCount; i++) {
        pOutStampIds[i] = info.mStampIds[i];
    }

    return i;
}

/**
 * @brief Gives the stamps the touch assist info of the main controller.
 * @param rInfo actor init info
 */
void StampDirector::initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) {
    auto* assistList = static_cast<DrcAssistDirectorList*>(
        rInfo.getActorSceneInfo().sceneObjHolder->getObj(SceneObjID_DrcAssistDirectorList));

    if (assistList == nullptr) {
        return;
    }

    DrcAssistDirector* assist = assistList->getDrcAssist(al::getMainControllerPort());

    for (Stamp& stamp : *mStamps) {
        stamp.setDrcTouchAssistInfo(assist->getTouchAssistInfo());
    }

    mTouchAssistInfo = assist->getTouchAssistInfo();
}

/**
 * @brief Adds the stamp collected in this course to a save file's unlock list.
 * @param fileIndex save file unlock list index
 * @return whether there was a stamp to collect
 */
bool StampDirector::updateUnlockedStamps(s32 fileIndex) {
    if (mCollectStampId < 0) {
        return false;
    }

    mFileUnlockInfos[fileIndex].insertStamp(mCollectStampId);
    return true;
}

/**
 * @brief Resets the stamp rotation.
 */
void StampDirector::start() {
    mRotation = 0.0f;
}

/**
 * @brief Hides the stamp layout.
 */
void StampDirector::finish() {
    if (mLayoutActor->isAlive()) {
        mLayoutActor->kill();
    }
}

/**
 * @brief Applies the stamp inputs and updates the stamp layout.
 * @param rotateState rotation input
 * @param incrementState stamp selection input
 * @return whether the selected stamp changed
 */
bool StampDirector::update(StampRotateState rotateState, StampIncrementState incrementState) {
    bool isChanged = false;

    if (mActiveStamp != nullptr) {
        if (rotateState != StampRotateState_None) {
            if (rotateState == StampRotateState_Left) {
                mActiveStamp->rotateStamp(-1.0f);
            } else if (rotateState == StampRotateState_Right) {
                mActiveStamp->rotateStamp(1.0f);
            }

            if (mAudioKeeperUser != nullptr) {
                al::holdSe(mAudioKeeperUser, "PgStampRoll", nullptr);
            }
        }

        switch (incrementState) {
        case StampIncrementState_None:
            mRepeatTimer = 10;
            mIncrementState = incrementState;
            break;
        case StampIncrementState_Increment:
            if (mRepeatTimer > 0 && mIncrementState == StampIncrementState_Increment) {
                mRepeatTimer--;
            } else {
                incrementStamp();
                mRepeatTimer = 10;
                isChanged = true;
            }

            mIncrementState = incrementState;
            break;
        case StampIncrementState_Decrement:
            if (mRepeatTimer > 0 && mIncrementState == StampIncrementState_Decrement) {
                mRepeatTimer--;
            } else {
                decrementStamp();
                mRepeatTimer = 10;
                isChanged = true;
            }

            mIncrementState = incrementState;
            break;
        default:
            break;
        }
    }

    if (mActiveStamp != nullptr && mActiveStamp->isHidden()) {
        if (!mLayoutActor->isAlive()) {
            mLayoutActor->appear();
        }
    } else if (mLayoutActor->isAlive()) {
        mLayoutActor->kill();
    }

    if (mLayoutActor->isAlive()) {
        sead::Vector2f pos = mActiveStamp->get2DPos();
        al::calcLayoutPosFromScreenPos(&pos, pos);
        al::setPaneLocalTrans(mLayoutActor, "Stamp", pos);
        al::setPaneLocalScale(mLayoutActor, "Stamp", mActiveStamp->getCurrentTextureScale());
        al::setPaneLocalRotate(mLayoutActor, "Stamp",
                               {0.0f, 0.0f, mActiveStamp->getUIRotation() * -360.0f});
        al::setPaneLocalAlpha(mLayoutActor, "Stamp", 100.0f);
    }

    return isChanged;
}

/**
 * @brief Switches the held stamp to the next unlocked stamp.
 */
void StampDirector::incrementStamp() {
    if (mActiveStamp == nullptr) {
        return;
    }

    if (mAudioKeeperUser != nullptr) {
        al::startSe(mAudioKeeperUser, "PgStampSelect", nullptr);
    }

    mActiveStamp->setStamp(getNextStampID(mActiveStamp->getStampId(), 1));
    mStartingStampId = mActiveStamp->getStampId();
    refreshStampLayoutTexture();
}

/**
 * @brief Switches the held stamp to the previous unlocked stamp.
 */
void StampDirector::decrementStamp() {
    if (mActiveStamp == nullptr) {
        return;
    }

    if (mAudioKeeperUser != nullptr) {
        al::startSe(mAudioKeeperUser, "PgStampSelect", nullptr);
    }

    mActiveStamp->setStamp(getNextStampID(mActiveStamp->getStampId(), -1));
    mStartingStampId = mActiveStamp->getStampId();
    refreshStampLayoutTexture();
}

/**
 * @brief Makes the held stamp appear.
 * @param stampId stamp to show
 */
void StampDirector::appearStamp(s32 stampId) {
    if (mActiveStamp != nullptr) {
        mActiveStamp->appear(stampId);
    }
}

/**
 * @brief Puts the held stamp to sleep.
 */
void StampDirector::sleepStamp() {
    if (mActiveStamp != nullptr) {
        mActiveStamp->sleep();
    }
}

/**
 * @brief Finds the unlocked stamp next to a stamp (wrapping around).
 * @param stampId current stamp
 * @param direction >= 1 for the next stamp, otherwise the previous one
 * @return neighbouring stamp, or stampId if it is not unlocked
 */
s32 StampDirector::getNextStampID(s32 stampId, s32 direction) {
    for (s32 i = 0; i < mUnlockInfo.mCount; i++) {
        if (mUnlockInfo.mStampIds[i] != stampId) {
            continue;
        }

        if (direction >= 1) {
            if (i + 1 >= mUnlockInfo.mCount) {
                return mUnlockInfo.mStampIds[0];
            }

            return mUnlockInfo.mStampIds[i + 1];
        }

        if (i == 0) {
            return mUnlockInfo.mStampIds[mUnlockInfo.mCount - 1];
        }

        return mUnlockInfo.mStampIds[i - 1];
    }

    return stampId;
}

/**
 * @brief Shows the held stamp's texture in the stamp layout.
 */
void StampDirector::refreshStampLayoutTexture() {
    al::StringTmp<128> textureName("%s_c", mActiveStamp->getTextureName());
    al::updateTextureInfo(mTextureInfo, mActiveStamp->getTextureData());
    al::setPaneTexture(mLayoutActor, "PicStamp", mTextureInfo);
}

/**
 * @brief Sends an invalid touch message to the collision parts between the camera and a point.
 * @param rTargetPos point the touch ray aims at
 * @return always nullptr
 */
Stamp* StampDirector::getStamp(const sead::Vector3f& rTargetPos) {
    if (mActiveStamp == nullptr) {
        return nullptr;
    }

    sead::Vector3f cameraPos = al::getCameraPosSub(mActiveStamp);
    sead::Vector3f dir;
    al::normalizeOrZero(&dir, rTargetPos - cameraPos);
    s32 hitNum = alCollisionUtil::checkStrikeArrow(mActiveStamp, cameraPos + dir * 0.0f,
                                                   dir * 5000.0f, nullptr, nullptr);

    if (hitNum == 0) {
        return nullptr;
    }

    al::HitSensor* sensor = al::getHitSensor(mActiveStamp, nullptr);

    for (s32 i = 0; i != hitNum; i++) {
        const al::Triangle& triangle =
            alCollisionUtil::getStrikeArrowInfo(mActiveStamp, i)->mTriangle;

        if (triangle.isValid() && al::isFloorCode("IgnoreTouch", triangle)) {
            continue;
        }

        al::HitSensor* triangleSensor = triangle.getSensor();
        triangleSensor->getHost()->getName();
        al::sendMsgScreenPointInvalidCollisionParts(triangleSensor, sensor);
    }

    return nullptr;
}

/**
 * @brief Takes a dead stamp and makes it the held stamp.
 * @return held stamp, or nullptr if every stamp is in use
 */
Stamp* StampDirector::getNewStamp() {
    if (mActiveStamp != nullptr) {
        return mActiveStamp;
    }

    for (Stamp& stamp : *mStamps) {
        if (!al::isDead(&stamp)) {
            continue;
        }

        activateStamp(&stamp, mStartingStampId);

        if (mAudioKeeperUser != nullptr) {
            al::startSe(mAudioKeeperUser, "PgStampAppear", nullptr);
        }

        stamp.setRotation(mRotation);
        mIsHolding = true;

        if (mSnapshotLayout != nullptr) {
            mSnapshotLayout->rotatePreviewImage(stamp.getUIRotation() * -360.0f);
        }

        return &stamp;
    }

    if (mAudioKeeperUser != nullptr) {
        al::startSe(mAudioKeeperUser, "PgStampInvalid", nullptr);
    }

    return nullptr;
}

/**
 * @brief Makes a stamp the held stamp if none is held.
 * @param pStamp stamp to hold
 * @param stampId stamp to show
 */
void StampDirector::activateStamp(Stamp* pStamp, s32 stampId) {
    if (mActiveStamp != nullptr) {
        return;
    }

    mActiveStamp = pStamp;

    if (pStamp != nullptr) {
        pStamp->appear(stampId);
    }

    if (mSnapshotLayout != nullptr) {
        mSnapshotLayout->updatePreviewImage(pStamp);
        mSnapshotLayout->rotatePreviewImage(pStamp->getUIRotation() * -360.0f);
    }

    refreshStampLayoutTexture();
}

/**
 * @brief Drops the held stamp: places it, or removes it if it is off-screen.
 * @param pStamp released stamp
 */
void StampDirector::releaseStamp(Stamp* pStamp) {
    if (mActiveStamp != nullptr) {
        mRotation = mActiveStamp->getRotation();
        mStartingStampId = mActiveStamp->getStampId();
        mIsHolding = false;

        if (mActiveStamp->isHidden()) {
            if (mAudioKeeperUser != nullptr) {
                al::startSe(mAudioKeeperUser, "PgStampVanish", nullptr);
            }

            mActiveStamp->kill();
        } else {
            if (mAudioKeeperUser != nullptr) {
                al::startSe(mAudioKeeperUser, "PgStamp", nullptr);
            }

            mActiveStamp->startPlaced();
        }
    }

    mActiveStamp = nullptr;
}

/**
 * @brief Throws the held stamp away.
 * @param pStamp thrown stamp
 * @param rVelocity throw velocity
 */
void StampDirector::throwStamp(Stamp* pStamp, const sead::Vector3f& rVelocity) {
    if (mActiveStamp != nullptr && mAudioKeeperUser != nullptr) {
        al::startSe(mAudioKeeperUser, "PgStampRemove", nullptr);
    }

    if (mActiveStamp != nullptr) {
        s32 stampId = mActiveStamp->getStampId();
        mIsHolding = false;
        mStartingStampId = stampId;
        mActiveStamp->startErase(rVelocity);
    }

    mActiveStamp = nullptr;
}

/**
 * @brief Kills every stamp.
 */
void StampDirector::hideAllStamps() {
    for (Stamp& stamp : *mStamps) {
        stamp.kill();
    }
}

/**
 * @brief Unlocks the stamp collected in a course.
 * @param courseId course the stamp was collected in
 */
void StampDirector::setCollectStamp(s32 courseId) {
    if (mCourseId == courseId && mCollectStampId >= 0 && mCollectStampIndex >= 0) {
        mUnlockInfo.insertStamp(mCollectStampIndex, mCollectStampId);
    }
}

/**
 * @brief Finds the stamp owning a model.
 * @param pModel model to look for
 * @return stamp owning the model, or nullptr
 */
Stamp* StampDirector::getStampFromModel(alModelCafe* pModel) const {
    for (Stamp& stamp : *mStamps) {
        if (stamp.getModelKeeper()->getModelCafe() == pModel) {
            return &stamp;
        }
    }

    return nullptr;
}

/**
 * @brief Gets the held stamp.
 * @return held stamp, or nullptr
 */
Stamp* StampDirector::getActiveStamp() const {
    return mActiveStamp;
}

/**
 * @brief Draws the 2D stamp layout (nothing to do).
 * @param pRenderBuffer render buffer
 */
void StampDirector::draw2D(const agl::RenderBuffer* pRenderBuffer) {}

}  // namespace rc
