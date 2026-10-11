#pragma once

#include <basis/seadTypes.h>

class PlayerActionGraph;
class PlayerActionNode;

/// Kills the player when the time runs out, it falls into the abyss, ...
class PlayerKiller {
public:
    PlayerKiller(PlayerActionGraph* pActionGraph, PlayerActionNode* pDieNode);
    void killTimeUp();

private:
    u8 _0[0x10];
};
