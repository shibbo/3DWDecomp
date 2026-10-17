#include "MapObj/OceanWaveWake.hpp"
#include "Library/ActorUtil.hpp"
#include "MapObj/OceanWaterDeferred.hpp"
#include <cfloat>
#include <cstring>
#include <detail/aglGPUMemBlockMgr.h>
#include <math/seadMathCalcCommon.h>
#include <nvn/nvn_FuncPtrInline.h>

void OceanWaveWake::init() {
  mWakes.clear();
  memset(mPoints, 0, sizeof(mPoints));
  mSpring = 1.0f / 32.0f;
  mDamping = 1.0f / 512.0f;
  mDecay = 1.0f / 32.0f;
}
bool OceanWaveWake::detachWakeFromActor(al::LiveActor *actor) {
  for (auto &wake : mWakes) {
    if (wake.actor == actor) {
      mWakes.erase(&wake);
      return true;
    }
  }
  return false;
}
bool OceanWaveWake::attachWakeToActor(al::LiveActor *actor,
                                      const Params &params, int priority) {
  Wake *entry = nullptr;
  for (auto &wake : mWakes) {
    if (wake.actor == actor) {
      entry = &wake;
      break;
    }
  }
  if (!entry) {
    if (!mWakes.isFull())
      entry = mWakes.emplaceBack();
    else {
      const auto &pos = al::getTrans(actor);
      f32 dx = pos.x - mCenterX, dz = pos.z - mCenterZ;
      f32 distance = dx * dx + dz * dz;
      f32 farthest = -FLT_MAX;
      Wake *candidate = nullptr;
      for (auto &wake : mWakes) {
        const auto &otherPos = al::getTrans(wake.actor);
        if (wake.priority < priority) {
          entry = &wake;
          break;
        }
        f32 dx = otherPos.x - mCenterX, dz = otherPos.z - mCenterZ;
        f32 otherDistance = dx * dx + dz * dz;
        if (otherDistance > distance && otherDistance > farthest) {
          candidate = &wake;
          farthest = otherDistance;
        }
      }
      if (!entry && candidate)
        entry = candidate;
    }
    if (!entry)
      return false;
    entry->initialized = false;
  }
  entry->params = params;
  entry->actor = actor;
  entry->priority = priority;
  return true;
}
OceanWavePoint *OceanWaveWake::updateWavePoint(OceanWavePoint *prev, long x,
                                               long z) {
  auto *next = &point(x, z);
  f32 difference = next->mHeight - prev->mHeight;
  long force = long(mSpring * difference);
  f32 sum = next->mHeight + prev->mHeight;
  long damping = long(mDamping * sum);
  prev->mVelocity += force - damping;
  next->mVelocity -= force + damping;
  return next;
}
void OceanWaveWake::update(const sead::Vector3f &cameraPos,
                           const sead::Vector3f &) {
  if (!mWakes.isEmpty()) {
    auto *actor = mWakes.begin()->actor;
    if (!actor)
      return;
    const auto &pos = al::getTrans(actor);
    mCenterX = long(pos.x);
    mCenterZ = long(pos.z);
  } else {
    mCenterX = long(cameraPos.x);
    mCenterZ = long(cameraPos.z);
  }
  mSampleCenterX = mCenterX;
  mSampleCenterZ = mCenterZ;
  for (auto &wake : mWakes) {
    const auto &pos = al::getTrans(wake.actor);
    sead::Vector3f front;
    al::calcFrontDir(&front, wake.actor);
    f32 dx = pos.x - mCenterX;
    f32 dz = pos.z - mCenterZ;
    if (dx * dx + dz * dz > 4194304.0f) {
      wake.pos = pos;
      wake.front = front;
      continue;
    }
    dx = pos.x - wake.pos.x;
    dz = pos.z - wake.pos.z;
    wake.pos = pos;
    f32 distanceSq = dx * dx + dz * dz;
    if (distanceSq <= 4.0f)
      continue;
    long speed = long((sead::Mathf::sqrt(distanceSq) - 2.0f) * 256.0f);
    if (speed < 1)
      continue;
    long strength = long(wake.params.amplitude * speed / 10.0f);
    long x = long(pos.x + front.x * wake.params.offset);
    long z = long(pos.z + wake.params.offset * front.z);
    long ix = x >> 5, iz = z >> 5;
    long first;
    long second;
    long third;
    long quadrant = ((x >> 4) & 1) | ((z >> 3) & 2);
    if (quadrant == 1) {
      first = index(ix + 1, iz);
      second = index(ix, iz);
      third = index(ix + 1, iz + 1);
    } else if (quadrant == 2) {
      first = index(ix, iz + 1);
      second = index(ix + 1, iz + 1);
      third = index(ix, iz);
    } else if (quadrant == 3) {
      first = index(ix + 1, iz + 1);
      second = index(ix + 1, iz);
      third = index(ix, iz + 1);
    } else {
      first = index(ix, iz);
      second = index(ix, iz + 1);
      third = index(ix + 1, iz);
    }
    if (mPoints[first].mHeight < 6144) {
      mPoints[first].mHeight += strength;
      mPoints[first].mVelocity = 0;
    }
    if (mPoints[second].mHeight < 6144) {
      mPoints[second].mHeight += strength;
      mPoints[second].mVelocity = 0;
    }
    if (mPoints[third].mHeight < 6144) {
      mPoints[third].mHeight += strength;
      mPoints[third].mVelocity = 0;
    }
  }
  long x = mCenterX >> 5, z = mCenterZ >> 5;
  long startX = x + 192, startZ = z + 192;
  clearBorder(x, z);
  for (long i = 0; i < 128; ++i) {
    auto *prev = &point(i + startX, z + 191);
    for (long j = 0; j < 129; ++j)
      prev = updateWavePoint(prev, i + startX, startZ + j);
  }
  for (long i = 0; i < 128; ++i) {
    auto *prev = &point(x + 191, i + startZ);
    for (long j = 0; j < 129; ++j)
      prev = updateWavePoint(prev, startX + j, i + startZ);
  }
  clearBorder(x, z);
  for (long i = 0; i < 128; ++i) {
    for (long j = 0; j < 128; ++j) {
      auto &p = point(startX + j, i + startZ);
      p.mHeight += p.mVelocity;
      p.mVelocity -= mDecay * p.mVelocity;
    }
  }
}
void OceanWaveWake::CalcWaterVtxColor(WavePatch *) {}
void OceanWaveWake::updateWavePatch(WavePatch *patch) {
  long count = patch->mSize * patch->mSize;
  for (long i = 0; i < count; ++i) {
    auto &block = *patch->mVertexBlock;
    auto *vertices = reinterpret_cast<OceanVertex *>(
        static_cast<u8 *>(
            nvnMemoryPoolMap(block.getMemoryPool()->getDriverPool())) +
        block.getByteOffset());
    auto &pos = vertices[int(i)].mPos;
    if (pos.y < -5000.0f)
      continue;
    long x = long(pos.x), z = long(pos.z);
    long dx = mSampleCenterX - x, dz = mSampleCenterZ - z;
    if (static_cast<unsigned long>(dx * dx + dz * dz) > 4194304)
      continue;
    long ix = x >> 5, iz = z >> 5, fx = x & 31, fz = z & 31;
    long h =
        ((32 - fx) * point(ix, iz).mHeight + fx * point(ix + 1, iz).mHeight) *
            (32 - fz) +
        ((32 - fx) * point(ix, iz + 1).mHeight +
         fx * point(ix + 1, iz + 1).mHeight) *
            fz;
    if (!h)
      continue;
    long remaining = 3145728 - h;
    h = 3145728 - (remaining & ~(remaining >> 31));
    pos.y = f32(h) / 262144.0f;
  }
}
f32 OceanWaveWake::getHeight(long x, long z) {
  long dx = mSampleCenterX - x, dz = mSampleCenterZ - z;
  if (static_cast<unsigned long>(dx * dx + dz * dz) > 4194304)
    return 0.0f;
  long ix = x >> 5, iz = z >> 5, fx = x & 31, fz = z & 31;
  long upper =
      (32 - fx) * point(ix, iz).mHeight + fx * point(ix + 1, iz).mHeight;
  long h = upper * (32 - fz);
  h += fz * (32 - fx) * point(ix, iz + 1).mHeight;
  h += fz * fx * point(ix + 1, iz + 1).mHeight;
  long remaining = 3145728 - h;
  h = 3145728 - (remaining & ~(remaining >> 31));
  return f32(h) / 262144.0f;
}
