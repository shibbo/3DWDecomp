#pragma once

#include <container/seadPtrArray.h>
#include "Player/PlayerDef.hpp"

namespace al { class LiveActor; class SklAnimRetargettingInfo; }

class IUsePlayerRetargettingInfoCreator {
public:
    virtual s32 createRetargettingInfo(
        const char* pModelName, EPlayerFigure figure, const char* pAnimName) = 0;
};

class IUsePlayerRetargettingSelector {
public:
    virtual void applyDefaultRetargettingInfo(al::LiveActor* pActor, s32 index) = 0;
    virtual void applyCharaRetargettingInfo(al::LiveActor* pActor, s32 index) = 0;
    virtual void applyFigureRetargettingInfo(al::LiveActor* pActor, s32 index) = 0;
    virtual al::SklAnimRetargettingInfo* getDefaultRetargettingInfo(s32 index) = 0;
    virtual al::SklAnimRetargettingInfo* getCharaRetargettingInfo(s32 index) = 0;
    virtual al::SklAnimRetargettingInfo* getFigureRetargettingInfo(s32 index) = 0;
};

class PlayerRetargettingSelector : public IUsePlayerRetargettingInfoCreator,
                                  public IUsePlayerRetargettingSelector {
public:
    PlayerRetargettingSelector();
    s32 createRetargettingInfo(
        const char* pModelName, EPlayerFigure figure, const char* pAnimName) override;
    void applyDefaultRetargettingInfo(al::LiveActor* pActor, s32 index) override;
    void applyCharaRetargettingInfo(al::LiveActor* pActor, s32 index) override;
    void applyFigureRetargettingInfo(al::LiveActor* pActor, s32 index) override;
    al::SklAnimRetargettingInfo* getDefaultRetargettingInfo(s32 index) override;
    al::SklAnimRetargettingInfo* getCharaRetargettingInfo(s32 index) override;
    al::SklAnimRetargettingInfo* getFigureRetargettingInfo(s32 index) override;

private:
    sead::PtrArray<al::SklAnimRetargettingInfo> mDefaultInfos;
    sead::PtrArray<al::SklAnimRetargettingInfo> mCharaInfos;
    sead::PtrArray<al::SklAnimRetargettingInfo> mFigureInfos;
};
static_assert(sizeof(PlayerRetargettingSelector) == 0x40);
