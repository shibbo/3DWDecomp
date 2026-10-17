#pragma once
#include "Library/Scene/ISceneObj.hpp"
namespace al { class IUseSceneObjHolder; }
class InkPatch;
class InkPatchSpecial : public al::ISceneObj {
public:
    static InkPatch* tryGetSpecialInkPatch(const al::IUseSceneObjHolder*);
    static InkPatchSpecial* tryGetInkPatchSpecial(const al::IUseSceneObjHolder*);
    InkPatchSpecial();
    void addUnlockerInkPatch(InkPatch*);
    bool isUnlockerInkPatch(InkPatch*);
    bool isLastUnlockerInkPatch(InkPatch*);
    const char* getSceneObjName() const override;
    void setSpecialPatch(InkPatch* patch) { mSpecialPatch = patch; }
    int getProgress() const { return _20; }

    /**
     * @brief Advances the progress counter, saturating at the largest int.
     */
    void incProgress() {
        if (_20 != 0x7fffffff) {
            _20++;
        }
    }
private:
    InkPatch* mSpecialPatch = nullptr;
    InkPatch* mUnlockers[2] = {};
    int _20 = 0;
};
