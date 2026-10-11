#include "NPC/NekoParent.hpp"

#include <prim/seadBitFlag.h>
#include <prim/seadSafeString.h>

#include "Camera/CameraPoserFollowLimit.hpp"
#include "Camera/DummyCameraTarget.hpp"
#include "Layout/GuideBalloon.hpp"
#include "Layout/IslandMap.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/GoalItem.hpp"
#include "NPC/ActorStateSupportStroke.hpp"
#include "NPC/NekoNormal.hpp"
#include "NPC/NpcFunction.hpp"
#include "NPC/NpcHeadController.hpp"
#include "NPC/NpcStateParam.hpp"
#include "NPC/NpcTargetFinder.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/ScenarioInfo.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraDirector.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraTurnInfo.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace {
NERVE_DECL(NekoParent, Wait)
NERVE_DECL(NekoParent, Stroke)
NERVE_DECL(NekoParent, Appear)
NERVE_DECL(NekoParent, WaitDisasterAnticipation)
NERVE_DECL(NekoParent, HitReact)
NERVE_DECL(NekoParent, DemoIntro)
NERVE_DECL(NekoParent, DemoTargetWait)
NERVE_DECL(NekoParent, DemoTakeOut)
NERVE_DECL(NekoParent, DemoCoinGive)
NERVE_DECL(NekoParent, GoalWait)

NERVES_MAKE_NOSTRUCT(NekoParent, Wait, Stroke, Appear, WaitDisasterAnticipation, HitReact,
                     DemoIntro, DemoTargetWait, DemoTakeOut, DemoCoinGive, GoalWait)

sead::Vector3f sReactSensorOffset(0.0f, 100.0f, 150.0f);
sead::Vector3f sGuideBalloonOffset(0.0f, 120.0f, 0.0f);
NpcTargetFinderParam sTargetFinderParam(550.0f, 100.0f, 30.0f, -1, 700.0f, 300.0f, 100.0f, 700.0f,
                                        false, 10);
NpcTargetFinderParam sCollectFinderParam(650.0f, 100.0f, 50.0f, -1, 800.0f, 500.0f, 100.0f,
                                         800.0f, false, 1);
NpcStateParam sStateParam(2.25f, 0.98f, 0.89f, 300.0f, 800.0f, 80.0f, 40.0f, 150.0f, 0, 0.0f);
const sead::BitFlag32 sSearchTargetTypes(npc::NpcFindTargetType_Player |
                                         npc::NpcFindTargetType_Neko);

/**
 * @brief Get the regular cat mode of a cat.
 * @param pNeko The cat.
 * @return The regular cat mode, or nullptr if the cat is not a regular cat.
 */
NekoNormal* tryGetNekoNormal(const Neko* pNeko) {
    if (pNeko->getNormalModeActor()->getNekoType() <= 4) {
        return static_cast<NekoNormal*>(pNeko->getNormalModeActor());
    }

    return nullptr;
}

/**
 * @brief Start an action once the current action reached its last frame.
 * @param pActor Actor to animate.
 * @param pActionName Name of the action to start.
 */
void tryStartActionAfterCurrentEnd(al::LiveActor* pActor, const char* pActionName) {
    if (al::isActionPlaying(pActor, pActionName)) {
        return;
    }

    const char* currentName = al::getActionName(pActor);
    if (!al::isActionPlaying(pActor, currentName)) {
        return;
    }

    if (al::getActionFrame(pActor) >= al::getActionFrameMax(pActor, currentName) - 1.0f) {
        al::startAction(pActor, pActionName);
    }
}
}  // namespace

/**
 * @brief Construct the cat parent mode.
 * @param pHost Host cat of the mode.
 */
NekoParent::NekoParent(Neko* pHost) : IUseNekoModeActor("NekoParent"), mHost(pHost) {
    mParam = new neko::Param();
}

/**
 * @brief Initialize the cat parent, its kittens, drop targets and rewards.
 * @param rInfo Placement information.
 * @param colorType Coat color of the cat.
 * @param pTargetFinder Target finder shared with the other modes of the cat.
 */
