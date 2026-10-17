#include "CourseSelect/CourseSelectWall.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectNode.hpp"
#include "CourseSelect/CourseSelectNodeParts.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Resource/Resource.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"

namespace {
NERVE_DECL(CourseSelectWall, Wait);
NERVE_DECL(CourseSelectWall, AppearPrevEnd);
NERVE_DECL(CourseSelectWall, Appear);
NERVE_DECL(CourseSelectWall, AppearNextStart);
NERVE_DECL(CourseSelectWall, End);
NERVES_MAKE_NOSTRUCT(CourseSelectWall, Wait, AppearPrevEnd, Appear, AppearNextStart, End)
}

/** @brief Connect the wall's opening sequence to course-select actor control. */
class CourseSelectWallController : public ICourseSelectActorController {
public:
    explicit CourseSelectWallController(CourseSelectWall* pActor) : mpActor(pActor) {}
    al::LiveActor* getActor() override { return mpActor; }
    const CourseSelectActorInfo* getCourseSelectActorInfo() const override { return nullptr; }
    CourseSelectNode* getCourseSelectNode() const override { return nullptr; }
    void startPuppetDemo(CourseSelectPuppeteerGroup*) override {}
    void endPuppetDemo() override {}
    void startBind(CourseSelectPuppeteer*) override {}
    bool tryDecide(const CourseSelectDirector*) override { return false; }
    void startOpen() override { mpActor->startOpen(); }
    void startOpenImmediately() override { mpActor->startOpenImmediately(); }
    bool isEndOpen() const override { return mpActor->isEndOpen(); }
private:
    CourseSelectWall* mpActor;
};

/** @brief Create a course-map barrier and its opening controller. */
CourseSelectWall::CourseSelectWall(const char* pName)
    : al::LiveActor(pName), mpController(new CourseSelectWallController(this)) {}
CourseSelectWall::~CourseSelectWall() = default;

/** @brief Build the barrier's connected roads and optional closed-state collision. */
void CourseSelectWall::init(const al::ActorInitInfo& rInfo) {
    al::initActorChangeModel(this, rInfo);
    al::initNerve(this, &NrvCourseSelectWallWait, 0);
    al::hideModelIfShow(this);
    al::invalidateCollisionParts(this);
    al::PlacementInfo link;
    int links = al::calcLinkChildNum(rInfo, "NoDelete_NextObj");
    const al::PlacementInfo& placement = al::getPlacementInfo(rInfo);
    if (links > 0) al::getLinksInfo(&link, placement, "NoDelete_NextObj");
    else al::getLinksInfo(&link, placement, "NoDelete_NextNode");
    sead::Vector3f nextPos(0,0,0);
    al::getTrans(&nextPos, link);
    sead::Vector3f direction(0,0,0);
    CourseSelectNodeFunction::calcRoundOffDirH(&direction, al::getTrans(this), nextPos);
    sead::Vector3f offset(direction);
    float nextOffset = 350.0f;
    al::tryGetArg(&nextOffset, rInfo, "NextRoadOffset");
    offset *= nextOffset;
    offset.y = nextPos.y - al::getTrans(this).y;
    mpNode = new CourseSelectNode("コース選択分岐点[橋]", nullptr);
    mpNode->initBasic(rInfo);
    mpNode->setController(mpController);
    bool growToNext = true;
    al::tryGetArg(&growToNext, rInfo, "IsGrowToNext");
    if (growToNext) mpNode->initRoadList(rInfo, &offset);
    else mpNode->initNextNodePos(rInfo);
    float prevOffset = 315.0f;
    al::tryGetArg(&prevOffset, rInfo, "PrevRoadOffset");
    sead::Vector3f prevPos = -direction;
    prevPos *= prevOffset;
    prevPos += al::getTrans(this);
    mpPrevEnd = CourseSelectNodeFunction::createRoadEdge("コースセレクト道[終点]", rInfo, prevPos, direction, false);
    if (growToNext)
        mpNextStart = CourseSelectNodeFunction::createRoadEdge("コースセレクト道[始点]", rInfo, al::getTrans(this) + offset, -direction, true);
    if (al::getModelResource(this)->isExistFile("Close.kcl")) {
        mpClosedCollision = al::createCollisionObj(this, rInfo, "Close", al::getHitSensor(this, "CollisionBase"), nullptr, nullptr);
        mpClosedCollision->makeActorAppeared();
    }
    al::StringTmp<256> modelName;
    al::StringTmp<256> suffix;
    al::makeMapPartsModelName(&modelName, &suffix, rInfo);
    if (al::isEqualString(modelName.cstr(), "CourseSelectWallCastleA")) {
        int worldId = 0;
        al::tryGetArg(&worldId, rInfo, "WorldId");
        al::startMclAnimAndSetFrameAndStop(this, "Color", worldId);
    }
    makeActorDead();
}

