#include "MapObj/Fury/DisasterSpikeBouncy.hpp"

#include <attributes.h>

#include "Library/ActorUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "MapObj/Fury/DisasterSpikeDirector.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Project/Controller/PadRumbleKeeper.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
/// End of the bind after the player got bounced off the spike.
PlayerBindEndParam sBindEndParamBounce = {
    {}, 1, 10, true, true, true, 0, 1.2f, 0, false, {}};

/**
 * @brief Check whether the player pressed jump in the last few frames.
 * @param pPuppet Puppet of the player.
 * @return Whether the jump was triggered at most 4 frames ago.
 */
ALWAYS_INLINE bool isJustJumped(const IUsePlayerPuppet* pPuppet) {
    s32 jumpFrame = rc::getPuppetTrigJumpFrame(pPuppet);
    return jumpFrame >= 0 && jumpFrame <= 4;
}
}  // namespace

/**
 * @brief Construct a bouncy disaster spike.
 * @param pName Name of the actor.
 */
DisasterSpikeBouncy::DisasterSpikeBouncy(const char* pName) : DisasterSpike(pName) {}

/**
 * @brief Initialize the spike, its rumble keeper and the optional linked camera area.
 * @param rInfo Actor init info.
 */
void DisasterSpikeBouncy::init(const al::ActorInitInfo& rInfo) {
    DisasterSpike::init(rInfo);
    mPadRumbleKeeper = al::createPadRumbleKeeper(this, al::getMainControllerPort());
    al::tryGetArg(&mIsKeepPlayerVelocity, rInfo, "IsKeepPlayerVelocity");

    if (al::calcLinkChildNum(rInfo, "CameraArea") > 0) {
        al::PlacementInfo placementInfo;
        al::getLinksInfoByIndex(&placementInfo, rInfo.getPlacementInfo(), "CameraArea", 0);
        al::AreaInitInfo areaInitInfo(placementInfo, rInfo.getStageSwitchDirector());
        mCameraArea = new al::AreaObj("CameraArea");
        mCameraArea->init(areaInitInfo);
        mCameraArea->invalidate();

        al::AreaObjGroup* group = rc::tryFindAreaObjGroup(this, rc::AreaObjType::CameraArea);
        if (group != nullptr) {
            group->resisterAreaObj(mCameraArea);
        }
    }
}

/**
 * @brief Handle a sensor message: binds the player landing on the spike.
 * @param pMsg Received message.
 * @param pSender Sensor that sent the message.
 * @param pReceiver Sensor of this actor that received the message.
 * @return Whether the message was handled.
 */
bool DisasterSpikeBouncy::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                                     al::HitSensor* pReceiver) {
    if (al::isMsgPlayerFloorTouch(pMsg)) {
        rc::requestPlayerBind(pSender, al::getHitSensor(this, "Bind"));
        mIsBouncing = true;
        mIsBounceStarted = true;
        return true;
    }

    if (al::isMsgBindStart(pMsg)) {
        mIsBounceStarted = false;
        if (al::isDead(this)) {
            return false;
        }

        return !(rc::getPlayerVelocity(pSender).y > 0.0f);
    }

    if (al::isMsgBindInit(pMsg)) {
        mBindPlayerSensor = pSender;
        mPlayerSensor = pSender;
        if (mCameraArea != nullptr) {
            mCameraArea->validate();
        }

        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        mBindPlayerSensor = nullptr;
        mIsBouncing = false;
        mIsBounceStarted = false;
        if (mCameraArea != nullptr) {
            mCameraArea->invalidate();
        }

        return true;
    }

    return DisasterSpike::receiveMsg(pMsg, pSender, pReceiver);
}

/**
 * @brief Bounce a bound player up and track when they are back on the ground.
 */
void DisasterSpikeBouncy::control() {
    DisasterSpike::control();

    if (mBindPlayerSensor != nullptr) {
        IUsePlayerPuppet* puppet =
            rc::startPuppet(al::getHitSensor(this, "Bind"), mBindPlayerSensor);
        sead::Vector3f velocity = sead::Vector3f::zero;
        sead::Vector3f jump = sead::Vector3f::ey;
        if (mIsKeepPlayerVelocity) {
            al::verticalizeVec(&velocity, jump, rc::getPuppetVelocity(puppet));
        }

        s32 port = rc::getPlayerInputPort(rc::getPuppetSensor(puppet));
        mPadRumbleKeeper->setPort(port);

        const char* actionName;
        if (rc::isPuppetAction(puppet, "HipDrop") || rc::isPuppetAction(puppet, "GiantHipDrop") ||
            isJustJumped(puppet)) {
            jump *= mDisasterSpikeDirector->getParam().mBouncyJumpHighSpeed;
            rc::startPuppetSe(puppet, "JumpHigh");
            al::startHitReaction(this, "ジャンプ大");
            actionName = "ReactionHigh";
        } else {
            jump *= mDisasterSpikeDirector->getParam().mBouncyJumpSpeed;
            rc::startPuppetSe(puppet, "JumpVoice");
            al::startHitReaction(this, "ジャンプ");
            actionName = "Reaction";
        }

        al::startAction(this, actionName);
        velocity += jump;

        sead::Vector3f dir;
        if (!al::normalizeOrZero(&dir, velocity)) {
            dir.x += (sead::Vector3f::ey.x - dir.x) * 0.5f;
            dir.y += (sead::Vector3f::ey.y - dir.y) * 0.5f;
            dir.z += (sead::Vector3f::ey.z - dir.z) * 0.5f;
            velocity = dir * velocity.length();
        }

        rc::setPuppetVelocity(puppet, velocity);
        rc::endBindAndPuppetNull(&puppet, &sBindEndParamBounce);

        if (rc::isPlayerOnGround(mPlayerSensor)) {
            rc::requestPlayerBind(mBindPlayerSensor, al::getHitSensor(this, "Bind"));
            mBindPlayerSensor = nullptr;
            mIsBounceStarted = true;
            mIsBouncing = true;
        } else {
            mBindPlayerSensor = nullptr;
            mIsBouncing = false;
        }
    } else if (mIsBouncing) {
        if (mIsBounceStarted) {
            mIsBounceStarted = false;
        } else {
            mIsBouncing = false;
        }
    }

    if (mPlayerSensor != nullptr && rc::isPlayerOnGround(mPlayerSensor) && !mIsBouncing) {
        mPlayerSensor = nullptr;
        if (mCameraArea != nullptr) {
            mCameraArea->invalidate();
        }
    }
}

/**
 * @brief Kill the spike and disable its camera area.
 */
void DisasterSpikeBouncy::kill() {
    if (mCameraArea != nullptr) {
        mCameraArea->invalidate();
    }

    DisasterSpike::kill();
}
