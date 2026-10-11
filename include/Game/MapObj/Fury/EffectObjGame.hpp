#pragma once

#include "Library/Obj/EffectObj.hpp"

namespace rc {
class EffectObjGame : public al::EffectObj {
public:
    EffectObjGame(const char* pName);
    ~EffectObjGame() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void kill() override;

private:
    bool mStopInDisasterMode = false;
    bool mIsDisasterMode = false;
    bool mClearEffectOnKill = false;
    bool mIsSingleMode = false;
};
}
