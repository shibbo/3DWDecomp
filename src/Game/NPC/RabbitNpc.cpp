#include "Project/Base/StringUtil.hpp"
#include "NPC/RabbitNpc.hpp"
#include "NPC/NpcStateWait.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(RabbitNpc, Wait)
NERVES_MAKE_NOSTRUCT(RabbitNpc, Wait)
const sead::Vector3f sRabbitItemOffset(0.0f, 150.0f, 0.0f);
const sead::Vector3f sRabbitBigItemOffset(0.0f, 300.0f, 0.0f);
const NpcStateWaitParam sRabbitWaitParam("Wait", "WaitTeaseRhythm", nullptr, "Reaction", "Reaction",
                                         nullptr, nullptr, true, &sRabbitItemOffset, true);
const NpcStateWaitParam sRabbitBigWaitParam("Wait", "WaitTease", nullptr, "Reaction", "Reaction",
                                            nullptr, nullptr, true, &sRabbitBigItemOffset, false);
const NpcStateTurnParam sRabbitTurnParam(0.0f, 0.0f, 5.0f, -1.0f, true, true, 0);
const NpcStateRumbleParam sRabbitRumbleParam(30, 2.0f, 1.5f, 0.2f, 1.0f);
const NpcStateRumbleParam sRabbitBigRumbleParam(30, 1.0f, 1.5f, 0.1f, 1.0f);
} // namespace
/** @brief Creates the rabbit NPC. @param pName Actor name. */
RabbitNpc::RabbitNpc(const char* pName) : al::LiveActor(pName), mWaitState(nullptr) {}
/** @brief Releases the rabbit actor. */
RabbitNpc::~RabbitNpc() = default;
/** @brief Initializes rabbit animation and interaction settings. @param rInfo Placement
 * initialization. */
void RabbitNpc::init(const al::ActorInitInfo& rInfo) {
    al::initMapPartsActor(this, rInfo, "Npc", 0);
    al::initNerve(this, &NrvRabbitNpcWait, 1);
    al::StringTmp<256> modelName;
    al::StringTmp<256> modelPath;
    al::makeMapPartsModelName(&modelName, &modelPath, al::getPlacementInfo(rInfo));
    const bool isBig = al::isEqualString(modelName.cstr(), "RabbitBig");
    mWaitState =
        new NpcStateWait(this, rInfo, isBig ? &sRabbitBigWaitParam : &sRabbitWaitParam,
                         &sRabbitTurnParam, isBig ? &sRabbitBigRumbleParam : &sRabbitRumbleParam);
    al::initNerveState(this, mWaitState, &NrvRabbitNpcWait, "NPC\u5f85\u6a5f");
    makeActorAppeared();
}
/** @brief Forwards contact interactions. @param pSelf Own sensor. @param pOther Contact sensor. */
void RabbitNpc::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    mWaitState->attackSensor(pSelf, pOther);
}
/** @brief Forwards sensor messages. @param pMessage Message received. @param pOther Sender sensor.
 * @param pSelf Receiver sensor. @return Whether the state accepts the message. */
bool RabbitNpc::receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                           al::HitSensor* pSelf) {
    return mWaitState->receiveMsg(pMessage, pOther, pSelf);
}
/** @brief Forwards pointer interactions. @param pMessage Message received. @param pPointer Input
 * pointer. @param pTarget Point target. @return Whether the state accepts the message. */
bool RabbitNpc::receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                                      al::ScreenPointTarget* pTarget) {
    return mWaitState->receiveMsgScreenPoint(pMessage, pPointer, pTarget);
}
/** @brief Updates the waiting interaction state. */
void RabbitNpc::exeWait() { al::updateNerveState(this); }
