#include "MapObj/KoopaGraffiti.hpp"

#include <attributes.h>
#include <math/seadVector.h>

#include "Layout/GuideBalloon.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Screen/ScreenPointer.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "MapObj/AssistLeaf.hpp"
#include "MapObj/BoomerangFlower.hpp"
#include "MapObj/FireFlower.hpp"
#include "MapObj/GoalItem.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "MapObj/KinokoOneUp.hpp"
#include "MapObj/KinokoSuper.hpp"
#include "MapObj/KinokoTreasure.hpp"
#include "MapObj/SuperBell.hpp"
#include "MapObj/SuperBellSpecial.hpp"
#include "MapObj/SuperLeaf.hpp"
#include "MapObj/SuperStar.hpp"
#include "MapObj/WhiteBell.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_ACTION_IMPL(KoopaGraffiti, Wait)
NERVE_ACTION_IMPL(KoopaGraffiti, Broadcast)
NERVE_ACTION_IMPL(KoopaGraffiti, Painting)
NERVE_ACTION_IMPL(KoopaGraffiti, Finish)
NERVE_ACTIONS_MAKE_STRUCT(KoopaGraffiti, Wait, Broadcast, Painting, Finish)

/**
 * @brief Get the material animation showing the graffiti of a type.
 * @param type The graffiti type (1-9 have their own animation).
 * @return The animation name.
 */
const char* getGraffitiAnimName(s32 type) {
    switch (type) {
    case 1:
        return "GraffitiA";
    case 2:
        return "GraffitiB";
    case 3:
        return "GraffitiC";
    case 4:
        return "GraffitiD";
    case 5:
        return "GraffitiE";
    case 6:
        return "GraffitiF";
    case 7:
        return "GraffitiG";
    case 8:
        return "GraffitiH";
    case 9:
        return "GraffitiI";
    default:
        return "Graffiti";
    }
}

/**
 * @brief Get the monochrome material animation of a graffiti type.
 * @param type The graffiti type.
 * @return The animation name.
 */
const char* getGraffitiMonoAnimName(s32 type) {
    switch (type) {
    case 0:
        return "GraffitiMono";
    case 1:
        return "GraffitiAMono";
    case 2:
        return "GraffitiBMono";
    case 3:
        return "GraffitiCMono";
    case 4:
        return "GraffitiDMono";
    case 5:
        return "GraffitiEMono";
    case 6:
        return "GraffitiFMono";
    case 7:
        return "GraffitiGMono";
    case 8:
        return "GraffitiHMono";
    case 9:
        return "GraffitiIMono";
    default:
        return "Graffiti";
    }
}

/**
 * @brief Get the hit reaction played for a paint stroke.
 * @param count Number of paint strokes so far.
 * @return The hit reaction name.
 */
const char* getPaintReactionName(s32 count) {
    switch (count) {
    case 0:
    case 1:
        return "PaintA";
    case 2:
        return "PaintB";
    case 3:
        return "PaintC";
    default:
        return "Paint";
    }
}

/**
 * @brief Make an item pop up out of the graffiti and play its appear sound.
 * @param pItem The item to pop up.
 * @param pParam The pop-up parameters.
 */
template <typename T>
ALWAYS_INLINE void appearPopUpItem(T* pItem, const ItemStatePopUpFrontParam* pParam) {
    pItem->appearPopUpFront();
    pItem->setPopUpFrontParam(pParam);
    al::tryStartSe(pItem, "PgAppear");
}
}  // namespace

/**
 * @brief Construct a graffiti.
 * @param pName The actor name.
 */
