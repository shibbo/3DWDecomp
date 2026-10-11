#pragma once
#include "Library/LiveActor/LiveActor.hpp"

/** @brief Item that rides on a Tentack tentacle's scaffold. */
class TentackAttachItem : public al::LiveActor {
public:
    void releaseAndTryKill();
    void eat(bool isForce);
};
