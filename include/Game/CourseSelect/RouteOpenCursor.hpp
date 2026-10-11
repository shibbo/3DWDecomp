#pragma once

#include <basis/seadTypes.h>

#include "CourseSelect/CourseSelectNode.hpp"

class CourseSelectDirector;

/**
 * @brief Walks the nodes between two course-select nodes, opening the road from one node to the
 * next once the previous road finished opening.
 * @note The constructor, update() and isEnd() are inlined into CourseSelectStateClearDemo, which
 * indicates they were defined in the same translation unit; they are kept inline here.
 */
class RouteOpenCursor {
public:
    /**
     * @brief Creates an empty cursor.
     * @param pDirector Director the nodes are looked up in.
     */
    explicit RouteOpenCursor(CourseSelectDirector* pDirector) : mDirector(pDirector) {
        mNodes = new CourseSelectNode*[cNodeNumMax];
        for (s32 i = 0; i < cNodeNumMax; i++) {
            mNodes[i] = nullptr;
        }
    }

    bool initialize(CourseSelectNode* pStartNode, CourseSelectNode* pEndNode);

    /**
     * @brief Opens the road to the next node once the current node is waiting again.
     * @return true if the cursor went past the last node.
     */
    bool update() {
        if (!mNodes[mIndex]->isWait()) {
            return false;
        }

        mIndex++;
        if (mIndex < mNodeNum - 1) {
            mNodes[mIndex]->openRoadToNextNode(mNodes[mIndex + 1], false);
            return false;
        }

        return mIndex != mNodeNum - 1;
    }

    /**
     * @brief Checks whether the cursor reached the last node.
     * @return true if reached.
     */
    bool isEnd() const { return mNodeNum - 1 <= mIndex; }

private:
    /** Maximum number of nodes on one route. */
    static constexpr s32 cNodeNumMax = 32;

    CourseSelectDirector* mDirector;  // 0x0
    s32 mNodeNum = -1;  // 0x8
    s32 mIndex = 0;  // 0xc
    CourseSelectNode** mNodes;  // 0x10
};

static_assert(sizeof(RouteOpenCursor) == 0x18);
