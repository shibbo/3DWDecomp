#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al { class LayoutInitInfo; }

/** @brief Shows camera availability and the player controlling the camera. */
class CameraIcon : public al::LayoutActor {
public:
    CameraIcon(const al::LayoutInitInfo& rInfo, bool isOwnerChangeEnabled);
    void setCameraOwnerID(int ownerId);
    void startAppear();
    void startEnd();
    void reset();
    void setCameraNoIcon();
    void exeAppear();
    void exeWait();
    void exeNoCamera();
    void exeEnd();
    void exeSwitchOwner();

private:
    bool mUnusedFlag = false;
    bool mIsOwnerChangeEnabled;
    bool mIsBrigade = false;
    int mCameraOwnerId = -1;
};

static_assert(sizeof(CameraIcon) == 0x128);

