#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class CollisionObj; }
class CourseSelectNode;
class CourseSelectNodeParts;
class CourseSelectWallController;
class CourseSelectWall : public al::LiveActor {
public:
    explicit CourseSelectWall(const char* pName);
    ~CourseSelectWall() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void startOpen();
    bool isEndOpen() const;
    void startOpenImmediately();
    void appear() override;
    void kill() override;
    void exeWait();
    void exeAppearPrevEnd();
    void exeAppear();
    void exeAppearNextStart();
    void exeEnd();
private:
    CourseSelectNode* mpNode = nullptr;
    CourseSelectWallController* mpController;
    CourseSelectNodeParts* mpPrevEnd = nullptr;
    CourseSelectNodeParts* mpNextStart = nullptr;
    al::CollisionObj* mpClosedCollision = nullptr;
};
static_assert(sizeof(CourseSelectWall) == 0x170);
