#include "MapObj/PanelNotePaintGroup.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Audio/AudioGeneralPurposeAreaChecker.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/PanelNotePaint.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
NERVE_DECL(PanelNotePaintGroup, Wait)
NERVE_DECL(PanelNotePaintGroup, End)
NERVE_DECL(PanelNotePaintGroup, Complete)
NERVES_MAKE_NOSTRUCT(PanelNotePaintGroup, Wait, End, Complete)
} // namespace
static void changePanelBgmPhase(al::LiveActor* actor, int phase) {
    switch (phase) {
    case 0:
        al::changeBgmSituation(actor, "PaintDoremiPhase0");
        break;
    case 1:
        al::changeBgmSituation(actor, "PaintDoremiPhase1");
        break;
    case 2:
        al::changeBgmSituation(actor, "PaintDoremiPhase2");
        break;
    case 3:
        al::changeBgmSituation(actor, "PaintDoremiPhase3");
        break;
    case 4:
        al::changeBgmSituation(actor, "PaintDoremiPhase4");
        break;
    }
}
PanelNotePaintGroup::PanelNotePaintGroup(const char* name) : al::LiveActor(name) {
    mPositions = new PosInfo*[32];
    for (int i = 0; i < 32; ++i)
        mPositions[i] = new PosInfo;
}
void PanelNotePaintGroup::init(const al::ActorInitInfo& info) {
    mPanelCount = al::calcLinkChildNum(info, "PanelNotePaint");
    mPanels = new PanelNotePaint*[mPanelCount];
    mWasOn = new bool[mPanelCount];
    mEffectStarted = new bool[mPanelCount];
    for (int i = 0; i < mPanelCount; ++i) {
        mPanels[i] = new PanelNotePaint("ドレミ床：子供");
        al::initLinksActor(mPanels[i], info, "PanelNotePaint", i);
        mWasOn[i] = false;
        mEffectStarted[i] = false;
    }
    al::initActorWithArchiveName(this, info, "PanelNotePaintWatcher", nullptr);
    al::initActorAudioKeeperWithout3D(this, info, "PanelNotePaintGroup", nullptr);
    al::tryGetArg(&mSeType, info, "SeType");
    al::setEffectFollowMtxPtr(this, "Complete", &mEffectMtx);
    al::invalidateClipping(this);
    al::initNerve(this, &NrvPanelNotePaintGroupWait, 0);
    makeActorAppeared();
    mPitch = mPanelCount > 90 ? -12 : 0;
    mAlternateOctaves = mPanelCount > 50;
    mAreaChecker = new al::AudioGeneralPurposeAreaChecker("PanelNotePaintRoomCheckArea");
    mAreaChecker->init(getAreaObjDirector());
    mAreaChecker->setPlayerHolder(getSceneInfo()->playerHolder);
}
void PanelNotePaintGroup::control() {
    mAreaChecker->update(-1);
    if (mAreaChecker->isEnter()) {
        if (al::isNerve(this, &NrvPanelNotePaintGroupEnd)) {
            if (mSeType == 1)
                al::startBgm(this, "PaintDoremiRoomCompleteZelda", -1, 0, -1, -1);
            else
                al::startBgm(this, "PaintDoremiRoomComplete", -1, 0, -1, -1);
            al::changeBgmSituation(this, "SkipIntro");
        } else {
            al::startBgm(this, "PaintDoremiRoom", -1, 0, -1, -1);
            changePanelBgmPhase(this, mBgmPhase);
        }
        if (!mBgmStarted)
            mBgmStarted = true;
        al::disableBgmChangeArea(this);
    } else if (mAreaChecker->isExit()) {
        if (mSeType == 1)
            al::stopBgm(this, "PaintDoremiRoomCompleteZelda", -1, -1);
        else
            al::stopBgm(this, "PaintDoremiRoomComplete", -1, -1);
        al::enableBgmChangeArea(this);
    }
}
void PanelNotePaintGroup::exeWait() {
    if (al::isFirstStep(this))
        al::invalidateClipping(this);
    int count = 0;
    for (int i = 0; i < mPanelCount; ++i)
        count += mPanels[i]->isOn();
    bool complete = count == mPanelCount;
    for (int i = 0; i < mPanelCount; ++i) {
        if (mPanels[i]->isOn()) {
            if (!mWasOn[i]) {
                if (mPanels[i]->isInitiallyOn())
                    mWasOn[i] = true;
                else {
                    if (mLastActivatedIndex < 0)
                        al::startSeWithParam(mPanels[i], "PgOn", float(mPitch), nullptr);
                    else
                        al::startSeWithParam(mPanels[i], "PgOn", float(mPitch + mPitchOffset),
                                             nullptr);
                    mPanels[i]->setPitch(mPitch + mPitchOffset);
                    updatePitch();
                    mLastActivatedIndex = i;
                    mWasOn[i] = true;
                }
            }
        } else
            mPanels[i]->setPitch(mPitch + mPitchOffset);
    }
    float percent = float(count) / float(mPanelCount) * 100.0f;
    int phase;
    if (percent < 20.0f)
        phase = 0;
    else if (percent < 50.0f)
        phase = 1;
    else if (percent < 75.0f)
        phase = 2;
    else if (percent < 85.0f)
        phase = 3;
    else
        phase = 4;
    if (mBgmPhase < phase) {
        mBgmPhase = phase;
        changePanelBgmPhase(this, phase);
    }
    if (complete)
        al::setNerve(this, &NrvPanelNotePaintGroupComplete);
}
void PanelNotePaintGroup::updatePitch() {
    if (mAlternateOctaves) {
        if (mPitchOffset >= 1) {
            if (mPitch > 38)
                mPitch -= 5;
            else
                ++mPitch;
            mPitchOffset = 0;
        } else
            mPitchOffset = 12;
    } else {
        mPitchOffset = 0;
        ++mPitch;
    }
}
void PanelNotePaintGroup::exeComplete() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        if (mSeType == 1) {
            if (mAreaChecker->isInArea()) {
                al::stopBgm(this, "PaintDoremiRoom", -1, -1);
                al::startBgm(this, "PaintDoremiRoomCompleteZelda", -1, 0, -1, -1);
            }
        } else {
            al::startSe(this, "PgComplete", nullptr);
            if (mAreaChecker->isInArea()) {
                al::stopBgm(this, "PaintDoremiRoom", -1, -1);
                al::startBgm(this, "PaintDoremiRoomComplete", -1, 0, -1, -1);
            }
        }
        al::tryOnStageSwitch(this, "SwitchTrampleOn");
        al::emitEffect(this, "Complete", nullptr);
        mCompleteRadius = 0.0f;
        if (mPanels[mLastActivatedIndex]->getScoreSensor())
            rc::addScore(this, mPanels[mLastActivatedIndex]->getScoreSensor(),
                         al::getTrans(mPanels[mLastActivatedIndex]), 0);
    }
    if (al::isStep(this, 2))
        al::startSe(this, "PgCompleteEffect", nullptr);
    sead::Vector3f origin = al::getTrans(mPanels[mLastActivatedIndex]);
    bool complete = true;
    for (int i = 0; i < mPanelCount; ++i) {
        if (!mEffectStarted[i]) {
            sead::Vector3f position = al::getTrans(mPanels[i]);
            if ((position - origin).length() < mCompleteRadius) {
                mEffectStarted[i] = true;
                mPanels[i]->setCompleteEffect();
            }
            complete = false;
        }
    }
    if (complete)
        al::setNerve(this, &NrvPanelNotePaintGroupEnd);
    mCompleteRadius += 30.0f;
}
void PanelNotePaintGroup::exeEnd() {}
sead::Vector3f* PanelNotePaintGroup::findPosInfo(int id) {
    for (int i = 0; i < 32; ++i)
        if (mPositions[i]->id == id)
            return mPositions[i]->pos;
    return nullptr;
}
void PanelNotePaintGroup::setPosInfo(int id, sead::Vector3f* pos) {
    for (int i = 0; i < 32; ++i) {
        if (mPositions[i]->id < 0) {
            mPositions[i]->pos = pos;
            mPositions[i]->id = id;
            return;
        }
    }
}
