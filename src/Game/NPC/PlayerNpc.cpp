#include "NPC/PlayerNpc.hpp"
#include "Player/PlayerNameItem.hpp"
#include "Layout/RingPlayerSelect.hpp"
#include "Demo/DemoScenePlayerModel.hpp"
#include "Player/Normal/PlayerRetargettingSelectorSceneObj.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Controller/PadRumbleKeeper.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
namespace {
NERVE_DECL(PlayerNpc, Wait)
class PlayerNpcNrvSurprise : public al::Nerve {
public:
    /** @brief Runs the surprised idle state. @param pKeeper State owner. */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<PlayerNpc>()->exeWait();
    }
};
NERVE_DECL(PlayerNpc, Happy)
NERVES_MAKE_NOSTRUCT(PlayerNpc, Happy)
const struct {
    PlayerNpcNrvWait Wait;
    PlayerNpcNrvSurprise Surprise;
} NrvPlayerNpc;
} // namespace
/** @brief Creates a locked-character NPC. @param pName Actor name. */
PlayerNpc::PlayerNpc(const char* pName) : al::LiveActor(pName), mModel(nullptr) {}
/** @brief Releases the NPC actor. */
PlayerNpc::~PlayerNpc() = default;
/** @brief Creates the character model and contact sensors. @param rInfo Placement initialization.
 */
void PlayerNpc::init(const al::ActorInitInfo& rInfo) {
    const char* pModel = "Luigi";
    alPlacementFunction::tryGetModelName(&pModel, rInfo);
    const auto* pName = PlayerNameItem::getPlayerNameItem(pModel);
    mCharacter = pName->mCharacter;
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    al::StringTmp<128> modelName("DemoModel_%s", pModel);
    modelName.cstr();
    al::setScale(this, sead::Vector3f(1, 1, 1));
    al::initActorClipping(this, rInfo);
    al::initActorModelKeeper(this, rInfo, "ObjectData/Mario", 6, "ObjectData/MarioAnimation");
    al::initActorOceanWaveKeeper(this, rInfo, "Mario", "InitOceanWaveHitReaction");
    al::initActorEffectKeeper(this, rInfo, "PlayerHitReaction", true);
    al::initActorItemKeeper(this, rInfo, "ObjectData/Mario", "InitItemItemStock");
    al::initActorAudioKeeper(this, rInfo, "PlayerActor", nullptr);
    al::setSeSeqLocalVariableDefault(this, 0, pName->mVoice);
    initActionKeeper("Mario", nullptr);
    al::initHitReactionKeeper(this, "Player");
    al::initExecutorMapObj(this, rInfo);
    al::invalidateClipping(this);
    al::initNerve(this, &NrvPlayerNpc.Wait, 0);
    al::createPadRumbleKeeper(this, -1);
    auto* pSelector =
        static_cast<PlayerRetargettingSelectorSceneObj*>(al::createSceneObj(this, 25));
    mModel = DemoScenePlayerModel::createSingle(rInfo, pModel, 0, pSelector, this, nullptr);
    mModel->showShadow();
    auto* pActor = mModel->getModel();
    initHitSensor(2);
    al::addHitSensorMapObj(this, rInfo, "Body", 90, 16, sead::Vector3f(0, 30, 0));
    al::setHitSensorMtxPtr(this, "Body", al::getJointMtxPtr(pActor, "JointRoot"));
    al::addHitSensorEye(this, rInfo, "Search", 35, 64, sead::Vector3f(0, 100, 0));
    mConnector = al::tryCreateMtxConnector(this, rInfo);
    al::tryGetArg(&mScenario, rInfo, "ScenarioID");
    al::hideModel(this);
}
/** @brief Attaches to the floor and hides characters already unlocked. */
void PlayerNpc::initAfterPlacement() {
    if (mConnector) {
        al::attachMtxConnectorToCollision(mConnector, this, 50, 300);
    }
    mZone = mPlacementHolder->getZoneNo();
    if (SingleModeDataFunction::hasUnlockedChar(GameDataHolderAccessor(this), mCharacter)) {
        kill();
    } else {
        makeActorAppeared();
    }
}
/** @brief Shows the actor and its character model. */
void PlayerNpc::appear() {
    al::LiveActor::appear();
    mModel->appear();
}
/** @brief Hides the character model and actor. */
void PlayerNpc::kill() {
    mModel->kill();
    al::LiveActor::kill();
}
/** @brief Updates the character model from the actor pose. */
void PlayerNpc::control() { mModel->update(this); }
/** @brief Alternates idle and surprised animations. */
void PlayerNpc::exeWait() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvPlayerNpc.Wait)) {
            mModel->startAction("Wait");
        } else {
            mModel->startAction("CourseSelectSurprise");
        }
    }
    if (al::isNerve(this, &NrvPlayerNpc.Wait)) {
        if (al::isStep(this, 120)) {
            al::setNerve(this, &NrvPlayerNpc.Surprise);
        }
    } else if (al::isActionEnd(mModel->getModel())) {
        al::setNerve(this, &NrvPlayerNpc.Wait);
    }
}
/** @brief Finishes the celebration and unlocks the character. */
void PlayerNpc::exeHappy() {
    if (al::isFirstStep(this)) {
        mModel->startAction("GoalPoseShort");
    }
    if (al::isActionEnd(mModel->getModel())) {
        {
            auto trans = al::getTrans(this) + sead::Vector3f(0, 393, 0);
            al::setTrans(this, trans);
        }
        sead::Matrix34f mtx;
        alActorPoseFunction::calcBaseMtx(&mtx, this);
        if (getModelKeeper() && getModelKeeper()->isNeedSetBaseMtxAndCalcAnim()) {
            al::setBaseMtxAndCalcAnim(this, mtx, al::getScale(this));
        }
        al::startHitReaction(this, "\u6d88\u6ec5");
        SingleModeDataFunction::setUnlockedChar(GameDataHolderWriter(this), mCharacter);
        kill();
        auto* pSelect = al::getSceneObj<RingPlayerSelect>(this, 41);
        if (pSelect) {
            pSelect->requestUnlockSequence(mCharacter);
        }
    }
}
/** @brief Starts the unlock celebration on player contact. @param pSelf Own sensor. @param pOther
 * Contact sensor. */
void PlayerNpc::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorMapObj(pSelf) && al::isSensorPlayer(pOther)) {
        if (!al::isNerve(this, &NrvPlayerNpcHappy)) {
            al::setNerve(this, &NrvPlayerNpcHappy);
        }
        if (al::sendMsgPush(pOther, pSelf)) {
            return;
        }
    }
    if (al::isSensorNpc(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}
/** @brief Ignores ordinary sensor messages. @param pMessage Unused message. @param pOther Unused
 * sender. @param pSelf Unused receiving sensor. @return Always false. */
bool PlayerNpc::receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                           al::HitSensor* pSelf) {
    return false;
}
/** @brief Accepts touch assistance. @param pMessage Touch message. @param pPointer Unused pointer.
 * @param pTarget Unused target. @return Whether this is a touch-assist message. */
bool PlayerNpc::receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                                      al::ScreenPointTarget* pTarget) {
    return al::isMsgTouchAssist(pMessage);
}
