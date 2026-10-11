#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { struct LayoutInitInfo; }
class GameDataHolder;
class ListStampResult : public al::LayoutActor {
public:
    ListStampResult(const al::LayoutInitInfo&, const GameDataHolder*);
    void startAppear(int);
    void startAppearCharacterComplete(int);
    bool isEnd() const;
private:
    u8 mUnreconstructed[0x17];
};
static_assert(sizeof(ListStampResult) == 0x138);
