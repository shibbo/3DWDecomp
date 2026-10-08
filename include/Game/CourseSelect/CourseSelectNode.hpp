#pragma once

#include <container/seadObjArray.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

class CourseSelectMiniature;
class CourseSelectNodeParts;
class CourseSelectRoad;
class ICourseSelectActorController;

/**
 * @brief Branch point of the course-select map roads. It owns the roads growing to its next
 * nodes, the road parts placed on it (start, end or curve) and the point marker, and opens them
 * one after the other when the players unlock the way; the controller of the object placed on it
 * (miniature, dokan...) is entered from it.
 */
class CourseSelectNode : public al::LiveActor {
public:
    explicit CourseSelectNode(const char* pName, const CourseSelectMiniature* pMiniature = nullptr);

    void init(const al::ActorInitInfo& rInfo) override;
    void initBasic(const al::ActorInitInfo& rInfo);
    void initRoadList(const al::ActorInitInfo& rInfo, const sead::Vector3f* pOffset);
    void appear() override;
    void kill() override;
    void initNextNodePos(const al::ActorInitInfo& rInfo);
    void initAfterConnect(const al::ActorInitInfo& rInfo);
    s32 getNextNodeNum() const;
    void setSubDistance(f32 distance, const sead::Vector3f& rNextNodePos);
    void initAfterPlacement() override;
    bool isMiniatureCleared() const;
    s32 getMiniatureCourseId() const;
    void openRoadToNextNode(CourseSelectNode* pNextNode, bool isImmediately);
    bool tryAppearPointObj();
    void exeWait();
    void exeOpenRoad();
    void endOpenRoadSelf(bool isImmediately);
    void exeOpenControllerUser();
    void exeOpenCurve();
    void exeOpenRoadEnd();
    void exeOpenRoadStart();
    bool tryStartOpenNextRoad();
    void exeNextOpenRoadEnd();
    bool isWait() const;
    void setNodeId(s32 nodeId);
    const sead::Vector3f* getNextNodePos(s32 index) const;
    bool isCurve() const;
    bool tryStartOpenCurve();
    bool tryOpenRoadEnd(bool isImmediately);
    bool isEqual(const sead::Vector3f& rTrans) const;
    bool isGateKeeperNode() const;
    void addLinkedList(s32 nodeId);
    void addTargetNodeList(s32 nodeId);
    bool isLinkedByTarget(const CourseSelectNode* pNode) const;

    /** @brief Sets the controller of the object placed on the node. @param pController Controller. */
    void setController(ICourseSelectActorController* pController) { mController = pController; }
    /** @brief Gets the controller of the object placed on the node. @return The controller. */
    ICourseSelectActorController* getController() const { return mController; }
    /** @brief Gets the number of nodes linking to this one. @return The link count. */
    s32 getLinkNum() const { return mLinkedList.size(); }
    /** @brief Gets the director index of the first node linking to this one. @return The index. */
    s32 getFrontLinkNodeIndex() const { return *mLinkedList.front(); }

private:
    inline void createPointObj(const al::ActorInitInfo& rInfo);
    inline bool isMiniatureNone() const;

    s32 mNodeId = -1;
    ICourseSelectActorController* mController = nullptr;
    const CourseSelectMiniature* mMiniature;
    CourseSelectRoad** mRoads = nullptr;
    al::LiveActor* mPointObj = nullptr;
    CourseSelectNodeParts* mRoadEnd = nullptr;
    CourseSelectNodeParts* mRoadStart = nullptr;
    s32 mRoadNum = 0;
    s32 mOpenRoadIndex = -1;
    CourseSelectNode* mNextOpenNode = nullptr;
    sead::ObjArray<s32> mLinkedList;
    sead::ObjArray<s32> mTargetNodeList;
    sead::ObjArray<sead::Vector3f> mNextNodePosList;
    bool mIsRoadReached = false;
    bool mIsGrowToNext = true;
    bool mIsOpening = false;
};

static_assert(sizeof(CourseSelectNode) == 0x1f0);

namespace CourseSelectNodeFunction {
void calcRoundOffDirH(sead::Vector3f* pOut, const sead::Vector3f& rFrom, const sead::Vector3f& rTo);
CourseSelectNodeParts* createRoadEdge(const char* pName, const al::ActorInitInfo& rInfo,
                                      const sead::Vector3f& rTrans, const sead::Vector3f& rDir,
                                      bool isReverse);
}  // namespace CourseSelectNodeFunction
