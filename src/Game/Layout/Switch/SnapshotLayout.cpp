#include "Layout/Switch/SnapshotLayout.hpp"

#include <attributes.h>

#include <common/aglTextureData.h>
#include <eui/euiUtility.h>
#include <g3d/aglNW4FToNN.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>
#include <nn/gfx/gfx_ResTexture.h>
#include <nn/gfx/gfx_ResTextureData.h>
#include <nn/gfx/gfx_Texture.h>
#include <nn/ui2d/ui2d_TextureInfo.h>
#include <nn/util/util_ResDic.h>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutResource.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Resource/Resource.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Texture/TextureUtil.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/Stamp.hpp"
#include "MapObj/StampDirector.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Util/AttachObjectList.hpp"
#include "Util/InputUtil.hpp"

namespace {
NERVE_DECL(SnapshotLayout, Wait);
NERVE_DECL(SnapshotLayout, Appear);
NERVE_DECL(SnapshotLayout, End);
NERVES_MAKE_NOSTRUCT(SnapshotLayout, Wait, Appear, End)

/** Message labels of the filters, indexed by filter number. */
const char* const cFilterNames[] = {
    "NoFilter",    "Sepia",      "BlackAndWhite", "Sharpen", "Smear", "FishEyeLens",
    "Hypercolor",  "OilPainting", "PencilSketch", "Neon",    "Manga", "Tile",
};

/** Number of frames the button guide stays visible without input. */
constexpr s32 cInfoDisplayTime = 120;

/** Port used for raw touch-panel input. */
constexpr s32 cTouchPort = 5;

/** Width of the "Miiverse" stamps, which use a dedicated pane. */
constexpr s32 cMiiverseStampWidth = 400;

/** First stamp id that uses the "Miiverse" pane. */
constexpr s32 cMiiverseStampIdStart = 85;

/**
 * @brief Name dictionary of the textures in a texture file.
 * @param pTextureFile texture file
 * @return the dictionary
 */
inline const nn::util::ResDic* getTextureDic(const nn::gfx::ResTextureFile* pTextureFile) {
    return pTextureFile->ToData().textureContainerData.pTextureDic.Get();
}

/**
 * @brief Scales the size of the large preview pane to the size of a "Miiverse" stamp.
 * @param pSize Pane size to scale.
 * @param stampId Id of the previewed stamp.
 */
inline void scaleMiiverseStampSize(sead::Vector2f* pSize, s32 stampId) {
    switch (stampId) {
    case 85:
        break;
    case 86:
    case 89:
    case 90:
    case 91:
        pSize->x *= 2.0f;
        pSize->y *= 2.0f;
        break;
    default:
        pSize->x *= 1.25f;
        pSize->y *= 1.25f;
        break;
    }
}

/**
 * @brief Texture view of a texture resource.
 * @param rData texture resource
 * @return the texture view
 */
inline const nn::gfx::TextureView& getTextureView(const nn::gfx::ResTextureData& rData) {
    return *static_cast<const nn::gfx::TextureView*>(rData.pTextureView.Get());
}
}  // namespace

/**
 * @brief Creates the snapshot layout.
 * @param rInfo Layout initialization context.
 * @param pStampDirector Director owning the stamps shown in the preview.
 */
SnapshotLayout::SnapshotLayout(const al::LayoutInitInfo& rInfo, rc::StampDirector* pStampDirector)
    : al::LayoutActor("SnapshotLayout"), mStampDirector(pStampDirector) {
    al::initLayoutActor(this, rInfo, "RCS_SnapshotSceneLayout", nullptr);
    mTextureInfo = al::createTextureInfo();
    initNerve(&NrvSnapshotLayoutWait, 0);
    al::getPaneStringBufferLength(this, "TxtFilter");
}

/** @brief Shows the layout with the button guide and the starting stamp. */
void SnapshotLayout::appear() {
    al::LayoutActor::appear();
    mIsInfoDisplay = true;
    al::startAction(this, "InfoDisplay_On", "Info");
    mIsInfoHiddenByUser = false;
    mInfoDisplayTimer = cInfoDisplayTime;
    setStartingStampTexture();
    al::setNerve(this, &NrvSnapshotLayoutAppear);
    mIsWaitFirstInput = true;
    mLogoType = LogoType_None;
}

/** @brief Shows the small (100) preview pane. */
inline void SnapshotLayout::setPreviewTextureSmall() {
    al::setPaneTexture(this, "PicStamp100", mTextureInfo);
    al::startAction(this, "StampDummy100", "Stamp");
}

