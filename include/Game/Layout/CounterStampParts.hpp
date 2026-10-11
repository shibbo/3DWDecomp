#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al { class LayoutInitInfo; }

class IllustItemKeeper;

class CounterStampParts : public al::LayoutActor {
public:
    CounterStampParts(const al::LayoutInitInfo& rInfo, const char* pName,
                      const char* pPartsName, al::LayoutActor* pParent,
                      const IllustItemKeeper* pKeeper);
    void exeHide();
    void exeGet();
    void exeWait();

private:
    al::LayoutActor* mParent;
    const IllustItemKeeper* mKeeper;
};
static_assert(sizeof(CounterStampParts) == 0x138);

