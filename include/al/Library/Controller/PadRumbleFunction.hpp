#pragma once

#include <math/seadVector.h>

namespace al {
class LayoutActor;
class LiveActor;
class PadRumbleDirector;

struct PadRumbleParam {
    PadRumbleParam(f32 near = 0.0f, f32 far = 3000.0f, f32 volumeLeft = 1.0f,
                   f32 volumeRight = 1.0f, f32 pitchLeft = 1.0f, f32 pitchRight = 1.0f)
        : near(near), far(far), volumeLeft(volumeLeft), volumeRight(volumeRight),
          pitchLeft(pitchLeft), pitchRight(pitchRight) {}

    void setVolumeByBalance(f32 balance);

    f32 near;
    f32 far;
    f32 volumeLeft;
    f32 volumeRight;
    f32 pitchLeft;
    f32 pitchRight;
    s32 _18 = 0;
    bool _1c = false;
    bool _1d = false;
};
}  // namespace al

namespace alPadRumbleFunction {
al::PadRumbleDirector* getPadRumbleDirector(const al::LiveActor* pActor);
al::PadRumbleDirector* getPadRumbleDirector(const al::LayoutActor* pActor);
void pauseActivePadRumbles(al::LiveActor* pActor);
void resumePausedPadRumbles(al::LiveActor* pActor);
void startPadRumble(const al::LiveActor* pActor, const char* pName, s32 port = -1,
                    bool isFlag = false);
void startPadRumbleNo3D(const al::LiveActor* pActor, const char* pName, s32 port = -1,
                        bool isFlag = false);
void stopPadRumbleDirect(const al::LiveActor* pActor, s32 port);
void startPadRumble(al::PadRumbleDirector* pDirector, const sead::Vector3f& rPos,
                    const char* pName, f32 near, f32 far, s32 port = -1, bool isFlag = false);
void startPadRumbleWithParam(al::PadRumbleDirector* pDirector, const sead::Vector3f& rPos,
                             const char* pName, const al::PadRumbleParam& rParam, s32 port = -1,
                             bool isFlag = false);
void startPadRumble(const al::LiveActor* pActor, const char* pName, f32 near, f32 far,
                    s32 port = -1, bool isFlag = false);
void startPadRumbleNo3DWithParam(const al::LiveActor* pActor, const char* pName,
                                 const al::PadRumbleParam& rParam, s32 port = -1,
                                 bool isFlag = false);
void startPadRumbleWithParam(const al::LiveActor* pActor, const sead::Vector3f& rPos,
                             const char* pName, const al::PadRumbleParam& rParam, s32 port = -1,
                             bool isFlag = false);
void startPadRumblePos(const al::LiveActor* pActor, const sead::Vector3f& rPos, const char* pName,
                       f32 near, f32 far, s32 port = -1, bool isFlag = false);
void startPadRumbleNo3D(al::PadRumbleDirector* pDirector, const char* pName, s32 port = -1,
                        bool isFlag = false);
void startPadRumbleNo3DWithParam(al::PadRumbleDirector* pDirector, const char* pName,
                                 const al::PadRumbleParam& rParam, s32 port = -1,
                                 bool isFlag = false);
void startPadRumbleNo3DWithParam(al::PadRumbleDirector* pDirector, const char* pName,
                                 f32 volumeLeft, f32 volumeRight, f32 pitchLeft, f32 pitchRight,
                                 s32 port = -1, bool isFlag = false);
void startPadRumbleNo3DWithParam(const al::LiveActor* pActor, const char* pName, f32 volumeLeft,
                                 f32 volumeRight, f32 pitchLeft, f32 pitchRight, s32 port = -1,
                                 bool isFlag = false);
void stopPadRumbleOneTime(al::PadRumbleDirector* pDirector, const char* pName, s32 port = -1);
void stopPadRumbleOneTime(const al::LiveActor* pActor, const char* pName, s32 port = -1);
void startPadRumbleLoop(al::PadRumbleDirector* pDirector, const char* pName,
                        const sead::Vector3f* pPos, f32 near, f32 far, s32 port = -1,
                        bool isFlag = false);
void startPadRumbleLoopWithParam(al::PadRumbleDirector* pDirector, const char* pName,
                                 const sead::Vector3f* pPos, const al::PadRumbleParam& rParam,
                                 s32 port = -1, bool isFlag = false);
void startPadRumbleLoop(const al::LiveActor* pActor, const char* pName, const sead::Vector3f* pPos,
                        f32 near, f32 far, s32 port = -1, bool isFlag = false);
void startPadRumbleLoopWithParam(const al::LiveActor* pActor, const char* pName,
                                 const sead::Vector3f* pPos, const al::PadRumbleParam& rParam,
                                 s32 port = -1, bool isFlag = false);
void startPadRumbleLoopNo3D(al::PadRumbleDirector* pDirector, const char* pName,
                            const sead::Vector3f* pPos, s32 port = -1, bool isFlag = false);
void startPadRumbleLoopNo3DWithParam(al::PadRumbleDirector* pDirector, const char* pName,
                                     const sead::Vector3f* pPos, const al::PadRumbleParam& rParam,
                                     s32 port = -1, bool isFlag = false);
void startPadRumbleLoopNo3D(const al::LiveActor* pActor, const char* pName,
                            const sead::Vector3f* pPos, s32 port = -1, bool isFlag = false);
void startPadRumbleLoopNo3DWithParam(const al::LiveActor* pActor, const char* pName,
                                     const sead::Vector3f* pPos, const al::PadRumbleParam& rParam,
                                     s32 port = -1, bool isFlag = false);
void stopPadRumbleLoop(al::PadRumbleDirector* pDirector, const char* pName,
                       const sead::Vector3f* pPos, s32 port = -1);
void stopPadRumbleLoop(const al::LiveActor* pActor, const char* pName, const sead::Vector3f* pPos,
                       s32 port = -1);
bool checkIsAlivePadRumbleLoop(al::PadRumbleDirector* pDirector, const char* pName,
                               const sead::Vector3f* pPos, s32 port = -1);
bool checkIsAlivePadRumbleLoop(const al::LiveActor* pActor, const char* pName,
                               const sead::Vector3f* pPos, s32 port = -1);
void startPadRumbleLoopControlable(const al::LiveActor* pActor, const char* pName,
                                   const sead::Vector3f* pPos, s32 port = -1);
void changePadRumbleLoopVolmue(const al::LiveActor* pActor, const char* pName,
                               const sead::Vector3f* pPos, f32 volumeLeft, f32 volumeRight,
                               s32 port = -1);
void changePadRumbleLoopVolmueEaseInRange(const al::LiveActor* pActor, const char* pName,
                                          const sead::Vector3f* pPos, f32 value, f32 min, f32 max,
                                          f32 volumeLeft, f32 volumeRight, s32 port = -1);
void changePadRumbleLoopPitch(const al::LiveActor* pActor, const char* pName,
                              const sead::Vector3f* pPos, f32 pitchLeft, f32 pitchRight,
                              s32 port = -1);
void startPadRumbleDirectValue(const al::LiveActor* pActor, f32 volumeLeft, f32 pitchLeft,
                               f32 frequencyLeft, f32 volumeRight, f32 pitchRight,
                               f32 frequencyRight, s32 port = -1);
void stopPadRumbleDirectValue(const al::LiveActor* pActor, s32 port = -1);
void startPadRumbleWithVolume(const al::LiveActor* pActor, const char* pName, f32 volumeLeft,
                              f32 volumeRight, s32 port = -1);
void startPadRumbleWithVolume(al::PadRumbleDirector* pDirector, const char* pName, f32 volumeLeft,
                              f32 volumeRight, s32 port = -1);
void makePadRumbleParamNearFarVolume(al::PadRumbleParam* pParam, f32 near, f32 far, f32 volume);
void makePadRumbleParamNearFarVolumeLR(al::PadRumbleParam* pParam, f32 near, f32 far,
                                       f32 volumeLeft, f32 volumeRight);
void makePadRumbleParamNearFarVolumePitch(al::PadRumbleParam* pParam, f32 near, f32 far,
                                          f32 volume, f32 pitch);
void makePadRumbleParamNearFarVolumePitchLR(al::PadRumbleParam* pParam, f32 near, f32 far,
                                            f32 volumeLeft, f32 volumeRight, f32 pitchLeft,
                                            f32 pitchRight);
}  // namespace alPadRumbleFunction
