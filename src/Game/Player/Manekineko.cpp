#include "Player/Manekineko.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"
#include "Player/PlayerDef.hpp"
#include "Project/Base/StringUtil.hpp"

/**
 * Creates the statue actor that follows the player's lucky-cat model.
 * @param rInfo the actor init info
 * @param pModelName the player model name the statue archive is named after
 * @param pModelHolder the player's model holder
 * @param pCollision the player's collision, used to detect landing
 * @param pSensor the sensor credited for dropped coins
 */
Manekineko::Manekineko(const al::ActorInitInfo& rInfo, const char* pModelName,
                       const PlayerModelHolder* pModelHolder,
                       const IUsePlayerCollision* pCollision, const al::HitSensor* pSensor)
    : al::LiveActor("まねきねこ"), mModelHolder(pModelHolder), mCollision(pCollision),
      mSensor(pSensor) {
    al::StringTmp<128> archiveName("%sClimbStatue", pModelName);
    al::initActorWithArchiveName(this, rInfo, archiveName.cstr(), nullptr);
    al::startAction(this, "Wait");
    mBaseMtx = pModelHolder->getModel(EPlayerFigure::Manekineko)->getBaseMtx();
    makeActorDead();
}

/**
 * Snaps the statue to the player model and starts tracking the fall height.
 */
void Manekineko::appear() {
    al::updatePoseMtx(this, mBaseMtx);
    mCoinDropY = al::getTrans(this).y;
    al::startAction(this, "Wait");
    al::LiveActor::appear();
}

/**
 * Follows the player model and drops a coin for every fixed distance fallen.
 */
void Manekineko::control() {
    al::updatePoseMtx(this, mBaseMtx);

    if (mCollision->isOnFloor()) {
        mCoinDropY = al::getTrans(this).y;
    } else if (mCoinDropY - al::getTrans(this).y > 80.0f) {
        al::setAppearItemAttackerSensor(this, mSensor);
        al::appearItemTiming(this, "コイン", al::getTrans(this) + sead::Vector3f::ey * 100.0f,
                             sead::Vector3f::ey);
        mCoinDropY -= 80.0f;
    }
}
