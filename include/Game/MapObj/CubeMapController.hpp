#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class CubeMapController : public al::LiveActor {
public:
    CubeMapController(const char* pName);
    ~CubeMapController() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void appear() override;
    void control() override;
    virtual void update();
    void triggerFadeTo();
    void triggerFadeFrom();
    void setAnimationFrames(unsigned int frames);
    void setCubeMap(const char* pName);
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exeWait();
    void exeFadeOut();
    void exeFadeIn();
private:
    const char* mCubeMapName = nullptr;
    const char* mTargetCubeMapName = nullptr;
    float mFadeSpeed = 1.0f;
};
