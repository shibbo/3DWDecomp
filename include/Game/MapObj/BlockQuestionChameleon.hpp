#pragma once
#include "Library/LiveActor/LiveActor.hpp"
struct RenderMaterialIndirectParam;
class ChameleonStateGiantPlayer;
class ChameleonStateTouch;
class ChameleonStateMic;
class ChameleonStateHipDrop;
class BlockQuestionChameleon : public al::LiveActor {
public:
    BlockQuestionChameleon();
    ~BlockQuestionChameleon() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void respawn() override;
    void exeWait();
    void exeGiantPlayer();
    void exeDRCTouch();
    void exeMicReaction();
    void exeHipDropReaction();
private:
    RenderMaterialIndirectParam* mParam = nullptr;
    ChameleonStateGiantPlayer* mGiantPlayer = nullptr;
    ChameleonStateTouch* mTouch = nullptr;
    ChameleonStateMic* mMic = nullptr;
    ChameleonStateHipDrop* mHipDrop = nullptr;
    bool mIsLong = false;
    int mTouchSoundDelay = 0;
};
static_assert(sizeof(BlockQuestionChameleon) == 0x178);
