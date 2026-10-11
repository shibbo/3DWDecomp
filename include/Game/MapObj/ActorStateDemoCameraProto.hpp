#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>
namespace al { class ActorInitInfo; class CameraTicket; }
class CameraLookAtPoint;
class ActorStateDemoCameraProto : public al::ActorStateBase {
public:
    ActorStateDemoCameraProto(al::LiveActor*, const al::ActorInitInfo&, const char*, int, int, bool);
    void appear() override;
    int getPlayStep() const;
    bool isPlayStep(int) const;
    bool isLessEqualPlayStep(int) const;
    bool isGreaterEqualPlayStep(int) const;
    bool tryStart(const al::Nerve*);
    void exePrepare();
    bool isCameraMoving();
    void exePlayWait();
    void setCameraPoints(const CameraLookAtPoint*);
    void exePlay();
    void forceCameraDone();
    void jumpToFinish();
    void exePlayEnd();
    void exeDone();

    /** @brief Play the camera without starting a demo. */
    void setNoDemo() { mNoDemo = true; }
private:
    const char* mCameraName;
    const CameraLookAtPoint* mCameraPoints;
    al::CameraTicket* mCameraTicket;
    u32 mPointIndex;
    int mPlayStep;
    sead::Vector3f mStartAt;
    sead::Vector3f mStartPos;
    int mStartWaitTime;
    int mStartMoveTime;
    sead::Vector3f mCameraAt;
    sead::Vector3f mCameraPos;
    bool mAtPoint;
    bool mFinished;
    bool mMoving;
    bool mCameraActive;
    bool mSkip;
    bool mFreeze;
    bool mIntro;
    bool mCapture;
    bool mNoDemo = false;
};
static_assert(sizeof(ActorStateDemoCameraProto) == 0x88);
