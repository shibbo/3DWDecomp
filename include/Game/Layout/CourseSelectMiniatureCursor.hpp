#pragma once

#include <math/seadVector.h>
#include "Library/Nerve/IUseNerve.hpp"
namespace al { class IUseCamera; class LiveActor; class LayoutInitInfo; class NetworkSystem; }
class CourseSelectDirector;
class ICourseSelectActorController;

/**
 * @brief Cursor shown over the selected course miniature on the world map.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectMiniatureCursor : public al::IUseNerve {
public:
    CourseSelectMiniatureCursor(const al::LayoutInitInfo& rInfo, CourseSelectDirector* pDirector,
                               const al::NetworkSystem* pNetwork);
    al::NerveKeeper* getNerveKeeper() const override { return mNerveKeeper; }
    void startDemo();
    void endDemo();
    void startPause();
    void endPause();
    bool isDecide() const;
    bool isEnableEnterCourse() const;
    bool isActiveCursor() const;
    bool setSelectedActorController(ICourseSelectActorController* pController, bool isDrc, bool isForce);
    void startDecide();
    void update();
    static bool calcMiniatureLayoutPos(sead::Vector2f* pOut, al::IUseCamera* pCamera,
                                       const al::LiveActor* pActor,
                                       const sead::Vector3f& rOffset, float scale);
    bool isAppearGreenStarLock() const;
private:
    al::NerveKeeper* mNerveKeeper;
    u8 mUnreconstructed[0x50];
};
static_assert(sizeof(CourseSelectMiniatureCursor) == 0x60);
