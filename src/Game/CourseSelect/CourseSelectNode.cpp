#include "CourseSelect/CourseSelectNode.hpp"

#include <attributes.h>
#include <math/seadMathCalcCommon.h>

#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectMiniature.hpp"
#include "CourseSelect/CourseSelectNodeParts.hpp"
#include "CourseSelect/CourseSelectRoad.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"

namespace {
NERVE_DECL(CourseSelectNode, Wait)
NERVE_DECL(CourseSelectNode, OpenControllerUser)
NERVE_DECL(CourseSelectNode, OpenRoad)
NERVE_DECL(CourseSelectNode, OpenRoadEnd)
NERVE_DECL(CourseSelectNode, OpenRoadStart)
NERVE_DECL(CourseSelectNode, OpenCurve)
NERVE_DECL(CourseSelectNode, NextOpenRoadEnd)
NERVES_MAKE_STRUCT(CourseSelectNode, Wait, OpenControllerUser, OpenRoad, OpenRoadEnd,
                     OpenRoadStart, OpenCurve, NextOpenRoadEnd)

/** @brief Maximum number of next nodes (and of nodes linking to a node). */
const s32 cLinkNumMax = 3;

/** @brief Distance the roads are shortened by at a curve or a road end. */
const f32 cRoadEdgeDistance = 75.0f;

/** @brief Distance the roads are moved forward by at a curve. */
const f32 cCurveOffset = 100.0f;

/**
 * @brief Checks whether two positions are at the same place of the map (same tile, close enough
 * in height).
 * @param rA First position.
 * @param rB Second position.
 * @return true if they are at the same place.
 */
inline bool isSameNodePos(const sead::Vector3f& rA, const sead::Vector3f& rB) {
    return al::isInRange(rA.x - rB.x, -10.0f, 10.0f) &&
           al::isInRange(rA.y - rB.y, -251.0f, 251.0f) &&
           al::isInRange(rA.z - rB.z, -10.0f, 10.0f);
}

s32 getNextNodeLinksInfo(al::PlacementInfo* pInfos, const al::ActorInitInfo& rInfo);

/**
 * @brief Gets the placement infos of the objects linked to an actor with a link name.
 * @param pInfos Output array of placement infos.
 * @param rInfo Actor init info.
 * @param pLinkName Name of the link.
 * @return The number of linked objects.
 */
inline s32 getLinksInfoList(al::PlacementInfo* pInfos, const al::ActorInitInfo& rInfo,
                            const char* pLinkName) {
    const al::PlacementInfo& placementInfo = al::getPlacementInfo(rInfo);
    al::PlacementInfo linksInfo;
    if (!al::tryGetPlacementInfoByKey(&linksInfo, placementInfo, "Links")) {
        return 0;
    }

    al::PlacementInfo linkInfo;
    if (!al::tryGetPlacementInfoByKey(&linkInfo, linksInfo, pLinkName)) {
        return 0;
    }

    s32 num = linkInfo.placementIter.getSize();
    s32 i = 0;
    for (; i < num; i++) {
        al::getLinksInfoByIndex(&pInfos[i], al::getPlacementInfo(rInfo), pLinkName, i);
    }

    return i;
}

}  // namespace

/**
 * @brief Constructs the node.
 * @param pName Actor name.
 * @param pMiniature Miniature the node is placed on, or nullptr for a plain branch point.
 */
CourseSelectNode::CourseSelectNode(const char* pName, const CourseSelectMiniature* pMiniature)
    : al::LiveActor(pName), mMiniature(pMiniature) {}

/**
 * @brief Initializes the node and the roads growing to its next nodes.
 * @param rInfo Actor init info.
 */
void CourseSelectNode::init(const al::ActorInitInfo& rInfo) {
    initBasic(rInfo);
    initRoadList(rInfo, nullptr);
}

/**
 * @brief Creates the point marker shown on the node.
 * @param rInfo Actor init info.
 */