/** @brief Match the preceding road endpoint to the connected node's height. */
void CourseSelectWall::initAfterPlacement() {
    auto* pDirector = CourseSelectDirector::getCourseSelectDirector(this);
    CourseSelectNode* pNode = mpNode;
    CourseSelectNodeParts* pPrevEnd = mpPrevEnd;
    al::setTransY(pPrevEnd, al::getTrans(pDirector->getNode(pNode->getFirstNextNodeId())).y);
}

/** @brief Begin opening after the preceding road grows into the barrier. */
void CourseSelectWall::startOpen() {
    if (isEndOpen()) return;
    appear();
    al::invalidateClipping(this);
    al::setNerve(this, &NrvCourseSelectWallAppearPrevEnd);
}
bool CourseSelectWall::isEndOpen() const {
    return (mpNextStart && mpNextStart->isEndGrow()) || al::isNerve(this, &NrvCourseSelectWallEnd);
}
/** @brief Immediately open the barrier and both adjoining road segments. */
void CourseSelectWall::startOpenImmediately() {
    appear();
    mpPrevEnd->startOpenImmediately();
    if (mpNextStart) mpNextStart->startOpenImmediately();
    al::showModelIfHide(this);
    al::setNerve(this, &NrvCourseSelectWallEnd);
}

/** @brief Activate the barrier, its node, collision, and adjoining road segments. */
void CourseSelectWall::appear() {
    al::LiveActor::appear();
    mpNode->appear();
    if (mpClosedCollision) mpClosedCollision->appear();
    if (mpPrevEnd) mpPrevEnd->appear();
    if (mpNextStart) mpNextStart->appear();
}
/** @brief Deactivate the barrier and all of its associated actors. */
void CourseSelectWall::kill() {
    al::LiveActor::kill();
    mpNode->kill();
    if (mpClosedCollision) mpClosedCollision->kill();
    if (mpPrevEnd) mpPrevEnd->kill();
    if (mpNextStart) mpNextStart->kill();
}
void CourseSelectWall::exeWait() {}
/** @brief Grow the preceding road and start the barrier animation when it finishes. */
void CourseSelectWall::exeAppearPrevEnd() {
    if (al::isFirstStep(this)) mpPrevEnd->startOpen();
    if (mpPrevEnd->isEndGrow()) al::setNerve(this, &NrvCourseSelectWallAppear);
}
/** @brief Reveal the barrier animation, then open the following road if present. */
void CourseSelectWall::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear");
    if (al::isStep(this, 1)) al::showModelIfHide(this);
    if (al::isActionEnd(this)) {
        if (mpNextStart) al::setNerve(this, &NrvCourseSelectWallAppearNextStart);
        else al::setNerve(this, &NrvCourseSelectWallEnd);
    }
}
/** @brief Grow the following road before completing the opening sequence. */
void CourseSelectWall::exeAppearNextStart() {
    if (al::isFirstStep(this)) mpNextStart->startOpen();
    if (mpNextStart->isEndGrow()) al::setNerve(this, &NrvCourseSelectWallEnd);
}
/** @brief Restore clipping and exchange closed-state collision for the open barrier. */
void CourseSelectWall::exeEnd() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::validateCollisionParts(this);
        if (mpClosedCollision) {
            al::invalidateCollisionParts(mpClosedCollision);
            mpClosedCollision->kill();
        }
    }
}
