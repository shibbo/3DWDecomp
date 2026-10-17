#include "NPC/GhostPlayerPlayer.hpp"
#include "NPC/GhostPlayer.hpp"
#include "NPC/GhostPlayerFunction.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include <math/seadMathCalcCommon.h>

/** @brief Creates the replay controller and its fixed-capacity actor lists. */
GhostPlayerPlayer::GhostPlayerPlayer(const char*) : al::LiveActor("プレイヤーゴースト保持") {}

void GhostPlayerPlayer::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    GhostPlayerFunction::registerGhostPlayerPlayer(this);
    GhostPlayerFunction::isExistGhostPlayerDirector(this);
    makeActorDead();
}

/** @brief Removes excess ghosts or disables playback when the setting is off. */
void GhostPlayerPlayer::update() {
    int maxGhosts = rc::getControlUserNumMax() - rc::getActiveControlUserNum(GameDataHolderAccessor(this));
    if (maxGhosts < mActiveGhosts.size())
        mActiveGhosts.popBack()->kill();
    if (GameDataFunction::getGhostSetting(GameDataHolderAccessor(this)) == 1) {
        while (!mActiveGhosts.isEmpty())
            mActiveGhosts.popBack()->kill();
        kill();
    }
}

/** @brief Starts available ghosts after limiting their count to unused player slots. */
bool GhostPlayerPlayer::tryStartPlay() {
    if (al::isDead(this))
        return false;
    if (mActiveGhosts.isEmpty())
        return false;
    int maxGhosts = rc::getControlUserNumMax() - rc::getActiveControlUserNum(GameDataHolderAccessor(this));
    int removeCount = sead::Mathi::min(sead::Mathi::max(mActiveGhosts.size() - maxGhosts, 0),
                                       mActiveGhosts.size());
    for (int i = 0; i < removeCount; ++i)
        mActiveGhosts.popBack();
    for (auto it = mActiveGhosts.begin(); it != mActiveGhosts.end(); ++it)
        it->appear();
    return true;
}

void GhostPlayerPlayer::tryStartFromObj(const char* pName) {
    for (auto it = mActiveGhosts.begin(); it != mActiveGhosts.end(); ++it)
        it->tryStartFromObj(pName);
}

bool GhostPlayerPlayer::tryEndPlay() {
    if (al::isDead(this) || mActiveGhosts.isEmpty())
        return false;
    for (auto it = mActiveGhosts.begin(); it != mActiveGhosts.end(); ++it)
        it->kill();
    return true;
}

bool GhostPlayerPlayer::waitForStartFromWarpObj() {
    if (al::isDead(this) || mActiveGhosts.isEmpty())
        return false;
    for (auto it = mActiveGhosts.begin(); it != mActiveGhosts.end(); ++it)
        it->kill();
    return true;
}

void GhostPlayerPlayer::stopAndHideGhostPlayerAll() {
    for (auto it = mActiveGhosts.begin(); it != mActiveGhosts.end(); ++it)
        it->stopAndHide(true);
}

void GhostPlayerPlayer::restartGhostPlayerAll() {
    for (auto it = mActiveGhosts.begin(); it != mActiveGhosts.end(); ++it)
        it->restart();
}
