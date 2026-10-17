#pragma once
#include "Library/MapObj/RailMoveMapParts.hpp"
#include "MapObj/Fury/DisasterAnimPart.hpp"
#include "Util/AttachObjectList.hpp"
class FloatingIslandRailPart : public al::RailMoveMapParts, public DisasterAnimPart {
public:
    explicit FloatingIslandRailPart(const char*);
    ~FloatingIslandRailPart() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void control() override;
    void exeMove() override;
    void exeMoveSign() override;
    void exeStandBy() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void onDisasterModeStateChange(DisasterModeController::State) override;
private:
    float mBaseSpeed;
    int mNoTouchFrames = 0;
    int mSpeedChangeFrames = 0;
    bool mUnoccupied = true;
    sead::Matrix34f mFarLodMtx;
    const sead::Matrix34f* mJointMtx;
    rc::AttachObjectList mAttachedObjects;
};
static_assert(sizeof(FloatingIslandRailPart) == 0x328);
