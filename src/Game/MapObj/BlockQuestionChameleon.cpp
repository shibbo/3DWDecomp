#include "MapObj/BlockQuestionChameleon.hpp"
#include "MapObj/ChameleonStateGiantPlayer.hpp"
#include "MapObj/ChameleonStateTouch.hpp"
#include "MapObj/ChameleonStateMic.hpp"
#include "MapObj/ChameleonStateHipDrop.hpp"
#include "MapObj/ChameleonStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include <math/seadBoundBox.h>
namespace {
    const RenderMaterialIndirectParam sTouchParam = {0.05f, 0.8f, {1, 1, 1, 1}, {1, 0.6f, 0, 0.25f}, {1, 1, 1, 1}};
    const RenderMaterialIndirectParam sWaitParam = {0.06f, 0.0f, {1, 1, 1, 1}, {1, 1, 0, 0}, {0, 0, 0, 1}};
    const RenderMaterialIndirectParam sHipDropParam = {0.05f, 0.8f, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}};
    const RenderMaterialIndirectParam sGiantPlayerParam = {0.05f, 0.8f, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}};
    const RenderMaterialIndirectParam sMicParam = {0.3f, 5.0f, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}};
    NERVE_DECL(BlockQuestionChameleon, Wait);
    NERVE_DECL(BlockQuestionChameleon, GiantPlayer);
    NERVE_DECL(BlockQuestionChameleon, DRCTouch);
    NERVE_DECL(BlockQuestionChameleon, MicReaction);
    NERVE_DECL(BlockQuestionChameleon, HipDropReaction);
    NERVES_MAKE_STRUCT(BlockQuestionChameleon, Wait, GiantPlayer, DRCTouch, MicReaction, HipDropReaction)
}
BlockQuestionChameleon::BlockQuestionChameleon() : al::LiveActor("ハテナブロックカメレオン") {}
BlockQuestionChameleon::~BlockQuestionChameleon() {}
void BlockQuestionChameleon::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info,
        al::isObjectName(info, "BlockTransparent") ? "BlockQuestionChameleon" : "BlockQuestionLongChameleon",
        al::isSingleMode(info) ? "SM" : nullptr);
    al::setMaterialProgrammable(this);
    mIsLong = !al::isObjectName(info, "BlockTransparent");
    mParam = new RenderMaterialIndirectParam;
    al::initNerve(this, &NrvBlockQuestionChameleon.Wait, 5);
    mGiantPlayer = new ChameleonStateGiantPlayer(this, mParam, &sGiantPlayerParam, &sWaitParam, 50.0f, 1000.0f);
    mTouch = new ChameleonStateTouch(this, mParam, &sTouchParam, &sWaitParam, 50.0f, 300.0f);
    mMic = new ChameleonStateMic(this, mParam, &sMicParam);
    mHipDrop = new ChameleonStateHipDrop(this, mParam, &sHipDropParam, &sWaitParam);
    al::initNerveState(this, mGiantPlayer, &NrvBlockQuestionChameleon.GiantPlayer, "[state]カメレオン巨大プレイヤー反応");
    al::initNerveState(this, mTouch, &NrvBlockQuestionChameleon.DRCTouch, "[state]カメレオンDRCタッチ反応");
    al::initNerveState(this, mMic, &NrvBlockQuestionChameleon.MicReaction, "[state]カメレオンマイク反応");
    al::initNerveState(this, mHipDrop, &NrvBlockQuestionChameleon.HipDropReaction, "[state]カメレオンヒップドロップ反応");
    makeActorAppeared();
}
void BlockQuestionChameleon::control() {
    if (al::isExistShadow(this, "シャドウマスク"))
        al::setShadowIntensityUser(this, mParam->mIntensity * 255.0f, "シャドウマスク");
    if (mParam->mIntensity < 0.001f) al::hideModelIfShow(this);
    else al::showModelIfHide(this);
    ChameleonStateUtil::setRenderMaterialIndirectParam(this, mParam);
    if (mTouchSoundDelay - 1 >= 0) --mTouchSoundDelay;
}
bool BlockQuestionChameleon::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgStrokeTransparent(msg)) {
        if (mTouchSoundDelay == 0) al::startSe(this, "TouchTrg", nullptr);
        mTouchSoundDelay = 2;
        return true;
    }
    if (al::isSensorName(receiver, "HipDropAppear")) {
        if (ChameleonStateUtil::tryRequestHipDropAppearChameleon(msg, sender, mHipDrop)) {
            al::setNerve(this, &NrvBlockQuestionChameleon.HipDropReaction);
            return true;
        }
        if (ChameleonStateUtil::tryRequestGiantPlayerAppearChameleon(msg, sender, mGiantPlayer)) {
            al::setNerve(this, &NrvBlockQuestionChameleon.GiantPlayer);
            return true;
        }
    }
    if (al::isSensorName(receiver, "AttackAppear") && ChameleonStateUtil::isMsgHitAppearChameleon(msg)) {
        if (al::isMsgExplosion(msg) || rc::isMsgKillerShockWave(msg)) {
            sead::Vector3f max(50.0f, 50.0f, 50.0f);
            sead::Vector3f min(-50.0f, -50.0f, -50.0f);
            if (mIsLong) { max.x = 150.0f; min.x = -150.0f; }
            sead::BoundBox3f box(min, max);
            sead::Matrix34f mtx = *getBaseMtx();
            mtx.setTranslation(al::getSensorPos(al::getHitSensor(this, "AttackAppear")));
            if (!al::isHitBoxSensor(sender, mtx, box)) return false;
        }
        mHipDrop->resetAppearDelay();
        al::setNerve(this, &NrvBlockQuestionChameleon.HipDropReaction);
    }
    return false;
}
bool BlockQuestionChameleon::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isMsgTouchAssistAll(msg) && al::isNerve(this, &NrvBlockQuestionChameleon.Wait))
        al::setNerve(this, &NrvBlockQuestionChameleon.DRCTouch);
    return false;
}
void BlockQuestionChameleon::respawn() { if (al::isDead(this)) makeActorAppeared(); }
void BlockQuestionChameleon::exeWait() {
    if (al::isMicInputOn(this)) {
        al::setNerve(this, &NrvBlockQuestionChameleon.MicReaction);
        return;
    }
    ChameleonStateUtil::updateIndirectParam(mParam, &sWaitParam);
}
void BlockQuestionChameleon::exeGiantPlayer() { al::updateNerveStateAndNextNerve(this, &NrvBlockQuestionChameleon.Wait); }
void BlockQuestionChameleon::exeDRCTouch() { al::updateNerveStateAndNextNerve(this, &NrvBlockQuestionChameleon.Wait); }
void BlockQuestionChameleon::exeMicReaction() { al::updateNerveStateAndNextNerve(this, &NrvBlockQuestionChameleon.Wait); }
void BlockQuestionChameleon::exeHipDropReaction() { al::updateNerveStateAndNextNerve(this, &NrvBlockQuestionChameleon.Wait); }
