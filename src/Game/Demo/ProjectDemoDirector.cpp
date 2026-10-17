#include "Demo/ProjectDemoDirector.hpp"

#include "Demo/DemoActorGroup.hpp"
#include "Demo/DemoPlayerControllerHolder.hpp"
#include "Demo/DemoPlayerModelDirector.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/DemoActorGroupUtil.hpp"

/** @brief Gets the Camera demo identifier. @return The demo name. */
const char* ProjectDemoDirector::getDemoNameCamera() { return "カメラデモ"; }

/** @brief Gets the MovingCamera demo identifier. @return The demo name. */
const char* ProjectDemoDirector::getDemoNameMovingCamera() { return "MovingCamera"; }

/** @brief Gets the Intro demo identifier. @return The demo name. */
const char* ProjectDemoDirector::getDemoNameIntro() { return "IntroCamera"; }

/** @brief Gets the Player demo identifier. @return The demo name. */
const char* ProjectDemoDirector::getDemoNamePlayer() { return "プレイヤーデモ"; }

/** @brief Gets the Binding demo identifier. @return The demo name. */
const char* ProjectDemoDirector::getDemoNameBinding() { return "バインド中デモ"; }

/** @brief Gets the Cutscene demo identifier. @return The demo name. */
const char* ProjectDemoDirector::getDemoNameCutscene() { return "Cutscene"; }

/** @brief Gets the InGameCutscene demo identifier. @return The demo name. */
const char* ProjectDemoDirector::getDemoNameInGameCutscene() { return "InGameCutscene"; }

/** @brief Gets the PlayerCutscene demo identifier. @return The demo name. */
const char* ProjectDemoDirector::getDemoNamePlayerCutscene() { return "PlayerCutscene"; }

/** @brief Creates the project demo controllers and groups.
 * @param pPlayerHolder Players available to participate in demos.
 * @param maxActors Actor capacity, or a nonpositive value to use 32.
 */
ProjectDemoDirector::ProjectDemoDirector(al::PlayerHolder* pPlayerHolder, s32 maxActors)
    : al::DemoDirector(maxActors < 1 ? 32 : maxActors), mPlayerControllerHolder(nullptr),
      mPlayerModelDirector(nullptr), mActorGroups(nullptr) {
    mPlayerControllerHolder = new DemoPlayerControllerHolder(pPlayerHolder);
    mPlayerModelDirector = new DemoPlayerModelDirector;
    mActorGroups = new DemoActorGroup[32];
}

/** @brief Creates requested demo models. @param rInfo Actor initialization context. */
void ProjectDemoDirector::endInit(const al::ActorInitInfo& rInfo) {
    mPlayerModelDirector->endInit(rInfo);
}

/** @brief Requests a Camera demo. @param pActor Requesting actor.
 * @param pName Optional actor group name.
 * @return Whether the request was accepted. */
bool ProjectDemoDirector::requestStartDemoCamera(const al::LiveActor* pActor, const char* pName) {
    bool result = tryRequestStartDemo(pActor, getDemoNameCamera());
    if (pName && result) {
        for (int i = 0; i < 32; i++) {
            if (!mActorGroups[i].mName)
                break;
            if (al::isEqualString(mActorGroups[i].mName, pName)) {
                DemoActorGroupUtil::addDemoActorGroup(&mActorGroups[i], this);
                break;
            }
        }
    }
    return result;
}

/** @brief Ends a Camera demo request. @param pActor Requesting actor. */
void ProjectDemoDirector::requestEndDemoCamera(const al::LiveActor* pActor) {
    requestEndDemo(pActor, getDemoNameCamera());
}

/** @brief Requests a MovingCamera demo. @param pActor Requesting actor.
 * @param pName Optional actor group name.
 * @return Whether the request was accepted. */
