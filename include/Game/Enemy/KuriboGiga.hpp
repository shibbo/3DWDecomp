#pragma once
#include "Enemy/Kuribo.hpp"

/** @brief Large Goomba that splits into two smaller Goombas when defeated. */
class KuriboGiga : public Kuribo {
public:
    explicit KuriboGiga(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void initSpecial(const al::ActorInitInfo& rInfo, f32 scale);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
private:
    KuriboGiga* mFirstChild = nullptr;
    KuriboGiga* mSecondChild = nullptr;
};
static_assert(sizeof(KuriboGiga) == 0x218);
