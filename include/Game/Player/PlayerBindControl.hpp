#pragma once

#include <basis/seadTypes.h>

class IUsePlayerBind;
class IUsePlayerEndBind;
class PlayerActionGraph;
class PlayerActionNode;

/// Moves the player's action graph into and out of the bind action (pipes, cannons, ...).
class PlayerBindControl {
public:
    PlayerBindControl(IUsePlayerBind* pBind, IUsePlayerEndBind* pEndBind,
                      PlayerActionGraph* pActionGraph, PlayerActionNode* pBindNode);
    void update();
    void clearBindable();
    void tryPermit();
    void notifyEnd();
    void forceBind();
    void cancelBind();

private:
    u8 _0[0x28];
};
static_assert(sizeof(PlayerBindControl) == 0x28);
