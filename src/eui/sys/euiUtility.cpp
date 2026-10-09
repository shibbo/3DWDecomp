#include <eui/euiUtility.h>
#include <eui/euiCapturePane.h>
#include <eui/euiDynamicCapturePane.h>
#include <eui/euiDynamicCaptureUsePictureEx.h>
#include <eui/euiDynamicCaptureUseWindowEx.h>
#include <eui/euiAlignPane.h>
#include <eui/euiBoundingEx.h>
#include <eui/euiMassDrawPane.h>
#include <eui/euiMultiFilterPictureEx.h>
#include <eui/euiMultiFilterWindowEx.h>
#include <eui/euiScalableFontMgr.h>
#include <eui/euiScalableFontTextBoxEx.h>
#include <eui/euiSharcArchive.h>
#include <eui/euiScissorPane.h>
#include <eui/euiLayoutEx.h>
#include <eui/euiMessageString.h>
#include <eui/euiPartsEx.h>
#include <eui/euiPictureEx.h>
#include <eui/euiRootPane.h>
#include <eui/euiScreen.h>
#include <eui/euiScreenMgr.h>
#include <eui/euiTextBoxEx.h>
#include <eui/euiWindowEx.h>
#include <nn/ui2d/ui2d_ControlSrc.h>
#include <nn/ui2d/ui2d_DrawInfo.h>
#include <nn/ui2d/ui2d_DynamicCast.h>
#include <nn/ui2d/ui2d_Group.h>
#include <nn/ui2d/ui2d_Material.h>
#include <nn/ui2d/ui2d_TexMap.h>
#include <nn/ui2d/ui2d_TextureContainer.h>
#include <nn/ui2d/ui2d_TextureInfo.h>
#include <nn/ui2d/ui2d_Util.h>
#include <nn/gfx/gfx_ResTexture.h>
#include <nn/font/font_TextWriterBase.h>
#include <nn/util.h>
#include <cmath>
#include <cstring>

#include <common/aglTextureData.h>
#include <driver/aglNVNMgr.h>
#include <g3d/aglTextureDataInitializerG3D.h>
#include <gfx/seadCamera.h>
#include <gfx/seadProjection.h>
#include <math/seadBoundBox.h>
#include <math/seadMathCalcCommon.h>
#include <math/seadMatrix.h>
#include <message/seadMessageSet.h>
#include <nn/gfx/gfx_DescriptorSlot.h>
#include <nn/gfx/gfx_Texture.h>
#include <nn/gfx/gfx_Sampler.h>
#include <prim/seadDelegate.h>
#include <utility/aglMultiFilter.h>
#include <utility/aglResParameter.h>

