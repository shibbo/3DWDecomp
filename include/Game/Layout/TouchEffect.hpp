#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class DrcTouchAssistInfo;

class TouchEffect : public al::LayoutActor {
public:
    TouchEffect(const al::LayoutInitInfo& rInfo, const DrcTouchAssistInfo* pTouchInfo);
    void setCharacter(int character);
    void setIcon();
    void setStampIcon(bool isStamp);
    void setInvalidChar();
    void appear() override;
    void movement() override;
    void setTransparent(bool isTransparent);
private:
    float mAlpha = 1.0f;
    int mCharacter = 0;
    bool mIsStamp = false;
    const DrcTouchAssistInfo* mTouchInfo;
};
