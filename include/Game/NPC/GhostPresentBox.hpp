#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al {
class PoseHistoryPath;
}
class GhostPresentBox : public al::LiveActor {
public:
    GhostPresentBox(const char* pName, int itemIndex);
    ~GhostPresentBox() override;
    const char* getItemName() const;
    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void setPoseHistoryPath(const al::PoseHistoryPath* pPath);
    void start();
    void updatePoseByHistroryPath();
    void stop(bool showEffect);
    void exeStart();
    void exeWait();
    void exeStop();
    void exePopUp();
    void exePopUpInRouteDokan();
    void exeOpen();
    void exeEnd();
    bool isInRouteDokan() const override;

private:
    const al::PoseHistoryPath* mHistory;
    int mItemIndex;
    float mOffset;
    bool mInRouteDokan;
    bool mHidden;
};
static_assert(sizeof(GhostPresentBox) == 0x160);
