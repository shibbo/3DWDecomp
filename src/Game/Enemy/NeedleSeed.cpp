#include "Enemy/NeedleSeed.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace EnemyStateUtil {
bool isMsgRouteDokanAttack(const al::SensorMsg* pMsg);
}

namespace {
NERVE_DECL(NeedleSeed, Wait)
NERVE_DECL(NeedleSeed, Break)
NERVES_MAKE_NOSTRUCT(NeedleSeed, Wait, Break)
const sead::Vector3f cOneUpItemOffset(0.0f, -50.0f, 0.0f);
}

/** @brief Constructs a spike seed.
 * @param pName Actor name.
 */
NeedleSeed::NeedleSeed(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes the spike seed and its optional item drop.
 * @param rInfo Actor placement and scene information.
 */
void NeedleSeed::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvNeedleSeedWait, 0);
    al::trySetShadowLength(this, rInfo, nullptr);
    mItemType = "NoItem";
    al::tryGetStringArg(&mItemType, rInfo, "ItemType");
    makeActorAppeared();
}

/** @brief Attacks players and enemy bodies while the seed is intact.
 * @param pSelf Attacking sensor belonging to the seed.
 * @param pOther Contacted sensor.
 */
void NeedleSeed::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvNeedleSeedBreak)) {
        return;
    }
    if (al::isSensorEnemyAttack(pSelf)
        && (al::isSensorPlayer(pOther) || al::isSensorEnemyBody(pOther))) {
        al::sendMsgEnemyRouteDokanAttack(pOther, pSelf);
    }
}

/** @brief Handles attacks, awards combo score, and starts breaking the seed.
 * @param pMsg Incoming sensor message.
 * @param pOther Attacker's sensor.
 * @param pSelf Receiving sensor belonging to the seed.
 * @return Whether the message was accepted.
 */
bool NeedleSeed::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                           al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvNeedleSeedBreak)) {
        return false;
    }
    if (al::isMsgExplosion(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg)
        || al::isMsgPlayerGiantHipDrop(pMsg) || EnemyStateUtil::isMsgRouteDokanAttack(pMsg)) {
        if (al::isMsgPlayerInvincibleAttack(pMsg) && !rc::isPlayerBinded(pOther)) {
            return false;
        }
        if (al::isMsgPlayerRouteDokanFireBallAttack(pMsg) || al::isMsgEnemyRouteDokanFire(pMsg)) {
            al::startHitReaction(this, "燃える");
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        } else {
            al::startHitReaction(this, "破壊");
        }
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::setNerve(this, &NrvNeedleSeedBreak);
        if (al::isEqualString(mItemType, "KinokoOneUp")) {
            al::setAppearItemOffset(this, cOneUpItemOffset);
        }
        return true;
    }
    return rc::isMsgRouteDokanPlayerReflect(pMsg);
}

/** @brief Starts the idle animation on entry. */
void NeedleSeed::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
}

/** @brief Plays the break animation, drops the configured item, and removes the seed. */
void NeedleSeed::exeBreak() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Break");
    }
    if (al::isActionEnd(this)) {
        if (!al::isEqualString(mItemType, "NoItem")) {
            al::appearItemTiming(this, mItemType);
        }
        kill();
    }
}

