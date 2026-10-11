#include "CourseSelect/BlockQuestionCourseSelect.hpp"
#include "CourseSelect/CourseSelectFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/BlockEmptyCourseSelect.hpp"
#include "MapObj/CoinCountUp.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"

namespace {
NERVE_DECL(BlockQuestionCourseSelect, Wait);
NERVE_DECL(BlockQuestionCourseSelect, AppearItem);
NERVE_DECL(BlockQuestionCourseSelect, End);
NERVES_MAKE_NOSTRUCT(BlockQuestionCourseSelect, Wait)
NERVES_MAKE_STRUCT(BlockQuestionCourseSelect, AppearItem, End)
}  // namespace

/**
 * @brief Constructs the block.
 * @param pName Actor name.
 */
BlockQuestionCourseSelect::BlockQuestionCourseSelect(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the model and nerve, and creates the empty block and the count-up coin.
 * If the block was already hit in this save, only the empty block is shown.
 * @param rInfo Actor init info.
 */
void BlockQuestionCourseSelect::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString("BlockQuestion"), "CourseSelect");
    al::initNerve(this, &NrvBlockQuestionCourseSelectWait, 0);
    mObjectId = rc::getCourseSelectObjectID(this, rInfo);

    mEmptyBlock = new BlockEmptyCourseSelect("コース選択空ブロック");
    al::initCreateActorWithPlacementInfo(mEmptyBlock, rInfo);

    if (rc::isDisappearCourseSelectObject(GameDataHolderAccessor(this), mObjectId)) {
        mEmptyBlock->makeActorAppeared();
        makeActorDead();
        return;
    }

    mEmptyBlock->makeActorDead();
    makeActorAppeared();
    mCoin = new CoinCountUp("カウントアップ用コイン");
    al::initCreateActorWithPlacementInfo(mCoin, rInfo);
}

/**
 * @brief Pops out a coin on an upper punch and marks the block as used in the save.
 * @param pMsg Received message.
 * @param pSender Sender sensor.
 * @param pReceiver Receiver sensor.
 * @return Whether the message was handled.
 */
bool BlockQuestionCourseSelect::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                                           al::HitSensor* pReceiver) {
    if (!al::isMsgPlayerUpperPunch(pMsg)) {
        return false;
    }

    if (al::isNerve(this, &NrvBlockQuestionCourseSelect.AppearItem)) {
        return false;
    }

    if (al::isNerve(this, &NrvBlockQuestionCourseSelect.End)) {
        return false;
    }

    al::setNerve(this, &NrvBlockQuestionCourseSelect.AppearItem);
    mCoin->appearQuick();
    al::startSe(mCoin, "PgGetDelay");
    al::acquirerItem(this, pSender, "コインx1[自動取得]");
    rc::acquirerCourseSelectObject(GameDataHolderWriter(this), mObjectId);
    return true;
}

/**
 * @brief Appears unless the block was already used in this save.
 */
void BlockQuestionCourseSelect::appear() {
    if (!rc::isDisappearCourseSelectObject(GameDataHolderAccessor(this), mObjectId)) {
        al::LiveActor::appear();
    }
}

/**
 * @brief Idle state.
 */
void BlockQuestionCourseSelect::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
}

/**
 * @brief Plays the bump reaction, then swaps in the empty block at the same pose.
 */
void BlockQuestionCourseSelect::exeAppearItem() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reaction");
    }

    if (al::isActionEnd(this)) {
        al::copyPose(mEmptyBlock, this);
        mEmptyBlock->appear();
        al::setNerve(this, &NrvBlockQuestionCourseSelect.End);
    }
}

/**
 * @brief Final state; the question block is removed.
 */
void BlockQuestionCourseSelect::exeEnd() {
    kill();
}
