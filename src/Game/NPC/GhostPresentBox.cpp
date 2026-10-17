#include "NPC/GhostPresentBox.hpp"
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/Movement/PoseHistoryPath.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(GhostPresentBox, Stop)
NERVE_DECL(GhostPresentBox, Wait)
NERVE_DECL(GhostPresentBox, PopUp)
NERVE_DECL(GhostPresentBox, Start)
NERVE_DECL(GhostPresentBox, Open)
NERVE_DECL(GhostPresentBox, End)
const struct {
    GhostPresentBoxNrvStop Stop;
    GhostPresentBoxNrvWait Wait;
    GhostPresentBoxNrvPopUp PopUp;
    GhostPresentBoxNrvStart Start;
    GhostPresentBoxNrvOpen Open;
    GhostPresentBoxNrvEnd End;
} NrvGhostPresentBox;
} // namespace
/** @brief Creates a ghost gift box. @param pName Actor name. @param itemIndex Item-pattern index.
 */
GhostPresentBox::GhostPresentBox(const char* pName, int itemIndex)
    : al::LiveActor(pName), mHistory(nullptr), mItemIndex(itemIndex), mOffset(0),
      mInRouteDokan(false), mHidden(false) {}
/** @brief Releases the gift box. */
GhostPresentBox::~GhostPresentBox() = default;
/** @brief Selects a gift from world and remaining-life settings. @return Configured item timing, or
 * one coin. */
const char* GhostPresentBox::getItemName() const {
    if (al::isExistModelResourceYaml(this, "ItemPattern", nullptr)) {
        al::ByamlIter root(al::getModelResourceYaml(this, "ItemPattern", nullptr));
        int world = GameDataFunction::calcPlayingWorldId(GameDataHolderAccessor(this));
        int lives = GameDataFunction::getPlayerLife(GameDataHolderAccessor(this));
        al::ByamlIter life;
        root.tryGetIterByKey(&life, lives > 9 ? "\u6b8b\u6a5f\u657010\u8d85\u3048"
                                              : "\u6b8b\u6a5f\u657010\u4ee5\u4e0b");
        if (life.isValid()) {
            al::ByamlIter items;
            if (world <= 2) {
                life.tryGetIterByKey(&items, "World1\uff5e2");
            } else if (world == 3) {
                life.tryGetIterByKey(&items, "World3");
            } else {
                life.tryGetIterByKey(&items, "World4\u4ee5\u964d");
            }
            if (items.isValid() && items.isTypeArray()) {
                int size = items.getSize();
                int index = mItemIndex % size;
                if (al::isTypeStringByIndex(items, index)) {
                    const char* pItem = "";
                    items.tryGetStringByIndex(&pItem, index);
                    return pItem;
                }
            }
        }
    }
    return "\u30b3\u30a4\u30f3x1";
}
/** @brief Initializes the inactive gift box. @param rInfo Placement initialization. */
void GhostPresentBox::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "GhostPresentBox", nullptr);
    al::offCollide(this);
    al::initNerve(this, &NrvGhostPresentBox.Stop, 0);
    makeActorDead();
}
/** @brief Handles attacks and item collection. @param pMessage Incoming message. @param pOther
 * Sender sensor. @param pSelf Receiver sensor. @return Whether collection started. */
bool GhostPresentBox::receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                                 al::HitSensor* pSelf) {
    if (isInRouteDokan()) {
        return false;
    }
    if (mHidden) {
        return false;
    }
    if (al::isMsgPlayerCatch(pMessage) || al::isMsgPlayerObjHipDropAll(pMessage) ||
        al::isMsgPlayerSlidingAttack(pMessage) || al::isMsgPlayerObjRollingAttack(pMessage) ||
        al::isMsgPlayerClimbAttack(pMessage) || al::isMsgPlayerSpinAttack(pMessage) ||
        al::isMsgPlayerTailAttack(pMessage) || al::isMsgPlayerBodyAttack(pMessage) ||
        al::isMsgPlayerBodyLanding(pMessage) || al::isMsgKickKouraItemGet(pMessage) ||
        rc::isMsgSkateShoesAttack(pMessage) || al::isMsgPlayerGiantAttack(pMessage) ||
        rc::isMsgGhostPresentGet(pMessage)) {
        if (al::isNerve(this, &NrvGhostPresentBox.Wait)) {
            al::setAppearItemFactor(this, "\u901a\u5e38", pOther);
            al::setNerve(this, &NrvGhostPresentBox.PopUp);
            return true;
        }
        return false;
    }
    return false;
}
/** @brief Handles touch collection. @param pMessage Incoming message. @param pPointer Input
 * pointer. @param pTarget Point target. @return Whether collection started. */
