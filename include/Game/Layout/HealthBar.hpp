#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LiveActor; class LayoutInitInfo; }
namespace rc {
/** @brief Boss health bar with animated damage and healing. */
class HealthBar : public al::LayoutActor {
public:
    HealthBar(const al::LiveActor* pActor, const al::LayoutInitInfo& rInfo, float (*pGetRate)(const al::LiveActor*), const char* pArchive);
    void kill() override;
    al::SceneCameraInfo* getSceneCameraInfo() const override { return mCameraInfo; }
    virtual void updatePanePos();
    void exeAppear();
    void refreshHealthBar();
    void exeWait();
    void updateHealthBar();
    void exeHeal();
    void exeDamageStart();
    void exeDamage();
    void exeEnd();
    void hide();
    void show();
    void startAppear(bool isInstant);
    void end();
    void setEndAfterDamage();
    void startFinish();
    void setHoldTime(float time);
    bool isEnd() const;
private:
    al::SceneCameraInfo* mCameraInfo = nullptr;
    float mHealth = 0.0f;
    float mTargetHealth = 0.0f;
    float mOldHealth = 0.0f;
    float mDamageStep = -1.0f;
    const al::LiveActor* mActor;
    float (*mGetRate)(const al::LiveActor*);
    float mHeight = 6000.0f;
    bool mEndAfterDamage = false;
};
static_assert(sizeof(HealthBar) == 0x158);
}
