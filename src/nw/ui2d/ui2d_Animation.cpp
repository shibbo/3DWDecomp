#include <nn/ui2d/ui2d_AnimTransform.h>

#include <cstring>
#include <nn/ui2d/ui2d_AnimResource.h>
#include <nn/ui2d/ui2d_DynamicCast.h>
#include <nn/ui2d/ui2d_ExtUserData.h>
#include <nn/ui2d/ui2d_Group.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_Material.h>
#include <nn/ui2d/ui2d_Parts.h>
#include <nn/ui2d/ui2d_ResourceAccessor.h>
#include <nn/ui2d/ui2d_StateMachine.h>
#include <nn/ui2d/ui2d_TextBox.h>
#include <nn/ui2d/ui2d_TextureInfo.h>
#include <nn/ui2d/ui2d_Util.h>
#include <nn/ui2d/ui2d_Window.h>
#include <nn/util.h>
#include <nn/util/util_BinTypes.h>

namespace nn::ui2d {
namespace {
using nn::util::MakeSignature;

/** @brief Number of characters compared in a pane name. */
const int ResourceNameStrMax = 24;
/** @brief Number of characters compared in a material name. */
const int MaterialNameStrMax = 28;
/** @brief Number of material color animation targets stored as bytes. */
const int AnimTargetMatColorMax = 28;
/** @brief Index of Pane::mPositionX when the pane is viewed as an array of floats. */
const int PaneSrtFirstElement = 0x30 / sizeof(float);
/** @brief Number of constant colors of the combiners. */
const int ConstantColorMax = 5;

/** @brief Kind of the object an animation content animates. */
enum AnimContentKind {
    AnimContentType_Pane,
    AnimContentType_Material,
    AnimContentType_ExtUserData,
    AnimContentType_StateMachine,
    AnimContentType_PartsStateLayer,
};

/** @brief Type of the values of an extended user data. */
enum ExtUserDataType {
    ExtUserDataType_String,
    ExtUserDataType_Int,
    ExtUserDataType_Float,
};

/** @brief Curve type of an animation target. */
enum AnimCurveType {
    AnimCurveType_Constant,
    AnimCurveType_Step,
    AnimCurveType_Hermite,
    AnimCurveType_ParameterizedAnim,
};

/** @brief Signature of an animation file ("FLAN"). */
const u32 AnimationFileSignature = MakeSignature('F', 'L', 'A', 'N');

/** @brief Signatures of the blocks of an animation file. */
enum AnimationBlockSignature : u32 {
    AnimationBlockSignature_Tag = MakeSignature('p', 'a', 't', '1'),
    AnimationBlockSignature_Animation = MakeSignature('p', 'a', 'i', '1'),
    AnimationBlockSignature_Share = MakeSignature('p', 'a', 'h', '1'),
};

/** @brief Kinds of the animation infos of a content. */
enum AnimInfoKind : u32 {
    AnimInfoKind_PaneSrt = MakeSignature('F', 'L', 'P', 'A'),
    AnimInfoKind_Visibility = MakeSignature('F', 'L', 'V', 'I'),
    AnimInfoKind_VertexColor = MakeSignature('F', 'L', 'V', 'C'),
    AnimInfoKind_ProceduralShape = MakeSignature('F', 'L', 'P', 'S'),
    AnimInfoKind_PerCharacterTransform = MakeSignature('F', 'L', 'C', 'T'),
    AnimInfoKind_MaskTexSrt = MakeSignature('F', 'L', 'M', 'T'),
    AnimInfoKind_WindowFrameSize = MakeSignature('F', 'L', 'W', 'N'),
    AnimInfoKind_DropShadow = MakeSignature('F', 'L', 'D', 'S'),
    AnimInfoKind_StateMachineEvent = MakeSignature('F', 'S', 'M', 'A'),
    AnimInfoKind_MaterialColor = MakeSignature('F', 'L', 'M', 'C'),
    AnimInfoKind_TextureSrt = MakeSignature('F', 'L', 'T', 'S'),
    AnimInfoKind_TexturePattern = MakeSignature('F', 'L', 'T', 'P'),
    AnimInfoKind_IndirectSrt = MakeSignature('F', 'L', 'I', 'M'),
    AnimInfoKind_AlphaCompare = MakeSignature('F', 'L', 'A', 'C'),
    AnimInfoKind_FontShadow = MakeSignature('F', 'L', 'F', 'S'),
    AnimInfoKind_BrickRepeat = MakeSignature('F', 'T', 'B', 'R'),
    AnimInfoKind_VectorGraphics = MakeSignature('F', 'V', 'G', 'A'),
};

/** @brief Window frame size animation targets. */
enum AnimTargetWindowFrame {
    AnimTargetWindowFrame_Top,
    AnimTargetWindowFrame_Bottom,
    AnimTargetWindowFrame_Left,
    AnimTargetWindowFrame_Right,
};

/** @brief Key of a step animation curve. */
struct ResStepKey {
    float frame;
    u16 value;
    u16 padding;
};

/** @brief Header of an animation file and of each of its blocks. */
struct ResAnimationFileHeader {
    u32 signature;
    u16 byteOrder;
    u16 headerSize;
    u32 version;
    u32 fileSize;
    u16 dataBlocks;
    u16 reserved;
};

/** @brief Header of a block of an animation file. */
struct ResAnimationBlockHeader {
    u32 kind;
    u32 size;
};

/** @brief System extended user data attaching a state machine to a pane. */
struct StateMachineSystemData {
    PaneSystemDataType type;
    StateMachine* pStateMachine;
};

/**
 * @brief Compares two pane names over at most ResourceNameStrMax characters.
 * @param pName1 First name.
 * @param pName2 Second name.
 * @return Whether the names are equal.
 */
inline bool EqualsResName(const char* pName1, const char* pName2) {
    for (int i = 0; i < ResourceNameStrMax; ++i) {
        if (pName1[i] != pName2[i]) {
            return false;
        }

        if (pName1[i] == '\0') {
            return true;
        }
    }

    return true;
}

/**
 * @brief Compares two material names over at most MaterialNameStrMax characters.
 * @param pName1 First name.
 * @param pName2 Second name.
 * @return Whether the names are equal.
 */
inline bool EqualsMaterialName(const char* pName1, const char* pName2) {
    for (int i = 0; i < MaterialNameStrMax; ++i) {
        if (pName1[i] != pName2[i]) {
            return false;
        }

        if (pName1[i] == '\0') {
            return true;
        }
    }

    return true;
}

/**
 * @brief Checks the signature of a binary file.
 * @param signature Signature of the file.
 * @param expected Expected signature.
 */
inline void CheckSignature(u32 signature, u32 expected) {
    if (signature != expected) {
        char message[256];
        nn::util::SNPrintf(message, sizeof(message),
                           "Signature check failed ('%c%c%c%c' must be '%c%c%c%c').",
                           (signature >> 24) & 0xff, (signature >> 16) & 0xff,
                           (signature >> 8) & 0xff, signature & 0xff, (expected >> 24) & 0xff,
                           (expected >> 16) & 0xff, (expected >> 8) & 0xff, expected & 0xff);
    }
}

/**
 * @param pBlock Animation block.
 * @return Offsets of the contents of the block, relative to the block.
 */
inline const u32* GetAnimContentOffsets(const ResAnimationBlock* pBlock) {
    return reinterpret_cast<const u32*>(reinterpret_cast<const u8*>(pBlock) +
                                        pBlock->contentOffsets);
}

/**
 * @param pBlock Animation block.
 * @param pOffsets Offsets of the contents of the block.
 * @param index Index of the content.
 * @return The content at index.
 */
inline const ResAnimationContent* GetAnimContent(const ResAnimationBlock* pBlock,
                                                 const u32* pOffsets, int index) {
    return reinterpret_cast<const ResAnimationContent*>(reinterpret_cast<const u8*>(pBlock) +
                                                        pOffsets[index]);
}

/**
 * @param rContent Animation content.
 * @return Whether the content has an extended header naming its target.
 */
inline bool HasExtendedHeader(const ResAnimationContent& rContent) {
    return rContent.type == AnimContentType_PartsStateLayer ||
           rContent.type == AnimContentType_ExtUserData;
}

/**
 * @param rContent Animation content.
 * @return Offsets of the animation infos of the content, relative to the content.
 */
inline const u32* GetAnimInfoOffsets(const ResAnimationContent& rContent) {
    const u8* pBase = reinterpret_cast<const u8*>(&rContent);
    const u32 offset = HasExtendedHeader(rContent) ?
                           *reinterpret_cast<const u32*>(pBase + sizeof(ResAnimationContent)) :
                           sizeof(ResAnimationContent);
    return reinterpret_cast<const u32*>(pBase + offset);
}

/**
 * @param rContent Animation content.
 * @param pOffsets Offsets of the animation infos of the content.
 * @param index Index of the animation info.
 * @return The animation info at index.
 */
inline const ResAnimationInfo* GetAnimInfo(const ResAnimationContent& rContent,
                                           const u32* pOffsets, int index) {
    return reinterpret_cast<const ResAnimationInfo*>(reinterpret_cast<const u8*>(&rContent) +
                                                     pOffsets[index]);
}

/**
 * @param pInfo Animation info.
 * @return Offsets of the targets of the info, relative to the info.
 */
inline const u32* GetAnimTargetOffsets(const ResAnimationInfo* pInfo) {
    return pInfo->targetOffsets;
}

/**
 * @param pInfo Animation info.
 * @param pOffsets Offsets of the targets of the info.
 * @param index Index of the target.
 * @return The target at index.
 */
inline const ResAnimationTarget* GetAnimTarget(const ResAnimationInfo* pInfo, const u32* pOffsets,
                                               int index) {
    return reinterpret_cast<const ResAnimationTarget*>(reinterpret_cast<const u8*>(pInfo) +
                                                       pOffsets[index]);
}

/**
 * @param rContent Animation content.
 * @return Name of the extended user data or variable the content targets, or nullptr.
 */
inline const char* GetExtUserDataTargetName(const ResAnimationContent& rContent) {
    if (!HasExtendedHeader(rContent)) {
        return nullptr;
    }

    const u8* pBase = reinterpret_cast<const u8*>(&rContent);
    const u32* pNameOffset = reinterpret_cast<const u32*>(
        pBase + *reinterpret_cast<const u32*>(pBase + sizeof(ResAnimationContent) + sizeof(u32)));
    return reinterpret_cast<const char*>(pNameOffset) + *pNameOffset;
}

/**
 * @param pTarget Animation target.
 * @return The keys of the curve of the target.
 */
template <typename T>
inline const T* GetKeys(const ResAnimationTarget* pTarget) {
    return reinterpret_cast<const T*>(reinterpret_cast<const u8*>(pTarget) + pTarget->keysOffset);
}

/**
 * @brief Evaluates a step curve.
 * @param frame Frame to evaluate.
 * @param pKeys Keys of the curve.
 * @param keyCount Number of keys.
 * @return Value of the key active at frame.
 */
inline u16 GetStepCurveValue(float frame, const ResStepKey* pKeys, u32 keyCount) {
    if (keyCount == 1 || frame <= pKeys[0].frame) {
        return pKeys[0].value;
    }

    if (frame >= pKeys[keyCount - 1].frame) {
        return pKeys[keyCount - 1].value;
    }

    int left = 0;
    int right = keyCount - 1;
    while (left != right - 1 && left != right) {
        const int center = (left + right) / 2;
        if (frame < pKeys[center].frame) {
            right = center;
        } else {
            left = center;
        }
    }

    const float diff = frame - pKeys[right].frame;
    if (-0.001f < diff && diff < 0.001f) {
        return pKeys[right].value;
    }

    return pKeys[left].value;
}

/**
 * @param pParameter Event segment of a parameterized animation.
 * @return Array of the three arguments of the event.
 */
inline const void* const* GetEvents(const ResParameterizedAnimParameter* pParameter) {
    return *reinterpret_cast<const void* const* const*>(&pParameter->value);
}

/**
 * @param pPane Pane whose system data is searched.
 * @return The state machine system data of the pane, or nullptr.
 */
inline const StateMachineSystemData* GetStateMachineSystemData(const Pane* pPane) {
    return static_cast<const StateMachineSystemData*>(
        pPane->GetSystemExtDataByType(PaneSystemDataType_StateMachine));
}

/**
 * @param pPane Pane to check.
 * @return Whether the pane owns a state machine, directly or through its parts layout.
 */
inline bool HasStateMachine(Pane* pPane) {
    if (std::strcmp(pPane->GetName(), "RootPane") == 0) {
        const StateMachineSystemData* pData = GetStateMachineSystemData(pPane);
        if (pData != nullptr && pData->pStateMachine != nullptr) {
            return true;
        }
    }

    Parts* pParts = DynamicCast<Parts*>(pPane);
    if (pParts != nullptr) {
        const StateMachineSystemData* pData =
            GetStateMachineSystemData(pParts->m_pLayout->GetRootPane());
        if (pData != nullptr && pData->pStateMachine != nullptr) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Sets a state machine variable, clamped to its range.
 * @param rManager Variable manager owning the variable.
 * @param pName Name of the variable.
 * @param value New value.
 * @return Whether the value changed.
 */
inline bool SetVariableFloatValue(StateMachineVariableManager& rManager, const char* pName,
                                  float value) {
    StateMachineVariable* pVariable = rManager.FindByName_(pName);
    detail::ClampValue(value, pVariable->minimum, pVariable->maximum);
    const float previous = pVariable->value;
    if (value == previous) {
        return false;
    }

    pVariable->value = value;
    rManager.PushModifyEvent_(pVariable->name, pVariable);
    for (auto& rCalculated : pVariable->GetCalculatedVariables()) {
        rManager.DoUpdateCalcVarOnValueChanged_(&rCalculated, previous, value);
    }

    return true;
}

/**
 * @brief Sets a state machine variable and notifies the listener.
 * @param pStateMachine State machine owning the variable.
 * @param pName Name of the variable.
 * @param value New value.
 */
inline void SetStateMachineFloatValue(StateMachine* pStateMachine, const char* pName, float value) {
    StateMachineVariableManager& rManager = pStateMachine->m_VariableManager;
    if (rManager.FindRefOnlyByName_(pName) == nullptr) {
        return;
    }

    const float previous = rManager.FindRefOnlyByName_(pName)->value;
    if (!SetVariableFloatValue(rManager, pName, value)) {
        return;
    }

    const float current = rManager.FindRefOnlyByName_(pName)->value;
    if (pStateMachine->m_pListener != nullptr) {
        pStateMachine->m_pListener->OnVariableChanged(pStateMachine->m_pName, pName, previous,
                                                      current);
    }
}

/**
 * @param value Animated color value.
 * @return The value rounded and clamped to a color element.
 */
inline float ClampColorElement(float value) {
    return std::min(std::max(value + 0.5f, 0.0f), 255.0f);
}

/**
 * @param color Color packed as RGBA bytes.
 * @param element Index of the element.
 * @return The element of the color.
 */
inline int GetElement(u32 color, int element) {
    switch (element) {
    case 0:
        return color & 0xff;
    case 1:
        return (color >> 8) & 0xff;
    case 2:
        return (color >> 16) & 0xff;
    case 3:
        return color >> 24;
    default:
        return 0;
    }
}

/**
 * @param color Color.
 * @return The color packed as RGBA bytes.
 */
inline u32 ToPackedColor(nn::util::Unorm8x4 color) {
    u32 packed;
    std::memcpy(&packed, &color, sizeof(packed));
    return packed;
}

/**
 * @param packed Color packed as RGBA bytes.
 * @return The color.
 */
inline nn::util::Unorm8x4 FromPackedColor(u32 packed) {
    nn::util::Unorm8x4 color;
    std::memcpy(&color, &packed, sizeof(color));
    return color;
}

/**
 * @brief Sets an element of a color.
 * @param color Color to modify.
 * @param element Index of the element.
 * @param value New value of the element.
 * @return The modified color.
 */
inline nn::util::Unorm8x4 SetElement(nn::util::Unorm8x4 color, int element, int value) {
    switch (element) {
    case 0:
        color.v[0] = value;
        break;
    case 1:
        color.v[1] = value;
        break;
    case 2:
        color.v[2] = value;
        break;
    case 3:
        color.v[3] = value;
        break;
    default:
        break;
    }

    return color;
}

/**
 * @param pPane Pane.
 * @param index Index of the element.
 * @return The mask texture SRT element of the pane; the pane must have a mask.
 */
inline float GetMaskTexSrtElement(const Pane* pPane, int index) {
    const u8* pData =
        static_cast<const u8*>(pPane->GetSystemExtDataByType(PaneSystemDataType_Mask));
    return reinterpret_cast<const float*>(pData + 0x18)[index];
}

/**
 * @param pPane Pane.
 * @param index Index of the element.
 * @return The procedural shape element of the pane, or 0 when it has no procedural shape.
 */
inline float GetProceduralShapeElement(const Pane* pPane, int index) {
    const u8* pData =
        static_cast<const u8*>(pPane->GetSystemExtDataByType(PaneSystemDataType_ProceduralShape));
    if (pData == nullptr) {
        return 0.0f;
    }

    return reinterpret_cast<const float*>(pData + 0x1c)[index];
}

/**
 * @brief Sets a value of an extended user data.
 * @param pExtUserData Extended user data.
 * @param index Index of the value.
 * @param value New value.
 */
template <typename T>
inline void SetExtUserDataValue(ResExtUserData* pExtUserData, int index, T value) {
    T* pData = reinterpret_cast<T*>(reinterpret_cast<u8*>(pExtUserData) + pExtUserData->dataOffset);
    pData[index] = value;
}

/**
 * @brief Animates the values of an extended user data.
 * @param pExtUserData Extended user data to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateExtUserData(ResExtUserData* pExtUserData, const ResAnimationInfo* pInfo,
                               const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const ResHermiteKey* pKeys = GetKeys<ResHermiteKey>(pTarget);
        switch (pExtUserData->type) {
        case ExtUserDataType_Int: {
            const int value = GetHermiteCurveValue(frame, pKeys, pTarget->keyCount);
            SetExtUserDataValue<int>(pExtUserData, pTarget->id, value);
            break;
        }
        case ExtUserDataType_Float: {
            const float value = GetHermiteCurveValue(frame, pKeys, pTarget->keyCount);
            SetExtUserDataValue<float>(pExtUserData, pTarget->id, value);
            break;
        }
        default:
            break;
        }
    }
}

/**
 * @brief Animates the mask texture SRT of a pane.
 * @param pPane Pane to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateMaskTexSrt(Pane* pPane, const ResAnimationInfo* pInfo,
                              const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const u8 curveType = pTarget->curveType;
        const void* pCurve = GetKeys<void>(pTarget);
        float value;
        if (curveType == AnimCurveType_ParameterizedAnim) {
            value = GetParameterizedAnimValue(frame, GetMaskTexSrtElement(pPane, pTarget->target),
                                              static_cast<const ResParameterizedAnim*>(pCurve));
        } else {
            value = GetHermiteCurveValue(frame, static_cast<const ResHermiteKey*>(pCurve),
                                         pTarget->keyCount);
        }

        const u8 target = pTarget->target;
        u8* pData = static_cast<u8*>(pPane->GetSystemExtDataForModify(PaneSystemDataType_Mask));
        if (pData != nullptr) {
            reinterpret_cast<float*>(pData + 0x18)[target] = value;
        }
    }
}

/**
 * @param pPane Pane.
 * @param index Index of the element, from the X translation to the height.
 * @return The translation, rotation, scale or size element of the pane.
 */
inline float& GetSrtElement(Pane* pPane, int index) {
    return reinterpret_cast<float*>(pPane)[PaneSrtFirstElement + index];
}

/**
 * @brief Animates the translation, rotation, scale and size of a pane.
 * @param pPane Pane to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimatePaneSrt(Pane* pPane, const ResAnimationInfo* pInfo, const u32* pTargetOffsets,
                           float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const u8 curveType = pTarget->curveType;
        const void* pCurve = GetKeys<void>(pTarget);
        float value;
        if (curveType == AnimCurveType_ParameterizedAnim) {
            value = GetParameterizedAnimValue(frame, GetSrtElement(pPane, pTarget->target),
                                              static_cast<const ResParameterizedAnim*>(pCurve));
        } else {
            value = GetHermiteCurveValue(frame, static_cast<const ResHermiteKey*>(pCurve),
                                         pTarget->keyCount);
        }

        GetSrtElement(pPane, pTarget->target) = value;
        pPane->SetGlobalMatrixDirty();
    }
}

/**
 * @brief Animates the visibility of a pane.
 * @param pPane Pane to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateVisibility(Pane* pPane, const ResAnimationInfo* pInfo,
                              const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const u16 value =
            GetStepCurveValue(frame, GetKeys<ResStepKey>(pTarget), pTarget->keyCount);
        pPane->mFlags = static_cast<u8>((pPane->mFlags & ~Pane::PaneFlag_Visible) |
                                        (value != 0 ? Pane::PaneFlag_Visible : 0));
    }
}

/**
 * @brief Animates the drop shadow of a pane.
 * @param pPane Pane to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateDropShadow(Pane* pPane, const ResAnimationInfo* pInfo,
                              const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const float value =
            GetHermiteCurveValue(frame, GetKeys<ResHermiteKey>(pTarget), pTarget->keyCount);
        const u8 target = pTarget->target;
        u8* pData =
            static_cast<u8*>(pPane->GetSystemExtDataForModify(PaneSystemDataType_DropShadow));
        if (pData != nullptr) {
            reinterpret_cast<float*>(pData + 0x20)[target] = value;
        }
    }
}

/**
 * @brief Finds the state machine system data an event of a pane is sent to.
 * @param pPane Pane playing the event.
 * @param pTargetName Name of the event target; "body" sends to the owner of the pane.
 * @return The system data, or nullptr.
 */
inline const StateMachineSystemData* FindEventStateMachine(Pane* pPane, const char* pTargetName) {
    if (std::strcmp(pTargetName, "body") != 0) {
        Parts* pParts = DynamicCast<Parts*>(pPane);
        return GetStateMachineSystemData(pParts->m_pLayout->GetRootPane());
    }

    if (pPane == nullptr) {
        return nullptr;
    }

    Pane* pOwner = pPane;
    while (!HasStateMachine(pOwner)) {
        pOwner = pOwner->GetParent();
        if (pOwner == nullptr) {
            return nullptr;
        }
    }

    return GetStateMachineSystemData(pOwner);
}

/**
 * @brief Sends the state machine events of a pane whose frame was reached.
 * @param pPane Pane to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateStateMachineEvent(Pane* pPane, const ResAnimationInfo* pInfo,
                                     const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const ResParameterizedAnim* pAnim = GetKeys<ResParameterizedAnim>(pTarget);
        const int parameterCount = pAnim->parameterCount;
        for (int j = 0; j < parameterCount; ++j) {
            const ResParameterizedAnimParameter* pParameter = pAnim->GetParameter(j);
            if (!(pParameter->offset <= frame && frame <= pParameter->offset + 0.1f)) {
                continue;
            }

            const void* const* pEvents = GetEvents(pParameter);
            if (pEvents[0] == nullptr && pEvents[1] == nullptr && pEvents[2] == nullptr) {
                continue;
            }

            const StateMachineSystemData* pData =
                FindEventStateMachine(pPane, static_cast<const char*>(pEvents[0]));
            if (pData == nullptr || pData->pStateMachine == nullptr) {
                continue;
            }

            int type;
            switch (pParameter->parameterizedAnimType) {
            case 0x10:
                type = 8;
                break;
            case 0x11:
                type = 9;
                break;
            case 0x12:
                type = 10;
                break;
            default:
                NN_UNEXPECTED_DEFAULT;
            }

            pEvents = GetEvents(pParameter);
            pData->pStateMachine->m_EventQueue.Push(type, static_cast<const char*>(pEvents[0]),
                                                    pEvents[1],
                                                    static_cast<const char*>(pEvents[2]), 0);
        }
    }
}

/**
 * @brief Animates the vertex colors and alpha of a pane.
 * @param pPane Pane to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateVertexColor(Pane* pPane, const ResAnimationInfo* pInfo,
                               const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const u8 curveType = pTarget->curveType;
        const void* pCurve = GetKeys<void>(pTarget);
        float value;
        if (curveType == AnimCurveType_ParameterizedAnim) {
            value = GetParameterizedAnimValue(frame, pPane->GetColorElement(pTarget->target),
                                              static_cast<const ResParameterizedAnim*>(pCurve));
        } else {
            value = GetHermiteCurveValue(frame, static_cast<const ResHermiteKey*>(pCurve),
                                         pTarget->keyCount);
        }

        pPane->SetColorElement(pTarget->target, static_cast<int>(ClampColorElement(value)));
    }
}

/**
 * @brief Animates the procedural shape of a pane.
 * @param pPane Pane to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateProceduralShape(Pane* pPane, const ResAnimationInfo* pInfo,
                                   const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const u8 curveType = pTarget->curveType;
        const void* pCurve = GetKeys<void>(pTarget);
        float value;
        if (curveType == AnimCurveType_ParameterizedAnim) {
            value = GetParameterizedAnimValue(frame,
                                              GetProceduralShapeElement(pPane, pTarget->target),
                                              static_cast<const ResParameterizedAnim*>(pCurve));
        } else {
            value = GetHermiteCurveValue(frame, static_cast<const ResHermiteKey*>(pCurve),
                                         pTarget->keyCount);
        }

        const u8 target = pTarget->target;
        u8* pData =
            static_cast<u8*>(pPane->GetSystemExtDataForModify(PaneSystemDataType_ProceduralShape));
        if (pData != nullptr) {
            reinterpret_cast<float*>(pData + 0x1c)[target] = value;
        }
    }
}

/**
 * @param size Animated frame size in pixels.
 * @return The frame size clamped to 0.
 */
inline u16 ToFrameSize(int size) {
    return size < 0 ? 0 : size;
}

/**
 * @brief Animates the frame size of a window pane.
 * @param pWindow Window to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateWindowFrameSize(Window* pWindow, const ResAnimationInfo* pInfo,
                                   const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const float value =
            GetHermiteCurveValue(frame, GetKeys<ResHermiteKey>(pTarget), pTarget->keyCount);
        const int size = static_cast<int>(value);
        switch (pTarget->target) {
        case AnimTargetWindowFrame_Top:
            pWindow->m_WindowSize.frameSize.top = ToFrameSize(size);
            break;
        case AnimTargetWindowFrame_Bottom:
            pWindow->m_WindowSize.frameSize.bottom = ToFrameSize(size);
            break;
        case AnimTargetWindowFrame_Left:
            pWindow->m_WindowSize.frameSize.left = ToFrameSize(size);
            break;
        case AnimTargetWindowFrame_Right:
            pWindow->m_WindowSize.frameSize.right = ToFrameSize(size);
            break;
        default:
            break;
        }
    }
}

/**
 * @brief Animates the per character transform of a text box.
 * @param pTextBox Text box to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimatePerCharacterTransform(TextBox* pTextBox, const ResAnimationInfo* pInfo,
                                         const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const u8 curveType = pTarget->curveType;
        const void* pCurve = GetKeys<void>(pTarget);
        float value;
        if (curveType == AnimCurveType_ParameterizedAnim) {
            value = GetParameterizedAnimValue(frame,
                                              pTextBox->GetPerCharacterTransform(pTarget->target),
                                              static_cast<const ResParameterizedAnim*>(pCurve));
        } else {
            value = GetHermiteCurveValue(frame, static_cast<const ResHermiteKey*>(pCurve),
                                         pTarget->keyCount);
        }

        pTextBox->SetPerCharacterTransform(pTarget->target, value);
    }
}

/**
 * @param pMaterial Material.
 * @return The alpha compare settings of the material.
 */
inline ResAlphaCompare* GetAlphaCompare(const Material* pMaterial) {
    return reinterpret_cast<ResAlphaCompare*>(static_cast<u8*>(pMaterial->m_pMem) +
                                              pMaterial->GetAlphaCompareOffset());
}

/**
 * @param pMaterial Material.
 * @return The brick repeat texture generation parameters of the material.
 */
inline ResBrickRepeatTexGenParameters* GetBrickRepeatParameters(const Material* pMaterial) {
    return reinterpret_cast<ResBrickRepeatTexGenParameters*>(
        static_cast<u8*>(pMaterial->m_pMem) + pMaterial->GetBrickRepeatTexGenParametersOffset());
}

/**
 * @param pMaterial Material.
 * @return The indirect texture parameter of the material.
 */
inline ResIndirectParameter* GetIndirectParameter(const Material* pMaterial) {
    return reinterpret_cast<ResIndirectParameter*>(static_cast<u8*>(pMaterial->m_pMem) +
                                                   pMaterial->GetIndirectParameterOffset());
}

/**
 * @param pMaterial Material.
 * @return The texture SRT parameters of the material.
 */
inline ResTexSrt* GetTexSrts(const Material* pMaterial) {
    return reinterpret_cast<ResTexSrt*>(static_cast<u8*>(pMaterial->m_pMem) +
                                        pMaterial->GetTexSrtOffset());
}

/**
 * @param pMaterial Material.
 * @return The vector graphics texture references of the material.
 */
inline detail::RefVectorGraphicsTextureInfo* GetVectorGraphicsRefInfos(const Material* pMaterial) {
    return reinterpret_cast<detail::RefVectorGraphicsTextureInfo*>(
        static_cast<u8*>(pMaterial->m_pMem) + pMaterial->GetVectorGraphicsTextureRefInfoOffset());
}

/**
 * @param pMaterial Material.
 * @return The font shadow parameter of the material.
 */
inline ResFontShadowParameter* GetFontShadowParameter(const Material* pMaterial) {
    return reinterpret_cast<ResFontShadowParameter*>(static_cast<u8*>(pMaterial->m_pMem) +
                                                     pMaterial->GetFontShadowParameterOffset());
}

/**
 * @param pMaterial Material.
 * @return The detailed combiner settings of the material.
 */
inline ResDetailedCombinerStageInfo* GetDetailedCombinerStageInfo(const Material* pMaterial) {
    return reinterpret_cast<ResDetailedCombinerStageInfo*>(
        static_cast<u8*>(pMaterial->m_pMem) + pMaterial->GetDetailedCombinerStageInfoOffset());
}

/**
 * @param pMaterial Material.
 * @return The combiner user shader settings of the material.
 */
inline ResCombinerUserShader* GetCombinerUserShader(const Material* pMaterial) {
    return reinterpret_cast<ResCombinerUserShader*>(static_cast<u8*>(pMaterial->m_pMem) +
                                                    pMaterial->GetCombinerUserShaderOffset());
}

/**
 * @brief Sets an element of a brick repeat parameter of a material.
 * @param pMaterial Material.
 * @param index Index of the texture.
 * @param element Index of the element.
 * @param value New value.
 */
inline void SetBrickRepeatElement(Material* pMaterial, int index, int element, float value) {
    reinterpret_cast<float*>(&GetBrickRepeatParameters(pMaterial)[index])[element] =
        value;
}

/**
 * @brief Sets an element of a vector graphics texture of a material.
 * @param pMaterial Material.
 * @param index Index of the texture.
 * @param element Index of the element.
 * @param value New value.
 */
inline void SetVectorGraphicsElement(Material* pMaterial, int index, int element, float value) {
    reinterpret_cast<float*>(&GetVectorGraphicsRefInfos(pMaterial)[index])[element] =
        value;
}

/**
 * @param pMaterial Material.
 * @param index Index of the texture.
 * @param element Index of the element.
 * @return An element of a texture SRT of the material.
 */
inline float GetTexSrtElement(const Material* pMaterial, int index, int element) {
    return reinterpret_cast<const float*>(&GetTexSrts(pMaterial)[index])[element];
}

/**
 * @brief Sets an element of a texture SRT of a material.
 * @param pMaterial Material.
 * @param index Index of the texture.
 * @param element Index of the element.
 * @param value New value.
 */
inline void SetTexSrtElement(Material* pMaterial, int index, int element, float value) {
    reinterpret_cast<float*>(&GetTexSrts(pMaterial)[index])[element] = value;
}

/**
 * @brief Sets an element of the font shadow parameter of a material.
 * @param pMaterial Material.
 * @param element Index of the element.
 * @param value New value.
 */
inline void SetFontShadowElement(Material* pMaterial, int element, u8 value) {
    reinterpret_cast<u8*>(GetFontShadowParameter(pMaterial))[element] = value;
}

/**
 * @brief Sets the alpha compare reference value of a material.
 * @param pMaterial Material.
 * @param value New reference value; clamped to [0, 1].
 */
inline void SetAlphaCompareRef(Material* pMaterial, float value) {
    const float ref = std::min(std::max(value, 0.0f), 1.0f);
    ResAlphaCompare* pAlphaCompare = GetAlphaCompare(pMaterial);
    *pAlphaCompare = ResAlphaCompare(static_cast<AlphaTest>(pAlphaCompare->func), ref);
}

/**
 * @brief Animates the alpha compare reference value of a material.
 * @param pMaterial Material to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateAlphaCompare(Material* pMaterial, const ResAnimationInfo* pInfo,
                                const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const float value =
            GetHermiteCurveValue(frame, GetKeys<ResHermiteKey>(pTarget), pTarget->keyCount);
        SetAlphaCompareRef(pMaterial, value);
    }
}

/**
 * @brief Animates the brick repeat parameters of a material.
 * @param pMaterial Material to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateBrickRepeat(Material* pMaterial, const ResAnimationInfo* pInfo,
                               const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const float value =
            GetHermiteCurveValue(frame, GetKeys<ResHermiteKey>(pTarget), pTarget->keyCount);
        SetBrickRepeatElement(pMaterial, pTarget->id, pTarget->target, value);
    }
}

/**
 * @brief Animates the indirect texture SRT of a material.
 * @param pMaterial Material to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateIndirectSrt(Material* pMaterial, const ResAnimationInfo* pInfo,
                               const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const u8 target = pTarget->target;
        const float value =
            GetHermiteCurveValue(frame, GetKeys<ResHermiteKey>(pTarget), pTarget->keyCount);
        reinterpret_cast<float*>(GetIndirectParameter(pMaterial))[target] = value;
    }
}

/**
 * @brief Animates the texture SRT of a material.
 * @param pMaterial Material to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateTextureSrt(Material* pMaterial, const ResAnimationInfo* pInfo,
                              const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const u8 curveType = pTarget->curveType;
        const void* pCurve = GetKeys<void>(pTarget);
        float value;
        if (curveType == AnimCurveType_ParameterizedAnim) {
            value = GetParameterizedAnimValue(
                frame, GetTexSrtElement(pMaterial, pTarget->id, pTarget->target),
                static_cast<const ResParameterizedAnim*>(pCurve));
        } else {
            value = GetHermiteCurveValue(frame, static_cast<const ResHermiteKey*>(pCurve),
                                         pTarget->keyCount);
        }

        SetTexSrtElement(pMaterial, pTarget->id, pTarget->target, value);
    }
}

/**
 * @brief Animates the vector graphics textures of a material.
 * @param pMaterial Material to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateVectorGraphics(Material* pMaterial, const ResAnimationInfo* pInfo,
                                  const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const float value =
            GetHermiteCurveValue(frame, GetKeys<ResHermiteKey>(pTarget), pTarget->keyCount);
        SetVectorGraphicsElement(pMaterial, pTarget->id, pTarget->target, value);
    }
}

/**
 * @brief Animates the textures of a material.
 * @param pMaterial Material to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 * @param pTextures Textures of the animation.
 */
inline void AnimateTexturePattern(Material* pMaterial, const ResAnimationInfo* pInfo,
                                  const u32* pTargetOffsets, float frame,
                                  const TextureInfo** pTextures) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const u16 fileIndex =
            GetStepCurveValue(frame, GetKeys<ResStepKey>(pTarget), pTarget->keyCount);
        if (pTextures[fileIndex]->IsValid()) {
            pMaterial->SetTextureInfo(pTarget->id, pTextures[fileIndex]);
        }
    }
}

/**
 * @brief Animates the colors of a material.
 * @param pMaterial Material to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateMaterialColor(Material* pMaterial, const ResAnimationInfo* pInfo,
                                 const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const u8 curveType = pTarget->curveType;
        const void* pCurve = GetKeys<void>(pTarget);
        float value;
        if (curveType == AnimCurveType_ParameterizedAnim) {
            value = GetParameterizedAnimValue(frame, pMaterial->GetColorElement(pTarget->target),
                                              static_cast<const ResParameterizedAnim*>(pCurve));
        } else {
            value = GetHermiteCurveValue(frame, static_cast<const ResHermiteKey*>(pCurve),
                                         pTarget->keyCount);
        }

        const int target = pTarget->target;
        if (target <= AnimTargetMatColorMax) {
            pMaterial->SetColorElement(target, static_cast<int>(ClampColorElement(value)));
        } else {
            const u32 colorIndex = static_cast<u32>((target - AnimTargetMatColorMax) / 4);
            if (colorIndex < MaterialColor_Max) {
                switch (target % 4) {
                case 0:
                    pMaterial->m_pFloatColors[colorIndex].x = value;
                    break;
                case 1:
                    pMaterial->m_pFloatColors[colorIndex].y = value;
                    break;
                case 2:
                    pMaterial->m_pFloatColors[colorIndex].z = value;
                    break;
                case 3:
                    pMaterial->m_pFloatColors[colorIndex].w = value;
                    break;
                default:
                    break;
                }
            }
        }
    }
}

/**
 * @brief Animates the font shadow colors of a material.
 * @param pMaterial Material to animate.
 * @param pInfo Animation info.
 * @param pTargetOffsets Offsets of the targets of the info.
 * @param frame Current frame.
 */
inline void AnimateFontShadow(Material* pMaterial, const ResAnimationInfo* pInfo,
                              const u32* pTargetOffsets, float frame) {
    for (int i = 0; i < pInfo->count; ++i) {
        const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, i);
        const float value =
            GetHermiteCurveValue(frame, GetKeys<ResHermiteKey>(pTarget), pTarget->keyCount);
        SetFontShadowElement(pMaterial, pTarget->target,
                             static_cast<int>(ClampColorElement(value)));
    }
}
}  // namespace

