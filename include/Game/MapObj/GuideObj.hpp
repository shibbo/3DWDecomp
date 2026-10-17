#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Scene/ISceneObj.hpp"
#include <container/seadPtrArray.h>
namespace al { class SimpleLayoutAppearWaitEnd; }
/** @brief Places a goal guide at an actor's projected screen position. */
class GuideObj : public al::LiveActor {
public:
    explicit GuideObj(const char* pName);
    ~GuideObj() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void appear() override;
    void kill() override;
    void startPause();
    void endPause();
    void exeAppear();
    void exeWait();
private:
    al::SimpleLayoutAppearWaitEnd* mGuide = nullptr;
    bool mWasGuideAlive = false;
};
static_assert(sizeof(GuideObj) == 0x158);
/** @brief Pauses and resumes the stage's goal guides. */
class GuideObjHolder : public al::ISceneObj {
public:
    GuideObjHolder();
    const char* getSceneObjName() const override { return "GuideObjHolder"; }
    void registerGuideObj(GuideObj* pGuide);
    void startPause();
    void endPause();
private:
    sead::PtrArray<GuideObj> mGuides;
};
static_assert(sizeof(GuideObjHolder) == 0x18);
