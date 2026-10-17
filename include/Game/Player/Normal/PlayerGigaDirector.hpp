#pragma once

#include <basis/seadTypes.h>

namespace al {
class LiveActor;
}

class IUsePlayerCollision;
class IUsePlayerEventReceiver;
class IUsePlayerInput;
class PlayerActionGraph;
class PlayerActionNode;
class PlayerFigureDirector;
class PlayerSimpleFlag;
struct PlayerProperty;

/// Runs the player's giga form (giga bell).
class PlayerGigaDirector {
public:
    PlayerGigaDirector(const IUsePlayerCollision* pCollision, const IUsePlayerInput* pInput,
                       PlayerProperty* pProperty, IUsePlayerEventReceiver* pEventReceiver,
                       PlayerFigureDirector* pFigureDirector, const PlayerSimpleFlag* pFlag);
    void update();
    void start(const al::LiveActor* pBell, bool isClimb, bool isFirst);
    f32 getScaleRate() const;
    f32 getScaleMax() const;
    void forceEnd();
    void resetScale();
    bool isFullScale() const;

    /// Whether the giga form is kept until told otherwise (see stay()).
    bool isStaying() const { return mTimer < 0; }

    bool isGiga() const { return mTimer > 0 || mIsGiga; }

    /// Keep the giga form until told otherwise.
    void stay(bool isResetEndTimer) {
        mTimer = -1;

        if (isResetEndTimer) {
            mEndTimer = 60;
        }
    }

    /// Let the giga form run out again.
    void unstay(bool isResetEndTimer) {
        if (isGiga()) {
            mTimer = 50000;
        } else {
            mTimer = 0;
        }

        if (isResetEndTimer) {
            mEndTimer = 60;
        }
    }

    /**
     * @brief Sets the action graph and the nodes the giga form shifts to.
     * @param pActionGraph The player's action graph.
     * @param pStartNode The node played when the giga form starts.
     * @param pEndNode The node played when the giga form ends.
     * @param pClimbStartNode The node played when the giga form starts while climbing.
     */
    void setAction(PlayerActionGraph* pActionGraph, PlayerActionNode* pStartNode,
                   PlayerActionNode* pEndNode, PlayerActionNode* pClimbStartNode) {
        mActionGraph = pActionGraph;
        mStartNode = pStartNode;
        mEndNode = pEndNode;
        mClimbStartNode = pClimbStartNode;
    }

private:
    u8 _0[0x28];
    bool mIsGiga;  // 0x28
    s32 mTimer;  // 0x2c
    s32 mEndTimer;  // 0x30
    PlayerActionGraph* mActionGraph;  // 0x38
    PlayerActionNode* mStartNode;  // 0x40
    PlayerActionNode* mEndNode;  // 0x48
    PlayerActionNode* mClimbStartNode;  // 0x50
    u8 _58[0x70 - 0x58];
};
