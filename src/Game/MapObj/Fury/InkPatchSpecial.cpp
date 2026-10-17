#include "MapObj/Fury/InkPatchSpecial.hpp"
#include "MapObj/Fury/InkPatch.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
InkPatch* InkPatchSpecial::tryGetSpecialInkPatch(const al::IUseSceneObjHolder* pUser) {
    auto* special = tryGetInkPatchSpecial(pUser);
    return special ? special->mSpecialPatch : nullptr;
}
InkPatchSpecial* InkPatchSpecial::tryGetInkPatchSpecial(const al::IUseSceneObjHolder* pUser) {
    return static_cast<InkPatchSpecial*>(al::tryGetSceneObj(pUser, 61));
}
InkPatchSpecial::InkPatchSpecial() {}
void InkPatchSpecial::addUnlockerInkPatch(InkPatch* pPatch) {
    for (int i = 0; i < 2; ++i) {
        if (!mUnlockers[i]) {
            mUnlockers[i] = pPatch;
            return;
        }
    }
}
bool InkPatchSpecial::isUnlockerInkPatch(InkPatch* pPatch) {
    for (int i = 0; i < 2; ++i) {
        if (mUnlockers[i] == pPatch)
            return true;
    }
    return false;
}
bool InkPatchSpecial::isLastUnlockerInkPatch(InkPatch* pPatch) {
    if (!isUnlockerInkPatch(pPatch))
        return false;
    for (int i = 0; i < 2; ++i) {
        if (!mUnlockers[i])
            return false;
        if (mUnlockers[i] != pPatch && !al::isDead(mUnlockers[i]))
            return false;
    }
    return true;
}
const char* InkPatchSpecial::getSceneObjName() const { return "InkPatch"; }
