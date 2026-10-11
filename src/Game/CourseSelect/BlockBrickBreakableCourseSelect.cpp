#include "CourseSelect/BlockBrickBreakableCourseSelect.hpp"
#include "CourseSelect/CourseSelectFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(BlockBrickBreakableCourseSelect, Wait);
NERVE_DECL(BlockBrickBreakableCourseSelect, Break);
NERVE_DECL(BlockBrickBreakableCourseSelect, Reaction);
NERVES_MAKE_NOSTRUCT(BlockBrickBreakableCourseSelect, Wait)
NERVES_MAKE_STRUCT(BlockBrickBreakableCourseSelect, Break, Reaction)
}  // namespace

/**
 * @brief Constructs the block.
 * @param pName Actor name.
 */
BlockBrickBreakableCourseSelect::BlockBrickBreakableCourseSelect(const char* pName)
    : al::LiveActor(pName) {}

/**
 * @brief Initializes the model, nerve and break model; stays dead if the block was already
 * broken in this save.
 * @param rInfo Actor init info.
 */
void BlockBrickBreakableCourseSelect::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString("BlockBrick"), "CourseSelect");
    al::initNerve(this, &NrvBlockBrickBreakableCourseSelectWait, 0);
    mObjectId = rc::getCourseSelectObjectID(this, rInfo);

    if (rc::isDisappearCourseSelectObject(GameDataHolderAccessor(this), mObjectId)) {
        makeActorDead();
        return;
    }

    mBreakModel = new al::BreakModel(this, "コースセレクトレンガブロック壊れモデル", "BlockBrickBreak",
                                     "CourseSelect", nullptr, "Break", true);
    al::initCreateActorNoPlacementInfo(mBreakModel, rInfo);
    makeActorAppeared();
}

/**
 * @brief Breaks the block on an upper punch (mini players only make it react).
 * @param pMsg Received message.
 * @param pSender Sender sensor.
 * @param pReceiver Receiver sensor.
 * @return Whether the message was handled.
 */
bool BlockBrickBreakableCourseSelect::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                                                 al::HitSensor* pReceiver) {
    if (al::isNerve(this, &NrvBlockBrickBreakableCourseSelect.Break)) {
        return false;
    }

    if (!al::isMsgPlayerUpperPunch(pMsg)) {
        return false;
    }

    if (rc::isPlayerMini(pSender)) {
        al::setNerve(this, &NrvBlockBrickBreakableCourseSelect.Reaction);
        return true;
    }

    al::appearBreakModelRandomRotateY(mBreakModel);
    al::hideModel(this);
    al::invalidateCollisionParts(this);
    al::invalidateClipping(this);
    al::setNerve(this, &NrvBlockBrickBreakableCourseSelect.Break);
    rc::acquirerCourseSelectObject(GameDataHolderWriter(this), mObjectId);
    return true;
}

/**
 * @brief Appears unless the block was already broken in this save.
 */
void BlockBrickBreakableCourseSelect::appear() {
    if (!rc::isDisappearCourseSelectObject(GameDataHolderAccessor(this), mObjectId)) {
        al::LiveActor::appear();
    }
}

/**
 * @brief Idle state.
 */
void BlockBrickBreakableCourseSelect::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
}

/**
 * @brief Bump reaction; returns to Wait when the action ends.
 */
void BlockBrickBreakableCourseSelect::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reaction");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBlockBrickBreakableCourseSelectWait);
    }
}

/**
 * @brief Broken state; the actor is killed a few frames after breaking.
 */
void BlockBrickBreakableCourseSelect::exeBreak() {
    if (al::isStep(this, 5)) {
        kill();
    }
}
