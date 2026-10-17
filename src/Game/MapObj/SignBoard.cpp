#include "MapObj/SignBoard.hpp"
#include "MapObj/SnowCover.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/ScoreUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(SignBoard, Wait);
NERVE_DECL(SignBoard, Break);
NERVES_MAKE_NOSTRUCT(SignBoard, Wait, Break)
}
SignBoard::SignBoard(const char* name) : al::LiveActor(name) {}
SignBoard::~SignBoard() {}
void SignBoard::init(const al::ActorInitInfo& info) {
    const char* objectName = nullptr;
    al::getObjectName(&objectName, info);
    int phaseColor = 0;
    al::tryGetArg(&phaseColor, info, "SignboardPhaseColor");
    const char* suffix = nullptr;
    if (info.getActorSceneInfo().isSingleMode && objectName && al::isEqualString(objectName, "SignBoardCat")) {
        if (phaseColor == 1) suffix = "Blue";
        if (phaseColor == 2) suffix = "Red";
    }
    al::initActorWithArchiveName(this, info, objectName, suffix);
    if (al::isEqualString(objectName, "SignBoardWood") || al::isEqualString(objectName, "SignBoardCat")) mIsBreakable = true;
    mConnector = al::tryCreateMtxConnector(this, info);
    al::initNerve(this, &NrvSignBoardWait, 0);
    mSnowCover = SnowCoverFunction::tryCreateSnowCover(this, info, "SignBoardSnowCover", !mIsBreakable, nullptr);
    makeActorAppeared();
    const char* modelName = nullptr;
    alPlacementFunction::getModelName(&modelName, info);
    float frame;
    if (al::isEqualSubString(modelName, "LeftDown")) frame = 7.0f;
    else if (al::isEqualSubString(modelName, "LeftUp")) frame = 6.0f;
    else if (al::isEqualSubString(modelName, "RightDown")) frame = 5.0f;
    else if (al::isEqualSubString(modelName, "RightUp")) frame = 4.0f;
    else if (al::isEqualSubString(modelName, "Right")) frame = 3.0f;
    else if (al::isEqualSubString(modelName, "Left")) frame = 2.0f;
    else if (al::isEqualSubString(modelName, "Down")) frame = 1.0f;
    else if (al::isEqualSubString(modelName, "Up")) frame = 0.0f;
    else frame = -1.0f;
    if (al::isVisAnimExist(this, "SignAim")) al::startVisAnimAndSetFrameAndStop(this, "SignAim", frame);
    if (al::getScale(this).x > 1.0f) mIsLarge = true;
    if (!mIsLarge) {
        mBreakModel = al::tryGetSubActor(this, "木製看板壊れモデル");
        if (mBreakModel && al::isMtpAnimExist(mBreakModel) && al::isMtpAnimExist(mBreakModel, "SignBoardCatBreak"))
            al::startMtpAnimAndSetFrameAndStop(mBreakModel, "SignBoardCatBreak", phaseColor);
        mTraceModel = al::tryGetSubActor(this, "木製看板残留モデル");
        if (mTraceModel) {
            mTraceModel->setGlobalAlphaPtr(&mTraceModel->mGlobalAlphaLastFrame);
            if (al::isMtpAnimExist(mTraceModel) && al::isMtpAnimExist(mTraceModel, "SignBoardCatTrace"))
                al::startMtpAnimAndSetFrameAndStop(mTraceModel, "SignBoardCatTrace", phaseColor);
        }
    }
    if (al::isSingleMode(info)) {
        if (al::isMtpAnimExist(this, "SignLeaves")) al::startMtpAnimAndSetFrameAndStop(this, "SignLeaves", phaseColor);
        if (al::calcLinkChildNum(info, "TouchReactionPoint") > 0) {
            ProjectActorFactory factory;
            al::ActorInitInfo childInfo;
            al::PlacementInfo placement;
            al::getLinksInfoByIndex(&placement, info, "TouchReactionPoint", 0);
            childInfo.initNoViewId(&placement, info);
            if (SingleModeDataFunction::isValidPlessieChasePlacement(GameDataHolderAccessor(this), childInfo))
                mTouchReaction = al::createLinksActorFromFactory(factory, info, "TouchReactionPoint", 0);
        }
    }
}
void SignBoard::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, false);
}
void SignBoard::respawn() {
    if (al::isAlive(this)) return;
    if (mSnowCover) mSnowCover->respawn();
    if (mTraceModel) mTraceModel->kill();
    if (mBreakModel) mBreakModel->kill();
    appear();
    al::setNerve(this, &NrvSignBoardWait);
}
void SignBoard::control() {
    if (mConnector) al::connectPoseQT(this, mConnector);
}
bool SignBoard::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (!mIsBreakable) return false;
    if (al::isMsgKickKouraCollideNoReflect(msg)) return true;
    if (al::isNerve(this, &NrvSignBoardBreak)) return false;
    if (al::isMsgKickKouraAttackCollide(msg)) {
        rc::addScore(this, sender, 0.0f, 0);
        rc::requestHitReactionToAttacker("キック甲羅ヒット", receiver, sender);
        al::setNerve(this, &NrvSignBoardBreak);
        return true;
    }
    if (al::isMsgPlayerRollingAttack(msg) || al::isMsgPlayerObjHipDropAll(msg) ||
        al::isMsgPlayerInvincibleAttack(msg) || al::isMsgPlayerFireBallAttack(msg) ||
        al::isMsgPlayerBoomerangAttack(msg) ||
        (al::isMsgPlayerClimbAttack(msg) && !((al::getActorTrans(sender) - al::getActorTrans(receiver)).y > 200.0f)) ||
        al::isMsgPlayerClimbSlidingAttack(msg) ||
        (al::isMsgPlayerTailAttack(msg) && !((al::getActorTrans(sender) - al::getActorTrans(receiver)).y > 200.0f)) ||
        (al::isMsgPlayerSpinAttack(msg) && !((al::getActorTrans(sender) - al::getActorTrans(receiver)).y > 200.0f)) ||
        al::isMsgPlayerGiantAttack(msg) || al::isMsgLaserAttack(msg) || al::isMsgPlayerSlidingAttack(msg) ||
        al::isMsgBallAttack(msg) || al::isMsgBallTrample(msg) || al::isMsgKickKouraAttack(msg) ||
        al::isMsgPlayerKouraAttack(msg) || al::isMsgExplosion(msg) || al::isMsgExplosionCollide(msg) ||
        al::isMsgPlayerBodyAttack(msg) || rc::isMsgRaidonAttack(msg) || rc::isMsgSkateShoesAttack(msg) ||
        al::isMsgEnemyAttackFire(msg) || rc::isMsgBullAttack(msg) ||
        (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && al::isMsgNekoAttack(msg))) {
        rc::addScore(this, sender, 0.0f, 0);
        rc::requestHitReactionToAttacker(msg, receiver, sender);
        al::setNerve(this, &NrvSignBoardBreak);
        return true;
    }
    return false;
}
bool SignBoard::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (al::isMsgTouchAssistTrig(msg)) {
        rc::addScore(this, pointer, 0.0f, 0);
        al::setNerve(this, &NrvSignBoardBreak);
        return true;
    }
    return false;
}
void SignBoard::exeWait() {}
void SignBoard::exeBreak() {
    if (mIsLarge) al::startHitReaction(this, "拡大破壊");
    else al::startHitReactionBreak(this);
    if (mSnowCover) mSnowCover->tryBreak();
    if (mBreakModel) mBreakModel->appear();
    if (mTraceModel) {
        al::tryStartAction(mTraceModel, "Appear");
        mTraceModel->appear();
    }
    if (mTouchReaction) mTouchReaction->kill();
    al::LiveActor::kill();
}
