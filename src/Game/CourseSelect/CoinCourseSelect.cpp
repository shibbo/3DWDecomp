#include "CourseSelect/CoinCourseSelect.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/CourseSelectUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

/**
 * @brief Create a collectible coin on the course-select map.
 * @param pName Actor name.
 */
CoinCourseSelect::CoinCourseSelect(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Load the coin, resolve its saved state, and establish its rotation axis.
 * @param rInfo Actor placement and course-select object identification data.
 */
void CoinCourseSelect::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString("Coin"), "CourseSelect");
    al::addTransOffsetLocalDir(this, 70.0f, 1);
    mBaseQuat.set(al::getQuat(this));
    mObjectId = rc::getCourseSelectObjectID(this, rInfo);
    if (rc::isDisappearCourseSelectObject(GameDataHolderAccessor(this), mObjectId)) {
        makeActorDead();
    } else {
        sead::Vector3f up(0.0f, 0.0f, 0.0f);
        al::calcQuatUp(&up, mBaseQuat);
        sead::Vector3f side;
        side.setCross(up, sead::Vector3f::ez);
        if (!al::isNearZero(side, 0.001f)) {
            al::makeQuatUpFront(&mBaseQuat, up, sead::Vector3f::ez);
        }
        makeActorAppeared();
    }
}

/** @brief Apply the shared coin rotation around the placement's up axis. */
void CoinCourseSelect::control() {
    al::rotateQuatYDirDegree(this, mBaseQuat, rc::getCoinRotateY(this));
}

/**
 * @brief Award the coin and persist its acquisition when a player collects it.
 * @param pMsg Sensor message to inspect.
 * @param pSender Collecting player's sensor.
 * @param pReceiver Receiving coin sensor; unused.
 * @return Whether the message was a handled coin-collection request.
 */
bool CoinCourseSelect::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (rc::isMsgCoinGet(pMsg)) {
        al::startHitReactionGet(this);
        rc::acquirerItemCoin(this, pSender);
        rc::acquirerCourseSelectObject(GameDataHolderWriter(this), mObjectId);
        kill();
        return true;
    }
    return false;
}

/** @brief Appear only if this course-select object has not already been collected. */
void CoinCourseSelect::appear() {
    if (!rc::isDisappearCourseSelectObject(GameDataHolderAccessor(this), mObjectId)) {
        al::LiveActor::appear();
    }
}

/** @brief Destroy the course-select coin actor. */
CoinCourseSelect::~CoinCourseSelect() = default;
