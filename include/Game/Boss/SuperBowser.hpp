#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief Fury Bowser, the giant Bowser of Bowser's Fury.
 * @note Only what reconstructed code needs is declared so far.
 */
class SuperBowser : public al::LiveActor {
public:
    explicit SuperBowser(const char* pName);

    void addSpawnPoint(const al::ActorInitInfo& rInfo);
    void tryDrawDebugText();
    bool isPlessieChaseV2SpecialAttack() const;
    bool isPlayerInBetweenLegsArea() const;
};
