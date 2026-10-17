#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/** @brief Debug actor that loads the "AnimTest" archive and idles in a wait state. */
class AnimTest : public al::LiveActor {
public:
    AnimTest(const char* pName);
    /** @brief Releases the animation test actor. */
    ~AnimTest() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void exeWait();
};
