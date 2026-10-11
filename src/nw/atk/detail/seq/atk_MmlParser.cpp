#include <nn/atk/atk_MmlSequenceTrack.h>

namespace nn::atk::detail::Util { u32 CalcRandom(); }
namespace nn::atk::detail::driver {
bool MmlParser::mPrintVarEnabledFlag = false;
MmlParser::~MmlParser() {}
// track supplies the sequence cursor and playback state. playNotes suppresses note
// creation when false, while commands and cursor advancement still take effect.
int MmlParser::Parse(MmlSequenceTrack* track, bool playNotes) const {
    int command = *track->mContext.mPosition++;
    SequenceSoundPlayer* player = track->mPlayer;
    bool execute = true;

    if (command == 0xa2) {
        command = *track->mContext.mPosition++;
        execute = track->mContext.mCondition;
    }

    SeqArgType extra = SeqArgType_None;

    switch (static_cast<u8>(command)) {
    case 0xa3: extra = SeqArgType_Short; command = *track->mContext.mPosition++; break;
    case 0xa4: extra = SeqArgType_Random; command = *track->mContext.mPosition++; break;
    case 0xa5: extra = SeqArgType_Variable; command = *track->mContext.mPosition++; break;
    }

    SeqArgType argument = SeqArgType_None;
    bool overrideArg = false;

    if (command == 0xa0) {
        overrideArg = true;
        argument = SeqArgType_Random;
        command = *track->mContext.mPosition++;
    } else if (command == 0xa1) {
        overrideArg = true;
        argument = SeqArgType_Variable;
        command = *track->mContext.mPosition++;
    }

    if ((command & 0x80) == 0) {
        int velocity = *track->mContext.mPosition++;
        int length = ReadArg(&track->mContext.mPosition, player, track, overrideArg ? argument : SeqArgType_VariableLength);

        if (execute) {
            if (!track->mContext.mMuted && playNotes) {
                int key = int(command) + track->mTranspose;

                if (key < 0) key = 0;

                if (key > 127) key = 127;
                NoteOnCommandProc(track, key, velocity, length > 0 ? length : -1, track->mContext.mTie);
            }

            if (track->mContext.mNoteWait) {
                track->mContext.mWait = length;

                if (length == 0) track->mWaitForNote = true;
            }
        }

        return 0;
    }

    switch (command & 0xf0) {
    case 0x80:
        switch (command) {
        case 0x80: {
            int wait = ReadArg(&track->mContext.mPosition, player, track, overrideArg ? argument : SeqArgType_VariableLength);

            if (execute) track->mContext.mWait = wait;
            break;
        }
        case 0x81: {
            int value = ReadArg(&track->mContext.mPosition, player, track, overrideArg ? argument : SeqArgType_VariableLength);

            if (execute) CommandProc(track, command, value, 0);
            break;
        }
        case 0x88: {
            int index = *track->mContext.mPosition++;
            int offset = Read24(&track->mContext.mPosition);

            if (execute) CommandProc(track, command, index, offset);
            break;
        }
        case 0x89: case 0x8a: {
            int offset = Read24(&track->mContext.mPosition);

            if (execute) CommandProc(track, command, offset, 0);
            break;
        }
        }

        break;
    case 0xb0: case 0xc0: case 0xd0: {
        int value = ReadArg(&track->mContext.mPosition, player, track, overrideArg ? argument : SeqArgType_Byte);
        int second = extra != SeqArgType_None ? ReadArg(&track->mContext.mPosition, player, track, extra) : 0;

        if (execute) {
            int first;

            switch (command) {
            case 0xc3: case 0xc4: first = static_cast<s8>(value); break;
            default: first = static_cast<u8>(value); break;
            }

            CommandProc(track, command, first, second);
        }

        break;
    }
    case 0xe0: {
        int value = ReadArg(&track->mContext.mPosition, player, track, overrideArg ? argument : SeqArgType_Short);

        if (execute) CommandProc(track, command, static_cast<s16>(value), 0);
        break;
    }
    case 0xf0:
        switch (command) {
        case 0xfe: track->mContext.mPosition += 2; break;
        case 0xff: if (execute) return 1; break;
        case 0xf0: {
            u8 extended = *track->mContext.mPosition++;
            auto dispatch = [&](int value) { CommandProc(track, extended | command << 8, value, 0); };

            switch (extended & 0xf0) {
            case 0x80: case 0x90: {
                int index = *track->mContext.mPosition++;
                int value = ReadArg(&track->mContext.mPosition, player, track, overrideArg ? argument : SeqArgType_Short);

                if (execute) CommandProc(track, static_cast<u16>(extended | command << 8), index, static_cast<s16>(value));
                break;
            }
            case 0xa0: case 0xb0: {
                int value = *track->mContext.mPosition++;

                if (execute) dispatch(value);
                break;
            }
            case 0xe0: {
                int value = ReadArg(&track->mContext.mPosition, player, track, overrideArg ? argument : SeqArgType_Short);

                if (execute) dispatch(static_cast<u16>(value));
                break;
            }
            }

            break;
        }
        default: if (execute) CommandProc(track, command, 0, 0); break;
        }

        break;
    case 0x90:
        if (execute) CommandProc(track, command, 0, 0);
        break;
    }

    return 0;
}

// track is the command destination; command identifies a normal or extended MML
// opcode. first and second are its decoded operands (values, indices, offsets, or ramp durations).
void MmlParser::CommandProc(MmlSequenceTrack* track, u32 command, int first, int second) const {
    SequenceSoundPlayer* player = track->mPlayer;

    if (command <= 0xff) {
        switch (command) {
        case 0x81: if (first < 0x10000) track->mProgram = static_cast<u16>(first); break;
        case 0x88: {
            SequenceTrack* other = player->GetPlayerTrack(first);

            if (other != nullptr && other != track) {
                other->Close(); other->SetSeqData(track->mContext.mSequenceData, second); other->Open();
            }

            break;
        }
        case 0x89: track->mContext.mPosition = track->mContext.mSequenceData + first; break;
        case 0x8a:
            if (track->mContext.mStackDepth < 10) {
                track->mContext.PushCall();
                track->mContext.mPosition = track->mContext.mSequenceData + first;
            }

            break;
        case 0xb0: player->mParamB0 = first; break;
        case 0xb1: track->mHold = static_cast<u8>(first); break;
        case 0xb2:
            track->mContext.mMono = first != 0;

            if (first) { track->ReleaseAllChannel(-1); track->FreeAllChannel(); }
            break;
        case 0xb3: track->mParamB3 = first; break;
        case 0xb4: track->mParamB4 = first; break;
        case 0xb5: track->mParamB5 = float(first) / 127.0f; break;
        case 0xb6: track->mParamB6 = first; break;
        case 0xbd: track->mLfo[0].range = first; break;
        case 0xbe: track->mLfo[0].target = first; break;
        case 0xbf: track->mContext.mParamBF = first != 0; break;
        case 0xc0: track->mPan.Set(first - 64, second); break;
        case 0xc1: track->mVolume.Set(first, second); break;
        case 0xc2: player->mVolume.Set(first, second); break;
        case 0xc3: track->mTranspose = first; break;
        case 0xc4: track->mPitchBend.Set(first, second); break;
        case 0xc5: track->mBendRange = first; break;
        case 0xc6: track->mPriority = first; break;
        case 0xc7: track->mContext.mNoteWait = first != 0; break;
        case 0xc8:
            track->mContext.mTie = first != 0;
            track->ReleaseAllChannel(-1); track->FreeAllChannel();
            break;
        case 0xc9:
            track->mPortamentoKey = first + track->mTranspose;
            track->mPortamento = true;
            break;
        case 0xca: track->mLfo[0].depth = static_cast<u8>(first) * (1.0f / 128.0f); break;
        case 0xcb: track->mLfo[0].speed = static_cast<u8>(first) * 0.390625f; break;
        case 0xcc: track->mLfoShape[0] = first; break;
        case 0xcd: track->mLfo[0].type = first; break;
        case 0xce: track->mPortamento = first != 0; break;
        case 0xcf: track->mPortamentoTime = first; break;
        case 0xd0: track->mAttack = first; break;
        case 0xd1: track->mDecay = first; break;
        case 0xd2: track->mSustain = first; break;
        case 0xd3: track->mRelease = first; break;
        case 0xd4:
            if (track->mContext.mStackDepth < 10) {
                const u8* position = track->mContext.mPosition;
                auto& entry = track->mContext.mStack[track->mContext.mStackDepth];
                entry.count = first;
                entry.position = position;
                entry.isLoop = true;
                ++track->mContext.mStackDepth;
            }

            break;
        case 0xd5: track->mVolume2.Set(first, second); break;
        case 0xd6: if (mPrintVarEnabledFlag) GetVariablePtr(player, track, first); break;
        case 0xd7: track->mSurroundPan.Set(first, second); break;
        case 0xd8: track->mParamD8 = (first - 64) * (1.0f / 64.0f); break;
        case 0xd9: track->mParamD9 = first; break;
        case 0xda: track->mParamDA = first; break;
        case 0xdb: track->mParamDB = first; break;
        case 0xdc: track->mParamDC = first - 64; break;
        case 0xdd: track->SetMute(static_cast<SequenceMute>(first)); break;
        case 0xde: track->mParamDE = first; break;
        case 0xdf: track->mParamDF = (first & 0xc0) != 0; break;
        case 0xe0: track->mLfo[0].delay = first * 5; break;
        case 0xe1:
            if (first < 0) first = 0;

            if (first > 1023) first = 1023;
            player->mTempo = first;
            break;
        case 0xe3: track->mPitchSweep = first * (1.0f / 64.0f); break;
        case 0xe4: {
            // A zero period stops modulation; otherwise convert ticks to frequency.
            first = static_cast<s16>(first);

            if (!first) { track->mLfo[0].speed = 0.0f; break; }
            track->mLfo[0].speed = 100.0f / first;
            break;
        }
        case 0xfb:
            track->mAttack = track->mDecay = track->mSustain = track->mRelease = -1;
            track->mHold = 255;
            break;
        case 0xfc:
            if (track->mContext.mStackDepth) {
                auto& entry = track->mContext.mStack[track->mContext.mStackDepth - 1];

                if (entry.isLoop) {
                    u8 count = entry.count;

                    if (count && --count == 0) --track->mContext.mStackDepth;
                    else { entry.count = count; track->mContext.mPosition = entry.position; }
                }
            }

            break;
        case 0xfd:
            while (track->mContext.mStackDepth) {
                --track->mContext.mStackDepth;

                if (!track->mContext.mStack[track->mContext.mStackDepth].isLoop) {
                    track->mContext.mPosition = track->mContext.mStack[track->mContext.mStackDepth].position;
                    break;
                }
            }

            break;
        }
    } else if (command <= 0xffff) {
        vs16* variable = nullptr;

        if ((command & 0xf0) == 0x80 || (command & 0xf0) == 0x90) {
            variable = GetVariablePtr(player, track, first);

            if (!variable) return;
        }

        switch (command & 0xff) {
        case 0x80: *variable = second; break;
        case 0x81: *variable += second; break;
        case 0x82: *variable -= second; break;
        case 0x83: *variable *= second; break;
        case 0x84: if (second) *variable /= static_cast<s16>(second); break;
        case 0x85:
            if (second >= 0) *variable <<= second;
            else *variable >>= -second;
            break;
        case 0x86: {
            int negative = 0;

            if (second < 0) { second = static_cast<s16>(-second); negative = 1; }
            int random = Util::CalcRandom() & 0xffff;
            int value = (random * (second + 1)) >> 16;

            *variable = negative ? -value : value;
            break;
        }
        case 0x87: *variable &= second; break;
        case 0x88: *variable |= second; break;
        case 0x89: *variable ^= second; break;
        case 0x8a: *variable = ~second; break;
        case 0x8b: if (second) *variable %= second; break;
        case 0x90: track->mContext.mCondition = *variable == second; break;
        case 0x91: track->mContext.mCondition = *variable >= second; break;
        case 0x92: track->mContext.mCondition = *variable > second; break;
        case 0x93: track->mContext.mCondition = *variable <= second; break;
        case 0x94: track->mContext.mCondition = *variable < second; break;
        case 0x95: track->mContext.mCondition = *variable != second; break;
        case 0xa0: track->mLfo[1].target = first; break;
        case 0xa1: track->mLfo[1].range = first; break;
        case 0xa2: track->mLfo[1].depth = static_cast<u8>(first) * (1.0f / 128.0f); break;
        case 0xa3: track->mLfo[1].speed = static_cast<u8>(first) * 0.390625f; break;
        case 0xa4: track->mLfoShape[1] = first; break;
        case 0xa5: track->mLfo[1].type = first; break;
        case 0xa6: track->mLfo[2].target = first; break;
        case 0xa7: track->mLfo[2].range = first; break;
        case 0xa8: track->mLfo[2].depth = static_cast<u8>(first) * (1.0f / 128.0f); break;
        case 0xa9: track->mLfo[2].speed = static_cast<u8>(first) * 0.390625f; break;
        case 0xaa: track->mLfoShape[2] = first; break;
        case 0xab: track->mLfo[2].type = first; break;
        case 0xac: track->mLfo[3].target = first; break;
        case 0xad: track->mLfo[3].range = first; break;
        case 0xae: track->mLfo[3].depth = static_cast<u8>(first) * (1.0f / 128.0f); break;
        case 0xaf: track->mLfo[3].speed = static_cast<u8>(first) * 0.390625f; break;
        case 0xb0: track->mLfoShape[3] = first; break;
        case 0xb1: track->mLfo[3].type = first; break;
        case 0xe0: player->CallSequenceUserprocCallback(first, track); break;
        case 0xe1: track->mLfo[1].delay = first * 5; break;
        case 0xe2: {
            // A zero period stops modulation; otherwise convert ticks to frequency.
            first = static_cast<s16>(first);

            if (!first) { track->mLfo[1].speed = 0.0f; break; }
            track->mLfo[1].speed = 100.0f / first;
            break;
        }
        case 0xe3: track->mLfo[2].delay = first * 5; break;
        case 0xe4: {
            // A zero period stops modulation; otherwise convert ticks to frequency.
            first = static_cast<s16>(first);

            if (!first) { track->mLfo[2].speed = 0.0f; break; }
            track->mLfo[2].speed = 100.0f / first;
            break;
        }
        case 0xe6: {
            // A zero period stops modulation; otherwise convert ticks to frequency.
            first = static_cast<s16>(first);

            if (!first) { track->mLfo[3].speed = 0.0f; break; }
            track->mLfo[3].speed = 100.0f / first;
            break;
        }
        case 0xe5: track->mLfo[3].delay = first * 5; break;
        }
    }
}

// position points to the sequence cursor; consume a little-endian 24-bit argument.
u32 MmlParser::Read24(const u8** position) const {
    u32 first = *(*position)++;
    u32 second = *(*position)++;
    u32 third = *(*position)++;
    return first | second << 8 | third << 16;
}

// position points to the sequence cursor; consume a little-endian 16-bit argument.
u16 MmlParser::Read16(const u8** position) const {
    int first = *(*position)++;
    int second = *(*position)++;
    return first | second << 8;
}

// position advances through seven-bit groups, most significant group first.
u32 MmlParser::ReadVar(const u8** position) const {
    u32 result = 0;
    u8 value;

    do {
        value = *(*position)++;
        result = (result << 7) | (value & 0x7f);
    } while (value & 0x80);
    return result;
}

// index selects player variables 0-31 or track variables 32-47. player and track
// own those respective banks; out-of-range positive indices return null.
vs16* MmlParser::GetVariablePtr(SequenceSoundPlayer* player, SequenceTrack* track, int index) const {
    if (index < 32) return player->GetVariablePtr(index);

    if (index < 48) return track->GetVariablePtr(index - 32);
    return nullptr;
}

// position advances over an argument encoded by type. player and track supply
// variable values; random arguments contain signed inclusive lower and upper bounds.
int MmlParser::ReadArg(const u8** position, SequenceSoundPlayer* player, SequenceTrack* track,
                       SeqArgType type) const {
    switch (type) {
    case SeqArgType_Byte: return *(*position)++;
    case SeqArgType_Short: return static_cast<u16>(Read16(position));
    case SeqArgType_VariableLength: return ReadVar(position);
    case SeqArgType_Random: {
        int lowByte = *(*position)++;
        int highByte = *(*position)++;
        int low = lowByte | static_cast<s16>(highByte << 8);
        lowByte = *(*position)++;
        highByte = *(*position)++;
        int high = lowByte | static_cast<s16>(highByte << 8);
        return low + ((int(Util::CalcRandom() & 0xffff) * (high - low + 1)) >> 16);
    }
    case SeqArgType_Variable: {
        vs16* variable = GetVariablePtr(player, track, *(*position)++);
        return variable ? *variable : 0;
    }
    default: return 0;
    }
}

// track receives a note with key, velocity, duration length, and tie state.
Channel* MmlParser::NoteOnCommandProc(MmlSequenceTrack* track, int key, int velocity, int length, bool tie) const {
    return track->NoteOn(key, velocity, length, tie);
}

// data is the sequence base and offset selects its initial command. trackMask
// receives the optional big-endian allocation mask, always including track zero.
u32 MmlParser::ParseAllocTrack(const void* data, u32 offset, u32* trackMask) {
    const u8* position = static_cast<const u8*>(data) + offset;

    if (*position == 0xfe) {
        *trackMask = (u32(position[1]) << 8 | position[2]) | 1;
        return offset + 3;
    }

    *trackMask = 1;
    return offset;
}
}
