#include "NPC/NekoBindPuppeteer.hpp"

#include <attributes.h>
#include <math/seadVector.h>

#include "MapObj/PuppetStickRouteSelecter.hpp"
#include "NPC/NekoNormal.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Player.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"

namespace {

NERVE_DECL(NekoBindPuppeteer, Wait)
NERVE_DECL(NekoBindPuppeteer, Exit)

/**
 * @brief Nerve that throws the player out of the cat but keeps the cat in place; shares exeExit.
 */
class NekoBindPuppeteerNrvExitStay : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<NekoBindPuppeteer>()->exeExit();
    }
};

NERVE_DECL(NekoBindPuppeteer, DizzyStart)
NERVE_DECL(NekoBindPuppeteer, Hide)
NERVE_DECL(NekoBindPuppeteer, End)
NERVE_DECL(NekoBindPuppeteer, DizzyLoop)
NERVE_DECL(NekoBindPuppeteer, DizzyEnd)

NERVES_MAKE_NOSTRUCT(NekoBindPuppeteer, DizzyStart, Hide, DizzyLoop, DizzyEnd)

// Non-const nerve objects: the game merges them into one block with the end parameters below.
NekoBindPuppeteerNrvWait NrvNekoBindPuppeteerWait;
NekoBindPuppeteerNrvExit NrvNekoBindPuppeteerExit;
NekoBindPuppeteerNrvExitStay NrvNekoBindPuppeteerExitStay;
NekoBindPuppeteerNrvEnd NrvNekoBindPuppeteerEnd;

/// End parameter of a binding the cat ends by itself.
PlayerBindEndParam sEndParam = {{}, 0, 20, true, false, true, 0, -1.0f, 0};
/// End parameter of a player thrown out of the cat.
PlayerBindEndParam sExitEndParam = {{}, 0, 20, true, false, true, 0, -1.0f, 0};
/// End parameter of a player thrown out of a cat that stays in place.
PlayerBindEndParam sExitStayEndParam = {{}, 0, 20, true, false, true, 0, -1.0f, 0};

/// Length of the arrow that checks for a ceiling above the player when it leaves the cat.
const f32 cCeilingCheckLength = 300.0f;
/// Height the player gets lifted by when leaving a moving cat.
const f32 cExitLiftHeight = 100.0f;
/// Height the player gets lifted by when leaving a cat that stays in place.
const f32 cExitStayLiftHeight = 100.0f;

/**
 * @brief Check for a ceiling right above the player.
 * @param pPlayer The player.
 * @return Whether there is a ceiling above the player.
 */
ALWAYS_INLINE bool isCeilingAbove(al::LiveActor* pPlayer) {
    sead::Vector3f pos = al::getTrans(pPlayer);
    sead::Vector3f dir = {0.0f, cCeilingCheckLength, 0.0f};
    sead::Vector3f hitPos;
    return alCollisionUtil::getFirstPolyOnArrow(pPlayer, &hitPos, nullptr, pos, dir, nullptr,
                                                nullptr);
}

/**
 * @brief Let a dizzy player fall down until it stands on the floor.
 * @param pPuppeteer Puppeteer that binds the player.
 */
NOINLINE void updateDizzyFall(NekoBindPuppeteer* pPuppeteer) {
    IUsePlayerPuppet* puppet = pPuppeteer->getPlayerPuppet();

    if (rc::isOnFloorPuppet(puppet) && rc::getPuppetVelocity(puppet).y <= 0.0f) {
        rc::setPuppetVelocity(puppet, sead::Vector3f::zero);
        return;
    }

    sead::Vector3f velocity = rc::getPuppetVelocity(puppet);
    velocity.y += -1.0f;
    rc::setPuppetVelocity(puppet, velocity);
    rc::solveAirPuppet(puppet);
}

}  // namespace

/**
 * @brief Construct the puppeteer.
 * @param pNeko Cat the player gets bound to.
 */
NekoBindPuppeteer::NekoBindPuppeteer(NekoNormal* pNeko)
    : BindPuppeteer("ネコバインド操作"), mNeko(pNeko) {
    initNerve(&NrvNekoBindPuppeteerWait, 0);
    mRouteSelecter = new PuppetStickRouteSelecter(1);
}

/**
 * @brief Check whether a sensor belongs to the bound player.
 * @param pSensor Sensor to check.
 * @return Whether the host of the sensor is the bound player.
 */
bool NekoBindPuppeteer::isTargetSensor(al::HitSensor* pSensor) {
    if (mPlayerSensor == nullptr) {
        return false;
    }

    return al::getSensorHost(pSensor) == al::getSensorHost(mPlayerSensor);
}

/**
 * @brief Start binding a player to the cat.
 * @param pSensor Sensor of the player.
 * @return Always false, the binding starts through the bind messages instead.
 */
bool NekoBindPuppeteer::startEnter(al::HitSensor* pSensor) {
    return false;
}

/**
 * @brief Throw the bound player out of the cat.
 */
void NekoBindPuppeteer::startExit() {
    if (isBind()) {
        al::setNerve(this, &NrvNekoBindPuppeteerExit);
    }
}

