#include "Layout/Switch/ShardCounterParts.hpp"
#include <prim/seadBitFlag.h>
#include "Layout/SingleModeSceneLayout.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
NERVE_DECL(ShardCounterParts, Add);
class ShardCounterPartsNrvAddComplete : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<ShardCounterParts>()->exeAdd();
    }
};
NERVE_DECL(ShardCounterParts, Complete);
NERVE_DECL(SingleModeCounterBase, Wait);
NERVES_MAKE_NOSTRUCT(ShardCounterParts, AddComplete, Add, Complete)
NERVES_MAKE_NOSTRUCT(SingleModeCounterBase, Wait)
}
/** @brief Creates the island shard counter and prepares its font correction.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Layout parts name.
 * @param pParent Owning single-mode HUD. */
ShardCounterParts::ShardCounterParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                   const char* pPartsName, al::LayoutActor* pParent)
    : SingleModeCounterBase(rInfo, pName, pPartsName, pParent),
      mParent(static_cast<SingleModeSceneLayout*>(pParent)) {
    mTextInfo.setTextBox(getLayoutKeeper()->getLayout(), "TxtShard", 2);
}
/** @brief Shows the counter with its moving shard icon hidden. */
void ShardCounterParts::appear() {
    al::LayoutActor::appear();
    al::hidePaneNoRecursive(this, "TxtShardGet");
}
/** @brief Applies text layout correction each frame. */
void ShardCounterParts::control() { mTextInfo.applyFix(); }
/** @brief Animates collecting a shard and starts the collecting player's feedback.
 * @param islandId Island identifier.
 * @param shardIndex One-based shard index.
 * @param isComplete Whether this shard completes the collection.
 * @param isDemo Whether Bowser Jr. collected the shard. */
void ShardCounterParts::addShard(s32 islandId, s32 shardIndex, bool isComplete, bool isDemo) {
    mPendingShard = isComplete ? shardIndex - 1 : -1;
    if (isHide()) {
        appear();
        al::startAction(this, "Show", "Main");
    }
    mIslandId = islandId;
    al::showPaneNoRecursive(this, "TxtShardGet");
    sead::Vector3f pos;
    al::StringTmp<32> paneName("ShardPos%d", shardIndex);
    pos = al::getPaneLocalTrans(this, paneName.cstr());
    al::setPaneLocalTrans(this, "TxtShardGet", pos);
    updateString(mIslandId, shardIndex - 1);
    al::startAction(this, "Add", "Shard");
    al::LiveActor* pPlayer;
    if (isDemo) pPlayer = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
    else pPlayer = rc::tryFindPlayerFromInputPort(mParent->getPlayerHolder(), al::getMainControllerPort(), false);
    if (pPlayer) al::startHitReaction(pPlayer, "ShardGet");
    if (isComplete) al::setNerve(this, &NrvShardCounterPartsAddComplete);
    else al::setNerve(this, &NrvShardCounterPartsAdd);
}
/** @brief Builds five shard icons while excluding the moving shard.
 * @param islandId Island identifier.
 * @param excludedShard Zero-based index to omit, or -1. */
void ShardCounterParts::updateString(s32 islandId, s32 excludedShard) {
    sead::WFixedSafeString<16> text;
    sead::BitFlag8 flags(SingleModeDataFunction::getShardFlag(GameDataHolderAccessor(this), islandId));
    if (mPendingShard >= 0) flags.setBit(mPendingShard);
    if (excludedShard >= 0) flags.resetBit(excludedShard);
    int count = 0;
    for (int i = 0; i < 5; ++i) {
        if (flags.isOnBit(i)) {
            text.append(LayoutFontUtil::getIconFontShard());
            ++count;
        } else text.append(LayoutFontUtil::getIconFontCatShineEmpty());
    }
    if (count < 5 || mIsShardDemo) al::setPaneString(this, "TxtShard", text.cstr(), 0, -1);
    else hide();
}
/** @brief Refreshes icons when entering another island.
 * @param islandId Island identifier, or -1 outside an island. */
void ShardCounterParts::updateCount(s32 islandId) {
    if (SingleModeDataFunction::isPhase0(GameDataHolderAccessor(this))) {
        al::setPaneString(this, "TxtShard", u"", 0, -1);
        al::setPaneString(this, "TxtShardBase", u"", 0, -1);
        return;
    }
    if (al::isNerve(this, &NrvShardCounterPartsAdd) ||
        al::isNerve(this, &NrvShardCounterPartsAddComplete) ||
        al::isNerve(this, &NrvShardCounterPartsComplete) || mIslandId == islandId) return;
    mIslandId = islandId;
    if (islandId >= 0) updateString(islandId, -1);
    else hide();
}
/** @brief Finishes a shard's motion and proceeds to completion or idle. */
void ShardCounterParts::exeAdd() {
    if (!al::isActionEnd(this, "Shard")) return;
    al::hidePaneNoRecursive(this, "TxtShardGet");
    if (mIslandId < 0) { hide(); return; }
    updateString(mIslandId, -1);
    if (al::isNerve(this, &NrvShardCounterPartsAddComplete))
        al::setNerve(this, &NrvShardCounterPartsComplete);
    else al::setNerve(this, &NrvSingleModeCounterBaseWait);
}
/** @brief Delays the completion animation, then hides the counter. */
void ShardCounterParts::exeComplete() {
    if (al::isLessStep(this, 15)) return;
    if (al::isStep(this, 15)) al::startAction(this, "Complete", nullptr);
    if (al::isActionEnd(this, nullptr)) hide();
}
/** @brief Clears the collection demo and pending shard. */
void ShardCounterParts::endShardDemo() {
    mIsShardDemo = false;
    mPendingShard = -1;
}
/** @brief Holds the completed state. */
void ShardCounterParts::exeCompleteEnd() {}
/** @brief Refreshes the current island's icons after a demo. */
void ShardCounterParts::endDemo() {
    if (mIslandId >= 0) updateString(mIslandId, -1);
}