KoopaGraffiti::KoopaGraffiti(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initialize the graffiti from its placement.
 * @param rInfo The actor init info.
 */
void KoopaGraffiti::init(const al::ActorInitInfo& rInfo) {
    bool isSpecial = false;
    al::tryGetArg(&isSpecial, rInfo, "IsSpecialGraffiti");
    al::initNerveAction(this, "Wait", &NrvKoopaGraffiti.collector, 0);
    if (isSpecial) {
        al::initActorWithArchiveName(this, rInfo, "KoopaGraffitiSpecial", nullptr);
    } else {
        al::initActorWithArchiveName(this, rInfo, "KoopaGraffiti", nullptr);
    }

    al::tryGetArg(&mType, rInfo, "Type");
    makeActorAppeared();
    al::tryGetZoneID(&mZoneId, al::getPlacementInfo(rInfo));
    al::tryGetArg(&mGraffitiId, rInfo, "GraffitiID");

    const char* pItemType = "Dummy";
    if (al::tryGetStringArg(&pItemType, rInfo, "ItemType")) {
        createItem(rInfo);
    }

    if (isSpecial) {
        mType = 0;
    }

    if (al::calcLinkChildNum(rInfo, "GoalItem") >= 1) {
        mGoalItem = new GoalItem("GraffitiGoalItem");
        al::initLinksActor(mGoalItem, rInfo, "GoalItem", 0);
        mGoalItem->makeActorDead();

        if (!mGoalItem->isCollected() && mZoneId >= 1 &&
            SingleModeDataFunction::isVandalizedIsland(GameDataHolderAccessor(this), mZoneId - 1,
                                                       mGraffitiId, &mIsPainted)) {
            mIsPainted = false;
            SingleModeDataFunction::clearVandalizeIsland(GameDataHolderWriter(this), mZoneId - 1,
                                                         mGraffitiId);
        }
    }

    if (mZoneId >= 1) {
        if (SingleModeDataFunction::isVandalizedIsland(GameDataHolderAccessor(this), mZoneId - 1,
                                                       mGraffitiId, &mIsPainted)) {
            setVandalizedAnim(mIsPainted);
        } else {
            al::startMtpAnim(this, "Graffiti");
            al::setMtpAnimFrameAndStop(this, 0.0f);
        }
    }

    al::tryGetArg(&mItemVelY, rInfo, "ItemVelY");
    al::tryGetArg(&mItemVelZ, rInfo, "ItemVelZ");

    sead::Vector3f upDir;
    al::calcUpDir(&upDir, this);
    sead::Vector3f sideDir;
    al::calcSideDir(&sideDir, this);
    sead::Vector3f checkUpDir = sead::Vector3f::ey;
    al::calcUpDir(&checkUpDir, this);
    bool isUpsideDown = al::isNear(checkUpDir.y, -1.0f, 0.001f);

    if (isUpsideDown) {
        mGuideBalloon = new GuideBalloon("ガイドバルーン", al::getLayoutInitInfo(rInfo),
                                         &al::getSensorPos(al::getHitSensor(this, "Broadcast")),
                                         sead::Vector3f::ey * 90.0f + sideDir * 0.0f,
                                         getSceneInfo()->isSingleMode, nullptr);
        al::startAction(mGuideBalloon, "SetBalloonUpsideDown", "Beaks");
    } else {
        mGuideBalloon = new GuideBalloon("ガイドバルーン", al::getLayoutInitInfo(rInfo),
                                         al::getTransPtr(this), upDir * 25.0f + sideDir * 0.0f,
                                         getSceneInfo()->isSingleMode, nullptr);
    }

    if (al::calcLinkChildNum(rInfo, "CameraArea") != 0) {
        al::PlacementInfo placementInfo;
        al::getLinksInfoByIndex(&placementInfo, rInfo.getPlacementInfo(), "CameraArea", 0);
        al::AreaInitInfo areaInitInfo(placementInfo, rInfo.getStageSwitchDirector());
        mCameraArea = new al::AreaObj("CameraArea");
        mCameraArea->init(areaInitInfo);
        mCameraArea->invalidate();

        al::AreaObjGroup* pGroup = rc::tryFindAreaObjGroup(this, rc::AreaObjType::CameraArea);
        if (pGroup != nullptr) {
            pGroup->resisterAreaObj(mCameraArea);
        }
    }

    f32 displayRadius = 1500.0f;
    if (al::tryGetArg(&displayRadius, rInfo, "DisplayRadius")) {
        al::setSensorRadius(this, "Display", displayRadius);
    }

    mIsBroadcasting = false;
}

/**
 * @brief Create the item given by the "ItemType" placement argument.
 * @param rInfo The actor init info.
 */
void KoopaGraffiti::createItem(const al::ActorInitInfo& rInfo) {
    mItemType = static_cast<rc::ItemType>(rc::getItemType(rInfo));

    s32 type = 0;
    switch (mItemType) {
    case rc::ItemType_KinokoOneUp:
        mItem = new KinokoOneUp("1UPキノコ");
        type = 0;
        break;
    case rc::ItemType_KinokoSuper:
        mItem = new KinokoSuper("スーパーキノコ");
        type = 2;
        break;
    case rc::ItemType_SuperBell:
        mItem = new SuperBell("スーパーベル");
        type = 1;
        break;
    case rc::ItemType_FireFlower:
        mItem = new FireFlower("ファイアフラワー");
        type = 3;
        break;
    case rc::ItemType_SuperLeaf:
        mItem = new SuperLeaf("スーパーこのは");
        type = 5;
        break;
    case rc::ItemType_BoomerangFlower:
        mItem = new BoomerangFlower("ブーメランフラワー");
        type = 4;
        break;
    case rc::ItemType_SuperStar:
        mItem = new SuperStar("スーパースター");
        type = 0;
        break;
    case rc::ItemType_AssistLeaf:
        mItem = new AssistLeaf("無敵このは");
        type = 0;
        break;
    case rc::ItemType_SuperBellSpecial:
        mItem = new SuperBellSpecial("まねきネコベル");
        type = 6;
        break;
    case rc::ItemType_KinokoTreasure:
        mItem = new KinokoTreasure("KinokoTreasure");
        type = 0;
        break;
    case rc::ItemType_WhiteBell:
        mItem = new WhiteBell("WhiteBell");
        type = 7;
        break;
    default:
        break;
    }

    if (mType == 0) {
        mType = type;
    }

    if (mItem != nullptr) {
        al::initCreateActorNoPlacementInfo(mItem, rInfo);
        mItem->kill();
        return;
    }

    mItemType = static_cast<rc::ItemType>(rc::getItemType(rInfo));
    initItemKeeper(1);
    rc::addItemByHostInfo(this, rInfo, "通常アイテム", nullptr);
}

/**
 * @brief Show the graffiti as already handled in this island phase.
 * @param isPainted True if Bowser Jr. finished painting it, false if it is only shown monochrome.
 */
void KoopaGraffiti::setVandalizedAnim(bool isPainted) {
    if (isPainted) {
        al::startMtpAnim(this, getGraffitiAnimName(mType));
        al::setMtpAnimFrameAndStop(this, 3.0f);
        al::tryOnStageSwitch(this, "SwitchInstantOn");
        mIsSwitchOnInstant = true;
        al::startNerveAction(this, "Finish");
        mIsAppearItem = false;
    } else {
        al::startMtpAnim(this, getGraffitiMonoAnimName(mType));
        al::setMtpAnimFrameAndStop(this, 3.0f);
        al::startNerveAction(this, "Wait");
        mIsMono = true;
    }
}

/**
 * @brief Let Bowser Jr. know he touches the graffiti.
 * @param pSelf The graffiti's sensor.
 * @param pOther The touching sensor.
 */
void KoopaGraffiti::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorMapObj(pSelf)) {
        return;
    }

    if (al::isNerve(this, NrvKoopaGraffiti.Broadcast.data())) {
        rc::sendMsgGraffiti(pOther, pSelf, 0);
    }

    if (al::isSensorKoopaJr(pOther)) {
        static_cast<PlayerKoopaJr*>(al::getSensorHost(pOther))->setTouchGraffitiSensor(pSelf);
        mIsKoopaJrTouch = true;
    }
}

