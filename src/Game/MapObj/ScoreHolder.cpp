#include "Library/LiveActor/LiveActor.hpp"
#include "Project/Base/StringUtil.hpp"
#include "MapObj/ScoreHolder.hpp"
#include "Layout/ScoreNumber.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "System/Data/StageDataHolder.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
ScoreHolder::ScoreHolder() { mTable = new al::ByamlIter(al::tryGetBymlFromObjectResource("ScoreTable", "ScoreTable")); }
void ScoreHolder::addScoreAndPopNumberFromIter(const al::LiveActor* actor, int users, const al::ByamlIter& iter, int index, const sead::Vector3f& offset, const sead::Vector3f* position, bool show, int delay) {
    if (al::isTypeIntByIndex(iter, index)) {
        int score = 0;
        iter.tryGetIntByIndex(&score, index);
        if (mStageData) {
            for (int i = 0; i < 4; ++i) if (users & (1 << i)) mStageData->addScore(score, i);
        }
        if (show && mStageData) {
            if (position) mNumbers[mNextNumber]->startScore(score, users, position, offset, delay);
            else mNumbers[mNextNumber]->startScore(score, users, offset, delay);
            advanceNumber();
        }
    } else if (al::isTypeStringByIndex(iter, index)) {
        const char* value = nullptr;
        iter.tryGetStringByIndex(&value, index);
        if (al::isEqualString(value, "1Up")) {
            GameDataHolderAccessor accessor(actor);
            GameDataFunction::addPlayerLife(accessor, 1);
            if (show) {
                if (position) popUpPlayerUp(actor, 1, position, offset, delay);
                else popUpPlayerUp(actor, 1, offset, delay);
            }
        }
    }
}
void ScoreHolder::popUpPlayerUp(const al::LiveActor* actor, int count, const sead::Vector3f* position, const sead::Vector3f& offset, int delay) {
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(actor))) return;
    mNumbers[mNextNumber]->startPlayerUp(count, position, offset, delay);
    advanceNumber();
}
void ScoreHolder::popUpPlayerUp(const al::LiveActor* actor, int count, const sead::Vector3f& position, int delay) {
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(actor))) return;
    mNumbers[mNextNumber]->startPlayerUp(count, position, delay);
    advanceNumber();
}
void ScoreHolder::addScoreAndPopNumber(const al::LiveActor* actor, al::HitSensor* sensor, const char* name, int level, const sead::Vector3f& offset, const sead::Vector3f* position) {
    if (!sensor) return;
    int users = rc::tryFindRelativeControlUserIdBitFlag(sensor);
    if (!users) return;
    al::ByamlIter entry;
    if (!mTable->tryGetIterByKey(&entry, name)) return;
    al::ByamlIter scores;
    if (!entry.tryGetIterByKey(&scores, "Score")) return;
    if (!scores.isTypeArray()) return;
    bool show = true;
    entry.tryGetBoolByKey(&show, "IsShow");
    level = sead::Mathi::clamp(level, 0, scores.getSize() - 1);
    if (al::isTypeArrayByIndex(scores, level)) {
        al::ByamlIter list;
        scores.tryGetIterByIndex(&list, level);
        int count = list.getSize();
        for (int i = 0; i < count; ++i) addScoreAndPopNumberFromIter(actor, users, list, i, offset, position, show, i * 20);
    } else addScoreAndPopNumberFromIter(actor, users, scores, level, offset, position, show, 0);
}
int ScoreHolder::getScoreMaxLevel(const char* name) const {
    al::ByamlIter entry;
    if (!mTable->tryGetIterByKey(&entry, name)) return 0;
    al::ByamlIter scores;
    if (!entry.tryGetIterByKey(&scores, "Score")) return 0;
    return scores.getSize();
}
void ScoreHolder::initAfterPlacementSceneObj(const al::ActorInitInfo& info) {
    mNumberCount = 32;
    mNextNumber = 0;
    mNumbers = new ScoreNumber*[mNumberCount];
    for (int i = 0; i < mNumberCount; ++i) mNumbers[i] = new ScoreNumber(al::getLayoutInitInfo(info));
}
void ScoreHolder::setStageDataHolder(StageDataHolder* data) { mStageData = data; }
void ScoreHolder::popUpScore(const al::LiveActor* actor, al::HitSensor* sensor, const char* name, int level, const sead::Vector3f& position) {
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(actor))) return;
    addScoreAndPopNumber(actor, sensor, name, level, position, nullptr);
}
void ScoreHolder::popUpScore(const al::LiveActor* actor, al::HitSensor* sensor, const char* name, int level, const sead::Vector3f* position, const sead::Vector3f& offset) {
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(actor))) return;
    addScoreAndPopNumber(actor, sensor, name, level, offset, position);
}
void ScoreHolder::initScore() {}
int ScoreHolder::getPlayerScore(int user) const { return mStageData ? mStageData->getScore(user) : 0; }
ScoreHolder* ScoreHolderUtil::initScoreHolder(const al::IUseSceneObjHolder* holder) { return al::getSceneObj<ScoreHolder>(holder, 20); }
const char* ScoreHolder::getSceneObjName() const { return "スコア管理"; }
