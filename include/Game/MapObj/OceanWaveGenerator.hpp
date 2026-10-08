#pragma once

#include <basis/seadTypes.h>

namespace al {
class LiveActor;
}

class Ocean;
struct WavePatch;

/**
 * @brief Drives the ambient waves of an Ocean around the sea center.
 */
class OceanWaveGenerator {
  public:
    void init(al::LiveActor* pActor);
    void initOcean(Ocean* pOcean);
    s16 getSeaOffset() const;
    void update(long frame);
    void updateWavePatch(WavePatch* pPatch);

  private:
    u8 _0[0xa48];
};

static_assert(sizeof(OceanWaveGenerator) == 0xa48);
