#include "Library/Obj/PartsFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Boss/KoopaLastBlockPow.hpp"
#include "Boss/KoopaLastBlockPowStateDemo.hpp"
#include "Scene/ProjectActorFactoryTypes.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Project/Base/StringUtil.hpp"
namespace {
NERVE_DECL(KoopaLastBlockPow, Wait);
NERVE_DECL(KoopaLastBlockPow, Demo);
NERVE_DECL(KoopaLastBlockPow, Reaction);
NERVES_MAKE_STRUCT(KoopaLastBlockPow, Wait, Demo, Reaction)
} // namespace
/** @brief Creates the final POW block encounter. @param pName Actor name. */
KoopaLastBlockPow::KoopaLastBlockPow(const char *pName) : LiveActor(pName) {}
/** @brief Destroys the POW block actor. */
KoopaLastBlockPow::~KoopaLastBlockPow() = default;
/** @brief Creates the linked bosses and final-hit cutscene. @param rInfo Initialization data. */
void KoopaLastBlockPow::init(const al::ActorInitInfo &rInfo) {
    al::initActorWithArchiveName(this, rInfo, "BlockPowKoopa", nullptr);
    int count = al::calcLinkChildNum(rInfo, "DemoEndKoopa");
    mWallClimbers.allocBuffer(count, nullptr);
    for (int i = 0; i < count; i++) {
        auto *actor = new KoopaLastWallClimb("真クッパ(壁のぼり)");
        al::initLinksActor(actor, rInfo, "DemoEndKoopa", i);
        mWallClimbers.pushBack(actor);
    }
    if (al::calcLinkChildNum(rInfo, "DemoTargetKoopa") != 1) {
        makeActorDead();
        return;
    }
    auto *defender = new KoopaLastDefendeBlockPow("真クッパ(Powブロック守り)");
    al::initLinksActor(defender, rInfo, "DemoTargetKoopa", 0);
    mDefender = defender;
    mDemoState = new KoopaLastBlockPowStateDemoEnd(this, defender, rInfo);
    al::initNerve(this, &NrvKoopaLastBlockPow.Wait, 1);
    al::initNerveState(this, mDemoState, &NrvKoopaLastBlockPow.Demo, "死亡デモ");
    int userNum = rc::getControlUserNumMax();
    mUserHitTimers.tryAllocBuffer(userNum, nullptr);
    for (int i = 0; i < userNum; i++)
        mUserHitTimers[i] = 0;
    al::calcLinkChildNum(rInfo, "TargetPos");
    al::tryStartMclAnimIfNotPlaying(this, "ReactionCount");
    al::setMclAnimFrameAndStop(this, 0);
    mCollision = al::createCollisionObj(this, rInfo, "BlockPowKoopa3M",
                                        al::getHitSensor(this, "Body"), nullptr, nullptr);
    mCollision->kill();
    al::invalidateHitSensor(this, "3M");
    al::invalidateHitSensor(this, "2M");
    al::invalidateHitSensor(this, "1M");
    makeActorAppeared();
}
/** @brief Handles eligible player attacks and triggers the final hit. @return Whether handled. */
bool KoopaLastBlockPow::receiveMsg(const al::SensorMsg *pMsg, al::HitSensor *pOther,
                                   al::HitSensor *pSelf) {
    if ((!al::isSensorPlayer(pOther) && !al::isMsgPlayerBoomerangBreak(pMsg)) ||
        al::isNerve(this, &NrvKoopaLastBlockPow.Demo))
        return false;
    auto checkHeight = [&](float offset) {
        int count = sead::Mathi::min(mHitCount, 3);
        float otherY = al::getActorTrans(pOther).y;
        float selfY = al::getActorTrans(pSelf).y;
        float height = otherY - selfY;
        if (offset != 0)
            height += offset;
        return height > count * 30.0f - 50.0f;
    };
    if (!(al::isMsgPlayerUpperPunch(pMsg) || (al::isMsgPlayerBodyAttack(pMsg) && checkHeight(50)) ||
          (al::isMsgPlayerBoomerangBreak(pMsg) && checkHeight(-50)) ||
          (al::isMsgPlayerClimbAttack(pMsg) && checkHeight(50)) ||
          (al::isMsgPlayerSpinAttack(pMsg) && checkHeight(50)) ||
          (al::isMsgPlayerTailAttack(pMsg) && checkHeight(0))))
        return false;
    int id = rc::tryFindRelativeControlUserId(pOther);
    if (!al::isMsgPlayerUpperPunch(pMsg) && mHitUsers.isOnBit(id))
        return false;
    ++mHitCount;
    int hits = mHitCount;
    if (hits <= 3) {
        mHitUsers.setBit(id);
        mUserHitTimers[id] = 0;
        al::sendMsgExplosion(al::getHitSensor(mDefender, "PowBlock"), pSelf, nullptr);
        al::setNerve(this, &NrvKoopaLastBlockPow.Reaction);
        return true;
    } else {
        al::startAction(this, "Reaction04");
        al::tryStartMclAnimIfNotPlaying(this, "ReactionCount");
        al::startHitReaction(this, "Pow反応[ラスト]");
        al::setMclAnimFrameAndStop(this, al::getMclAnimFrameMax(this) * .25f * hits);
        al::invalidateHitSensors(this);
        al::invalidateClipping(this);
        al::tryOnStageSwitch(this, "SwitchCameraStartOn");
        al::setNerve(this, &NrvKoopaLastBlockPow.Demo);
        for (int i = 0, count = mWallClimbers.size(); i < count; i++)
            mWallClimbers.at(i)->startLastPowDamage();
        mDefender->startLastPowDamage();
        return true;
    }
}
/** @brief Waits for the next player attack. */
void KoopaLastBlockPow::exeWait() {}
/** @brief Updates the hit reaction, collision size, and player cooldowns. */
void KoopaLastBlockPow::exeReaction() {
    if (al::isFirstStep(this)) {
        int count = sead::Mathi::min(mHitCount, 4);
        al::startAction(this, al::StringTmp<32>("Reaction%02d", count).cstr());
        al::tryStartMclAnimIfNotPlaying(this, "ReactionCount");
        al::setMclAnimFrameAndStop(this, al::getMclAnimFrameMax(this) * .25f * mHitCount);
        al::startHitReaction(this, "Pow反応");
        al::startSeWithParam(this, "PgReaction", float(count), nullptr);
        if (al::isValidCollisionParts(this)) {
            al::invalidateCollisionParts(this);
            mCollision->appear();
        }
        switch (count) {
        case 1:
            al::invalidateHitSensor(this, "4M");
            al::validateHitSensor(this, "3M");
            al::invalidateHitSensor(this, "2M");
            al::invalidateHitSensor(this, "1M");
            break;
        case 2:
            al::invalidateHitSensor(this, "4M");
            al::invalidateHitSensor(this, "3M");
            al::validateHitSensor(this, "2M");
            al::invalidateHitSensor(this, "1M");
            break;
        case 3:
            al::invalidateHitSensor(this, "4M");
            al::invalidateHitSensor(this, "3M");
            al::invalidateHitSensor(this, "2M");
            al::validateHitSensor(this, "1M");
            break;
        case 4:
            al::invalidateHitSensor(this, "4M");
            al::invalidateHitSensor(this, "3M");
            al::invalidateHitSensor(this, "2M");
            al::validateHitSensor(this, "1M");
            break;
        default:
            return;
        }
    }
    for (int i = 0, count = mUserHitTimers.size(); i < count; i++) {
        if (mHitUsers.isOnBit(i)) {
            ++mUserHitTimers[i];
            if (mUserHitTimers[i] >= 30)
                mHitUsers.resetBit(i);
        }
    }
    if (al::isActionEnd(this) && al::isGreaterEqualStep(this, 30))
        al::setNerve(this, &NrvKoopaLastBlockPow.Wait);
}
/** @brief Removes the block and collision after its cutscene. */
void KoopaLastBlockPow::exeDemo() {
    if (al::updateNerveState(this)) {
        kill();
        mCollision->kill();
    }
}
