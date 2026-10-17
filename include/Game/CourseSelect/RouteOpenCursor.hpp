#pragma once
class CourseSelectDirector;
class CourseSelectNode;
class RouteOpenCursor {
public:
    explicit RouteOpenCursor(CourseSelectDirector* pDirector);
    bool initialize(CourseSelectNode* pStart, CourseSelectNode* pEnd);
    bool update();
    bool isEnd() const;
private:
    CourseSelectDirector* mpDirector;
    int mNodeCount = -1;
    int mIndex = 0;
    CourseSelectNode** mpNodes;
};
static_assert(sizeof(RouteOpenCursor) == 0x18);
