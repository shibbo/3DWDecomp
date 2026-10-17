#include "Layout/ListStampDemoParts.hpp"
#include "Layout/ListStampParts.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
namespace {
NERVE_DECL(ListStampDemoParts, Wait);
NERVE_DECL(ListStampDemoParts, Decide);
NERVES_MAKE_NOSTRUCT(ListStampDemoParts, Wait, Decide)
}
/** @brief Creates stamp presentation parts and binds the parent's stamp effects.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Parts pane name.
 * @param pParent Parent layout and effect owner.
 * @param pList Optional stamp list providing the stamp texture. */
ListStampDemoParts::ListStampDemoParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                     const char* pPartsName, al::LayoutActor* pParent,
                                     ListStampParts* pList)
    : al::LayoutActor(pName), mParent(pParent), mList(pList) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvListStampDemoPartsWait, 0);
    al::setEffectFollowPosPtr(mParent, "Stamp100", &mEffectPos);
    al::setEffectFollowPosPtr(mParent, "Stamp200", &mEffectPos);
    al::initLayoutPartsAudioKeeper(this, rInfo, "ListStampDemoParts");
    mTextureInfo = al::createTextureInfo();
}
/** @brief Activates the layout with its root pane hidden. */
void ListStampDemoParts::appear() {
    al::LayoutActor::appear();
    al::hidePaneRoot(this);
}
/** @brief Shows the stamp texture and begins the acquisition presentation.
 * @param pStampName Stamp animation name.
 * @param rTrans Position of the stamp.
 * @param pPaneName Stamp pane whose color texture is copied. */
void ListStampDemoParts::startDecide(const char* pStampName, const sead::Vector3f& rTrans,
                                   const char* pPaneName) {
    appear();
    al::showPaneRoot(this);
    const char* pType = al::isEqualSubString(pStampName, "Stamp100") ? "Stamp100" : "Stamp200";
    al::StringTmp<64> picturePane("Pic%s", pType);
    if (mList) {
        al::StringTmp<64> sourcePane("%s_c", pPaneName);
        al::getPaneTextureInfo(mTextureInfo, mList, sourcePane.cstr());
        al::setPaneTexture(this, picturePane.cstr(), mTextureInfo);
    }
    al::setLocalTrans(this, rTrans);
    al::tryStartAction(this, pType, "Main");
    mEffectPos.set(rTrans);
    al::emitEffect(mParent, al::isEqualSubString(pStampName, "Stamp100") ? "Stamp100" : "Stamp200", nullptr);
    al::setNerve(this, &NrvListStampDemoPartsDecide);
}
/** @brief Checks whether the acquisition presentation has returned to idle. */
bool ListStampDemoParts::isDecideEnd() const { return al::isNerve(this, &NrvListStampDemoPartsWait); }
/** @brief Hides the root pane on entry to idle. */
void ListStampDemoParts::exeWait() {
    if (al::isFirstStep(this)) al::hidePaneRoot(this);
}
/** @brief Plays timed stamp sounds and waits for the presentation to finish. */
void ListStampDemoParts::exeDecide() {
    if (al::isStep(this, 10)) al::startSe(this, "PgStampStart", nullptr);
    if (al::isStep(this, 60)) al::startSe(this, "PgStampOn", nullptr);
    if (al::isActionEnd(this, "Main")) al::setNerve(this, &NrvListStampDemoPartsWait);
}
