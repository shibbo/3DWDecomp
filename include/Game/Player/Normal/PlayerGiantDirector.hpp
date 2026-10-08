#pragma once

#include <basis/seadTypes.h>

class IUsePlayerEventReceiver;
class PlayerActionGraph;
class PlayerActionNode;
class PlayerConstParam;
struct PlayerProperty;

/// Runs the player's giant (mega mushroom) form.
class PlayerGiantDirector {
public:
    PlayerGiantDirector(PlayerProperty* pProperty, const PlayerConstParam* pConstParam,
                        IUsePlayerEventReceiver* pEventReceiver);
    void update();
    void start();
    void end();
    void forceEnd();

    bool isGiant() const { return mTimer > 0 || mIsGiant; }

    /** @brief Tests whether only the timer still keeps the giant form. @return True if running out. */
    bool isRunningOut() const { return mTimer >= 1 && !mIsGiant; }

    /** @brief Gets the remaining giant frames. @return Remaining frames. */
    u32 getTimer() const { return mTimer; }

    u32 getChangeFrame() const;

    /**
     * @brief Sets the action graph and the nodes the giant form shifts to.
     * @param pActionGraph The player's action graph.
     * @param pStartNode The node played when the giant form starts.
     * @param pEndNode The node played when the giant form ends.
     */
    void setAction(PlayerActionGraph* pActionGraph, PlayerActionNode* pStartNode,
                   PlayerActionNode* pEndNode) {
        mActionGraph = pActionGraph;
        mStartNode = pStartNode;
        mEndNode = pEndNode;
    }

private:
    u8 _0[0x18];
    bool mIsGiant;  // 0x18
    s32 mTimer;  // 0x1c
    PlayerActionGraph* mActionGraph;  // 0x20
    PlayerActionNode* mStartNode;  // 0x28
    PlayerActionNode* mEndNode;  // 0x30
    u8 _38[0x40 - 0x38];
};
