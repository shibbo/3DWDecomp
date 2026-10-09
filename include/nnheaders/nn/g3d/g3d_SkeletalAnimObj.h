#pragma once

#include <nn/g3d/g3d_AnimObj.h>
#include <nn/g3d/g3d_ResSkeletalAnim.h>
#include <nn/g3d/g3d_Resources.h>
#include <nn/types.h>
#include <nn/util/util_MathTypes.h>
#include <algorithm>
#include <attributes.h>

namespace nn::g3d {
class ResSkeleton;
class SkeletonObj;
struct QuatToMtx;
struct EulerToMtx;
struct AxesToMtx;
struct AxesToQuat;
struct QuatToAxes;
struct QuatToQuat;
struct EulerToQuat;
struct EulerToAxes;

class SkeletalAnimObj : public ModelAnimObj {
  public:
    struct InitializeArgument {
        /** @brief Initialize unset capacities and empty workspace blocks with curve caching enabled. */
        InitializeArgument() {
            boneCount = boneAnimCount = curveCount = -1;
            isContextEnabled = true;
            isContextAvailable = false;
            isRetargetingEnabled = false;
            memorySize = 0;
            memoryAlignment = 0;
            for (int i = 0; i < 4; ++i) {
                blocks[i].Initialize(0);
            }
        }

        /** @brief Set the maximum number of target bones.
         * @param count Nonnegative bone capacity sufficient for every target skeleton.
         */
        void SetMaxBoneCount(int count) { boneCount = count; }

        /**
         * @brief Increase capacities to accommodate a skeletal animation resource.
         * @param pResAnim Non-null animation this object must be able to evaluate.
         */
        void Reserve(const ResSkeletalAnim* pResAnim) {
            boneAnimCount = std::max(boneAnimCount, pResAnim->GetBoneAnimCount());
            curveCount = std::max(curveCount, pResAnim->GetCurveCount());
            isContextAvailable |= !pResAnim->IsCurveBaked();
        }

        void CalculateMemorySize();
        /** @brief Query the last calculated workspace requirement.
         * @return Required bytes, or zero before the layout is calculated.
         */
        size_t GetWorkMemorySize() const { return memorySize; }

        int boneCount = -1;
        int boneAnimCount = -1;
        int curveCount = -1;
        bool isContextEnabled = true;
        bool isContextAvailable = false;
        bool isRetargetingEnabled = false;
        size_t memorySize = 0;
        size_t memoryAlignment = 0;
        detail::WorkMemoryBlock blocks[4];
    };

    static_assert(sizeof(InitializeArgument) == 0xa0);

    struct BindArgument {
        const ResSkeleton* pTargetSkeleton;
        const ResSkeleton* pSourceSkeleton;
        bool isRetargetingEnabled;
        bool isMirroringEnabled;
    };

    /** @brief Construct an empty skeletal animation object without allocated working storage. */
    SkeletalAnimObj() {}
    /** @brief Destroy the animation object without releasing caller-owned workspace. */
    ~SkeletalAnimObj() override = default;

    void ClearResult() override;
    void Calculate() override;
    BindResult Bind(const ResModel* pModel) override;
    BindResult Bind(const ModelObj* pModel) override;
    void BindFast(const ResModel* pModel) override;
    void ApplyTo(ModelObj* pModel) const override;

    bool Initialize(const InitializeArgument& rArg, void* pBuffer, size_t bufferSize);
    void SetResource(const ResSkeletalAnim* pRes);
    BindResult Bind(const ResSkeleton* pSkeleton);
    BindResult Bind(const SkeletonObj* pSkeleton);
    BindResult Bind(const BindArgument& rArg);
    BindResult BindFast(const BindArgument& rArg);
    BindResult Bind(const ResSkeleton* pTarget, const ResSkeleton* pSource);
    BindResult Bind(const SkeletonObj* pTarget, const SkeletonObj* pSource);
    BindResult Bind(const ResModel* pTarget, const ResModel* pSource);
    BindResult Bind(const ModelObj* pTarget, const ModelObj* pSource);
    BindResult BindFast(const ResSkeleton* pTarget, const ResSkeleton* pSource);
    BindResult BindFast(const ResModel* pTarget, const ResModel* pSource);
    void ClearResult(const ResSkeleton* pSkeleton);
    void BindFast(const ResSkeleton* pSkeleton);
    void SetBindFlag(const ResSkeleton* pSkeleton, int boneIndex, BindFlag flag);
    void ApplyTo(SkeletonObj* pSkeleton) const;

