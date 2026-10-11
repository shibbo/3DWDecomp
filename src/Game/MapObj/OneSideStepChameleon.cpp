#include "MapObj/OneSideStepChameleon.hpp"
#include "MapObj/ChameleonStateHipDrop.hpp"
#include "MapObj/ChameleonStateTouch.hpp"
#include "MapObj/ChameleonStateMic.hpp"
#include "MapObj/ChameleonStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include <math/seadMathCalcCommon.h>
namespace {
const RenderMaterialIndirectParam sStepTouchParam = {0.05f, 0.8f, {1, 1, 1, 1}, {1, 0.6f, 0, 0.25f}, {1, 1, 1, 1}};
const RenderMaterialIndirectParam sStepWaitParam = {0.06f, 0, {1, 1, 1, 1}, {1, 1, 0, 0}, {0, 0, 0, 1}};
const RenderMaterialIndirectParam sStepHipDropParam = {0.05f, 0.8f, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}};
const RenderMaterialIndirectParam sStepMicParam = {0.3f, 5, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}};
NERVE_DECL(OneSideStepChameleon, Wait);
NERVE_DECL(OneSideStepChameleon, Reaction);
NERVE_DECL(OneSideStepChameleon, Touch);
NERVE_DECL(OneSideStepChameleon, MicReaction);
NERVE_DECL(OneSideStepChameleon, Appear);
NERVES_MAKE_STRUCT(OneSideStepChameleon, Appear, Wait, Reaction, Touch, MicReaction)

}
OneSideStepChameleon::OneSideStepChameleon(const char* name) : al::LiveActor(name) {}
OneSideStepChameleon::~OneSideStepChameleon() {}
void OneSideStepChameleon::init(const al::ActorInitInfo& info) {
    const char* model;
    alPlacementFunction::tryGetModelName(&model, info);
    const char* archive;
    if (al::isEqualString(model, "OneSideStepChameleon4x4M")) archive = "OneSideStepChameleon4x4M";
    else if (al::isEqualString(model, "OneSideStepChameleon6x6M")) archive = "OneSideStepChameleon6x6M";
    else { kill(); return; }
    al::initActorWithArchiveName(this, info, archive, nullptr);
    al::setMaterialProgrammable(this);
    mParam = new RenderMaterialIndirectParam{sStepWaitParam.mBlendRate, sStepWaitParam.mIntensity, sStepWaitParam.mVector0, sStepWaitParam.mVector1, sStepWaitParam.mVector2};
    ChameleonStateUtil::setRenderMaterialIndirectParam(this, mParam);
    al::setShadowIntensityUser(this, 0, "Body");
    al::initNerve(this, &NrvOneSideStepChameleon.Wait, 3);
    mHipDrop = new ChameleonStateHipDrop(this, mParam, &sStepHipDropParam, &sStepWaitParam);
    mTouch = new ChameleonStateTouch(this, mParam, &sStepTouchParam, &sStepWaitParam, 50.0f, 300.0f);
    mMic = new ChameleonStateMic(this, mParam, &sStepMicParam);
    al::initNerveState(this, mHipDrop, &NrvOneSideStepChameleon.Reaction, "[state]カメレオンヒップドロップ反応");
    al::initNerveState(this, mTouch, &NrvOneSideStepChameleon.Touch, "[state]カメレオンDRCタッチ反応");
    al::initNerveState(this, mMic, &NrvOneSideStepChameleon.MicReaction, "[state]カメレオンマイク反応");
    makeActorAppeared();
}
void OneSideStepChameleon::control() {
    al::LiveActor* player = rc::tryFindNearestActivePlayerOrKoopaJrActorInSphere(this, 1500.0f);
    if (player) {
        sead::Vector3f pos = al::getTrans(this);
        const sead::Vector3f playerPos = al::getTrans(player);
        sead::Vector3f offset;
        offset.set(pos.x - playerPos.x, pos.y - playerPos.y, pos.z - playerPos.z);
        float rate = 1.0f - sead::Mathf::clamp(offset.length() / 1000.0f, 0.0f, 1.0f);
        rate = rate * rate * sead::Mathf::clamp(al::getVelocity(player).length() / 12.0f, 0.0f, 1.0f);
        float intensity = rate * 2.0f + (1.0f - rate) * 0.0f;
        float alpha = rate + (1.0f - rate);
        ChameleonStateUtil::updateIndirectParam(mParam, 0.06f, intensity, alpha);
        if (!al::isNerve(this, &NrvOneSideStepChameleon.Appear)) {
            sead::Vector4f color = rate * sead::Vector4f(0.038f, 0.063f, 0.1f, 1) + (1.0f - rate) * sead::Vector4f(0, 0, 0, 1);
            RenderMaterialIndirectParam* param = mParam;
            param->mVector2.x = color.x;
            param->mVector2.y = color.y;
            param->mVector2.z = color.z;
            param->mVector2.w = color.w;
        }
    } else {
        ChameleonStateUtil::updateIndirectParam(mParam, 0.06f, 0.0f, 1.0f);
        mParam->mVector2 = sStepWaitParam.mVector2;
    }
    ChameleonStateUtil::setRenderMaterialIndirectParam(this, mParam);
    if (al::isExistShadow(this)) al::setShadowIntensityUser(this, sead::Mathf::clamp(mParam->mIntensity * 160.0f, 0.0f, 255.0f), "Body");
    if (mParam->mIntensity < 0.004f) al::hideModelIfShow(this);
    else al::showModelIfHide(this);
    if (mTouchSoundDelay - 1 >= 0) --mTouchSoundDelay;
}
bool OneSideStepChameleon::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgPlayerDisregard(msg)) return true;
    if (al::isSensorName(receiver, "HipDropAppear") && ChameleonStateUtil::tryRequestHipDropAppearChameleon(msg, sender, mHipDrop)) {
        al::setNerve(this, &NrvOneSideStepChameleon.Reaction);
        return true;
    }
    if (al::isSensorName(receiver, "AttackAppear") &&
        (al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerBoomerangAttack(msg) || al::isMsgBallAttack(msg) ||
         al::isMsgBallTrample(msg) || al::isMsgKickKouraAttack(msg) || al::isMsgExplosion(msg) || al::isMsgPlayerSpinAttack(msg))) {
        mHipDrop->resetAppearDelay();
        al::setNerve(this, &NrvOneSideStepChameleon.Reaction);
    }
    return false;
}
bool OneSideStepChameleon::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isMsgTouchAssistNoPat(msg)) {
        if (al::isNerve(this, &NrvOneSideStepChameleon.Wait)) al::setNerve(this, &NrvOneSideStepChameleon.Touch);
        return true;
    }
    return false;
}
void OneSideStepChameleon::exeWait() {
    if (al::isMicInputOn(this)) {
        float distance = 0.0f;
        al::tryFindNearestPlayerDisatanceFromTarget(&distance, this, al::getTrans(this));
        if (distance < 4000.0f) al::setNerve(this, &NrvOneSideStepChameleon.MicReaction);
    }
}
void OneSideStepChameleon::exeAppear() {
    float rate = float(al::getNerveStep(this)) / 30.0f;
    mParam->mVector0 = rate * sead::Vector4f(1, 1, 1, 1) + (1.0f - rate) * sead::Vector4f(0.577f, 0.739f, 1, 0);
    ChameleonStateUtil::setRenderMaterialIndirectParam(this, mParam);
    if (al::isGreaterEqualStep(this, 30)) al::setNerve(this, &NrvOneSideStepChameleon.Wait);
}
void OneSideStepChameleon::exeReaction() { al::updateNerveStateAndNextNerve(this, &NrvOneSideStepChameleon.Wait); }
void OneSideStepChameleon::exeTouch() {
    if (al::isFirstStep(this) && !al::isHideModel(this)) al::startSe(this, "TouchTrg");
    al::updateNerveStateAndNextNerve(this, &NrvOneSideStepChameleon.Wait);
}
void OneSideStepChameleon::exeMicReaction() { al::updateNerveStateAndNextNerve(this, &NrvOneSideStepChameleon.Wait); }
void OneSideStepChameleon::appearChameleon() {
    mParam->mVector0.set(0.577f, 0.739f, 1, 0);
    ChameleonStateUtil::setRenderMaterialIndirectParam(this, mParam);
    al::LiveActor::appear();
    al::setNerve(this, &NrvOneSideStepChameleon.Appear);
}
