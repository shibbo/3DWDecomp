#pragma once
#include "Library/Obj/EffectObj.hpp"

class EffectObjBattleArenaBunBun : public al::EffectObj {
public:
    EffectObjBattleArenaBunBun(const char* pName);
    ~EffectObjBattleArenaBunBun() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    void exeAppeared();
    void exeWait();
};
