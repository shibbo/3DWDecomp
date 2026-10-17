#include "NPC/FairyNpcWithGlasses.hpp"
#include "NPC/NpcStateWait.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/SubActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(FairyNpcWithGlasses, Wait)
NERVE_DECL(FairyNpcWithGlasses, ViewPoint)
const struct {
    FairyNpcWithGlassesNrvWait Wait;
    FairyNpcWithGlassesNrvViewPoint ViewPoint;
} NrvFairyNpcWithGlasses;
const sead::Vector3f sGlassesItemOffset(0.0f, 150.0f, 0.0f);
const NpcStateWaitParam sGlassesWaitParam("Wait", "ViewPoint", nullptr, "Reaction", "ReactionMic",
                                          "TouchJoy", "Trampled", true, &sGlassesItemOffset, false);
const NpcStateWaitParam sGlassesViewParam("Wait", "ViewPoint", nullptr, "ReactionViewPoint",
                                          "ReactionMicViewPoint", "TouchJoyViewPoint",
                                          "TrampledViewPoint", true, &sGlassesItemOffset, false);
const NpcStateTurnParam sGlassesTurnParam(0.00001f, 0.0f, 3.5f, 3000.0f, true, false, 0);
const NpcStateTurnParam sGlassesViewTurnParam(0.0f, 0.0f, 0.0f, 0.0f, false, false, 0);
} // namespace
/** @brief Creates a fairy with binoculars. @param pName Actor name. */
FairyNpcWithGlasses::FairyNpcWithGlasses(const char* pName)
    : al::LiveActor(pName), mWaitState(nullptr), mInitialFront(0.0f, 0.0f, 0.0f), mGlasses(nullptr),
      mTurnComplete(false) {}
/** @brief Releases the fairy actor. */
FairyNpcWithGlasses::~FairyNpcWithGlasses() = default;
/** @brief Initializes the fairy, binoculars, and observation states. @param rInfo Placement
 * initialization. */
void FairyNpcWithGlasses::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "FairyNpc", "WithGlasses");
    mInitialFront = al::getFront(this);
    al::normalize(&mInitialFront);
    al::initNerve(this, &NrvFairyNpcWithGlasses.Wait, 2);
    mWaitState = new NpcStateWait(this, rInfo, &sGlassesWaitParam, &sGlassesTurnParam, nullptr);
    al::initNerveState(this, mWaitState, &NrvFairyNpcWithGlasses.Wait, "NPC\u5f85\u6a5f");
    al::initNerveState(this, mWaitState, &NrvFairyNpcWithGlasses.ViewPoint,
                       "ViewPoint\u773a\u3081\u308b");
    makeActorAppeared();
    al::startAction(this, "Wait");
    mGlasses = al::tryGetSubActor(this, "\u53cc\u773c\u93e1");
    al::startAction(mGlasses, "Wait");
}
/** @brief Forwards contact interactions. @param pSelf Own sensor. @param pOther Contact sensor. */
void FairyNpcWithGlasses::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    mWaitState->attackSensor(pSelf, pOther);
}
/** @brief Forwards sensor messages. @param pMessage Message received. @param pOther Sender sensor.
 * @param pSelf Receiver sensor. @return Whether the state accepts the message. */
bool FairyNpcWithGlasses::receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                                     al::HitSensor* pSelf) {
    return mWaitState->receiveMsg(pMessage, pOther, pSelf);
}
/** @brief Forwards pointer interactions. @param pMessage Message received. @param pPointer Input
 * pointer. @param pTarget Point target. @return Whether the state accepts the message. */
bool FairyNpcWithGlasses::receiveMsgScreenPoint(const al::SensorMsg* pMessage,
                                                al::ScreenPointer* pPointer,
                                                al::ScreenPointTarget* pTarget) {
    return mWaitState->receiveMsgScreenPoint(pMessage, pPointer, pTarget);
}
/** @brief Runs waiting interactions and switches to observation when requested. */
void FairyNpcWithGlasses::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(mGlasses, "Wait");
    }
    al::updateNerveState(this);
    if (mWaitState->isWaitAfter()) {
        mWaitState->setWaitParam(&sGlassesViewParam);
        mWaitState->setTurnParam(&sGlassesViewTurnParam);
        al::setNerve(this, &NrvFairyNpcWithGlasses.ViewPoint);
    }
}
/** @brief Faces the observation direction and runs the binocular interaction phase. */
void FairyNpcWithGlasses::exeViewPoint() {
    if (al::isFirstStep(this)) {
        al::startAction(mGlasses, "ViewPoint");
        al::startSe(this, "PgViewPoint", nullptr);
        mTurnComplete = false;
    }
    if (!mTurnComplete) {
        sead::Vector3f front = al::getFront(this);
        const bool complete = al::turnDirectionDegree(this, &front, mInitialFront, 10.0f);
        al::setFront(this, front);
        if (complete) {
            mTurnComplete = true;
        }
    }
    al::updateNerveState(this);
    if (!mWaitState->isWaitAfter()) {
        mWaitState->setWaitParam(&sGlassesWaitParam);
        mWaitState->setTurnParam(&sGlassesTurnParam);
        al::setNerve(this, &NrvFairyNpcWithGlasses.Wait);
    }
}