    /** @brief Access the selected skeletal animation resource.
     * @return Active resource, or nullptr before resource selection.
     */
    const ResSkeletalAnim* GetResource() const { return mResource; }

    /** @brief Query the number of bone animations in the selected resource.
     * @return Number of entries in the result and binding arrays.
     */
    int GetBoneAnimCount() const { return mBindTable.mAnimCount; }

    /**
     * @brief Access a binding-table entry.
     * @param index Animation or bone index within the binding table.
     * @return Packed target index, animation index and bind flags.
     */
    u32 GetBindEntry(ptrdiff_t index) const { return mBindTable.mEntries[index]; }

    /** @brief Access the calculated bone animation results.
     * @return Result array indexed by bone animation.
     */
    BoneAnimResult* GetResultArray() const { return static_cast<BoneAnimResult*>(mResult); }

    /** @brief Query how the selected resource stores rotations.
     * @return Zero for quaternions, one for Euler angles.
     */
    u32 GetRotateMode() const { return (mResource->flags >> 12) & 7; }

    /** @brief Access the skeleton used by the last binding.
     * @return Bound skeleton, or nullptr before binding.
     */
    const ResSkeleton* GetBoundSkeleton() const { return m_pBoundSkeleton; }

  private:
    const ResSkeletalAnim* mResource = nullptr;
    struct Impl;
    template <bool mirrored, bool retargeted> void ClearImpl(const ResSkeleton* pSkeleton);
    template <bool mirrored, bool retargeted> void CalculateImpl();
    template <class Converter> void ApplyToImpl(SkeletonObj* pSkeleton) const;
    BindResult BindImpl(const ResSkeleton* pSkeleton);
    BindResult InitRetargeting(const ResSkeleton* pTarget, const ResSkeleton* pSource);
    void BindFastImpl(const ResSkeleton* pTarget);
    void MirrorConstantResult(BoneAnimResult* pResult, const ResBoneAnim* pAnim, const ResBone* pBone) const;
    template <bool retargeted, class Evaluator>
    void CalculateMirroredBone(BoneAnimResult* pResults, int animIndex, const Evaluator& rEvaluate);

    /**
     * @brief Find the animation bound to a skeleton bone.
     * @param boneIndex Bone index within the binding table.
     * @return Animation index, or 0x7fff when no animation targets the bone.
     */
    u32 GetAnimIndex(ptrdiff_t boneIndex) const { return (mBindTable.mEntries[boneIndex] >> 15) & 0x7fff; }

    /**
     * @brief Find the bone that mirrors a skeleton bone.
     * @param pSkeleton Skeleton with mirroring information.
     * @param boneIndex Bone index within the skeleton.
     * @return Mirrored bone index, or a negative value when the bone has no counterpart.
     */
    static int GetMirroringBoneIndex(const ResSkeleton* pSkeleton, int boneIndex) {
        return pSkeleton->ToData().pMirroringBoneTable.Get()[boneIndex];
    }

    const ResBoneAnim* m_pBoneAnims = nullptr;
    int m_BoneAnimCapacity = 0;
    u32 m_Flags = 0;
    util::Vector4fType* m_pRetargeting = nullptr;
    const ResSkeleton* m_pBoundSkeleton = nullptr;
};

static_assert(sizeof(SkeletalAnimObj) == 0x90);

/** @brief Weighted per-bone accumulator used while blending skeletal animations. */
struct BoneAnimBlendResult {
    nn::util::Vector3fType scale;
    nn::util::Vector3fType translate;
    /** First two rotation matrix rows, or a quaternion in axisX when quaternion storage is active. */
    nn::util::Vector3fType axisX;
    nn::util::Vector3fType axisY;
    u32 flags;
    float weight;
    u8 _48[8];
};

static_assert(sizeof(BoneAnimBlendResult) == 0x50);

using SkeletalAnimBlendResult = BoneAnimBlendResult;

/** @brief Hook that can adjust each bone's weight before it is blended. */
class ICalculateBlendCallback {
  public:
    struct CallbackArg {
        const SkeletalAnimObj* pAnimObj;
        int boneIndex;
        float weight;
    };

