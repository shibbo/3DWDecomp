#pragma once
#include <container/seadPtrArray.h>
#include "Library/Scene/ISceneObj.hpp"
class CourseSelectDokan;
class CourseSelectDokanHolder : public al::ISceneObj {
public:
    CourseSelectDokanHolder();
    const char* getSceneObjName() const override;
    void initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) override;
    void registerDokanSrc(CourseSelectDokan* pDokan);
    void registerDokanDst(CourseSelectDokan* pDokan);
private:
    sead::PtrArray<CourseSelectDokan> mSources;
    sead::PtrArray<CourseSelectDokan> mDestinations;
};
static_assert(sizeof(CourseSelectDokanHolder) == 0x28);
namespace CourseSelectDokanFuction {
void registerDokanSrc(CourseSelectDokan* pDokan);
void registerDokanDst(CourseSelectDokan* pDokan);
}
