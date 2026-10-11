#pragma once

#include <basis/seadTypes.h>
#include <container/seadObjArray.h>
#include <container/seadPtrArray.h>
#include <nn/nfp/nfp_types.h>

namespace al {
class LiveActor;
class NfpDirector;
struct NfpInfo;
}  // namespace al

class PlayerAmiiboDirector;

/**
 * @brief Shared state of every player's PlayerAmiiboDirector: owns the access to the NFP
 * director, counts the pending scan requests and remembers the tags already used.
 */
class PlayerAmiiboDirectorWatcher {
public:
    typedef sead::PtrArray<PlayerAmiiboDirector> DirectorArray;
    typedef sead::ObjArray<nn::nfp::TagId> TagIdArray;

    static constexpr s32 cUsedTagNumMax = 200;

    explicit PlayerAmiiboDirectorWatcher(bool isDisable);

    bool tryRegisterDirector(PlayerAmiiboDirector* pDirector);
    void clear();
    void requestStop(bool isForce, const al::LiveActor* pPlayer);
    bool isOtherScanQueued(const PlayerAmiiboDirector* pDirector) const;
    void requestScan(const al::LiveActor* pPlayer);
    bool hadNfpError() const;
    al::NfpInfo* getNfpInfo(const al::LiveActor* pPlayer);
    bool isValidTag(const nn::nfp::TagId& rTagId);
    void update();

    /**
     * @brief Check whether a scan is pending on the NFP director.
     * @return True while at least one scan request is active.
     */
    bool isScanRequested() const { return mScanRequestCount > 0; }

    /**
     * @brief Check whether amiibo scanning is enabled at all.
     * @return True if amiibo may be scanned.
     */
    bool isEnable() const { return mIsEnable; }

private:
    al::NfpDirector* mNfpDirector;  // 0x00
    DirectorArray mDirectors;       // 0x08
    TagIdArray mUsedTags;           // 0x18
    s32 mScanRequestCount;          // 0x38
    bool mIsEnable;                 // 0x3c
};
