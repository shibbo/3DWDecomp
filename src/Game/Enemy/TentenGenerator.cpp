#include "Enemy/TentenGenerator.hpp"
#include "Bgm/BgmBeatRateTrigger.hpp"
#include "Enemy/ActorRailBrakeMover.hpp"
#include "Enemy/EnemyAttachItem.hpp"
#include "Enemy/Tenten.hpp"
#include "MapObj/Fury/CloudBonusWatcher.hpp"
#include "Util/ItemUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/MapObj/SupportFreezeSyncGroupHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Rail/RailKeeper.hpp"
#include "Library/Rail/RailRider.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Base/StringUtil.hpp"
#include <math/seadMathCalcCommon.h>

namespace {
NERVE_DECL(TentenGenerator, Watch)
NERVE_DECL(TentenGenerator, Move)
NERVE_DECL(TentenGenerator, Stop)
NERVE_DECL(TentenGenerator, SupportFreeze)
NERVE_DECL(TentenGenerator, SyncSupportFreeze)
NERVE_DECL(TentenGenerator, Turn)
NERVE_DECL(TentenGenerator, Wait)
NERVE_DECL(TentenGenerator, WaitHipDrop)
class TentenGeneratorNrvTurnContinue : public al::Nerve {
public:
    /** @brief Resumes a turn interrupted by a support freeze without reversing the front again.
     * @param pKeeper Generator nerve keeper.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<TentenGenerator>()->exeTurn();
    }
};
NERVES_MAKE_NOSTRUCT(TentenGenerator, Watch, Move, Stop, SupportFreeze, SyncSupportFreeze, Turn,
                     Wait, WaitHipDrop, TurnContinue)

/** @brief Animation frames of the "Color" animation, one per Tenten color. */
const f32 sColorFrames[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f};

/** @brief Offset of an attached item above its host Tenten. */
const sead::Vector3f sAttachItemOffset = {0.0f, 125.0f, 0.0f};

/** @brief Offset of an attached green star above its host Tenten. */
const sead::Vector3f sAttachGreenStarOffset = {0.0f, 175.0f, 0.0f};

/** @brief Moves the actor's rail position to the rail point closest to its current position.
 * @param pActor Actor whose rail position is set.
 * @return Whether the chosen rail point is the last one on the rail.
 */
bool setRailPosToNearestRailPoint(al::LiveActor* pActor) {
    if (al::getRailPointNum(pActor) == 1) {
        al::setRailPosToRailPoint(pActor, 0);
        return true;
    }

    sead::Vector3f trans = al::getTrans(pActor);
    sead::Vector3f pointPos = {0.0f, 0.0f, 0.0f};
    f32 minDistance = -1.0f;
    s32 nearestIndex = 0;
    for (s32 i = 0; i < al::getRailPointNum(pActor); i++) {
        al::calcRailPointPos(&pointPos, pActor, i);
        f32 distanceSq = (pointPos - trans).squaredLength();
        if (minDistance < 0.0f || sead::Mathf::sqrt(distanceSq) < minDistance) {
            minDistance = sead::Mathf::sqrt(distanceSq);
            nearestIndex = i;
        }
    }

    al::setRailPosToRailPoint(pActor, nearestIndex);
    return nearestIndex + 1 == al::getRailPointNum(pActor);
}
}  // namespace

/** @brief Constructs a generator and renames it after the kind of Tenten it spawns.
 * @param pName Actor name.
 */
TentenGenerator::TentenGenerator(const char* pName) : al::LiveActor(pName) {
    if (al::isEqualString(pName, "テンテン")) {
        mActorName = "テンテンジェネレータ";
    } else if (al::isEqualString(pName, "パタテンテン")) {
        mActorName = "パタテンテンジェネレータ";
    }
}

/** @brief Spawns the Tenten grid, its attached item and the rail movement helpers.
 * @param rInfo Actor placement and scene information.
 */
void TentenGenerator::init(const al::ActorInitInfo& rInfo) {
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    al::initActorClipping(this, rInfo);
    al::initGroupClipping(this, rInfo, 64);
    al::initStageSwitch(this, rInfo);
    if (mIsSingleMode) {
        al::initExecutorEnemyMapObjMovement(this, rInfo);
    } else {
        al::initExecutorMapObjMovement(this, rInfo);
    }

    initHitSensor(1);
    al::initNerve(this, &NrvTentenGeneratorWatch, 0);
    al::tryGetArg(&mWidthNum, rInfo, "WidthNum");
    al::tryGetArg(&mHeightNum, rInfo, "HeightNum");
    s32 moveSpeed = 6;
    f32 speed = al::tryGetArg(&moveSpeed, rInfo, "MoveSpeed") ? moveSpeed : 6.0f;
    al::tryGetArg(&mIsInvalidTurn, rInfo, "IsInvalidTurn");
    al::tryGetArg(&mOffsetWidth, rInfo, "OffsetWidth");
    al::tryGetArg(&mOffsetHeight, rInfo, "OffsetHeight");
    const char* displayName = nullptr;
    al::getDisplayName(&displayName, rInfo);
    s32 color = 0;
    al::tryGetArg(&color, rInfo, "TentenColor");
    bool isSetSameColorAll = false;
    al::tryGetArg(&isSetSameColorAll, rInfo, "IsSetSameColorAll");
    if (mIsSingleMode) {
        mPlacementIndex = rInfo.mPlacementInfo->_28;
        mInitTrans = al::getTrans(this);
    }

    bool isRailEnd = false;
    if (al::isExistRail(rInfo)) {
        al::setNerve(this, &NrvTentenGeneratorMove);
        initRailKeeper(rInfo);
        isRailEnd = setRailPosToNearestRailPoint(this);
    }

    mTentens = new al::DeriveActorGroup<Tenten>("テンテングループ", mWidthNum * mHeightNum);
    for (s32 y = 0; y < mHeightNum; y++) {
        for (s32 x = 0; x < mWidthNum; x++) {
            Tenten* tenten = new Tenten(displayName, this);
            al::initCreateActorWithPlacementInfo(tenten, rInfo);
            if (mIsSingleMode && al::isMtpAnimExist(tenten, "Color")) {
                al::startMtpAnimAndSetFrameAndStop(
                    tenten, "Color", sColorFrames[isSetSameColorAll ? color : (y + x + color) % 5]);
            } else {
                al::startMclAnimAndSetFrameAndStop(
                    tenten, "Color", sColorFrames[isSetSameColorAll ? color : (y + x + color) % 5]);
            }

            tenten->setOffset(mOffsetWidth * (mWidthNum - x - 1), mOffsetHeight * y);
            mTentens->registerActor(tenten);
        }
    }

    if (al::isExistRail(this)) {
        mRailBrakeMover = new ActorRailBrakeMover(this, speed, nullptr);
        f32 radius = 0.0f;
        f32 halfHeight = (mHeightNum - 1) * 0.5f * 130.0f;
        al::calcRailClippingInfo(&mClippingPos, &radius, this, 100.0f, 100.0f);
        mClippingPos.y += halfHeight;
        radius = sead::Mathf::sqrt(radius * radius + halfHeight * halfHeight);
        al::setClippingInfo(this, radius, &mClippingPos);
        for (s32 i = 0; i < mTentens->getActorCount(); i++) {
            al::setClippingInfo(mTentens->getDeriveActor(i), radius, &mClippingPos);
        }

        if (!isRailEnd || al::isLoopRail(this)) {
            al::moveSyncRail(this, mOffsetWidth * (mWidthNum - 1));
        } else {
            reverseRailAll();
            al::setSyncRailToCoord(this, al::getRailTotalLength(this) -
                                             mOffsetWidth * (mWidthNum - 1));
        }
    }

    const char* itemName = "NoItem";
    if (al::tryGetStringArg(&itemName, rInfo, "ItemType") &&
        !al::isEqualString(itemName, "NoItem")) {
        mAttachItemInfo = new TentenGeneratorAttachItemInfo;
        al::tryGetArg(&mAttachItemInfo->mWidthIndex, rInfo, "AttachItemWidthNum");
        mAttachItemInfo->mWidthIndex =
            sead::Mathi::clamp(mAttachItemInfo->mWidthIndex - 1, 0, mWidthNum);
        if (mAttachItemInfo->mWidthIndex >= mWidthNum) {
            mAttachItemInfo->mWidthIndex = mWidthNum - 1;
        }

        s32 itemType = rc::getItemType(rInfo);
        mAttachItemInfo->mIsAttachToRail = false;
        al::tryGetArg(&mAttachItemInfo->mIsAttachToRail, rInfo, "IsAttachItemToRail");
        if (mAttachItemInfo->mIsAttachToRail) {
            mAttachItemInfo->mHostTenten = nullptr;
            mAttachItemInfo->mItem = new EnemyAttachItem(
                this, "テンテンにくっつくアイテム", itemName, itemType,
                &mAttachItemInfo->mRailPos, nullptr, sead::Vector3f::zero, false);
            al::initCreateActorWithPlacementInfo(mAttachItemInfo->mItem, rInfo);
            f32 railOffset = 0.0f;
            al::tryGetArg(&railOffset, rInfo, "AttachItemRailOffset");
            if (al::getRailTotalLength(mAttachItemInfo->mItem) < railOffset &&
                !al::isLoopRail(mAttachItemInfo->mItem)) {
                railOffset = al::getRailTotalLength(mAttachItemInfo->mItem) * 2 - railOffset;
            }

            al::setSyncRailToCoord(mAttachItemInfo->mItem, railOffset);
        } else {
            mAttachItemInfo->mHostTenten = tryFindTopTenten(mAttachItemInfo->mWidthIndex);
            mAttachItemInfo->mItem = new EnemyAttachItem(
                this, "テンテンにくっつくアイテム", itemName, itemType,
                al::getTransPtr(mAttachItemInfo->mHostTenten), nullptr,
                itemType == rc::ItemType_GreenStar ? sAttachGreenStarOffset : sAttachItemOffset,
                false);
            al::initCreateActorWithPlacementInfo(mAttachItemInfo->mItem, rInfo);
        }

        mAttachItemInfo->mItem->makeActorAppeared();
    }

    al::addHitSensorEye(this, rInfo, "Eye", 10.0f, 8, sead::Vector3f(0.0f, 0.0f, 0.0f));
    al::registSupportFreezeSyncGroup(this, rInfo);
    if (al::isExistRail(this) &&
        al::listenStageSwitchOnStart(this, al::FunctorV0M(this, &TentenGenerator::startMove))) {
        al::setNerve(this, &NrvTentenGeneratorStop);
    }

    al::listenStageSwitchOnKill(this, al::FunctorV0M(this, &TentenGenerator::killBySwitch));
    al::initActorAudioKeeper(this, rInfo, "Tenten", nullptr);
    mBgmBeatRateTrigger = new BgmBeatRateTrigger(this, 0.4f);
    mBgmName = new sead::FixedSafeString<64>();
    makeActorAppeared();
}

/** @brief Reverses the generator's rail and the rails of all Tentens still standing. */
void TentenGenerator::reverseRailAll() {
    al::reverseRail(this);
    al::moveSyncRail(this, mOffsetWidth * (mWidthNum - 1));
    for (s32 i = 0; i < mTentens->getActorCount(); i++) {
        if (!mTentens->getDeriveActor(i)->isDown()) {
            al::reverseRail(mTentens->getDeriveActor(i));
        }
    }
}

/** @brief Finds the highest Tenten of a column that has not been knocked down.
 * @param widthIndex Column of the formation.
 * @return The top standing Tenten, or nullptr if the column is empty or out of range.
 */
Tenten* TentenGenerator::tryFindTopTenten(s32 widthIndex) const {
    if (widthIndex >= mWidthNum) {
        return nullptr;
    }

    for (s32 y = mHeightNum - 1; y >= 0; y--) {
        Tenten* tenten = mTentens->getDeriveActor(mWidthNum * y + widthIndex);
        if (!tenten->isDown()) {
            return tenten;
        }
    }

    return nullptr;
}

/** @brief Starts moving along the rail once the start switch turns on. */
void TentenGenerator::startMove() {
    al::setNerve(this, &NrvTentenGeneratorMove);
}

/** @brief Kills the generator together with all of its Tentens. */
void TentenGenerator::killBySwitch() {
    if (al::isDead(this)) {
        return;
    }

    mTentens->killAll();
    kill();
}

/** @brief Registers the generator with the cloud bonus watcher in single mode. */
void TentenGenerator::initAfterPlacement() {
    if (mIsSingleMode) {
        auto* watcher = al::getSceneObj<CloudBonusWatcher>(this, 52);
        if (watcher != nullptr) {
            watcher->tryRegisterActor(this, mPlacementIndex);
        }
    }
}

/** @brief Resets the formation to its initial placement before appearing again. */
void TentenGenerator::reappear() {
    if (mIsSingleMode) {
        al::setTrans(this, mInitTrans);
        if (al::isExistRail(this)) {
            al::setNerve(this, &NrvTentenGeneratorMove);
            getRailKeeper()->getRailRider()->setMoveGoingEnd();
            mRailBrakeMover->resetSpeed();
            bool isRailEnd = setRailPosToNearestRailPoint(this);
            for (s32 y = 0; y < mHeightNum; y++) {
                for (s32 x = 0; x < mWidthNum; x++) {
                    Tenten* tenten = mTentens->getDeriveActor(y * mWidthNum + x);
                    tenten->reset();
                    tenten->setOffset(mOffsetWidth * (mWidthNum - x - 1), mOffsetHeight * y);
                }
            }

            if (!isRailEnd || al::isLoopRail(this)) {
                al::moveSyncRail(this, mOffsetWidth * (mWidthNum - 1));
            } else {
                reverseRailAll();
                al::setSyncRailToCoord(this, al::getRailTotalLength(this) -
                                                 mOffsetWidth * (mWidthNum - 1));
            }
        } else {
            for (s32 i = 0; i < mTentens->getActorCount(); i++) {
                mTentens->getDeriveActor(i)->reset();
            }

            al::setNerve(this, &NrvTentenGeneratorWatch);
        }
    }

    appear();
}

/** @brief Kills the generator once every Tenten is gone. */
void TentenGenerator::control() {
    if (mTentens->calcAliveActorNum() == 0) {
        kill();
    }
}

/** @brief Kills the generator together with all of its Tentens.
 * @param isForce Unused.
 */
void TentenGenerator::killComplete(bool isForce) {
    if (al::isDead(this)) {
        return;
    }

    mTentens->killAll();
    kill();
}

/** @brief Handles support-freeze queries and synchronized freezes from the sync group.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the generator.
 * @return Whether the message was handled.
 */
bool TentenGenerator::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                 al::HitSensor* pSelf) {
    if (al::isMsgIsNerveSupportFreeze(pMsg)) {
        return al::isNerve(this, &NrvTentenGeneratorSupportFreeze);
    }

    if (al::isMsgOnSyncSupportFreeze(pMsg)) {
        if (al::isNerve(this, &NrvTentenGeneratorSupportFreeze) ||
            al::isNerve(this, &NrvTentenGeneratorSyncSupportFreeze)) {
            return false;
        }

        return tryStartSupportFreeze(&NrvTentenGeneratorSyncSupportFreeze);
    }

    if (al::isMsgOffSyncSupportFreeze(pMsg)) {
        if (!al::isNerve(this, &NrvTentenGeneratorSyncSupportFreeze)) {
            return false;
        }

        endSupportFreeze();
        return true;
    }

    return false;
}

