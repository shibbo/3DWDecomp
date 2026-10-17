#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/Fury/DisasterAnimPart.hpp"
#include "Util/AttachObjectList.hpp"
#include <container/seadPtrArray.h>
namespace al { class CameraTicket; class WipeSimple; }
class DemoSkipLayout;
class LuckyIslandController;
class LuckyIsland : public al::LiveActor, public DisasterAnimPart {
public:
    explicit LuckyIsland(const char* pName);
    ~LuckyIsland() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void control() override;
    void onDisasterModeStateChange(DisasterModeController::State) override;
    void DisasterAppear();
    void DisasterDisappear();
    void setController(LuckyIslandController*);
    void StartDemo();
    void disableCollision();
    void UpdatePos(const sead::Vector3f&);
    void endDemo();
    void exeWait();
    void exeDescend();
    void exeDemoIntro();
    void exeFade();
private:
    int getCollectedObjectNum() const { return mCollectedObjects.size(); }
    LuckyIslandController* mController = nullptr;
    rc::AttachObjectList mObjects;
    sead::Vector3f mHome;
    sead::Vector3f mCameraDir;
    float mCameraDistance;
    sead::Vector3f mCameraAt;
    sead::Vector3f mCameraPos;
    sead::Vector3f mUnused;
    sead::PtrArray<al::LiveActor> mCollectedObjects;
    bool mInitialHide = true;
    bool mDemoRequested = false;
    bool mInDisasterDemo = false;
    al::CameraTicket* mCamera;
    DemoSkipLayout* mSkip;
    al::WipeSimple* mWipe;
};
static_assert(sizeof(LuckyIsland) == 0x330);
