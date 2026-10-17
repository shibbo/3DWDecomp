#include "MapObj/BlockHard.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Obj/PartsModel.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/LiveActor/LiveActorFunc.hpp"
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include <cstdlib>
#include <math/seadBoundBox.h>
namespace {
    NERVE_DECL(BlockHard, Wait);
    NERVE_DECL(BlockHard, Reaction);
    NERVE_DECL(BlockHard, BreakStart);
    NERVES_MAKE_NOSTRUCT(BlockHard, Wait)
    NERVES_MAKE_STRUCT(BlockHard, BreakStart, Reaction)
}
BlockHard::BlockHard(const char* name) : al::LiveActor(name) {}
BlockHard::~BlockHard() {
    if (mConnector) {
        mConnector->al::MtxConnector::~MtxConnector();
        ::operator delete(mConnector);
        mConnector = nullptr;
    }
}
void BlockHard::init(const al::ActorInitInfo& info) {
    const char* suffix = rc::getBlockSuffixName(info, al::isSingleMode(info));
    al::initActorWithArchiveName(this, info, "BlockHard", suffix);
    al::initNerve(this, &NrvBlockHardWait, 1);
    bool useDepth = true;
    al::tryGetArg(&useDepth, info, "IsUseZPrePass");
    if (useDepth) {
        al::initSubActorKeeperNoFile(this, info, 1);
        auto* depth = new al::PartsModel("硬ブロックZプリパスモデル");
        depth->initPartsMtx(this, info, "BlockHardDepth", getBaseMtx(), false);
        depth->setGlobalAlphaPtr(&mGlobalAlphaLastFrame);
        al::registerSubActorSyncClipping(this, depth, al::isSingleMode(info));
    }
    mBreakModel = new al::BreakModel(this, "硬ブロック壊れモデル", "BlockHardBreak", nullptr, nullptr, "Break", true);
    al::initCreateActorNoPlacementInfo(mBreakModel, info);
    mConnector = al::tryCreateMtxConnector(this, info);
    al::tryGetArg(&mSaveIfBroken, info, "SaveIfBroken");
    if (al::isSingleMode(info)) {
        bool destroyed = false;
        if (mSaveIfBroken) {
            int zone = mPlacementHolder->getZoneNo();
            al::StringTmp<32> id("%s", mPlacementHolder->getId());
            mSaveId = std::atoi(id.getPart(3).cstr()) | (zone << 16);
            if (SingleModeDataFunction::isBlockHardDestroyed(GameDataHolderAccessor(this), mSaveId)) {
                makeActorDead();
                al::appearBreakModelRandomRotateY(mBreakModel);
                destroyed = true;
            }
        }
        if (al::listenStageSwitchOnOffAppear(this, al::FunctorV0M(this, &BlockHard::appear), al::FunctorV0M(this, &BlockHard::kill))) {
            if (!destroyed) makeActorDead();
        } else appear();
    } else makeActorAppeared();
}
void BlockHard::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, false);
    al::updateMaterialCodeWater(this);
}
void BlockHard::appear() {
    if (mSaveIfBroken && SingleModeDataFunction::isBlockHardDestroyed(GameDataHolderAccessor(this), mSaveId)) return;
    al::LiveActor::appear();
}
void BlockHard::kill() { al::LiveActor::kill(); }
void BlockHard::updateLinkedTrans(const sead::Vector3f& trans) {
    al::setNeedSetBaseMtxAndCalcAnimFlag(this, true);
    alLiveActorFunction::forceUpdateTrans(this, trans, true);
    if (al::isNerve(this, &NrvBlockHardWait) && al::isGreaterEqualStep(this, 3)) al::setNeedSetBaseMtxAndCalcAnimFlag(this, false);
}
void BlockHard::control() {
    if (mConnector) al::connectPoseQT(this, mConnector);
    al::LiveActor::control();
}
bool BlockHard::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    auto breakBlock = [this, sender]() {
        kill();
        rc::addScoreByFactor(this, sender, "壊れ", 0.0f, 0);
        al::appearBreakModelRandomRotateY(mBreakModel);
        al::startSe(mBreakModel, "Break", nullptr);
        if (mSaveIfBroken) SingleModeDataFunction::destroyBlockHard(GameDataHolderWriter(this), mSaveId);
    };
    if (rc::isMsgRaidonBreakLightReaction(msg)) return true;
    if (al::isNerve(this, &NrvBlockHard.BreakStart)) return false;
    if (al::isSensorPlayer(sender) && al::isSensorName(sender, "Eye")) return false;
    if (!al::isSensorName(receiver, "Body")) return false;
    if (al::isMsgLaserAttack(msg)) {
        breakBlock();
        return true;
    }
    auto testGiantHit = [this, sender](float y, const sead::BoundBox3f& bounds) {
        sead::Vector3f center = al::getTrans(this) + sead::Vector3f(0.0f, y, 0.0f);
        return al::isHitBoxSensor(sender, center, bounds);
    };
    sead::BoundBox3f box(sead::Vector3f(-105.0f, -180.0f, -105.0f), sead::Vector3f(105.0f, 180.0f, 105.0f));
    if (al::isHitBoxSensor(sender, al::getTrans(this) + sead::Vector3f(0.0f, 120.0f, 0.0f), box)) {
        if (al::isMsgPlayerTailAttack(msg) || al::isMsgPlayerBoomerangAttackCollide(msg) ||
            al::isMsgPlayerHipDropAll(msg) || al::isMsgPlayerRollingAttack(msg) ||
            al::isMsgPlayerUpperPunch(msg) || al::isMsgPlayerSpinAttack(msg) ||
            al::isMsgPlayerGiantHipDrop(msg)) {
            if (al::isMsgPlayerTailAttack(msg) || al::isMsgPlayerSpinAttack(msg)) {
                if (al::getSensorPos(sender).y - al::getTrans(this).y > 220.0f) return false;
                if (al::getSensorPos(sender).y - al::getTrans(this).y < 0.0f) return false;
            }
            al::setNerve(this, &NrvBlockHard.Reaction);
            return false;
        }
        if (al::isMsgPlayerGiantTouch(msg)) {
            mAttacker = sender;
            al::setNerve(this, &NrvBlockHard.BreakStart);
            return false;
        }
        if (al::isMsgExplosion(msg) || rc::isMsgBullAttack(msg) || rc::isMsgTuccondorAttack(msg) || al::isMsgKickKouraAttackCollide(msg)) {
            breakBlock();
            return true;
        }
    }
    if (al::isMsgPlayerGiantAttack(msg)) {
        sead::BoundBox3f giantBox(sead::Vector3f(-110.0f, -75.0f, -110.0f), sead::Vector3f(110.0f, 75.0f, 110.0f));
        if (testGiantHit(-50.0f, giantBox)) {
            breakBlock();
            return false;
        }
    }
    if (!al::isSensorPlayer(sender) || !rc::isPlayerGiant(sender) || al::isMsgPlayerGiantHipDrop(msg) || !rc::isPlayerHipDropping(sender)) return false;
    if (rc::getPlayerVelocity(sender).y > -5.0f) return false;
    sead::Vector3f direction(al::getSensorPos(receiver));
    direction -= al::getSensorPos(sender);
    if (al::calcAngleDegree(rc::getPlayerVelocity(sender), direction) > 35.0f) return false;
    sead::BoundBox3f giantBox(sead::Vector3f(-110.0f, -75.0f, -110.0f), sead::Vector3f(110.0f, 75.0f, 110.0f));
    if (!testGiantHit(180.0f, giantBox)) return false;
    breakBlock();
    return false;
}
void BlockHard::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
    if (!mConnector && al::isStep(this, 3)) al::setNeedSetBaseMtxAndCalcAnimFlag(this, false);
}
void BlockHard::exeReaction() {
    if (al::isFirstStep(this)) {
        al::setNeedSetBaseMtxAndCalcAnimFlag(this, true);
        al::startAction(this, "Reaction");
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvBlockHardWait);
}
void BlockHard::exeBreakStart() {
    if (al::isFirstStep(this)) {
        al::setNeedSetBaseMtxAndCalcAnimFlag(this, true);
        al::startAction(this, "BreakStart");
    }
    if (al::isActionEnd(this)) {
        kill();
        rc::addScoreByFactor(this, mAttacker, "壊れ", 0.0f, 0);
        al::appearBreakModelRandomRotateY(mBreakModel);
        al::startSe(mBreakModel, "Break", nullptr);
        if (mSaveIfBroken) SingleModeDataFunction::destroyBlockHard(GameDataHolderWriter(this), mSaveId);
    }
}
