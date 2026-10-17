#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

namespace al {
    class LiveActor;
    class ActorInitInfo;
    class CameraInfo;
    class CameraTicket;
    class AreaObjGroup;
};  // namespace al

class ActorStateDemoCameraParam {
public:
    ActorStateDemoCameraParam(int, int, int, const sead::Vector3f*, const sead::Vector3f*);

    int _0;
    int _4;
    int _8;
    const sead::Vector3f* _10;
    const sead::Vector3f* _18;
    sead::Vector3f mPositionOffset;  // 0x20
    sead::Vector3f mCameraOffset;  // 0x2c
};

static_assert(sizeof(ActorStateDemoCameraParam) == 0x38);

class ActorStateDemoCamera : public al::ActorStateBase {
public:
    ActorStateDemoCamera(al::LiveActor*, const al::ActorInitInfo&, const char*, const ActorStateDemoCameraParam*, bool);
    bool tryStart(const al::Nerve*);
    void appear() override;
    int getPlayStep() const;
    bool isPlayStep(int) const;
    bool isLessEqualPlayStep(int) const;
    void exePrepare();
    void exePlayWait();
    void exePlay();
    void exePlayEnd();
    bool isGreaterEqualPlayStep(int step) const;

private:
    const ActorStateDemoCameraParam* mParam;
    al::CameraInfo* mCamera = nullptr;
    al::CameraInfo* mEndCamera = nullptr;
    sead::FixedSafeString<64> mEndCameraName;
    al::AreaObjGroup* mStartArea = nullptr;
    al::CameraTicket* mCameraTicket = nullptr;
    bool mSingleMode = false;
    sead::Vector3f mCameraPos;
    sead::Vector3f mCameraAt;
};

static_assert(sizeof(ActorStateDemoCamera) == 0xc0);