/** @brief Constructs a transform without resource. */
AnimTransform::AnimTransform() : m_pResource(nullptr), mFrame(0), mEnabled(true) {}

/** @brief Destroys the transform. */
AnimTransform::~AnimTransform() = default;

/** @return Number of frames of the animation. */
u16 AnimTransform::GetFrameSize() const {
    return m_pResource->frameSize;
}

/**
 * @brief Advances the frame; the base transform has no playback behavior.
 * @param step Number of frames to advance.
 */
void AnimTransform::UpdateFrame(float step) {}

/**
 * @brief Enables or disables the animation.
 * @param enabled Whether the transform applies its values.
 */
void AnimTransform::SetEnabled(bool enabled) {
    mEnabled = enabled;
}

/** @return Whether the animation loops. */
bool AnimTransform::IsLoopData() const {
    return m_pResource->loop != 0;
}

/** @return Whether the animation has no frames. */
bool AnimTransform::IsWaitData() const {
    return m_pResource->frameSize == 0;
}

/** @brief Constructs a transform without bindings. */
AnimTransformBasic::AnimTransformBasic() : _28(nullptr), _30(nullptr), _38(0) {}

/** @brief Releases the texture and bind arrays. */
AnimTransformBasic::~AnimTransformBasic() {
    if (_30 != nullptr) {
        Layout::FreeMemory(_30);
    }

    if (_28 != nullptr) {
        Layout::FreeMemory(_28);
    }
}

