#pragma once

class PlayerActionGraph;
class PlayerActionNode;
class PlayerLandingChecker;

/// Restarts the player's action graph from a given action.
class PlayerActionGraphRestarter {
public:
    PlayerActionGraphRestarter();
    void restartOnGround();

    void setActionGraph(PlayerActionGraph* pActionGraph) { mActionGraph = pActionGraph; }

    void setRestartNode(PlayerActionNode* pNode) { mRestartNode = pNode; }

    void setLandingChecker(const PlayerLandingChecker* pChecker) { mLandingChecker = pChecker; }

private:
    PlayerActionGraph* mActionGraph;  // 0x0
    PlayerActionNode* mRestartNode;  // 0x8
    const PlayerLandingChecker* mLandingChecker;  // 0x10
};