/**
 * @brief Handle the player touching the graffiti and Bowser Jr. painting it.
 * @param pMsg The message.
 * @param pSelf The graffiti's sensor.
 * @param pOther The sending sensor.
 * @return True if the message was handled.
 */
bool KoopaGraffiti::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                               al::HitSensor* pOther) {
    if (al::isNerve(this, NrvKoopaGraffiti.Finish.data())) {
        return false;
    }

    if (al::isMsgPlayerDisregard(pMsg) && al::isNerve(this, NrvKoopaGraffiti.Wait.data())) {
        mIsPlayerTouch = true;
        return false;
    }

    if (al::isSensorMapObj(pOther)) {
        return false;
    }

    if (rc::isMsgGraffiti(pMsg)) {
        if (al::isNerve(this, NrvKoopaGraffiti.Broadcast.data()) ||
            al::isNerve(this, NrvKoopaGraffiti.Wait.data())) {
            mPaintCount = 0;
            al::startNerveAction(this, "Painting");
            if (!mIsMono) {
                al::startMtpAnim(this, getGraffitiAnimName(mType));
                al::setMtpAnimFrameAndStop(this, 0.0f);
            }

            return true;
        }

        if (al::isNerve(this, NrvKoopaGraffiti.Painting.data())) {
            if (mPaintCount == 0) {
                al::tryStartVisAnimIfExist(this, "Finish");
            }

            mPaintCount++;
            al::tryStartSklAnimIfExist(this, "Painting");
            al::tryStartMtpAnimIfNotPlaying(this, getGraffitiAnimName(mType));
            al::setMtpAnimFrameAndStop(this, sead::Mathi::clamp(mPaintCount, 0, 3));
            if (mPaintCount <= 3) {
                al::startHitReaction(this, getPaintReactionName(mPaintCount));
            }

            return true;
        }
    }

    return false;
}

