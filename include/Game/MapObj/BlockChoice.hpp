#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadVector.h>
class BlockChoice : public al::LiveActor {
public:
    BlockChoice(const char*);
    ~BlockChoice() override;
    void init(const al::ActorInitInfo&) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exeWait();
    void exeReact();
    void exeReactHipDrop();
    void appearItemAll(bool);
    void setDisappear();
    bool isActivated() const { return mIsActivated; }
private:
    bool mIsActivated = false;
    bool mIsBig = false;
    int mItemType = 0;
    float mRotateY = 0.0f;
    sead::Vector3f mFront = sead::Vector3f::zero;
};