/**
 * @brief Sets the resource with as many binds as the resource has contents.
 * @param pDevice Graphics device.
 * @param pAccessor Accessor resolving the textures.
 * @param pResource Animation block.
 */
void AnimTransformBasic::SetResource(nn::gfx::Device* pDevice, ResourceAccessor* pAccessor,
                                     const ResAnimationBlock* pResource) {
    SetResource(pDevice, pAccessor, pResource, pResource->contentCount);
}

/** @brief Releases the resource and the arrays built from it. */
void AnimTransformBasic::ResetAnimResource() {
    m_pResource = nullptr;

    if (_30 != nullptr) {
        Layout::FreeMemory(_30);
    }

    _30 = nullptr;

    if (_28 != nullptr) {
        Layout::FreeMemory(_28);
    }

    _28 = nullptr;
}

/**
 * @brief Sets the resource and allocates the texture and bind arrays.
 * @param pDevice Graphics device.
 * @param pAccessor Accessor resolving the textures.
 * @param pResource Animation block.
 * @param capacity Maximum number of binds.
 */
void AnimTransformBasic::SetResource(nn::gfx::Device* pDevice, ResourceAccessor* pAccessor,
                                     const ResAnimationBlock* pResource, u16 capacity) {
    m_pResource = pResource;
    m_pFileResArray = nullptr;

    if (pResource->textureCount != 0) {
        m_pFileResArray = Layout::NewArray<const TextureInfo*>(pResource->textureCount);
        if (m_pFileResArray != nullptr) {
            const u32* pNameOffsets = reinterpret_cast<const u32*>(pResource + 1);
            for (int i = 0; i < pResource->textureCount; ++i) {
                const char* pName = reinterpret_cast<const char*>(pNameOffsets) + pNameOffsets[i];
                m_pFileResArray[i] = pAccessor->AcquireTexture(pDevice, pName);
            }
        }
    }

    m_pBindPairArray = Layout::NewArray<BindPair>(capacity);
    if (m_pBindPairArray != nullptr) {
        mBindCapacity = capacity;
    }
}