/**
 * @brief Let Bowser Jr. target the graffiti when the player points at it.
 * @param pMsg The message.
 * @param pPointer The screen pointer.
 * @param pTarget The graffiti's screen point target.
 * @return True if Bowser Jr. started targetting the graffiti.
 */
bool KoopaGraffiti::receiveMsgScreenPointSM(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                            al::ScreenPointTarget* pTarget) {
    if (!al::isNerve(this, NrvKoopaGraffiti.Wait.data())) {
        return false;
    }

    sead::Vector3f upDir;
    al::calcUpDir(&upDir, this);
    if (upDir.dot(pPointer->getHitNormal()) < 0.0f) {
        return false;
    }

    sead::Vector3f cameraPos = getCameraDirector_RS()->getLookAtMain().getPos();
    sead::Vector3f arrow = (pPointer->getHitPos() - cameraPos) * 100.0f;
    sead::Vector3f hitPos;
    if (!alCollisionUtil::getFirstPolyOnArrow(this, &hitPos, nullptr, cameraPos, arrow, nullptr,
                                              nullptr)) {
        return false;
    }

    sead::Vector3f diff = hitPos - pTarget->getPos();
    if (!(pTarget->getRadius() > diff.length())) {
        return false;
    }

    if (!(sead::Mathf::abs(diff.dot(upDir)) < 15.0f)) {
        return false;
    }

    sead::Vector3f toSensor = al::getSensorPos(al::getHitSensor(this, "Broadcast")) - cameraPos;
    if (!(toSensor.squaredLength() < 100000000.0f)) {
        return false;
    }

    if (alCollisionUtil::checkStrikeArrow(this, cameraPos, toSensor, nullptr, nullptr) != 0) {
        return false;
    }

    PlayerKoopaJr* pKoopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
    if (pKoopaJr == nullptr) {
        return false;
    }

    if (!pKoopaJr->tryTargetGraffiti(al::getHitSensor(this, "Broadcast"))) {
        return false;
    }

    al::startNerveAction(this, "Broadcast");
    return true;
}