/** @brief Freezes all Tentens and remembers the nerve to resume afterwards.
 * @param pNerve Freeze nerve to change to.
 * @return Whether the freeze started.
 */
bool TentenGenerator::tryStartSupportFreeze(const al::Nerve* pNerve) {
    const al::Nerve* currentNerve = getNerveKeeper()->getCurrentNerve();
    if (currentNerve == pNerve) {
        return false;
    }

    if (currentNerve != &NrvTentenGeneratorSupportFreeze &&
        currentNerve != &NrvTentenGeneratorSyncSupportFreeze) {
        mNerveBeforeSupportFreeze = currentNerve;
    }

    for (s32 i = 0; i < mTentens->getActorCount(); i++) {
        mTentens->getDeriveActor(i)->startSupportFreeze(mSupportFreezeTouchActor);
    }

    al::setNerve(this, pNerve);
    return true;
}

/** @brief Releases all Tentens and resumes the nerve that was interrupted by the freeze. */
void TentenGenerator::endSupportFreeze() {
    for (s32 i = 0; i < mTentens->getActorCount(); i++) {
        mTentens->getDeriveActor(i)->endSupportFreeze();
    }

    const al::Nerve* nerve = mNerveBeforeSupportFreeze;
    mNerveBeforeSupportFreeze = nullptr;
    if (nerve == &NrvTentenGeneratorTurn) {
        al::setNerve(this, &NrvTentenGeneratorTurnContinue);
    } else {
        al::setNerve(this, nerve);
    }
}

