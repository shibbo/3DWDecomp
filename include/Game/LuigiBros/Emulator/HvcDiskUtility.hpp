#pragma once
#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"

namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {

class CHvcDiskUtility {
public:
    struct _S6502Register {
        unsigned char status;
        unsigned char accumulator;
        unsigned char x;
        unsigned char y;
        unsigned char stack;
        unsigned char _05;
        unsigned short pc;
    };

    CHvcDiskUtility();
    virtual ~CHvcDiskUtility();
    long LoadOnColdStart(CPlatformHvcUnit::_SEntity* pEntity);
    long PurgeSave(CPlatformHvcUnit::_SEntity* pEntity, const unsigned char* pDiskId,
                   unsigned char fileIndex, const unsigned char* pHeader);
    bool CallSubroutine(_S6502Register* pRegisters, unsigned short address,
                        CPlatformHvcUnit::_SEntity* pEntity);
    void _Load(_S6502Register* pRegisters, CPlatformHvcUnit::_SEntity* pEntity);
    unsigned short _GetAddress(unsigned short address, CPlatformHvcUnit::_SEntity* pEntity);
    void _TransferCharacterRamData(_S6502Register* pRegisters, CPlatformHvcUnit::_SEntity* pEntity);
    void _TransferVram(_S6502Register* pRegisters, CPlatformHvcUnit::_SEntity* pEntity);
    void _Save();
    void _Erase();
    void _Keep();
    void _Purge();
    void _Recover();
    void _PurgeSave();
    void _DumpDirectory();
    void _DumpFullDirectory();
    void _WriteVram();
    void _ReserveWriteBgData();
    void _ReserveWriteCharacterData();
    void _ReadVram();
    void _ReserveReadBgData();
    void _ConvertObgHVToBgAddress();
    void _ConvertBgAddressToObgHV();
    void _JumpIndex();
    void _WaitNmi();
    void _BlankObjBg();
    void _AppearObjBG();
    void _BlankObj();
    void _AppearObj();
    void _BlankBg();
    void _AppearBg();
    void _RefreshTimer();
    void _ClearVram();
    void _ClearWram();
    void _InputController1();
    void _InputController3();
    void _InputController4();
    void _InputController5();
    void _SenseKeyboard();
    void _SetOamForDmaTransfer();
    void _SetScrollCounter();
    void _InputController2();
    void _SetAreaSoftTimer();
    void _SetAreaRandom();
    void _TransferOamByDma();
    void _SetMilliSecondTimer();
};

} } } }
