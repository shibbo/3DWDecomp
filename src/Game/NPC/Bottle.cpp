#include "NPC/Bottle.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/SubActorUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(Bottle, WaitGoalPole)
NERVE_DECL(Bottle, Break)
NERVES_MAKE_NOSTRUCT(Bottle, WaitGoalPole, Break)
} // namespace
/** @brief Creates the fairy bottle. @param pName Actor name. */
Bottle::Bottle(const char* pName) : al::LiveActor(pName) {}
/** @brief Releases the bottle actor. */
Bottle::~Bottle() = default;
/** @brief Initializes the bottle and its inner model. @param rInfo Placement initialization. */
void Bottle::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "BottleOuter", nullptr);
    al::initNerve(this, &NrvBottleWaitGoalPole, 0);
    al::startAction(this, "WaitGoalPole");
    al::startAction(getInnerActor(), "WaitGoalPole");
    makeActorAppeared();
}
/** @brief Gets the bottle's inner model. @return Inner actor. */
al::LiveActor* Bottle::getInnerActor() const { return al::getSubActor(this, "\u5185\u5074"); }
/** @brief Gets the bottle cap. @return Cap actor. */
al::LiveActor* Bottle::getCapActor() const {
    return al::getSubActor(this, "\u30ad\u30e3\u30c3\u30d7");
}
/** @brief Pushes players touching the bottle. @param pSelf Bottle sensor. @param pOther Contact
 * sensor. */
void Bottle::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}
/** @brief Accepts explosive and Bullet Bill attacks. @param pMessage Received message. @param
 * pOther Unused sender sensor. @param pSelf Unused receiver sensor. @return Whether the attack is
 * accepted. */
bool Bottle::receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                        al::HitSensor* pSelf) {
    return al::isMsgExplosion(pMessage) || al::isMsgKillerAttack(pMessage);
}
/** @brief Starts breaking the bottle. */
void Bottle::setBreak() { al::setNerve(this, &NrvBottleBreak); }
/** @brief Keeps the bottle and inner model in the goal-pole waiting animation. */
void Bottle::exeWaitGoalPole() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "WaitGoalPole");
        al::tryStartActionIfNotPlaying(getInnerActor(), "WaitGoalPole");
    }
}
/** @brief Plays the break animation and removes the cap and shadow. */
void Bottle::exeBreak() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Break");
        al::startAction(getInnerActor(), "Break");
        getCapActor()->kill();
        al::invalidateShadow(this);
    }
}
