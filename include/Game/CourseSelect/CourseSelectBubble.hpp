#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class CourseSelectBubble : public al::LiveActor {
public:
    explicit CourseSelectBubble(al::LiveActor* pPlayer);
    ~CourseSelectBubble() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void startAppear();
    void startBurst();
    void exeAppear();
    void exeWait();
    void exeBurst();
    void exeHide();

private:
    al::LiveActor* mpPlayer;
};

static_assert(sizeof(CourseSelectBubble) == 0x150);
