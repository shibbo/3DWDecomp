#include "MapObj/WarpCubeLockedPiece.hpp"
#include "Layout/CounterWarpCube.hpp"
#include "MapObj/ItemStateAssistRotate.hpp"
#include "MapObj/CoinRotater.hpp"
#include "Util/CoinUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
namespace {
NERVE_DECL(WarpCubeLockedPiece, Wait);
NERVE_DECL(WarpCubeLockedPiece, AssistRotate);
NERVE_DECL(WarpCubeLockedPiece, AppearRise);
NERVE_DECL(WarpCubeLockedPiece, AppearFall);
NERVES_MAKE_NOSTRUCT(WarpCubeLockedPiece, AppearFall)
NERVES_MAKE_STRUCT(WarpCubeLockedPiece, Wait, AssistRotate, AppearRise)
}
WarpCubeLockedPiece::WarpCubeLockedPiece(const char* name) : al::LiveActor(name) {}
WarpCubeLockedPiece::~WarpCubeLockedPiece() {}
void WarpCubeLockedPiece::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "WarpCubeLockedPiece", nullptr);
    al::initNerve(this, &NrvWarpCubeLockedPiece.Wait, 1);
    al::tryAddDisplayOffset(this, info);
    mBaseQuat.set(al::getQuat(this));
    mBaseTrans.set(al::getTrans(this));
    mAssistRotate = new ItemStateAssistRotate(this, CoinUtil::getCoinAssistRotateParam());
    al::initNerveState(this, mAssistRotate, &NrvWarpCubeLockedPiece.AssistRotate, "スピン");
    float shadowLength = -1.0f;
    if (al::tryGetArg(&shadowLength, info, "ShadowLength") && shadowLength > 0.0f)
        al::setShadowDropLength(this, shadowLength, "WarpCubeLockedPiece");
    if (al::trySyncStageSwitchAppear(this)) {
        bool rise = false;
        if (al::tryGetArg(&rise, info, "IsAppearRise") && rise) {
            al::setNerve(this, &NrvWarpCubeLockedPiece.AppearRise);
            mIsAppearRise = true;
        }
    }
}
void WarpCubeLockedPiece::appear() {
    al::LiveActor::appear();
    if (al::isNerve(this, &NrvWarpCubeLockedPiece.AppearRise)) {
        al::invalidateClipping(this);
        al::invalidateHitSensors(this);
    }
}
void WarpCubeLockedPiece::kill() {
    al::LiveActor::kill();
    al::tryOnStageSwitch(this, "SwitchGetOn");
}
void WarpCubeLockedPiece::control() {
    if (al::isNerve(this, &NrvWarpCubeLockedPiece.Wait))
        CoinUtil::tryStartSpinCoinIfMicInputOn(this, &NrvWarpCubeLockedPiece.AssistRotate);
}
bool WarpCubeLockedPiece::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (!al::isAlive(this)) return false;
    if (al::isMsgItemGetAll(msg)) {
        mCounter->addCount(this, sender);
        al::startHitReactionGet(this);
        kill();
        return true;
    }
    return false;
}
bool WarpCubeLockedPiece::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isMsgTouchAssistNoPat(msg)) {
        if (al::isNerve(this, &NrvWarpCubeLockedPiece.Wait))
            al::setNerve(this, &NrvWarpCubeLockedPiece.AssistRotate);
        return true;
    }
    return false;
}
void WarpCubeLockedPiece::rotate(float speed) {
    if (!mIsAppearRise) {
        al::rotateQuatYDirDegree(this, mBaseQuat, rc::getCoinRotateY(this));
        return;
    }
    mRotateY = al::wrapAngle(mRotateY + speed);
    al::rotateQuatYDirDegree(this, mBaseQuat, mRotateY);
}
void WarpCubeLockedPiece::exeAppearRise() {
    if (al::isFirstStep(this)) {
        al::emitEffect(this, "Appear", nullptr);
        al::startSe(this, "PgAppearRise", nullptr);
    }
    rotate(20.0f);
    float y = al::calcNerveEaseOutValue(this, 24, mBaseTrans.y, mBaseTrans.y + 200.0f);
    al::getTransPtr(this)->y = y;
    al::setNerveAtStep(this, &NrvWarpCubeLockedPieceAppearFall, 24);
}
void WarpCubeLockedPiece::exeAppearFall() {
    if (al::isFirstStep(this)) al::validateHitSensors(this);
    rotate(al::calcNerveValue(this, 24, 20.0f, rc::getCoinRotateYByFrame(this)));
    float y = al::calcNerveEaseInValue(this, 24, mBaseTrans.y + 200.0f, mBaseTrans.y);
    al::getTransPtr(this)->y = y;
    if (al::isGreaterEqualStep(this, 24)) {
        al::validateClipping(this);
        al::setNerve(this, &NrvWarpCubeLockedPiece.Wait);
    }
}
void WarpCubeLockedPiece::exeWait() {
    if (al::isFirstStep(this) && !al::isEffectEmitting(this, "Twinkle"))
        al::emitEffect(this, "Twinkle", nullptr);
    rotate(rc::getCoinRotateYByFrame(this));
}
void WarpCubeLockedPiece::exeAssistRotate() {
    al::holdSe(this, "PgRotate", nullptr);
    al::updateNerveStateAndNextNerve(this, &NrvWarpCubeLockedPiece.Wait);
}
