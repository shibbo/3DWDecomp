#pragma once

/**
 * @brief Interface for objects that want to know when a goal item finished its appear animation.
 */
class IGoalItemListener {
public:
    /**
     * @brief Called once the goal item's animation is done.
     */
    virtual void goalItemAnimIsDone() = 0;
};
