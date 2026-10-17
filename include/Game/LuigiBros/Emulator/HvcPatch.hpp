#pragma once
namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {
class CHvc;
/** @brief Per-title hooks attached to the HVC platform. */
class CHvcPatch {
public:
    virtual ~CHvcPatch();
    void Configure();
    void _PatchPostProgress_Kirby();
    void _PatchPostProgress_LinkNoBouken();
    void _PatchPostProgress_ParthenaNoKagami();
    void _PatchAnteProgress_Mother();
    void _PatchDefault();
    bool _replacePaletteColorToDirectColorInOrder(const unsigned char* pattern, const unsigned int* colors, const unsigned char* palette);
    void _PatchExport_ParthenaNoKagami();
    void _PatchImport_ParthenaNoKagami();
    void _PatchPostCommence_Kirby();
    void _PatchPostProgress_Excitebike();
    void _PatchPostProgress_SuperMarioUSA();
    void _PatchAnteProgress_SuperMarioUSA();
    void _PatchAttach_BalloonFight();
    void _PatchPostProgress_YoshiNoTamago();
    void _PatchAttach_PunchOut();
    void _PatchAttach_LinkNoBouken();
    void _PatchAttach_LinkNoBouken_NonJP();
    void _PatchAttach_WreckingCrew();
    void _PatchAttach_Metroid();
    void _PatchAttach_Kirby();
    void _PatchAttach_YoshiNoCookie();
    void _PatchPostProgress_SuperMario3();
    void _PatchAnteProgress_LinkNoBouken();
    void _PatchAttach_Mother();
    void _PatchPostCommence_ParthenaNoKagami();
    void _PatchAnteProgress_ParthenaNoKagami();
    CHvc* mpHvc;
    char mName[8];
    typedef void (CHvcPatch::*Hook)();
    Hook mHooks[7];
};
static_assert(sizeof(CHvcPatch) == 0x88, "HVC patch layout");
} } } }
