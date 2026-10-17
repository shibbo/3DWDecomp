#include "Boss/BossGorobonRock.hpp"
#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(BossGorobonRock, Shot)
NERVE_DECL(BossGorobonRock, Brake)
NERVES_MAKE_NOSTRUCT(BossGorobonRock, Shot, Brake)
}  // namespace

/**
 * @brief Creates a rock with no roll axis or break model yet.
 * @param pName Actor name.
 */
BossGorobonRock::BossGorobonRock(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model and the break model, then waits dead until thrown.
 * @param rInfo Actor initialization information.
 */
void BossGorobonRock::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "BossGorobonRock", nullptr);
    al::initNerve(this, &NrvBossGorobonRockShot, 0);
    mBreakModel = new al::BreakModel(this, "ボスゴロボン岩壊れモデル", "GorobonBreak", nullptr,
                                     nullptr, "Break", true);
    al::initCreateActorNoPlacementInfo(mBreakModel, rInfo);
    al::offCollide(this);
    makeActorDead();
}

/** @brief Appears in the rolling state with the appearance reaction. */
void BossGorobonRock::appear() {
    al::setNerve(this, &NrvBossGorobonRockShot);
    al::startHitReaction(this, "出現");
    makeActorAppeared();
}

/**
 * @brief Hits enemies with the boss attack message and damages players while rolling.
 * @param pSelf Sensor of the rock.
 * @param pOther Sensor that was touched.
 */
void BossGorobonRock::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvBossGorobonRockBrake)) {
        return;
    }

    if (al::isSensorEnemy(pOther) && rc::sendMsgBossGorobonAttack(pOther, pSelf)) {
        return;
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        al::sendMsgEnemyAttack(pOther, pSelf);
    }
}

/**
 * @brief Breaks the rock when another Gorobon attack hits it.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the rock.
 * @return Always false.
 */
bool BossGorobonRock::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                 al::HitSensor* pSelf) {
    if (!al::isNerve(this, &NrvBossGorobonRockBrake) && rc::isMsgGorobonAttack(pMsg)) {
        al::setNerve(this, &NrvBossGorobonRockBrake);
    }

    return false;
}

/** @brief Rolls under gravity and breaks on hitting the ground, a wall, a ceiling or timing out. */
void BossGorobonRock::exeShot() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::onCollide(this);
        al::calcSideDir(&mSideDir, this);
    }

    al::holdSe(this, "PgShotLv");
    al::addVelocityToGravity(this, 0.4f);
    f32 speed = al::calcSpeed(this);
    sead::Quatf quat = al::getQuat(this);
    al::rotateQuatRadian(&quat, quat, mSideDir, sead::Mathf::deg2rad(speed * -0.3f));
    al::setQuat(this, quat);
    if ((al::isCollidedGround(this) && al::isGreaterEqualStep(this, 10)) ||
        al::isCollidedWall(this) || al::isCollidedCeiling(this) ||
        al::isGreaterEqualStep(this, 600)) {
        if (al::isCollidedGround(this)) {
            al::startHitReaction(this, "地面衝突");
        }

        al::setNerve(this, &NrvBossGorobonRockBrake);
    }
}

/** @brief Swaps the model for the break model and dies once the pieces are gone. */
void BossGorobonRock::exeBrake() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "PgBreak");
        al::setVelocityZero(this);
        al::setQuat(this, sead::Quatf::unit);
        al::appearBreakModelRandomRotateY(mBreakModel);
        al::hideModel(this);
        al::tryDeleteEffect(this, "Body");
    }

    if (al::isDead(mBreakModel)) {
        al::showModel(this);
        kill();
    }
}