/** @brief Shows the large (200) preview pane. */
inline void SnapshotLayout::setPreviewTextureLarge() {
    al::setPaneTexture(this, "PicStamp200", mTextureInfo);
    al::startAction(this, "StampDummy200", "Stamp");
}

/** @brief Shows the texture of the stamp the snapshot mode starts with in the preview. */
void SnapshotLayout::setStartingStampTexture() {
    al::setPaneLocalRotate(this, "Stamp", sead::Vector3f::zero);
    al::ByamlIter list(
        al::findResourceYaml(mStampDirector->getStampResource(), "StampList", nullptr));
    list.getSize();
    s32 stampId = mStampDirector->getStartingStampId();
    al::ByamlIter entry;
    const char* itemName = nullptr;

    if (!list.tryGetIterByIndex(&entry, stampId)) {
        return;
    }

    {
        const char* itemType = nullptr;
        entry.tryGetStringByKey(&itemType, "IllustItemType");

        // Course stamps also carry a course id, which is read but not used here.
        if (al::isEqualString(itemType, "Course")) {
            s32 courseId = 0;
            entry.tryGetIntByKey(&courseId, "CourseId");
        }
    }

    entry.tryGetStringByKey(&itemName, "IllustItemName");

    if (itemName == nullptr) {
        return;
    }

    nn::gfx::ResTextureFile* textureFile =
        mStampDirector->getLayoutResource()->getArchiveList().begin()->mTextureFile;

    if (textureFile == nullptr) {
        return;
    }

    al::StringTmp<128> layoutTextureName("%s^", itemName);
    s32 count = getTextureDic(textureFile)->GetCount();
    s32 index = -1;

    for (s32 i = 0; i < count; i++) {
        const char* name =
            getTextureDic(textureFile)->ToData().entries[i + 1].pKey.Get()->GetData();

        if (al::isEqualSubString(name, layoutTextureName.cstr())) {
            index = i;
            break;
        }
    }

    if (index < 0) {
        nn::g3d::ResFile* resFile = mStampDirector->getStampResource()->getResFile();

        if (resFile == nullptr) {
            return;
        }

        textureFile = agl::g3d::ResFile::getResTextureFile(resFile);

        if (textureFile == nullptr) {
            return;
        }

        index = getTextureDic(textureFile)->FindIndex(itemName);

        if (index < 0) {
            return;
        }
    }

    nn::gfx::ResTexture* texture =
        textureFile->ToData().textureContainerData.pTexturePtrArray.Get()[index].Get();

    if (texture == nullptr) {
        return;
    }

    const nn::gfx::ResTextureData& textureData = texture->ToData();
    const nn::gfx::TextureView& view = getTextureView(textureData);
    eui::RegisterSlotForTexture(&mTextureInfo->mDescriptor, view, nullptr);
    agl::TextureData aglTexture;
    aglTexture.initializeFromNVNtexture(
        *static_cast<const NVNtexture*>(view.ToData()->pNvnTexture.ptr));
    u32 width = aglTexture.getWidth(0);
    f32 defaultWidth = rc::Stamp::getDefaultWidth();

    if (textureData.textureInfoData.width == cMiiverseStampWidth) {
        al::setPaneTexture(this, "PicStampMiiverse", mTextureInfo);
        al::startAction(this, "StampDummy400", "Stamp");
        sead::Vector2f size = sead::Vector2f::ones;
        al::getPaneLocalSize(&size, this, "PicStamp200");

        scaleMiiverseStampSize(&size, stampId);

        al::setPaneLocalSize(this, "PicStampMiiverse", size);
    } else if (width / defaultWidth != 1.0f) {
        setPreviewTextureSmall();
    } else {
        setPreviewTextureLarge();
    }
}

/** @brief Starts closing the layout. */
void SnapshotLayout::end() {
    al::setNerve(this, &NrvSnapshotLayoutEnd);
}

/**
 * @brief Shows the name of the selected filter.
 * @param filterIndex Index of the filter (wraps around).
 */
void SnapshotLayout::updateFilterText(s32 filterIndex) {
    al::setPaneSystemMessage(this, "TxtFilter", "SnapshotFilterNames",
                             cFilterNames[filterIndex % 12]);
}

/**
 * @brief Shows the texture of a stamp in the preview.
 * @param pStamp Stamp to preview, or nullptr to keep the current preview.
 */
