#pragma once

#include <nn/gfx/detail/gfx_RequiredMemory.h>
#include <nn/gfx/detail/gfx_State-api.nvn.8.h>

namespace nn::gfx {

template <class TTarget>
class TRasterizerState : public detail::RasterizerStateImpl<TTarget> {
    NN_NO_COPY(TRasterizerState);

public:
    typedef RasterizerStateInfo InfoType;

    TRasterizerState();

    void Initialize(TDevice<TTarget>*, const InfoType&);
    void Finalize(TDevice<TTarget>*);
};

template <class TTarget>
class TBlendState : public detail::BlendStateImpl<TTarget>,
                    private detail::RequiredMemory<detail::BlendStateImpl<TTarget>> {
    NN_NO_COPY(TBlendState);

public:
    typedef BlendStateInfo InfoType;

    static size_t GetRequiredMemorySize(const InfoType&);

    TBlendState() {}
    void SetMemory(void* pMemory, size_t size) {
        return detail::BlendStateImpl<TTarget>::SetMemory(pMemory, size);
    }
    void* GetMemory() { return detail::BlendStateImpl<TTarget>::GetMemory(); }
    const void* GetMemory() const { return detail::BlendStateImpl<TTarget>::GetMemory(); }
    void Initialize(TDevice<TTarget>* pDevice, const InfoType& info) {
        return detail::BlendStateImpl<TTarget>::Initialize(pDevice, info);
    }
    void Finalize(TDevice<TTarget>* pDevice) {
        return detail::BlendStateImpl<TTarget>::Finalize(pDevice);
    }
};

template <class TTarget>
class TDepthStencilState : public detail::DepthStencilStateImpl<TTarget> {
    NN_NO_COPY(TDepthStencilState);

public:
    typedef DepthStencilStateInfo InfoType;

    TDepthStencilState() {}
    void Initialize(TDevice<TTarget>*, const InfoType&);
    void Finalize(TDevice<TTarget>*);
};

template <typename TTarget>
class TVertexState
    : public detail::VertexStateImpl<typename detail::TargetVariation<TTarget>::Type>,
      private detail::RequiredMemory<
          detail::VertexStateImpl<typename detail::TargetVariation<TTarget>::Type>> {
    NN_NO_COPY(TVertexState);

    typedef detail::VertexStateImpl<typename detail::TargetVariation<TTarget>::Type> Impl;

public:
    typedef typename Impl::Target Target;
    typedef VertexStateInfo InfoType;

    // needed to avoid ambiguity
    enum RequiredMemoryInfo {
        RequiredMemoryInfo_Alignment =
            detail::RequiredMemory<Impl>::MemoryInfoImpl::RequiredMemoryInfo_Alignment
    };

    static size_t GetRequiredMemorySize(const InfoType& info) {
        return detail::RequiredMemory<Impl>::MemoryFuncImpl::GetRequiredMemorySize(info);
    }

    TVertexState() {}

    void SetMemory(void* pMemory, size_t size) {
        return detail::RequiredMemory<Impl>::MemoryFuncImpl::SetMemory(pMemory, size);
    }

    void* GetMemory() { return detail::RequiredMemory<Impl>::MemoryFuncImpl::GetMemory(); }

    const void* GetMemory() const {
        return detail::RequiredMemory<Impl>::MemoryFuncImpl::GetMemory();
    }

    void Initialize(TDevice<Target>* pDevice, const InfoType& info,
                    const TShader<Target>* pVertexShader = nullptr) {
        return Impl::Initialize(pDevice, info, pVertexShader);
    }
    void Finalize(TDevice<Target>* pDevice) { return Impl::Finalize(pDevice); }
};

template <class TTarget>
class TViewportScissorState
    : public detail::ViewportScissorStateImpl<TTarget>,
      private detail::RequiredMemory<detail::ViewportScissorStateImpl<TTarget>> {
    NN_NO_COPY(TViewportScissorState);

public:
    typedef ViewportScissorStateInfo InfoType;

    static size_t GetRequiredMemorySize(const InfoType&);

    TViewportScissorState() = default;
    void SetMemory(void*, size_t);
    void* GetMemory();
    const void* GetMemory() const;
    // device owns the state; info supplies the viewport and scissor arrays.
    void Initialize(TDevice<TTarget>* device, const InfoType& info) {
        detail::ViewportScissorStateImpl<TTarget>::Initialize(device, info);
    }
    // device is the device used to initialize this state.
    void Finalize(TDevice<TTarget>* device) { detail::ViewportScissorStateImpl<TTarget>::Finalize(device); }
};

}  // namespace nn::gfx
