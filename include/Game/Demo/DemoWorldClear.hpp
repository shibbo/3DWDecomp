#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class CourseSelectMiniature;
class DemoSceneActorHolder;
class DemoSkipLayout;
namespace al { class WipeSimple; }

/**
 * @brief World clear demo played at Bowser's castle miniature of the course-select map.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class DemoWorldClear : public al::LiveActor {
public:
    explicit DemoWorldClear(CourseSelectMiniature* pMiniature);
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void startDemo();
    void endDemo();
    bool isEndDemo() const;
    void checkCancel();
    void exeWait();
    void exePlay();
    void exeFade();
    void exeFadeOpenWait();
    void exeOpenRouteDokan();
    void exeEnding();
    void exeCancel();

private:
    CourseSelectMiniature* mMiniature;
    DemoSceneActorHolder* mDemo = nullptr;
    al::WipeSimple* mWipe = nullptr;
    DemoSkipLayout* mSkipLayout = nullptr;
};

static_assert(sizeof(DemoWorldClear) == 0x168);
