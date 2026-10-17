#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class GoalPole;
class DemoSceneActorHolder;
namespace al { class WipeSimple; }

/**
 * @brief Bowser demo played after the goal of world 7.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class DemoKoopaW7 : public al::LiveActor {
public:
    explicit DemoKoopaW7(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void setGoalPole(GoalPole* pGoalPole);
    void startDemo();
    bool isEndDemo() const;
    void exeCloseFade();
    void exeWaitFade();
    void exePlay();
    void exeEndFade();
    void exeWaitEnd();
    void exeDone();
    /** @brief Gets the goal pole. @return Goal pole used to order players. */
    GoalPole* getGoalPole() const { return mGoalPole; }

private:
    DemoSceneActorHolder* mDemo = nullptr;
    al::WipeSimple* mCurtain = nullptr;
    al::WipeSimple* mWhiteFade = nullptr;
    GoalPole* mGoalPole = nullptr;
};