void SnapshotLayout::updatePreviewImage(rc::Stamp* pStamp) {
    if (pStamp == nullptr) {
        return;
    }

    s32 stampId = pStamp->getStampId();
    const nn::gfx::TextureView& view =
        getTextureView(*pStamp->getTextureReplacer()->getResTexture());
    eui::RegisterSlotForTexture(&mTextureInfo->mDescriptor, view, nullptr);
    pStamp->getTextureScale();

    if (pStamp->getCurrentTextureScale().x != 1.0f) {
        setPreviewTextureSmall();
    } else if (stampId >= cMiiverseStampIdStart) {
        al::setPaneTexture(this, "PicStampMiiverse", mTextureInfo);
        al::startAction(this, "StampDummy400", "Stamp");
        sead::Vector2f size = sead::Vector2f::ones;
        al::getPaneLocalSize(&size, this, "PicStamp200");
        scaleMiiverseStampSize(&size, stampId);
        al::setPaneLocalSize(this, "PicStampMiiverse", size);
    } else {
        setPreviewTextureLarge();
    }
}

/**
 * @brief Rotates the stamp preview.
 * @param degree Rotation around the z axis in degrees.
 */
void SnapshotLayout::rotatePreviewImage(f32 degree) {
    al::setPaneLocalRotate(this, "Stamp", {0.0f, 0.0f, degree});
}

/** @brief Switches to the next logo and plays the change sound. */
void SnapshotLayout::incLogoType() {
    mLogoType = static_cast<LogoType>((mLogoType + 1) % 4);

    if (mLogoType != LogoType_None) {
        al::startSe(this, "PgLogoChange");
    } else {
        al::startSe(this, "PgLogoChangeNone");
    }
}

/** @brief Switches to the previous logo. */
void SnapshotLayout::decLogoType() {
    mLogoType = mLogoType == LogoType_None ? LogoType_Combo : static_cast<LogoType>(mLogoType - 1);
}

/**
 * @brief Shows the logo variant facing the given camera angle.
 * @param angle Camera angle in degrees.
 */
void SnapshotLayout::updateLogoAngle(f32 angle) {
    const char* direction = angle > -60.0f ? (angle < 60.0f ? "N" : "W") : "E";
    const char* logoName = "";

    switch (mLogoType) {
    case LogoType_None:
        al::startAction(this, "Logo_None", "Logo");
        return;
    case LogoType_3DWorld:
        logoName = "Logo_3DWorld";
        break;
    case LogoType_SingleMode:
        logoName = "Logo_SingleMode";
        break;
    case LogoType_Combo:
        logoName = "Logo_Combo";
        break;
    }

    al::startAction(this, al::StringTmp<32>("%s_%s", logoName, direction).cstr(), "Logo");
}

/**
 * @brief Shows or hides the button guide.
 * @param isUserToggle Whether the user toggled it (hiding it then keeps it hidden).
 */
void SnapshotLayout::toggleInfoDisplay(bool isUserToggle) {
    bool isDisplayed = mIsInfoDisplay;
    mIsInfoDisplay = isDisplayed ^ true;

    if (isUserToggle) {
        if (isDisplayed) {
            mIsInfoHiddenByUser = true;
        } else {
            mInfoDisplayTimer = cInfoDisplayTime;
            mIsInfoHiddenByUser = false;
        }
    }

    al::startAction(this, mIsInfoDisplay ? "InfoDisplay_On" : "InfoDisplay_Off", "Info");
}

/** @brief Sets the button guide texts for the current controller style. */
ALWAYS_INLINE void SnapshotLayout::updateGuideMessage() {
    if (al::isPadTypeHandheld(al::getMainControllerPort())) {
        al::setPaneSystemMessage(this, "TxtPlaceStamp", "RCS_SnapshotSceneLayout",
                                 "PlaceStamp_Handheld");
        al::setPaneSystemMessage(this, "TxtRotateStamp", "RCS_SnapshotSceneLayout",
                                 "RotateStamp_Handheld");
        al::setPaneSystemMessage(this, "TxtRotateStampRight", "RCS_SnapshotSceneLayout",
                                 "RotateStamp_Handheld_Right");
        al::setPaneSystemMessage(this, "TxtChangeStamp", "RCS_SnapshotSceneLayout",
                                 "ChangeStamp_Handheld");
        al::setPaneSystemMessage(this, "TxtChangeStampRight", "RCS_SnapshotSceneLayout",
                                 "ChangeStamp_Handheld_Right");
    } else {
        al::setPaneSystemMessage(this, "TxtPlaceStamp", "RCS_SnapshotSceneLayout",
                                 "PlaceStamp");
        al::setPaneSystemMessage(this, "TxtRotateStamp", "RCS_SnapshotSceneLayout",
                                 "RotateStamp");
        al::setPaneSystemMessage(this, "TxtRotateStampRight", "RCS_SnapshotSceneLayout",
                                 "RotateStamp_Right");
        al::setPaneSystemMessage(this, "TxtChangeStamp", "RCS_SnapshotSceneLayout",
                                 "ChangeStamp");
        al::setPaneSystemMessage(this, "TxtChangeStampRight", "RCS_SnapshotSceneLayout",
                                 "ChangeStamp_Right");
    }
}

