#include "MapObj/KinokoGiga.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "Demo/DemoAnimatic.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
    NERVE_DECL(KinokoGiga, Wait);
    NERVE_DECL(KinokoGiga, PopUpFront);
    NERVES_MAKE_NOSTRUCT(KinokoGiga, Wait, PopUpFront)
}
KinokoGiga::KinokoGiga(const char* name) : al::LiveActor(name) {}
void KinokoGiga::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "KinokoGiga", nullptr);
    al::initNerve(this, &NrvKinokoGigaWait, 1);
    if (al::calcLinkChildNum(info, "SwitchCutscene") >= 1) {
        al::PlacementInfo placement;
        al::getLinksInfoByIndex(&placement, info, "SwitchCutscene", 0);
        al::ActorInitInfo demoInfo;
        demoInfo.initNoViewId(&placement, info);
        mDemo = new DemoAnimatic("GigaGetDemo", static_cast<alSeFunction::DemoType>(2));
        mDemo->init(demoInfo);
        bool endScene = false;
        al::tryGetArg(&endScene, info, "IsDemoEndScene");
        if (endScene) mDemo->setEndSceneFlag();
    }
    mPopUpState = new ItemStatePopUpFront(this);
    al::initNerveState(this, mPopUpState, &NrvKinokoGigaPopUpFront, "PopUpFront");
    makeActorAppeared();
}
void KinokoGiga::appear() {
    al::LiveActor::appear();
    al::startSe(this, "PgAppear", nullptr);
    al::setNerve(this, &NrvKinokoGigaWait);
    al::invalidateClipping(this);
}
void KinokoGiga::appearPopUpFront() {
    al::setScaleAll(this, 4.0f);
    al::setSensorRadius(this, "Body", 1200.0f);
    ItemStatePopUpFrontParam param;
    appearPopUpFront(param);
}
void KinokoGiga::appearPopUpFront(const ItemStatePopUpFrontParam& param) {
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::hideModelIfShow(this);
    al::LiveActor::appear();
    mPopUpState->setParam(param, nullptr);
    al::setNerve(this, &NrvKinokoGigaPopUpFront);
}
void KinokoGiga::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvKinokoGigaWait) && al::isSensorMapObj(receiver) && al::isSensorEye(sender) && al::isSensorName(sender, "KinokoGigaBody")) al::sendMsgPushAndKillVelocityToTarget(this, sender, receiver);
}
bool KinokoGiga::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvKinokoGigaWait) && al::isSensorEye(sender) && al::isSensorName(sender, "KinokoGigaBody") && al::isSensorMapObj(receiver) && al::tryReceiveMsgPushAndAddVelocity(this, msg, sender, receiver, 1.0f)) return true;
    if (al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerBoomerangReflect(msg)) return true;
    if (!al::isSensorPlayer(sender)) return false;
    if (al::isMsgItemGetDirectAll(msg)) {
        if (mDemo) mDemo->startDemo();
        rc::addScore(this, sender, 0.0f, 0);
        rc::acquirerItemKinokoGiga(this, sender);
        al::startHitReactionGet(this);
        al::validateClipping(this);
        kill();
        return true;
    }
    return false;
}
bool KinokoGiga::receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) { return false; }
void KinokoGiga::control() { al::updateMaterialCodeWater(this); }
void KinokoGiga::exeWait() {}
void KinokoGiga::exePopUpFront() { al::updateNerveStateAndNextNerve(this, &NrvKinokoGigaWait); }
