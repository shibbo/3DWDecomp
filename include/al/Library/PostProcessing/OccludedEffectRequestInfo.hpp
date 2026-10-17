#pragma once
#include <math/seadVector.h>

namespace agl::fx {
class OfxLensFlareDynamic;
}

namespace al {

class OccludedEffectRequestInfo {
public:
    OccludedEffectRequestInfo(agl::fx::OfxLensFlareDynamic* pLensFlare);

    void requestByPos(const sead::Vector3f&);

private:
    agl::fx::OfxLensFlareDynamic* mLensFlare;
};

}  // namespace al
