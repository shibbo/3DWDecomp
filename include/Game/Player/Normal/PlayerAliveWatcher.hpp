#pragma once

#include <basis/seadTypes.h>

#include "Library/Scene/ISceneObj.hpp"

namespace al {
class ActorInitInfo;
class IUseSceneObjHolder;
class PlayerHolder;
class LiveActor;
class SceneObjHolder;
}  // namespace al

class PlayerActor;
class PlayerAliveWatcherGroup;
class PlayerAmiiboDirectorWatcher;

/// Remembers the actors that currently disable a bubble feature, each for a limited time.
class DisableBubbleRequestHolder {
public:
    /// One actor's request and the frames left until it expires.
    struct Request {
        /**
         * @brief Count the request down and drop it when it expires.
         * @return True if the slot is free afterwards.
         */
        bool update() {
            if (actor == nullptr) {
                return true;
            }

            time--;
            if (time <= 0) {
                actor = nullptr;
                return true;
            }

            return false;
        }

        al::LiveActor* actor = nullptr;  // 0x00
        s32 time = 0;                    // 0x08
    };

    static constexpr s32 cRequestNum = 10;
    static constexpr s32 cRequestTime = 600;

    /**
     * @brief Register (or refresh) a request of an actor. Ignored when every slot is used.
     * @param pActor The requesting actor.
     */
    void setDisableRequest(al::LiveActor* pActor) {
        for (s32 i = 0; i < cRequestNum; i++) {
            if (mRequests[i].actor == pActor) {
                mRequests[i].time = cRequestTime;
                return;
            }

            if (mRequests[i].actor == nullptr) {
                mRequests[i].actor = pActor;
                mRequests[i].time = cRequestTime;
                return;
            }
        }
    }

    /**
     * @brief Counts every request down and drops the expired ones.
     * @return True if no request is left.
     */
    bool update() {
        bool isEmpty = true;

        for (s32 i = 0; i < cRequestNum; i++) {
            isEmpty &= mRequests[i].update();
        }

        return isEmpty;
    }

    /**
     * @brief Drops the request of an actor.
     * @param pActor The requesting actor.
     * @return True if no request is left.
     */
    bool resetDisableRequest(al::LiveActor* pActor) {
        bool isEmpty = true;

        for (s32 i = 0; i < cRequestNum; i++) {
            if (mRequests[i].actor == pActor) {
                mRequests[i].actor = nullptr;
            } else {
                isEmpty &= mRequests[i].actor == nullptr;
            }
        }

        return isEmpty;
    }

private:
    Request mRequests[cRequestNum];  // 0x00
};

static_assert(sizeof(DisableBubbleRequestHolder) == 0xa0);

/// Scene object that revives dead players in bubbles during multiplayer.
class PlayerAliveWatcher : public al::ISceneObj {
public:
    static constexpr s32 cGroupNum = 9;
    static constexpr s32 cDefaultBubbleDelayTime = 180;

    static PlayerAliveWatcher* getPlayerAliveWatcher(const al::IUseSceneObjHolder* pHolder);
    static PlayerAliveWatcher* tryGetPlayerAliveWatcher(const al::IUseSceneObjHolder* pHolder);

    PlayerAliveWatcher(const al::ActorInitInfo& rInfo, al::PlayerHolder* pPlayerHolder,
                       bool isNoLifeDecrease, bool isSingleMode, bool isNoAmiibo);
    void appear();
    void addBubbleDelayTime(s32 frames);
    void startDemo();
    void endDemo();
    void startPause();
    void endPause();
    void onGameOver();
    bool isGameOver() const;
    bool isWaitBubbleForRevive(s32 characterType) const;
    bool isDeactivePlayer(s32 characterType) const;
    void deactivatePlayer(s32 characterType);
    bool isEnableExitStage(s32 characterType) const;
    bool isEnableIslandWarp(s32 characterType) const;
    bool isEnableGyroCamera() const;
    s32 calcAllActivePlayerNum() const;
    bool isEnableBubbleWithInput(PlayerAliveWatcherGroup* pGroup) const;
    PlayerActor* activatePlayer(s32 characterType);
    s32 calcDoubleMarioNum() const;
    void update();
    void setDisableReviveBubble(al::LiveActor* pActor);
    void resetDisableReviveBubble(al::LiveActor* pActor);
    void setDisableBubbleFrameOut(al::LiveActor* pActor);
    void resetDisableBubbleFrameOut(al::LiveActor* pActor);
    s32 isActivePlayerPort(s32 port) const;

    al::SceneObjHolder* mSceneObjHolder;                        // 0x08
    PlayerAliveWatcherGroup** mGroups = nullptr;                // 0x10
    s32 mGroupNum = 0;                                          // 0x18
    DisableBubbleRequestHolder* mReviveBubbleRequests = nullptr;  // 0x20
    DisableBubbleRequestHolder* mFrameOutRequests = nullptr;    // 0x28
    bool mIsEnableBubbleRevive = true;                          // 0x30
    bool mIsEnableBubbleScreenOut = true;                       // 0x31
    bool mIsAbyss = false;                                      // 0x32, last player loss was a fall
    s32 mUpdatedGroupNum = 0;                                   // 0x34
    bool _38 = false;                                           // 0x38
    bool mIsNoLifeDecrease;                                     // 0x39
    bool mIsSingleMode;                                         // 0x3a, guess: no off-screen guide
    PlayerAmiiboDirectorWatcher* mAmiiboDirectorWatcher = nullptr;  // 0x40
    s32 mBubbleDelayTime = cDefaultBubbleDelayTime;             // 0x48
};

static_assert(sizeof(PlayerAliveWatcher) == 0x50);
