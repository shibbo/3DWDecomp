#include "MapObj/BoxKillerBlockBrick.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
    NERVE_DECL(BoxKillerBlockBrick, Wait);
    NERVE_DECL(BoxKillerBlockBrick, Reaction);
    NERVES_MAKE_NOSTRUCT(BoxKillerBlockBrick, Wait, Reaction)
}

BoxKillerBlockBrick::BoxKillerBlockBrick(const char* pName, int type)
    : al::LiveActor(pName), mType(type) {}
BoxKillerBlockBrick::~BoxKillerBlockBrick() {}

void BoxKillerBlockBrick::init(const al::ActorInitInfo& rInfo) {
    const char* archive;
    if (mType == 0)
        archive = "BoxKillerBlockBrick";
    else if (mType == 1)
        archive = "BoxKillerBlockEmpty";
    else {
        makeActorDead();
        return;
    }
    al::initActorWithArchiveName(this, rInfo, archive, nullptr);
    al::initNerve(this, &NrvBoxKillerBlockBrickWait, 0);
    if (mType == 0) {
        mBreakModel = new al::BreakModel(this, "キラーボックス用ブロック壊れモデル",
            "BoxKillerBlockBrickBreak", nullptr, nullptr, "Break", true);
        al::initCreateActorNoPlacementInfo(mBreakModel, rInfo);
    }
    al::offCollide(this);
    makeActorAppeared();
}

void BoxKillerBlockBrick::kill() {
    al::appearBreakModelRandomRotateY(mBreakModel);
    al::LiveActor::kill();
}

bool BoxKillerBlockBrick::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
    al::HitSensor* pSelf) {
    if (mType != 0)
        return false;
    if (!rc::isMsgForBlockAll(pMsg, pOther, pSelf, 100.0f))
        return false;
    if (!isBreakable(pMsg, pOther)) {
        al::startAction(this, al::isMsgPlayerHipDropAll(pMsg) ? "ReactionHipDrop" : "Reaction");
        al::setNerve(this, &NrvBoxKillerBlockBrickReaction);
        return rc::getMsgReturnValueForBlock(pMsg);
    }
    if (al::isSensorPlayer(pOther))
        rc::addScore(this, pOther, 0.0f, 0);
    kill();
    return true;
}

bool BoxKillerBlockBrick::isBreakable(const al::SensorMsg* pMsg, al::HitSensor* pOther) const {
    if (al::isSensorPlayer(pOther) && rc::isPlayerMini(pOther))
        return false;
    return true;
}

void BoxKillerBlockBrick::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
}

void BoxKillerBlockBrick::exeReaction() {
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvBoxKillerBlockBrickWait);
}
