#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class MtxConnector;
}

/** @brief A demo start marker that follows the collision surface it is attached to. */
class DemoStartPosition : public al::LiveActor {
public:
    explicit DemoStartPosition(const char* pName);
    /** @brief Destroys the actor through the LiveActor base. */
    ~DemoStartPosition() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void control() override;

private:
    al::MtxConnector* mConnector = nullptr;
    sead::Matrix34f mLocalMtx = sead::Matrix34f::ident;
};

static_assert(sizeof(DemoStartPosition) == 0x180);
