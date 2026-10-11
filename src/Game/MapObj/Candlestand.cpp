#include "MapObj/Candlestand.hpp"
#include "MapObj/CandlestandWatcher.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Light/PrePassLightKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
    NERVE_DECL(Candlestand, LightOn);
    NERVE_DECL(Candlestand, LightOff);
    NERVE_DECL(Candlestand, LightOnAndShake);
    NERVES_MAKE_NOSTRUCT(Candlestand, LightOn, LightOff, LightOnAndShake)
}
Candlestand::Candlestand(const char* pName, CandlestandWatcher* pWatcher) : al::LiveActor(pName), mWatcher(pWatcher) {}
Candlestand::~Candlestand() {}
void Candlestand::init(const al::ActorInitInfo& rInfo) {
    const char* modelName = "Candlestand";
    alPlacementFunction::tryGetModelName(&modelName, rInfo);
    bool isLightFire = false;
    al::tryGetArg(&isLightFire, rInfo, "IsLightFire");
    if (al::isEqualString(modelName, "Candlestand"))
        al::initActorWithArchiveName(this, rInfo, "Candlestand", nullptr);
    else {
        al::initActorWithArchiveName(this, rInfo, "CandlestandNoStand", nullptr);
        isLightFire = true;
    }
    al::tryGetArg(&mIsEnableLightPrePass, rInfo, "IsEnableLightPrePass");
    if (mIsEnableLightPrePass) LightPrePassFunction::declareUsingPointLight(this, 1);
    else al::killPrePassLightAll(this, -1);
    if (isLightFire) {
        al::isValidStageSwitch(this, "SwitchFireOn");
        al::initNerve(this, &NrvCandlestandLightOn, 0);
        al::emitEffect(this, "Fire", nullptr);
    } else {
        al::invalidateHitSensor(this, "FireAttack");
        al::initNerve(this, &NrvCandlestandLightOff, 0);
    }
    makeActorAppeared();
}
void Candlestand::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvCandlestandLightOff)) return;
    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther) && al::sendMsgEnemyAttack(pOther, pSelf)) return;
    if (al::isSensorEnemyAttack(pSelf) && al::isSensorEnemyBody(pOther)) al::sendMsgEnemyAttackFire(pOther, pSelf);
}
bool Candlestand::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor*) {
    if (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgEnemyAttackFire(pMsg)) {
        if (al::isNerve(this, &NrvCandlestandLightOff)) {
            al::validateHitSensor(this, "FireAttack");
            al::setNerve(this, &NrvCandlestandLightOn);
            al::startHitReaction(this, "点灯");
            al::tryOnStageSwitch(this, "SwitchFireOn");
            if (mWatcher && mWatcher->isEnableAddScore()) rc::addScore(this, pSender, 0.0f, 0);
            return true;
        }
        return false;
    }
    return false;
}
void Candlestand::exeLightOn() {
    if (al::isFirstStep(this)) {
        if (mIsEnableLightPrePass) al::appearPrePassLight(this, "燭台の炎", -1);
        al::startSe(this, "Burning", nullptr);
    }
    if (al::isMicInputOn(this)) {
        al::deleteEffect(this, "Fire");
        al::setNerve(this, &NrvCandlestandLightOnAndShake);
    }
}
void Candlestand::exeLightOnAndShake() {
    if (al::isFirstStep(this)) al::tryEmitEffect(this, "FireShake", nullptr);
    if (al::isGreaterEqualStep(this, 60)) {
        al::tryDeleteEffect(this, "FireShake");
        al::emitEffect(this, "Fire", nullptr);
        al::setNerve(this, &NrvCandlestandLightOn);
    }
}
void Candlestand::exeLightOff() {
    if (al::isFirstStep(this) && mIsEnableLightPrePass) al::killPrePassLight(this, "燭台の炎", -1);
}
bool Candlestand::isLightOff() { return al::isNerve(this, &NrvCandlestandLightOff); }
