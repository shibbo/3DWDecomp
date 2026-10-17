#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>

namespace al {
class MtxConnector;
}

class PackunTrace : public al::LiveActor {
public:
    PackunTrace(al::LiveActor* pOwner);
    /** @brief Releases the Piranha Plant remains actor. */
    ~PackunTrace() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void control() override;
    void appear() override;

    /** @brief Sets the rotation applied on top of the collision pose. */
    void setBaseQuat(const sead::Quatf& rQuat) { mBaseQuat = rQuat; }

private:
    al::LiveActor* mOwner;
    al::MtxConnector* mConnector = nullptr;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
};
