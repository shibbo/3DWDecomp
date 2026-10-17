#include "Library/Audio/System/AudioRequestKeeperSyncedBgm.hpp"

#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"

namespace al {
/**
 * Creates the request slots.
 */
AudioRequestKeeperSyncedBgm::AudioRequestKeeperSyncedBgm() {
    mRequests = new sead::PtrArray<SyncedBgmRequest>();
    mRequests->allocBuffer(10, nullptr);

    for (s32 i = 0; i < 10; i++) {
        mRequests->pushBack(new SyncedBgmRequest);
    }
}

/**
 * Creates the audio keeper.
 * @param pDirector Audio director.
 */
void AudioRequestKeeperSyncedBgm::init(const AudioDirector* pDirector) {
    mAudioKeeper = createAudioKeeper(nullptr, pDirector);
}

/**
 * Executes the requests whose beat is triggered.
 */
void AudioRequestKeeperSyncedBgm::update() {
    if (!isEnableRhythmAnim(this, nullptr)) {
        return;
    }

    bool isTrigger[10] = {};

    for (s32 i = 0; i < mRequests->size(); i++) {
        SyncedBgmRequest* request = mRequests->unsafeAt(i);

        if (!request->isDone && isTriggerBeat(this, request->beat)) {
            isTrigger[i] = true;
        }
    }

    for (s32 i = 0; i < mRequests->size(); i++) {
        SyncedBgmRequest* request = mRequests->unsafeAt(i);

        if (request->isDone || !isTrigger[i]) {
            continue;
        }

        const BgmPlayingRequest& bgmRequest = request->request;

        switch (request->type) {
        case BgmPlayingType_Start:
            startBgm(this, bgmRequest);
            break;
        case BgmPlayingType_Stop:
            stopBgm(this, bgmRequest);
            break;
        case BgmPlayingType_Pause:
            pauseBgm(this, bgmRequest.name, bgmRequest.fadeInFrames);
            break;
        case BgmPlayingType_Resume:
            resumeBgm(this, bgmRequest.name, bgmRequest.fadeInFrames);
            break;
        default:
            break;
        }

        request->isDone = true;
    }
}

/**
 * Stores a request into a free slot.
 * @param type Request type.
 * @param rRequest BGM request.
 * @param beat Beat that triggers the request.
 */
void AudioRequestKeeperSyncedBgm::requestBgm(BgmPlayingType type, const BgmPlayingRequest& rRequest, s32 beat) {
    SyncedBgmRequest* freeRequest = nullptr;

    for (s32 i = 0; i < mRequests->size(); i++) {
        if (mRequests->unsafeAt(i)->isDone) {
            freeRequest = mRequests->unsafeAt(i);
            break;
        }
    }

    if (freeRequest == nullptr) {
        return;
    }

    freeRequest->type = type;
    freeRequest->request.name = rRequest.name;
    freeRequest->request.fadeInFrames = rRequest.fadeInFrames;
    freeRequest->request.startDelayFrames = rRequest.startDelayFrames;
    freeRequest->request.fadeOutFrames = rRequest.fadeOutFrames;
    freeRequest->request.isRestart = rRequest.isRestart;
    freeRequest->request._15 = rRequest._15;
    freeRequest->request._18 = rRequest._18;
    freeRequest->request._1c = rRequest._1c;
    freeRequest->beat = beat;
    freeRequest->isDone = false;
}

/**
 * Gets the audio keeper.
 * @return Audio keeper.
 */
AudioKeeper* AudioRequestKeeperSyncedBgm::getAudioKeeper() const {
    return mAudioKeeper;
}
}  // namespace al
