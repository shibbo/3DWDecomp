#include "MapObj/BoxCoin.hpp"
#include "MapObj/HeadgearPoseBuilder.hpp"
#include "MapObj/HeadgearStateBlow.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Project/Controller/PadRumbleKeeper.hpp"
#include "Util/PlayerUtil.hpp"
#include <prim/seadDelegate.h>
#include "Project/Base/StringUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
BoxCoinReleaseParam::BoxCoinReleaseParam(const char* name, int level, float height, bool direction)
    : mAction(name), mLevel(level), mHeight(height), mDirection(direction) {}
namespace {
NERVE_DECL(BoxCoin, Wait);
NERVE_DECL(BoxCoin, Blow);
NERVE_DECL(BoxCoin, Appear);
NERVE_DECL(BoxCoin, Attach);
NERVE_MAKE_CONST(BoxCoin, Attach);
NERVE_DECL(BoxCoin, Carry);
NERVE_MAKE_CONST(BoxCoin, Carry);
const struct { BoxCoinNrvWait Wait; BoxCoinNrvBlow Blow; BoxCoinNrvAppear Appear; } NrvBoxCoin;
const int releaseSteps[] = {3, 6, 9, 20, 40};
const BoxCoinReleaseParam releaseParams[] = {
    {"BodyAttack", 1, 120.0f, false},
    {"ClimbAttack", 2, 120.0f, false},
    {"ClimbBodyAttack", 1, 120.0f, false},
    {"ClimbClimbSlidingAttack", 1, 120.0f, false},
    {"ClimbClimbWallRun", 2, 150.0f, false},
    {"ClimbRollingAttack", 1, 120.0f, false},
    {"Dash", 3, 120.0f, false},
    {"DashSuper", 2, 120.0f, false},
    {"DashPanelDash", 1, 120.0f, false},
    {"FallSlowly", 3, 120.0f, false},
    {"HipDrop", 0, 120.0f, false},
    {"JumpBackHigh", 1, 120.0f, false},
    {"JumpHipDrop", 1, 150.0f, false},
    {"JumpTurn", 2, 120.0f, false},
    {"JumpSkate", 2, 120.0f, false},
    {"JumpSuper", 1, 150.0f, false},
    {"RollingAir", 1, 120.0f, false},
    {"RollingAirShort", 2, 120.0f, false},
    {"RollingPropeller", 2, 120.0f, false},
    {"RollingShortPropeller", 2, 120.0f, false},
    {"RouteDokanBazookaFly", 1, 120.0f, false},
    {"RouteDokanJump", 1, 200.0f, false},
    {"RouteDokanLauncherMove", 1, 120.0f, false},
    {"RouteDokanMove", 1, 120.0f, false},
    {"SkateShoesWait", 2, 120.0f, false},
    {"Sliding", 2, 120.0f, false},
    {"SpinAttack", 1, 120.0f, true},
    {"SpinAttackJump", 1, 120.0f, true},
    {"SpinGroundL", 2, 120.0f, true},
    {"SpinGroundR", 2, 120.0f, true},
    {"SpinJumpL", 1, 125.0f, true},
    {"SpinJumpR", 1, 125.0f, true},
    {"SwimDive", 1, 120.0f, false},
    {"SwimHipDrop", 1, 120.0f, false},
    {"SwimStand", 3, 120.0f, false},
    {"SwimStandMove", 3, 120.0f, false},
    {"SwimStandSurface", 3, 120.0f, false},
    {"TailAttack", 1, 120.0f, true},
    {"Walk", 4, 120.0f, false},
    {"WallKeep", 2, 120.0f, false},
    {"WallKeepReverse", 2, 120.0f, false},
    {"WallSlideL", 2, 120.0f, false},
    {"WallSlideR", 2, 120.0f, false},
};
const BoxCoinReleaseParam* findReleaseParam(const char* name) {
    for (size_t i = 0; i < 43; ++i)
        if (releaseParams[i].isEqualActionName(name)) return &releaseParams[i];
    return nullptr;
}
bool isNormalJump(const HeadgearPoseBuilder* pose) {
    if (pose->isPlayerActionPlaying("JumpBackHigh")) return false;
    if (pose->isPlayerActionPlaying("JumpHipDrop")) return false;
    if (pose->isPlayerActionPlaying("JumpTurn")) return false;
    if (pose->isPlayerActionPlaying("JumpSkate")) return false;
    if (pose->isPlayerActionPlaying("SpinAttackJump")) return false;
    if (pose->isPlayerActionPlaying("SpinJumpL")) return false;
    if (pose->isPlayerActionPlaying("SpinJumpR")) return false;
    if (al::isEqualString(pose->getPlayerActionName(), "Fall")) return true;
    return al::isEqualSubString(pose->getPlayerActionName(), "Jump");
}
const BoxCoinReleaseParam* selectReleaseParam(HeadgearPoseBuilder* pose) {
    auto* poseSensor = pose->getPlayerSensor();
    auto* player = al::getSensorHost(poseSensor);
    const char* action = al::getActionName(player);
    if (rc::isPlayerDeadOrBubble(player)) return nullptr;
    else if (al::isEqualSubString(action, "TailAttack")) return findReleaseParam("TailAttack");
    else if (rc::isPlayerSliding(player)) return findReleaseParam("Sliding");
    else if (pose->isPlayerActionPlaying("Move") || pose->isPlayerActionPlaying("Fall") || isNormalJump(pose)) {
        if (isNormalJump(pose) && sead::Mathf::abs(rc::getPlayerVelocity(poseSensor).y) > 25.0f) return findReleaseParam("JumpSuper");
        else if (rc::getPlayerSpeedH(poseSensor) > 16.0f) return findReleaseParam("DashPanelDash");
        else if (rc::isPlayerDashFast(player)) return findReleaseParam("DashSuper");
        else if (rc::isPlayerDash(player)) return findReleaseParam("Dash");
        else if (!rc::isPlayerWait(player)) return findReleaseParam("Walk");
    } else {
        for (size_t i = 0; i < 43; ++i)
            if (pose->isPlayerActionPlaying(releaseParams[i].mAction)) return &releaseParams[i];
    }
    return nullptr;
}

void rotateReleaseDir(sead::Vector3f* dir, const BoxCoinReleaseParam* param) {
        bool isSpinJump = param->isEqualActionName("SpinJumpL") || param->isEqualActionName("SpinJumpR");
        float angle = 135.0f;
        if (isSpinJump) angle = 45.0f;
        if (param->isEqualActionName("SpinGroundR") || param->isEqualActionName("SpinJumpR") ||
            param->isEqualActionName("TailAttack") || param->isEqualActionName("TailAttackAir")) angle = -angle;
        al::rotateVectorDegreeY(dir, angle);
}

}
BoxCoin::BoxCoin(const char* name) : al::LiveActor(name),
    mItem(new al::LiveActor("コインボックス【アイテム状態】")), mBlowParam(new HeadgearStateBlowParam) {}