    /** @brief Destroy the callback. */
    virtual ~ICalculateBlendCallback() {}
    virtual void Exec(CallbackArg* pArg) = 0;
};

class SkeletalAnimBlender {
  public:
    enum BlendMode { BlendMode_Interpolate = 0, BlendMode_Additive = 1 };
    struct InitializeArgument {
        /** @brief Initialize an empty result-storage block with an unset bone capacity. */
        InitializeArgument() { blocks[0].Initialize(0); }

        /** @brief Set the capacity of the blending result array.
         * @param count Nonnegative number
         * of bones in the largest target skeleton. */
        void SetMaxBoneCount(int count) { boneCount = count; }

        void CalculateMemorySize();
        /** @brief Query the calculated workspace size.
         * @return Required bytes, or zero before
         * sizing. */
        size_t GetWorkMemorySize() const { return memorySize; }

        int boneCount = -1;
        size_t memorySize = 0;
        size_t memoryAlignment = 0;
        detail::WorkMemoryBlock blocks[1];
    };

    /** @brief Construct an empty blender without allocating storage. */
    SkeletalAnimBlender() {}

    bool Initialize(const InitializeArgument& rArg, void* pBuffer, size_t bufferSize);
    void ClearResult();
    void EndBlend();
    void BeginBlend(u32 flags);
    void Blend(SkeletalAnimObj* pAnimObj, float weight);
    void Blend(SkeletalAnimObj* pAnimObj, SkeletalAnimObj* pBaseAnimObj, float weight);
    void ApplyTo(SkeletonObj* pSkeleton) const;

    /** @brief Access the accumulated per-bone transforms.
     * @return Caller-owned result array, or
     * nullptr before initialization. */
    BoneAnimBlendResult* GetResult() const { return mResult; }
    /** @brief Query the number of active bones.
     * @return Active bone count established during
     * initialization. */
    int GetBoneCount() const { return mBoneCount; }
    /** @brief Query the result-array capacity.
     * @return Maximum number of bone results that fit in the
     * workspace. */
    int GetMaxBoneCount() const { return mMaxBoneCount; }

    /** @brief Per-bone difference between two animation results. */
    struct BoneAnimDiffResult {
        BoneAnimResult result;
    };

    void BlendResult(const BoneAnimResult& rResult, int boneIndex, float weight);
    void BlendResult(const BoneAnimDiffResult& rResult, int boneIndex, float weight);
    static void CalculateBoneAnimDiff(BoneAnimDiffResult* pResults, int count, SkeletalAnimObj* pAnimObj,
                                      SkeletalAnimObj* pBaseAnimObj);
    static void CalculateBoneAnimDiff(BoneAnimDiffResult* pResults, int count, SkeletalAnimObj* pAnimObj);

  private:
    struct Impl;
    template <class Converter, BlendMode mode> void ApplyToImpl(SkeletonObj* pSkeleton) const;
    template <class Converter> void ConvertResultRotate();
    template <class Converter, bool useCallback> void BlendImpl(SkeletalAnimObj* pAnimObj, float weight);
    template <class Converter>
    NOINLINE void BlendResultImpl(BoneAnimBlendResult* pResults, const BoneAnimResult* pResult, int boneIndex,
                         float weight);
    template <class Converter>
    void ConvertValidResult(BoneAnimResult* pOut, const BoneAnimBlendResult& rBlend,
                            const BoneAnimResult& rResult);
    template <class AnimConverter, class BaseConverter>
    static void CalculateBoneAnimDiffImpl(BoneAnimResult* pOut, const BoneAnimResult& rAnim,
                                          const BoneAnimResult& rBase);
    void BlendDiffAnim(SkeletalAnimObj* pAnimObj, SkeletalAnimObj* pBaseAnimObj, float weight);
    BoneAnimBlendResult* mResult = nullptr;
    u16 mBoneCount = 0;
    u16 mMaxBoneCount = 0;
    mutable u32 m_Flags = 0;
    void* m_pWorkMemory = nullptr;
    ICalculateBlendCallback* m_pCallback = nullptr;
};

static_assert(sizeof(SkeletalAnimBlender) == 0x20);

} // namespace nn::g3d
