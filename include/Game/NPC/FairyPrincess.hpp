#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

/**
 * @brief One of the sprixie princesses (妖精プリンセス) shown at goal poles and castle clears.
 */
class FairyPrincess : public al::LiveActor {
public:
    FairyPrincess(const char* pName);

    void initFairyWithArchiveName(const al::ActorInitInfo& rInfo, const char* pArchiveName,
                                  const char* pActionName);
    void startDemoAction(const char* pActionName, const al::LiveActor* pHost);
    void createBottle(const al::ActorInitInfo& rInfo);
    bool tryBreakBottle();
    al::LiveActor* getWingActor() const;
    al::LiveActor* getBottleActor() const;
    al::LiveActor* getBottleInnerActor() const;
    al::LiveActor* getBottleCapActor() const;

    /**
     * @brief Checks whether the princess is trapped in a bottle.
     * @return Whether a bottle was created.
     */
    bool isExistBottle() const { return mBottle != nullptr; }

    /**
     * @brief Sets whether the bottled princess reacts to the player.
     * @param isEnable Whether the reaction is enabled.
     */
    void setEnableBottleReaction(bool isEnable) { mIsEnableBottleReaction = isEnable; }

private:
    al::LiveActor* mBottle;  // 0x148
    u8 mUnreconstructed150[0x49];
    bool mIsEnableBottleReaction;  // 0x199
};

static_assert(sizeof(FairyPrincess) == 0x1a0);
