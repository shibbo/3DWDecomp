#include "Enemy/AnimTest.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(AnimTest, Wait)
NERVES_MAKE_NOSTRUCT(AnimTest, Wait)
}

/** @brief Constructs the animation test actor.
 * @param pName Actor name.
 */
AnimTest::AnimTest(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes the actor from the "AnimTest" archive and starts waiting.
 * @param rInfo Actor placement and scene information.
 */
void AnimTest::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "AnimTest", nullptr);
    al::initNerve(this, &NrvAnimTestWait, 0);
    makeActorAppeared();
}

/** @brief Ignores screen pointer messages.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Targeted screen point.
 * @return Always false.
 */
bool AnimTest::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                     al::ScreenPointTarget* pTarget) {
    return false;
}

/** @brief Wait state; does nothing beyond checking for the first step. */
void AnimTest::exeWait() {
    if (al::isFirstStep(this)) {
    }
}
