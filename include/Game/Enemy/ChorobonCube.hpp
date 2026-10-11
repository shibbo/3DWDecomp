#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class ChorobonCube : public al::LiveActor {
public:
    ChorobonCube(const char* pName);
    /** @brief Releases the Fuzzy cube actor. */
    ~ChorobonCube() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeFollow();
};
