#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

/** @brief The shuriken Punpun (Motley Bossblob) holds and throws. */
class PunpunShuriken : public al::LiveActor {
public:
    explicit PunpunShuriken(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void initShurikenWithArchiveName(const al::ActorInitInfo& rInfo, const char* pArchiveName);
    void appear() override;
    void appearDemo();
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void shoot(const sead::Vector3f& rDir, bool isLv2);
    void exeWait();
};
