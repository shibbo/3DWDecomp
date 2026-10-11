#include "MapObj/BlockChoice.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ItemUtil.hpp"

namespace {
const char* const sItemNames[3][4] = {
    {"スーパーベル", "スーパーキノコ", "スーパーキノコ", "ファイアフラワー"},
    {"スーパーベル", "ファイアフラワー", "スーパーキノコ", "スーパーこのは"},
    {"スーパーベル", "ファイアフラワー", "スーパーこのは", "ブーメランフラワー"},
};
NERVE_DECL(BlockChoice, Wait);
NERVE_DECL(BlockChoice, React);
NERVE_DECL(BlockChoice, ReactHipDrop);
NERVES_MAKE_STRUCT(BlockChoice, Wait, React, ReactHipDrop)
}

BlockChoice::BlockChoice(const char* name) : al::LiveActor(name) {}
BlockChoice::~BlockChoice() {}

void BlockChoice::init(const al::ActorInitInfo& info) {
    al::initMapPartsActor(this, info, nullptr, 0);
    if (al::tryGetArg(&mItemType, info, "ItemType")) {
        mIsBig = al::isEqualString(al::getModelName(this), "BlockChoiceBig");
        al::initNerve(this, &NrvBlockChoice.Wait, 0);
        al::calcFrontDir(&mFront, this);
        makeActorAppeared();
    } else {
        makeActorDead();
    }
}

void BlockChoice::attackSensor(al::HitSensor*, al::HitSensor*) {}

bool BlockChoice::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvBlockChoice.Wait) &&
        (rc::isMsgForBlockAll(msg, sender, receiver, mIsBig ? 200.0f : 100.0f) ||
         al::isMsgPlayerObjHipDropAll(msg))) {
        if (al::isMsgPlayerUpperPunch(msg)) {
            mIsActivated = true;
            al::setNerve(this, &NrvBlockChoice.React);
            return true;
        }
        if (al::isMsgPlayerObjHipDropAll(msg)) {
            mIsActivated = true;
            al::setNerve(this, &NrvBlockChoice.ReactHipDrop);
            return true;
        }
        if (al::isMsgPlayerBoomerangAttackCollide(msg) || al::isMsgPlayerClimbAttack(msg) ||
            al::isMsgPlayerTailAttack(msg) || al::isMsgPlayerBodyAttack(msg) ||
            al::isMsgPlayerSpinAttack(msg)) {
            mIsActivated = true;
            al::setNerve(this, &NrvBlockChoice.React);
            return true;
        }
    }
    return false;
}

bool BlockChoice::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvBlockChoice.Wait) && al::isMsgTouchAssistTrig(msg)) {
        mIsActivated = true;
        al::setNerve(this, &NrvBlockChoice.React);
        return true;
    }
    return false;
}

void BlockChoice::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
    mRotateY += 1.2f;
    if (mRotateY > 360.0f)
        mRotateY -= 360.0f;
    al::setRotateY(this, mRotateY);
}

void BlockChoice::exeReact() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reaction");
        al::invalidateCollisionParts(this);
    }
    if (al::isActionEnd(this))
        kill();
}

void BlockChoice::exeReactHipDrop() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ReactionHipDrop");
        al::invalidateCollisionParts(this);
    }
    if (al::isActionEnd(this))
        kill();
}

void BlockChoice::appearItemAll(bool isWinner) {
    if (isWinner) {
        {
            sead::Vector3f direction = mFront;
            sead::Vector3f trans = al::getTrans(this);
            al::rotateVectorDegreeY(&direction, -37.5f);
            trans.x += -75.0f;
            al::rotateVectorDegreeX(&direction, 0.0f);
            if (mIsBig)
                trans.y += 225.0f;
            else
                trans.y += 150.0f;
            al::appearItemTiming(this, sItemNames[mItemType][0], trans, direction);
        }
        {
            sead::Vector3f direction = mFront;
            sead::Vector3f trans = al::getTrans(this);
            al::rotateVectorDegreeY(&direction, -12.5f);
            trans.x += -25.0f;
            al::rotateVectorDegreeX(&direction, 0.0f);
            if (mIsBig)
                trans.y += 225.0f;
            else
                trans.y += 150.0f;
            al::appearItemTiming(this, sItemNames[mItemType][1], trans, direction);
        }
        {
            sead::Vector3f direction = mFront;
            sead::Vector3f trans = al::getTrans(this);
            al::rotateVectorDegreeY(&direction, 12.5f);
            trans.x += 25.0f;
            al::rotateVectorDegreeX(&direction, 0.0f);
            if (mIsBig)
                trans.y += 225.0f;
            else
                trans.y += 150.0f;
            al::appearItemTiming(this, sItemNames[mItemType][2], trans, direction);
        }
        {
            sead::Vector3f direction = mFront;
            sead::Vector3f trans = al::getTrans(this);
            al::rotateVectorDegreeY(&direction, 37.5f);
            trans.x += 75.0f;
            al::rotateVectorDegreeX(&direction, 0.0f);
            if (mIsBig)
                trans.y += 225.0f;
            else
                trans.y += 150.0f;
            al::appearItemTiming(this, sItemNames[mItemType][3], trans, direction);
        }
        al::startHitReaction(this, "あたり");
        al::startSequenceBgm(this, "KinopioHouseBingo", 0, 7);
    } else {
        if (mIsBig)
            al::setAppearItemOffset(this, sead::Vector3f::ey * 225.0f);
        else
            al::setAppearItemOffset(this, sead::Vector3f::ey * 150.0f);
        al::appearItemTiming(this, "スーパーキノコ");
        al::startHitReaction(this, "はずれ");
    }
}

void BlockChoice::setDisappear() {
    al::startHitReactionDisappear(this);
    kill();
}
