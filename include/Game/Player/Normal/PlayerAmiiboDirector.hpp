#pragma once

#include <basis/seadTypes.h>
#include <prim/seadSafeString.h>

#include "Library/Nerve/IUseNerve.hpp"
#include "Player/PlayerDef.hpp"

namespace al {
class ActorInitInfo;
class NerveKeeper;
struct NfpInfo;
}  // namespace al

class AmiiboLayout;
class PlayerActor;
class PlayerAmiiboDirectorWatcher;

/**
 * @brief Per-player amiibo scanning: holding the left button starts a scan on the player's
 * controller and a recognized figure hands out its reward.
 */
class PlayerAmiiboDirector : public al::IUseNerve {
public:
    PlayerAmiiboDirector(const PlayerActor* pPlayer, PlayerAmiiboDirectorWatcher* pWatcher,
                         const al::ActorInitInfo& rInfo);

    static void initRandomSeed();

    void update();
    void reset(bool isDemo);
    void clear();
    EPlayerChara getPlayerChara();
    void startPause(bool isDemo);
    void endPause();
    void setPlayerActor(const PlayerActor* pPlayer);
    bool isScanQueued() const;
    void exeWaitForInput();
    void exeHold();
    void exeScan();
    bool isAmiiboTypeAllowed(const al::NfpInfo* pInfo) const;
    void exePostScan();
    void exeScanFailed();
    bool isKoopaAllowed() const;
    bool isKoopaJrAllowed() const;
    bool isPlayerInKoopaRestrictedArea() const;
    bool isCurrentPlayerActor(const PlayerActor* pActor);

    al::NerveKeeper* getNerveKeeper() const override { return mNerveKeeper; }

private:
    AmiiboLayout* mLayout;                    // 0x08
    const PlayerActor* mPlayer;               // 0x10
    al::ActorInitInfo* mInitInfo;             // 0x18
    al::NerveKeeper* mNerveKeeper;            // 0x20
    bool mIsDuplicate;                        // 0x28, another director already serves the chara
    PlayerAmiiboDirectorWatcher* mWatcher;    // 0x30
    bool mIsBusy;                             // 0x38, recomputed every frame by update()
    bool mIsPause;                            // 0x39
    bool mIsSingleMode;                       // 0x3a
    sead::SafeString mTagIdString;            // 0x40
    char mTagIdBuffer[0x20];                  // 0x50
    sead::SafeString mCharacterIdString;      // 0x70
    char mCharacterIdBuffer[0x10];            // 0x80
    bool mIsScanQueued;                       // 0x90
};