bool GhostPresentBox::receiveMsgScreenPoint(const al::SensorMsg* pMessage,
                                            al::ScreenPointer* pPointer,
                                            al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssistTrig(pMessage)) {
        if (al::isNerve(this, &NrvGhostPresentBox.Wait)) {
            al::setAppearItemFactor(this, "\u901a\u5e38", nullptr);
            al::setNerve(this, &NrvGhostPresentBox.PopUp);
            return true;
        }
        return false;
    }
    return false;
}
/** @brief Sets the ghost path to follow. @param pPath Recorded pose history. */
void GhostPresentBox::setPoseHistoryPath(const al::PoseHistoryPath* pPath) { mHistory = pPath; }
/** @brief Starts following the recorded ghost path. */
void GhostPresentBox::start() {
    if (!al::isNerve(this, &NrvGhostPresentBox.Stop)) {
        return;
    }
    updatePoseByHistroryPath();
    if (mHidden) {
        al::setNerve(this, &NrvGhostPresentBox.Wait);
    } else {
        al::setNerve(this, &NrvGhostPresentBox.Start);
    }
    appear();
}
/** @brief Follows recorded poses and updates pipe offsets and visibility. */
void GhostPresentBox::updatePoseByHistroryPath() {
    if (mHistory) {
        const char* pAction = "";
        mHistory->calcPoseAndTrans(al::getQuatPtr(this), al::getTransPtr(this), &pAction, 150.0f);
        if (al::isEqualString(pAction, "RouteDokanMove") ||
            al::isEqualString(pAction, "RouteDokanLauncherMove") ||
            al::isEqualString(pAction, "RouteDokanLaunchWait")) {
            mInRouteDokan = true;
            mHidden = false;
        } else if (al::isEqualString(pAction, "RaidonMove")) {
            mInRouteDokan = false;
            mHidden = true;
        } else {
            mInRouteDokan = false;
            mHidden = false;
        }
    }
    if (al::isInAreaObj(this, "GhostPresentBoxHideArea", al::getTrans(this))) {
        mHidden = true;
    }
    mOffset = al::converge(mOffset, mInRouteDokan ? -80.0f : 0.0f, 20.0f);
    al::addTransOffsetLocalDir(this, mOffset, 1);
    if (mHidden) {
        al::hideModelIfShow(this);
    } else {
        al::showModelIfHide(this);
    }
}
/** @brief Stops the following box. @param showEffect Show its disappearance reaction. */
void GhostPresentBox::stop(bool showEffect) {
    if (al::isNerve(this, &NrvGhostPresentBox.Start) ||
        al::isNerve(this, &NrvGhostPresentBox.Wait)) {
        if (showEffect) {
            al::startHitReactionDisappear(this);
        }
        al::setNerve(this, &NrvGhostPresentBox.Stop);
        kill();
    }
}
/** @brief Plays the appearance action while following the ghost. */
void GhostPresentBox::exeStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
    }
    updatePoseByHistroryPath();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvGhostPresentBox.Wait);
    }
}
/** @brief Follows the ghost while awaiting collection. */
void GhostPresentBox::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
    updatePoseByHistroryPath();
}
/** @brief Keeps the stopped state idle. */
void GhostPresentBox::exeStop() {}
/** @brief Launches the collected box and waits for landing. */
void GhostPresentBox::exePopUp() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PopUp");
        al::setVelocityJump(this, 28.0f);
    }
    if (al::isStep(this, 3)) {
        al::onCollide(this);
    }
    al::addVelocityToGravity(this, 1.1f);
    al::scaleVelocity(this, 0.999f);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setVelocityZero(this);
        al::offCollide(this);
        al::setNerve(this, &NrvGhostPresentBox.Open);
    } else if (al::isInDeathArea(this)) {
        al::setNerve(this, &NrvGhostPresentBox.End);
    }
}
/** @brief Plays the stationary collection action inside a pipe. */
void GhostPresentBox::exePopUpInRouteDokan() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PopUp");
        al::setVelocityZero(this);
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvGhostPresentBox.Open);
    }
}
/** @brief Opens the box and emits its configured item. */
void GhostPresentBox::exeOpen() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Open");
    }
    if (al::isActionEnd(this)) {
        al::appearItemTiming(this, getItemName());
        al::setNerve(this, &NrvGhostPresentBox.End);
    }
}
/** @brief Removes the finished box. */
void GhostPresentBox::exeEnd() { kill(); }
/** @brief Reports the recorded pipe state. @return Whether the box follows a pipe traversal. */
bool GhostPresentBox::isInRouteDokan() const { return mInRouteDokan; }
