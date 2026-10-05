#pragma once

namespace al {
class LiveActor;
struct ActorInitInfo;
}

namespace rc {
void initEchoEmitterHolder(const al::LiveActor* pActor, const al::ActorInitInfo& rInfo);
}
