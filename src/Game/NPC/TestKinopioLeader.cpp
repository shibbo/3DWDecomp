#include "NPC/TestKinopioLeader.hpp"
#include "MapObj/GreenStar.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(TestKinopioLeader, Move)
NERVE_DECL(TestKinopioLeader, RouteDokan)
NERVE_DECL(TestKinopioLeader, Appear)
NERVE_DECL(TestKinopioLeader, StandBy)
NERVE_DECL(TestKinopioLeader, Wait)
NERVE_DECL(TestKinopioLeader, TakeOut)
const struct {
    TestKinopioLeaderNrvMove Move;
    TestKinopioLeaderNrvRouteDokan RouteDokan;
    TestKinopioLeaderNrvAppear Appear;
    TestKinopioLeaderNrvStandBy StandBy;
    TestKinopioLeaderNrvWait Wait;
    TestKinopioLeaderNrvTakeOut TakeOut;
} NrvTestKinopioLeader;
} // namespace
/** @brief Creates the Captain Toad test actor. @param pName Actor name. */
TestKinopioLeader::TestKinopioLeader(const char* pName)
    : al::LiveActor(pName), mState(0), mMoveSpeed(10.0f), mGreenStar(nullptr) {}
/** @brief Releases the test actor. */
TestKinopioLeader::~TestKinopioLeader() = default;
/** @brief Initializes rail movement, switches, and the linked Green Star. @param rInfo Placement
 * initialization. */
void TestKinopioLeader::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::tryGetArg(&mMoveSpeed, rInfo, "MoveSpeed");
    al::tryGetArg(&mState, rInfo, "State");
    const bool syncAppear = al::trySyncStageSwitchAppear(this);
    if (al::calcLinkChildNum(rInfo, "GreenStar")) {
        mGreenStar = new GreenStar("\u30b0\u30ea\u30fc\u30f3\u30b9\u30bf\u30fc", nullptr, false);
        al::initLinksActor(mGreenStar, rInfo, "GreenStar", 0);
        al::copyPose(mGreenStar, this);
        if (al::isValidSwitchStart(this) | syncAppear) {
            mGreenStar->makeActorDead();
        } else {
            mGreenStar->makeActorAppeared();
        }
    }
    if (al::isExistRail(this)) {
        al::setSyncRailToNearestPos(this);
        if (mState == 0) {
            al::startAction(this, "Move");
            al::setSklAnimBlendWeight(this, 0.0f, 0);
            al::setSklAnimBlendWeight(this, 1.0f, 2);
            al::initNerve(this, &NrvTestKinopioLeader.Move, 0);
        } else if (mState == 1) {
            al::startAction(this, "RouteDokanMove");
            al::initNerve(this, &NrvTestKinopioLeader.RouteDokan, 0);
        } else {
            makeActorDead();
            return;
        }
    } else if (syncAppear) {
        al::initNerve(this, &NrvTestKinopioLeader.Appear, 0);
    } else if (mGreenStar) {
        al::initNerve(this, &NrvTestKinopioLeader.StandBy, 0);
    } else {
        al::initNerve(this, &NrvTestKinopioLeader.Wait, 0);
    }
}
/** @brief Pushes players contacting the actor. @param pSelf Own sensor. @param pOther Contact
 * sensor. */
void TestKinopioLeader::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}
/** @brief Rejects sensor messages. @param pMessage Unused message. @param pOther Unused sender.
 * @param pSelf Unused receiver. @return Always false. */
bool TestKinopioLeader::receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                                   al::HitSensor* pSelf) {
    return false;
}
/** @brief Plays the arrival animation and selects the reward or waiting phase. */
void TestKinopioLeader::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WallHitLand");
    }
    if (al::isActionEnd(this)) {
        if (mGreenStar) {
            if (al::isValidSwitchStart(this)) {
                al::setNerve(this, &NrvTestKinopioLeader.StandBy);
            } else {
                al::setNerve(this, &NrvTestKinopioLeader.TakeOut);
            }
        } else {
            al::setNerve(this, &NrvTestKinopioLeader.Wait);
        }
    }
}
/** @brief Waits for the switch that releases the reward. */
void TestKinopioLeader::exeStandBy() {
    if (al::isFirstStep(this)) {
        if (mState == 2) {
            al::startAction(this, "JumpBackStart");
        } else {
            al::startAction(this, "Wait");
        }
    }
    if (al::isOnSwitchStart(this)) {
        al::setNerve(this, &NrvTestKinopioLeader.TakeOut);
    }
}
/** @brief Presents the Green Star after the goal-pose delay. */
void TestKinopioLeader::exeTakeOut() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "GoalPose");
    }
    if (al::isGreaterEqualStep(this, 120)) {
        mGreenStar->appearWithPos(al::getTrans(this) + sead::Vector3f(0.0f, 300.0f, 0.0f));
        al::setNerve(this, &NrvTestKinopioLeader.Wait);
    }
}
/** @brief Plays the idle animation. */
void TestKinopioLeader::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
}
/** @brief Follows the rail until the start switch requests the reward phase. */
void TestKinopioLeader::exeMove() {
    al::moveSyncRailTurn(this, mMoveSpeed);
    al::faceToDirection(this, al::getRailDir(this));
    if (al::isOnSwitchStart(this)) {
        if (mGreenStar) {
            al::setNerve(this, &NrvTestKinopioLeader.TakeOut);
        } else {
            al::setNerve(this, &NrvTestKinopioLeader.Wait);
        }
    }
}
/** @brief Follows the pipe route and keeps an uncollected star ahead on the rail. */
void TestKinopioLeader::exeRouteDokan() {
    al::moveSyncRailTurn(this, mMoveSpeed);
    al::faceToDirection(this, al::getRailDir(this));
    if (mGreenStar) {
        if (al::isDead(mGreenStar)) {
            if (al::isOnSwitchStart(this)) {
                mGreenStar->makeActorAppeared();
            }
        } else if (!mGreenStar->isAcquiredInScene()) {
            sead::Vector3f* pPosition = al::getTransPtr(mGreenStar);
            al::calcRailPosAtCoord(pPosition, this, al::getRailCoord(this) + 1000.0f);
        }
    }
}
