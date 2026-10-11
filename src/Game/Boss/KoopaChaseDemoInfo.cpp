#include "Boss/KoopaChaseDemoInfo.hpp"

/**
 * @brief Stores the camera, actor, and parameters for a chase demo.
 * @param pCameraInfo Camera information used by the demo.
 * @param pActor Actor associated with the demo.
 * @param pName Primary demo name.
 * @param value18 Demo parameter at offset 0x18; meaning not yet identified.
 * @param value1C Demo parameter at offset 0x1c; meaning not yet identified.
 * @param value20 Demo parameter at offset 0x20; meaning not yet identified.
 * @param pName28 Additional name at offset 0x28; meaning not yet identified.
 */
KoopaChaseDemoInfo::KoopaChaseDemoInfo(al::CameraInfo* pCameraInfo, al::LiveActor* pActor,
                                     const char* pName, int value18, int value1C,
                                     int value20, const char* pName28)
    : mCameraInfo(pCameraInfo), mActor(pActor), mName(pName), mValue18(value18),
      mValue1C(value1C), mValue20(value20), mName28(pName28), mFlag30(false) {
}
