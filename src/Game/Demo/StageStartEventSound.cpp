#include "Demo/StageStartEventSound.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
NERVE_DECL(StageStartEventSound, Play);
NERVE_DECL(StageStartEventSound, End);
NERVES_MAKE_NOSTRUCT(StageStartEventSound, Play, End)
}

/** @brief Creates a stage-entry sound event. @param pName Actor name. */
StageStartEventSound::StageStartEventSound(const char* pName) : StageStartEventBase(pName) {}

/**
 * @brief Initializes scene services and chooses the delay for the entry sound.
 * @param rInfo Actor initialization data containing the SoundName argument.
 */
void StageStartEventSound::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    al::initNerve(this, &NrvStageStartEventSoundPlay, 0);
    al::initExecutorUpdate(this, rInfo, "デモオブジェクト");
    al::initActorAudioKeeperWithout3D(this, rInfo, "StageStartEventSound", nullptr);
    al::initStageSwitch(this, rInfo);
    makeActorDead();
    al::tryGetStringArg(&mSoundName, rInfo, "SoundName");
    const char* pSoundName = mSoundName;
    int duration = 0;
    if (pSoundName) {
        if (al::isEqualString(pSoundName, "Dokan")) {
            duration = 70;
        } else if (al::isEqualString(pSoundName, "TeresaHouse")) {
            duration = 150;
        } else if (al::isEqualString(pSoundName, "Door")) {
            duration = 70;
        }
    }
    mDuration = duration;
}

/** @brief Activates the actor and begins the entry sound sequence. */
void StageStartEventSound::startDemo() {
    appear();
    al::setNerve(this, &NrvStageStartEventSoundPlay);
}

/** @brief Requests the sequence's ending state. */
void StageStartEventSound::endDemo() {
    al::setNerve(this, &NrvStageStartEventSoundEnd);
}

/** @brief Checks whether the ending state is active. @return Whether the demo has ended. */
bool StageStartEventSound::isEndDemo() const {
    return al::isNerve(this, &NrvStageStartEventSoundEnd);
}

/** @brief Gates the opening wipe on the ending state. @return Whether the wipe may open. */
bool StageStartEventSound::isEnableOpenStartWipe() const {
    return al::isNerve(this, &NrvStageStartEventSoundEnd);
}

/** @brief Starts the sound once and waits past its configured duration. */
void StageStartEventSound::exePlay() {
    if (al::isFirstStep(this)) {
        al::startSe(this, mSoundName);
    }
    if (!al::isLessEqualStep(this, mDuration)) {
        al::setNerve(this, &NrvStageStartEventSoundEnd);
    }
}

/** @brief Starts stage music on entry to the ending state and kills the event actor. */
void StageStartEventSound::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startBgm(this, "Stage", -1, 0, -1, -1);
    }
    kill();
}
