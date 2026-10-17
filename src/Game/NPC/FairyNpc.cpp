#include "NPC/FairyNpc.hpp"
#include "NPC/NpcStateWait.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(FairyNpc, Wait)
NERVES_MAKE_NOSTRUCT(FairyNpc, Wait)
const sead::Vector3f sFairyItemOffset(0.0f, 150.0f, 0.0f);
const NpcStateTurnParam sFairyTurnParam(0.00001f, 0.0f, 3.5f, 3000.0f, true, false, 0);
} // namespace
/** @brief Creates the fairy NPC. @param pName Actor name. */
FairyNpc::FairyNpc(const char* pName)
    : al::LiveActor(pName), mWaitState(nullptr), mWaitParam(nullptr) {}
/** @brief Releases the fairy actor. */
FairyNpc::~FairyNpc() = default;
/** @brief Initializes fairy animation and interaction settings. @param rInfo Placement
 * initialization. */
void FairyNpc::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    bool isDance;
    al::tryGetArg(&isDance, rInfo, "IsDance");
    const char* pWaitAction = isDance ? "DanceStep" : "Wait";
    al::initNerve(this, &NrvFairyNpcWait, 1);
    mWaitParam = new NpcStateWaitParam(pWaitAction, "SeeOff", nullptr, "Reaction", "ReactionMic",
                                       "TouchJoy", "Trampled", true, &sFairyItemOffset, false);
    mWaitState = new NpcStateWait(this, rInfo, mWaitParam, &sFairyTurnParam, nullptr);
    al::initNerveState(this, mWaitState, &NrvFairyNpcWait, "NPC\u5f85\u6a5f");
    makeActorAppeared();
    al::startAction(this, pWaitAction);
}
/** @brief Forwards contact interactions. @param pSelf Own sensor. @param pOther Contact sensor. */
void FairyNpc::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    mWaitState->attackSensor(pSelf, pOther);
}
/** @brief Forwards sensor messages. @param pMessage Message received. @param pOther Sender sensor.
 * @param pSelf Receiver sensor. @return Whether the state accepts the message. */
bool FairyNpc::receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                          al::HitSensor* pSelf) {
    return mWaitState->receiveMsg(pMessage, pOther, pSelf);
}
/** @brief Forwards pointer interactions. @param pMessage Message received. @param pPointer Input
 * pointer. @param pTarget Point target. @return Whether the state accepts the message. */
bool FairyNpc::receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                                     al::ScreenPointTarget* pTarget) {
    return mWaitState->receiveMsgScreenPoint(pMessage, pPointer, pTarget);
}
/** @brief Updates the waiting interaction state. */
void FairyNpc::exeWait() { al::updateNerveState(this); }
