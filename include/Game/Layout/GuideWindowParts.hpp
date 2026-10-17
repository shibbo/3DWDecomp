#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }

class GuideWindowParts : public al::LayoutActor {
public:
    GuideWindowParts(const al::LayoutInitInfo& rInfo, const char* pName,
                     const char* pPartsName, al::LayoutActor* pParent);
    virtual void appear(const char* pActionName);
    void setText(const char16_t* pText);
    void end();
    bool isWait() const;
    void exeAppear();
    void exeWait();
    void exeEnd();
};
