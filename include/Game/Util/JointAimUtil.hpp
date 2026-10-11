#pragma once

namespace al {
class LiveActor;
class JointAimInfo;
}

namespace JointAimUtil {
void updateEyeJointInfo(al::LiveActor* pActor, al::JointAimInfo* pInfo,
                        float distance, float rate);
}
