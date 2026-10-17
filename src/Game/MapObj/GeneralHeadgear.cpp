#include "MapObj/GeneralHeadgear.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
NERVE_DECL(GeneralHeadgear, Wait);
NERVE_DECL(GeneralHeadgear, Appear);
NERVE_DECL(GeneralHeadgear, Carry);
NERVE_DECL(GeneralHeadgear, Kill);
NERVES_MAKE_STRUCT(GeneralHeadgear, Wait, Appear, Carry, Kill)
const char* const sHeadgearCharaAnims[] = {"Mario", "Luigi", "Peach", "Kinopio", "Rosetta", "KinopioBrigate", "KinopioBrigate", "KinopioBrigate", "KinopioBrigate"};
}
GeneralHeadgear::GeneralHeadgear(const char* name) : al::LiveActor(name) {}
GeneralHeadgear::~GeneralHeadgear() {}
void GeneralHeadgear::init(const al::ActorInitInfo& info) {
    if (!al::tryGetStringArg(&mArchive, info, "ModelArc")) mArchive = "BlockBrick";
    const char* name = nullptr;
    if (!al::tryGetStringArg(&name, info, "ObjName")) name = "無名汎用被り物";
    mName.format("%s", name);
    mActorName = mName.cstr();
    al::initActorWithArchiveName(this, info, mArchive, nullptr);
    al::initNerve(this, &NrvGeneralHeadgear.Wait, 0);
    if (al::isPlaced(info)) makeActorAppeared();
    else makeActorDead();
    al::offCollide(this);
}
void GeneralHeadgear::appear() {
    al::LiveActor::appear();
    al::validateHitSensors(this);
    al::validateClipping(this);
    if (al::isExistCollisionParts(this)) al::validateCollisionParts(this);
    al::offCollide(this);
    al::setNerve(this, &NrvGeneralHeadgear.Appear);
}
void GeneralHeadgear::attackSensor(al::HitSensor*, al::HitSensor*) {}
bool GeneralHeadgear::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (!al::isSensorPlayer(sender)) return false;
    if (al::isMsgPlayerReleaseEquipmentGoal(msg)) { shiftKill(true); return true; }
    if (al::isMsgPlayerReleaseEquipment(msg)) { shiftKill(false); return true; }
    if (al::isNerve(this, &NrvGeneralHeadgear.Wait)) {
        if (al::isMsgPlayerObjUpperPunch(msg)) return tryShiftCarry(sender, receiver);
        if (al::isMsgPlayerFloorTouch(msg)) mFloorTouched = true;
    } else if (al::isNerve(this, &NrvGeneralHeadgear.Appear) && al::isMsgPlayerObjUpperPunch(msg)) return tryShiftCarry(sender, receiver);
    return false;
}
void GeneralHeadgear::shiftKill(bool goal) {
    sead::Vector3f velocity;
    al::calcFrontDir(&velocity, al::getSensorHost(mPlayerSensor));
    float up = goal ? 10.0f : 15.0f;
    float back = goal ? -5.0f : -4.0f;
    velocity *= back;
    velocity.y = up;
    al::getVelocityPtr(this)->set(velocity);
    mPlayerSensor = nullptr;
    al::tryStartAction(this, "Blow");
    al::setNerve(this, &NrvGeneralHeadgear.Kill);
}
bool GeneralHeadgear::tryShiftCarry(al::HitSensor* sender, al::HitSensor* receiver) {
    if (!rc::tryPlayerEquipHeadgear(sender, receiver, nullptr, 0)) return false;
    mPlayerSensor = sender;
    al::invalidateClipping(this);
    al::invalidateHitSensors(this);
    if (al::isExistCollisionParts(this)) al::invalidateCollisionParts(this);
    al::setNerve(this, &NrvGeneralHeadgear.Carry);
    return true;
}
void GeneralHeadgear::exeAppear() {
    if (al::isFirstStep(this)) {
        if (mAppearHipDrop) mActionExists = al::tryStartAction(this, "AppearHipDrop");
        else mActionExists = al::tryStartAction(this, "Appear");
    }
    if (mActionExists ? al::isActionEnd(this) : al::isGreaterEqualStep(this, 30)) al::setNerve(this, &NrvGeneralHeadgear.Wait);
}
void GeneralHeadgear::exeWait() {
    if (al::isFirstStep(this)) { mActionExists = al::tryStartAction(this, "WaitItem"); mFloorTouched = false; }
    if (mFloorTouched) {
        if (!mActionExists || !al::isActionPlaying(this, "WaitItemStop")) mActionExists = al::tryStartAction(this, "WaitItemStop");
    } else {
        if (!mActionExists || !al::isActionPlaying(this, "WaitItem")) mActionExists = al::tryStartAction(this, "WaitItem");
    }
    mFloorTouched = false;
}
void GeneralHeadgear::exeCarry() {
    if (al::isFirstStep(this)) {
        mMini = false;
        mActionExists = al::tryStartAction(this, rc::isPlayerMini(mPlayerSensor) ? "AttachMini" : "Attach");
        startCharaMtpAnim();
    }
    if (!mActionExists || (al::isActionOneTime(this, al::getActionName(this)) && al::isActionEnd(this))) {
        if (rc::isPlayerMini(mPlayerSensor)) { mActionExists = al::tryStartAction(this, "WaitMini"); mMini = true; }
        else mActionExists = al::tryStartAction(this, "Wait");
    }
    if (mMini && !rc::isPlayerMini(mPlayerSensor)) { mActionExists = al::tryStartAction(this, "Wait"); mMini = false; }
    al::updatePoseMtx(this, rc::getPlayerModelJointMtxPtr(mPlayerSensor, "JointRoot"));
    if (rc::isPlayerDamageTrigOn(mPlayerSensor)) rc::removePlayerEquipHeadgear(mPlayerSensor, false);
}
void GeneralHeadgear::startCharaMtpAnim() {
    if (rc::isPlayerChara(mPlayerSensor, EPlayerChara::Mario)) al::tryStartMtpAnimIfExist(this, sHeadgearCharaAnims[EPlayerChara(EPlayerChara::Mario)]);
    if (rc::isPlayerChara(mPlayerSensor, EPlayerChara::Luigi)) al::tryStartMtpAnimIfExist(this, sHeadgearCharaAnims[EPlayerChara(EPlayerChara::Luigi)]);
    if (rc::isPlayerChara(mPlayerSensor, EPlayerChara::Peach)) al::tryStartMtpAnimIfExist(this, sHeadgearCharaAnims[EPlayerChara(EPlayerChara::Peach)]);
    if (rc::isPlayerChara(mPlayerSensor, EPlayerChara::Kinopio)) al::tryStartMtpAnimIfExist(this, sHeadgearCharaAnims[EPlayerChara(EPlayerChara::Kinopio)]);
    if (rc::isPlayerChara(mPlayerSensor, EPlayerChara::Rosetta)) al::tryStartMtpAnimIfExist(this, sHeadgearCharaAnims[EPlayerChara(EPlayerChara::Rosetta)]);
    if (rc::isPlayerChara(mPlayerSensor, EPlayerChara::KinopioBrigade)) al::tryStartMtpAnimIfExist(this, sHeadgearCharaAnims[EPlayerChara(EPlayerChara::KinopioBrigade)]);
    if (rc::isPlayerChara(mPlayerSensor, EPlayerChara::KinopioBrigadeMember)) al::tryStartMtpAnimIfExist(this, sHeadgearCharaAnims[EPlayerChara(EPlayerChara::KinopioBrigadeMember)]);
    if (rc::isPlayerChara(mPlayerSensor, EPlayerChara::KinopioBrigadeMember1)) al::tryStartMtpAnimIfExist(this, sHeadgearCharaAnims[EPlayerChara(EPlayerChara::KinopioBrigadeMember1)]);
    if (rc::isPlayerChara(mPlayerSensor, EPlayerChara::KinopioBrigadeMember2)) al::tryStartMtpAnimIfExist(this, sHeadgearCharaAnims[EPlayerChara(EPlayerChara::KinopioBrigadeMember2)]);
}
void GeneralHeadgear::exeKill() {
    if (al::isFirstStep(this)) al::onCollide(this);
    *al::getVelocityPtr(this) += sead::Vector3f(0.0f, -1.0f, 0.0f);
    if (al::getVelocity(this).y < -25.0f) al::getVelocityPtr(this)->y = -25.0f;
    if (al::isExistActorCollider(this)) {
        if (al::isCollidedCeilingVelocity(this)) al::getVelocityPtr(this)->y = 0.0f;
        if (al::isCollidedWallVelocity(this)) { al::getVelocityPtr(this)->x = 0.0f; al::getVelocityPtr(this)->z = 0.0f; }
        if (al::isCollidedGround(this) && al::getVelocity(this).y < 0.0f) al::getVelocityPtr(this)->set(0.0f, 0.0f, 0.0f);
    }
    if (al::isGreaterEqualStep(this, 10)) kill();
}
