#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>
#include <math/seadQuat.h>
#include <container/seadPtrArray.h>
#include <prim/seadSafeString.h>
namespace al {
class ActorInitInfo;
class WipeSimple;
} // namespace al
class KoopaChaseDemoInfo;
class DemoSkipLayout;
class KoopaChaseStateDemo : public al::ActorStateBase {
  public:
    KoopaChaseStateDemo(const char *, al::LiveActor *, const al::ActorInitInfo &,
                        KoopaChaseDemoInfo *);
    ~KoopaChaseStateDemo() override;
    void appear() override;
    void forceKill();
    void end(bool skipped);
    void setReturnTrans(const sead::Vector3f &);
    void setReturnTransAndQuat(const sead::Vector3f &, const sead::Quatf &);
    void entryDemoActor(al::LiveActor *);
    void placePlayersToReturnPos();
    void exeWait();
    void exeStart();
    void exeAction();
    void exeFade();
    void exeFadeDone();
    bool isSkipped() const { return mIsSkipped; }

  private:
    KoopaChaseDemoInfo *mInfo;
    sead::Vector3f mReturnPos = {0, 0, 0};
    sead::Quatf mReturnQuat = sead::Quatf::unit;
    sead::PtrArray<al::LiveActor> mActors;
    sead::FixedSafeString<32> mStartRecorderId;
    sead::FixedSafeString<32> mEndRecorderId;
    bool mIsSkipped = false;
    DemoSkipLayout *mSkipLayout;
    al::WipeSimple *mWipe;
};
static_assert(sizeof(KoopaChaseStateDemo) == 0xe0);