void NekoParent::init(const al::ActorInitInfo& rInfo, neko::ColorType colorType,
                      NpcTargetFinder* pTargetFinder) {
    neko::Param* param = mParam;
    al::tryGetArg(&param->mChaseRangeOverride, rInfo, "ChaseRange");
    al::tryGetArg(&param->mIsEnableCliffCheck, rInfo, "IsEnableCliffCheck");
    al::tryGetArg(&param->mIsEnableShoreCheck, rInfo, "IsEnableShoreCheck");
    al::tryGetArg(&param->mIsDisabledPR, rInfo, "isDisabledPR");
    al::tryGetArg(&param->mIsDisablePlessieChase, rInfo, "isDisablePlessieChase");
    al::tryGetStringArg(&param->mComment, rInfo, "Comment");
    al::tryGetArg(&param->mChaseRange, rInfo, "ChaseRange");
    mColorType = colorType;
    al::initActorWithArchiveName(this, rInfo, "NekoParent", nullptr);

    mTargetFinder = pTargetFinder;
    pTargetFinder->setParam(&sTargetFinderParam);
    mTargetFinder->setSearchTypes(sSearchTargetTypes.getDirect());
    mTargetFinder->setTargetTypePriority(npc::NpcFindTargetType_Player, 0);
    mTargetFinder->setTargetTypePriority(npc::NpcFindTargetType_Neko, 1);

    const al::Resource* modelResource = al::getModelResource(this);
    if (al::ByamlIter initIter;
        al::tryGetActorInitFileIter(&initIter, modelResource, "InitNeko", nullptr)) {
        if (al::ByamlIter headIter; initIter.tryGetIterByKey(&headIter, "NpcHeadControl")) {
            mHeadController = neko::makeHeadController(this, headIter, mTargetFinder,
                                                       npc::NpcFindTargetType_All);
        }
    }

    if (al::tryGetSceneObj(this, SceneObjID_NekoParentHolder) == nullptr) {
        al::setSceneObj(this, new NekoParentHolder(), SceneObjID_NekoParentHolder);
    }

    al::tryGetSceneObj<NekoParentHolder>(this, SceneObjID_NekoParentHolder)->add(this);
    al::initNerve(this, &NrvNekoParentWait, 2);
    mStateSupportStroke = new ActorStateSupportStroke(this);
    al::initNerveState(this, mStateSupportStroke, &NrvNekoParentStroke, "[state]Stroke");

    s32 targetNum = al::calcLinkChildNum(al::getPlacementInfo(rInfo), "GoalSeekTarget");
    al::calcLinkChildNum(al::getPlacementInfo(rInfo), "GoalItem");
    s32 kittenNum = al::calcLinkChildNum(al::getPlacementInfo(rInfo), "Kitten");

    if (targetNum > 0) {
        mDropTargets.allocBuffer(targetNum, nullptr);
        al::PlacementInfo targetInfo;
        for (s32 i = 0; i < targetNum; i++) {
            al::getLinksInfoByIndex(&targetInfo, al::getPlacementInfo(rInfo), "GoalSeekTarget", i);
            auto* target = new neko::Target(targetInfo, mHost->getUID(), i, this);
            target->mRange = mParam->mChaseRange;
            mDropTargets.pushBack(target);
        }
    }

    if (kittenNum > 0) {
        SingleModeDataFunction::tryGetNekoSaveDataByParentID(this, mHost->getUID(), &mDropTargets);
        mKittens.allocBuffer(kittenNum, nullptr);
        mFoundKittens.allocBuffer(kittenNum, nullptr);
        for (s32 i = 0; i < kittenNum; i++) {
            auto* kitten = new Neko("Neko", mHost);
            al::ActorInitInfo kittenInfo;
            al::PlacementInfo kittenPlacement;
            al::getLinksActorInfo(&kittenInfo, &kittenPlacement, rInfo, "Kitten", i);
            kitten->initAsModelName(kittenInfo, "NekoE");
            kitten->startKill();
            mKittens.pushBack(kitten);
            if (isKittenAtDropTarget(kitten)) {
                mFoundKittens.pushBack(kitten);
            }
        }
    }

    if (al::isExistLinkChild(al::getPlacementInfo(rInfo), "GoalItem", 0)) {
        mGoalItem = new GoalItem("GoalItem");
        al::initLinksActor(mGoalItem, rInfo, "GoalItem", 0);
        mGoalItem->makeActorDead();
    }

    al::createAndSetColliderSpecialPurpose(this, "NekoMoveLimit");
    al::setSensorFollowPosOffset(this, "React", sReactSensorOffset);
    al::calcJointPos(mHeadPos, this, "Head");
    mGuideBalloon = new GuideBalloon("guide", al::getLayoutInitInfo(rInfo), mHeadPos,
                                     sGuideBalloonOffset, false, nullptr);

    if (al::isExistLinkChild(al::getPlacementInfo(rInfo), "NextNekoParentAppear", 0)) {
        mNextParent = new Neko("Neko", mHost);
        al::ActorInitInfo nextInfo;
        al::PlacementInfo nextPlacement;
        const char* modelName = nullptr;
        al::getLinksActorInfo(&nextInfo, &nextPlacement, rInfo, "NextNekoParentAppear", 0);
        alPlacementFunction::getModelName(&modelName, nextInfo);
        mNextParent->initAsModelName(nextInfo, modelName);
    }

    mDemoCameraTarget = new DummyCameraTarget("NekoParentCamera");
    mDemoCameraTarget->init(rInfo);

    if (SingleModeDataFunction::hasNekoParentSeenDemo(this, mHost->getUID())) {
        mIsSeenDemo = true;
    } else {
        al::Resource* cameraResource = al::findOrCreateResource("ObjectData/DemoCamera", nullptr);
        mDemoCamera = al::initAnimCamera_RS(this, rInfo, cameraResource, getBaseMtx(),
                                            "DemoNekoParentIntro", true);
        if (mDemoCamera != nullptr) {
            al::startAnimCameraAnim(mDemoCamera, "DemoNekoParentIntro", -1, -1, -1);
        } else {
            mDemoCamera = al::initObjectCamera_RS(this, rInfo, nullptr);
        }
    }

    al::tryStartActionIfNotPlaying(this, !isAllKittensFound() ? "WaitSad" : "WaitCollectAll");
    tryStartDefaultBehavior();
}

