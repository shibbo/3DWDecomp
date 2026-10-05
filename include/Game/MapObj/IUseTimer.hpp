#pragma once

namespace al {
struct ActorInitInfo;
class LiveActor;
class AreaObjGroup;
}

namespace rc {
class IUseTimer {
public:
    virtual void forceCancel() = 0;
    virtual void reset() = 0;
    virtual bool canCancel() const { return true; }

    bool init(const al::ActorInitInfo& rInfo, const char* pLinkName);
    bool isPlayerNotInBounds(al::LiveActor* pActor, bool isCheckDokan);

    al::AreaObjGroup* mAreaGroup = nullptr; // 0x8
};
}
