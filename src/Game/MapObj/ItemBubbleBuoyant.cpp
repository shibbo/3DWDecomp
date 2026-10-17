#include "MapObj/ItemBubbleBuoyant.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "MapObj/CoinAttach.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include <cmath>
#include <math/seadMathCalcCommon.h>
namespace {
NERVE_DECL(ItemBubbleBuoyant, Fall)
NERVE_DECL(ItemBubbleBuoyant, FloatOnWater)
NERVES_MAKE_NOSTRUCT(ItemBubbleBuoyant, Fall, FloatOnWater)
} // namespace
ItemBubbleBuoyant::ItemBubbleBuoyant(const char *name, int type)
    : ItemBubble(name, type) {}
void ItemBubbleBuoyant::init(const al::ActorInitInfo &info) {
  ItemBubble::init(info);
  if (mItemActor)
    static_cast<CoinAttach *>(mItemActor)->setCheckWaterEntryOnly(true);
}
void ItemBubbleBuoyant::appear() {
  ItemBubble::appear();
  mLife = 0;
  al::setNerve(this, &NrvItemBubbleBuoyantFall);
}
void ItemBubbleBuoyant::control() {
  ++mLife;
  if (mLife >= 600 || al::isDead(mItemActor))
    kill();
}
void ItemBubbleBuoyant::exeFall() {
  if (al::isFirstStep(this))
    al::startAction(this, "Wait");
  if (rc::isInWaterArea(this)) {
    al::setVelocity(this, sead::Vector3f::zero);
    al::setNerve(this, &NrvItemBubbleBuoyantFloatOnWater);
    return;
  }
  al::addVelocityToGravity(this, 0.5f);
  al::scaleVelocity(this, 0.996f);
  updatePosture();
  if (alCollisionUtil::getFirstPolyOnArrow(
          this, nullptr, nullptr, al::getTrans(this),
          sead::Vector3f::ey * -50.0f, nullptr, nullptr))
    kill();
}
void ItemBubbleBuoyant::exeFloatOnWater() {
  if (al::isFirstStep(this))
    mFloatOrigin = al::getTrans(this);
  float angle = static_cast<float>(al::getNerveStep(this)) / 120.0f * 360.0f;
  float height = sead::Mathf::sin(sead::Mathf::deg2rad(angle)) * 25.0f;
  al::setTrans(this, height * sead::Vector3f::ey + mFloatOrigin);
  updatePosture();
}