/** @brief Starts or extends a support freeze triggered by the assist player touching a Tenten.
 * @param pTouchActor Actor that touched the Tenten.
 */
void TentenGenerator::receiveMsgTouchAssist(const al::LiveActor* pTouchActor) {
    mSupportFreezeTouchActor = pTouchActor;
    mSupportFreezeStep = 0;
    if (!al::isNerve(this, &NrvTentenGeneratorSupportFreeze)) {
        tryStartSupportFreeze(&NrvTentenGeneratorSupportFreeze);
    }
}

/** @brief Clips the generator together with its attached item. */
void TentenGenerator::startClipped() {
    al::LiveActor::startClipped();
    if (mAttachItemInfo == nullptr) {
        return;
    }

    EnemyAttachItem* item = mAttachItemInfo->mItem;
    if (al::isAlive(item) && !al::isClipped(item)) {
        item->startClipped();
    }
}

/** @brief Unclips the generator together with its attached item. */
void TentenGenerator::endClipped() {
    al::LiveActor::endClipped();
    if (mAttachItemInfo == nullptr) {
        return;
    }

    EnemyAttachItem* item = mAttachItemInfo->mItem;
    if (al::isAlive(item) && al::isClipped(item)) {
        item->endClipped();
    }
}

/** @brief Hands the attached item to the next Tenten of its column when its host goes down.
 * @param pTenten Tenten that went down.
 */
