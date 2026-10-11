#include "Boss/KoopaLastPartsModel.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Obj/PartsFunction.hpp"

/**
 * @brief Creates a host-synchronized part model.
 * @param pName Actor name.
 */
KoopaLastPartsModel::KoopaLastPartsModel(const char* pName)
    : al::LiveActor(pName), mMatrix(sead::Matrix34f::ident) {}

/**
 * @brief Initializes a part attached to an external matrix.
 * @param pHost Actor whose visibility is followed.
 * @param rInfo Actor initialization information.
 * @param pArchiveName Part model archive.
 * @param pMtx Attachment matrix.
 * @param pSuffix Archive suffix.
 */
void KoopaLastPartsModel::initPartsMtx(al::LiveActor* pHost, const al::ActorInitInfo& rInfo,
    const char* pArchiveName, const sead::Matrix34f* pMtx, const char* pSuffix) {
    mHost = pHost;
    mHostMtx = pMtx;
    mIsUseMatrix = true;
    al::initActorWithArchiveNameNoPlacementInfo(this, rInfo, sead::SafeString(pArchiveName), pSuffix);
    al::invalidateClipping(this);
    makeActorAppeared();
}

/** @brief Updates the attachment pose and clears the hidden state on appearance. */
void KoopaLastPartsModel::makeActorAppeared() {
    updatePose();
    al::LiveActor::makeActorAppeared();
    mIsHidden = false;
}

/** @brief Applies the fixed attachment transform or copies the host's pose. */
void KoopaLastPartsModel::updatePose() {
    if (mIsUseMatrix) {
        sead::Matrix34f matrix = *mHostMtx;
        al::normalize(&matrix);
        const sead::Matrix34f attachment(
            0.16622719168663025f, -0.09033581614494324f, 0.9819409251213074f, 21.736360549926758f,
            0.02954971045255661f, -0.9948914051055908f, -0.0965295135974884f, -44.295204162597656f,
            0.9856446385383606f, 0.045061901211738586f, -0.1627085953950882f, -2.3378236293792725f);
        matrix.setMul(matrix, attachment);
        al::updatePoseMtx(this, &matrix);
    } else {
        al::copyPose(this, mHost);
    }
}

/** @brief Follows the host's visibility and updates visible attachment poses. */
void KoopaLastPartsModel::control() {
    if (al::updateSyncHostVisible(&mIsHidden, this, mHost, false)) {
        al::showModelIfHide(this);
        updatePose();
    } else {
        al::hideModelIfShow(this);
    }
}

/** @brief Destroys the part model's base actor resources. */
KoopaLastPartsModel::~KoopaLastPartsModel() = default;
