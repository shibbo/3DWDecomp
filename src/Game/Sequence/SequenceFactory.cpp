#include "System/SequenceFactory.hpp"

#include "Sequence/ProductSequence.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
/**
 * @brief Allocate the requested sequence type.
 * @param pName Name passed to the sequence constructor.
 * @return Newly allocated sequence.
 */
template <class T>
al::Sequence* createSequence(const char* pName) {
    return new T(pName);
}
using Creator = al::Sequence* (*)(const char*);
struct Entry { const char* pName; Creator creator; };
const Entry cEntries[] = {{"ProductSequence", &createSequence<ProductSequence>}};
/**
 * @brief Look up a registered sequence constructor.
 * @param pName Sequence class name.
 * @return Constructor, or nullptr if the name is not registered.
 */
Creator findCreator(const char* pName) {
    for (const auto& rEntry : cEntries) {
        if (al::isEqualString(pName, rEntry.pName)) {
            return rEntry.creator;
        }
    }
    return nullptr;
}
}

/**
 * @brief Create the sequence registered under the given class name.
 * @param pName Registered class name; must be ProductSequence.
 * @return Newly allocated product sequence.
 */
al::Sequence* SequenceFactory::createSequence(const char* pName) {
    return findCreator(pName)(pName);
}
