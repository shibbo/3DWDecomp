#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Scene/ISceneObj.hpp"
#include <container/seadPtrArray.h>
namespace al { class CameraInfo; class AreaObj; class AudioDirector; class SimpleLayoutAppearWaitEnd; }
class GuideObj;
class SuperbViewArea : public al::LiveActor {
public:
    explicit SuperbViewArea(const char* name);
    ~SuperbViewArea() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void offFilterLayout();
    void onFilterLayout();
    void exeWait();
    void exeLook();
    void startPause();
    void endPause();
private:
    al::CameraInfo* mCamera = nullptr;
    al::AreaObj* mArea = nullptr;
    al::AudioDirector* mAudioDirector = nullptr;
    GuideObj** mGuides = nullptr;
    int mGuideCount = 0;
    al::SimpleLayoutAppearWaitEnd* mFilter = nullptr;
    bool mCheckOnGround = true;
};
static_assert(sizeof(SuperbViewArea) == 0x180);
class SuperbViewAreaHolder : public al::ISceneObj {
public:
    SuperbViewAreaHolder();
    const char* getSceneObjName() const override;
    void registerArea(SuperbViewArea*);
    void startPause();
    void endPause();
private:
    sead::PtrArray<SuperbViewArea> mAreas;
};