void TentenGenerator::noticeDeadChild(const Tenten* pTenten) {
    if (mAttachItemInfo == nullptr || mAttachItemInfo->mHostTenten == nullptr ||
        mAttachItemInfo->mHostTenten != pTenten) {
        return;
    }

    mAttachItemInfo->mHostTenten = tryFindTopTenten(mAttachItemInfo->mWidthIndex);
    Tenten* host = mAttachItemInfo->mHostTenten;
    EnemyAttachItem* item = mAttachItemInfo->mItem;
    if (host != nullptr) {
        item->setFollowTransPtr(al::getTransPtr(host));
    } else {
        item->endAttach(true);
    }
}

/** @brief Waits without moving. */
void TentenGenerator::exeWatch() {}

/** @brief Waits for the start switch. */
void TentenGenerator::exeStop() {}

/** @brief Moves the formation along the rail and turns around at the rail ends. */
void TentenGenerator::exeMove() {
    if (al::isFirstStep(this) && !mIsInvalidTurn) {
        for (s32 i = 0; i < mTentens->getActorCount(); i++) {
            al::turnToRailDirImmediately(mTentens->getDeriveActor(i));
        }
    }

    if (tryChangeWaitHipDrop()) {
        return;
    }

    f32 prevCoord = al::getRailCoord(this);
    bool isRailEnd = mRailBrakeMover->moveSyncRailBrake();
    f32 diff = al::getRailCoord(this) - prevCoord;
    f32 speed = diff > 0.0f ? diff : -diff;
    if (al::isLoopRail(this)) {
        speed = mRailBrakeMover->mMaxSpeed;
    }

    for (s32 i = 0; i < mTentens->getActorCount(); i++) {
        if (!mTentens->getDeriveActor(i)->isDown()) {
            mTentens->getDeriveActor(i)->moveSyncRailAddOffset(speed);
        }
    }

    if (mAttachItemInfo != nullptr && mAttachItemInfo->mItem != nullptr &&
        mAttachItemInfo->mIsAttachToRail) {
        al::moveSyncRail(mAttachItemInfo->mItem, speed);
        mAttachItemInfo->mRailPos = al::getRailPos(mAttachItemInfo->mItem);
    }

    updateBgmBeatRateTrigger();
    if (isRailEnd) {
        reverseRailAll();
        if (!mIsInvalidTurn) {
            al::setNerve(this, &NrvTentenGeneratorTurn);
        }
    }
}