/** @brief Sets up the button guide and plays the appear animation. */
void SnapshotLayout::exeAppear() {
    if (al::isFirstStep(this)) {
        updateGuideMessage();
        al::startAction(this, "Appear", nullptr);
    }

    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvSnapshotLayoutWait);
    }
}

/** @brief Handles the logo and button guide input while the snapshot mode is active. */
void SnapshotLayout::exeWait() {
    if (rc::isControllerAssignmentChanged()) {
        updateGuideMessage();
    }

    bool isHold = al::isPadHoldTouch(cTouchPort) || al::isPadHoldAny(al::getMainControllerPort());

    if (mIsWaitFirstInput && isHold) {
        mIsWaitFirstInput = false;
    }

    if (al::isPadTriggerUp(al::getMainControllerPort())) {
        incLogoType();
    }

    if (al::isPadTriggerY(al::getMainControllerPort())) {
        toggleInfoDisplay(true);
    } else if (mIsInfoDisplay) {
        s32 timer = mInfoDisplayTimer - 1;

        if (timer < 0) {
            toggleInfoDisplay(false);
        } else if (isHold || mIsWaitFirstInput) {
            mInfoDisplayTimer = cInfoDisplayTime;
        } else {
            mInfoDisplayTimer = timer;
        }
    } else if (!mIsInfoHiddenByUser && isHold) {
        toggleInfoDisplay(true);
    }
}

/** @brief Plays the end animation, then kills the layout. */
void SnapshotLayout::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
    }

    if (al::isActionEnd(this, nullptr)) {
        kill();
    }
}

namespace rc {
/**
 * @brief Creates the objects linked as "AttachObject" and remembers their offsets.
 * @param rInfo Actor initialization context of the owner.
 * @param isKill Whether the created objects start dead.
 */
void AttachObjectList::init(const al::ActorInitInfo& rInfo, bool isKill) {
    ProjectActorFactory factory;
    s32 num = al::calcLinkChildNum(rInfo, "AttachObject");
    mObjects.allocBuffer(num, nullptr);
    sead::Vector3f baseTrans = {0.0f, 0.0f, 0.0f};
    al::tryGetTrans(&baseTrans, rInfo);

    for (s32 i = 0; i < num; i++) {
        al::PlacementInfo placement;
        al::ActorInitInfo childInfo;
        al::getLinksInfoByIndex(&placement, rInfo, "AttachObject", i);
        childInfo.initViewIdSelf(&placement, rInfo);

        if (SingleModeDataFunction::isActorInPlessieChaseDisabled(childInfo)) {
            continue;
        }

        al::LiveActor* actor = al::createLinksActorFromFactory(factory, rInfo, "AttachObject", i);

        if (actor == nullptr) {
            continue;
        }

        Object* object = mObjects.emplaceBack();
        object->actor = actor;

        if (actor->getPoseKeeper() != nullptr) {
            object->offset = al::getTrans(actor);
        } else {
            al::getTrans(&object->offset, placement);
        }

        object->offset -= baseTrans;

        if (isKill) {
            actor->makeActorDead();
        }
    }
}

/**
 * @brief Moves all objects to their offsets from a position.
 * @param rPosition Base position.
 */
void AttachObjectList::syncObjectsToPosition(const sead::Vector3f& rPosition) {
    for (auto it = mObjects.begin(); it != mObjects.end(); ++it) {
        it->actor->updateLinkedTrans(rPosition + it->offset);
    }
}

/**
 * @brief Moves all objects to their rotated offsets from a position.
 * @param rPosition Base position.
 * @param rRotate Rotation applied to the offsets.
 */
void AttachObjectList::syncObjectsToPositionWithRotate(const sead::Vector3f& rPosition,
                                                       const sead::Quatf& rRotate) {
    for (auto it = mObjects.begin(); it != mObjects.end(); ++it) {
        sead::Vector3f offset = it->offset;
        offset.rotate(rRotate);
        it->actor->updateLinkedTrans(rPosition + offset);
    }
}
}  // namespace rc
