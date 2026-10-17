#include "NPC/GhostPlayerPlayer.hpp"
#include "NPC/GhostPlayer.hpp"
#include "NPC/GhostPlayerFunction.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/ControlUserUtil.hpp"
#include "math/seadMathCalcCommon.h"
/** @brief Creates the scene playback controller. @param pName Unused placement name; the original
 * uses a fixed actor name. */
GhostPlayerPlayer::GhostPlayerPlayer(const char* pName)
    : al::LiveActor("\u30d7\u30ec\u30a4\u30e4\u30fc\u30b4\u30fc\u30b9\u30c8\u4fdd\u6301"),
      mLoader(nullptr) {}
/** @brief Releases the playback actor. */
GhostPlayerPlayer::~GhostPlayerPlayer() = default;
/** @brief Registers playback with the scene. @param rInfo Placement initialization. */
void GhostPlayerPlayer::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    GhostPlayerFunction::registerGhostPlayerPlayer(this);
    GhostPlayerFunction::isExistGhostPlayerDirector(this);
    makeActorDead();
}
/** @brief Removes excess ghosts or disables playback according to the ghost setting. */
void GhostPlayerPlayer::update() {
    int max = rc::getControlUserNumMax();
    int active = rc::getActiveControlUserNum(GameDataHolderAccessor(this));
    if (max - active < mActivePlayers.size()) {
        mActivePlayers.popBack()->kill();
    }
    if (GameDataFunction::getGhostSetting(GameDataHolderAccessor(this)) == 1) {
        while (!mActivePlayers.isEmpty()) {
            mActivePlayers.popBack()->kill();
        }
        kill();
    }
}
/** @brief Starts ghosts within the available player slots. @return Whether playback is available.
 */
bool GhostPlayerPlayer::tryStartPlay() {
    if (al::isDead(this)) {
        return false;
    }
    if (mActivePlayers.isEmpty()) {
        return false;
    }
    int max = rc::getControlUserNumMax();
    int active = rc::getActiveControlUserNum(GameDataHolderAccessor(this));
    int remove = sead::Mathi::min(sead::Mathi::max(mActivePlayers.size() - (max - active), 0),
                                  mActivePlayers.size());
    for (int i = 0; i < remove; ++i) {
        mActivePlayers.popBack();
    }
    for (auto it = mActivePlayers.begin(); it != mActivePlayers.end(); ++it) {
        it->appear();
    }
    return true;
}
/** @brief Starts ghosts from a recorded object. @param pId Placement identifier. */
void GhostPlayerPlayer::tryStartFromObj(const char* pId) {
    for (auto it = mActivePlayers.begin(); it != mActivePlayers.end(); ++it) {
        it->tryStartFromObj(pId);
    }
}
/** @brief Ends active ghost playback. @return Whether any active ghosts were available. */
bool GhostPlayerPlayer::tryEndPlay() {
    if (al::isDead(this)) {
        return false;
    }
    if (mActivePlayers.isEmpty()) {
        return false;
    }
    for (auto it = mActivePlayers.begin(); it != mActivePlayers.end(); ++it) {
        it->kill();
    }
    return true;
}
/** @brief Hides ghosts while awaiting a warp-object start. @return Whether any active ghosts were
 * available. */
bool GhostPlayerPlayer::waitForStartFromWarpObj() {
    if (al::isDead(this)) {
        return false;
    }
    if (mActivePlayers.isEmpty()) {
        return false;
    }
    for (auto it = mActivePlayers.begin(); it != mActivePlayers.end(); ++it) {
        it->kill();
    }
    return true;
}
/** @brief Stops and hides each active ghost. */
void GhostPlayerPlayer::stopAndHideGhostPlayerAll() {
    for (auto it = mActivePlayers.begin(); it != mActivePlayers.end(); ++it) {
        it->stopAndHide(true);
    }
}
/** @brief Restarts every active ghost. */
void GhostPlayerPlayer::restartGhostPlayerAll() {
    for (auto it = mActivePlayers.begin(); it != mActivePlayers.end(); ++it) {
        it->restart();
    }
}
