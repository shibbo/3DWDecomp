#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include "Library/Scene/ISceneObj.hpp"
/** @brief Interface prefix for the character selection layout's scene-object access. */
class RingPlayerSelect : public al::LayoutActor, public al::ISceneObj {
public:
    void requestUnlockSequence(int character);
};
static_assert(sizeof(al::LayoutActor) == 0x128);
