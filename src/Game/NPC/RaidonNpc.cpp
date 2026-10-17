#include "NPC/RaidonNpc.hpp"
#include "Raidon/RaidonEndState.hpp"
#include "NPC/ActorStateSupportStroke.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
namespace {
NERVE_DECL(RaidonNpc, Wait)
NERVES_MAKE_NOSTRUCT(RaidonNpc, Wait)
} // namespace
/** @brief Creates the non-ridable Plessie actor. @param pName Actor name. */
RaidonNpc::RaidonNpc(const char* pName)
    : RaidonActor(pName, "Npc"), mEndState(nullptr), mStroke(nullptr) {}
/** @brief Releases the NPC actor. */
RaidonNpc::~RaidonNpc() = default;
/** @brief Initializes the model and idle interaction states. @param rInfo Placement initialization.
 */
void RaidonNpc::init(const al::ActorInitInfo& rInfo) {
    RaidonActor::init(rInfo, "Raidon");
    mStroke = new ActorStateSupportStroke(this);
    mStroke->disableFeedback();
    mStroke->appear();
    mEndState = new RaidonEndState("\u7d42\u4e86\u72b6\u614b", this, mStroke);
    mEndState->enableWaitStart();
    al::initNerve(this, &NrvRaidonNpcWait, 1);
    al::initNerveState(this, mEndState, &NrvRaidonNpcWait, "\u5f85\u6a5f\u72b6\u614b");
    makeActorAppeared();
}
/** @brief Forwards contact interactions. @param pSelf Own sensor. @param pOther Contact sensor. */
void RaidonNpc::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    mEndState->attackSensor(pSelf, pOther);
}
/** @brief Forwards sensor messages. @param pMessage Message received. @param pOther Sender sensor.
 * @param pSelf Receiver sensor. @return Whether the state accepts the message. */
bool RaidonNpc::receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                           al::HitSensor* pSelf) {
    return mEndState->receiveMsg(pMessage, pOther, pSelf);
}
/** @brief Forwards pointer interactions. @param pMessage Message received. @param pPointer Input
 * pointer. @param pTarget Point target. @return Whether the state accepts the message. */
bool RaidonNpc::receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                                      al::ScreenPointTarget* pTarget) {
    return mEndState->receiveMsgScreenPoint(pMessage, pPointer, pTarget);
}
/** @brief Updates idle behavior and rewards stroking with an item. */
void RaidonNpc::exeWait() {
    al::updateNerveState(this);
    mStroke->update();
    if (mStroke->shouldAppearItem()) {
        const sead::Vector3f position = al::getTrans(this) + sead::Vector3f(0.0f, 200.0f, 0.0f);
        al::appearItemTiming(this, "\u64ab\u3067\u308b", position, sead::Vector3f::ey);
    }
}
