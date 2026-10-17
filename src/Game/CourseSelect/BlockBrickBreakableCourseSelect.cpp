#include "CourseSelect/BlockBrickBreakableCourseSelect.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/CourseSelectUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(BlockBrickBreakableCourseSelect, Wait);
NERVE_DECL(BlockBrickBreakableCourseSelect, Reaction);
NERVE_DECL(BlockBrickBreakableCourseSelect, Break);
NERVES_MAKE_NOSTRUCT(BlockBrickBreakableCourseSelect, Wait, Reaction, Break)
}

/**
 * @brief Create a breakable course-select brick block.
 * @param pName Actor name.
 */
BlockBrickBreakableCourseSelect::BlockBrickBreakableCourseSelect(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Restore the block's saved state and create its debris model.
 * @param rInfo Actor placement and scene initialization data.
 */
void BlockBrickBreakableCourseSelect::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString("BlockBrick"), "CourseSelect");
    al::initNerve(this, &NrvBlockBrickBreakableCourseSelectWait, 0);
    mObjectId = rc::getCourseSelectObjectID(this, rInfo);
    if (rc::isDisappearCourseSelectObject(GameDataHolderAccessor(this), mObjectId)) {
        makeActorDead();
    } else {
        mpBreakModel = new al::BreakModel(this, "コースセレクトレンガブロック壊れモデル", "BlockBrickBreak",
                                         "CourseSelect", nullptr, "Break", true);
        al::initCreateActorNoPlacementInfo(mpBreakModel, rInfo);
        makeActorAppeared();
    }
}

/**
 * @brief React to a small player's punch or break and save the block's removal.
 * @param pMsg Sensor message to inspect.
 * @param pSender Punching player's sensor.
 * @param pReceiver Receiving block sensor; unused.
 * @return Whether the punch was handled.
 */
bool BlockBrickBreakableCourseSelect::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                                               al::HitSensor* pReceiver) {
    if (al::isNerve(this, &NrvBlockBrickBreakableCourseSelectBreak))
        return false;
    if (al::isMsgPlayerUpperPunch(pMsg)) {
        if (rc::isPlayerMini(pSender)) {
            al::setNerve(this, &NrvBlockBrickBreakableCourseSelectReaction);
        } else {
            al::appearBreakModelRandomRotateY(mpBreakModel);
            al::hideModel(this);
            al::invalidateCollisionParts(this);
            al::invalidateClipping(this);
            al::setNerve(this, &NrvBlockBrickBreakableCourseSelectBreak);
            rc::acquirerCourseSelectObject(GameDataHolderWriter(this), mObjectId);
        }
        return true;
    }
    return false;
}

/** @brief Appear only if this block has not already been broken. */
void BlockBrickBreakableCourseSelect::appear() {
    if (!rc::isDisappearCourseSelectObject(GameDataHolderAccessor(this), mObjectId))
        al::LiveActor::appear();
}

/** @brief Start the idle action when entering the waiting state. */
void BlockBrickBreakableCourseSelect::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
}

/** @brief Play the punch reaction and return to the waiting state. */
void BlockBrickBreakableCourseSelect::exeReaction() {
    if (al::isFirstStep(this))
        al::startAction(this, "Reaction");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvBlockBrickBreakableCourseSelectWait);
}

/** @brief Remove the block five steps after it breaks. */
void BlockBrickBreakableCourseSelect::exeBreak() {
    if (al::isStep(this, 5))
        kill();
}

/** @brief Destroy the course-select brick block. */
BlockBrickBreakableCourseSelect::~BlockBrickBreakableCourseSelect() = default;