inline void CourseSelectNode::createPointObj(const al::ActorInitInfo& rInfo) {
    auto* pointObj = new al::LiveActor("コースセレクト道[ポイント]");
    al::initActorWithArchiveName(pointObj, rInfo, "CourseSelectPoint", nullptr);
    al::setTrans(pointObj, al::getTrans(this));
    pointObj->makeActorDead();
    mPointObj = pointObj;
}

/**
 * @brief Initializes the actor, the link lists and the point marker, and registers the node to
 * the director.
 * @param rInfo Actor init info.
 */
void CourseSelectNode::initBasic(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTQSV(this);
    al::initActorSRT(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initNerve(this, &NrvCourseSelectNode.Wait, 0);
    al::initActorAudioKeeper(this, rInfo, "CourseSelectRoad", nullptr);
    al::initActorClipping(this, rInfo);
    al::invalidateClipping(this);
    mLinkedList.allocBuffer(cLinkNumMax, nullptr);
    mTargetNodeList.allocBuffer(cLinkNumMax, nullptr);
    al::tryGetArg(&mIsGrowToNext, rInfo, "IsGrowToNext");

    bool isUsePoint = false;
    if (al::tryGetArg(&isUsePoint, rInfo, "IsUsePoint") && isUsePoint) {
        createPointObj(rInfo);
    }

    CourseSelectDirector::getCourseSelectDirector(this)->registerNode(this);
}

/**
 * @brief Creates the roads growing to the next nodes (or to the linked dokan of a miniature),
 * and the road start of a miniature with a single road.
 * @param rInfo Actor init info.
 * @param pOffset Offset applied to the roads, or nullptr.
 */
void CourseSelectNode::initRoadList(const al::ActorInitInfo& rInfo,
                                    const sead::Vector3f* pOffset) {
    if (!mIsGrowToNext) {
        initNextNodePos(rInfo);
        return;
    }

    al::PlacementInfo infos[cLinkNumMax];
    mRoadNum = getNextNodeLinksInfo(infos, rInfo);
    if (mRoadNum == 0 && mMiniature != nullptr) {
        mRoadNum = getLinksInfoList(infos, rInfo, "NoDelete_CourseSelectDokan");
    }

    if (mRoadNum > 0) {
        if (mMiniature != nullptr) {
            sead::Vector3f trans = {0.0f, 0.0f, 0.0f};
            al::getTrans(&trans, infos[0]);
            al::setTransY(this, trans.y);
        }

        mRoads = new CourseSelectRoad*[mRoadNum];
        for (s32 i = 0; i < mRoadNum; i++) {
            mRoads[i] = nullptr;
        }

        for (s32 i = 0; i < mRoadNum; i++) {
            sead::Vector3f trans = {0.0f, 0.0f, 0.0f};
            al::getTrans(&trans, infos[i]);
            if (!al::isInRange(al::getTrans(this).y - trans.y, -251.0f, 251.0f)) {
                continue;
            }

            if (al::isInRange(al::getTrans(this).x - trans.x, -10.0f, 10.0f)) {
                trans.x = al::getTrans(this).x;
            }

            if (al::isInRange(al::getTrans(this).z - trans.z, -10.0f, 10.0f)) {
                trans.z = al::getTrans(this).z;
            }

            mRoads[i] = new CourseSelectRoad("コースセレクト道", this, trans);
            mRoads[i]->initRoad(rInfo, pOffset);

            f32 prevRoadOffset = 0.0f;
            if (al::tryGetArg(&prevRoadOffset, infos[i], "PrevRoadOffset")) {
                mRoads[i]->subDistance(prevRoadOffset);
            }
        }

        if (mMiniature != nullptr && !mMiniature->isGateKeeper() && mRoadNum == 1) {
            f32 startOffset = 400.0f;
            al::tryGetArg(&startOffset, rInfo, "RoadStartOffset");

            sead::Vector3f dir = sead::Vector3f::ez;
            CourseSelectNodeFunction::calcRoundOffDirH(&dir, al::getTrans(this),
                                                       mRoads[0]->getTargetTrans());
            sead::Vector3f trans = al::getTrans(this);
            trans += dir * startOffset;
            mRoadStart = CourseSelectNodeFunction::createRoadEdge(
                "コースセレクト始点[ミニチュア]", rInfo, trans, -dir, true);

            sead::Vector3f* roadTrans = al::getTransPtr(mRoads[0]);
            sead::Vector3f offset = {0.0f, 0.0f, startOffset};
            al::calcTransLocalOffset(roadTrans, mRoads[0], offset);
            mRoads[0]->subDistance(startOffset);
        }
    }

    makeActorDead();
}

/**
 * @brief Appears the opened roads, the reached point marker and the grown road parts.
 */
void CourseSelectNode::appear() {
    for (s32 i = 0; i < mRoadNum; i++) {
        if (mRoads[i]->isEndOpen()) {
            mRoads[i]->appear();
        }
    }

    if (mIsRoadReached && mPointObj != nullptr) {
        mPointObj->appear();
    }

    if (mRoadEnd != nullptr && mRoadEnd->isEndGrow()) {
        mRoadEnd->appear();
    }

    if (mRoadStart != nullptr && mRoadStart->isEndGrow()) {
        mRoadStart->appear();
    }

    if (mIsOpening) {
        al::LiveActor::appear();
    }
}

/**
 * @brief Kills the node, or the opened roads and road parts when the node is not opening.
 */
void CourseSelectNode::kill() {
    if (mIsOpening) {
        al::LiveActor::kill();
        return;
    }

    for (s32 i = 0; i < mRoadNum; i++) {
        if (mRoads[i]->isEndOpen()) {
            mRoads[i]->kill();
        }
    }

    if (mIsRoadReached && mPointObj != nullptr) {
        mPointObj->kill();
    }

    if (mRoadEnd != nullptr && mRoadEnd->isEndGrow()) {
        mRoadEnd->kill();
    }

    if (mRoadStart != nullptr && mRoadStart->isEndGrow()) {
        mRoadStart->kill();
    }
}

/**
 * @brief Stores the positions of the next nodes of a node that grows no road.
 * @param rInfo Actor init info.
 */
void CourseSelectNode::initNextNodePos(const al::ActorInitInfo& rInfo) {
    al::PlacementInfo infos[cLinkNumMax];
    s32 num = getNextNodeLinksInfo(infos, rInfo);
    mNextNodePosList.allocBuffer(num, nullptr);
    for (s32 i = 0; i < num; i++) {
        sead::Vector3f trans = {0.0f, 0.0f, 0.0f};
        al::getTrans(&trans, infos[i]);
        mNextNodePosList.pushBack(trans);
    }

    makeActorDead();
}

namespace {

/**
 * @brief Gets the placement infos of the next nodes, miniatures, walls and dokans linked to a
 * node.
 * @param pInfos Output array of placement infos.
 * @param rInfo Actor init info of the node.
 * @return The number of linked objects.
 */
NOINLINE s32 getNextNodeLinksInfo(al::PlacementInfo* pInfos, const al::ActorInitInfo& rInfo) {
    s32 num = getLinksInfoList(pInfos, rInfo, "NoDelete_NextNode");
    num += getLinksInfoList(&pInfos[num], rInfo, "NoDelete_NextMiniature");
    num += getLinksInfoList(&pInfos[num], rInfo, "NoDelete_NextWall");
    num += getLinksInfoList(&pInfos[num], rInfo, "NoDelete_NextDokan");
    return num;
}

}  // namespace

namespace CourseSelectNodeFunction {

/**
 * @brief Calculates the horizontal direction between two positions, rounded off to an axis.
 * @param pOut Output direction.
 * @param rFrom Start position.
 * @param rTo End position.
 */
void calcRoundOffDirH(sead::Vector3f* pOut, const sead::Vector3f& rFrom,
                      const sead::Vector3f& rTo) {
    pOut->setSub(rTo, rFrom);
    pOut->y = 0.0f;
    al::normalize(pOut);
    al::roundOffVec(pOut);
}

/**
 * @brief Creates a road start or road end part.
 * @param pName Actor name.
 * @param rInfo Actor init info.
 * @param rTrans Position of the part.
 * @param rDir Direction the part faces.
 * @param isReverse Whether the part grows in reverse.
 * @return The created part.
 */
CourseSelectNodeParts* createRoadEdge(const char* pName, const al::ActorInitInfo& rInfo,
                                      const sead::Vector3f& rTrans, const sead::Vector3f& rDir,
                                      bool isReverse) {
    auto* parts = new CourseSelectNodeParts(pName, isReverse ? "GrowReverse" : "Grow",
                                            isReverse ? "WaitReverse" : "Wait");
    parts->initPartsWithArchiveName(rInfo, "CourseSelectRoadEnd");
    al::setTrans(parts, rTrans);
    al::faceToDirection(parts, rDir);
    return parts;
}

}  // namespace CourseSelectNodeFunction

/**
 * @brief Creates the point marker, the road ends and the curve of the node once the nodes are
 * connected to each other.
 * @param rInfo Actor init info.
 */
void CourseSelectNode::initAfterConnect(const al::ActorInitInfo& rInfo) {
    if (!mIsGrowToNext) {
        return;
    }

    if (mMiniature == nullptr && getNextNodeNum() == 0 && mLinkedList.size() == 0) {
        return;
    }

    if (mPointObj == nullptr &&
        (mRoadNum + mLinkedList.size() > 2 || (mMiniature != nullptr && mMiniature->isGateKeeper()))) {
        createPointObj(rInfo);
    }

    s32 roadEndNum = 0;
    for (s32 i = 0; i < mTargetNodeList.size(); i++) {
        CourseSelectNode* target =
            CourseSelectDirector::getCourseSelectDirector(this)->getNodes()[*mTargetNodeList[i]];
        if (target->mMiniature == nullptr || target->mMiniature->isGateKeeper()) {
            continue;
        }

        if (roadEndNum++ < 1) {
            sead::Vector3f dir = {0.0f, 0.0f, 0.0f};
            CourseSelectNodeFunction::calcRoundOffDirH(&dir, al::getTrans(this),
                                                       al::getTrans(target));
            mRoadEnd = CourseSelectNodeFunction::createRoadEdge("コースセレクト道[終端]", rInfo,
                                                                al::getTrans(this), dir, false);
        }
    }

    if (roadEndNum > 0 || mMiniature != nullptr) {
        return;
    }

    if (mRoadNum == 1 && mLinkedList.size() == 1) {
        sead::Vector3f dirToLinked = {0.0f, 0.0f, 0.0f};
        sead::Vector3f dirToRoad = {0.0f, 0.0f, 0.0f};
        const sead::Vector3f& trans = al::getTrans(this);
        CourseSelectNode* linked =
            CourseSelectDirector::getCourseSelectDirector(this)->getNodes()[*mLinkedList.front()];
        CourseSelectNodeFunction::calcRoundOffDirH(&dirToLinked, trans, al::getTrans(linked));
        CourseSelectNodeFunction::calcRoundOffDirH(&dirToRoad, al::getTrans(this),
                                                   mRoads[0]->getTargetTrans());

        f32 angle = al::calcAngleOnPlaneDegree(dirToLinked, dirToRoad, sead::Vector3f::ey);
        if (!al::isInRange(sead::Mathf::abs(angle), 89.0f, 91.0f)) {
            return;
        }

        bool isLeft = al::calcAngleOnPlaneDegree(dirToLinked, dirToRoad, sead::Vector3f::ey) < 0.0f;
        mRoadEnd = new CourseSelectNodeParts("コースセレクト道[カーブ]",
                                             isLeft ? "GrowLeft" : "GrowRight",
                                             isLeft ? "WaitLeft" : "WaitRight");
        mRoadEnd->initPartsWithArchiveName(rInfo, "CourseSelectRoadCurve");
        al::setTrans(mRoadEnd, al::getTrans(this));

        const sead::Vector3f* side;
        f32 rotateDegree = angle;
        if (angle > 0.0f) {
            al::faceToDirection(mRoadEnd, dirToRoad);
            side = &dirToLinked;
        } else {
            al::faceToDirection(mRoadEnd, dirToLinked);
            side = &dirToRoad;
            rotateDegree = -angle;
        }

        *al::getTransPtr(mRoadEnd) += *side * cCurveOffset;
        al::rotateQuatYDirDegree(mRoadEnd, sead::Mathf::abs(rotateDegree));

        for (s32 i = 0; i < mRoadNum; i++) {
            sead::Vector3f roadTrans = {0.0f, 0.0f, 0.0f};
            sead::Vector3f offset = {0.0f, 0.0f, cCurveOffset};
            al::calcTransLocalOffset(&roadTrans, mRoads[i], offset);
            al::setTrans(mRoads[i], roadTrans);
            mRoads[i]->subDistance(cCurveOffset);
        }

        return;
    }

    if (mController != nullptr || mRoadNum != 0 || mLinkedList.size() != 1) {
        return;
    }

    CourseSelectNode* linked =
        CourseSelectDirector::getCourseSelectDirector(this)->getNodes()[*mLinkedList.front()];
    linked->setSubDistance(cRoadEdgeDistance, al::getTrans(this));

    sead::Vector3f dir = {0.0f, 0.0f, 0.0f};
    sead::Vector3f trans = al::getTrans(this);
    CourseSelectNodeFunction::calcRoundOffDirH(&dir, al::getTrans(linked), al::getTrans(this));
    trans -= dir * cRoadEdgeDistance;
    mRoadEnd = CourseSelectNodeFunction::createRoadEdge("コースセレクト道[終端]", rInfo, trans,
                                                        dir, false);
}

/**
 * @brief Gets the number of next nodes.
 * @return The number of stored next node positions, or the number of roads.
 */
s32 CourseSelectNode::getNextNodeNum() const {
    return mNextNodePosList.isBufferReady() ? mNextNodePosList.size() : mRoadNum;
}

/**
 * @brief Shortens the road growing to a next node.
 * @param distance Distance to shorten the road by.
 * @param rNextNodePos Position of the next node.
 */
void CourseSelectNode::setSubDistance(f32 distance, const sead::Vector3f& rNextNodePos) {
    if (mRoads == nullptr) {
        return;
    }

    for (s32 i = 0; i < getNextNodeNum(); i++) {
        if (isSameNodePos(rNextNodePos, *getNextNodePos(i))) {
            mRoads[i]->subDistance(distance);
            return;
        }
    }
}

/**
 * @brief Opens immediately the roads to the cleared miniatures.
 */
void CourseSelectNode::initAfterPlacement() {
    if (mLinkedList.size() > 0) {
        return;
    }

    for (s32 i = 0; i < mTargetNodeList.size(); i++) {
        CourseSelectNode* target =
            CourseSelectDirector::getCourseSelectDirector(this)->getNodes()[*mTargetNodeList[i]];
        if (target->mMiniature == nullptr || target->mMiniature->isGateKeeper()) {
            continue;
        }

        if (!target->isMiniatureCleared() ||
            GameDataFunction::isStageLastPlayAndFirstClear(this, target->getMiniatureCourseId())) {
            openRoadToNextNode(target, true);
        }
    }
}

/**
 * @brief Checks whether the course of the miniature the node is placed on is cleared.
 * @return true if cleared.
 */
bool CourseSelectNode::isMiniatureCleared() const {
    if (mMiniature == nullptr) {
        return false;
    }

    return CourseInfoFunction::isClear(this, mMiniature->getCourseId());
}

/**
 * @brief Gets the course of the miniature the node is placed on.
 * @return The course id, or -1 without a miniature.
 */
s32 CourseSelectNode::getMiniatureCourseId() const {
    if (mMiniature == nullptr) {
        return -1;
    }

    return mMiniature->getCourseId();
}

/**
 * @brief Checks whether the node has no miniature, or only the gate keeper.
 * @return true without a course miniature.
 */
inline bool CourseSelectNode::isMiniatureNone() const {
    return mMiniature == nullptr || mMiniature->isGateKeeper();
}

/**
 * @brief Starts opening the road to a next node.
 * @param pNextNode Next node.
 * @param isImmediately Whether the road opens immediately, without its demo.
 */
void CourseSelectNode::openRoadToNextNode(CourseSelectNode* pNextNode, bool isImmediately) {
    mIsOpening = true;
    appear();
    tryAppearPointObj();
    mNextOpenNode = pNextNode;

    if (mRoads != nullptr) {
        mOpenRoadIndex = -1;
        for (s32 i = 0; i < mRoadNum; i++) {
            if (isSameNodePos(*getNextNodePos(i), al::getTrans(pNextNode))) {
                mOpenRoadIndex = i;
                break;
            }
        }

        if (mOpenRoadIndex < 0) {
            mNextOpenNode->tryAppearPointObj();
            if (mNextOpenNode != nullptr && mNextOpenNode->mController != nullptr) {
                al::setNerve(this, &NrvCourseSelectNode.OpenControllerUser);
            } else {
                al::setNerve(this, &NrvCourseSelectNode.Wait);
            }

            return;
        }

        pNextNode->mIsRoadReached = true;
        if (pNextNode->isCurve()) {
            mRoads[mOpenRoadIndex]->subDistance(cRoadEdgeDistance);
        }

        if (isImmediately && pNextNode->isMiniatureNone()) {
            mRoads[mOpenRoadIndex]->startOpen(true);
        }
    }

    if (isImmediately) {
        if (mNextOpenNode != nullptr && mNextOpenNode->mController != nullptr) {
            mNextOpenNode->mController->startOpenImmediately();
        }

        if (mRoadEnd != nullptr && al::isDead(mRoadEnd)) {
            mRoadEnd->startOpenImmediately();
        }

        if (mRoadStart != nullptr && al::isDead(mRoadStart)) {
            mRoadStart->startOpenImmediately();
        }

        endOpenRoadSelf(true);
        return;
    }

    if (tryStartOpenCurve()) {
        return;
    }

    if (!pNextNode->isMiniatureNone() && mRoadEnd != nullptr && al::isDead(mRoadEnd)) {
        al::setNerve(this, &NrvCourseSelectNode.OpenRoadEnd);
        return;
    }

    if (mRoadStart != nullptr && al::isDead(mRoadStart)) {
        al::setNerve(this, &NrvCourseSelectNode.OpenRoadStart);
        return;
    }

    if (pNextNode->isMiniatureNone() && tryStartOpenNextRoad()) {
        return;
    }

    if (mNextOpenNode != nullptr && mNextOpenNode->mController != nullptr) {
        mNextOpenNode->tryAppearPointObj();
        al::setNerve(this, &NrvCourseSelectNode.OpenControllerUser);
        return;
    }

    endOpenRoadSelf(false);
}

/**
 * @brief Appears the point marker if it is not shown yet.
 * @return true if it appeared.
 */
bool CourseSelectNode::tryAppearPointObj() {
    if (mPointObj == nullptr || al::isAlive(mPointObj)) {
        return false;
    }

    mPointObj->appear();
    return true;
}

/**
 * @brief Waits: kills the node when it is not opening anymore.
 */
void CourseSelectNode::exeWait() {
    if (al::isFirstStep(this)) {
        kill();
        mIsOpening = false;
    }
}

/**
 * @brief Opens the road to the next node, then opens the next node.
 */
void CourseSelectNode::exeOpenRoad() {
    if (al::isFirstStep(this)) {
        mRoads[mOpenRoadIndex]->startOpen(false);
    }

    if (mRoads[mOpenRoadIndex]->isEndOpen()) {
        mNextOpenNode->tryAppearPointObj();
        if (mNextOpenNode->mController != nullptr) {
            al::setNerve(this, &NrvCourseSelectNode.OpenControllerUser);
            return;
        }

        al::startSe(this, "PointAppear");
        endOpenRoadSelf(false);
    }
}

/**
 * @brief Ends opening the road of this node, and opens the road end of the next node.
 * @param isImmediately Whether the road end opens immediately.
 */
void CourseSelectNode::endOpenRoadSelf(bool isImmediately) {
    if (mNextOpenNode != nullptr && mNextOpenNode->tryOpenRoadEnd(isImmediately)) {
        al::setNerve(this, &NrvCourseSelectNode.NextOpenRoadEnd);
        return;
    }

    al::setNerve(this, &NrvCourseSelectNode.Wait);
}

/**
 * @brief Opens the object placed on the next node.
 */
void CourseSelectNode::exeOpenControllerUser() {
    if (al::isFirstStep(this)) {
        mNextOpenNode->mController->startOpen();
    }

    if (mNextOpenNode->mController->isEndOpen()) {
        endOpenRoadSelf(false);
    }
}

/**
 * @brief Grows the curve, then opens the road.
 */
void CourseSelectNode::exeOpenCurve() {
    if (mRoadEnd->isEndGrow()) {
        al::setNerve(this, &NrvCourseSelectNode.OpenRoad);
    }
}

/**
 * @brief Grows the road end, then opens the next node.
 */
void CourseSelectNode::exeOpenRoadEnd() {
    if (al::isFirstStep(this)) {
        mRoadEnd->startOpen();
    }

    if (mRoadEnd->isEndGrow()) {
        if (mNextOpenNode == nullptr) {
            al::setNerve(this, &NrvCourseSelectNode.Wait);
            return;
        }

        if (mNextOpenNode->tryAppearPointObj()) {
            al::startSe(this, "PointAppear");
        }

        if (mNextOpenNode->mController != nullptr) {
            al::setNerve(this, &NrvCourseSelectNode.OpenControllerUser);
            return;
        }

        endOpenRoadSelf(false);
    }
}

/**
 * @brief Grows the road start, then opens the road.
 */
void CourseSelectNode::exeOpenRoadStart() {
    if (al::isFirstStep(this)) {
        mRoadStart->startOpen();
    }

    if (mRoadStart->isEndGrow()) {
        if (tryStartOpenNextRoad()) {
            return;
        }

        endOpenRoadSelf(false);
    }
}

/**
 * @brief Starts opening the road to the next node if it is waiting.
 * @return true if it started.
 */
bool CourseSelectNode::tryStartOpenNextRoad() {
    if (mRoads == nullptr || mOpenRoadIndex < 0 || mRoads[mOpenRoadIndex] == nullptr ||
        !mRoads[mOpenRoadIndex]->isWait()) {
        return false;
    }

    al::setNerve(this, &NrvCourseSelectNode.OpenRoad);
    return true;
}

/**
 * @brief Waits for the next node to end opening.
 */
void CourseSelectNode::exeNextOpenRoadEnd() {
    if (mNextOpenNode->isWait()) {
        al::setNerve(this, &NrvCourseSelectNode.Wait);
    }
}

/**
 * @brief Checks whether the node is waiting (or has opened its road).
 * @return true if waiting.
 */
bool CourseSelectNode::isWait() const {
    if (al::isNerve(this, &NrvCourseSelectNode.OpenRoad) &&
        mRoads[mOpenRoadIndex]->isEndOpen()) {
        return true;
    }

    return al::isNerve(this, &NrvCourseSelectNode.Wait);
}

/**
 * @brief Sets the index of the node in the director, if not set yet.
 * @param nodeId Node index.
 */
void CourseSelectNode::setNodeId(s32 nodeId) {
    if (mNodeId == -1) {
        mNodeId = nodeId;
    }
}

/**
 * @brief Gets the position of a next node.
 * @param index Index of the next node.
 * @return The position.
 */
const sead::Vector3f* CourseSelectNode::getNextNodePos(s32 index) const {
    if (mNextNodePosList.isBufferReady() && mNextNodePosList.size() > 0) {
        return mNextNodePosList[index];
    }

    return &mRoads[index]->getTargetTrans();
}

/**
 * @brief Checks whether the node has a curve.
 * @return true with a curve.
 */
bool CourseSelectNode::isCurve() const {
    if (mRoadEnd == nullptr) {
        return false;
    }

    return mRoadEnd->isCurve();
}

/**
 * @brief Starts growing the curve if it is not shown yet.
 * @return true if it started.
 */
bool CourseSelectNode::tryStartOpenCurve() {
    if (mRoadEnd == nullptr || !mRoadEnd->isCurve() || al::isAlive(mRoadEnd)) {
        return false;
    }

    mRoadEnd->startOpen();
    al::setNerve(this, &NrvCourseSelectNode.OpenCurve);
    return true;
}

/**
 * @brief Starts growing the road end of a node without road, if it is not shown yet.
 * @param isImmediately Whether the road end opens immediately.
 * @return true if it started.
 */
bool CourseSelectNode::tryOpenRoadEnd(bool isImmediately) {
    if (mRoadNum != 0) {
        return false;
    }

    if (mRoadEnd == nullptr || !al::isDead(mRoadEnd) || mRoadEnd->isCurve()) {
        return false;
    }

    mIsOpening = true;
    if (isImmediately) {
        mRoadEnd->startOpenImmediately();
        al::setNerve(this, &NrvCourseSelectNode.Wait);
    } else {
        mNextOpenNode = nullptr;
        al::setNerve(this, &NrvCourseSelectNode.OpenRoadEnd);
    }

    return true;
}

/**
 * @brief Checks whether the node is placed at a position.
 * @param rTrans Position.
 * @return true if at the position.
 */
bool CourseSelectNode::isEqual(const sead::Vector3f& rTrans) const {
    return isSameNodePos(al::getTrans(this), rTrans);
}

/**
 * @brief Checks whether the node is placed on the gate keeper.
 * @return true on the gate keeper.
 */
bool CourseSelectNode::isGateKeeperNode() const {
    if (mMiniature == nullptr) {
        return false;
    }

    return mMiniature->isGateKeeper();
}

/**
 * @brief Adds a node linking to this one.
 * @param nodeId Index of the node.
 */
void CourseSelectNode::addLinkedList(s32 nodeId) {
    if (mLinkedList.size() < cLinkNumMax) {
        mLinkedList.pushBack(nodeId);
    }
}

/**
 * @brief Adds a node this one links to.
 * @param nodeId Index of the node.
 */
void CourseSelectNode::addTargetNodeList(s32 nodeId) {
    mTargetNodeList.pushBack(nodeId);
}

/**
 * @brief Checks whether a node links to this one.
 * @param pNode Node.
 * @return true if it links to this one.
 */
bool CourseSelectNode::isLinkedByTarget(const CourseSelectNode* pNode) const {
    for (s32 i = 0; i < mLinkedList.size(); i++) {
        if (*mLinkedList[i] == pNode->mNodeId) {
            return true;
        }
    }

    return false;
}
