#include "CourseSelect/BlockQuestionCourseSelect.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/BlockEmptyCourseSelect.hpp"
#include "MapObj/CoinCountUp.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/CourseSelectUtil.hpp"

namespace {
NERVE_DECL(BlockQuestionCourseSelect, Wait);
NERVE_DECL(BlockQuestionCourseSelect, AppearItem);
NERVE_DECL(BlockQuestionCourseSelect, End);
NERVES_MAKE_NOSTRUCT(BlockQuestionCourseSelect, Wait, AppearItem, End)
}

/**
 * @brief Create a course-select question block.
 * @param pName Actor name.
 */
BlockQuestionCourseSelect::BlockQuestionCourseSelect(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Restore the block's contents and create its empty replacement and coin.
 * @param rInfo Actor placement and scene initialization data.
 */
void BlockQuestionCourseSelect::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString("BlockQuestion"), "CourseSelect");
    al::initNerve(this, &NrvBlockQuestionCourseSelectWait, 0);
    mObjectId = rc::getCourseSelectObjectID(this, rInfo);
    mpEmptyBlock = new BlockEmptyCourseSelect("コース選択空ブロック");
    al::initCreateActorWithPlacementInfo(mpEmptyBlock, rInfo);
    if (rc::isDisappearCourseSelectObject(GameDataHolderAccessor(this), mObjectId)) {
        mpEmptyBlock->makeActorAppeared();
        makeActorDead();
    } else {
        mpEmptyBlock->makeActorDead();
        makeActorAppeared();
        mpCoin = new CoinCountUp("カウントアップ用コイン");
        al::initCreateActorWithPlacementInfo(mpCoin, rInfo);
    }
}

/**
 * @brief Release and award the coin on the first accepted punch.
 * @param pMsg Sensor message to inspect.
 * @param pSender Punching player's sensor.
 * @param pReceiver Receiving block sensor; unused.
 * @return Whether a punch was accepted.
 */
bool BlockQuestionCourseSelect::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                                         al::HitSensor* pReceiver) {
    if (!al::isMsgPlayerUpperPunch(pMsg))
        return false;
    if (al::isNerve(this, &NrvBlockQuestionCourseSelectAppearItem))
        return false;
    if (al::isNerve(this, &NrvBlockQuestionCourseSelectEnd))
        return false;
    al::setNerve(this, &NrvBlockQuestionCourseSelectAppearItem);
    mpCoin->appearQuick();
    al::startSe(mpCoin, "PgGetDelay", nullptr);
    al::acquirerItem(this, pSender, "コインx1[自動取得]");
    rc::acquirerCourseSelectObject(GameDataHolderWriter(this), mObjectId);
    return true;
}

/** @brief Appear only if the block's coin has not already been collected. */
void BlockQuestionCourseSelect::appear() {
    if (!rc::isDisappearCourseSelectObject(GameDataHolderAccessor(this), mObjectId))
        al::LiveActor::appear();
}

/** @brief Start the idle action on entering the waiting state. */
void BlockQuestionCourseSelect::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
}

/** @brief Play the punch reaction and replace the block with its empty version. */
void BlockQuestionCourseSelect::exeAppearItem() {
    if (al::isFirstStep(this))
        al::startAction(this, "Reaction");
    if (al::isActionEnd(this)) {
        al::copyPose(mpEmptyBlock, this);
        mpEmptyBlock->appear();
        al::setNerve(this, &NrvBlockQuestionCourseSelectEnd);
    }
}

/** @brief Remove the spent question block. */
void BlockQuestionCourseSelect::exeEnd() {
    kill();
}

/** @brief Destroy the course-select question block. */
BlockQuestionCourseSelect::~BlockQuestionCourseSelect() = default;
