#pragma once

#include "Demo/DemoSceneActor.hpp"

/** @brief Specialized Bowser model for demo scenes.
 * @note The additional playback state remains to be reconstructed. */
class DemoSceneDarkBowser : public DemoSceneActor {
public:
    DemoSceneDarkBowser(const char* pName, DemoSceneActorHolder* pHolder);
private:
    unsigned char mPlaybackState[0x270 - 0x1c0];
};
static_assert(sizeof(DemoSceneDarkBowser) == 0x270);
