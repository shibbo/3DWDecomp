#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class MtxConnector; }
class SnowCover;
class SignBoard : public al::LiveActor {
public:
    explicit SignBoard(const char*);
    ~SignBoard() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void respawn() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exeWait();
    void exeBreak();
private:
    bool mIsBreakable = false;
    al::MtxConnector* mConnector = nullptr;
    SnowCover* mSnowCover = nullptr;
    al::LiveActor* mBreakModel = nullptr;
    al::LiveActor* mTraceModel = nullptr;
    al::LiveActor* mTouchReaction = nullptr;
    bool mIsLarge = false;
};
static_assert(sizeof(SignBoard) == 0x178);
