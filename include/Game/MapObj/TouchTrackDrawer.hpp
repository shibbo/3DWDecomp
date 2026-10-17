#pragma once
#include <container/seadPtrArray.h>
#include <math/seadVector.h>
namespace al { class LiveActor; class FootPrint; struct ActorInitInfo; }
class TouchTrackDrawer {
public:
    TouchTrackDrawer(al::LiveActor*);
    void init(const al::ActorInitInfo&, const char*, int);
    void update();
private:
    al::LiveActor* mHost;
    sead::PtrArray<al::FootPrint> mActiveTracks;
    sead::PtrArray<al::FootPrint> mAvailableTracks;
    bool mHasPreviousPoint = false;
    sead::Vector3f mPreviousPoint = {0.0f, 0.0f, 0.0f};
};
