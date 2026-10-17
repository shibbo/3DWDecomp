#pragma once
#include "Library/MapObj/FixMapParts.hpp"
namespace al { class CameraTicket; }
class InkPatch : public al::FixMapParts {
public:
    explicit InkPatch(const char*);
    ~InkPatch() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    void kill() override;
    bool canLinkYOffset() const override { return false; }
    void fullKill(bool);
    void triggerSpecialInkPatch();
    void disappear();
    void startCamera();
    void endSpecialInkPatch(bool);
    al::CameraTicket* getInkPatchCameraTicket();
    bool isSpecialInkPatchDone();
    bool isDone();
    void forceDisappear();
    void endCamera();
    void exeWait();
    void exeDisappear();
    void exeDone();
private:
    al::CameraTicket* mCamera;
    bool mIsSpecialUnlockInk = false;
    bool mIsSpecialInkUnlocker = false;
};
static_assert(sizeof(InkPatch) == 0x160);