/**
 * @brief Throw the bound player out of a cat that stays in place.
 */
void NekoBindPuppeteer::startExitStay() {
    if (isBind()) {
        al::setNerve(this, &NrvNekoBindPuppeteerExitStay);
    }
}

/**
 * @brief Throw the bound player out of the cat dizzy.
 */
void NekoBindPuppeteer::startDizzy() {
    if (isBind()) {
        al::setNerve(this, &NrvNekoBindPuppeteerDizzyStart);
    }
}

/**
 * @brief Stop binding, release the bound player if there is one.
 */
void NekoBindPuppeteer::stopBind() {
    if (isBind()) {
        endNekoBind();
        return;
    }

    mPlayerSensor = nullptr;
    al::setNerve(this, &NrvNekoBindPuppeteerWait);
}

/**
 * @brief Make the bound player visible and controllable again.
 */
inline void NekoBindPuppeteer::restorePuppet() {
    rc::setPlayerColorAnimDefault(mNeko, "Color");
    rc::validatePuppetSensors(getPlayerPuppet());
    rc::setPuppetVelocity(getPlayerPuppet(), sead::Vector3f::zero);
    rc::showPuppet(getPlayerPuppet());
    rc::showPuppetSilhouette(getPlayerPuppet());
}

/**
 * @brief Release the bound player where it is.
 */
void NekoBindPuppeteer::endNekoBind() {
    restorePuppet();
    mRouteSelecter->clearPuppet(getPlayerPuppet());
    endBind(&sEndParam);
    mPlayerSensor = nullptr;
    al::setNerve(this, &NrvNekoBindPuppeteerWait);
}

/**
 * @brief Release the bound player where it is, whatever it is doing.
 */
void NekoBindPuppeteer::endBindForce() {
    restorePuppet();
    mRouteSelecter->clearPuppet(getPlayerPuppet());
    endBind(&sEndParam);
    mPlayerSensor = nullptr;
    al::setNerve(this, &NrvNekoBindPuppeteerWait);
}

/**
 * @brief Release the bound player because it fell into the abyss.
 */
void NekoBindPuppeteer::endBindForceAbyss() {
    restorePuppet();
    mRouteSelecter->clearPuppet(getPlayerPuppet());
    BindPuppeteer::endBindForceAbyss();
    mPlayerSensor = nullptr;
    al::setNerve(this, &NrvNekoBindPuppeteerWait);
}

/**
 * @brief Update the puppeteer.
 */
void NekoBindPuppeteer::update() {
    updateNerve();
}

/**
 * @return Route selecter that steers the cat with the stick of the bound player.
 */
PuppetStickRouteSelecter* NekoBindPuppeteer::getRouteSelecter() const {
    return mRouteSelecter;
}

/**
 * @brief Handle the bind messages of the player.
 * @param pMsg Received message.
 * @param pOther Sensor of the player.
 * @param pSelf Sensor of the cat.
 * @return Whether the message was handled.
 */
bool NekoBindPuppeteer::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                   al::HitSensor* pSelf) {
    if (al::isMsgBindStart(pMsg)) {
        return true;
    }

    if (al::isMsgBindInit(pMsg)) {
        if (rc::isPlayerHolding(pOther, mNeko)) {
            rc::requestPlayerRelease(pOther);
        }

        if (rc::isPlayerEquipHeadgear(pOther)) {
            rc::removePlayerEquipHeadgear(pOther, false);
        }

        al::setNerve(this, mEnterNerve);
        startBind(pOther, pSelf);
        mRouteSelecter->addPuppet(getPlayerPuppet());
        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        mNeko->receivedBindCancel(this, pOther);
        mRouteSelecter->clearPuppet(getPlayerPuppet());
        restorePuppet();
        cancelBind();
        mPlayerSensor = nullptr;
        al::setNerve(this, &NrvNekoBindPuppeteerWait);
        return true;
    }

    if (al::isMsgBindDamage(pMsg)) {
        rc::damagePuppet(getPlayerPuppet());
        return true;
    }

    return false;
}

/**
 * @brief Wait for a player to bind.
 */
void NekoBindPuppeteer::exeWait() {}

/**
 * @brief Wait a moment after the binding started.
 */
void NekoBindPuppeteer::exeBindWait() {
    al::isGreaterStep(this, 10);
}

/**
 * @brief Let the player jump into the cat.
 */
void NekoBindPuppeteer::exeEnter() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "NekoIn");
        rc::invalidatePuppetSensors(getPlayerPuppet());
    }

    if (rc::isPuppetActionEnd(getPlayerPuppet())) {
        rc::hidePuppet(getPlayerPuppet());
        rc::hidePuppetSilhouette(getPlayerPuppet());
        rc::moveSimplePuppet(getPlayerPuppet());
        al::setNerve(this, &NrvNekoBindPuppeteerHide);
    }
}

/**
 * @brief Carry the hidden player along with the cat.
 */
