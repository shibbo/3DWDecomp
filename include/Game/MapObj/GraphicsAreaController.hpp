#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class AreaObj; }
class GraphicsAreaController : public al::LiveActor {
public:
    explicit GraphicsAreaController(const char*);
    void init(const al::ActorInitInfo&) override;
    void finishInit(const al::ActorInitInfo&);
    void initAfterPlacement() override;
    void appear() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void switchToNormalDisaster();
    void switchToHardDisaster();
    void switchToSuperHardDisaster();
    void triggerFadeTo();
    void triggerFadeFrom();
    void triggerFadeOverFramesTo(int);
    void triggerFadeOverFramesFrom(int);
    void setLerpStep(int);
    void pauseFade();
    void resumeFade();
    void setLerp(float);
    void exeWait();
    void exeFadeOut();
    void exeFadeIn();
private:
    const char* mGraphicsAreaName = nullptr;
    const char* mDepthOfFieldAreaName = nullptr;
    const char* mNormalDisasterName = nullptr;
    const char* mHardDisasterName = nullptr;
    const char* mSuperHardDisasterName = nullptr;
    const char* mUnusedName = nullptr;
    al::AreaObj* mCurrentArea = nullptr;
    al::AreaObj* mNormalDisasterArea;
    al::AreaObj* mHardDisasterArea = nullptr;
    al::AreaObj* mSuperHardDisasterArea = nullptr;
    al::AreaObj* mDepthOfFieldArea = nullptr;
    void* mUnused;
};
static_assert(sizeof(GraphicsAreaController) == 0x1a8);
