#pragma once

#include <basis/seadTypes.h>
#include "Demo/StageStartEventBase.hpp"

/**
 * @brief Start demo of the first phase of Bowser's Fury.
 * @note Only what reconstructed code needs is declared so far.
 */
class StageStartPhase0Demo : public StageStartEventBase {
public:
    StageStartPhase0Demo(const char* pName, al::LiveActor* pDirtPile);

    s64 getEventType() const override;
    void startDemo() override;
    void endDemo() override;
    bool isEndDemo() const override;

    /**
     * Makes the demo start already skipped (the player cancelled it before).
     */
    void setSkipped() { mIsSkipped = true; }

private:
    u8 _148[0x175 - 0x148];
    bool mIsSkipped;  // 0x175
    u8 _176[0x178 - 0x176];
};
static_assert(sizeof(StageStartPhase0Demo) == 0x178);
