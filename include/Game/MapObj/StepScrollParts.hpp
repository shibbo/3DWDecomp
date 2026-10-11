#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class StepScrollParts : public al::LiveActor {
public:
    StepScrollParts(const char* pName);
    ~StepScrollParts() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    bool isOverBound(const sead::Vector3f& direction, const sead::Vector3f& origin);
    void scroll(const sead::Vector3f& direction, float distance);

private:
    sead::Vector3f mLocalClippingCenter = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mClippingCenter = {0.0f, 0.0f, 0.0f};
    float mClippingRadius = 0.0f;
};
