#include "Course/GuideBalloonBrigade.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"

namespace {
const char* const cBrigadeActions[4][2] = {
    {"Brigade1_Greyed", "Brigade1"},
    {"Brigade2_Greyed", "Brigade2"},
    {"Brigade3_Greyed", "Brigade3"},
    {"Brigade4_Greyed", "Brigade4"}
};
}

/**
 * @brief Creates the four Captain Toad brigade indicators.
 * @param pName Unused caller-provided name; this layout uses its fixed resource name.
 * @param rInfo Layout initialization context.
 * @param pTrans Followed world position.
 * @param rOffset Offset from the followed position.
 * @param pDemoDirector Demo director controlling visibility.
 */
GuideBalloonBrigade::GuideBalloonBrigade(const char* pName, const al::LayoutInitInfo& rInfo,
                                       const sead::Vector3f* pTrans,
                                       const sead::Vector3f& rOffset,
                                       al::DemoDirector* pDemoDirector)
    : GuideBalloon("GuideBalloonBrigade", "GuideBalloonBrigade", rInfo, pTrans, rOffset,
                   true, pDemoDirector) {
    mpBrigade1 = new al::LayoutActor("L_parts_00");
    al::initLayoutPartsActor(mpBrigade1, this, rInfo, "L_parts_00", nullptr);
    mpBrigade2 = new al::LayoutActor("L_parts_01");
    al::initLayoutPartsActor(mpBrigade2, this, rInfo, "L_parts_01", nullptr);
    mpBrigade3 = new al::LayoutActor("L_parts_02");
    al::initLayoutPartsActor(mpBrigade3, this, rInfo, "L_parts_02", nullptr);
    mpBrigade4 = new al::LayoutActor("L_parts_03");
    al::initLayoutPartsActor(mpBrigade4, this, rInfo, "L_parts_03", nullptr);
    kill();
}

/**
 * @brief Shows each brigade member as active or greyed out when the balloon is hidden.
 * @param flags Low four bits indicate the active brigade members.
 */
void GuideBalloonBrigade::startShowBrigade(sead::BitFlag<u8> flags) {
    if (isAlive()) {
        return;
    }
    al::LayoutActor::appear();
    setAppear();
    mpBrigade1->appear();
    const char* action1 = cBrigadeActions[0][flags.isOn(1)];
    al::startAction(mpBrigade1, action1);
    mpBrigade2->appear();
    const char* action2 = cBrigadeActions[1][flags.isOn(2)];
    al::startAction(mpBrigade2, action2);
    mpBrigade3->appear();
    const char* action3 = cBrigadeActions[2][flags.isOn(4)];
    al::startAction(mpBrigade3, action3);
    mpBrigade4->appear();
    const char* action4 = cBrigadeActions[3][flags.isOn(8)];
    al::startAction(mpBrigade4, action4);
}
