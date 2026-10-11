#pragma once

#include "NPC/NpcTargetFinder.hpp"

namespace al {
class LiveActor;
}  // namespace al

/**
 * @brief Interface that lets an actor reject targets found by its NpcTargetFinder.
 */
class IUseTargetFinderFilter {
public:
    virtual bool acceptTarget(const al::LiveActor* pTarget,
                              const npc::NpcFindTargetType& rType) const = 0;
};