/**
 * @brief Start waiting for the kittens, or waiting at the goal once they are all found.
 * @return Whether the nerve changed.
 */
bool NekoParent::tryStartDefaultBehavior() {
    if (!isAllKittensFound()) {
        if (al::isNerve(this, &NrvNekoParentWait)) {
            return false;
        }

        al::setNerve(this, &NrvNekoParentWait);
        return true;
    }

    if (al::isNerve(this, &NrvNekoParentGoalWait)) {
        return false;
    }

    al::setNerve(this, &NrvNekoParentGoalWait);
    return true;
}

/**
 * @brief Show the reward shine of the cat parent on the island map.
 */
void NekoParent::addSpecialShineLocation() {
    IslandMap* islandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
    if (islandMap == nullptr) {
        return;
    }

    GoalItem* goalItem = mGoalItem;
    if (goalItem == nullptr) {
        return;
    }

    islandMap->addSpecialShineLocation(
        mHost, al::getTrans(mHost), {goalItem->getIslandId() - 1, goalItem->getShineId() - 1});
    islandMap->setSpecialShineIconComplete(mHost, isAllKittensFound());
}

/**
 * @brief Hide the guide balloon if it is shown.
 */
void NekoParent::forceHideGuideBalloon() {
    if (mGuideBalloon->isAlive()) {
        mGuideBalloon->endShow();
    }
}

/**
 * @brief Initialize the cat parents after placement: only the first one still waiting for its
 * kittens appears.
 * @param rInfo Placement information.
 */
void NekoParentHolder::initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) {
    mCurrentParent = nullptr;
    for (s32 i = 0; i < mParents.size(); i++) {
        if (mCurrentParent != nullptr) {
            mParents[i]->startKillHost();
            continue;
        }

        mParents[i]->addSpecialShineLocation();
        if (!mParents.unsafeAt(i)->tryKillAll()) {
            mParents.unsafeAt(i)->startAppearHost();
            mCurrentParent = mParents[i];
        }
    }
}

/**
 * @brief Kill the host cat.
 */
void NekoParent::startKillHost() {
    mHost->startKill();
}

/**
 * @brief Kill the cat and its kittens once they are all found and the next parent exists.
 * @return Whether everything was killed.
 */
bool NekoParent::tryKillAll() {
    if (mNextParent == nullptr || !isAllKittensFound()) {
        return false;
    }

    for (s32 i = 0; i < mKittens.size(); i++) {
        mKittens[i]->startKill();
    }

    mHost->startKill();
    return true;
}

/**
 * @brief Make the host cat appear.
 */
void NekoParent::startAppearHost() {
    mHost->startAppearNormal();
}

/**
 * @brief Clip the cat, and its host if this is the active mode.
 */
void NekoParent::startClipped() {
    if (mHost->getModeActor() == this) {
        mHost->tryStartClipped();
    }

    al::LiveActor::startClipped();
}

/**
 * @brief Unclip the cat, and kill the cat parent riding the host once it is done.
 */
void NekoParent::endClipped() {
    if (mHost->getModeActor() == this) {
        mHost->tryEndClipped();
    }

    al::LiveActor::endClipped();

    Neko* rideNeko = mHost->getRideNeko();
    if (rideNeko != nullptr && rideNeko->isMode(Neko::Mode_Parent) &&
        static_cast<NekoParent*>(rideNeko->getModeActor())->tryKillAll()) {
        mHost->setRideNeko(nullptr);
    }
}

/**
 * @brief Attach the cat parent to its host cat.
 * @param rReason Why the mode gets attached.
 */
void NekoParent::startAttach(const NekoAttachReason& rReason) {
    mTargetFinder->clearTarget();
    mTargetFinder->setParam(&sTargetFinderParam);
    mTargetFinder->setSearchTypes(sSearchTargetTypes.getDirect());
    mTargetFinder->setTargetTypePriority(npc::NpcFindTargetType_Player, 1);
    mTargetFinder->setTargetTypePriority(npc::NpcFindTargetType_Neko, 0);

    switch (rReason.mType) {
    case NekoAttachReason::Type_AppearAtHost:
        al::resetPosition(this, al::getTrans(mHost), false);
        al::faceToDirection(this, al::getFront(mHost));
        al::hideModelIfShow(this);
        mHitReactCoolTime = 5;
        al::setNerve(this, &NrvNekoParentAppear);
        return;
    case NekoAttachReason::Type_Hide:
        mHost->tryStartHide();
        return;
    default:
        tryStartDefaultBehavior();
        return;
    }
}

/**
 * @brief Make the kittens appear, at their drop target if they were already brought back.
 */
