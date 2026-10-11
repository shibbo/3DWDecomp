#pragma once

#include <math/seadVector.h>

namespace al { class LiveActor; class HitSensor; }
class BallStateFallParam;
class BallStateThrowParam;

namespace BallStateFunction {
void calcLaunchSpeed(al::LiveActor*, const sead::Vector3f&, const BallStateThrowParam*);
void setPositionOnRelease(al::LiveActor*, const al::HitSensor*, const al::LiveActor*);
float calcSpeedFront(al::LiveActor*);
void rotateOnGround(al::LiveActor*, const sead::Vector3f&);
void rotateOnAir(al::LiveActor*, float, bool);
void sendMsgToCollision(al::LiveActor*, bool);
void reboundCollisionWallOrCeiling(al::LiveActor*);
bool reboundCollisionGround(al::LiveActor*, const BallStateFallParam*, bool);
void calcReflectSpeed(al::LiveActor*, const al::LiveActor*, const BallStateFallParam*);
al::HitSensor* tryGetRelativePlayerSensor(al::LiveActor*, al::HitSensor*);
bool getCollidedNormalAndPos(al::LiveActor*, sead::Vector3f*, sead::Vector3f*);
void setColliderReturnedSlowly(al::LiveActor*, s32, s32);
float calcRotateSpeed(al::LiveActor*, const sead::Vector3f&);
}