BoxCoin::~BoxCoin() {}
void BoxCoin::init(const al::ActorInitInfo& info) {
    mSingle = info.getActorSceneInfo().isSingleMode;
    const char* suffix = mSingle ? "SM" : nullptr;
    al::initActorWithArchiveName(this, info, "BoxCoin", suffix);
    if (mSingle) alActorSystemFunction::tryCompletelyRemoveFromExecutorDraw(this, &mDrawers);
    al::initNerve(this, &NrvBoxCoin.Wait, 1);
    al::offCollide(this);
    al::hideSilhouetteModel(this);
    al::hideModel(this);
    mPoseBuilder = new HeadgearPoseBuilder(this, nullptr);
    mBlowState = new HeadgearStateBlow(this, mBlowParam);
    al::initNerveState(this, mBlowState, &NrvBoxCoin.Blow, "吹き飛び状態");
    al::initActorWithArchiveName(mItem, info, "BoxCoin", "Item");
    mItem->makeActorDead();
    mRumble = al::createPadRumbleKeeper(this, -1);
    if (al::isPlaced(info)) makeActorAppeared();
    else makeActorDead();
    mDelegate = new sead::Delegate<HeadgearPoseBuilder>(mPoseBuilder, &HeadgearPoseBuilder::updateAndCalcAnimDirect);
}
void BoxCoin::appear() {
    al::LiveActor::appear();
    al::copyPose(mItem, this);
    mItem->appear();
    al::setNerve(this, &NrvBoxCoin.Appear);
    if (al::isExistCollisionParts(this)) al::invalidateCollisionParts(this);
    if (mSingle) {
        al::hideSilhouetteModelIfShow(this);
        al::hideModelIfShow(this);
        al::validateClipping(this);
        al::validateHitSensors(this);
        mCoinCount = 0;
    }
}
void BoxCoin::kill() {
    if (mDrawRestored && mSingle) {
        mDrawRestored = false;
        alActorSystemFunction::removeFromExecutorDraw(this, &mDrawers);
    }
    al::LiveActor::kill();
}
void BoxCoin::appearHipDrop(bool collision) {
    mHipDrop = true;
    appear();
    if (collision && al::isExistCollisionParts(this)) al::validateCollisionParts(this);
}
bool BoxCoin::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (!al::isSensorPlayerOrPlayerWeapon(sender)) return false;
    if (mPoseBuilder->getPlayerSensor() && !al::isNerve(this, &NrvBoxCoin.Blow) &&
        mBlowState->tryStart(msg, mPoseBuilder->getPlayerSensor())) {
        mPoseBuilder->clearPlayerSensor();
        al::setNerve(this, &NrvBoxCoin.Blow);
        return true;
    }
    if ((al::isNerve(this, &NrvBoxCoin.Appear) || al::isNerve(this, &NrvBoxCoin.Wait)) &&
        al::isMsgPlayerObjUpperPunch(msg)) return !tryShiftCarry(receiver, sender);
    if (al::isNerve(this, &NrvBoxCoin.Wait) && al::isMsgPlayerFloorTouch(msg)) mFloorTouch = true;
    return false;
}
bool BoxCoin::tryShiftCarry(al::HitSensor* self, al::HitSensor* player) {
    if (rc::isPlayerManekinekoStatueOn(player)) return false;
    if (!rc::tryPlayerEquipHeadgear(player, self, mDelegate, 0)) return false;
    if (mSingle) {
        mDrawRestored = true;
        alActorSystemFunction::addBackToExecutorDraw(this, &mDrawers);
    }
    mPoseBuilder->start(player);
    al::showSilhouetteModel(this);
    al::invalidateClipping(this);
    al::invalidateHitSensors(this);
    if (al::isExistCollisionParts(this)) al::invalidateCollisionParts(this);
    mRumble->setPort(rc::getPlayerInputPort(player));
    al::setNerve(this, &NrvBoxCoinAttach);
    return true;
}
void BoxCoin::applyVisDark() {
    al::startVisAnim(this, "Dark");
    al::startVisAnim(mItem, "Dark");
}
bool BoxCoin::isCarried() const { return !al::isNerve(this, &NrvBoxCoin.Appear) && !al::isNerve(this, &NrvBoxCoin.Wait); }
void BoxCoin::disappear(bool reaction) {
    if (reaction) al::startHitReactionDisappear(this);
    if (mItem) mItem->kill();
    kill();
}
void BoxCoin::exeAppear() {
    if (al::isFirstStep(this)) {
        if (mHipDrop) al::startAction(mItem, "AppearHipDrop");
        else al::startAction(mItem, "Appear");
    }
    if (al::isActionEnd(mItem)) {
        if (al::isExistCollisionParts(this) && !al::isValidCollisionParts(this)) al::validateCollisionParts(this);
        al::setNerve(this, &NrvBoxCoin.Wait);
    }
}
void BoxCoin::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(mItem, "WaitItem");
        mFloorTouch = false;
    }
    if (mFloorTouch) {
        if (!al::isActionPlaying(mItem, "WaitItemStop")) al::startAction(mItem, "WaitItemStop");
    } else {
        if (!al::isActionPlaying(mItem, "WaitItem")) al::startAction(mItem, "WaitItem");
    }
    mFloorTouch = false;
}
bool BoxCoin::updateCarryPose() {
    auto* sensor = mPoseBuilder->getPlayerSensor();
    if (rc::isPlayerDamageTrigOn(sensor) || rc::isPlayerDead(al::getSensorHost(sensor))) {
        rc::removePlayerEquipHeadgear(sensor, false);
        return false;
    }
    mPoseBuilder->update();
    return true;
}
void BoxCoin::exeAttach() {
    if (!updateCarryPose()) return;
    if (al::isFirstStep(this)) {
        mItem->kill();
        al::showModelIfHide(this);
        al::startAction(this, "Attach");
    }
    if (al::isActionEnd(this)) {
        al::startAction(this, "Wait");
        al::setNerve(this, &NrvBoxCoinCarry);
    }
}
void BoxCoin::exeCarry() {
    if (!updateCarryPose()) return;
    if (mCoinCount > 99) return;
    auto* sensor = mPoseBuilder->getPlayerSensor();
    if (mSingle && rc::isPlayerInDokanNotRouteDokan(al::getSensorHost(sensor))) return;
    if (mSuperJump && (rc::isPlayerOnGround(sensor) || rc::isPlayerInWater(sensor))) mSuperJump = false;
    if (mRouteJump && rc::isPlayerOnGround(sensor) && !rc::isPlayerBinded(sensor)) mRouteJump = false;
    const BoxCoinReleaseParam* param = selectReleaseParam(mPoseBuilder);
    if (mSuperJump) param = findReleaseParam("JumpSuper");
    if (param && mRouteJump) {
        if (!param->isEqualActionName("RouteDokanMove") && isNormalJump(mPoseBuilder))
            param = findReleaseParam("RouteDokanJump");
    }
    mReleaseParam = param;
    if (!mReleaseParam) return;
    if (!al::isIntervalStep(this, param->getReleaseStep(), 0)) return;
    al::startAction(this, "Generate");
    al::setAppearItemOffset(this, sead::Vector3f(0.0f, param->mHeight, 0.0f));
    al::setAppearItemAttackerSensor(this, sensor);
    if (param->mDirection) {
        rotateReleaseDir(&mReleaseDir, param);
        al::appearItemTiming(this, "前方コイン", al::getTrans(this), mReleaseDir);
    } else al::appearItemTiming(this, "コイン");
    al::startSe(this, "EmitCoin", nullptr);
    if (!mSuperJump && param->isEqualActionName("JumpSuper")) mSuperJump = true;
    if (!mRouteJump && param->isEqualActionName("RouteDokanMove")) mRouteJump = true;
    if (mCoinCount++ >= 99) {
        rc::removePlayerEquipHeadgear(sensor, false);
        mBlowParam->mDamageAction = "BlowSuccess";
    }
}
bool BoxCoinReleaseParam::isEqualActionName(const char* name) const { return al::isEqualString(name, mAction); }
int BoxCoinReleaseParam::getReleaseStep() const { return releaseSteps[mLevel]; }
void BoxCoin::exeBlow() {
    if (al::isFirstStep(this)) {
        al::showModelIfHide(this);
        al::validateShadow(this);
    }
    if (al::updateNerveState(this)) kill();
}
