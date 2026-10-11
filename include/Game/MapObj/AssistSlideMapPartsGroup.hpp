#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
class AssistSlideMapParts;
class AssistSlideMapPartsGroup : public al::LiveActor {
public:
    AssistSlideMapPartsGroup(const char*);
    ~AssistSlideMapPartsGroup() override;
    void init(const al::ActorInitInfo&) override;
    void requestTouchAssist();
private:
    al::DeriveActorGroup<AssistSlideMapParts>* mParts = nullptr;
};
