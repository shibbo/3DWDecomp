#pragma once

#include "Library/Layout/LayoutActor.hpp"
#include "Layout/TextBoxTextInfo.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

class GreenStarKeeper;

/**
 * @brief Counter of the green stars collected in the current stage.
 */
class CounterGreenStarParts : public al::LayoutActor {
public:
    CounterGreenStarParts(const al::LayoutInitInfo& rInfo, const char* pName,
                          const char* pPaneName, al::LayoutActor* pParent,
                          const GreenStarKeeper* pGreenStarKeeper);

    void control() override;
    void exeWait();
    void setPaneAcquiredGreenStar();

private:
    al::LayoutActor* mParent;
    const GreenStarKeeper* mGreenStarKeeper;
    int mAcquiredCount = 0;
    fix::TextBoxTextInfo mTextInfo;
    fix::TextBoxTextInfo mBaseTextInfo;
};

static_assert(sizeof(CounterGreenStarParts) == 0x180);