/**
 * @brief Appear the graffiti.
 */
void KoopaGraffiti::appear() {
    al::LiveActor::appear();
}

/**
 * @brief Kill the graffiti.
 */
void KoopaGraffiti::kill() {
    al::LiveActor::kill();
}

/**
 * @brief Hide the guide balloon when the graffiti gets clipped.
 */
void KoopaGraffiti::startClipped() {
    al::LiveActor::startClipped();
    if (mGuideBalloon != nullptr) {
        mGuideBalloon->endShow();
    }
}

/**
 * @brief Reset the graffiti to its saved state unless it is already finished.
 */
void KoopaGraffiti::reset() {
    if (al::isNerve(this, NrvKoopaGraffiti.Finish.data())) {
        return;
    }

    if (mZoneId >= 1 &&
        SingleModeDataFunction::isVandalizedIsland(GameDataHolderAccessor(this), mZoneId - 1,
                                                   mGraffitiId, &mIsPainted)) {
        setVandalizedAnim(mIsPainted);
        if (mIsMono) {
            mIsBroadcasting = false;
        }
    } else {
        al::startMtpAnim(this, getGraffitiAnimName(mType));
        al::setMtpAnimFrameAndStop(this, 0.0f);
        mIsBroadcasting = false;
    }

    al::startNerveAction(this, "Wait");
    al::validateHitSensor(this, "Broadcast");
    al::validateHitSensor(this, "Display");
}

/**
 * @brief Wait for Bowser Jr. and show the matching guide while someone is near.
 */
void KoopaGraffiti::exeWait() {
    if (al::isFirstStep(this)) {
        if (mIsMono) {
            al::tryStartSklAnimIfExist(this, "Finish");
            al::tryStartVisAnimIfExist(this, "Finish");
        } else {
            al::tryStartSklAnimIfExist(this, "Wait");
            al::tryStartVisAnimIfExist(this, "Wait");
        }
    }

    if (mGuideBalloon == nullptr || mIsBroadcasting) {
        return;
    }

    PlayerKoopaJr* pKoopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
    bool isNearPlayer = false;
    bool isShowGuide = false;
    if (mIsPlayerTouch || mIsKoopaJrTouch) {
        const sead::LookAtCamera& rCamera = getCameraDirector_RS()->getLookAtMain();
        al::LiveActor* pPlayer = al::tryFindNearestPlayerActor(this);
        if (pPlayer != nullptr) {
            sead::Vector3f sensorPos = al::getSensorPos(al::getHitSensor(this, "Broadcast"));
            sead::Vector3f cameraPos = rCamera.getPos();
            sead::Vector3f toSensor = sensorPos - cameraPos;
            al::CollisionPartsFilterActor filter(this);
            if (alCollisionUtil::getStrikeArrowCollisionParts(this, nullptr, cameraPos, toSensor,
                                                              &filter, nullptr) == nullptr) {
                sead::Vector3f cameraDir = rCamera.getAt() - rCamera.getPos();
                cameraDir.normalize();
                toSensor.normalize();
                if (cameraDir.dot(toSensor) > 0.76604444f) {
                    if (pKoopaJr != nullptr && pKoopaJr->isAIMovement()) {
                        isShowGuide = mIsKoopaJrTouch;
                    } else if (mIsPlayerTouch) {
                        s32 distance = (sensorPos - al::getTrans(pPlayer)).squaredLength();
                        isNearPlayer = distance < 300000;
                        isShowGuide = distance < 850000;
                    }
                }
            }
        }
    }

    if (isShowGuide) {
        if (pKoopaJr != nullptr && pKoopaJr->isAIMovement()) {
            if (!mIsShowKoopaJrGuide) {
                mGuideBalloon->endShow();
                mIsShowKoopaJrGuide = true;
            }

            mGuideBalloon->startShow(u"");
        } else {
            if (mIsShowKoopaJrGuide) {
                mGuideBalloon->endShow();
                mIsShowKoopaJrGuide = false;
            }

            mGuideBalloon->startShowDrcTouch(true);
        }
    } else {
        mGuideBalloon->endShow();
    }

    updateGuideMessage(isNearPlayer);
    mIsPlayerTouch = false;
    mIsKoopaJrTouch = false;
}

