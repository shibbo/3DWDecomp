#pragma once

#include <basis/seadTypes.h>
#include <container/seadObjList.h>
#include <math/seadVector.h>

namespace al {
class LiveActor;
}

struct WavePatch;

/**
 * @brief One sample of the ocean height field.
 */
struct OceanWavePoint {
    s16 mHeight;
};

/**
 * @brief Object list that keeps its node storage inline, right after the list header.
 * @tparam T Element type.
 * @tparam N Capacity.
 */
template <typename T, s32 N>
class OceanFixedObjList : public sead::ObjList<T> {
  public:
    /**
     * @brief Construct the list over its inline node storage.
     */
    OceanFixedObjList() : sead::ObjList<T>(N, mWork) {}

  private:
    u8 mWork[sead::ObjList<T>::calculateWorkBufferSize(N)];
};

/**
 * @brief Simulated ocean height field with ripples and actor wakes.
 */
class Ocean {
  public:
    /**
     * @brief A ripple spreading on the surface.
     */
    struct Ripple {
        u8 _0[0x68];
    };

    /**
     * @brief A wake trailing behind an actor.
     */
    struct Wake {
        u8 _0[0x30];
    };

    static constexpr s32 cRippleMax = 64;
    static constexpr s32 cWakeMax = 8;

    void init();
    void update(const sead::Vector3f& rCameraPos, const sead::Vector3f& rCameraAt);
    void updateWavePatch(WavePatch* pPatch, s16 offset);
    void updateWavePatch_new(WavePatch* pPatch, s16 offset);
    void updateWakePatch(WavePatch* pPatch);
    void createRipple(long x, long z, long size, long speed, long time, long amp, long len);
    void attachWakeTo(al::LiveActor* pActor);
    void detachWakeFrom(al::LiveActor* pActor);
    s32 getRippleHeight(long x, long z);
    s32 getWakeHeight(long x, long z);
    const OceanWavePoint* getWavePoint(s32 x, s32 z) const;
    void waveWakeUpdateWavePatch(WavePatch* pPatch);
    void waveRippleUpdateWavePatch(WavePatch* pPatch);

  private:
    u8 _0[0xa850];
    OceanFixedObjList<Ripple, cRippleMax> mRippleList;  // 0xa850
    OceanFixedObjList<Wake, cWakeMax> mWakeList;        // 0xc680
    u8 _c8b0[0x4c8e0 - 0xc8b0];
    s64 mTime;  // 0x4c8e0

  public:
    /**
     * @brief Get the simulation time of the ocean.
     * @return The time, in frames.
     */
    s64 getTime() const { return mTime; }
};

static_assert(sizeof(Ocean) == 0x4c8e8);