void NekoParent::startAppearLinks() {
    if (tryKillAll()) {
        return;
    }

    for (s32 i = 0; i < mKittens.size(); i++) {
        Neko* kitten = mKittens[i];
        kitten->startAppearNormal();

        neko::Target target;
        if (SingleModeDataFunction::tryGetNekoSaveData(kitten, kitten->getUID(), &target)) {
            kitten->setActivePosition(target.mTrans);
            tryGetNekoNormal(kitten)->setNekoParent(this);
            kitten->startSeekTarget(mDropTargets[target.mIndex], true);
        }
    }
}

/**
 * @brief Start anticipating Fury Bowser's disaster.
 * @param isEmitEffect Whether to emit the anticipation effect.
 */
void NekoParent::startDisasterAnticipation(bool isEmitEffect) {
    neko::trySetNerve(this, &NrvNekoParentWaitDisasterAnticipation);
    if (isEmitEffect) {
        al::tryEmitEffect(this, "DisasterAnticipation", nullptr);
    }
}

/**
 * @brief Kill the cat parent.
 * @param isDeleteParticle Whether to also delete the particles of the anticipation effect.
 */
void NekoParent::startKill(bool isDeleteParticle) {
    forceHideGuideBalloon();
    if (isDeleteParticle) {
        al::tryDeleteEffectAndParticle(this, "DisasterAnticipation");
    } else {
        al::tryDeleteEffect(this, "DisasterAnticipation");
    }

    kill();
}

/**
 * @brief Update the cool times and forget the touched cat.
 */
void NekoParent::control() {
    if (mHitReactCoolTime > 0) {
        mHitReactCoolTime--;
    }

    if (mPackunEatCoolTime > 0) {
        mPackunEatCoolTime--;
    }

    mTouchNeko = nullptr;
}

/**
 * @brief Push away the actors touching the cat and remember the touched kittens.
 * @param pSelf Sensor of the cat.
 * @param pOther Touched sensor.
 */
void NekoParent::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isInteractive()) {
        mTargetFinder->attackSensor(pSelf, pOther);
    }

    if (al::isSensorNpc(pSelf)) {
        al::sendMsgNpcTouch(pOther, pSelf);
        if (neko::isSensorEnemyReactAttack(pOther)) {
            if (!isDemoGoalSequence() && !al::isNerve(this, &NrvNekoParentAppear) &&
                al::sendMsgNekoAttack(pOther, pSelf)) {
                neko::trySetNerve(this, &NrvNekoParentHitReact);
            }

            return;
        }

        if (al::isSensorPlayer(pOther) || al::isSensorEnemyBody(pOther) ||
            al::isSensorNpc(pOther) || al::isSensorKickKoura(pOther) ||
            al::isSensorKoopaJr(pOther) ||
            (al::isSensorMapObj(pOther) && al::isSensorName(pOther, "Body"))) {
            al::sendMsgPush(pOther, pSelf);
        }

        if (al::isSensorRide(pOther)) {
            al::sendMsgNekoPush(pOther, pSelf);
        }
    }

    if (al::isSensorEye(pSelf) && npc::isSensorNeko(pOther)) {
        auto* neko = static_cast<NekoNormal*>(al::getSensorHost(pOther));
        if (neko->canCollect() && !neko->isHold()) {
            mTouchNeko = neko;
        }
    }
}

/**
 * @brief Check whether a kitten is being brought to its drop target.
 * @return Whether a kitten is being brought to its drop target.
 */
bool NekoParent::isDemoGoalSequence() const {
    return mCollectedNeko != nullptr;
}

/**
 * @brief React to attacks.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the cat.
 * @return Whether the message was handled.
 */
bool NekoParent::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                            al::HitSensor* pSelf) {
    if (isDemoGoalSequence() || al::isNerve(this, &NrvNekoParentAppear) ||
        !al::isSensorNpc(pSelf)) {
        return false;
    }

    if ((al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
         al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgPlayerBodyAttackReflect(pMsg) ||
         al::isMsgPlayerObjStatueDrop(pMsg)) &&
        (!al::isNerve(this, &NrvNekoParentHitReact) || mHitReactCoolTime <= 0)) {
        al::setNerve(this, &NrvNekoParentHitReact);
        al::startSe(this, "PgTrample", nullptr);
        rc::requestHitReactionToAttackerNpc(pSelf, pOther);
        mHitReactCoolTime = 30;
        return true;
    }

    if (neko::isMsgHitReaction(this, pMsg, pOther, pSelf) || al::isMsgKickKouraReflect(pMsg) ||
        al::isMsgPlayerObjRollingAttack(pMsg) || al::isMsgPlayerObjHipDropAll(pMsg)) {
        if (mHitReactCoolTime > 0) {
            return false;
        }

        al::setNerve(this, &NrvNekoParentHitReact);
        if (neko::isMsgNpcAttackerHitReaction(this, pMsg, pOther, pSelf) ||
            al::isMsgPlayerObjRollingAttack(pMsg) || al::isMsgPlayerObjHipDropAll(pMsg)) {
            rc::requestHitReactionToAttackerNpc(pSelf, pOther);
        } else if (rc::isMsgPackunEat(pMsg)) {
            sead::Vector3f dir;
            al::calcDirBetweenSensors(&dir, pSelf, pOther);
            dir *= al::getSensorRadius(pSelf);
            al::startHitReactionHitEffect(this, "ＮＰＣヒット", al::getSensorPos(pSelf) + dir);
        } else if (pOther != nullptr && pSelf != nullptr) {
            al::startHitReactionHitEffect(this, "ＮＰＣヒット", pOther, pSelf);
        }

        if (al::isMsgPlayerFireBallAttack(pMsg)) {
            mHitReactCoolTime = 8;
        } else if (al::isMsgPlayerClimbAttack(pMsg)) {
            mHitReactCoolTime = 30;
        } else if (al::isMsgNekoAttack(pMsg) || al::isMsgExplosion(pMsg)) {
            mHitReactCoolTime = 60;
        } else {
            mHitReactCoolTime = 30;
        }

        return true;
    }

    if ((neko::isMsgMeraWanwanTrackAttack(this, pMsg, pOther, nullptr) ||
         al::isMsgDisasterSpikeAttack(pMsg) || al::isMsgGigaEnemyAttack(pMsg)) &&
        mHost != nullptr && mHost->tryStartHide()) {
        return true;
    }

    if (rc::isMsgPackunEatStart(pMsg) && mPackunEatCoolTime <= 0) {
        mPackunEatCoolTime = 120;
        return true;
    }

    return false;
}

