#include "MapObj/PanelNoteGroup.hpp"
#include "MapObj/PanelNote.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
    NERVE_DECL(PanelNoteGroup, Wait);
    NERVE_DECL(PanelNoteGroup, Complete);
    NERVES_MAKE_NOSTRUCT(PanelNoteGroup, Wait, Complete)
}
PanelNoteGroup::PanelNoteGroup(const char* pName) : al::LiveActor(pName) {}
PanelNoteGroup::~PanelNoteGroup() {}
void PanelNoteGroup::init(const al::ActorInitInfo& rInfo) {
    mPanelCount = al::calcLinkChildNum(rInfo, "PanelNote");
    mPanels = new PanelNote*[mPanelCount];
    mWasOn = new bool[mPanelCount];
    for (int i = 0; i < mPanelCount; ++i) {
        mPanels[i] = new PanelNote("ドレミ床：子供");
        al::initLinksActor(mPanels[i], rInfo, "PanelNote", i);
        mPanels[i]->setGroupControlled(true);
        mWasOn[i] = false;
    }
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTQSV(this);
    al::initActorSRT(this, rInfo);
    al::initActorClipping(this, rInfo);
    al::initGroupClipping(this, rInfo, 128);
    al::initStageSwitch(this, rInfo);
    al::initActorAudioKeeperWithout3D(this, rInfo, "CoinBlowGenerator", nullptr);
    al::initExecutorWatchObj(this, rInfo);
    al::initNerve(this, &NrvPanelNoteGroupWait, 0);
    makeActorAppeared();
}
void PanelNoteGroup::exeWait() {
    if (al::isFirstStep(this))
        al::invalidateClipping(this);
    int count = 0;
    for (int i = 0; i < mPanelCount; ++i) {
        if (mPanels[i]->isOn()) {
            if (!mWasOn[i])
                mLastActivatedIndex = i;
            ++count;
            mWasOn[i] = true;
        }
    }
    if (count == mPanelCount)
        al::setNerve(this, &NrvPanelNoteGroupComplete);
    else
        mLastActivatedIndex = 0;
}
void PanelNoteGroup::exeComplete() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::tryOnStageSwitch(this, "SwitchTrampleOn");
        for (int i = 0; i < mPanelCount; ++i)
            mPanels[i]->setGroupControlled(false);
    }
    kill();
}