void NekoBindPuppeteer::exeHide() {
    if (al::isFirstStep(this)) {
        rc::setPlayerColorAnimBySensor(mNeko, mPlayerSensor, "Color");
    }

    rc::setPuppetTrans(getPlayerPuppet(), al::getTrans(mNeko));

    sead::Vector3f front = al::getVelocity(mNeko);
    front.y = 0.0f;

    if (!al::isNearZero(front)) {
        al::normalize(&front);
        rc::setPuppetFrontVec(getPlayerPuppet(), front);
    }
}

/**
 * @brief Throw the player out of the cat.
 */
void NekoBindPuppeteer::exeExit() {
    restorePuppet();

    bool isHitCeiling = isCeilingAbove(al::getSensorHost(mPlayerSensor));
    f32 liftHeight;

    if (isHitCeiling) {
        liftHeight = 0.0f;
    } else {
        liftHeight = al::isNerve(this, &NrvNekoBindPuppeteerExit) ? cExitLiftHeight :
                                                                     cExitStayLiftHeight;
    }

    sead::Vector3f front = rc::getPuppetFrontVec(getPlayerPuppet());
    front.y = 0.0f;
    al::normalizeOrDirZ(&front);

    sead::Vector3f trans = rc::getPuppetTrans(getPlayerPuppet());
    sead::Vector3f offset = front * 10.0f;
    front.x *= -4.5f;
    front.z *= -4.5f;
    trans += offset;
    trans.y += liftHeight;
    front.y = isHitCeiling ? 5.0f : 18.0f;
    rc::setPuppetTrans(getPlayerPuppet(), trans);
    rc::setPuppetVelocity(getPlayerPuppet(), front);
    mRouteSelecter->clearPuppet(getPlayerPuppet());

    if (isHitCeiling) {
        endBindSquat();
    } else if (al::isNerve(this, &NrvNekoBindPuppeteerExitStay)) {
        endBind(&sExitStayEndParam);
    } else {
        endBind(&sExitEndParam);
    }

    mPlayerSensor = nullptr;
    al::setNerve(this, &NrvNekoBindPuppeteerEnd);
}

/**
 * @brief Release the dizzy player when it is somewhere deadly.
 * @return Whether the binding ended.
 */
inline bool NekoBindPuppeteer::tryEndDizzy() {
    auto* player = static_cast<PlayerActor*>(al::getSensorHost(mPlayerSensor));

    if (player != nullptr) {
        const IUsePlayerCollision* collision = player->getPlayer()->getCollision();

        if (rc::isInDeathArea(player)) {
            endBindForceAbyss();
            return true;
        }

        if (PlayerActionFunc::checkMapCode(collision, "DamageFire") ||
            PlayerActionFunc::checkMapCode(collision, "Poison")) {
            endBindForce();
            return true;
        }
    }

    return false;
}

/**
 * @brief Throw the player out of the cat dizzy.
 */
void NekoBindPuppeteer::exeDizzyStart() {
    if (al::isFirstStep(this)) {
        restorePuppet();

        bool isHitCeiling = isCeilingAbove(al::getSensorHost(mPlayerSensor));

        sead::Vector3f front = rc::getPuppetFrontVec(getPlayerPuppet());
        front.y = 0.0f;
        al::normalizeOrDirZ(&front);

        sead::Vector3f trans = rc::getPuppetTrans(getPlayerPuppet());
        sead::Vector3f offset = front * 10.0f;
        front.x *= -4.5f;
        front.z *= -4.5f;
        trans += offset;
        trans.y += isHitCeiling ? 0.0f : cExitLiftHeight;
        front.y = isHitCeiling ? 5.0f : 18.0f;
        rc::setPuppetTrans(getPlayerPuppet(), trans);
        rc::setPuppetVelocity(getPlayerPuppet(), front);
        rc::startPuppetAction(getPlayerPuppet(), "NekoDizzyStart");
    }

    updateDizzyFall(this);

    if (tryEndDizzy()) {
        return;
    }

    if (rc::isPuppetActionEnd(getPlayerPuppet())) {
        al::setNerve(this, &NrvNekoBindPuppeteerDizzyLoop);
    }
}

/**
 * @brief Keep the thrown out player dizzy.
 */
void NekoBindPuppeteer::exeDizzyLoop() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "NekoDizzyLoop");
    }

    updateDizzyFall(this);

    if (tryEndDizzy()) {
        return;
    }

    if (rc::isPuppetActionEnd(getPlayerPuppet())) {
        al::setNerve(this, &NrvNekoBindPuppeteerDizzyEnd);
    }
}

/**
 * @brief Let the player recover from being dizzy and release it.
 */
void NekoBindPuppeteer::exeDizzyEnd() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "NekoDizzyEnd");
    }

    updateDizzyFall(this);

    if (tryEndDizzy()) {
        return;
    }

    if (rc::isPuppetActionEnd(getPlayerPuppet())) {
        mRouteSelecter->clearPuppet(getPlayerPuppet());

        IUsePlayerPuppet* puppet = getPlayerPuppet();
        rc::endBindOnGroundAndPuppetNull(&puppet);
        mPlayerSensor = nullptr;
        setNullPlayerPuppet();
        al::setNerve(this, &NrvNekoBindPuppeteerEnd);
    }
}