/**
 * @brief Let the player stroke the cat with the touch screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Screen point target of the cat.
 * @return Whether the message was handled.
 */
bool NekoParent::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget) {
    if (!mStateSupportStroke->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (!al::isNerve(this, &NrvNekoParentStroke)) {
        al::setNerve(this, &NrvNekoParentStroke);
    }

    return true;
}

/**
 * @brief Save the kitten brought back once the reward shine is collected.
 */
void NekoParent::goalItemCollectCallback() {
    if (mCollectTarget == nullptr || mCollectedNeko == nullptr) {
        return;
    }

    SingleModeDataFunction::setNekoSaveData(mCollectedNeko, mCollectTarget->mId, mCollectTarget);
}

/**
 * @brief Only accept kittens of this cat parent that can still be collected.
 * @param pTarget Target found by the target finder.
 * @param rType Kind of the target.
 * @return Whether the target is accepted.
 */
bool NekoParent::acceptTarget(const al::LiveActor* pTarget,
                              const npc::NpcFindTargetType& rType) const {
    if (rType != npc::NpcFindTargetType_Neko) {
        return true;
    }

    if (pTarget == nullptr) {
        return false;
    }

    auto* neko = static_cast<const NekoNormal*>(pTarget);
    if (!neko->canCollect() || neko->isAtGoal()) {
        return false;
    }

    return isOwnKitten(neko);
}

/**
 * @brief Check whether a cat is one of the kittens of this cat parent.
 * @param pNeko Cat to check.
 * @return Whether the cat is one of the kittens.
 */
bool NekoParent::isOwnKitten(const IUseNekoModeActor* pNeko) const {
    for (s32 i = 0; i < mKittens.size(); i++) {
        if (pNeko->getUID() == mKittens.unsafeAt(i)->getUID()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Wait for the kittens to be brought back.
 */
void NekoParent::exeWait() {
    if (al::isFirstStep(this)) {
        mCollectedNeko = nullptr;
        mCollectPlayer = nullptr;
        al::startAction(this, "WaitSad");
        mHeadController->setLookAtTarget(nullptr);
        al::showModelIfHide(this);
    }

    updateTryCollectNeko(true);
    mHeadController->update();
    if (tryStartDemoIntro()) {
        al::setNerve(this, &NrvNekoParentDemoIntro);
    }
}

/**
 * @brief Show the guide balloon to the nearby player and collect the kittens brought back.
 * @param isShowGuide Whether to show the guide balloon and the matching animations.
 */
void NekoParent::updateTryCollectNeko(bool isShowGuide) {
    mTargetFinder->forceUpdate();

    al::LiveActor* player = mTargetFinder->tryGetTarget(npc::NpcFindTargetType_Player);
    if (player != nullptr) {
        al::LiveActor* holdActor = nullptr;
        if (rc::isPlayerOnGround(player)) {
            holdActor = rc::getPlayerHoldingActor(player);
            if (holdActor != nullptr && al::isEqualString(holdActor->getName(), "Neko") &&
                tryCollectNeko(static_cast<IUseNekoModeActor*>(holdActor))) {
                mCollectPlayer = player;
                return;
            }
        }

        al::calcJointPos(mHeadPos, this, "Head");
        if (isShowGuide) {
            if (!mGuideBalloon->isAlive()) {
                mGuideBalloon->startShowNeko(getNekoType(), getKittensFound(), getMaxKittens());
                al::startSe(this, "GuidMessageAppear", nullptr);
            }

            if (holdActor != nullptr) {
                tryStartActionAfterCurrentEnd(this, "No");
            } else {
                tryStartActionAfterCurrentEnd(this, "Wait");
            }
        }
    } else {
        forceHideGuideBalloon();
        if (isShowGuide) {
            tryStartActionAfterCurrentEnd(this, "WaitSad");
        }
    }

    auto* neko =
        static_cast<NekoNormal*>(mTargetFinder->tryGetTarget(npc::NpcFindTargetType_Neko));
    if (neko != nullptr && neko->canCollect() && tryCollectNeko(neko)) {
        mCollectPlayer = al::findNearestPlayerActor(neko);
        return;
    }

    if (isDemoGoalSequence() || mTouchNeko == nullptr) {
        return;
    }

    if (npc::calcIsTargetInSight(mTargetFinder, mTouchNeko, &sCollectFinderParam) ||
        al::calcDistanceH(mTouchNeko, al::getSensorPos(mTargetFinder->getEyeSensor())) < 300.0f) {
        if (tryCollectNeko(mTouchNeko)) {
            mCollectPlayer = al::findNearestPlayerActor(mTouchNeko);
        }
    }
}

/**
 * @brief Start the intro demo the first time a player comes near.
 * @return Whether the demo started.
 */
bool NekoParent::tryStartDemoIntro() {
    if (mIsSeenDemo) {
        return false;
    }

    if (al::isClipped(this) || !rc::isAllPlayerOnGround(this)) {
        return false;
    }

    if (!al::isNearPlayerFromTarget(this, al::getSensorPos(mTargetFinder->getEyeSensor()),
                                    850.0f)) {
        return false;
    }

    if (!rc::requestStartDemoInGameCutscene(this)) {
        return false;
    }

    IslandMap::setIslandMapEnable(this, false);
    al::invalidateClipping(this);
    rc::setDemoAudioType(this, alSeFunction::DemoType(3));
    al::changeBgmVolume(this, 0.5f, 45);
    al::startSe(this, "IntroCameraRing", nullptr);
    rc::addDemoActor(this);
    rc::addDemoActor(mHost);
    al::startAnimCameraAnim(mDemoCamera, "DemoNekoParentIntro", -1, -1, -1);
    al::startCamera_RS(this, mDemoCamera, 65);
    sead::Vector3f offset(0.0f, 100.0f, 0.0f);
    al::setFixedActor(mDemoCamera, this, 400.0f, 0.0f, 10.0f, &offset);
    SingleModeDataFunction::setNekoParentSeenDemo(this, mHost->getUID());
    mIsSeenDemo = true;
    return true;
}

/**
 * @brief Wait while Fury Bowser's disaster is anticipated.
 */
void NekoParent::exeWaitDisasterAnticipation() {
    if (al::isFirstStep(this)) {
        mCollectedNeko = nullptr;
        mCollectPlayer = nullptr;
        al::tryStartActionIfNotPlaying(this, !isAllKittensFound() ? "WaitSad" : "WaitCollectAll");
        mHeadController->setLookAtTarget(nullptr);
        mTargetFinder->clearTarget();
        forceHideGuideBalloon();
    }

    mHeadController->update();

    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (controller == nullptr) {
        return;
    }

    if (controller->getFlowStep() <= 5) {
        tryStartDefaultBehavior();
    }

    if (controller->getState() == DisasterModeController::State(9) &&
        controller->getStateFrame() < 120) {
        al::tryHoldSe(this, "PgDisasterAnticipation", nullptr);
    }
}

/**
 * @brief Play the intro demo.
 */
void NekoParent::exeDemoIntro() {
    if (!al::isGreaterEqualStep(this, 232)) {
        return;
    }

    al::endCamera_RS(this, mDemoCamera, 100, false);
    IslandMap::setIslandMapEnable(this, true);
    rc::requestEndDemoInGameCutscene(this);
    al::changeBgmVolume(this, 1.0f, 60);
    al::validateClipping(this);
    tryStartDefaultBehavior();
}

/**
 * @brief Wait for the collected kitten to reach its drop target.
 */
void NekoParent::exeDemoTargetWait() {
    if (al::isFirstStep(this)) {
        if (rc::isAnyActiveDemo(this) || !tryStartDemoCollect()) {
            al::setNerve(this, &NrvNekoParentDemoTargetWait);
            return;
        }

        al::startAction(this, "Wait");
        forceHideGuideBalloon();
        mHeadController->setLookAtTarget(al::getTransPtr(mCollectedNeko));
    }

    mHeadController->update();

    if (mCollectTarget == nullptr || mCollectedNeko == nullptr) {
        if (tryStartDefaultBehavior()) {
            endDemoCollect();
        }

        return;
    }

    if (!mCollectedNeko->isEnableGoal()) {
        mFoundKittens.erase(mFoundKittens.indexOf(mCollectedNeko->getHost()));
        mCollectTarget->mIsActive = true;
        mCollectTarget = nullptr;
        mCollectedNeko = nullptr;
        return;
    }

    if (!mCollectedNeko->isAtGoal()) {
        updateDemoCamera();
        return;
    }

    if (!isAllKittensFound()) {
        al::setNerve(this, &NrvNekoParentDemoCoinGive);
    } else {
        al::setNerve(this, &NrvNekoParentDemoTakeOut);
    }
}

/**
 * @brief Start the demo of a kitten brought back.
 * @return Whether the demo started.
 */
bool NekoParent::tryStartDemoCollect() {
    if (!rc::requestStartDemoInGameCutscene(this)) {
        return false;
    }

    al::invalidateClipping(this);
    rc::setDemoAudioType(this, alSeFunction::DemoType(3));
    rc::setDemoFullSensorUpdate(this, true);
    rc::addDemoActor(this);
    for (s32 i = 0; i < mKittens.size(); i++) {
        Neko* kitten = mKittens[i];
        if (kitten != nullptr && isKittenAtDropTarget(kitten) && kitten->getModeActor() != nullptr) {
            rc::addDemoActor(kitten->getModeActor());
        }
    }

    mDemoCameraTarget->onTarget();
    return true;
}

/**
 * @brief Make the camera follow the kitten brought back.
 */
void NekoParent::updateDemoCamera() {
    al::CameraPoser_RS* poser =
        mActorSceneInfo->cameraDirector->getCurrentTicket()->getPoser();
    al::setTrans(mDemoCameraTarget, al::getTrans(mCollectedNeko));

    if (!al::isEqualString(poser->getName(), "Follow") &&
        !al::isEqualString(poser->getName(), "Parallel")) {
        return;
    }

    f32 distance = al::calcDistance(mCollectedNeko, poser->getEye());
    if (distance < 1000.0f || distance > 3000.0f) {
        mDemoCameraTarget->setRequestDistance(sead::Mathf::clamp(distance, 1000.0f, 3000.0f));
    }

    const char* requesterName = mDemoCameraTarget->getName();
    sead::Vector3f turnDir = -al::getFront(this);
    al::CameraTurnInfo turnInfo = {requesterName, turnDir, 0.1f, 0.2f, true, true};
    poser->requestTurnToDirection(&turnInfo);
    static_cast<CameraPoserFollowLimit*>(poser)->setTargetAngleV(12.0f);
}

/**
 * @brief End the demo of a kitten brought back.
 */
void NekoParent::endDemoCollect() {
    if (!rc::isActiveDemoInGameCutscene(this)) {
        return;
    }

    IslandMap::setIslandMapEnable(this, true);
    rc::requestEndDemoInGameCutscene(this);
    mDemoCameraTarget->offTarget();
    mDemoCameraTarget->setRequestDistance(-1.0f);
    al::validateClipping(this);
}

/**
 * @brief Thank the player for a kitten brought back with coins.
 */
void NekoParent::exeDemoCoinGive() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Happy");
        mHeadController->setLookAtTarget(&al::findNearestPlayerPos(this));
        al::startSe(this, "NekoCollect", nullptr);
    }

    mHeadController->update();
    updateDemoCamera();
    if (!al::isActionEnd(this) || !tryStartDefaultBehavior()) {
        return;
    }

    al::LiveActor* kitten = mCollectedNeko;
    al::appearItemTiming(kitten, "collect", al::getTrans(kitten) + sead::Vector3f::ey * 100.0f,
                         sead::Vector3f::ey, al::getHitSensor(kitten, "Body"), false);
    al::startSe(this, "PgCoinAppear", nullptr);
    endDemoCollect();
}

/**
 * @brief Give the reward shine once every kitten is brought back.
 */
void NekoParent::exeDemoTakeOut() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "TakeOut");
        mHeadController->setLookAtTarget(&al::findNearestPlayerPos(this));
        al::startSe(this, "NekoComplete", nullptr);
    }

    mHeadController->update();
    updateDemoCamera();
    if (al::isStep(this, al::getActionFrameMax(this, "TakeOut") - 5.0f)) {
        sead::Vector3f appearPos = al::findNearestPlayerPos(this);
        rc::addDemoActor(mGoalItem);
        mGoalItem->setCollectListener(this);
        mGoalItem->appearCollect(true);
        appearPos.y += 100.0f;
        al::setTrans(mGoalItem, appearPos);
        mDemoCameraTarget->offTarget();
        mDemoCameraTarget->setRequestDistance(-1.0f);

        IslandMap* islandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
        if (islandMap != nullptr) {
            islandMap->setSpecialShineIconComplete(mHost, true);
        }
    }

    if (al::isActionEnd(this) && !rc::isAnyActiveDemo(this) && tryStartDefaultBehavior()) {
        IslandMap::setIslandMapEnable(this, true);
        al::validateClipping(this);
    }
}

