#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include "prim/seadSafeString.h"
namespace al { class LayoutInitInfo; }
class GameDataHolder;
class StampList;
class ListStampDemoParts;
/** @brief Stamp collection parts with an acquisition animation. */
class ListStampParts : public al::LayoutActor {
public:
    ListStampParts(const al::LayoutInitInfo& rInfo, const char* pName,
                   const char* pPartsName, al::LayoutActor* pParent,
                   const GameDataHolder* pGameData);
    void appear() override;
    void startOff(const char* pName);
    void startOn(const char* pName);
    void startDecide();
    bool isDecideEnd() const;
    void setAcquiredStampNameByCourseId(int courseId);
    void setAcquiredStampNameByCharacterComplete(int characterId);
    void exeWait();
    void exeDecide();
private:
    StampList* mStampList = nullptr;
    const GameDataHolder* mGameData;
    ListStampDemoParts* mDemo = nullptr;
    sead::FixedSafeString<64> mAcquiredStampName;
    bool mHasNoAcquiredStamp = true;
};
static_assert(sizeof(ListStampParts) == 0x1a0);