/**
 * @brief Show or hide the gyro controls guide message.
 * @param isShow True to show the message.
 */
void KoopaGraffiti::updateGuideMessage(bool isShow) {
    if (isShow) {
        if (!rc::isGuideGameWindowActive(this)) {
            mIsShowGuideWindow = false;
        } else if (mIsShowGuideWindow) {
            return;
        }

        if (al::isNerve(this, NrvKoopaGraffiti.Painting.data())) {
            return;
        }

        GameDataHolderAccessor accessor(this);
        const char* pMessage = al::isPadTypeHandheld(al::getMainControllerPort()) ?
                                   "GyroControlsGuide_Handheld" :
                                   "GyroControlsGuide_DualJoycons";
        mIsShowGuideWindow = rc::appearGuideGameWindowWithPriority(
            this, "SingleMode_GuideMessage", pMessage, GuideMessagePriority(2), -1, 0.0f);
    } else if (rc::isCurrentGuideGameWindowUser(this)) {
        rc::disappearGuideGameWindow(this);
        mIsShowGuideWindow = false;
    }
}

/**
 * @brief Wait while Bowser Jr. flies over to paint the graffiti.
 */
void KoopaGraffiti::exeBroadcast() {
    if (al::isFirstStep(this)) {
        if (mGuideBalloon != nullptr) {
            mGuideBalloon->endShow();
        }

        mIsBroadcasting = true;
        if (rc::isCurrentGuideGameWindowUser(this)) {
            rc::disappearGuideGameWindow(this);
            mIsShowGuideWindow = false;
        }

        al::startHitReaction(this, "Touched");
    }

    PlayerKoopaJr* pKoopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
    if (pKoopaJr == nullptr ||
        pKoopaJr->isTargettingGraffiti(al::getHitSensor(this, "Broadcast"))) {
        return;
    }

    mIsBroadcasting = false;
    al::startNerveAction(this, "Wait");
}

/**
 * @brief Get painted by Bowser Jr. until the painting is complete.
 */
void KoopaGraffiti::exePainting() {
    if (al::isFirstStep(this)) {
        if (rc::isCurrentGuideGameWindowUser(this)) {
            rc::disappearGuideGameWindow(this);
            mIsShowGuideWindow = false;
        }

        al::invalidateClipping(this);
        if (mGuideBalloon != nullptr) {
            mGuideBalloon->endShow();
        }

        al::invalidateHitSensor(this, "Broadcast");
        al::invalidateHitSensor(this, "Display");
        mIsMono = false;
    }

    if (mPaintCount < 4) {
        if (!al::isGreaterEqualStep(this, 300)) {
            return;
        }

        mPaintCount = 3;
        al::setMtpAnimFrameAndStop(this, 3.0f);
    }

    al::startNerveAction(this, "Finish");
    al::startHitReaction(this, "Finale");
}

/**
 * @brief Finish the painting: turn the switch on, hand out the rewards and save the state.
 */