namespace eui {

namespace {

/** @brief Kind of the value stored in an extended user data entry. */
enum ExtUserDataType : u8 {
    ExtUserDataType_String = 0,
    ExtUserDataType_Int = 1,
    ExtUserDataType_Float = 2,
};

/**
 * @param pData Extended user data entry.
 * @return The payload of the entry.
 */
template <typename T>
inline const T* GetExtUserData(const nn::ui2d::ResExtUserData* pData) {
    return reinterpret_cast<const T*>(reinterpret_cast<const char*>(pData) + pData->dataOffset);
}

/**
 * @param pPane Pane to check.
 * @return Whether the pane has a type deriving from T.
 */
template <typename T>
inline bool IsDerived(const nn::ui2d::Pane* pPane) {
    return nn::ui2d::IsDerivedFrom<T>(pPane);
}

}  // namespace

/**
 * Calculates the axis-aligned bounds of a pane from its global matrix and size.
 * @param pBox Receives the bounds.
 * @param rPane Pane to measure.
 */
void CalcPaneBoundBox(sead::BoundBox2f* pBox, const nn::ui2d::Pane& rPane) {
    const auto& mtx = *reinterpret_cast<const sead::Matrix34f*>(rPane.GetGlobalMtx());
    const f32 halfWidth = sead::Mathf::abs(rPane.GetSizeX() * mtx(0, 0)) * 0.5f;
    const f32 halfHeight = sead::Mathf::abs(rPane.GetSizeY() * mtx(1, 1)) * 0.5f;
    sead::Vector2f center(mtx(0, 3), mtx(1, 3));

    switch (rPane.GetBasePositionX()) {
    case nn::ui2d::HorizontalPosition_Left:
        center.x = center.x + halfWidth;
        break;
    case nn::ui2d::HorizontalPosition_Right:
        center.x = center.x - halfWidth;
        break;
    default:
        break;
    }

    switch (rPane.GetBasePositionY()) {
    case nn::ui2d::VerticalPosition_Top:
        center.y = center.y - halfHeight;
        break;
    case nn::ui2d::VerticalPosition_Bottom:
        center.y = center.y + halfHeight;
        break;
    default:
        break;
    }

    pBox->setMin(sead::Vector2f(center.x - halfWidth, center.y - halfHeight));
    pBox->setMax(sead::Vector2f(halfWidth + center.x, halfHeight + center.y));
}

/**
 * Points a texture info at the image of an agl texture.
 * @param pInfo Texture info to set up.
 * @param rTexture Texture providing the image.
 * @param pComponentSelection Unused component selection.
 * @return Always true.
 */
bool SetupTextureInfoByAglTextureData(nn::ui2d::TextureInfo* pInfo,
                                      const agl::TextureData& rTexture,
                                      const agl::TextureCompSel* pComponentSelection) {
    const u16 width = rTexture.getWidth(0);
    const u16 height = rTexture.getHeight(0);
    auto* placement = nn::ui2d::DynamicCast<nn::ui2d::PlacementTextureInfo*>(pInfo);

    if (placement != nullptr) {
        placement->mWidth = width;
        placement->mHeight = height;
    }

    rTexture.getTexture().setReference_();
    pInfo->mDescriptor.ToData()->value = static_cast<u32>(rTexture.getTextureID());
    return true;
}

/**
 * Converts an agl texture format to a layout image format; no format is supported.
 * @param pFormat Receives the converted format.
 * @param format Format to convert.
 * @param isSrgb Whether the sRGB variant is wanted.
 * @return Always false.
 */
bool AglTextureFormatToLytFormat(nn::gfx::ImageFormat* pFormat, agl::TextureFormat format,
                                 bool isSrgb) {
    *pFormat = static_cast<nn::gfx::ImageFormat>(0);
    return false;
}

/**
 * Initializes an agl texture from the resource texture of a texture info.
 * @param pTexture Texture to initialize.
 * @param rInfo Resource texture info to read.
 * @return Whether the texture info had a texture view.
 */
bool SetupAglTextureDataByTextureInfo(agl::TextureData* pTexture,
                                      const nn::ui2d::TextureInfo& rInfo) {
    if (rInfo.GetTextureView() == nullptr) {
        return false;
    }

    const auto& resourceInfo = static_cast<const nn::ui2d::ResourceTextureInfo&>(rInfo);
    nn::gfx::ResTexture* resource = resourceInfo.m_pResource;
    resource->ToData().userDescriptorSlot.value = rInfo.mDescriptor.ToData()->value;
    agl::g3d::TextureDataInitializerG3D::initialize(pTexture, *resource);
    return true;
}

/**
 * Finds an extended user data entry by name.
 * @param pList List to search; may be null.
 * @param pName Name of the entry.
 * @return The entry, or null if it does not exist.
 */
const nn::ui2d::ResExtUserData* FindExtUserDataFromList(const nn::ui2d::ResExtUserDataList* pList, const char* pName) {
    if (pList == nullptr) {
        return nullptr;
    }

    const u32 count = pList->count;
    const auto* entry = pList->entries;

    for (size_t i = 0; i < count; ++i, ++entry) {
        const char* name = entry->nameOffset ? reinterpret_cast<const char*>(entry) + entry->nameOffset : nullptr;

        if (std::strcmp(pName, name) == 0) {
            return entry;
        }
    }

    return nullptr;
}

/**
 * Reverses a cardinal direction.
 * @param direction Direction to reverse; an invalid value is treated as up.
 * @return The opposite direction.
 */
Direction GetOppositeDirection(Direction direction)
{
    switch (direction.value()) {
    case Direction::cDown: return Direction::cUp;
    case Direction::cLeft: return Direction::cRight;
    case Direction::cRight: return Direction::cLeft;
    default: return Direction::cDown;
    }
}

/**
 * Converts a cardinal direction to an angle measured counterclockwise from right.
 * @param direction Direction to convert; invalid values return zero.
 * @return The angle in radians.
 */
f32 GetRadAngleOfDirection(Direction direction)
{
    switch (direction.value()) {
    case Direction::cUp: return 1.5707963705062866f;
    case Direction::cDown: return 4.71238899230957f;
    case Direction::cLeft: return 3.1415927410125732f;
    default: return 0.0f;
    }
}

/**
 * Resizes a pane to fit the text boxes listed in its "AdjustToTextOn" user data.
 * @param pPane Pane to resize.
 * @param pLayout Layout in which the listed text boxes are searched.
 */
void AdjustPaneSizeToTextSize(nn::ui2d::Pane* pPane, LayoutEx* pLayout) {
    const auto* adjustOn = pPane->FindExtUserDataByName("AdjustToTextOn");

    if (adjustOn == nullptr || adjustOn->type != ExtUserDataType_String) {
        return;
    }

    const char* names = GetExtUserData<char>(adjustOn);
    sead::FixedSafeString<25> name;
    f32 textWidth;
    f32 textHeight;

    if (names[0] == '\0') {
        textHeight = 0.0f;
        textWidth = textHeight;
    } else {
        textWidth = 0.0f;
        textHeight = textWidth;
        s32 start = 0;
        const char* line = names;

        do {
            s32 end = start;
            s32 length = -1;
            char c;

            do {
                c = names[end++];
                length++;
            } while ((c != '\0') != (c == '\n'));

            end--;

            if (length > 24) {
                return;
            }

            if (length >= 1 && names[end - 1] == '\r') {
                length--;
            }

            start = c == '\n' ? end + 1 : end;
            name.copy(sead::SafeString(line), length);
            nn::ui2d::Pane* textPane = pLayout->findPaneByName(name.cstr());

            if (textPane == nullptr) {
                return;
            }

            if (IsDerived<TextBoxEx>(textPane)) {
                const nn::font::Rectangle rect =
                    static_cast<TextBoxEx*>(textPane)->GetTextDrawRect();
                const f32 width = rect.right - rect.left;
                const f32 height = rect.top - rect.bottom;
                textWidth = textWidth > width ? textWidth : width;
                textHeight = textHeight > height ? textHeight : height;

                if (!textPane->IsLocationAdjust()) {
                    textPane->mFlags |= nn::ui2d::Pane::PaneFlag_LocationAdjust |
                                        nn::ui2d::Pane::PaneFlag_IsGlobalMatrixDirty;
                }
            } else {
                const f32 width = textPane->GetSizeX();
                const f32 height = textPane->GetSizeY();
                textWidth = textWidth > width ? textWidth : width;
                textHeight = textHeight > height ? textHeight : height;
            }

            line = names + start;
        } while (*line != '\0');
    }

    sead::Vector2f margin = sead::Vector2f::zero;
    bool isWidthOnly = false;
    const auto* marginData = pPane->FindExtUserDataByName("AdjustToTextMargin");

    if (marginData != nullptr) {
        const f32* values = GetExtUserData<f32>(marginData);
        margin.x = values[0];

        if (marginData->count >= 2) {
            margin.y = values[1];
        } else {
            isWidthOnly = true;
        }
    }

    sead::Vector2f minSize = sead::Vector2f::zero;

    if (IsDerived<WindowEx>(pPane)) {
        const auto* window = static_cast<const WindowEx*>(pPane);
        const nn::ui2d::WindowFrameSize frameSize = window->GetFrameSize();
        minSize.x = window->m_WindowKind == nn::ui2d::WindowKind_HorizontalNoContent ?
                        frameSize.right * 2.0f :
                        frameSize.left + frameSize.right;
        minSize.y = frameSize.top + frameSize.bottom;
    }

    const auto* minSizeData = pPane->FindExtUserDataByName("AdjustToTextMinSize");

    if (minSizeData != nullptr && minSizeData->count != 0) {
        const f32* values = GetExtUserData<f32>(minSizeData);
        minSize.x = minSize.x > values[0] ? minSize.x : values[0];

        if (minSizeData->count != 1) {
            minSize.y = minSize.y > values[1] ? minSize.y : values[1];
        }
    }

    nn::ui2d::Size size;
    const f32 width = textWidth + (margin.x + margin.x);
    size.width = width > minSize.x ? width : minSize.x;

    if (isWidthOnly) {
        size.height = pPane->GetSizeY();
    } else {
        const f32 height = textHeight + (margin.y + margin.y);
        size.height = height > minSize.y ? height : minSize.y;
    }

    pPane->SetSize(size);
    reinterpret_cast<u8*>(&pLayout->mFlags)[0] |= 1;
}

/**
 * Lays out the two text boxes named by "CenteringPairLeft"/"CenteringPairRight" so that the
 * pair is centered.
 * @param pPane Pane holding the pair settings.
 */
void CenteringPanePair(nn::ui2d::Pane* pPane) {
    const auto* leftData = pPane->FindExtUserDataByName("CenteringPairLeft");

    if (leftData == nullptr) {
        return;
    }

    const auto* rightData = pPane->FindExtUserDataByName("CenteringPairRight");

    if (rightData == nullptr) {
        return;
    }

    nn::ui2d::Pane* left = pPane->FindPaneByName(GetExtUserData<char>(leftData), true);
    nn::ui2d::Pane* right = pPane->FindPaneByName(GetExtUserData<char>(rightData), true);

    if (left == nullptr || right == nullptr) {
        return;
    }

    nn::ui2d::Pane* textPane;
    nn::ui2d::Pane* other;
    f32 sign;

    if (IsDerived<TextBoxEx>(right)) {
        if (static_cast<TextBoxEx*>(right)->GetTextPositionH() !=
            nn::ui2d::HorizontalPosition_Left) {
            return;
        }

        sign = 1.0f;
        textPane = right;
        other = left;
    } else if (IsDerived<TextBoxEx>(left)) {
        if (static_cast<TextBoxEx*>(left)->GetTextPositionH() !=
            nn::ui2d::HorizontalPosition_Right) {
            return;
        }

        sign = -1.0f;
        textPane = left;
        other = right;
    } else {
        return;
    }

    auto* textBox = static_cast<TextBoxEx*>(textPane);
    const auto* marginData = pPane->FindExtUserDataByName("CenteringPairMargin");
    const f32 margin = marginData != nullptr ? *GetExtUserData<f32>(marginData) : 0.0f;
    const f32 textWidth = textBox->calcStringWidth_();
    const f32 otherWidth = other->GetSizeX();
    const f32 totalWidth = margin + (textWidth + otherWidth);
    other->SetPositionX(sign * (otherWidth - totalWidth) * 0.5f);
    textBox->SetPositionX(sign * ((totalWidth + textBox->GetSizeX()) * 0.5f - textWidth));
}

/**
 * Sets a texture on one texture map of every material of a pane.
 * @param pPane Pane whose materials are changed.
 * @param rTexture Texture to set.
 * @param index Index of the texture map to replace.
 */
void ApplyTextureInfoToMaterial(nn::ui2d::Pane* pPane, const nn::ui2d::TextureInfo& rTexture,
                                int index) {
    const u8 count = pPane->GetMaterialCount();

    for (int i = 0; i != count; i++) {
        nn::ui2d::Material* material = pPane->GetMaterial(i);

        if (index < material->GetTexMapNum()) {
            material->SetTextureInfo(index, &rTexture);
        }
    }
}

/**
 * Binds the capture texture named by "CaptureUseName" to a picture or window pane.
 * @param pPane Pane using the capture.
 * @param pLayout Layout from which the capture pane path is resolved.
 */
void ApplyCaptureUse(nn::ui2d::Pane* pPane, LayoutEx* pLayout) {
    if (!IsDerived<PictureEx>(pPane) && !IsDerived<WindowEx>(pPane)) {
        return;
    }

    const auto* nameData = pPane->FindExtUserDataByName("CaptureUseName");

    if (nameData == nullptr) {
        return;
    }

    const auto* indexData = pPane->FindExtUserDataByName("CaptureUseIndex");

    if (indexData == nullptr) {
        return;
    }

    auto* capture = nn::ui2d::DynamicCast<CapturePane*>(
        FindPaneByPath(GetExtUserData<char>(nameData), pLayout, nullptr));

    if (capture == nullptr) {
        return;
    }

    ApplyTextureInfoToMaterial(pPane, capture->mTextureInfo, *GetExtUserData<s32>(indexData));
}

/**
 * Finds a pane from a path of parts names separated by '/'.
 * @param pPath Path to the pane; a leading '/' starts at the topmost layout and ".." moves up.
 * @param pLayout Layout the path is relative to.
 * @param ppLayout Receives the layout containing the found pane; may be null.
 * @return The pane, or null if the path cannot be resolved.
 */
nn::ui2d::Pane* FindPaneByPath(const char* pPath, LayoutEx* pLayout, LayoutEx** ppLayout) {
    if (*pPath == '/') {
        while (pLayout->getParentLayout() != nullptr) {
            pLayout = pLayout->getParentLayout();
        }

        pPath++;
    }

    if (*pPath == '\0') {
        return nullptr;
    }

    const sead::SafeString path(pPath);
    sead::FixedSafeString<64> name;
    const sead::SafeString parentName("..");
    auto it = path.tokenBegin("/");
    const auto end = path.tokenEnd("/");

    while (true) {
        it.getAndForward(&name);

        if (it == end) {
            break;
        }

        if (name == parentName) {
            pLayout = pLayout->getParentLayout();

            if (pLayout == nullptr) {
                return nullptr;
            }
        } else {
            pLayout = pLayout->findPartsLayout(name.cstr());

            if (pLayout == nullptr) {
                return nullptr;
            }
        }
    }

    if (ppLayout != nullptr) {
        *ppLayout = pLayout;
    }

    return pLayout->getRootPane()->FindPaneByName(name.cstr(), true);
}

/**
 * Binds the dynamic capture named by "DynamicCaptureUseName" to a dynamic capture user pane.
 * @param pPane Pane using the capture.
 * @param pLayout Layout from which the capture pane path is resolved.
 */
void ApplyDynamicCaptureUse(nn::ui2d::Pane* pPane, LayoutEx* pLayout) {
    auto* picture = nn::ui2d::DynamicCast<DynamicCaptureUsePictureEx*>(pPane);
    auto* window = nn::ui2d::DynamicCast<DynamicCaptureUseWindowEx*>(pPane);

    if (picture == nullptr && window == nullptr) {
        return;
    }

    const auto* nameData = pPane->FindExtUserDataByName("DynamicCaptureUseName");

    if (nameData == nullptr) {
        return;
    }

    const auto* indexData = pPane->FindExtUserDataByName("DynamicCaptureUseIndex");

    if (indexData == nullptr) {
        return;
    }

    auto* capture = nn::ui2d::DynamicCast<DynamicCapturePane*>(
        FindPaneByPath(GetExtUserData<char>(nameData), pLayout, nullptr));

    if (capture == nullptr) {
        return;
    }

    if (picture != nullptr) {
        picture->setupDynamicCapture(capture, *GetExtUserData<s32>(indexData));
    } else if (window != nullptr) {
        window->setupDynamicCapture(capture, *GetExtUserData<s32>(indexData));
    }
}

/**
 * Creates the multi filter of a pane from its "MultiFilterFile" user data.
 * @param pHeap Heap for the filter.
 * @param rPane Pane holding the filter settings.
 * @param pLayout Layout of the pane.
 * @return The filter, or null if the pane uses none.
 */
agl::utl::MultiFilter* InitializeMultiFilter(sead::Heap* pHeap, const nn::ui2d::Pane& rPane,
                                             LayoutEx* pLayout) {
    const auto* fileData = rPane.FindExtUserDataByName("MultiFilterFile");

    if (fileData == nullptr) {
        const Screen* screen = pLayout->getScreen();

        if (screen == nullptr || screen->getViewerType() == 0) {
            return nullptr;
        }
    }

    auto* filter = new (pHeap, 8) agl::utl::MultiFilter();
    filter->initialize(pHeap, pHeap);
    filter->setUnknownBaa(true);

    if (fileData != nullptr && pLayout->getScreen() != nullptr) {
        const ScreenMgr* screenMgr = pLayout->getScreen()->getScreenMgr();
        const void* data = screenMgr->getMultiFilterParameterData(
            sead::FormatFixedSafeString<256>("%s.baglmf", GetExtUserData<char>(fileData)));

        if (data != nullptr) {
            const agl::utl::ResParameterArchive archive(data);
            filter->applyResParameterArchive(archive);
        }
    }

    return filter;
}

/**
 * Requests a capture from every capture pane of a pane tree.
 * @param pPane Root of the tree.
 */
void IteratePaneForRequestCapture(nn::ui2d::Pane* pPane) {
    auto* capture = nn::ui2d::DynamicCast<CapturePane*>(pPane);

    if (capture != nullptr) {
        capture->mCaptureRequired = true;
    }

    for (auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children;
         link = link->GetNext()) {
        IteratePaneForRequestCapture(nn::ui2d::Pane::FromLink(link));
    }
}

namespace {

/**
 * Converts a sead matrix to a layout projection matrix.
 * @param pDst Receives the matrix.
 * @param rSrc Matrix to convert.
 */
inline void ConvertMatrix(nn::util::MatrixT4x4fType* pDst, const sead::Matrix44f& rSrc) {
    const float32x4_t row0 = vld1q_f32(rSrc.m[0]);
    const float32x4_t row1 = vld1q_f32(rSrc.m[1]);
    const float32x4_t row2 = vld1q_f32(rSrc.m[2]);
    const float32x4_t row3 = vld1q_f32(rSrc.m[3]);
    pDst->_m.val[0] = row0;
    pDst->_m.val[1] = row1;
    pDst->_m.val[2] = row2;
    pDst->_m.val[3] = row3;
}

/**
 * Converts a sead matrix to a layout view matrix.
 * @param pDst Receives the matrix.
 * @param rSrc Matrix to convert.
 */
inline void ConvertMatrix(nn::util::MatrixT4x3fType* pDst, const sead::Matrix34f& rSrc) {
    const float32x4_t row0 = vld1q_f32(rSrc.m[0]);
    const float32x4_t row1 = vld1q_f32(rSrc.m[1]);
    const float32x4_t row2 = vld1q_f32(rSrc.m[2]);
    pDst->_m.val[0] = row0;
    pDst->_m.val[1] = row1;
    pDst->_m.val[2] = row2;
}

}  // namespace

/**
 * Sets up an orthographic projection and camera covering a screen size.
 * @param pDrawInfo Draw info to set up.
 * @param rSize Size of the screen.
 */
void SetupDrawInfoOrtho(nn::ui2d::DrawInfo* pDrawInfo, const nn::ui2d::Size& rSize) {
    const f32 top = rSize.height * 0.5f;
    const f32 right = rSize.width * 0.5f;
    const f32 bottom = rSize.height * -0.5f;
    const f32 left = rSize.width * -0.5f;
    const sead::OrthoProjection projection(0.0f, 300.0f, top, bottom, left, right);
    sead::OrthoCamera camera(projection);
    camera.updateViewMatrix();
    nn::util::MatrixT4x4fType projectionMtx;
    ConvertMatrix(&projectionMtx, projection.getDeviceProjectionMatrix());
    pDrawInfo->SetProjectionMtx(projectionMtx);
    ConvertMatrix(&pDrawInfo->m_ViewMtx, camera.getMatrix());
}

/**
 * Sets up a perspective projection and camera covering a screen size.
 * @param pDrawInfo Draw info to set up.
 * @param rSize Size of the screen.
 * @param fovy Vertical field of view in radians.
 */
void SetupDrawInfoPerspective(nn::ui2d::DrawInfo* pDrawInfo, const nn::ui2d::Size& rSize,
                              float fovy) {
    const f32 halfWidth = rSize.width * 0.5f;
    const f32 halfHeight = rSize.height * 0.5f;
    const f32 distance = halfHeight / std::tan(fovy * 0.5f);
    const sead::PerspectiveProjection projection(1.0f, 10000.0f, fovy, halfWidth / halfHeight);
    sead::LookAtCamera camera(sead::Vector3f(0.0f, 0.0f, distance), sead::Vector3f::zero,
                              sead::Vector3f::ey);
    camera.updateViewMatrix();
    nn::util::MatrixT4x4fType projectionMtx;
    ConvertMatrix(&projectionMtx, projection.getDeviceProjectionMatrix());
    pDrawInfo->SetProjectionMtx(projectionMtx);
    ConvertMatrix(&pDrawInfo->m_ViewMtx, camera.getMatrix());
}

namespace {

/**
 * Allocates a pane from the layout allocator and copy-constructs it.
 * @param rArgs Arguments forwarded to the constructor of T.
 * @return The new pane, or null if the allocation failed.
 */
template <typename T, typename... Args>
inline T* NewPane(const Args&... rArgs) {
    void* pMemory = nn::ui2d::Layout::AllocateMemory(sead::Mathi::roundUpPow2(sizeof(T), 16), 16);

    if (pMemory == nullptr) {
        return nullptr;
    }

    return new (pMemory) T(rArgs...);
}

/**
 * Copies a pane of type T.
 * @param pPane Pane to copy.
 * @return The copy.
 */
template <typename T>
inline T* ClonePane(const nn::ui2d::Pane* pPane) {
    return NewPane<T>(*static_cast<const T*>(pPane));
}

/**
 * Copies a pane of type T that belongs to a layout.
 * @param pPane Pane to copy.
 * @param pLayout Layout the copy belongs to.
 * @return The copy.
 */
template <typename T>
inline T* ClonePane(const nn::ui2d::Pane* pPane, LayoutEx* pLayout) {
    return NewPane<T>(*static_cast<const T*>(pPane), pLayout);
}

}  // namespace

/**
 * Copies a pane and all of its children.
 * @param pPane Root of the tree to copy.
 * @param pLayout Layout the copy belongs to.
 * @param isLayoutRoot Whether the copy becomes the root pane of pLayout.
 * @return The copy of the root.
 */
nn::ui2d::Pane* ClonePaneTree(const nn::ui2d::Pane* pPane, LayoutEx* pLayout, bool isLayoutRoot) {
    nn::ui2d::Pane* clone;

    if (IsDerived<CapturePane>(pPane)) {
        clone = ClonePane<CapturePane>(pPane, pLayout);
    } else if (IsDerived<DynamicCapturePane>(pPane)) {
        clone = ClonePane<DynamicCapturePane>(pPane, pLayout);
    } else if (IsDerived<ScissorPane>(pPane)) {
        clone = ClonePane<ScissorPane>(pPane);
    } else if (IsDerived<MassDrawPane>(pPane)) {
        clone = ClonePane<MassDrawPane>(pPane);
    } else if (IsDerived<MultiFilterPictureEx>(pPane)) {
        clone = ClonePane<MultiFilterPictureEx>(pPane, pLayout);
    } else if (IsDerived<MultiFilterWindowEx>(pPane)) {
        clone = ClonePane<MultiFilterWindowEx>(pPane, pLayout);
    } else if (IsDerived<DynamicCaptureUsePictureEx>(pPane)) {
        clone = ClonePane<DynamicCaptureUsePictureEx>(pPane);
    } else if (IsDerived<DynamicCaptureUseWindowEx>(pPane)) {
        clone = ClonePane<DynamicCaptureUseWindowEx>(pPane);
    } else if (IsDerived<ScalableFontTextBoxEx>(pPane)) {
        clone = ClonePane<ScalableFontTextBoxEx>(pPane, pLayout);
    } else if (IsDerived<PictureEx>(pPane)) {
        clone = ClonePane<PictureEx>(pPane);
    } else if (IsDerived<WindowEx>(pPane)) {
        clone = ClonePane<WindowEx>(pPane);
    } else if (IsDerived<TextBoxEx>(pPane)) {
        clone = ClonePane<TextBoxEx>(pPane, pLayout);
    } else if (IsDerived<BoundingEx>(pPane)) {
        clone = ClonePane<BoundingEx>(pPane);
    } else if (IsDerived<PartsEx>(pPane)) {
        if (isLayoutRoot) {
            PartsEx* parts = ClonePane<PartsEx>(pPane);
            parts->m_pLayout = pLayout;
            pLayout->mRootPane = parts;
            clone = parts;
        } else {
            const auto* parts = static_cast<const PartsEx*>(pPane);
            const auto* srcLayout = static_cast<const LayoutEx*>(parts->m_pLayout);
            void* pMemory = nn::ui2d::Layout::AllocateMemory(sizeof(LayoutEx), 16);
            auto* partsLayout = new (pMemory) LayoutEx(*srcLayout, parts->GetName(), pLayout, false);
            nn::ui2d::DynamicCast<PartsEx*>(partsLayout->getRootPane())->m_pLayout = partsLayout;
            return partsLayout->getRootPane();
        }
    } else if (IsDerived<AlignPane>(pPane)) {
        clone = ClonePane<AlignPane>(pPane, pLayout);
    } else if (IsDerived<RootPane>(pPane)) {
        clone = ClonePane<RootPane>(pPane, pLayout);
    } else {
        clone = NewPane<nn::ui2d::Pane>(*pPane);
    }

    for (const auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children;
         link = link->GetNext()) {
        clone->AppendChild(ClonePaneTree(nn::ui2d::Pane::FromLink(link), pLayout, false));
    }

    return clone;
}

/**
 * Creates a copy of a group whose panes are looked up in another pane tree.
 * @param rGroup Group to copy.
 * @param pRoot Root of the pane tree containing the panes of the copy.
 * @return The new group.
 */
nn::ui2d::Group* CloneGroup(const nn::ui2d::Group& rGroup, nn::ui2d::Pane* pRoot) {
    const char* name = rGroup.mName;
    auto* group = nn::ui2d::Layout::NewObj<nn::ui2d::Group>(name);

    for (const auto& link : rGroup.mPanes) {
        group->AppendPane(pRoot->FindPaneByName(link.pane->GetName(), true));
    }

    return group;
}

namespace {

/** @brief Maximum number of parent layouts used for a unique name. */
const s32 cUniqueNameDepthMax = 5;

/**
 * Appends the names of the parts layouts above an item, outermost first.
 * @param pName String to append to.
 * @param pLayout Layout containing the item.
 */
inline void AppendLayoutPrefix(sead::StringBuilder* pName, const LayoutEx* pLayout) {
    if (pLayout == nullptr || pLayout->getParentLayout() == nullptr) {
        return;
    }

    const LayoutEx* layouts[cUniqueNameDepthMax];
    s32 count = 0;

    for (const LayoutEx* layout = pLayout; layout->getParentLayout() != nullptr;
         layout = layout->getParentLayout()) {
        layouts[count++] = layout;

        if (count >= cUniqueNameDepthMax) {
            break;
        }
    }

    for (s32 i = count; i != 0; i--) {
        pName->append(layouts[i - 1]->getRootPane()->GetName(), -1);
        pName->append("-", -1);
    }
}

}  // namespace

/**
 * Builds a name of a layout item that is unique among all parts layouts.
 * @param pName Receives the name.
 * @param pItem Name of the item.
 * @param pLayout Layout containing the item.
 */
void CreateLayoutItemUniqueName(sead::StringBuilder* pName, const char* pItem,
                                const LayoutEx* pLayout) {
    pName->clear();
    AppendLayoutPrefix(pName, pLayout);
    pName->append(pItem, -1);
}

/**
 * Appends a name of a layout item that is unique among all parts layouts.
 * @param pName String to append to.
 * @param pItem Name of the item.
 * @param pLayout Layout containing the item.
 */
void AppendLayoutItemUniqueName(sead::StringBuilder* pName, const char* pItem,
                                const LayoutEx* pLayout) {
    AppendLayoutPrefix(pName, pLayout);
    pName->append(pItem, -1);
}

/**
 * Builds a unique name of a layout item from a path of parts names.
 * @param pName Receives the name.
 * @param pPath Path to the item; a leading '/' makes it absolute.
 * @param pLayout Layout the path is relative to.
 */
void CreateLayoutItemUniqueNameByPath(sead::StringBuilder* pName, const char* pPath,
                                      const LayoutEx* pLayout) {
    pName->clear();

    if (*pPath == '\0') {
        return;
    }

    if (*pPath == '/') {
        pName->copy(pPath + 1);
        pName->replaceChar('/', '-');
        return;
    }

    const sead::SafeString path(pPath);
    sead::FixedSafeString<64> name;
    auto it = path.tokenBegin("/");
    const auto end = path.tokenEnd("/");
    it.getAndForward(&name);
    CreateLayoutItemUniqueName(pName, name.cstr(), pLayout);

    while (end != it) {
        it.getAndForward(&name);
        pName->append("-", -1);
        pName->append(name.cstr(), -1);
    }
}

/**
 * Passes every application tag of a message to a delegate.
 * @param rMessage Message to scan.
 * @param pDelegate Delegate invoked with each tag whose group is not a system group.
 */
void ProcessMessageAppTag(const MessageString& rMessage,
                          sead::IDelegate1<const sead::MessageSet<char16_t>::TagInfo*>* pDelegate) {
    const char16_t* text = rMessage.getText();

    if (text == nullptr) {
        return;
    }

    const s32 length = rMessage.getLength();

    for (s32 i = 0; i < length;) {
        const char16_t* current = text + i;

        if ((*current | 1) == 0xf) {
            const char16_t* tag;
            current = MessageString::readTag_(current, &tag);
            const auto* tagInfo = reinterpret_cast<const sead::MessageSet<char16_t>::TagInfo*>(tag);

            if (tagInfo->mGroup >= 2) {
                pDelegate->invoke(tagInfo);
            }

            i = (reinterpret_cast<const char*>(current) - reinterpret_cast<const char*>(text)) /
                static_cast<s32>(sizeof(char16_t));
        } else {
            i++;
        }
    }
}

/**
 * Applies the after-build setup to every pane of a tree.
 * @param pPane Root of the tree.
 * @param pLayout Layout of the root; replaced by the parts layout inside parts panes.
 */
void IteratePaneForSetupPaneAfterBuild(nn::ui2d::Pane* pPane, LayoutEx* pLayout) {
    auto* parts = nn::ui2d::DynamicCast<PartsEx*>(pPane);

    if (parts != nullptr) {
        pLayout = static_cast<LayoutEx*>(parts->m_pLayout);
    }

    SetupPaneAfterBuild(pPane, pLayout);

    for (auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children;
         link = link->GetNext()) {
        IteratePaneForSetupPaneAfterBuild(nn::ui2d::Pane::FromLink(link), pLayout);
    }
}

/**
 * Applies the user data driven setup to a pane after it was built.
 * @param pPane Pane to set up.
 * @param pLayout Layout of the pane.
 */
void SetupPaneAfterBuild(nn::ui2d::Pane* pPane, LayoutEx* pLayout) {
    AdjustPaneSizeToTextSize(pPane, pLayout);
    CenteringPanePair(pPane);
    ApplyCaptureUse(pPane, pLayout);
    ApplyDynamicCaptureUse(pPane, pLayout);
}

/**
 * @return The heap used by the layout allocator.
 */
sead::Heap* GetNwAllocatorHeap() {
    return static_cast<sead::Heap*>(nn::ui2d::Layout::g_pUserDataForAllocator);
}

/**
 * Copies the descriptor of the texture of a texture map into a texture info.
 * @param pInfo Texture info to set.
 * @param rTexMap Texture map to read.
 */
void SetTextureInfoFromTexMap(nn::ui2d::TextureInfo* pInfo, const nn::ui2d::TexMap& rTexMap) {
    const nn::ui2d::TextureInfo* texture = rTexMap.m_pTextureInfo;
    pInfo->mDescriptor.Invalidate();
    pInfo->mDescriptor = texture->mDescriptor;
}

/**
 * Finds the layout a pane belongs to.
 * @param pPane Pane to look up.
 * @return The layout of the nearest parts pane or root pane above the pane, or null.
 */
LayoutEx* LocateBelongingLayout(const nn::ui2d::Pane* pPane) {
    const auto* parts = nn::ui2d::DynamicCast<const PartsEx*>(pPane);

    if (parts != nullptr) {
        return static_cast<LayoutEx*>(parts->m_pLayout);
    }

    if (pPane->GetParent() != nullptr) {
        return LocateBelongingLayout(pPane->GetParent());
    }

    const auto* root = nn::ui2d::DynamicCast<const RootPane*>(pPane);

    if (root != nullptr) {
        return root->m_pLayout;
    }

    return nullptr;
}

/**
 * Debug output of a layout as pane tree macros; empty in release builds.
 * @param rLayout Layout to print.
 * @param rFontMgr Font manager used to name fonts.
 */
void PrintRapidPaneTreeMacro(const LayoutEx& rLayout, const FontMgr& rFontMgr) {}

/**
 * Debug output of a pane tree as macros; empty in release builds.
 * @param pPane Root of the tree.
 * @param depth Indentation depth.
 * @param rFontMgr Font manager used to name fonts.
 */
void PrintRapidPaneTreeMacroRecursive_(const nn::ui2d::Pane* pPane, int depth,
                                       const FontMgr& rFontMgr) {}

/**
 * Debug output of a pane tree; empty in release builds.
 * @param pPane Root of the tree.
 * @param depth Indentation depth.
 */
void PrintPaneTree(const nn::ui2d::Pane* pPane, int depth) {}

/**
 * Checks whether a point lies inside the bounds of a pane.
 * @param rPosition Point to test.
 * @param pPane Pane to test against.
 * @return Whether the point is inside.
 */
bool IsHitPane(const sead::Vector2f& rPosition, const nn::ui2d::Pane* pPane) {
    sead::BoundBox2f box;
    CalcPaneBoundBox(&box, *pPane);
    return box.isInside(rPosition);
}

/**
 * Finds the innermost layout with a visible picture, window or text box under a point.
 * @param rPosition Point to test.
 * @param pLayout Layout whose pane tree is tested.
 * @return The layout that was hit, or null.
 */
LayoutEx* FindHitLayout(const sead::Vector2f& rPosition, LayoutEx* pLayout) {
    return FindHitLayoutRecursive_(rPosition, nullptr, pLayout, pLayout->getRootPane());
}

/**
 * Finds the innermost layout with a visible picture, window or text box under a point.
 * @param rPosition Point to test.
 * @param pHit Result so far.
 * @param pLayout Layout of pPane.
 * @param pPane Root of the pane tree to test.
 * @return The layout that was hit last, or pHit if nothing in the tree was hit.
 */
LayoutEx* FindHitLayoutRecursive_(const sead::Vector2f& rPosition, LayoutEx* pHit,
                                  LayoutEx* pLayout, const nn::ui2d::Pane* pPane) {
    if (!pPane->IsVisible()) {
        return pHit;
    }

    if (pPane->IsInfluencedAlpha() && pPane->GetAlpha() == 0) {
        return pHit;
    }

    if (IsDerived<CapturePane>(pPane) || IsDerived<DynamicCapturePane>(pPane)) {
        return pHit;
    }

    if ((IsDerived<PictureEx>(pPane) || IsDerived<WindowEx>(pPane) ||
         IsDerived<TextBoxEx>(pPane)) &&
        pPane->GetAlpha() != 0 && IsHitPane(rPosition, pPane)) {
        pHit = pLayout;
    }

    const auto* parts = nn::ui2d::DynamicCast<const PartsEx*>(pPane);

    if (parts != nullptr) {
        pLayout = static_cast<LayoutEx*>(parts->m_pLayout);
    }

    LayoutEx* result = pHit;

    for (const auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children;
         link = link->GetNext()) {
        LayoutEx* hit =
            FindHitLayoutRecursive_(rPosition, pHit, pLayout, nn::ui2d::Pane::FromLink(link));

        if (hit != nullptr) {
            result = hit;
        }
    }

    return result;
}

/**
 * Finds where a message overflows the width of a text box.
 * @param rTextBox Text box the message is shown in.
 * @param rMessage Message to measure.
 * @return The index of the first character past the width limit, or -1 if all of it fits.
 */
int FindTextBoxWidthOverPosition(const TextBoxEx& rTextBox, const MessageString& rMessage) {
    if (rMessage.getText() == nullptr) {
        return -1;
    }

    nn::font::TextWriterBase<u16> writer;
    rTextBox.SetupTextWriter(&writer);
    writer.SetWidthLimit(rTextBox.GetSizeX());
    const u16* limit = writer.FindPosOfWidthLimit(
        reinterpret_cast<const u16*>(rMessage.getText()), rMessage.getLength());
    const s32 position = limit - reinterpret_cast<const u16*>(rMessage.getText());
    s32 result = -1;

    if (position <= static_cast<s32>(rMessage.getLength()) && position >= 1) {
        const char16_t* current = rMessage.getText();
        const char16_t* end = rMessage.getText() + position;

        while (current < end) {
            if ((*current | 1) == 0xf) {
                const char16_t* tag;
                current = MessageString::readTag_(current, &tag);
            } else {
                current++;
            }
        }

        result = current - rMessage.getText();
    }

    return result;
}

/**
 * Registers a texture descriptor with the graphics driver.
 * @param pSlot Receives the descriptor ID.
 * @param rView Texture and view to register.
 * @param pUserData Unused callback context.
 * @return Always true.
 */
bool RegisterSlotForTexture(nn::gfx::DescriptorSlot* pSlot, const nn::gfx::TextureView& rView,
                            void* pUserData)
{
    auto data = rView.ToData();
    pSlot->ToData()->value = static_cast<u32>(agl::driver::NVNMgr::instance()->registerTexture(
        static_cast<const NVNtexture*>(data->pNvnTexture.ptr),
        static_cast<const NVNtextureView*>(data->pNvnTextureView.ptr), "ui2d"));
    return true;
}

/**
 * Registers a sampler descriptor with the graphics driver.
 * @param pSlot Receives the descriptor ID.
 * @param rSampler Sampler to register.
 * @param pUserData Unused callback context.
 * @return Always true.
 */
bool RegisterSlotForSampler(nn::gfx::DescriptorSlot* pSlot, const nn::gfx::Sampler& rSampler,
                            void* pUserData)
{
    pSlot->ToData()->value = agl::driver::NVNMgr::instance()->registerSampler(
        static_cast<const NVNsampler*>(rSampler.ToData()->pNvnSampler.ptr), "ui2d");
    return true;
}

/**
 * Releases a registered texture descriptor.
 * @param pSlot Slot containing the descriptor ID to release.
 * @param rView Unused texture view.
 * @param pUserData Unused callback context.
 */
void UnregisterSlotForTexture(nn::gfx::DescriptorSlot* pSlot, const nn::gfx::TextureView& rView,
                              void* pUserData)
{
    agl::driver::NVNMgr::instance()->releaseTexture(pSlot->ToData()->value);
}

/**
 * Releases a registered sampler descriptor.
 * @param pSlot Slot containing the descriptor ID to release.
 * @param rSampler Unused sampler.
 * @param pUserData Unused callback context.
 */
void UnregisterSlotForSampler(nn::gfx::DescriptorSlot* pSlot, const nn::gfx::Sampler& rSampler,
                              void* pUserData)
{
    agl::driver::NVNMgr::instance()->releaseSampler(pSlot->ToData()->value);
}

/**
 * Finds the sub font resource in a font package.
 * @param pData Font package data.
 * @return The sub font resource.
 */
const void* GetResSubFontFromBfcpx(const void* pData) {
    const u32 signature = *static_cast<const u32*>(pData);

    if (signature != 'XPCF') {
        char message[256];
        nn::util::SNPrintf(message, sizeof(message),
                           "Signature check failed ('%c%c%c%c' must be '%c%c%c%c').",
                           signature >> 24, (signature >> 16) & 0xff, (signature >> 8) & 0xff,
                           signature & 0xff, 'X', 'P', 'C', 'F');
    }

    const auto* offset = reinterpret_cast<const u32*>(static_cast<const u8*>(pData) + 0x14);
    return reinterpret_cast<const u8*>(offset) + *offset;
}

namespace {

/** @brief Header of a sub font description in a complex font package. */
struct ResSubFontHeader {
    u32 type;
    f32 size;
    u32 _8;
};

/** @brief Value of ResSubFontHeader::type for a scalable font. */
const u32 cSubFontType_Scalable = 1;

}  // namespace

/**
 * Registers every scalable complex font of a font archive with a texture cache setup.
 * @param pArg Texture cache arguments to set up.
 * @param pParameters Receives the parameters of the registered fonts.
 * @param pHeap Heap for the archive and the font names.
 * @param pData Archive data.
 * @param size Size of the archive data.
 * @return The number of registered fonts.
 */
s32 SetupScalableFontMgrInitializeArgParameterFromArchiveBinary(
    nn::font::TextureCache::InitializeArg* pArg,
    sead::Buffer<ScalableFontMgr::FontParameter>* pParameters, sead::Heap* pHeap,
    const void* pData, size_t size) {
    SharcArchive archive;
    archive.initialize(pHeap, const_cast<void*>(pData), size);
    pArg->SetDefault();
    pArg->noPlotWorkMemorySize = 0x19000;
    s32 count = 0;

    {
        SharcArchive::FileReader reader;
        archive.startFileReader(&reader);

        while (reader.readNext()) {
            const sead::SafeString& fileName = reader.getEntry().name;

            if (!fileName.startsWith("fcpx/")) {
                continue;
            }

            const auto* subFont = static_cast<const ResSubFontHeader*>(GetResSubFontFromBfcpx(
                reader.getFileDevice().getArchive()->getFileFast(reader.getIndex(), nullptr)));

            if (subFont->type != cSubFontType_Scalable) {
                continue;
            }

            if (!pParameters->isIndexValid(count)) {
                break;
            }

            ScalableFontMgr::FontParameter* parameters = pParameters->getBufferPtr();
            const s32 bufferSize = fileName.calcLength() - 4;
            char* buffer = new (pHeap, 8) char[bufferSize];
            sead::BufferedSafeString name(buffer, bufferSize);
            name.copy(fileName.getPart(5));
            name.chop(5);
            name.append("fcpx");
            auto& parameter = parameters[count];
            parameter.name = buffer;
            nn::ui2d::ComplexFontHelper::SetupTextureCacheArg(
                pArg, AcquireFontFunction_, &archive,
                reader.getFileDevice().getArchive()->getFileFast(reader.getIndex(), nullptr));
            parameter.face = count;
            count++;
            parameter.size = subFont->size;
            parameter._10 = subFont->_8;
        }
    }

    archive.finalize();
    return count;
}

/**
 * Loads a font of a complex font from the font archive.
 * @param pSize Receives the size of the font data; may be null.
 * @param pName Name of the font.
 * @param type Unused font type.
 * @param pUserData Font archive.
 * @return The font data.
 */
void* AcquireFontFunction_(size_t* pSize, const char* pName, u32 type, void* pUserData) {
    sead::FixedSafeString<256> path;
    path.format("scft/%s", pName);
    sead::ArchiveRes::FileInfo info = {};
    const void* data =
        static_cast<const SharcArchive*>(pUserData)->getArchive()->getFile(path, &info);

    if (pSize != nullptr) {
        *pSize = info.mLength;
    }

    return const_cast<void*>(data);
}

}  // namespace eui
