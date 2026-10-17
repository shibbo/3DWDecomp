#pragma once
#include "MapObj/Ocean.hpp"
class OceanWaveWake {
public:
  struct Params {
    f32 amplitude = 24.0f;
    f32 offset = -160.0f;
  };
  void init();
  bool detachWakeFromActor(al::LiveActor *);
  bool attachWakeToActor(al::LiveActor *, const Params &, int);
  OceanWavePoint *updateWavePoint(OceanWavePoint *, long, long);
  void update(const sead::Vector3f &, const sead::Vector3f &);
  void CalcWaterVtxColor(WavePatch *);
  void updateWavePatch(WavePatch *);
  f32 getHeight(long, long);

private:
  struct Wake {
    Params params;
    al::LiveActor *actor;
    s32 priority;
    bool initialized;
    sead::Vector3f front{0.0f, 0.0f, 0.0f};
    sead::Vector3f pos{0.0f, 0.0f, 0.0f};
  };
  static long index(long x, long z) { return ((z & 255) << 8) | (x & 255); }
  OceanWavePoint &point(long x, long z) {
    return mPoints[((z & 255) << 8) | (x & 255)];
  }
  void clearBorder(long x, long z) {
    for (long i = 0; i < 130; ++i) {
      auto *right = &point(x + 64, z + 191 + i);
      auto *left = &point(x + 191, z + 191 + i);
      auto *top = &point(x + 191 + i, z + 64);
      auto *bottom = &point(x + 191 + i, z + 191);
      *right = {};
      *left = {};
      *top = {};
      *bottom = {};
    }
  }
  OceanFixedObjList<Wake, 8> mWakes;
  long mCenterX;
  long mCenterZ;
  long mSampleCenterX;
  long mSampleCenterZ;
  f32 mSpring;
  f32 mDamping;
  f32 mDecay;
  OceanWavePoint mPoints[256 * 256];
};
static_assert(sizeof(OceanWaveWake) == 0x40260);