/**
 * @brief Wait at the goal once every kitten is brought back, and hand over to the next parent.
 */
void NekoParent::exeGoalWait() {
    if (al::isFirstStep(this)) {
        mCollectedNeko = nullptr;
        mCollectPlayer = nullptr;
        al::validateClipping(this);
        al::setVelocityZeroH(this);
        al::startAction(this, "WaitCollectAll");
        al::showModelIfHide(this);

        IslandMap* islandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
        bool isNextParentWaiting = false;
        if (mNextParent != nullptr) {
            mNextParent->startAppearNormal();
            auto* nextParent = static_cast<NekoParent*>(mNextParent->getModeActor());
            if (nextParent != nullptr && !nextParent->isAllKittensFound()) {
                al::tryGetSceneObj<NekoParentHolder>(this, SceneObjID_NekoParentHolder)
                    ->setCurrentParent(nextParent);
                isNextParentWaiting = true;
                if (islandMap != nullptr && nextParent->mGoalItem != nullptr) {
                    GoalItem* goalItem = nextParent->mGoalItem;
                    ScenarioInfo scenarioInfo = {goalItem->getIslandId() - 1,
                                                 goalItem->getShineId() - 1};
                    islandMap->addSpecialShineLocation(mNextParent, al::getTrans(mNextParent),
                                                       scenarioInfo);
                }
            }
        }

        mHeadController->setLookAtTarget(nullptr);
        if (!isNextParentWaiting && islandMap != nullptr) {
            islandMap->setSpecialShineIconComplete(mHost, true);
        }
    }

    mHeadController->update();
    if (al::isStep(this, 60)) {
        tryStartDefaultBehavior();
    }
}

