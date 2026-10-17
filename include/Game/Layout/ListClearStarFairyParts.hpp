#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class GameDataHolder;

class ListClearStarFairyParts : public al::LayoutActor {
public:
    ListClearStarFairyParts(const al::LayoutInitInfo& rInfo, const char* pName,
                           const char* pPartsName, al::LayoutActor* pParent,
                           const GameDataHolder* pGameData);
    void exeShow();
    void exeHide();
    void setHide();
    void setShow(int courseId, int worldLabelId);
private:
    al::LayoutActor* mParent;
    const GameDataHolder* mGameData;
};
