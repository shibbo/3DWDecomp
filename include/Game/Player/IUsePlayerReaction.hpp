#pragma once

/// Named reactions (rumble and the like) of the player (implemented by PlayerActor).
class IUsePlayerReaction {
public:
    virtual void notifyReaction(const char* pName) = 0;
};