/** @brief Stops the formation while a single-column Tenten is hip-dropped.
 * @return Whether the generator changed to the hip-drop wait.
 */
bool TentenGenerator::tryChangeWaitHipDrop() {
    if (mWidthNum > 1) {
        return false;
    }

    for (s32 i = 0; i < mTentens->getActorCount(); i++) {
        if (mTentens->getDeriveActor(i)->isHipDropDown()) {
            al::setNerve(this, &NrvTentenGeneratorWaitHipDrop);
            return true;
        }
    }

    return false;
}

/** @brief Makes the standing Tentens cry out to the beat of the BGM. */
void TentenGenerator::updateBgmBeatRateTrigger() {
    mBgmBeatRateTrigger->update();
    if (!mBgmBeatRateTrigger->isTrigger()) {
        return;
    }

    if (isBgmSingleModeIsland04()) {
        mBgmBeatRateTrigger->setRate(0.0f);
        mBeatCount++;
        if (mBeatCount > 0) {
            mBeatCount %= 3;
        }

        s32 voiceIndex;
        switch (mBeatCount) {
        case 0:
            if (mVoiceIndex != 0) {
                return;
            }

            voiceIndex = 1;
            break;
        case 2:
            if (mVoiceIndex != 1) {
                return;
            }

            voiceIndex = 0;
            break;
        default:
            return;
        }

        mBeatCount -= 3;
        mVoiceIndex = voiceIndex;
        for (s32 i = 0; i < mTentens->getActorCount(); i++) {
            if (!mTentens->getDeriveActor(i)->isDown()) {
                al::startSe(mTentens->getDeriveActor(i),
                            mVoiceIndex == 0 ? "TenVoice1" : "TenVoice2");
            }
        }

        return;
    }

    mBgmBeatRateTrigger->setRate(0.4f);
    if (mVoiceIndex == 0) {
        mVoiceIndex = 1;
        for (s32 i = 0; i < mTentens->getActorCount(); i++) {
            if (!mTentens->getDeriveActor(i)->isDown()) {
                al::startSe(mTentens->getDeriveActor(i), "TenVoice1");
            }
        }
    } else {
        mVoiceIndex = 0;
        for (s32 i = 0; i < mTentens->getActorCount(); i++) {
            if (!mTentens->getDeriveActor(i)->isDown()) {
                al::startSe(mTentens->getDeriveActor(i), "TenVoice2");
            }
        }
    }
}

