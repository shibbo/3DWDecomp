#include "Enemy/GeneralEnemy.hpp"

#include <attributes.h>
#include <prim/seadEnum.h>

#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
NERVE_DECL(GeneralEnemy, Common)
NERVES_MAKE_NOSTRUCT(GeneralEnemy, Common)

// clang-format off
SEAD_ENUM(ReactionType, Trample, HipDrop, Kick)
// clang-format on
}  // namespace

namespace GeneralEnemySub {

/** @brief Creates an empty nerve map; call allocBuffer() before inserting. */
NerveMap::NerveMap() = default;

/**
 * @brief Allocates room for the nerves.
 * @param size Maximum number of nerves.
 */
void NerveMap::allocBuffer(s32 size) {
    mNames.allocBuffer(size, nullptr);
    mNerves.allocBuffer(size, nullptr);
}

/**
 * @brief Adds a nerve to the map.
 * @param pName Name the nerve is looked up by.
 * @param pNerve Nerve to add.
 */
void NerveMap::insert(const char* pName, GeneralEnemyNerve* pNerve) {
    *mNames.birthBack() = pName;
    mNerves.pushBack(pNerve);
}

/**
 * @brief Looks up a nerve by name.
 * @param pName Name of the nerve.
 * @return The nerve, or nullptr if there is none with that name.
 */
GeneralEnemyNerve* NerveMap::find(const char* pName) {
    for (s32 i = 0; i < mNames.size(); i++) {
        if (al::isEqualString(pName, *mNames.unsafeAt(i))) {
            return mNerves.at(i);
        }
    }

    return nullptr;
}

/**
 * @brief Looks up the index of a nerve by name.
 * @param pName Name of the nerve.
 * @return Index of the nerve, or -1 if there is none with that name.
 */
s32 NerveMap::findIndex(const char* pName) {
    for (s32 i = 0; i < mNames.size(); i++) {
        if (al::isEqualString(pName, *mNames.unsafeAt(i))) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Removes a nerve from the map.
 * @param pName Name of the nerve.
 */
void NerveMap::erase(const char* pName) {
    s32 index = findIndex(pName);
    mNames.erase(index);
    mNerves.erase(index);
}

}  // namespace GeneralEnemySub

namespace {

using generalEnemyElements::behavior::GEBehaviorBase;
using generalEnemyElements::condition::GEConditionBase;

/** @brief Maps an element type name in the data file to the function creating it. */
template <typename T>
struct ElementCreator {
    const char* mName;
    T* (*mCreateFunc)(const al::ByamlIter* pIter);
};

/**
 * @brief Creates a nerve element and reads its parameters.
 * @param pIter Element data, or nullptr to keep the defaults.
 * @return The new element.
 */
template <typename Base, typename T>
Base* createElement(const al::ByamlIter* pIter) {
    Base* element = new T;
    if (pIter != nullptr) {
        element->init(*pIter);
    }

    return element;
}

const ElementCreator<GEBehaviorBase> cBehaviorCreators[] = {
    {"RailWait", createElement<GEBehaviorBase, generalEnemyElements::behavior::RailWait>},
    {"RailMove", createElement<GEBehaviorBase, generalEnemyElements::behavior::RailMove>},
    {"RailBrake", createElement<GEBehaviorBase, generalEnemyElements::behavior::RailBrake>},
    {"Kill", createElement<GEBehaviorBase, generalEnemyElements::behavior::Kill>},
};

const ElementCreator<GEConditionBase> cConditionCreators[] = {
    {"False", createElement<GEConditionBase, generalEnemyElements::condition::FalseCondition>},
    {"True", createElement<GEConditionBase, generalEnemyElements::condition::TrueCondition>},
    {"ActionEnd", createElement<GEConditionBase, generalEnemyElements::condition::ActionEnd>},
    {"Timer", createElement<GEConditionBase, generalEnemyElements::condition::Timer>},
    {"IsVelocityZero",
     createElement<GEConditionBase, generalEnemyElements::condition::IsVelocityZero>},
    {"Random", createElement<GEConditionBase, generalEnemyElements::condition::Random>},
};

/**
 * @brief Creates an element by its type name.
 * @param rCreators Table of the element types.
 * @param pName Type name.
 * @param pIter Element data.
 * @return The new element, or nullptr if the type is unknown.
 */
template <typename T, s32 N>
T* createElementByName(const ElementCreator<T> (&rCreators)[N], const char* pName,
                       const al::ByamlIter* pIter) {
    for (s32 i = 0; i < N; i++) {
        if (al::isEqualString(pName, rCreators[i].mName)) {
            return rCreators[i].mCreateFunc(pIter);
        }
    }

    return nullptr;
}

}  // namespace

/**
 * @brief Constructs the general enemy.
 * @param pName Actor name.
 */
GeneralEnemy::GeneralEnemy(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the enemy from its model archive and its data file.
 * @param rInfo Actor placement and scene information.
 */
void GeneralEnemy::init(const al::ActorInitInfo& rInfo) {
    if (!al::tryGetStringArg(&mModelArcName, rInfo, "ModelArc")) {
        mModelArcName = "TestAndoGeneralEnemy";
    }

    const char* enemyName = nullptr;
    if (!al::tryGetStringArg(&enemyName, rInfo, "EnemyName")) {
        enemyName = "未設定汎用敵";
    }

    mDisplayName.format("試作%s", enemyName);
    mActorName = mDisplayName.cstr();

    if (!al::tryGetStringArg(&mDataFileName, rInfo, "DataFile")) {
        mDataFileName = "GeneralEnemy";
    }

    const u8* data = al::tryGetBymlFromObjectResource(mModelArcName, mDataFileName);
    al::initActorWithArchiveName(this, rInfo, mModelArcName, mDataFileName);
    if (al::isExistRail(this)) {
        al::setRailClippingInfo(&mRailClippingInfo, this, 100.0f, 100.0f);
    }

    mReactions.allocBuffer(ReactionType::size(), nullptr);
    for (s32 i = 0; i < ReactionType::size(); i++) {
        GeneralEnemySub::Reaction* reaction = mReactions.emplaceBack();
        reaction->mName = ReactionType::text(i);
        reaction->mNerveName = nullptr;
    }

    initNerve(data);
    makeActorAppeared();
}

/**
 * @brief Builds the nerves described by the data file.
 * @param pData Data file, or nullptr to start without nerves.
 */
void GeneralEnemy::initNerve(const u8* pData) {
    if (pData == nullptr) {
        mNerveMap.allocBuffer(16);
        al::initNerve(this, &NrvGeneralEnemyCommon, 0);
        return;
    }

    al::ByamlIter root(pData);
    if (!root.isTypeHash()) {
        return;
    }

    al::ByamlIter nerves;
    if (!root.tryGetIterByKey(&nerves, "Nerve")) {
        return;
    }

    if (!nerves.isTypeArray() || nerves.getSize() == 0) {
        return;
    }

    mNerveMap.allocBuffer(nerves.getSize() + 16);

    const char* defaultNerveName = nullptr;
    for (s32 i = 0; i < nerves.getSize(); i++) {
        al::ByamlIter nerveIter;
        if (!nerves.tryGetIterByIndex(&nerveIter, i) || !nerveIter.isTypeHash()) {
            return;
        }

        const char* name = nullptr;
        if (!nerveIter.tryGetStringByKey(&name, "Name")) {
            return;
        }

        const char* actionName = nullptr;
        nerveIter.tryGetStringByKey(&actionName, "Action");
        const char* behaviorName = nullptr;
        nerveIter.tryGetStringByKey(&behaviorName, "Behavior");

        auto* nerve = new GeneralEnemySub::GeneralEnemyNerve(name, actionName, behaviorName);
        if (behaviorName != nullptr) {
            nerve->mBehavior = createElementByName(cBehaviorCreators, behaviorName, &nerveIter);
        }

        if (mNerveMap.find(name) != nullptr) {
            return;
        }

        al::ByamlIter shifts;
        if (nerveIter.tryGetIterByKey(&shifts, "Shift")) {
            if (!shifts.isTypeArray()) {
                return;
            }

            nerve->mShifts.allocBuffer(shifts.getSize() + 1, nullptr);
            for (s32 j = 0; j < shifts.getSize(); j++) {
                al::ByamlIter shiftIter;
                if (!shifts.tryGetIterByIndex(&shiftIter, j)) {
                    return;
                }

                auto* shift = new GeneralEnemySub::Shift;
                if (!shiftIter.tryGetStringByKey(&shift->mConditionName, "Condition") ||
                    !shiftIter.tryGetStringByKey(&shift->mNerveName, "Nerve")) {
                    return;
                }

                shift->mCondition =
                    createElementByName(cConditionCreators, shift->mConditionName, &shiftIter);
                nerve->mShifts.pushBack(shift);
            }
        }

        mNerveMap.insert(name, nerve);
        if (nerveIter.isExistKey("Default")) {
            defaultNerveName = name;
        }
    }

    if (defaultNerveName == nullptr) {
        al::ByamlIter firstNerveIter;
        nerves.tryGetIterByIndex(&firstNerveIter, 0);
        firstNerveIter.tryGetStringByKey(&defaultNerveName, "Name");
    }

    mCurrentNerve = mNerveMap.find(defaultNerveName);

    al::ByamlIter reactions;
    if (root.tryGetIterByKey(&reactions, "CommonReaction") && reactions.getSize() > 0) {
        initReaction(reactions);
    }

    al::initNerve(this, &NrvGeneralEnemyCommon, 0);
}

/**
 * @brief Does nothing; the enemy does not attack.
 * @param pSelf Sensor of this enemy.
 * @param pOther Sensor of the other actor.
 */
void GeneralEnemy::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {}

/**
 * @brief Checks for trampling but does not react to any message yet.
 * @param pMsg Received message.
 * @param pSelf Sensor of this enemy.
 * @param pOther Sensor of the sender.
 * @return Always false.
 */
bool GeneralEnemy::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                              al::HitSensor* pOther) {
    al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf);
    return false;
}

/** @brief Runs the current nerve's action and behavior, then takes the first met shift. */
void GeneralEnemy::exeCommon() {
    if (mCurrentNerve == nullptr) {
        return;
    }

    if (al::isFirstStep(this)) {
        for (s32 i = 0; i < mCurrentNerve->mShifts.size(); i++) {
            mCurrentNerve->mShifts.unsafeAt(i)->mCondition->setup(this);
        }

        if (mCurrentNerve->mActionName != nullptr &&
            al::isExistAction(this, mCurrentNerve->mActionName)) {
            al::startAction(this, mCurrentNerve->mActionName);
        }
    }

    if (mCurrentNerve->mBehavior != nullptr) {
        if (al::isFirstStep(this)) {
            mCurrentNerve->mBehavior->setup(this);
        }

        mCurrentNerve->mBehavior->update(this);
    }

    for (s32 i = 0; i < mCurrentNerve->mShifts.size(); i++) {
        if (mCurrentNerve->mShifts.unsafeAt(i)->mCondition->check(this)) {
            GeneralEnemySub::GeneralEnemyNerve* nextNerve =
                mNerveMap.find(mCurrentNerve->mShifts.unsafeAt(i)->mNerveName);
            if (nextNerve == nullptr) {
                return;
            }

            if (mCurrentNerve->mBehavior != nullptr) {
                mCurrentNerve->mBehavior->teardown(this);
            }

            setNerve(nextNerve);
            return;
        }
    }
}

/**
 * @brief Switches to another data-driven nerve.
 * @param pNerve Nerve to switch to.
 */
void GeneralEnemy::setNerve(GeneralEnemySub::GeneralEnemyNerve* pNerve) {
    al::setNerve(this, &NrvGeneralEnemyCommon);
    mCurrentNerve = pNerve;
}

/**
 * @brief Reads which nerve each reaction shifts to.
 * @param rIter Array of reaction entries.
 */
void GeneralEnemy::initReaction(const al::ByamlIter& rIter) {
    for (s32 i = 0; i < rIter.getSize(); i++) {
        al::ByamlIter reactionIter;
        if (!rIter.tryGetIterByIndex(&reactionIter, i) || !reactionIter.isTypeHash()) {
            return;
        }

        const char* reactionName = nullptr;
        if (!reactionIter.tryGetStringByKey(&reactionName, "Reaction")) {
            return;
        }

        const char* nerveName = nullptr;
        if (!reactionIter.tryGetStringByKey(&nerveName, "Nerve")) {
            return;
        }

        for (s32 j = 0; j < mReactions.size(); j++) {
            if (al::isEqualString(mReactions.unsafeAt(j)->mName, reactionName)) {
                mReactions.unsafeAt(j)->mNerveName = nerveName;
                break;
            }
        }
    }
}

/** @brief Destroys the general enemy. */
GeneralEnemy::~GeneralEnemy() {}

namespace generalEnemyElements {
namespace behavior {

/**
 * @brief Snaps the enemy to the nearest rail position.
 * @param pActor Enemy that runs the behavior.
 */
void RailWait::setup(al::LiveActor* pActor) {
    if (al::isExistRail(pActor)) {
        al::setSyncRailToNearestPos(pActor);
    }
}

/**
 * @brief Does nothing; the enemy stays where it is.
 * @param pActor Enemy that runs the behavior.
 */
void RailWait::update(al::LiveActor* pActor) {}

/**
 * @brief Reads the rail speed.
 * @param rIter Data of the nerve that owns the behavior.
 */
NOINLINE void RailMove::init(const al::ByamlIter& rIter) {
    rIter.tryGetFloatByKey(&mSpeed, "Speed");
}

/**
 * @brief Moves the enemy along its rail and faces it forward.
 * @param pActor Enemy that runs the behavior.
 */
void RailMove::update(al::LiveActor* pActor) {
    if (al::isExistRail(pActor)) {
        al::moveRailLoop(pActor, mSpeed);
        al::syncRailTrans(pActor);
        al::turnToRailDirImmediately(pActor);
    }
}

/**
 * @brief Reads the start speed and the deceleration.
 * @param rIter Data of the nerve that owns the behavior.
 */
NOINLINE void RailBrake::init(const al::ByamlIter& rIter) {
    rIter.tryGetFloatByKey(&mStartSpeed, "StartSpeed");
    rIter.tryGetFloatByKey(&mBrakeSpeed, "BrakeSpeed");
    mSpeed = mStartSpeed;
}

/**
 * @brief Restarts at the start speed.
 * @param pActor Enemy that runs the behavior.
 */
void RailBrake::setup(al::LiveActor* pActor) {
    mSpeed = mStartSpeed;
}

/**
 * @brief Slows down and moves the enemy along its rail.
 * @param pActor Enemy that runs the behavior.
 */
void RailBrake::update(al::LiveActor* pActor) {
    mSpeed = sead::Mathf::max(0.0f, mSpeed - mBrakeSpeed);
    if (al::isExistRail(pActor)) {
        al::moveRailLoop(pActor, mSpeed);
        al::syncRailTrans(pActor);
        al::turnToRailDirImmediately(pActor);
    }
}

/**
 * @brief Kills the enemy.
 * @param pActor Enemy that runs the behavior.
 */
void Kill::update(al::LiveActor* pActor) {
    pActor->kill();
}

}  // namespace behavior

namespace condition {

/**
 * @brief Reads the number of frames to wait.
 * @param rIter Data of the shift that owns the condition.
 */
NOINLINE void Timer::init(const al::ByamlIter& rIter) {
    rIter.tryGetIntByKey(&mFrame, "Frame");
}

/**
 * @brief Checks whether the nerve has run long enough.
 * @param pActor Enemy that owns the condition.
 * @return Whether the nerve has run for at least the given number of frames.
 */
bool Timer::check(al::LiveActor* pActor) {
    return al::isGreaterEqualStep(pActor, mFrame);
}

/**
 * @brief Remembers the current position.
 * @param pActor Enemy that owns the condition.
 */
void IsVelocityZero::setup(al::LiveActor* pActor) {
    mPrevTrans.e = al::getTrans(pActor).e;
    mStopCount = 0;
}

/**
 * @brief Counts the frames the enemy has not moved.
 * @param pActor Enemy that owns the condition.
 * @return Whether the enemy has not moved for more than three frames.
 */
bool IsVelocityZero::check(al::LiveActor* pActor) {
    if ((mPrevTrans - al::getTrans(pActor)).squaredLength() < 0.01f) {
        mStopCount++;
    } else {
        mStopCount = 0;
    }

    mPrevTrans.e = al::getTrans(pActor).e;
    return mStopCount > 3;
}

/**
 * @brief Reads the probability in percent.
 * @param rIter Data of the shift that owns the condition.
 */
NOINLINE void Random::init(const al::ByamlIter& rIter) {
    rIter.tryGetFloatByKey(&mProbability, "Probability");
}

/**
 * @brief Rolls for the shift.
 * @param pActor Enemy that owns the condition.
 * @return Whether the roll succeeded.
 */
bool Random::check(al::LiveActor* pActor) {
    if (mProbability == 0.0f) {
        return false;
    }

    return al::getRandom(100.0f) <= mProbability;
}

/**
 * @brief Checks whether the current action has finished.
 * @param pActor Enemy that owns the condition.
 * @return Whether the action has finished.
 */
bool ActionEnd::check(al::LiveActor* pActor) {
    return al::isActionEnd(pActor);
}

}  // namespace condition
}  // namespace generalEnemyElements
