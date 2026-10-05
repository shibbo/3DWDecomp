#include "MapObj/IUseTimer.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Util/PlayerUtil.hpp"

namespace rc {

bool IUseTimer::init(const al::ActorInitInfo& rInfo, const char* pLinkName) {
    if (al::calcLinkChildNum(rInfo, pLinkName) == 0) {
        return false;
    }
    mAreaGroup = new al::AreaObjGroup(pLinkName, pLinkName, rInfo);
    return true;
}

bool IUseTimer::isPlayerNotInBounds(al::LiveActor* pActor, bool isCheckDokan) {
    al::LiveActor* player = al::tryFindAlivePlayerActorFirst(pActor);
    if (!player) {
        return false;
    }
    if (mAreaGroup) {
        sead::Vector3f trans = al::getTrans(player);
        if (!mAreaGroup->getInVolumeAreaObj(trans)) {
            return true;
        }
    }
    if (isCheckDokan) {
        return rc::isPlayerInRouteDokanOrDokan(player);
    }
    return false;
}

}
