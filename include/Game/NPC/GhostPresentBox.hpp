#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class PoseHistoryPath; }
class GhostPresentBox : public al::LiveActor {
public:
    GhostPresentBox(const char*, int);
    const char* getItemName() const;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void setPoseHistoryPath(const al::PoseHistoryPath*);
    void start();
    void updatePoseByHistroryPath();
    void stop(bool);
    void exeStart();
    void exeWait();
    void exeStop();
    void exePopUp();
    void exePopUpInRouteDokan();
    void exeOpen();
    void exeEnd();
    bool isInRouteDokan() const override;
private:
    const al::PoseHistoryPath* mHistory = nullptr;
    int mPlayerIndex;
    float mOffset = 0.0f;
    bool mInRouteDokan = false;
    bool mHidden = false;
};
static_assert(sizeof(GhostPresentBox) == 0x160);