#pragma once
#include "Library/LiveActor/LiveActor.hpp"
struct RenderMaterialIndirectParam;
class ChameleonStateTouch;
class ChameleonStateHipDrop;
class ChameleonStateMic;
class OneSideStepChameleon : public al::LiveActor {
public:
    explicit OneSideStepChameleon(const char* name);
    ~OneSideStepChameleon() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exeWait();
    void exeAppear();
    void exeReaction();
    void exeTouch();
    void exeMicReaction();
    void appearChameleon();
private:
    RenderMaterialIndirectParam* mParam = nullptr;
    ChameleonStateTouch* mTouch = nullptr;
    ChameleonStateHipDrop* mHipDrop = nullptr;
    ChameleonStateMic* mMic = nullptr;
    int mTouchSoundDelay = 0;
};
static_assert(sizeof(OneSideStepChameleon) == 0x170);
