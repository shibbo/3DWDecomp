#pragma once

#include "Library/LiveActor/LiveActor.hpp"

// Partial declaration for callers; actor fields are not reconstructed yet.
class CheckPointStar : public al::LiveActor {
public:
    void cancel();
};