/**
 * @brief Binds the contents of the resource to a pane and its descendants.
 * @param pPane Pane to bind.
 * @param recursive Whether descendants are searched.
 */
void AnimTransformBasic::BindPane(Pane* pPane, bool recursive) {
    const ResAnimationBlock* pResource = m_pResource;
    const u32* pOffsets = GetAnimContentOffsets(pResource);
    for (int i = 0; i < pResource->contentCount; ++i) {
        const ResAnimationContent* pContent = GetAnimContent(pResource, pOffsets, i);
        switch (pContent->type) {
        case AnimContentType_Pane:
        case AnimContentType_StateMachine:
        case AnimContentType_PartsStateLayer: {
            Pane* pTarget = pPane->FindPaneByName(pContent->name, recursive);
            if (pTarget != nullptr) {
                BindPaneImpl(pTarget, pContent);
            }

            break;
        }
        case AnimContentType_Material: {
            Material* pTarget = pPane->FindMaterialByName(pContent->name, recursive);
            if (pTarget != nullptr) {
                BindMaterialImpl(pTarget, pContent);
            }

            break;
        }
        case AnimContentType_ExtUserData:
            BindExtUserDataToPane(pPane->FindPaneByName(pContent->name, recursive), *pContent);
            break;
        default:
            break;
        }
    }
}

