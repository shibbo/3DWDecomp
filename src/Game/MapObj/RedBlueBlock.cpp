#include "MapObj/RedBlueBlock.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
    NERVE_DECL(RedBlueBlock, Wait);
    NERVE_DECL(RedBlueBlock, Flip);
    NERVES_MAKE_NOSTRUCT(RedBlueBlock, Wait, Flip)
}

RedBlueBlock::RedBlueBlock(const char* pName) : al::LiveActor(pName) {}
RedBlueBlock::~RedBlueBlock() {}

void RedBlueBlock::init(const al::ActorInitInfo& rInfo) {
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::initNerve(this, &NrvRedBlueBlockWait, 0);
    al::initActorAudioKeeper(this, rInfo, "RedBlueBlock", nullptr);
    const sead::Vector3f& trans = al::getTrans(this);
    mBasePosition.x = trans.x;
    mBasePosition.y = trans.y;
    mBasePosition.z = trans.z;
    al::tryGetArg(&mMoveAxis, rInfo, "MoveAxis");
    al::tryGetArg(&mMoveDistance, rInfo, "MoveDistance");
    al::tryGetArg(&mReverseAxis, rInfo, "ReverseAxis");
    makeActorAppeared();
}

void RedBlueBlock::exeWait() {
    al::isFirstStep(this);
    if (rc::isAnyPlayerJumpTrigOn(this))
        al::setNerve(this, &NrvRedBlueBlockFlip);
}

void RedBlueBlock::exeFlip() {
    if (al::isFirstStep(this)) {
        mMoveStep = mMovingOut ? 0 : 10;
        al::startSeByName(this, "FlipSt", nullptr);
    }
    if (al::isGreaterStep(this, 20)) {
        al::holdSeByName(this, "Move", nullptr);
        bool reachedEnd;
        if (mMovingOut) {
            reachedEnd = mMoveStep >= 9;
            mMoveStep = reachedEnd ? 10 : mMoveStep + 1;
        } else {
            reachedEnd = mMoveStep <= 1;
            mMoveStep = reachedEnd ? 0 : mMoveStep - 1;
        }
        float rate = mMoveStep / 10.0f;
        if (mReverseAxis)
            rate = -rate;
        al::setTransOffsetLocalDir(this, al::getQuat(this), mBasePosition,
                                   mMoveDistance * rate, mMoveAxis);
        if (rc::isAnyPlayerJumpTrigOn(this)) {
            mMovingOut = !mMovingOut;
            al::setNerveAtStep(this, &NrvRedBlueBlockFlip, 20);
        } else if (reachedEnd) {
            mMovingOut = !mMovingOut;
            al::setNerve(this, &NrvRedBlueBlockWait);
            al::startSeByName(this, "MoveEnd", nullptr);
        }
    }
}
