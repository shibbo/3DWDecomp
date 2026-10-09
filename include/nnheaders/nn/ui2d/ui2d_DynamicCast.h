#pragma once

#include <nn/font/font_Util.h>
#include <type_traits>

namespace nn::ui2d {

/**
 * @brief Cast a runtime-typed object to a derived type.
 * @tparam TTo Pointer type to cast to.
 * @param pObject Object to cast.
 * @return pObject as TTo when it derives from it, otherwise nullptr.
 */
template <typename TTo, typename TFrom>
TTo DynamicCast(TFrom* pObject) {
    const font::detail::RuntimeTypeInfo* pTargetType =
        std::remove_pointer<TTo>::type::GetRuntimeTypeInfoStatic();

    if (pObject != nullptr) {
        for (const auto* pType = pObject->GetRuntimeTypeInfo(); pType != nullptr;
             pType = pType->m_ParentTypeInfo) {
            if (pType == pTargetType) {
                return static_cast<TTo>(pObject);
            }
        }
    }

    return nullptr;
}

/**
 * @brief Check whether a runtime-typed object derives from a type.
 * @tparam T Type to check against.
 * @param pObject Object to check.
 * @return True when pObject is not null and derives from T.
 */
template <typename T, typename TFrom>
bool IsDerivedFrom(const TFrom* pObject) {
    const font::detail::RuntimeTypeInfo* pTargetType = T::GetRuntimeTypeInfoStatic();

    if (pObject != nullptr) {
        for (const auto* pType = pObject->GetRuntimeTypeInfo(); pType != nullptr;
             pType = pType->m_ParentTypeInfo) {
            if (pType == pTargetType) {
                return true;
            }
        }
    }

    return false;
}

}  // namespace nn::ui2d
