#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TuccondorGlass : public al::LiveActor {
public:
    explicit TuccondorGlass(const char* pName);
    ~TuccondorGlass() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    void appearBlow(const sead::Vector3f& rDirection);
    void exeWait();
    void updateVelocity();
    void exeEnd();
    bool isReady();
    bool isEnd();
};
