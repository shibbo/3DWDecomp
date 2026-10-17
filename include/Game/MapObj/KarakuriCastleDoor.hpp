#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class GuideBalloon;
class KarakuriCastleDoor : public al::LiveActor {
public:
    KarakuriCastleDoor(const char*);
    ~KarakuriCastleDoor() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void startClipped() override;
    void exeOpenWait();
    void exeOpenReady();
    void exeOpening();
    void exeOpenFinish();
    void setParam(int, int, float);
    void startOpen(const sead::Vector3f&, int);
    bool isOpenReady() const;
    bool isOpen() const;
    bool isOpening() const;
    int getIndex() const { return mIndex; }
private:
    int mIndex = 0;
    int mDoorCount = 0;
    int mMoveFrames = 0;
    int mGuideBalloonType = 0;
    float mMoveSpeed = 20.0f;
    sead::Vector3f mInitialPosition = sead::Vector3f::zero;
    sead::Vector3f mDestination = sead::Vector3f::zero;
    GuideBalloon* mGuideBalloon = nullptr;
};
