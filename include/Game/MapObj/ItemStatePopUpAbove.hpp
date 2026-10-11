#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

class ItemStatePopUpAbove : public al::ActorStateBase {
public:
    explicit ItemStatePopUpAbove(al::LiveActor* pActor);
    void init() override;
    void appear() override;
    void kill() override;
    void exePunch();
    void exeFall();

    /**
     * @brief Set the speed scale of the pop-up motion.
     * @param scale The scale.
     */
    void setSpeedScale(float scale) { mSpeedScale = scale; }

private:
    bool mIsInWater;
    float mSpeedScale = 1.0f;
};
