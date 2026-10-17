#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Player/IUsePlayerWaterSurfaceInfo.hpp"

namespace al {
class IUseAreaObj;
}

/**
 * @brief Searches the water surface above or below a position.
 * @note Only what reconstructed code needs is declared so far.
 */
class PlayerWaterSurfaceFinder : public IUsePlayerWaterSurfaceInfo {
public:
    PlayerWaterSurfaceFinder(const al::IUseAreaObj* pAreaUser);

    void update(const sead::Vector3f& rPos, const sead::Vector3f& rUp);
    bool isWaterSurfaceExist() const override;
    f32 getWaterSurfaceHeight() const override;
    sead::Vector3f getWaterSurfacePosition() const override;
    void setWaterSurfaceCheckLength(f32 length);

private:
    const al::IUseAreaObj* mAreaUser;
    bool mIsExist;
    sead::Vector3f mSurfacePos;
    f32 mHeight;
    f32 mCheckLength;
};

static_assert(sizeof(PlayerWaterSurfaceFinder) == 0x28);
