#pragma once

#include <basis/seadTypes.h>

class GameDataHolder;

namespace al {
class LayoutInitInfo;
class NetworkSystem;
}  // namespace al

/**
 * @brief Keeps the wipes (fades, retry wipe, boot wipe...) shown between scenes.
 * @note Only the members used by already-decompiled callers are declared.
 */
class StageWipeKeeper {
public:
    StageWipeKeeper(const al::LayoutInitInfo& rInfo, al::NetworkSystem* pNetworkSystem);

    void closeBootWipe();
    void closeEndingWipe();
    void closeNoResultWipe(const GameDataHolder* pHolder, bool isSkip);
    void closeNoResultWipeGameOver();
    void closeNoResultWipeGameEnd();
    void closeNoResultWipeTitle();
    void closeNoResultWipeRestartTitleWhiteFade(bool isSkip);
    bool isCloseEndNoResultWipe() const;
    bool isActiveNoResultWipe() const;
    bool isActiveBootWipe() const;
    bool isActiveStartWipe() const;
    bool isActiveRetryWipe() const;
    void closeWipeFadeBlack(s32 frame);
    bool isCloseEndFadeBlack() const;
    void closeRetryWipe(bool isSkip);
    bool isCloseEndRetryWipe() const;
    void openRetryWipe();
    void tryOpenFadeBlack();
    void tryOpenStartOrRetryWipe();
    void tryOpenNoResultWipe();

private:
    u8 _0[0x80];  ///< Not reconstructed yet.
};

static_assert(sizeof(StageWipeKeeper) == 0x80);
