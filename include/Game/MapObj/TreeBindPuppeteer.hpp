#pragma once

#include <basis/seadTypes.h>

#include "MapObj/BindPuppeteer.hpp"

namespace al {
class ByamlIter;
class HitSensor;
}  // namespace al

class Tree;

/**
 * @brief Puppeteer that drives a player climbing (or doing a hand stand on) a tree.
 */
class TreeBindPuppeteer : public BindPuppeteer {
public:
    TreeBindPuppeteer(const char* pName, Tree* pTree);

    void startBind(al::HitSensor* pPlayerSensor, al::HitSensor* pBinderSensor);
    void setParam(const al::ByamlIter& rIter);
    void update();
    void updateInput();
    void bindCancel();
    void bindEnd();
    bool isClimb() const;
    bool isRelease() const;
    bool isGiantAtClimb();
    bool isEnableInput();

    /** @return Whether a player is allowed to start climbing through this puppeteer. */
    bool isEnableBind() const { return mIsEnableBind; }

    /** @brief Allows a player to start climbing through this puppeteer again. */
    void validateBind() { mIsEnableBind = true; }

    /** @return Whether the bound player is doing a hand stand on the tree top. */
    bool isHandStand() const { return mIsHandStand; }

private:
    u8 _1c[0x14];
    bool mIsEnableBind;  // 0x30
    bool mIsHandStand;   // 0x31
    u8 _32[0x26];
};

static_assert(sizeof(TreeBindPuppeteer) == 0x58);
