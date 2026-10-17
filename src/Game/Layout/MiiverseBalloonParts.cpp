#include "Layout/MiiverseBalloonParts.hpp"
#include "Layout/ButtonTouch.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "System/Data/StampType.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataFunction.hpp"
#include "System/CourseInfoHolder.hpp"
/** @brief Creates the balloon, touch handler, and local audio keeper. */
MiiverseBalloonParts::MiiverseBalloonParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName, al::LayoutActor* pParent, float scale)
    : al::LayoutActor(pName), mParent(pParent), mScale(scale) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    mTouch = new ButtonTouch("Miiverseバルーン", this, nullptr);
    al::initLayoutPartsAudioKeeper(this, rInfo, "MiiverseBalloonParts");
}
/** @brief Forwards the balloon touch control operation. */
void MiiverseBalloonParts::control() { mTouch->update(); }
/** @brief Forwards the balloon touch reset operation. */
void MiiverseBalloonParts::reset() { mTouch->reset(); }
/** @brief Forwards the balloon touch isWait operation. */
bool MiiverseBalloonParts::isWait() const { return mTouch->isWait(); }
/** @brief Forwards the balloon touch isDecide operation. */
bool MiiverseBalloonParts::isDecide() const { return mTouch->isDecide(); }
/** @brief Forwards the balloon touch isDecideEnd operation. */
bool MiiverseBalloonParts::isDecideEnd() const { return mTouch->isDecideEnd(); }
/** @brief Forwards the balloon touch setTouchEnable operation. */
void MiiverseBalloonParts::setTouchEnable() { mTouch->setTouchEnable(); }
/** @brief Forwards the balloon touch setTouchDisable operation. */
void MiiverseBalloonParts::setTouchDisable() { mTouch->setTouchDisable(); }
/** @brief Stamp database with accessors for its entry properties. */
class StampList {
public:
    al::ByamlIter* mIter;
    int getType(int index) const {
        const al::ByamlIter* pIter = mIter;
        al::ByamlIter entry;
        if (!pIter->tryGetIterByIndex(&entry, index)) return 0;
        const char* pType;
        entry.tryGetStringByKey(&pType, "IllustItemType");
        return StampType::calcStampTypeID(pType);
    }
    int getCourseId(int index) const {
        const al::ByamlIter* pIter = mIter;
        al::ByamlIter entry;
        if (!pIter->tryGetIterByIndex(&entry, index)) return 0;
        int courseId = 0;
        entry.tryGetIntByKey(&courseId, "CourseId");
        return courseId;
    }
    const char* getName(int index) const {
        const al::ByamlIter* pIter = mIter;
        al::ByamlIter entry;
        if (!pIter->tryGetIterByIndex(&entry, index)) return nullptr;
        const char* pName = nullptr;
        entry.tryGetStringByKey(&pName, "IllustItemName");
        return pName;
    }
};
namespace rc {
/** @brief Loads the shared stamp database. */
StampList* createStampList() {
    StampList* pList = new StampList;
    auto* pResource = al::findOrCreateResource("SystemData/StampData", nullptr);
    const u8* pData = al::findResourceYaml(pResource, "StampList", nullptr);
    pList->mIter = new al::ByamlIter(pData);
    return pList;
}
/** @brief Returns the number of database entries. */
int getStampSize(StampList* pList) { return pList->mIter->getSize(); }
/** @brief Checks the save condition associated with a stamp. */
bool isAcquiredStampByIndex(StampList* pList, int index, const GameDataHolder* pGameData) {
    GameDataHolderAccessor data(const_cast<GameDataHolder*>(pGameData));
    switch (pList->getType(index)) {
    case 0: return CourseInfoFunction::isAcquireIllustItem(data, pList->getCourseId(index), false, -1);
    case 1: return true;
    case 2: return GameDataFunction::isAcquireCharacterCompleteIllustItem(data, 0, false, -1);
    case 3: return GameDataFunction::isAcquireCharacterCompleteIllustItem(data, 1, false, -1);
    case 4: return GameDataFunction::isAcquireCharacterCompleteIllustItem(data, 2, false, -1);
    case 5: return GameDataFunction::isAcquireCharacterCompleteIllustItem(data, 3, false, -1);
    case 6: return GameDataFunction::isAcquireCharacterCompleteIllustItem(data, 4, false, -1);
    default: return false;
    }
}
/** @brief Looks up a stamp name by database index. */
const char* tryGetStampNameByIndex(StampList* pList, int index, const GameDataHolder* pGameData) {
    return pList->getName(index);
}
/** @brief Finds the stamp associated with a course. */
const char* tryGetStampNameByCourseId(StampList* pList, int courseId) {
    int count = pList->mIter->getSize();
    for (int i = 0; i < count; ++i) {
        if (pList->getCourseId(i) == courseId) return pList->getName(i);
    }
    return nullptr;
}
/** @brief Finds the completion stamp for a playable character. */
const char* tryGetCharacterCompleteStampName(StampList* pList, int character) {
    int count = pList->mIter->getSize();
    for (int i = 0; i < count; ++i) {
        int type = pList->getType(i);
        if ((character == 0 && type == 2) || (character == 1 && type == 3) ||
            (character == 2 && type == 4) || (character == 3 && type == 5) ||
            (character == 4 && type == 6)) return pList->getName(i);
    }
    return nullptr;
}
}
