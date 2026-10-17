#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "NPC/Tico.hpp"
#include "NPC/NpcStateWait.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(Tico, Wait)
NERVE_DECL(Tico, GoalPose)
NERVES_MAKE_NOSTRUCT(Tico, Wait, GoalPose)
const NpcStateWaitParam sTicoWaitParam("Wait", nullptr, nullptr, "Reaction", "ReactionMic",
                                       "TouchJoy", "Trampled", false, nullptr, false);
const NpcStateTurnParam sTicoTurnParam(0.0f, 0.0f, 2.0f, -1.0f, true, false, 0);
const NpcStateRumbleParam sTicoRumbleParam(30, 2.5f, 1.5f, 0.2f, 1.0f);
} // namespace
/** @brief Creates the Luma NPC. @param pName Actor name. */
Tico::Tico(const char* pName)
    : al::LiveActor(pName), mWaitState(nullptr), mInitialFront(0.0f, 0.0f, 0.0f) {}
/** @brief Releases the Luma actor. */
Tico::~Tico() = default;
/** @brief Initializes the model, waiting state, and search switch. @param rInfo Placement
 * initialization. */
void Tico::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "Tico", nullptr);
    al::initNerve(this, &NrvTicoWait, 1);
    if (al::isClassName(rInfo, "Tico")) {
        al::tryAddDisplayOffset(this, rInfo);
    }
    al::calcFrontDir(&mInitialFront, this);
    mWaitState = new NpcStateWait(this, rInfo, &sTicoWaitParam, &sTicoTurnParam, &sTicoRumbleParam);
    al::initNerveState(this, mWaitState, &NrvTicoWait, "NPC\u5f85\u6a5f");
    al::listenStageSwitchOn(this, "SwitchPlayerSearchInvalidOn",
                            al::Functor(this, &Tico::invalidateTurn));
    makeActorAppeared();
}
/** @brief Disables turning to look for players. */
void Tico::invalidateTurn() { mWaitState->invalidateTurn(); }
/** @brief Forwards contact interactions. @param pSelf Own sensor. @param pOther Contact sensor. */
void Tico::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvTicoGoalPose)) {
        return;
    }
    mWaitState->attackSensor(pSelf, pOther);
}
/** @brief Forwards sensor messages. @param pMessage Message received. @param pOther Sender sensor.
 * @param pSelf Receiver sensor. @return Whether the state accepts the message. */
bool Tico::receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvTicoGoalPose)) {
        return false;
    }
    return mWaitState->receiveMsg(pMessage, pOther, pSelf);
}
/** @brief Forwards pointer interactions. @param pMessage Message received. @param pPointer Input
 * pointer. @param pTarget Point target. @return Whether the state accepts the message. */
bool Tico::receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                                 al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvTicoGoalPose)) {
        return false;
    }
    return mWaitState->receiveMsgScreenPoint(pMessage, pPointer, pTarget);
}
/** @brief Updates interactions and restores the initial facing when searching is disabled. */
void Tico::exeWait() {
    al::updateNerveState(this);
    if (mWaitState->isTurnDisabled()) {
        al::turnToDirection(this, mInitialFront, sTicoTurnParam.mTurnDegrees);
    }
}
/** @brief Plays the Rosalina appearance demo animation and reveals the model after one frame. */
void Tico::exeGoalPose() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "DemoRosettaAppearance");
    }
    if (al::isStep(this, 1)) {
        al::showModelIfHide(this);
    }
}
/** @brief Prepares the actor for its goal pose. */
void Tico::startGoalPose() {
    al::invalidateClipping(this);
    al::faceToDirection(this, mInitialFront);
    al::hideModelIfShow(this);
    al::setNerve(this, &NrvTicoGoalPose);
}
