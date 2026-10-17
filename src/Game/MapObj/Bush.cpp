#include "MapObj/Bush.hpp"
#include "MapObj/BgmRhythmAnimeController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Project/Audio/AudioSystem.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(Bush, Wait)
NERVE_DECL(Bush, RabbitWait)
NERVE_DECL(Bush, ReactionMic)
NERVE_DECL(Bush, ReactionFire)
NERVE_DECL(Bush, ReactionDeath)
NERVE_DECL(Bush, RabbitAppear)
class BushNrvReactionNoItem : public al::Nerve {
public:
    void execute(al::NerveKeeper* keeper) const override { keeper->getParent<Bush>()->exeReaction(); }
};
NERVE_DECL(Bush, Reaction)
NERVE_DECL(Bush, ReactionAttack)
NERVE_DECL(Bush, ReactionTouch)
const struct {
    BushNrvWait Wait;
    BushNrvRabbitWait RabbitWait;
    BushNrvReactionMic ReactionMic;
    BushNrvReactionFire ReactionFire;
    BushNrvReactionDeath ReactionDeath;
    BushNrvRabbitAppear RabbitAppear;
    BushNrvReactionNoItem ReactionNoItem;
    BushNrvReaction Reaction;
    BushNrvReactionAttack ReactionAttack;
    BushNrvReactionTouch ReactionTouch;
} NrvBush{};
const sead::Vector3f sItemAppearOffset(0.0f, 80.0f, 0.0f);
}
Bush::Bush(const char* name) : al::LiveActor(name) {}
Bush::~Bush() = default;
void Bush::init(const al::ActorInitInfo& info) {
    mIsBush = al::isObjectName(info, "Bush");
    const char* model = mIsBush ? "Bush" : "Grass";
    alPlacementFunction::tryGetModelName(&model, info);
    int color = 0;
    al::tryGetArg(&color, info, "Color");
    initWithColorFrame(info, color, model);
}
void Bush::initWithColorFrame(const al::ActorInitInfo& info, int color, const char* model) {
    if (!model) model = mIsBush ? "Bush" : "Grass";
    const char* suffix = nullptr;
    int singleColor = 0;
    if (color == 1) { suffix = "Red"; singleColor = 2; }
    else if (color == 2) { suffix = "Blue"; singleColor = 1; }
    if (al::isSingleMode(info)) color = singleColor;
    al::initActorWithArchiveName(this, info, model, suffix);
    al::initNerve(this, &NrvBush.Wait, 0);
    al::startAction(this, "Wait");
    al::startMtpAnimAndSetFrameAndStop(this, model, float(color));
    al::startMtpAnimAndSetFrameAndStop(al::getSubActor(this, 0), al::getModelName(al::getSubActor(this, 0)), float(color));
    mConnector = al::tryCreateMtxConnector(this, info);
    const char* item = "None";
    if (al::tryGetStringArg(&item, info, "ItemType") && item && !al::isEqualString(item, "None")) {
        mItemType = item;
        al::setAppearItemOffset(this, sItemAppearOffset);
    }
    if (al::isSingleMode(info)) mIsSingleMode = true;
    else {
        float angle = al::getRandom(-25.0f, 25.0f);
        al::rotateQuatYDirDegree(this, angle);
        al::rotateQuatYDirDegree(al::getSubActor(this, 0), angle);
    }
    mRhythm = new BgmRhythmAnimeController(this, false);
    if (al::isExistShadow(this)) al::setShadowFixed(this, true);
    makeActorAppeared();
}
void Bush::initBushOnRabbit(const al::ActorInitInfo& info, const sead::Vector3f& trans, s32 color) {
    mIsBush = true;
    mRabbitAppeared = false;
    al::PlacementInfo placement;
    al::ActorInitInfo initInfo;
    initInfo.initViewIdHost(&placement, info);
    initWithColorFrame(initInfo, color, nullptr);
    al::setNerve(this, &NrvBush.RabbitWait);
    al::resetPosition(this, trans, false);
}
void Bush::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, 50.0f, 300.0f);
}
void Bush::control() {
    if (mReactionCooldown - 1 >= 0) --mReactionCooldown;
    if (mConnector) al::connectPoseQT(this, mConnector);
    if ((al::isNerve(this, &NrvBush.Wait) || al::isNerve(this, &NrvBush.RabbitWait)) && al::isMicInputOn(this)) al::setNerve(this, &NrvBush.ReactionMic);
}
bool Bush::trySetAppearItemFactorByMsg(const al::SensorMsg* msg, const al::HitSensor* other, const char* factor) {
    if (!mItemType) return false;
    if (factor) al::setAppearItemFactor(this, factor, other);
    else rc::setAppearItemFactorByMsg(this, msg, other);
    return true;
}
bool Bush::receiveMsg(const al::SensorMsg* msg, al::HitSensor* other, al::HitSensor*) {
    if (al::isNerve(this, &NrvBush.ReactionFire) || al::isNerve(this, &NrvBush.ReactionDeath)) return false;
    if (al::isMsgLaserAttack(msg) || al::isMsgPlayerFireBallAttack(msg) || al::isMsgEnemyAttackFire(msg)) {
        trySetAppearItemFactorByMsg(msg, other, nullptr);
        al::setNerve(this, &NrvBush.ReactionFire);
        return mIsBush;
    }
    if (!al::isNerve(this, &NrvBush.Wait) && !al::isNerve(this, &NrvBush.RabbitWait)) return false;
    if (mIsSingleMode && rc::isMsgBobsledBodyAttack(msg)) {
        trySetAppearItemFactorByMsg(msg, other, nullptr);
        al::setNerve(this, &NrvBush.ReactionDeath);
        return mIsBush;
    }
    if (((al::isSensorPlayer(other) || al::isSensorKoopaJr(other)) && al::isMsgPlayerItemGet(msg)) || al::isMsgNpcTouch(msg) || al::isSensorEnemyAttack(other)) {
        if (al::isMsgPlayerItemGet(msg)) {
            const sead::Vector3f& velocity = al::getActorVelocity(other);
            if (al::isNearZero(sead::Mathf::sqrt(velocity.x * velocity.x + velocity.z * velocity.z), 0.001f) && mReactionCooldown > 0) {
                mReactionCooldown = 15;
                return false;
            }
        }
        if (al::isMsgNpcTouch(msg) || al::isSensorEnemyBody(other)) {
            if (al::isNerve(this, &NrvBush.RabbitWait) || al::isNerve(this, &NrvBush.RabbitAppear)) return false;
            if (al::isNerve(this, &NrvBush.Wait)) al::setNerve(this, &NrvBush.ReactionNoItem);
        } else if (al::isNerve(this, &NrvBush.Wait)) {
            trySetAppearItemFactorByMsg(msg, other, "直接攻撃");
            al::setNerve(this, &NrvBush.Reaction);
        } else {
            const sead::Vector3f& velocity = al::getActorVelocity(other);
            if (al::isNearZero(sead::Mathf::sqrt(velocity.x * velocity.x + velocity.z * velocity.z), 0.001f)) mReactionCooldown = 15;
        }
        return false;
    }
    if ((EnemyStateUtil::isMsgBlowDown(msg) && !al::isMsgPlayerGiantAttack(msg)) || al::isMsgBallTrample(msg)) {
        trySetAppearItemFactorByMsg(msg, other, nullptr);
        al::setNerve(this, &NrvBush.ReactionAttack);
    }
    return false;
}
bool Bush::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvBush.ReactionFire)) return false;
    if (al::isMsgTouchAssistAll(msg)) {
        if (al::isNerve(this, &NrvBush.ReactionTouch)) return mIsBush;
        al::setAppearItemFactor(this, "間接攻撃", nullptr);
        rc::setAppearItemAttackerSensorByScreenPointer(this, pointer);
        al::setNerve(this, &NrvBush.ReactionTouch);
        return true;
    }
    return false;
}
void Bush::updateLinkedTrans(const sead::Vector3f& trans) {
    al::setTrans(this, trans);
    al::setTrans(al::getSubActor(this, 0), trans);
}
void Bush::exeWait() {
    if (al::isFirstStep(this)) al::tryStartActionIfNotPlaying(this, "Wait");
    if (mRhythm) mRhythm->update();
}
bool Bush::tryAppearItem() {
    if (!mItemType) return false;
    if (mIsSingleMode) {
        sead::Vector3f front = sead::Vector3f::ez;
        al::calcFrontDir(&front, this);
        al::appearItemTiming(this, mItemType, al::getTrans(this), front);
    } else al::appearItemTiming(this, mItemType);
    mItemType = nullptr;
    return true;
}
void Bush::exeReaction() {
    if (al::isFirstStep(this)) {
        if (!al::tryStartAction(this, "Reaction")) al::startAction(this, "ReactionTouch");
        if (al::isNerve(this, &NrvBush.Reaction)) tryAppearItem();
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, mRabbitAppeared ? static_cast<const al::Nerve*>(&NrvBush.Wait) : &NrvBush.RabbitWait);
        mReactionCooldown = 15;
    }
}
void Bush::exeReactionAttack() {
    if (al::isFirstStep(this)) {
        if (!al::tryStartAction(this, "ReactionAttack")) al::startAction(this, "ReactionTouch");
        tryAppearItem();
    }
    if (al::isActionEnd(this)) al::setNerve(this, mRabbitAppeared ? static_cast<const al::Nerve*>(&NrvBush.Wait) : &NrvBush.RabbitWait);
}
void Bush::exeReactionFire() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ReactionFireBall");
        tryAppearItem();
        al::getSubActor(this, 0)->appear();
    }
    if (al::isActionEnd(this)) kill();
}
void Bush::exeReactionDeath() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ReactionPlessie");
        tryAppearItem();
        al::getSubActor(this, 0)->appear();
    }
    if (al::isActionEnd(this)) { al::tryEmitEffect(this, "Disappear", nullptr); kill(); }
}
void Bush::exeReactionMic() {
    if (al::isFirstStep(this)) { mMicReleaseFrames = 0; al::startAction(this, "ReactionMic"); }
    if (al::isMicInputOn(this)) mMicReleaseFrames = 0;
    else if (mMicReleaseFrames++ >= 5) al::setNerve(this, mRabbitAppeared ? static_cast<const al::Nerve*>(&NrvBush.Wait) : &NrvBush.RabbitWait);
}
void Bush::exeReactionTouch() {
    if (al::isFirstStep(this)) { al::startAction(this, "ReactionTouch"); tryAppearItem(); }
    if (al::isActionEnd(this)) al::setNerve(this, mRabbitAppeared ? static_cast<const al::Nerve*>(&NrvBush.Wait) : &NrvBush.RabbitWait);
}
void Bush::exeRabbitWait() { if (al::isFirstStep(this)) al::startAction(this, "RabbitWait"); }
void Bush::exeRabbitAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "RabbitAppear");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvBush.Wait);
}
bool Bush::tryStartRabbitAppear() {
    if (!al::isAlive(this)) return false;
    if (!al::isNerve(this, &NrvBush.RabbitWait)) return false;
    al::setNerve(this, &NrvBush.RabbitAppear);
    mRabbitAppeared = true;
    return true;
}
