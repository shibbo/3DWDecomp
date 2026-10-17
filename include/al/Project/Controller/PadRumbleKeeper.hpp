#pragma once

#include <basis/seadTypes.h>

namespace al {
class LiveActor;

class PadRumbleKeeper {
public:
    PadRumbleKeeper(s32 port);

    s32 getPort() const { return mPort; }

    void setPort(s32 port) { mPort = port; }

private:
    s32 mPort;
};

PadRumbleKeeper* createPadRumbleKeeper(const LiveActor* pActor, s32 port);
}  // namespace al
