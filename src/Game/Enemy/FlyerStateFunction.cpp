#include "Enemy/FlyerStateFunction.hpp"
#include "Enemy/FlyerStateParam.hpp"
#include "Library/ActorUtil.hpp"

/** @brief Moves a flying actor's height toward its reference height.
 * @param pActor Flying actor.
 * @param height Reference height.
 * @param pParam Height damping settings.
 */
void FlyerStateFunction::recoverHeight(al::LiveActor* pActor, float height,
                                       const FlyerStateParam* pParam) {
    sead::Vector3f trans = al::getTrans(pActor);
    trans.y = height + (trans.y - height) * pParam->mHeightDamping;
    al::setTrans(pActor, trans);
}