/**
 * @brief Checks whether a content is already bound to a target.
 * @param pTarget Target object.
 * @param pContent Animation content.
 * @return Always false; the check is only made in debug builds.
 */
bool AnimTransformBasic::CheckBindAnimationDoubly(const void* pTarget,
                                                  const ResAnimationContent* pContent) const {
    return false;
}

/**
 * @brief Binds a content to a pane.
 * @param pTarget Pane to animate.
 * @param pContent Animation content.
 * @return Whether there was room for the bind.
 */
bool AnimTransformBasic::BindPaneImpl(Pane* pTarget, const ResAnimationContent* pContent) {
    if (mBindCount >= mBindCapacity) {
        return false;
    }

    if (CheckBindAnimationDoubly(pTarget, pContent)) {
        return false;
    }

    BindPair& rPair = m_pBindPairArray[mBindCount];
    ++mBindCount;
    rPair.pTarget = pTarget;
    rPair.pAnimContent = pContent;
    return true;
}

/**
 * @brief Binds a content to a material when the material can play all of its targets.
 * @param pTarget Material to animate.
 * @param pContent Animation content.
 * @return Whether there was room for the bind.
 */
bool AnimTransformBasic::BindMaterialImpl(Material* pTarget, const ResAnimationContent* pContent) {
    if (mBindCount >= mBindCapacity) {
        return false;
    }

    if (CheckBindAnimationDoubly(pTarget, pContent)) {
        return false;
    }

    if (pContent != nullptr) {
        int isBindable = 1;
        const u32* pInfoOffsets = GetAnimInfoOffsets(*pContent);
        for (int i = 0; i < pContent->count; ++i) {
            const ResAnimationInfo* pInfo = GetAnimInfo(*pContent, pInfoOffsets, i);
            const u32* pTargetOffsets = GetAnimTargetOffsets(pInfo);
            for (int j = 0; j < pInfo->count; ++j) {
                const ResAnimationTarget* pAnimTarget = GetAnimTarget(pInfo, pTargetOffsets, j);
                switch (pInfo->kind) {
                case AnimInfoKind_AlphaCompare:
                    if (pTarget->m_MemCap.alpComp == 0) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->target != 0) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->curveType != AnimCurveType_Hermite) {
                        isBindable = 0;
                    }

                    break;
                case AnimInfoKind_BrickRepeat:
                    if (pAnimTarget->id >= TexMapMax) {
                        isBindable = 0;
                    }

                    if (pTarget->GetBrickRepeatCap() <= pAnimTarget->id) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->target > 6) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->curveType != AnimCurveType_Hermite) {
                        isBindable = 0;
                    }

                    break;
                case AnimInfoKind_IndirectSrt:
                    if (pTarget->m_MemCap.indirectParameter == 0) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->target > 2) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->curveType != AnimCurveType_Hermite) {
                        isBindable = 0;
                    }

                    break;
                case AnimInfoKind_TextureSrt:
                    if (pAnimTarget->id >= TexMapMax) {
                        isBindable = 0;
                    }

                    if (pTarget->GetTexSrtCap() <= pAnimTarget->id) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->target > 4) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->curveType != AnimCurveType_Hermite) {
                        isBindable = 0;
                    }

                    break;
                case AnimInfoKind_VectorGraphics:
                    if (pAnimTarget->id >= TexMapMax) {
                        isBindable = 0;
                    }

                    if (pTarget->m_MemCap.vectorGraphicsTexture <= pAnimTarget->id) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->target != 0) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->curveType != AnimCurveType_Hermite) {
                        isBindable = 0;
                    }

                    break;
                case AnimInfoKind_TexturePattern:
                    if (m_pFileResArray == nullptr) {
                        isBindable = 0;
                    }

                    if (pTarget->GetTexMapNum() <= pAnimTarget->id) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->curveType != AnimCurveType_Step) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->target != 0) {
                        isBindable = 0;
                    }

                    for (int k = 0; k < m_pResource->textureCount; ++k) {
                        if (!m_pFileResArray[k]->IsValid()) {
                            isBindable = 0;
                            break;
                        }
                    }

                    break;
                case AnimInfoKind_MaterialColor:
                    if (pAnimTarget->target >= 0x38) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->curveType != AnimCurveType_Hermite) {
                        isBindable = 0;
                    }

                    break;
                case AnimInfoKind_FontShadow:
                    if (pTarget->m_MemCap.fontShadowParameter == 0) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->target > 6) {
                        isBindable = 0;
                    }

                    if (pAnimTarget->curveType != AnimCurveType_Hermite) {
                        isBindable = 0;
                    }

                    break;
                default:
                    break;
                }
            }
        }

        if (!isBindable) {
            return true;
        }
    }

    BindPair& rPair = m_pBindPairArray[mBindCount];
    ++mBindCount;
    rPair.pTarget = pTarget;
    rPair.pAnimContent = pContent;
    return true;
}

/**
 * @brief Binds a content to the extended user data of a pane it names.
 * @param pPane Pane owning the extended user data, or nullptr.
 * @param rContent Animation content.
 * @return False when there was no room for the bind.
 */
