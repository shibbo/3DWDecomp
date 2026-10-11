#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class MtxConnector; }
struct PanelNoteInfo {
    int noteNo;
    int pitch;
    int onColorFrame;
    int offColorFrame;
    const char* lightColor;
};
class PanelNote : public al::LiveActor {
public:
    PanelNote(const char*);
    ~PanelNote() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void makeActorAppeared() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    bool isOn() const;
    void setGroupControlled(bool value) { mIsGroupControlled = value; }
    void exeWait();
    void exeWaitWithGlow();
    void exeOnTrg();
    void exeOnLevel();
private:
    bool mIsTouched = false;
    const PanelNoteInfo* mNoteInfo = nullptr;
    int mNoteType = 1;
    int mCharacterType = 0;
    al::MtxConnector* mConnector = nullptr;
    int mNoteNo = -1;
    int mUnknown164 = -1;
    bool mIsGroupControlled = false;
    al::HitSensor* mLastTouchSensor = nullptr;
};
