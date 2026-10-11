#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class RouteDokanInOutEffect : public al::LiveActor {
public:
    RouteDokanInOutEffect(const char* pName);
    ~RouteDokanInOutEffect() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void startIn(const sead::Vector3f& rPosition, const sead::Vector3f& rDirection);
    void startOut(const sead::Vector3f& rPosition, const sead::Vector3f& rDirection);
    void start(const sead::Vector3f& rPosition, const sead::Vector3f& rDirection, const char* pReaction);
};
