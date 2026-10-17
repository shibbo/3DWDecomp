#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "math/seadMatrix.h"
namespace al { class SimpleLayoutAppearWaitEnd; }
class ActorJointLookController;
class CourseSelectFairy : public al::LiveActor {
public:
    CourseSelectFairy(const char* pName, int fairyId, bool showHelp);
    ~CourseSelectFairy() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    void startClipped() override;
    void endClipped() override;
    void calcAnim() override;
    void control() override;
    void startPause();
    void endPause();
    void exeWorldStartDemo();
    void exeWait();
    void exeHappy();
    void exeWaitDemo();
    void exeBackFromDemo();
    void exeBackToCastle();
    void exeBackEnd();
    void startWorldClearDemo();
    void startBackToCastleDemo(const sead::Matrix34f* pMtx);
    void startWorldStartDemo();
    void endWorldStartDemo();
    void forceBackToCastle();
private:
    bool mShowHelp;
    al::SimpleLayoutAppearWaitEnd* mpHelp = nullptr;
    int mFairyId;
    sead::Vector3f mCastlePos = sead::Vector3f(0,0,0);
    sead::Matrix34f mDemoMtx = sead::Matrix34f::ident;
    al::LiveActor* mpBottle = nullptr;
    ActorJointLookController* mpLookController = nullptr;
    sead::Matrix34f mLookMtx = sead::Matrix34f::ident;
};
static_assert(sizeof(CourseSelectFairy) == 0x1d0);
