#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveExecutor.hpp"

namespace al {
class HitSensor;
class LiveActor;
}  // namespace al

class IUsePlayerPuppet;
class PlayerBindEndParam;

/**
 * @brief Base of the objects that drive a bound player (its puppet) through a scripted move.
 */
class BindPuppeteer : public al::NerveExecutor {
public:
    BindPuppeteer(const char* pName);

    virtual void endBind(const PlayerBindEndParam* pParam);
    virtual void endBindOnGround();
    virtual void endBindSquat();
    virtual void endBindForceAbyss();
    virtual void cancelBind();

    void startBind(al::HitSensor* pPlayerSensor, al::HitSensor* pBinderSensor);
    void setNullPlayerPuppet();
    al::LiveActor* getTargetActor();
    s32 getControlUserId() const;
    IUsePlayerPuppet* getPlayerPuppet() const;
    /** @brief Checks for a bound player. @return Whether a puppet is attached. */
    bool isBind() const { return mPlayerPuppet != nullptr; }

private:
    IUsePlayerPuppet* mPlayerPuppet = nullptr;  // 0x10
    s32 mControlUserId = 0;  // 0x18
};

static_assert(sizeof(BindPuppeteer) == 0x20);
