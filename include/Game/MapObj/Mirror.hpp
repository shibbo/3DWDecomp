#pragma once

#include "Library/Shader/ForwardRendering/MirrorActorBase.hpp"

class Mirror : public al::MirrorActorBase {
public:
    Mirror(const char* pName);
    ~Mirror() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void startDisappear();
    void kill() override;
    void exeWait();
    void exeDisappear();

private:
    const char* mDisappearAnimName = nullptr;
};
