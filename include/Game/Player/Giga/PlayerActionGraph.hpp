#pragma once

class PlayerAction;

/// The graph of the player's actions and the shifts between them.
class PlayerActionGraph {
public:
    void init();
    const PlayerAction* getAction() const;
    bool checkShift();
    void checkShiftCondition();
    void move();
    void update();
};