void KoopaGraffiti::exeFinish() {
    if (al::isFirstStep(this)) {
        al::tryStartSklAnimIfExist(this, "Finish");
        al::tryStartVisAnimIfExist(this, "Finish");
        al::invalidateHitSensor(this, "Broadcast");
        al::invalidateHitSensor(this, "Display");
        if (!mIsSwitchOnInstant) {
            al::tryOnStageSwitch(this, "SwitchOn");
        }

        if (mIsAppearItem && mItemType >= 1) {
            appearItem();
        }

        if (mGoalItem != nullptr && !mIsPainted && al::isDead(mGoalItem)) {
            mGoalItem->appear();
        }

        if (mZoneId >= 1) {
            SingleModeDataFunction::setVandalizeIsland(GameDataHolderWriter(this), mZoneId - 1,
                                                       mGraffitiId);
            mIsPainted = true;
        }

        al::validateClipping(this);
    }
}

/**
 * @brief Pop the reward item out of the graffiti.
 */
void KoopaGraffiti::appearItem() {
    sead::Vector3f frontDir = sead::Vector3f::ey;
    al::calcUpDir(&frontDir, this);
    sead::Vector3f upDir = frontDir;
    if (al::isNear(sead::Mathf::abs(upDir.y), 1.0f, 0.001f)) {
        al::calcFrontDir(&frontDir, this);
    }

    if (mItem == nullptr) {
        al::appearItemTiming(this, "通常アイテム", al::getTrans(this) + upDir * 100.0f, frontDir);
        return;
    }

    ItemStatePopUpFrontParam param;
    param.setDefault();
    param._40.set(0.0f, mItemVelY, mItemVelZ);
    param._77 = true;
    al::resetPosition(mItem, al::getTrans(this) + upDir * 100.0f, false);
    if (al::tryGetQuatPtr(mItem) != nullptr) {
        sead::Vector3f side;
        side.setCross(sead::Vector3f::ey, frontDir);
        bool isFrontUp = al::isNearZero(side, 0.001f);
        sead::Quatf* pQuat = al::getQuatPtr(mItem);
        if (isFrontUp) {
            al::makeQuatUpFront(pQuat, sead::Vector3f::ey, sead::Vector3f::ez);
        } else {
            al::makeQuatUpFront(pQuat, sead::Vector3f::ey, frontDir);
        }
    } else {
        if (al::isNearDirection(frontDir, sead::Vector3f::ey, 0.01f)) {
            frontDir = sead::Vector3f::ez;
        }

        al::setFront(mItem, frontDir);
    }

    switch (mItemType) {
    case rc::ItemType_KinokoOneUp:
        appearPopUpItem(static_cast<KinokoOneUp*>(mItem), &param);
        break;
    case rc::ItemType_KinokoSuper:
        appearPopUpItem(static_cast<KinokoSuper*>(mItem), &param);
        break;
    case rc::ItemType_SuperBell:
        appearPopUpItem(static_cast<SuperBell*>(mItem), &param);
        break;
    case rc::ItemType_FireFlower:
        appearPopUpItem(static_cast<FireFlower*>(mItem), &param);
        break;
    case rc::ItemType_SuperLeaf:
        appearPopUpItem(static_cast<SuperLeaf*>(mItem), &param);
        break;
    case rc::ItemType_BoomerangFlower:
        appearPopUpItem(static_cast<BoomerangFlower*>(mItem), &param);
        break;
    case rc::ItemType_SuperStar:
        appearPopUpItem(static_cast<SuperStar*>(mItem), &param);
        break;
    case rc::ItemType_AssistLeaf:
        appearPopUpItem(static_cast<AssistLeaf*>(mItem), &param);
        break;
    case rc::ItemType_SuperBellSpecial:
        appearPopUpItem(static_cast<SuperBellSpecial*>(mItem), &param);
        break;
    case rc::ItemType_KinokoTreasure:
        appearPopUpItem(static_cast<KinokoTreasure*>(mItem), &param);
        break;
    case rc::ItemType_WhiteBell:
        appearPopUpItem(static_cast<WhiteBell*>(mItem), &param);
        break;
    default:
        break;
    }
}

/**
 * @brief Nerve for the Cat Shine appearing out of the graffiti (does nothing).
 */
void KoopaGraffiti::exeGoalItemAppear() {}
