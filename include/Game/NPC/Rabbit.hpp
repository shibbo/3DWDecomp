#pragma once

#include "Library/LiveActor/LiveActor.hpp"

// Partial declaration for callers; actor fields are not reconstructed yet.
class Rabbit : public al::LiveActor {
public:
    void cancel();
};