bool ProjectDemoDirector::requestStartDemoMovingCamera(const al::LiveActor* pActor, const char* pName) {
    bool result = tryRequestStartDemo(pActor, getDemoNameMovingCamera());
    if (pName && result) {
        for (int i = 0; i < 32; i++) {
            if (!mActorGroups[i].mName)
                break;
            if (al::isEqualString(mActorGroups[i].mName, pName)) {
                DemoActorGroupUtil::addDemoActorGroup(&mActorGroups[i], this);
                break;
            }
        }
    }
    return result;
}

/** @brief Ends a MovingCamera demo request. @param pActor Requesting actor. */
void ProjectDemoDirector::requestEndDemoMovingCamera(const al::LiveActor* pActor) {
    requestEndDemo(pActor, getDemoNameMovingCamera());
}

/** @brief Requests a Intro demo. @param pActor Requesting actor.
 * @param pName Optional actor group name.
 * @return Whether the request was accepted. */
bool ProjectDemoDirector::requestStartDemoIntro(const al::LiveActor* pActor, const char* pName) {
    bool result = tryRequestStartDemo(pActor, getDemoNameIntro());
    if (pName && result) {
        for (int i = 0; i < 32; i++) {
            if (!mActorGroups[i].mName)
                break;
            if (al::isEqualString(mActorGroups[i].mName, pName)) {
                DemoActorGroupUtil::addDemoActorGroup(&mActorGroups[i], this);
                break;
            }
        }
    }
    return result;
}

/** @brief Ends a Intro demo request. @param pActor Requesting actor. */
void ProjectDemoDirector::requestEndDemoIntro(const al::LiveActor* pActor) {
    requestEndDemo(pActor, getDemoNameIntro());
}

/** @brief Requests a Player demo. @param pActor Requesting actor.
 * @return Whether the request was accepted. */
bool ProjectDemoDirector::requestStartDemoPlayer(const al::LiveActor* pActor) {
    return tryRequestStartDemo(pActor, getDemoNamePlayer());
}

/** @brief Ends a Player demo request. @param pActor Requesting actor. */
void ProjectDemoDirector::requestEndDemoPlayer(const al::LiveActor* pActor) {
    requestEndDemo(pActor, getDemoNamePlayer());
}

/** @brief Requests a Binding demo. @param pActor Requesting actor.
 * @return Whether the request was accepted. */
bool ProjectDemoDirector::requestStartDemoBinding(const al::LiveActor* pActor) {
    return tryRequestStartDemo(pActor, getDemoNameBinding());
}

/** @brief Ends a Binding demo request. @param pActor Requesting actor. */
void ProjectDemoDirector::requestEndDemoBinding(const al::LiveActor* pActor) {
    requestEndDemo(pActor, getDemoNameBinding());
}

/** @brief Requests a Cutscene demo. @param pActor Requesting actor.
 * @return Whether the request was accepted. */
bool ProjectDemoDirector::requestStartDemoCutscene(const al::LiveActor* pActor) {
    return tryRequestStartDemo(pActor, getDemoNameCutscene());
}

/** @brief Ends a Cutscene demo request. @param pActor Requesting actor. */
void ProjectDemoDirector::requestEndDemoCutscene(const al::LiveActor* pActor) {
    requestEndDemo(pActor, getDemoNameCutscene());
}

/** @brief Requests a InGameCutscene demo. @param pActor Requesting actor.
 * @return Whether the request was accepted. */
bool ProjectDemoDirector::requestStartDemoInGameCutscene(const al::LiveActor* pActor) {
    return tryRequestStartDemo(pActor, getDemoNameInGameCutscene());
}

/** @brief Ends a InGameCutscene demo request. @param pActor Requesting actor. */
void ProjectDemoDirector::requestEndDemoInGameCutscene(const al::LiveActor* pActor) {
    requestEndDemo(pActor, getDemoNameInGameCutscene());
}

/** @brief Requests a PlayerCutscene demo. @param pActor Requesting actor.
 * @return Whether the request was accepted. */
bool ProjectDemoDirector::requestStartDemoPlayerCutscene(const al::LiveActor* pActor) {
    return tryRequestStartDemo(pActor, getDemoNamePlayerCutscene());
}

