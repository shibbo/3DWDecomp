#include "MapObj/BoxLightCollisionChecker.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
    NERVE_DECL(BoxLightCollisionChecker, NoHit);
    NERVE_DECL(BoxLightCollisionChecker, ToHitCheck);
    NERVE_DECL(BoxLightCollisionChecker, Hit);
    NERVE_DECL(BoxLightCollisionChecker, ToNoHitCheck);
    NERVES_MAKE_NOSTRUCT(BoxLightCollisionChecker, NoHit, ToHitCheck, Hit, ToNoHitCheck)
}

BoxLightCollisionChecker::BoxLightCollisionChecker()
    : al::NerveExecutor("ライトボックスのコリジョンチェック") {
    initNerve(&NrvBoxLightCollisionCheckerNoHit, 0);
}

void BoxLightCollisionChecker::exeNoHit() {
    if (mIsColliding)
        al::setNerve(this, &NrvBoxLightCollisionCheckerToHitCheck);
}

void BoxLightCollisionChecker::exeToHitCheck() {
    if (mIsColliding) {
        if (al::isGreaterStep(this, 5))
            al::setNerve(this, &NrvBoxLightCollisionCheckerHit);
    } else {
        al::setNerve(this, &NrvBoxLightCollisionCheckerNoHit);
    }
}

void BoxLightCollisionChecker::exeHit() {
    if (!mIsColliding)
        al::setNerve(this, &NrvBoxLightCollisionCheckerToNoHitCheck);
}

void BoxLightCollisionChecker::exeToNoHitCheck() {
    if (mIsColliding) {
        al::setNerve(this, &NrvBoxLightCollisionCheckerHit);
    } else if (al::isGreaterStep(this, 5)) {
        al::setNerve(this, &NrvBoxLightCollisionCheckerNoHit);
    }
}

bool BoxLightCollisionChecker::isHit() const {
    return al::isNerve(this, &NrvBoxLightCollisionCheckerHit) ||
           al::isNerve(this, &NrvBoxLightCollisionCheckerToNoHitCheck);
}
