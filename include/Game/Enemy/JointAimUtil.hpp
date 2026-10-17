#pragma once
#include <basis/seadTypes.h>
namespace al { class LiveActor; class JointAimInfo; }
namespace JointAimUtil {
void updateEyeJointInfo(al::LiveActor* pActor, al::JointAimInfo* pInfo, f32 radius, f32 rate);
}
