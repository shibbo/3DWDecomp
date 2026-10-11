#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadVector.h>

namespace al {
class MtxConnector;
class CollisionPartsFilterActor;
}
class KillerGenerator;

class KillerLauncher : public al::LiveActor {
public:
    KillerLauncher(const char* pName);
    /** @brief Releases the launcher actor. */
    ~KillerLauncher() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void control() override;
    void updatePoseGenerator();

private:
    KillerGenerator* mGenerator = nullptr;
    al::MtxConnector* mConnector = nullptr;
    al::CollisionPartsFilterActor* mFilter = nullptr;
    sead::Vector3f mGeneratorOffset = {0.0f, 0.0f, 0.0f};
};
