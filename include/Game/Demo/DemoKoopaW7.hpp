#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class GoalPole;

/**
 * @brief Bowser demo played after the goal of world 7.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class DemoKoopaW7 : public al::LiveActor {
public:
    explicit DemoKoopaW7(const char* pName);

    void setGoalPole(GoalPole* pGoalPole);
    void startDemo();
    bool isEndDemo() const;
};
