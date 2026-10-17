#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al { class LayoutInitInfo; }
class GameDataHolder;
class ListStampParts;

/** @brief Presents stamps earned by clearing a course or completing a character. */
class ListStampResult : public al::LayoutActor {
public:
    ListStampResult(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameData);
    void exeAppear();
    void exeWait();
    void exeEnd();
    void startAppear(int courseId);
    void startAppearCharacterComplete(int characterId);
    void startEnd();
    bool isEnd() const;

private:
    int mCourseId = 0;
    int mCharacterId = -1;
    ListStampParts* mStampParts = nullptr;
};

static_assert(sizeof(ListStampResult) == 0x138);