bool AnimTransformBasic::BindExtUserDataToPane(Pane* pPane, const ResAnimationContent& rContent) {
    if (pPane == nullptr) {
        return true;
    }

    const ResExtUserData* pExtUserData =
        pPane->FindExtUserDataByName(GetExtUserDataTargetName(rContent));
    if (pExtUserData == nullptr) {
        return true;
    }

    return BindExtUserDataImpl(const_cast<ResExtUserData*>(pExtUserData), &rContent);
}

/**
 * @brief Binds the contents that target the panes of a group.
 * @param pGroup Group to bind.
 */
void AnimTransformBasic::BindGroup(Group* pGroup) {
    const ResAnimationBlock* pResource = m_pResource;
    const u32* pOffsets = GetAnimContentOffsets(pResource);
    for (int i = 0; i < pResource->contentCount; ++i) {
        const ResAnimationContent* pContent = GetAnimContent(pResource, pOffsets, i);
        switch (pContent->type) {
        case AnimContentType_Pane:
        case AnimContentType_StateMachine:
        case AnimContentType_PartsStateLayer:
            for (auto& rLink : pGroup->mPanes) {
                if (EqualsResName(pContent->name, rLink.pane->GetName())) {
                    BindPaneImpl(rLink.pane, pContent);
                    break;
                }
            }

            break;
        case AnimContentType_Material:
            for (auto& rLink : pGroup->mPanes) {
                Material* pMaterial = rLink.pane->FindMaterialByName(pContent->name, false);
                if (pMaterial != nullptr) {
                    BindMaterialImpl(pMaterial, pContent);
                    break;
                }
            }

            break;
        case AnimContentType_ExtUserData:
            for (auto& rLink : pGroup->mPanes) {
                if (EqualsResName(pContent->name, rLink.pane->GetName())) {
                    const ResExtUserData* pExtUserData =
                        rLink.pane->FindExtUserDataByName(GetExtUserDataTargetName(*pContent));
                    if (pExtUserData != nullptr) {
                        BindExtUserDataImpl(const_cast<ResExtUserData*>(pExtUserData), pContent);
                    }

                    break;
                }
            }

            break;
        default:
            break;
        }
    }
}

/**
 * @brief Binds the contents that target a material.
 * @param pMaterial Material to bind.
 */
void AnimTransformBasic::BindMaterial(Material* pMaterial) {
    const ResAnimationBlock* pResource = m_pResource;
    const u32* pOffsets = GetAnimContentOffsets(pResource);
    for (int i = 0; i < pResource->contentCount; ++i) {
        const ResAnimationContent* pContent = GetAnimContent(pResource, pOffsets, i);
        if (pContent->type == AnimContentType_Material &&
            EqualsMaterialName(pMaterial->GetName(), pContent->name)) {
            if (!BindMaterialImpl(pMaterial, pContent)) {
                break;
            }
        }
    }
}

/**
 * @brief Binds the contents that target a source pane to another pane.
 * @param pPane Pane to animate.
 * @param pSource Pane whose name and materials select the contents.
 */
void AnimTransformBasic::ForceBindPane(Pane* pPane, const Pane* pSource) {
    const ResAnimationBlock* pResource = m_pResource;
    const u32* pOffsets = GetAnimContentOffsets(pResource);
    for (int i = 0; i < pResource->contentCount; ++i) {
        const ResAnimationContent* pContent = GetAnimContent(pResource, pOffsets, i);
        switch (pContent->type) {
        case AnimContentType_Pane:
            if (EqualsResName(pSource->GetName(), pContent->name)) {
                if (!BindPaneImpl(pPane, pContent)) {
                    return;
                }
            }

            break;
        case AnimContentType_Material: {
            const u8 materialCount = pSource->GetMaterialCount();
            for (u32 j = 0; j < materialCount; ++j) {
                const Material* pSourceMaterial = pSource->GetMaterial(j);
                if (pSourceMaterial != nullptr &&
                    EqualsMaterialName(pSourceMaterial->GetName(), pContent->name)) {
                    Material* pMaterial = pPane->GetMaterial(j);
                    if (pMaterial != nullptr) {
                        if (!BindMaterialImpl(pMaterial, pContent)) {
                            return;
                        }
                    }
                }
            }

            break;
        }
        case AnimContentType_ExtUserData:
            if (EqualsResName(pSource->GetName(), pContent->name)) {
                if (!BindExtUserDataToPane(pPane, *pContent)) {
                    return;
                }
            }

            break;
        default:
            break;
        }
    }
}

/**
 * @brief Binds a content to an extended user data.
 * @param pTarget Extended user data to animate.
 * @param pContent Animation content.
 * @return Whether there was room for the bind.
 */
bool AnimTransformBasic::BindExtUserDataImpl(ResExtUserData* pTarget,
                                             const ResAnimationContent* pContent) {
    if (mBindCount >= mBindCapacity) {
        return false;
    }

    if (CheckBindAnimationDoubly(pTarget, pContent)) {
        return false;
    }

    BindPair& rPair = m_pBindPairArray[mBindCount];
    ++mBindCount;
    rPair.pTarget = pTarget;
    rPair.pAnimContent = pContent;
    return true;
}

/**
 * @brief Unbinds a pane, its extended user data and its materials.
 * @param pPane Pane to unbind.
 */
void AnimTransformBasic::UnbindPane(const Pane* pPane) {
    const int bindCount = mBindCount;
    for (int i = 0; i < bindCount; ++i) {
        const BindPair& rPair = m_pBindPairArray[i];
        if (rPair.pTarget == pPane) {
            EraseBindPair(i);
            break;
        }

        const u16 extUserDataCount = pPane->GetExtUserDataCount();
        for (u32 j = 0; j < extUserDataCount; ++j) {
            if (&pPane->GetExtUserDataArray()[j] == rPair.pTarget) {
                EraseBindPair(i);
                break;
            }
        }
    }

    const u8 materialCount = pPane->GetMaterialCount();
    for (u32 i = 0; i < materialCount; ++i) {
        const Material* pMaterial = pPane->GetMaterial(i);
        if (pMaterial != nullptr) {
            UnbindMaterial(pMaterial);
        }
    }
}

/**
 * @brief Removes a bind by moving the last bind into its place.
 * @param index Index of the bind.
 */
void AnimTransformBasic::EraseBindPair(int index) {
    if (index + 1 < mBindCount) {
        m_pBindPairArray[index] = m_pBindPairArray[mBindCount - 1];
    }

    --mBindCount;
}

/**
 * @brief Unbinds the panes of a group.
 * @param pGroup Group to unbind.
 */
void AnimTransformBasic::UnbindGroup(const Group* pGroup) {
    for (const auto& rLink : pGroup->mPanes) {
        UnbindPane(rLink.pane);
    }
}

/**
 * @brief Unbinds a material.
 * @param pMaterial Material to unbind.
 */
void AnimTransformBasic::UnbindMaterial(const Material* pMaterial) {
    for (int i = 0; i < mBindCount; ++i) {
        if (m_pBindPairArray[i].pTarget == pMaterial) {
            EraseBindPair(i);
            break;
        }
    }
}

/** @brief Removes every bind. */
void AnimTransformBasic::UnbindAll() {
    mBindCount = 0;
}

/** @brief Applies the animation to every bound target. */
void AnimTransformBasic::Animate() {
    if (!mEnabled) {
        return;
    }

    const int bindCount = mBindCount;
    for (int i = 0; i < bindCount; ++i) {
        const BindPair& rPair = m_pBindPairArray[i];
        switch (rPair.pAnimContent->type) {
        case AnimContentType_Pane:
        case AnimContentType_StateMachine:
            AnimatePaneImpl(static_cast<Pane*>(rPair.pTarget), rPair.pAnimContent);
            break;
        case AnimContentType_Material:
            AnimateMaterialImpl(static_cast<Material*>(rPair.pTarget), rPair.pAnimContent);
            break;
        case AnimContentType_ExtUserData:
            AnimateExtUserDataImpl(static_cast<ResExtUserData*>(rPair.pTarget),
                                   rPair.pAnimContent);
            break;
        case AnimContentType_PartsStateLayer:
            AnimatePartsStateLayerImpl(static_cast<Pane*>(rPair.pTarget), rPair.pAnimContent);
            break;
        default:
            break;
        }
    }
}

/**
 * @brief Applies the animation to a pane and its materials.
 * @param pPane Pane to animate.
 */
void AnimTransformBasic::AnimatePane(Pane* pPane) {
    if (!mEnabled) {
        return;
    }

    for (int i = 0; i < mBindCount; ++i) {
        const BindPair& rPair = m_pBindPairArray[i];
        if (rPair.pTarget == pPane) {
            AnimatePaneImpl(pPane, rPair.pAnimContent);

            const u8 materialCount = pPane->GetMaterialCount();
            for (u32 j = 0; j < materialCount; ++j) {
                Material* pMaterial = pPane->GetMaterial(j);
                if (pMaterial != nullptr) {
                    AnimateMaterial(pMaterial);
                }
            }

            break;
        }
    }
}

/**
 * @brief Applies the animation to a material.
 * @param pMaterial Material to animate.
 */
void AnimTransformBasic::AnimateMaterial(Material* pMaterial) {
    if (!mEnabled) {
        return;
    }

    for (int i = 0; i < mBindCount; ++i) {
        const BindPair& rPair = m_pBindPairArray[i];
        if (rPair.pTarget == pMaterial) {
            AnimateMaterialImpl(pMaterial, rPair.pAnimContent);
            break;
        }
    }
}

/**
 * @brief Applies the pane animations of a content.
 * @param pPane Pane to animate.
 * @param pContent Animation content.
 */
