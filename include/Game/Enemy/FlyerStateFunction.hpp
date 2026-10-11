#pragma once

namespace al {
class LiveActor;
}
struct FlyerStateParam;

namespace FlyerStateFunction {
void recoverHeight(al::LiveActor* pActor, float height, const FlyerStateParam* pParam);
}
