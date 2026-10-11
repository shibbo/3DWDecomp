#include "Enemy/KillerLauncher.hpp"
#include "Enemy/Killer.hpp"
#include "Enemy/KillerGenerator.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"

namespace {
const sead::Vector3f cNormalOffset(0.0f, 76.0f, 30.0f);
const sead::Vector3f cMagnumOffset(0.0f, 300.0f, 50.0f);
}

/** @brief Constructs a Bullet Bill launcher.
 * @param pName Actor name.
 */
KillerLauncher::KillerLauncher(const char* pName) : al::LiveActor(pName) {}

/** @brief Creates the selected projectile generator and optional collision attachment.
 * @param rInfo Actor placement and scene information.
 */
void KillerLauncher::init(const al::ActorInitInfo& rInfo) {
    // Preserve the original stack-backed variant selection.
    volatile int type = 0;
    if (al::isObjectName(rInfo, "KillerSearchLauncher")) {
        type = 1;
    } else if (al::isObjectName(rInfo, "KillerMagnumLauncher")) {
        type = 2;
    } else if (al::isObjectName(rInfo, "KillerMagnumSearchLauncher")) {
        type = 3;
    }
    const char* pModelName = "KillerLauncher";
    if (!alPlacementFunction::tryGetModelName(&pModelName, rInfo) ||
        al::isEqualString("", pModelName)) {
        if (Killer::isMagnum(type)) {
            pModelName = "KillerMagnumLauncher";
        } else {
            pModelName = "KillerLauncher";
        }
    }
    al::initActorWithArchiveName(this, rInfo, pModelName, nullptr);
    mGeneratorOffset.set(cNormalOffset);
    if (Killer::isMagnum(type)) {
        mGeneratorOffset.set(cMagnumOffset);
    }
    bool isConnectCollision = false;
    if (al::tryGetArg(&isConnectCollision, rInfo, "IsConnectCollision") && isConnectCollision) {
        mConnector = al::createMtxConnector(this);
    }
    mFilter = new al::CollisionPartsFilterActor(this);
    bool isHideModel = false;
    if (al::tryGetArg(&isHideModel, rInfo, "IsHideModel") && isHideModel) {
        al::hideModel(this);
    }
    mGenerator = new KillerGenerator("キラージェネレータ", this, type, mFilter);
    al::initCreateActorWithPlacementInfo(mGenerator, rInfo);
    updatePoseGenerator();
    makeActorAppeared();
}

/** @brief Places the generator at the launcher muzzle and copies its orientation. */
void KillerLauncher::updatePoseGenerator() {
    sead::Vector3f trans(0.0f, 0.0f, 0.0f);
    al::calcTransLocalOffset(&trans, this, mGeneratorOffset);
    mGenerator->updateQT(trans, al::getQuat(this));
}

/** @brief Attaches the optional connector to nearby collision. */
void KillerLauncher::initAfterPlacement() {
    if (mConnector) {
        al::attachMtxConnectorToCollision(mConnector, this, false);
    }
}

/** @brief Follows moving collision and updates the generator pose. */
void KillerLauncher::control() {
    if (mConnector) {
        al::connectPoseQT(this, mConnector);
    }
    updatePoseGenerator();
}