void AnimTransformBasic::AnimatePaneImpl(Pane* pPane, const ResAnimationContent* pContent) {
    const u32* pInfoOffsets = GetAnimInfoOffsets(*pContent);
    for (int i = 0; i < pContent->count; ++i) {
        const ResAnimationInfo* pInfo = GetAnimInfo(*pContent, pInfoOffsets, i);
        const u32* pTargetOffsets = GetAnimTargetOffsets(pInfo);
        switch (pInfo->kind) {
        case AnimInfoKind_MaskTexSrt:
            AnimateMaskTexSrt(pPane, pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_PaneSrt:
            AnimatePaneSrt(pPane, pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_Visibility:
            AnimateVisibility(pPane, pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_DropShadow:
            AnimateDropShadow(pPane, pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_StateMachineEvent:
            AnimateStateMachineEvent(pPane, pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_VertexColor:
            AnimateVertexColor(pPane, pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_ProceduralShape:
            AnimateProceduralShape(pPane, pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_WindowFrameSize:
            AnimateWindowFrameSize(static_cast<Window*>(pPane), pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_PerCharacterTransform:
            AnimatePerCharacterTransform(static_cast<TextBox*>(pPane), pInfo, pTargetOffsets,
                                         mFrame);
            break;
        default:
            break;
        }
    }
}

/**
 * @brief Applies the material animations of a content.
 * @param pMaterial Material to animate.
 * @param pContent Animation content.
 */
void AnimTransformBasic::AnimateMaterialImpl(Material* pMaterial,
                                             const ResAnimationContent* pContent) {
    const u32* pInfoOffsets = GetAnimInfoOffsets(*pContent);
    for (int i = 0; i < pContent->count; ++i) {
        const ResAnimationInfo* pInfo = GetAnimInfo(*pContent, pInfoOffsets, i);
        const u32* pTargetOffsets = GetAnimTargetOffsets(pInfo);
        switch (pInfo->kind) {
        case AnimInfoKind_AlphaCompare:
            AnimateAlphaCompare(pMaterial, pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_BrickRepeat:
            AnimateBrickRepeat(pMaterial, pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_IndirectSrt:
            AnimateIndirectSrt(pMaterial, pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_TextureSrt:
            AnimateTextureSrt(pMaterial, pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_VectorGraphics:
            AnimateVectorGraphics(pMaterial, pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_TexturePattern:
            AnimateTexturePattern(pMaterial, pInfo, pTargetOffsets, mFrame, m_pFileResArray);
            break;
        case AnimInfoKind_MaterialColor:
            AnimateMaterialColor(pMaterial, pInfo, pTargetOffsets, mFrame);
            break;
        case AnimInfoKind_FontShadow:
            AnimateFontShadow(pMaterial, pInfo, pTargetOffsets, mFrame);
            break;
        default:
            break;
        }
    }
}

/**
 * @brief Applies the animations of a content to an extended user data.
 * @param pExtUserData Extended user data to animate.
 * @param pContent Animation content.
 */
void AnimTransformBasic::AnimateExtUserDataImpl(ResExtUserData* pExtUserData,
                                                const ResAnimationContent* pContent) {
    const u32* pInfoOffsets = GetAnimInfoOffsets(*pContent);
    for (int i = 0; i < pContent->count; ++i) {
        const ResAnimationInfo* pInfo = GetAnimInfo(*pContent, pInfoOffsets, i);
        AnimateExtUserData(pExtUserData, pInfo, GetAnimTargetOffsets(pInfo), mFrame);
    }
}

/**
 * @brief Applies the animations of a content to the state machine variable of a parts pane.
 * @param pPane Parts pane owning the state machine.
 * @param pContent Animation content.
 */
void AnimTransformBasic::AnimatePartsStateLayerImpl(Pane* pPane,
                                                    const ResAnimationContent* pContent) {
    const char* pVariableName = GetExtUserDataTargetName(*pContent);
    if (strnlen(pVariableName, 32) == 0) {
        return;
    }

    Parts* pParts = DynamicCast<Parts*>(pPane);
    const StateMachineSystemData* pData =
        GetStateMachineSystemData(pParts->m_pLayout->GetRootPane());
    StateMachine* pStateMachine = pData != nullptr ? pData->pStateMachine : nullptr;

    const u32* pInfoOffsets = GetAnimInfoOffsets(*pContent);
    for (int i = 0; i < pContent->count; ++i) {
        const ResAnimationInfo* pInfo = GetAnimInfo(*pContent, pInfoOffsets, i);
        const u32* pTargetOffsets = GetAnimTargetOffsets(pInfo);
        for (int j = 0; j < pInfo->count; ++j) {
            const ResAnimationTarget* pTarget = GetAnimTarget(pInfo, pTargetOffsets, j);
            const u8 curveType = pTarget->curveType;
            const void* pCurve = GetKeys<void>(pTarget);
            float value;
            if (curveType == AnimCurveType_ParameterizedAnim) {
                const float current =
                    pStateMachine->m_VariableManager.FindRefOnlyByName_(pVariableName)->value;
                value = GetParameterizedAnimValue(
                    mFrame, current, static_cast<const ResParameterizedAnim*>(pCurve));
            } else {
                value = GetHermiteCurveValue(mFrame, static_cast<const ResHermiteKey*>(pCurve),
                                             pTarget->keyCount);
            }

            SetStateMachineFloatValue(pStateMachine, pVariableName, value);
        }
    }
}

/** @brief Initializes the resource to no data. */
void AnimResource::Initialize() {
    mFile = nullptr;
    mAnimation = nullptr;
    mTag = nullptr;
    mSharedAnimations = nullptr;
}

/** @return Whether the resource has an animation block. */
bool AnimResource::CheckResource() const {
    return mAnimation != nullptr;
}

/**
 * @brief Sets the animation file and finds its blocks.
 * @param pResource Animation file.
 */
void AnimResource::Set(const void* pResource) {
    Initialize();

    const ResAnimationFileHeader* pFileHeader =
        static_cast<const ResAnimationFileHeader*>(pResource);
    CheckSignature(pFileHeader->signature, AnimationFileSignature);
    mFile = pResource;

    const u8* pBlock = static_cast<const u8*>(pResource) + pFileHeader->headerSize;
    for (int i = 0; i < pFileHeader->dataBlocks; ++i) {
        const ResAnimationBlockHeader* pBlockHeader =
            reinterpret_cast<const ResAnimationBlockHeader*>(pBlock);
        switch (pBlockHeader->kind) {
        case AnimationBlockSignature_Tag:
            mTag = reinterpret_cast<const ResAnimationTagBlock*>(pBlock);
            break;
        case AnimationBlockSignature_Share:
            mSharedAnimations = reinterpret_cast<const ResAnimationShareBlock*>(pBlock);
            break;
        case AnimationBlockSignature_Animation:
            mAnimation = reinterpret_cast<const ResAnimationBlock*>(pBlock);
            break;
        default:
            break;
        }

        pBlock += pBlockHeader->size;
    }
}

/** @return Order of the animation tag, or 0xffff without tag. */
u16 AnimResource::GetTagOrder() const {
    return mTag != nullptr ? mTag->order : 0xffff;
}

/** @return Name of the animation tag, or nullptr without tag. */
const char* AnimResource::GetTagName() const {
    return mTag != nullptr ? reinterpret_cast<const char*>(mTag) + mTag->nameOffset : nullptr;
}

/** @return Number of groups the animation tag binds. */
u16 AnimResource::GetGroupCount() const {
    return mTag != nullptr ? mTag->groupCount : 0;
}

/** @return Groups the animation tag binds, or nullptr without tag. */
const ResAnimationGroup* AnimResource::GetGroupArray() const {
    return mTag != nullptr ? reinterpret_cast<const ResAnimationGroup*>(
                                 reinterpret_cast<const char*>(mTag) + mTag->groupOffset) :
                             nullptr;
}

/** @return Extended user data of the animation tag, or nullptr. */
const ResExtUserDataList* AnimResource::GetExtUserDataList() const {
    if (mTag == nullptr) {
        return nullptr;
    }

    if (!mTag->userDataOffset) {
        return nullptr;
    }

    return reinterpret_cast<const ResExtUserDataList*>(reinterpret_cast<const char*>(mTag) +
                                                       mTag->userDataOffset);
}

/** @return Whether the groups are bound with their descendants. */
bool AnimResource::IsDescendingBind() const {
    return mTag != nullptr ? (mTag->flags & 1) != 0 : false;
}

/** @return Number of animation share infos. */
u16 AnimResource::GetAnimationShareInfoCount() const {
    return mSharedAnimations != nullptr ? mSharedAnimations->infoCount : 0;
}

/** @return Animation share infos, or nullptr. */
const ResAnimationShareInfo* AnimResource::GetAnimationShareInfoArray() const {
    return mSharedAnimations != nullptr ?
               reinterpret_cast<const ResAnimationShareInfo*>(
                   reinterpret_cast<const char*>(mSharedAnimations) +
                   mSharedAnimations->infoOffset) :
               nullptr;
}

/**
 * @brief Counts the contents that find their target from a pane.
 * @param pPane Pane to search.
 * @param isDescendingBind Whether descendants are searched.
 * @return Number of contents with a target.
 */
int AnimResource::CalculateAnimationCount(Pane* pPane, bool isDescendingBind) const {
    if (mAnimation == nullptr) {
        return 0;
    }

    int count = 0;
    const u32* pOffsets = GetAnimContentOffsets(mAnimation);
    for (int i = 0; i < mAnimation->contentCount; ++i) {
        const ResAnimationContent* pContent = GetAnimContent(mAnimation, pOffsets, i);
        if (pContent->type == AnimContentType_Pane) {
            if (pPane->FindPaneByName(pContent->name, isDescendingBind) != nullptr) {
                ++count;
            }
        } else {
            if (pPane->FindMaterialByName(pContent->name, isDescendingBind) != nullptr) {
                ++count;
            }
        }
    }

    return count;
}

/**
 * @brief Counts the contents that target a material.
 * @param pMaterial Material to check.
 * @return 1 when a content targets the material, otherwise 0.
 */
int AnimResource::CalculateAnimationCount(Material* pMaterial) const {
    if (mAnimation == nullptr) {
        return 0;
    }

    const u32* pOffsets = GetAnimContentOffsets(mAnimation);

    for (int i = 0; i < mAnimation->contentCount; ++i) {
        const ResAnimationContent* pContent = GetAnimContent(mAnimation, pOffsets, i);
        if (pContent->type == AnimContentType_Material &&
            EqualsMaterialName(pMaterial->GetName(), pContent->name)) {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Counts the contents that find their target from the panes of a group.
 * @param pGroup Group to search.
 * @param isDescendingBind Whether descendants are searched.
 * @return Number of contents with a target.
 */
int AnimResource::CalculateAnimationCount(Group* pGroup, bool isDescendingBind) const {
    int count = 0;
    for (auto& rLink : pGroup->mPanes) {
        count += CalculateAnimationCount(rLink.pane, isDescendingBind);
    }

    return count;
}

namespace detail {
/** @brief Constructs an empty pane tree. */
AnimPaneTree::AnimPaneTree() {
    m_AnimRes.Initialize();
    Initialize();
}

/** @brief Clears the contents of the pane tree. */
void AnimPaneTree::Initialize() {
    m_pPaneAnimContent = nullptr;
    for (int i = 0; i < MaterialMax; ++i) {
        m_pMatAnimContents[i] = nullptr;
    }

    m_AnimCount = 0;
    m_AnimMatCount = 0;
}

/**
 * @brief Constructs the pane tree of a source pane.
 * @param pTargetPane Source pane.
 * @param rResource Animation resource.
 */
AnimPaneTree::AnimPaneTree(Pane* pTargetPane, const AnimResource& rResource) {
    m_AnimRes.Initialize();
    Initialize();
    Set(pTargetPane, rResource);
}

/**
 * @brief Finds the contents that target a source pane and its materials.
 * @param pTargetPane Source pane.
 * @param rResource Animation resource.
 */
void AnimPaneTree::Set(Pane* pTargetPane, const AnimResource& rResource) {
    const ResAnimationBlock* pAnimBlock = rResource.mAnimation;
    u16 animCount = 0;

    const ResAnimationContent* pPaneAnimContent =
        FindAnimContent(pAnimBlock, pTargetPane->GetName(), AnimContentType_Pane);
    if (pPaneAnimContent != nullptr) {
        ++animCount;
    }

    const u32 materialCount = static_cast<u8>(pTargetPane->GetMaterialCount());
    const ResAnimationContent* pMatAnimContents[MaterialMax];
    for (int i = 0; i < materialCount; ++i) {
        pMatAnimContents[i] = FindAnimContent(pAnimBlock, pTargetPane->GetMaterial(i)->GetName(),
                                              AnimContentType_Material);
        if (pMatAnimContents[i] != nullptr) {
            ++animCount;
        }
    }

    if (animCount == 0) {
        return;
    }

    m_AnimRes = rResource;
    m_pPaneAnimContent = pPaneAnimContent;
    m_AnimMatCount = materialCount;
    if (materialCount != 0) {
        std::memcpy(m_pMatAnimContents, pMatAnimContents,
                    sizeof(const ResAnimationContent*) * materialCount);
    }

    m_AnimCount = animCount;
}

/**
 * @brief Finds a content by type and name.
 * @param pAnimBlock Animation block.
 * @param pName Name of the target.
 * @param contentType Type of the content.
 * @return The content, or nullptr.
 */
const ResAnimationContent* AnimPaneTree::FindAnimContent(const ResAnimationBlock* pAnimBlock,
                                                         const char* pName, u8 contentType) {
    const u32* pOffsets = GetAnimContentOffsets(pAnimBlock);
    for (int i = 0; i < pAnimBlock->contentCount; ++i) {
        const ResAnimationContent* pContent = GetAnimContent(pAnimBlock, pOffsets, i);
        if (pContent->type == contentType && EqualsMaterialName(pContent->name, pName)) {
            return pContent;
        }
    }

    return nullptr;
}

/**
 * @brief Creates a transform playing the contents of the pane tree on another pane.
 * @param pDevice Graphics device.
 * @param pLayout Layout creating the transform.
 * @param pTargetPane Pane to animate.
 * @param pResourceAccessor Accessor resolving the textures.
 * @return The new transform.
 */
AnimTransformBasic* AnimPaneTree::Bind(nn::gfx::Device* pDevice, Layout* pLayout,
                                       Pane* pTargetPane,
                                       ResourceAccessor* pResourceAccessor) const {
    AnimTransformBasic* pAnimTrans = pLayout->CreateAnimTransformBasic();
    pAnimTrans->SetResource(pDevice, pResourceAccessor, m_AnimRes.mAnimation, m_AnimCount);

    if (m_pPaneAnimContent != nullptr) {
        pAnimTrans->BindPaneImpl(pTargetPane, m_pPaneAnimContent);
    }

    const u8 materialCount = std::min<u8>(m_AnimMatCount, pTargetPane->GetMaterialCount());
    for (u8 i = 0; i < materialCount; ++i) {
        if (m_pMatAnimContents[i] != nullptr) {
            Material* pMaterial = pTargetPane->GetMaterial(i);
            if (!pAnimTrans->BindMaterialImpl(pMaterial, m_pMatAnimContents[i])) {
                break;
            }
        }
    }

    return pAnimTrans;
}
}  // namespace detail

/**
 * @brief Gets an element of a material color.
 * @param colorType Animation target of the element.
 * @return The element, or 0xff when the material has no such color.
 */
inline int Material::GetColorElement(int colorType) const {
    if (colorType < AnimTargetMatColorMax) {
        const int colorIndex = colorType / 4;
        if (static_cast<u32>(colorIndex) < MaterialColor_Max) {
            const u32 index = colorIndex;
            switch (colorType % 4) {
            case 0:
                return m_ByteColors[index].v[0];
            case 1:
                return m_ByteColors[index].v[1];
            case 2:
                return m_ByteColors[index].v[2];
            case 3:
                return m_ByteColors[index].v[3];
            default:
                return 0;
            }
        }

        if (static_cast<u32>(colorIndex) < MaterialColor_Max + ConstantColorMax) {
            if (m_MemCap.detailedCombinerParameter != 0) {
                return GetElement(ToPackedColor(GetConstantColor(colorIndex - MaterialColor_Max)),
                                  colorType % 4);
            }

            if (m_MemCap.combinerUserShaderParameter != 0) {
                const u8 constantIndex = colorIndex - MaterialColor_Max;
                const u32 color = *reinterpret_cast<const u32*>(
                    GetCombinerUserShader(this)->constantColor[constantIndex % ConstantColorMax]);
                return GetElement(color, colorType % 4);
            }
        }
    }

    return 0xff;
}

/**
 * @brief Gets a constant color of the combiner.
 * @param index Index of the constant color.
 * @return The color packed as RGBA bytes, or white when the material has no combiner constant
 *         colors.
 */
inline nn::util::Unorm8x4 Material::GetConstantColor(int index) const {
    u32 color = 0xffffffff;
    if (m_MemCap.detailedCombinerParameter != 0 && index < ConstantColorMax) {
        color = *reinterpret_cast<const u32*>(
            &GetDetailedCombinerStageInfo(this)->constantColor[static_cast<u32>(index) %
                                                              ConstantColorMax]);
    } else if (m_MemCap.combinerUserShaderParameter != 0 && index < ConstantColorMax) {
        color = *reinterpret_cast<const u32*>(
            GetCombinerUserShader(this)->constantColor[static_cast<u32>(index) % ConstantColorMax]);
    }

    nn::util::Unorm8x4 result;
    std::memcpy(&result, &color, sizeof(result));
    return result;
}

/**
 * @brief Sets an element of a material color.
 * @param colorType Animation target of the element.
 * @param value New value of the element.
 */
inline void Material::SetColorElement(int colorType, int value) {
    if (colorType < AnimTargetMatColorMax) {
        const int colorIndex = colorType / 4;
        if (static_cast<u32>(colorIndex) < MaterialColor_Max) {
            const u32 index = colorIndex;
            switch (colorType % 4) {
            case 0:
                m_ByteColors[index].v[0] = value;
                break;
            case 1:
                m_ByteColors[index].v[1] = value;
                break;
            case 2:
                m_ByteColors[index].v[2] = value;
                break;
            case 3:
                m_ByteColors[index].v[3] = value;
                break;
            default:
                break;
            }
        } else if (static_cast<u32>(colorIndex) < MaterialColor_Max + ConstantColorMax) {
            const u8 constantIndex = colorIndex - MaterialColor_Max;
            if (m_MemCap.detailedCombinerParameter != 0) {
                u32* pColor = reinterpret_cast<u32*>(
                    &GetDetailedCombinerStageInfo(this)->constantColor[constantIndex %
                                                                       ConstantColorMax]);
                *pColor = ToPackedColor(SetElement(FromPackedColor(*pColor), colorType % 4, value));
            } else if (m_MemCap.combinerUserShaderParameter != 0) {
                u32* pColor = reinterpret_cast<u32*>(
                    GetCombinerUserShader(this)->constantColor[constantIndex % ConstantColorMax]);
                *pColor = ToPackedColor(SetElement(FromPackedColor(*pColor), colorType % 4, value));
            }
        }
    }
}
}  // namespace nn::ui2d
