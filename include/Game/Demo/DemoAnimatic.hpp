#pragma once

#include "Demo/DemoCutscene.hpp"

/** @brief Plays a layout-only cutscene with a skip prompt. */
class DemoAnimatic : public DemoCutscene {
public:
    explicit DemoAnimatic(const char* pName,
        alSeFunction::DemoType type = static_cast<alSeFunction::DemoType>(5));
    void init(const al::ActorInitInfo& rInfo) override;
};

static_assert(sizeof(DemoAnimatic) == 0x368);
