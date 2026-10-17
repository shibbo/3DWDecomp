#pragma once
#include <math/seadQuat.h>
#include <math/seadVector.h>
namespace al {
class RotateHangDynamics {
  public:
    RotateHangDynamics();
    void initPosLength(const sead::Vector3f&, float, const sead::Vector3f&);
    void moveRootPosAndFix(const sead::Vector3f&);
    void moveRootPos(const sead::Vector3f&);
    void applyForcePos(const sead::Vector3f&, const sead::Vector3f&);
    void applyGravity(const sead::Vector3f&);
    void update(float);
    sead::Quatf mQuat;
    unsigned char _10[0x10];
    sead::Vector3f mOmega;
    unsigned char _2c[0x10];
    float _3c;
    float mDamping;
    float _44;
    float _48;
    float mMaxAngle;
    unsigned char _50[8];
    sead::Vector3f mRootPos;
    sead::Vector3f mTipPos;
};
static_assert(sizeof(RotateHangDynamics) == 0x70);
} // namespace al