/** @brief Ends a PlayerCutscene demo request. @param pActor Requesting actor. */
void ProjectDemoDirector::requestEndDemoPlayerCutscene(const al::LiveActor* pActor) {
    requestEndDemo(pActor, getDemoNamePlayerCutscene());
}

/** @brief Checks the active demo kind. @return Whether Camera is active. */
bool ProjectDemoDirector::isActiveDemoCamera() const {
    return al::isEqualString(getDemoNameCamera(), getActiveDemoName());
}

/** @brief Checks the active demo kind. @return Whether MovingCamera is active. */
bool ProjectDemoDirector::isActiveDemoMovingCamera() const {
    return al::isEqualString(getDemoNameMovingCamera(), getActiveDemoName());
}

/** @brief Checks the active demo kind. @return Whether Intro is active. */
bool ProjectDemoDirector::isActiveDemoIntro() const {
    return al::isEqualString(getDemoNameIntro(), getActiveDemoName());
}

/** @brief Checks the active demo kind. @return Whether Player is active. */
bool ProjectDemoDirector::isActiveDemoPlayer() const {
    return al::isEqualString(getDemoNamePlayer(), getActiveDemoName());
}

/** @brief Checks the active demo kind. @return Whether Binding is active. */
bool ProjectDemoDirector::isActiveDemoBinding() const {
    return al::isEqualString(getDemoNameBinding(), getActiveDemoName());
}

/** @brief Checks the active demo kind. @return Whether PlayerCutscene is active. */
bool ProjectDemoDirector::isActiveDemoPlayerCutscene() const {
    return al::isEqualString(getDemoNamePlayerCutscene(), getActiveDemoName());
}

/** @brief Checks the active demo kind. @return Whether Cutscene is active. */
bool ProjectDemoDirector::isActiveDemoCutscene() const {
    return al::isEqualString(getDemoNameCutscene(), getActiveDemoName());
}

/** @brief Checks the active demo kind. @return Whether InGameCutscene is active. */
bool ProjectDemoDirector::isActiveDemoInGameCutscene() const {
    return al::isEqualString(getDemoNameInGameCutscene(), getActiveDemoName());
}

/** @brief Gets a character's demo model. @param character Character index.
 * @return The requested model. */
DemoScenePlayerModel* ProjectDemoDirector::getDemoPlayerModel(int character) const {
    return mPlayerModelDirector->getDemoPlayerModel(character);
}

/** @brief Registers an actor with a named group. @param pActor Actor to register.
 * @param pName Group name, or null to skip registration. */
void ProjectDemoDirector::registerActorWithGroup(al::LiveActor* pActor, const char* pName) {
    if (!pName)
        return;
    int index = 0;
    for (int i = 0; i < 32; i++) {
        index = i;
        if (!mActorGroups[index].mName)
            break;
        if (al::isEqualString(mActorGroups[index].mName, pName)) {
            mActorGroups[index].addActor(pActor);
            return;
        }
    }
    mActorGroups[index].mName = pName;
    mActorGroups[index].addActor(pActor);
}

/** @brief Starts a demo and binds players when needed. @param pActor Demo owner.
 * @param pName Demo kind. @return Whether the demo can start. */
bool ProjectDemoDirector::startDemo(const al::LiveActor* pActor, const char* pName) {
    if (al::isEqualString(pName, getDemoNamePlayer()) &&
        !mPlayerControllerHolder->requestStartDemo())
        return false;
    mActiveDemoActor = pActor;
    return true;
}

/** @brief Ends the demo and releases its players. @param pActor Demo owner.
 * @param pName Demo kind. */
void ProjectDemoDirector::endDemo(const al::LiveActor* pActor, const char* pName) {
    mActiveDemoActor = nullptr;
    if (al::isEqualString(pName, getDemoNamePlayer()))
        mPlayerControllerHolder->requestEndDemo();
}
