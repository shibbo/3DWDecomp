#pragma once

#include <prim/seadSafeString.h>

namespace sead {
class Heap;
}

namespace agl::detail {

bool IsDelimiter(char c);
bool IsNumberDelimiter(char c);
const char* TextToReal(f64* pValue, const char* pText, bool* pIsReal);

class ShaderTextUtil {
public:
    struct ShaderDumpTextAnalyzeResult {
        ShaderDumpTextAnalyzeResult();

        s32 mAluClauseInstNum;
        s32 mTexClauseInstNum;
        s32 mExportNum;
        s32 mVaryingInNum;
        s32 mVaryingOutNum;
        const char* mDisassembly;
        s64 mDisassemblySize;
    };
    static_assert(sizeof(ShaderDumpTextAnalyzeResult) == 0x28);

    static void replaceMacro(sead::BufferedSafeString* pText, const char* const* pMacros,
                             const char* const* pValues, s32 macroNum, char* pWork,
                             s32 workSize);
    static s32 findLineFeedCode(const char* pText, s32* pLength);
    static void replace(char* pText, const char* pInsert, s32 begin, s32 end, void* pWork,
                        s32 workSize);
    static bool isUTF8(const char* pText);
    static sead::HeapSafeString* createRawText(const sead::SafeString& rText,
                                               const char* const* pSourceNames,
                                               const char* const* pSourceTexts, s32 sourceNum,
                                               bool* pUsedFlags, sead::Heap* pHeap);
    static sead::HeapSafeString* createUniformBufferReplaceText(const sead::SafeString& rText,
                                                                sead::Heap* pHeap);
    static void createUniformRegisterReplaceText(sead::SafeString** ppVertexText,
                                                 sead::SafeString** ppFragmentText,
                                                 const sead::SafeString& rVertexSource,
                                                 const sead::SafeString& rFragmentSource,
                                                 const sead::SafeString& rBlockName,
                                                 sead::Heap* pHeap);
    static void analyzeShaderDumpText(const sead::SafeString& rText,
                                      ShaderDumpTextAnalyzeResult* pResult);
    static void findFirstChar(char c, const char** ppText);
    static void skipChar(char c, const char** ppText);
    static bool skipFirstMatchedString(const sead::SafeString& rStr, const char** ppText);
    static bool matchString(const sead::SafeString& rStr, const char* pText);
    static bool skipString(const sead::SafeString& rStr, const char** ppText);
};

}  // namespace agl::detail