/**
 * @brief Appear next to the host cat.
 */
void NekoParent::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        al::showModelIfHide(this);
        forceHideGuideBalloon();
    }

    if (al::isActionEnd(this)) {
        tryStartDefaultBehavior();
    }
}

/**
 * @brief React to an attack.
 */
void NekoParent::exeHitReact() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "HitReact");
        al::showModelIfHide(this);
        forceHideGuideBalloon();
    }

    updateTryCollectNeko(false);
    if (al::isActionEnd(this)) {
        tryStartDefaultBehavior();
    }
}

/**
 * @brief Get stroked by the player with the touch screen.
 */
void NekoParent::exeStroke() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Stroke");
        al::showModelIfHide(this);
        al::setVelocityZeroH(this);
        forceHideGuideBalloon();
    }

    al::updateNerveState(this);
    if (!mStateSupportStroke->isTouch() && al::isGreaterStep(this, 60)) {
        tryStartDefaultBehavior();
    }
}

/**
 * @brief Check whether the cat parent still waits for kittens.
 * @return Whether some kittens are still missing.
 */
bool NekoParent::isInteractive() const {
    return !isAllKittensFound();
}

/**
 * @brief The cat parent never reacts to the player.
 * @return Always false.
 */
bool NekoParent::tryStartReactToPlayer() {
    return false;
}

