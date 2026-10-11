#pragma once

#include <basis/seadTypes.h>

namespace al {
class IUseEffectKeeper;

class ActionEffectCtrl {
public:
    static ActionEffectCtrl* tryCreate(IUseEffectKeeper* pEffectKeeper);

    void startAction(const char* pActionName);
    void update(f32 frame, f32 frameRate);
    void stopActionAndKillParticles();
};
}  // namespace al
