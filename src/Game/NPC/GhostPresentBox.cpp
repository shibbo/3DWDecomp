#include "NPC/GhostPresentBox.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Movement/PoseHistoryPath.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(GhostPresentBox, Stop)
NERVE_DECL(GhostPresentBox, Wait)
NERVE_DECL(GhostPresentBox, PopUp)
NERVE_DECL(GhostPresentBox, Start)
NERVE_DECL(GhostPresentBox, Open)
NERVE_DECL(GhostPresentBox, End)
NERVES_MAKE_STRUCT(GhostPresentBox, Stop, Wait, PopUp, Start, Open, End)
}
GhostPresentBox::GhostPresentBox(const char* name, int index) : al::LiveActor(name), mPlayerIndex(index) {}
const char* GhostPresentBox::getItemName() const {
    if (al::isExistModelResourceYaml(this, "ItemPattern", nullptr)) {
        al::ByamlIter root(al::getModelResourceYaml(this, "ItemPattern", nullptr));
        int world = GameDataFunction::calcPlayingWorldId(GameDataHolderAccessor(this));
        int lives = GameDataFunction::getPlayerLife(GameDataHolderAccessor(this));
        al::ByamlIter lifeItems;
        root.tryGetIterByKey(&lifeItems, lives > 9 ? "残機数10超え" : "残機数10以下");
        if (lifeItems.isValid()) {
            al::ByamlIter items;
            if (world <= 2) lifeItems.tryGetIterByKey(&items, "World1～2");
            else if (world == 3) lifeItems.tryGetIterByKey(&items, "World3");
            else lifeItems.tryGetIterByKey(&items, "World4以降");
            if (items.isValid() && items.isTypeArray()) {
                int count = items.getSize();
                int index = mPlayerIndex % count;
                if (al::isTypeStringByIndex(items, index)) {
                    const char* result = "";
                    items.tryGetStringByIndex(&result, index);
                    return result;
                }
            }
        }
    }
    return "コインx1";
}
void GhostPresentBox::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "GhostPresentBox", nullptr);
    al::offCollide(this);
    al::initNerve(this, &NrvGhostPresentBox.Stop, 0);
    makeActorDead();
}
bool GhostPresentBox::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (isInRouteDokan() || mHidden) return false;
    if (al::isMsgPlayerCatch(msg) || al::isMsgPlayerObjHipDropAll(msg)
        || al::isMsgPlayerSlidingAttack(msg) || al::isMsgPlayerObjRollingAttack(msg)
        || al::isMsgPlayerClimbAttack(msg) || al::isMsgPlayerSpinAttack(msg)
        || al::isMsgPlayerTailAttack(msg) || al::isMsgPlayerBodyAttack(msg)
        || al::isMsgPlayerBodyLanding(msg) || al::isMsgKickKouraItemGet(msg)
        || rc::isMsgSkateShoesAttack(msg) || al::isMsgPlayerGiantAttack(msg)
        || rc::isMsgGhostPresentGet(msg)) {
        if (al::isNerve(this, &NrvGhostPresentBox.Wait)) {
            al::setAppearItemFactor(this, "通常", sender);
            al::setNerve(this, &NrvGhostPresentBox.PopUp);
            return true;
        }
    }
    return false;
}
bool GhostPresentBox::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isMsgTouchAssistTrig(msg)) {
        if (al::isNerve(this, &NrvGhostPresentBox.Wait)) {
            al::setAppearItemFactor(this, "通常", nullptr);
            al::setNerve(this, &NrvGhostPresentBox.PopUp);
            return true;
        }
    }
    return false;
}
void GhostPresentBox::setPoseHistoryPath(const al::PoseHistoryPath* history) { mHistory = history; }
void GhostPresentBox::start() {
    if (al::isNerve(this, &NrvGhostPresentBox.Stop)) {
        updatePoseByHistroryPath();
        if (mHidden) al::setNerve(this, &NrvGhostPresentBox.Wait);
        else al::setNerve(this, &NrvGhostPresentBox.Start);
        appear();
    }
}
void GhostPresentBox::updatePoseByHistroryPath() {
    if (mHistory) {
        const char* action = "";
        mHistory->calcPoseAndTrans(al::getQuatPtr(this), al::getTransPtr(this), &action, 150.0f);
        if (al::isEqualString(action, "RouteDokanMove") || al::isEqualString(action, "RouteDokanLauncherMove")
            || al::isEqualString(action, "RouteDokanLaunchWait")) {
            mInRouteDokan = true;
            mHidden = false;
        } else if (al::isEqualString(action, "RaidonMove")) {
            mInRouteDokan = false;
            mHidden = true;
        } else {
            mInRouteDokan = false;
            mHidden = false;
        }
    }
    if (al::isInAreaObj(this, "GhostPresentBoxHideArea", al::getTrans(this))) mHidden = true;
    mOffset = al::converge(mOffset, mInRouteDokan ? -80.0f : 0.0f, 20.0f);
    al::addTransOffsetLocalDir(this, mOffset, 1);
    if (mHidden) al::hideModelIfShow(this);
    else al::showModelIfHide(this);
}
void GhostPresentBox::stop(bool effect) {
    if (al::isNerve(this, &NrvGhostPresentBox.Start) || al::isNerve(this, &NrvGhostPresentBox.Wait)) {
        if (effect) al::startHitReactionDisappear(this);
        al::setNerve(this, &NrvGhostPresentBox.Stop);
        kill();
    }
}
void GhostPresentBox::exeStart() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear");
    updatePoseByHistroryPath();
    if (al::isActionEnd(this)) al::setNerve(this, &NrvGhostPresentBox.Wait);
}
void GhostPresentBox::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
    updatePoseByHistroryPath();
}
void GhostPresentBox::exeStop() {}
void GhostPresentBox::exePopUp() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PopUp");
        al::setVelocityJump(this, 28.0f);
    }
    if (al::isStep(this, 3)) al::onCollide(this);
    al::addVelocityToGravity(this, 1.1f);
    al::scaleVelocity(this, 0.999f);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setVelocityZero(this);
        al::offCollide(this);
        al::setNerve(this, &NrvGhostPresentBox.Open);
    } else if (al::isInDeathArea(this)) al::setNerve(this, &NrvGhostPresentBox.End);
}
void GhostPresentBox::exePopUpInRouteDokan() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PopUp");
        al::setVelocityZero(this);
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvGhostPresentBox.Open);
}
void GhostPresentBox::exeOpen() {
    if (al::isFirstStep(this)) al::startAction(this, "Open");
    if (al::isActionEnd(this)) {
        al::appearItemTiming(this, getItemName());
        al::setNerve(this, &NrvGhostPresentBox.End);
    }
}
void GhostPresentBox::exeEnd() { kill(); }
bool GhostPresentBox::isInRouteDokan() const { return mInRouteDokan; }
namespace {
NERVE_IMPL(GhostPresentBox, Stop)
NERVE_IMPL(GhostPresentBox, Wait)
NERVE_IMPL(GhostPresentBox, PopUp)
NERVE_IMPL(GhostPresentBox, Start)
NERVE_IMPL(GhostPresentBox, Open)
NERVE_IMPL(GhostPresentBox, End)
}