/**
 * @brief Collect a kitten: send it to the nearest free drop target and start the demo.
 * @param pNeko Kitten to collect.
 * @return Whether the kitten got collected.
 */
bool NekoParent::tryCollectNeko(IUseNekoModeActor* pNeko) {
    if (pNeko == nullptr || !isOwnKitten(pNeko)) {
        return false;
    }

    neko::Target* target = tryGetNearDropTarget(pNeko);
    if (target == nullptr || !pNeko->startSeekTarget(target, true)) {
        return false;
    }

    mCollectedNeko = static_cast<NekoNormal*>(pNeko);
    mCollectedNeko->setNekoParent(this);
    mCollectTarget = target;
    mFoundKittens.pushBack(mCollectedNeko->getHost());
    mCollectTarget->mIsActive = false;
    mCollectTarget->mId = mCollectedNeko->getHost()->getUID();

    if (!SingleModeDataFunction::tryGetNekoSaveData(mCollectedNeko, mCollectTarget->mId,
                                                    nullptr) &&
        !isAllKittensFound()) {
        SingleModeDataFunction::setNekoSaveData(mCollectedNeko, mCollectTarget->mId,
                                                mCollectTarget);
    }

    if (!mIsSeenDemo) {
        SingleModeDataFunction::setNekoParentSeenDemo(this, mHost->getUID());
        mIsSeenDemo = true;
    }

    IslandMap::setIslandMapEnable(this, false);
    al::setNerve(this, &NrvNekoParentDemoTargetWait);
    return true;
}

/**
 * @brief Find the nearest free drop target.
 * @param pActor Actor to search around.
 * @return The nearest free drop target, or nullptr.
 */
neko::Target* NekoParent::tryGetNearDropTarget(const al::LiveActor* pActor) const {
    if (mDropTargets.isEmpty()) {
        return nullptr;
    }

    al::getTrans(pActor);

    neko::Target* nearTarget = nullptr;
    f32 minDistance = sead::Mathf::maxNumber();
    for (s32 i = 0; i < mDropTargets.size(); i++) {
        if (mDropTargets.unsafeAt(i)->mIsActive) {
            f32 distance = al::calcDistanceH(pActor, mDropTargets[i]->mTrans);
            if (distance < minDistance) {
                nearTarget = mDropTargets[i];
                minDistance = distance;
            }
        }
    }

    if (nearTarget != nullptr) {
        return nearTarget;
    }

    return nullptr;
}

/**
 * @brief Get the number of kittens of the cat parent.
 * @return The number of kittens.
 */
s32 NekoParent::getMaxKittens() const {
    return mKittens.capacity();
}

/**
 * @brief Get the number of kittens already brought back.
 * @return The number of kittens brought back.
 */
s32 NekoParent::getKittensFound() const {
    return mFoundKittens.size();
}
