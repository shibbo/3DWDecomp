#include "MapObj/Lantern.hpp"
#include "MapObj/LanternRing.hpp"
#include "MapObj/LanternSwingMovement.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Audio/AudioSystem.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include <prim/seadSafeString.h>
#include "Project/Base/StringUtil.hpp"
namespace {
NERVE_DECL(Lantern, Wait)
NERVE_DECL(Lantern, Swing)
NERVE_DECL(Lantern, SwingLerp)
NERVES_MAKE_NOSTRUCT(Lantern, Wait, Swing, SwingLerp)
const s32 sRingCounts[] = {6, 3, 10, 20};
struct SwingParam { s32 duration; float amplitude; s32 period; };
const SwingParam sSwingParams[] = {{120, 20.0f, 40}, {180, 30.0f, 55}};
void startSwingSe(al::LiveActor* actor, int duration) {
    s16 value = s16(float(duration) / 180.0f * 120.0f);
    al::startSeSetSeqLoacalVariableByName(actor, "プログラム", 0, value);
}
bool isSwingMessage(const al::SensorMsg* msg) {
    return al::isMsgBallAttack(msg) || al::isMsgBallTrample(msg) || al::isMsgExplosion(msg) ||
        al::isMsgKickKouraAttack(msg) || al::isMsgPlayerBoomerangAttack(msg) ||
        al::isMsgPlayerCooperationHipDrop(msg) || al::isMsgPlayerItemGet(msg) ||
        al::isMsgPlayerSpinAttack(msg) || al::isMsgPlayerTailAttack(msg);
}
}
Lantern::Lantern(const char* name) : al::LiveActor(name), mSwing(new LanternSwingMovement) {}
Lantern::~Lantern() = default;
void Lantern::init(const al::ActorInitInfo& info) {
    al::initMapPartsActor(this, info, nullptr, 0);
    al::initNerve(this, &NrvLanternWait, 0);
    mInitialQuat = al::getQuat(this);
    mInitialTrans = al::getTrans(this);
    int length = 0;
    if (al::tryGetArg(&length, info, "RingLength")) mRingCount = sRingCounts[length];
    sead::Vector3f down(0.0f, 0.0f, 0.0f);
    al::calcUpDir(&down, this);
    down.negate();
    mRings = new LanternRing*[mRingCount];
    for (int i = 0; i < mRingCount; ++i) {
        mRings[i] = new LanternRing("ランタンリング");
        al::initCreateActorWithPlacementInfo(mRings[i], info);
        al::setTrans(mRings[i], down * (float(i) * 33.0f) + mInitialTrans);
    }
    al::setTrans(this, down * (float(mRingCount - 1) * 33.0f + 18.5f) + mInitialTrans);
    al::StringTmp<256> modelName, archiveName;
    al::makeMapPartsModelName(&modelName, &archiveName, info);
    if (al::isEqualString("LanternA", modelName.cstr())) {
        float radius = al::getPrePassPointLightRadius(this, "ランタンの光源");
        al::setClippingInfo(this, radius, al::getTransPtr(this));
    } else {
        int useShadow = 0;
        al::tryGetArg(&useShadow, info, "UseShadow");
        if (useShadow) al::setPrePassSpotLightShadow(this, "ランタンの光源", true, true, 0.0f);
    }
    al::tryGetArg(&mCanAppearItem, info, "IsEnableAppearStrokeItem");
    makeActorAppeared();
}
void Lantern::control() {
    if ((al::isNerve(this, &NrvLanternSwing) || al::isNerve(this, &NrvLanternSwingLerp)) && al::isLessEqualStep(this, 75)) return;
    if (al::isMicInputOn(this)) {
        sead::Vector3f dir = al::getTrans(this) - al::getCameraPos(this);
        dir.y = 0.0f;
        if (!al::normalizeOrZero(&dir)) tryStartSwing(0, dir);
    }
}
bool Lantern::tryStartSwing(int type, const sead::Vector3f& dir) {
    sead::Vector3f axis = dir.cross(sead::Vector3f::ey);
    if (al::normalizeOrZero(&axis)) return false;
    mSwingAxis.set(axis);
    const auto& param = sSwingParams[type];
    mSwing->setParam(param.duration, param.amplitude, param.period);
    startSwingSe(this, param.duration);
    if (al::isNerve(this, &NrvLanternWait)) al::setNerve(this, &NrvLanternSwing);
    else al::setNerve(this, &NrvLanternSwingLerp);
    if (mCanAppearItem) {
        al::appearItem(this, al::getTrans(this) + sead::Vector3f(0.0f, 50.0f, 0.0f), dir);
        mCanAppearItem = false;
    }
    return true;
}
void Lantern::attackSensor(al::HitSensor* self, al::HitSensor* other) {
    if ((al::isNerve(this, &NrvLanternSwing) || al::isNerve(this, &NrvLanternSwingLerp)) && al::isLessEqualStep(this, 90)) return;
    if (al::isSensorPlayer(other) && al::isSensorName(self, "Push")) al::sendMsgPush(other, self);
}
bool Lantern::receiveMsg(const al::SensorMsg* msg, al::HitSensor* other, al::HitSensor* self) {
    if (al::isMsgPlayerCarryFront(msg) || al::isMsgPlayerCarryUp(msg)) return false;
    if ((al::isNerve(this, &NrvLanternSwing) || al::isNerve(this, &NrvLanternSwingLerp)) && al::isLessEqualStep(this, 75)) return false;
    if (!isSwingMessage(msg)) return false;
    sead::Vector3f dir(0.0f, 0.0f, 0.0f);
    bool fallback = false;
    if (al::isMsgExplosion(msg)) al::calcDirBetweenSensorsH(&dir, other, self);
    else {
        dir.set(al::getActorVelocity(other));
        al::verticalizeVec(&dir, sead::Vector3f::ey, dir);
        al::normalizeOrZero(&dir);
        if (al::isNearZero(dir, 0.001f)) {
            al::calcDirBetweenSensorsH(&dir, other, self);
            fallback = true;
        }
    }
    if (al::isNearZero(dir, 0.001f)) return false;
    if (fallback) {
        if (!tryStartSwing(EnemyStateUtil::isMsgBlowDown(msg), dir)) return false;
    } else if (!tryStartSwing(1, dir)) return false;
    if (al::isMsgBallAttack(msg) || al::isMsgPlayerItemGet(msg)) return false;
    return isSwingMessage(msg);
}
bool Lantern::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget* target) {
    if (al::isMsgTouchAssist(msg) || al::isMsgTouchAssistTrig(msg)) {
        if ((al::isNerve(this, &NrvLanternSwing) || al::isNerve(this, &NrvLanternSwingLerp)) && al::isLessEqualStep(this, 75)) return false;
        sead::Vector3f dir = target->getPos();
        dir -= al::getHitScreenPointTargetPos(pointer);
        al::normalizeOrZero(&dir);
        if (al::isNearZero(dir, 0.001f)) return false;
        return tryStartSwing(1, dir);
    }
    return false;
}
void Lantern::exeWait() {}
void Lantern::exeSwing() {
    if (al::isFirstStep(this)) {
        mSwing->startSwing();
        al::startSe(this, "Attack", nullptr);
    }
    mSwing->updateNerve();
    if (mSwing->isEnd()) { al::setNerve(this, &NrvLanternWait); return; }
    updateSwingMovement();
}
void Lantern::updateSwingMovement() {
    al::rotateQuatRadian(al::getQuatPtr(this), mInitialQuat, mSwingAxis, sead::Mathf::deg2rad(mSwing->getCurrentRotate()));
    sead::Vector3f down(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < mRingCount; ++i) {
        float angle = mSwing->getCurrentRotate() * float(i + 1) / float(mRingCount + 1);
        sead::Quatf quat = sead::Quatf::unit;
        al::rotateQuatRadian(&quat, mInitialQuat, mSwingAxis, sead::Mathf::deg2rad(angle));
        al::calcQuatUp(&down, quat);
        down.negate();
        al::setTrans(mRings[i], down * (float(i) * 33.0f) + mInitialTrans);
    }
    al::calcUpDir(&down, this);
    down.negate();
    al::setTrans(this, down * (float(mRingCount - 1) * 33.0f + 18.5f) + mInitialTrans);
    for (int i = 0; i < mRingCount; ++i) {
        sead::Vector3f dir(0.0f, 0.0f, 0.0f);
        if (i == mRingCount - 1) dir.set(al::getTrans(this) - al::getTrans(mRings[i]));
        else dir.set(al::getTrans(mRings[i + 1]) - al::getTrans(mRings[i]));
        dir.negate();
        al::turnQuatYDirRadian(al::getQuatPtr(mRings[i]), mInitialQuat, dir, sead::Mathf::pi());
    }
    al::setQuat(this, al::getQuat(mRings[mRingCount - 1]));
}
void Lantern::exeSwingLerp() {
    if (al::isFirstStep(this)) {
        mSwing->startSwing();
        mPreviousTrans.set(al::getTrans(this));
        mPreviousQuat.set(al::getQuat(this));
        for (int i = 0; i < mRingCount; ++i) {
            mRings[i]->setPrevTrans();
            mRings[i]->setPrevQuat();
        }
        al::startSe(this, "Attack", nullptr);
    }
    mSwing->updateNerve();
    if (mSwing->isEnd()) { al::setNerve(this, &NrvLanternWait); return; }
    updateSwingMovement();
    if (al::isLessEqualStep(this, 10)) {
        float rate = al::calcNerveRate(this, 10);
        al::lerpVec(al::getTransPtr(this), mPreviousTrans, al::getTrans(this), rate);
        al::slerpQuat(al::getQuatPtr(this), mPreviousQuat, al::getQuat(this), rate);
        (al::getTrans(this) - mInitialTrans).length();
        for (int i = 0; i < mRingCount; ++i) {
            al::lerpVec(al::getTransPtr(mRings[i]), mRings[i]->getPrevTrans(), al::getTrans(mRings[i]), rate);
            al::slerpQuat(al::getQuatPtr(mRings[i]), mRings[i]->getPrevQuat(), al::getQuat(mRings[i]), rate);
        }
    }
}