/** @brief Turns the standing Tentens around at a rail end. */
void TentenGenerator::exeTurn() {
    if (al::isFirstStep(this) && al::isNerve(this, &NrvTentenGeneratorTurn)) {
        for (s32 i = 0; i < mTentens->getActorCount(); i++) {
            mTentens->getDeriveActor(i)->setReverseFront();
        }
    }

    bool isTurnEnd = true;
    for (s32 i = 0; i < mTentens->getActorCount(); i++) {
        Tenten* tenten = mTentens->getDeriveActor(i);
        if (!tenten->isDown()) {
            isTurnEnd &= al::turnDirectionDegree(tenten, al::getFrontPtr(tenten),
                                                 tenten->getTargetFront(), getTurnSpeed());
        }
    }

    updateBgmBeatRateTrigger();
    if (isTurnEnd) {
        al::setNerve(this, &NrvTentenGeneratorWait);
    }
}

/** @brief Waits briefly after turning before moving again. */
void TentenGenerator::exeWait() {
    al::setNerveAtStep(this, &NrvTentenGeneratorMove, 15);
    updateBgmBeatRateTrigger();
}

/** @brief Waits for a hip-dropped Tenten to recover before moving again. */
void TentenGenerator::exeWaitHipDrop() {
    al::setNerveAtStep(this, &NrvTentenGeneratorMove, 45);
}

/** @brief Keeps the formation frozen for a short while after the last assist touch. */
void TentenGenerator::exeSupportFreeze() {
    if (al::isFirstStep(this)) {
        mSupportFreezeStep = 1;
        return;
    }

    if (mSupportFreezeStep++ >= 15) {
        endSupportFreeze();
    }
}

/** @brief Stays frozen until the sync group releases the freeze. */
void TentenGenerator::exeSyncSupportFreeze() {}

/** @brief Gets the turning speed of the Tentens.
 * @return Turning speed in degrees per frame.
 */
f32 TentenGenerator::getTurnSpeed() const {
    return 3.0f;
}

/** @brief Checks whether the Island04 BGM of single mode is playing.
 * @return Whether the generator is in single mode and the Island04 BGM is playing.
 */
bool TentenGenerator::isBgmSingleModeIsland04() {
    if (!mIsSingleMode) {
        return false;
    }

    mBgmName->format(al::getCurPlayingBgmPlayName(this));
    return mBgmName->isEqual("Island04");
}

/** @brief Destroys the generator. */
TentenGenerator::~TentenGenerator() = default;
