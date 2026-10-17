#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

class SimpleAppearWaitEndParts : public al::LayoutActor {
public:
    SimpleAppearWaitEndParts(const al::LayoutInitInfo& rInfo, const char* pName,
                            const char* pPartsName, al::LayoutActor* pParent);

    void appear() override;
    void end();
    bool isWait() const;
    void exeAppear();
    void exeWait();
    void exeEnd();
};
