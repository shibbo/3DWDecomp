#include "MapObj/PanelNote.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
    NERVE_DECL(PanelNote, Wait);
    NERVE_DECL(PanelNote, OnTrg);
    NERVE_DECL(PanelNote, OnLevel);
    NERVE_DECL(PanelNote, WaitWithGlow);
    NERVES_MAKE_STRUCT(PanelNote, Wait, OnTrg, OnLevel, WaitWithGlow)
    PanelNoteInfo sNoteInfo[] = {
        {0, 0, 2, 1, "赤色"}, {1, 2, 4, 3, "黄色"},
        {2, 4, 6, 5, "緑色"}, {3, 5, 8, 7, "水色"},
        {4, 7, 10, 9, "青色"}, {5, 9, 12, 11, "青紫色"},
        {6, 11, 14, 13, "紫色"}, {7, 12, 2, 1, "赤色"},
        {8, 14, 4, 3, "黄色"}, {9, 16, 6, 5, "緑色"},
        {10, 17, 8, 7, "水色"}, {11, 19, 10, 9, "青色"},
        {12, 21, 12, 11, "青紫色"}, {13, 23, 14, 13, "紫色"}
    };
}
PanelNote::PanelNote(const char* pName) : al::LiveActor(pName) {}
PanelNote::~PanelNote() {}
void PanelNote::init(const al::ActorInitInfo& rInfo) {
    al::initNerve(this, &NrvPanelNote.Wait, 0);
    al::initActorWithArchiveName(this, rInfo, "PanelNote", nullptr);
    al::tryGetArg(&mNoteNo, rInfo, "NoteNo");
    al::tryGetArg(&mNoteType, rInfo, "NoteType");
    const PanelNoteInfo* info = nullptr;
    for (int i = 0; i < 14; ++i) {
        if (sNoteInfo[i].noteNo == mNoteNo) {
            info = &sNoteInfo[i];
            break;
        }
    }
    mNoteInfo = info;
    mConnector = al::tryCreateMtxConnector(this, rInfo);
    al::trySyncStageSwitchAppear(this);
}
void PanelNote::initAfterPlacement() {
    if (mConnector)
        al::attachMtxConnectorToCollision(mConnector, this, false);
}
void PanelNote::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    al::setNerve(this, &NrvPanelNote.Wait);
    al::validateClipping(this);
}
void PanelNote::control() {
    if (mConnector)
        al::connectPoseQT(this, mConnector);
    mIsTouched = false;
}
bool PanelNote::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor*) {
    if (al::isMsgPlayerObjHipDropAll(pMsg) || al::isMsgKickKouraAttackCollide(pMsg) || al::isMsgFireBallFloorTouch(pMsg)) {
        mIsTouched = true;
        mCharacterType = 0;
    }
    if ((al::isMsgPlayerTouch(pMsg) && rc::isPlayerWallSnap(pOther)) || al::isMsgPlayerFloorTouch(pMsg)) {
        if (rc::isPlayerCharaMario(pOther))
            mCharacterType = 0;
        else if (rc::isPlayerCharaLuigi(pOther))
            mCharacterType = 1;
        else if (rc::isPlayerCharaPeach(pOther))
            mCharacterType = 2;
        else if (rc::isPlayerCharaKinopio(pOther))
            mCharacterType = 3;
        else if (rc::isPlayerCharaRosetta(pOther))
            mCharacterType = 2;
        else
            mCharacterType = 0;
    } else if (al::isMsgEnemyFloorTouch(pMsg)) {
        mCharacterType = -1;
    } else if (al::isMsgExplosion(pMsg)) {
        mCharacterType = 0;
    } else {
        return false;
    }
    mIsTouched = true;
    mLastTouchSensor = pOther;
    return true;
}
bool PanelNote::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isMsgTouchAssistNoPat(pMsg)) {
        mCharacterType = 1;
        mIsTouched = true;
        return true;
    }
    return false;
}
bool PanelNote::isOn() const { return !al::isNerve(this, &NrvPanelNote.Wait); }
void PanelNote::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Color");
        al::setMclAnimFrameAndStop(this, mNoteInfo->offColorFrame);
        al::validateClipping(this);
    }
    if (mIsTouched) {
        switch (mNoteType) {
        case 0: case 3: case 4: case 5:
            al::setNerve(this, &NrvPanelNote.OnTrg);
            break;
        case 1: case 2: case 6:
            al::setNerve(this, &NrvPanelNote.OnLevel);
            break;
        }
    }
}
void PanelNote::exeWaitWithGlow() {
    if (!mIsGroupControlled)
        al::setNerve(this, &NrvPanelNote.Wait);
    al::requestPrePassLightColor(this, "ドレミ床ライト", mNoteInfo->lightColor, 1.0f);
}
void PanelNote::exeOnTrg() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Color");
        al::setMclAnimFrameAndStop(this, mNoteInfo->onColorFrame);
    }
    al::requestPrePassLightColor(this, "ドレミ床ライト", mNoteInfo->lightColor, 1.0f);
    if (!mIsTouched) {
        if (mIsGroupControlled)
            al::setNerve(this, &NrvPanelNote.WaitWithGlow);
        else
            al::setNerve(this, &NrvPanelNote.Wait);
    }
}
void PanelNote::exeOnLevel() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Color");
        al::setMclAnimFrameAndStop(this, mNoteInfo->onColorFrame);
    }
    al::requestPrePassLightColor(this, "ドレミ床ライト", mNoteInfo->lightColor, 1.0f);
    if (!mIsTouched) {
        if (mIsGroupControlled)
            al::setNerve(this, &NrvPanelNote.WaitWithGlow);
        else
            al::setNerve(this, &NrvPanelNote.Wait);
    }
}
