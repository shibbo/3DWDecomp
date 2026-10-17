#pragma once

namespace al {
class CameraInfo;
class LiveActor;
}

class KoopaChaseDemoInfo {
public:
    KoopaChaseDemoInfo(al::CameraInfo* pCameraInfo, al::LiveActor* pActor,
                      const char* pName, int value18, int value1C, int value20,
                      const char* pName28);

    al::CameraInfo* mCameraInfo;
    al::LiveActor* mActor;
    const char* mName;
    int mValue18;
    int mValue1C;
    int mValue20;
    const char* mName28;
    bool mFlag30;
};
