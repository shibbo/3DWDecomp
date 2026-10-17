#include "MapObj/ItemStateLeaf.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/Collider.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include <cmath>
namespace {
NERVE_DECL(ItemStateLeaf, PopUp)
NERVE_DECL(ItemStateLeaf, TakeOut)
NERVE_DECL(ItemStateLeaf, FallLeaf)
NERVE_DECL(ItemStateLeaf, LandLeaf)
NERVES_MAKE_NOSTRUCT(ItemStateLeaf, PopUp, TakeOut, FallLeaf, LandLeaf)
} // namespace
ItemStateLeaf::ItemStateLeaf(al::LiveActor *actor,
                             const ItemStatePopUpFrontParam *param)
    : al::ActorStateBase("葉っぱ系アイテム出現ステート", actor), mParam(param) {
}
void ItemStateLeaf::init() { initNerve(&NrvItemStateLeafPopUp, 0); }
void ItemStateLeaf::appear() {
  al::invalidateClipping(mHostActor);
  al::invalidateHitSensors(mHostActor);
  if (mParam->_74)
    al::offCollide(mHostActor);
  mIsInWater = rc::isInWaterArea(mHostActor);
  al::ActorStateBase::appear();
  if (mIsTakeOut)
    al::setNerve(this, &NrvItemStateLeafTakeOut);
  else
    al::setNerve(this, &NrvItemStateLeafPopUp);
}
void ItemStateLeaf::kill() {
  if (!mIsTakeOut)
    al::validateClipping(mHostActor);
  al::onCollide(mHostActor);
  al::ActorStateBase::kill();
}
bool ItemStateLeaf::receiveMsg(const al::SensorMsg *msg, al::HitSensor *,
                               al::HitSensor *) {
  if (rc::isMsgJumpPanelAction(msg) && al::getVelocity(mHostActor).y <= 0.0f)
    al::getVelocityPtr(mHostActor)->y = 24.0f;
  return false;
}
void ItemStateLeaf::exePopUp() {
  bool inWater = rc::isInWaterArea(mHostActor);
  if (al::isFirstStep(this)) {
    al::showModelIfHide(mHostActor);
    al::tryStartAction(mHostActor, "PopUp");
    const sead::Vector3f &velocity = inWater ? mParam->_4C : mParam->_40;
    float vertical = velocity.y;
    float horizontal = velocity.z;
    sead::Vector3f front;
    al::calcFrontDir(&front, mHostActor);
    sead::Vector3f *velocityPtr = al::getVelocityPtr(mHostActor);
    *velocityPtr = horizontal * front - vertical * al::getGravity(mHostActor);
    if (std::fabs(vertical) < 0.1f && mParam->_77) {
      al::setNerve(this, &NrvItemStateLeafFallLeaf);
      return;
    }
  }
  if (EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(mHostActor))
    return;
  al::addVelocityToGravity(mHostActor, inWater ? mParam->_5C : mParam->_58);
  al::scaleVelocityHV(mHostActor, mParam->_60,
                      inWater ? mParam->_64 : mParam->_60);
  if (al::isGreaterEqualStep(this, 30)) {
    al::validateHitSensors(mHostActor);
    al::onCollide(mHostActor);
    al::setNerve(this, &NrvItemStateLeafFallLeaf);
  }
}
void ItemStateLeaf::exeTakeOut() {
  bool inWater = rc::isInWaterArea(mHostActor);
  if (al::isFirstStep(this)) {
    al::showModelIfHide(mHostActor);
    al::tryStartAction(mHostActor, "TakeOut");
    const sead::Vector3f &velocity = inWater ? mParam->_4C : mParam->_40;
    float vertical = velocity.y;
    float horizontal = velocity.z;
    sead::Vector3f front;
    al::calcFrontDir(&front, mHostActor);
    sead::Vector3f *velocityPtr = al::getVelocityPtr(mHostActor);
    *velocityPtr = horizontal * front - vertical * al::getGravity(mHostActor);
    al::onCollide(mHostActor);
  }
  if (EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(mHostActor))
    return;
  al::addVelocityToGravity(mHostActor, inWater ? mParam->_5C : mParam->_58);
  al::scaleVelocityHV(mHostActor, mParam->_60,
                      inWater ? mParam->_64 : mParam->_60);
  if (al::isGreaterEqualStep(this, 30))
    al::validateHitSensors(mHostActor);
  if (al::isOnGround(mHostActor, 0, 0.0f)) {
    al::validateHitSensors(mHostActor);
    kill();
  }
}
void ItemStateLeaf::exeFallLeaf() {
  if (al::isFirstStep(this))
    al::startAction(mHostActor, "FallLeaf");
  if (mParam->_77 && al::isStep(this, 30)) {
    al::validateHitSensors(mHostActor);
    al::onCollide(mHostActor);
  }
  if (EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(mHostActor))
    return;
  al::addVelocityToGravity(mHostActor, 0.012f);
  al::scaleVelocityHV(mHostActor, 0.97f, mParam->_60);
  sead::Vector3f hitPos(0.0f, 0.0f, 0.0f);
  sead::Vector3f direction(0.0f, -120.0f, 0.0f);
  al::Triangle triangle;
  if (alCollisionUtil::getFirstPolyOnArrow(mHostActor, &hitPos, &triangle,
                                           al::getTrans(mHostActor), direction,
                                           nullptr, nullptr)) {
    int frame = al::getActionFrame(mHostActor);
    float distance = (hitPos - al::getTrans(mHostActor)).length();
    if (frame < 15 || frame > 55 || distance < 105.0f) {
      al::getVelocityPtr(mHostActor)->y = distance * -0.0625f;
      al::setNerve(this, &NrvItemStateLeafLandLeaf);
      return;
    }
  }
  if (al::isOnGround(mHostActor, 0, 0.0f))
    kill();
}
void ItemStateLeaf::exeLandLeaf() {
  if (al::isFirstStep(this))
    al::startAction(mHostActor, "LandLeaf");
  if (EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(mHostActor))
    return;
  if (al::isOnGround(mHostActor, 0, 0.0f))
    kill();
}
