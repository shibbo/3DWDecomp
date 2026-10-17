#pragma once
#include "MapObj/ItemBubble.hpp"
class ItemBubbleBuoyant : public ItemBubble {
public:
  ItemBubbleBuoyant(const char *name, int type);
  void init(const al::ActorInitInfo &info) override;
  void appear() override;
  void control() override;
  void exeFall();
  void exeFloatOnWater();

private:
  int mLife = 0;
  sead::Vector3f mFloatOrigin = sead::Vector3f::zero;
};
static_assert(sizeof(ItemBubbleBuoyant) == 0x178);
