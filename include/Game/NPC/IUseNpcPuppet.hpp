#pragma once

#include <math/seadMatrix.h>
#include <math/seadVector.h>

namespace al {
class HitSensor;
}  // namespace al

/**
 * @brief How an NPC puppet binding ended.
 * @note The values have not been reconstructed yet.
 */
enum NpcPuppetBindEndType : s32 {};

/**
 * @brief Interface of NPCs that can be bound and moved around by a puppeteer.
 */
class IUseNpcPuppet {
public:
    virtual void startBindNpc(al::HitSensor* pSelf, al::HitSensor* pOther) = 0;
    virtual void endBindNpc(NpcPuppetBindEndType type) = 0;
    virtual void setTransVec(const sead::Vector3f& rTrans) = 0;
    virtual void setFrontVec(const sead::Vector3f& rFront) = 0;
    virtual void setUpVec(const sead::Vector3f& rUp) = 0;
    virtual void setMtx(const sead::Matrix34f* pMtx) = 0;
    virtual void setPlayerPuppetInputTurnStick(f32 x, f32 y) = 0;
    virtual const sead::Vector3f& getTransVec() const = 0;
    virtual const sead::Vector3f& getFrontVec() const = 0;
    virtual const sead::Vector3f& getUpVec() const = 0;
};
