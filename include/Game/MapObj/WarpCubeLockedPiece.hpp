#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>
#include <math/seadVector.h>
class CounterWarpCube;
class ItemStateAssistRotate;
class WarpCubeLockedPiece : public al::LiveActor {
public:
    WarpCubeLockedPiece(const char*);
    ~WarpCubeLockedPiece() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void kill() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void rotate(float);
    void exeAppearRise();
    void exeAppearFall();
    void exeWait();
    void exeAssistRotate();

    /** @brief Set the piece counter layout. @param pCounter The counter of the warp box. */
    void setCounter(CounterWarpCube* pCounter) { mCounter = pCounter; }

private:
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    sead::Vector3f mBaseTrans{0.0f, 0.0f, 0.0f};
    CounterWarpCube* mCounter = nullptr;
    ItemStateAssistRotate* mAssistRotate = nullptr;
    bool mIsAppearRise = false;
    float mRotateY = 0.0f;
};
