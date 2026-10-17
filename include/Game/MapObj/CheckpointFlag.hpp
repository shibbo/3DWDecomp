#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class PlacementId; }
// Reconstructed layout prefix used by ghost recording.
class CheckpointFlag : public al::LiveActor {
public:
    const al::PlacementId* getPlacementId() const { return mPlacementId; }
private:
    u64 _148;
    const al::PlacementId* mPlacementId;
};
