#include "CourseSelect/CourseSelectDokanHolder.hpp"
#include "CourseSelect/CourseSelectDokan.hpp"
#include "Library/Scene/SceneObjUtil.hpp"

/** @brief Allocate space for 32 source and 32 destination pipes. */
CourseSelectDokanHolder::CourseSelectDokanHolder() {
    mSources.allocBuffer(32, nullptr);
    mDestinations.allocBuffer(32, nullptr);
}
/** @brief Connect unpaired pipes with matching link IDs. @param rInfo Unused scene initialization data. */
void CourseSelectDokanHolder::initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) {
    int sourceCount = mSources.size();
    int destinationCount = mDestinations.size();
    for (int i = 0; i < sourceCount; ++i) {
        CourseSelectDokan* pSource = mSources.unsafeAt(i);
        for (int j = 0; j < destinationCount; ++j) {
            CourseSelectDokan* pDestination = mDestinations.at(j);
            if (pSource->getLinkId() == pDestination->getLinkId() && !pDestination->getDstDokan()) {
                pDestination->setDstDokan(pSource);
                pSource->setDstDokan(pDestination);
                break;
            }
        }
    }
}
/** @brief Register a source pipe. @param pDokan Pipe to register. */
void CourseSelectDokanHolder::registerDokanSrc(CourseSelectDokan* pDokan) { mSources.pushBack(pDokan); }
/** @brief Register a destination pipe. @param pDokan Pipe to register. */
void CourseSelectDokanHolder::registerDokanDst(CourseSelectDokan* pDokan) { mDestinations.pushBack(pDokan); }
/** @brief Get the scene-object name. @return Pipe-holder name. */
const char* CourseSelectDokanHolder::getSceneObjName() const { return "コース選択土管保持"; }
namespace CourseSelectDokanFuction {
/** @brief Create the scene holder and register a source pipe. @param pDokan Pipe to register. */
void registerDokanSrc(CourseSelectDokan* pDokan) {
    static_cast<CourseSelectDokanHolder*>(al::createSceneObj(pDokan, 26))->registerDokanSrc(pDokan);
}
/** @brief Create the scene holder and register a destination pipe. @param pDokan Pipe to register. */
void registerDokanDst(CourseSelectDokan* pDokan) {
    static_cast<CourseSelectDokanHolder*>(al::createSceneObj(pDokan, 26))->registerDokanDst(pDokan);
}
}
