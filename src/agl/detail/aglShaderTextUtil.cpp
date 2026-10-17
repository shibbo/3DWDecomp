#include "detail/aglShaderTextUtil.h"

#include <cstring>

#include <attributes.h>

#include <basis/seadNew.h>
#include <container/seadStrTreeMap.h>
#include <prim/seadMemUtil.h>

namespace agl::detail {

namespace {

inline bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

inline const char* findChar(const char* p, char c) {
    while (*p != '\0') {
        if (*p == c) {
            return p;
        }

        p++;
    }

    return nullptr;
}

inline const char* skipSpace(const char* p) {
    while (isSpace(*p)) {
        p++;
    }

    return *p == '\0' ? nullptr : p;
}

/**
 * Checks whether a character is an ASCII letter.
 * @param c character to check
 * @return whether the character is a letter
 */
inline bool isAlphabet(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

/**
 * Checks whether a character can start a GLSL identifier.
 * @param c character to check
 * @return whether the character is a letter or an underscore
 */
inline bool isIdentifierHead(char c) {
    return isAlphabet(c) || c == '_';
}

/**
 * Checks whether a character can be part of a GLSL identifier.
 * @param c character to check
 * @return whether the character is a digit, a letter or an underscore
 */
inline bool isIdentifierChar(char c) {
    return ('0' <= c && c <= '9') || isIdentifierHead(c);
}

/**
 * Checks whether a character is a valid GLSL vector size.
 * @param c character to check
 * @return whether the character is 2, 3 or 4
 */
inline bool isVectorSize(char c) {
    return '2' <= c && c <= '4';
}

/**
 * Checks whether a character is a valid GLSL matrix dimension.
 * @param c character to check
 * @return whether the character is 1, 2, 3 or 4
 */
inline bool isMatrixSize(char c) {
    return '1' <= c && c <= '4';
}

/**
 * Gets the part of a string after an offset, or an empty string if the offset is out of range.
 * @param rStr string to get the part of
 * @param at offset of the part
 * @return part of the string
 */
inline sead::SafeString getRest(const sead::SafeString& rStr, s32 at) {
    const s32 length = rStr.calcLength();

    return sead::SafeString(at < 0 || at > length ? &sead::SafeString::cNullChar :
                                                    rStr.getStringTop() + at);
}

const char* skipTypeName(const char* p);

/**
 * Uniform declaration found in a shader source.
 */
struct UniformInfo {
    const char* mTypeBegin;
    const char* mTypeEnd;
    const char* mNameBegin;
    const char* mNameEnd;
};

typedef sead::FixedStrTreeMap<64, UniformInfo, 256> UniformMap;

/**
 * Shader source processed by createUniformRegisterReplaceText.
 */
struct UniformSource {
    sead::SafeString mText;
    sead::BufferedSafeString* mOutput;
    s64 mInsertPos;
    sead::SafeString** mppResult;
};

}  // namespace

// NON_MATCHING: register allocation of the line feed length and the matched macro index
void ShaderTextUtil::replaceMacro(sead::BufferedSafeString* pText, const char* const* pMacros,
                                  const char* const* pValues, s32 macroNum, char* pWork,
                                  s32 workSize) {
    bool isReplaced[1024];

    for (s32 i = 0; i < macroNum; i++) {
        isReplaced[i] = false;
    }

    const char* src = pText->cstr();
    char* dst = pWork;

    s32 replacedNum = 0;

    for (;;) {
        s32 lineFeedLength;
        s32 lineFeedPos;
        s32 i;
        const char* macro;

        for (;;) {
            lineFeedPos = findLineFeedCode(src, &lineFeedLength);

            if (lineFeedPos == -1) {
                goto end;
            }

            if (*src == '#') {
                const char* p = src + 1;

                while (isSpace(*p)) {
                    p++;
                }

                if (p[0] == 'd' && p[1] == 'e' && p[2] == 'f' && p[3] == 'i' && p[4] == 'n' &&
                    p[5] == 'e' && (p[6] == ' ' || p[6] == '\t')) {
                    const char* name = p + 7;

                    while (isSpace(*name)) {
                        name++;
                    }

                    for (i = 0; i < macroNum; i++) {
                        if (isReplaced[i]) {
                            continue;
                        }

                        macro = pMacros[i];
                        bool match = true;
                        s32 j = 0;

                        for (; macro[j] != '\0'; j++) {
                            if (name[j] != macro[j]) {
                                match = false;
                                break;
                            }
                        }

                        if (match && (name[j] == ' ' || name[j] == '\t')) {
                            break;
                        }
                    }

                    if (i < macroNum) {
                        break;
                    }
                }
            }

            sead::MemUtil::copy(dst, src, lineFeedPos + lineFeedLength);
            dst += lineFeedPos + lineFeedLength;
            *dst = '\0';
            src += lineFeedPos + lineFeedLength;
        }

        dst += sead::BufferedSafeString(dst, workSize - s32(dst - pWork))
                   .format("#define %s %s", macro, pValues[i]);

        for (s32 k = 0; k < lineFeedLength; k++) {
            dst += sead::BufferedSafeString(dst, workSize - s32(dst - pWork))
                       .append(src[lineFeedPos + k]);
        }

        isReplaced[i] = true;
        src += lineFeedPos + lineFeedLength;

        replacedNum++;

        if (replacedNum == macroNum) {
            break;
        }
    }

end:
    while (*src != '\0') {
        *dst++ = *src++;
    }

    *dst = '\0';

    pText->copy(sead::SafeString(pWork));
}

/**
 * Finds the first line feed code in a text.
 * @param pText text to search
 * @param pLength receives the length of the line feed code (1 or 2), may be nullptr
 * @return offset of the line feed code, or -1 if there is none
 */
s32 ShaderTextUtil::findLineFeedCode(const char* pText, s32* pLength) {
    for (s32 i = 0; pText[i] != '\0'; i++) {
        const char* p = pText + i;
        s32 length;

        if (*p == '\n') {
            length = 1;
        } else if (*p == '\r') {
            length = p[1] == '\n' ? 2 : 1;
        } else {
            continue;
        }

        if (pLength) {
            *pLength = length;
        }

        return i;
    }

    return -1;
}

/**
 * Replaces the range [begin, end) of a text with another string.
 * @param pText text to modify in place
 * @param pInsert string to insert
 * @param begin offset of the first replaced character
 * @param end offset after the last replaced character
 * @param pWork work buffer receiving the text after the range
 * @param workSize size of the work buffer
 */
void ShaderTextUtil::replace(char* pText, const char* pInsert, s32 begin, s32 end, void* pWork,
                             s32 workSize) {
    char* work = static_cast<char*>(pWork);
    char* dst = work;

    for (const char* src = pText + end; *src != '\0'; src++) {
        *dst++ = *src;
    }

    *dst = '\0';

    dst = pText + begin;

    for (const char* src = pInsert; *src != '\0'; src++) {
        *dst++ = *src;
    }

    for (const char* src = work; *src != '\0'; src++) {
        *dst++ = *src;
    }

    *dst = '\0';
}

/**
 * Checks whether a text starts with a UTF-8 byte order mark.
 * @param pText text to check
 * @return whether the text starts with a UTF-8 byte order mark
 */
bool ShaderTextUtil::isUTF8(const char* pText) {
    return static_cast<u8>(pText[0]) == 0xef && static_cast<u8>(pText[1]) == 0xbb &&
           static_cast<u8>(pText[2]) == 0xbf;
}

// NON_MATCHING: include search loop structure and register allocation
sead::HeapSafeString* ShaderTextUtil::createRawText(const sead::SafeString& rText,
                                                    const char* const* pSourceNames,
                                                    const char* const* pSourceTexts,
                                                    s32 sourceNum, bool* pUsedFlags,
                                                    sead::Heap* pHeap) {
    if (pUsedFlags != nullptr) {
        for (s32 i = 0; i < sourceNum; i++) {
            pUsedFlags[i] = false;
        }
    }

    sead::HeapSafeString* text = new (pHeap) sead::HeapSafeString(pHeap, rText);
    s32 length = text->calcLength();
    const char* src = text->cstr();

    while (*src != '\0') {
        const char* const directiveBegin = findChar(src, '#') + 1;

        if (directiveBegin - 1 == nullptr) {
            break;
        }

        const char* directive = skipSpace(directiveBegin);

        if (directive[0] == 'i' && directive[1] == 'n' && directive[2] == 'c' &&
            directive[3] == 'l' && directive[4] == 'u' && directive[5] == 'd' &&
            directive[6] == 'e') {
            const char* const nameBegin = findChar(directive + 7, '"') + 1;

            if (nameBegin - 1 == nullptr) {
                continue;
            }

            const char* const includeEnd = findChar(nameBegin, '"') + 1;

            if (includeEnd - 1 == nullptr) {
                continue;
            }

            sead::FixedSafeString<1024> name;
            name.copy(nameBegin, s32(includeEnd - nameBegin) - 1);

            s32 i = 0;

            for (; i < sourceNum; i++) {
                if (name.isEqual(pSourceNames[i])) {
                    break;
                }
            }

            if (i >= sourceNum) {
                break;
            }

            const char* source = pSourceTexts[i];

            if (pUsedFlags != nullptr) {
                pUsedFlags[i] = true;
            }

            if (source == nullptr) {
                break;
            }

            if (isUTF8(source)) {
                source += 3;
            }

            const s32 sourceLength = sead::SafeString(source).calcLength();
            sead::HeapSafeString* newText =
                new (pHeap) sead::HeapSafeString(pHeap, sourceLength + length + 1);
            newText->copy(*text);

            const char* top = text->cstr();
            char* work = new (pHeap) char[length + 1];
            replace(const_cast<char*>(newText->cstr()), source, s32(directiveBegin - top) - 1,
                    s32(includeEnd - top), work, length + 1);
            delete[] work;

            delete text;
            text = new (pHeap) sead::HeapSafeString(pHeap, *newText);
            delete newText;

            src = text->cstr();
            length = text->calcLength();
        } else {
            src = directive;
        }
    }

    return text;
}

/**
 * Expands every uniform block of a shader source into plain uniform declarations: the members of
 * `uniform Name { ... };` are each prefixed with "uniform " (nested struct declarations included)
 * and the block braces are dropped.
 * @param rText shader source to convert
 * @param pHeap heap to allocate the work buffers and the result from
 * @return converted shader source
 */
// NON_MATCHING: block layout of the member loop and stack slot assignment
sead::HeapSafeString* ShaderTextUtil::createUniformBufferReplaceText(const sead::SafeString& rText,
                                                                     sead::Heap* pHeap) {
    auto* text = new (pHeap, -4) sead::BufferedSafeString(
        new (pHeap, -4) char[rText.calcLength() * 2], rText.calcLength() * 2);
    sead::SafeString pending = rText;
    sead::SafeString line = rText;
    text->clear();

    s64 offset = 0;

    while (!line.isEmpty()) {
        s32 lineFeedLength = 0;
        const s32 lineFeedPos = findLineFeedCode(line.cstr(), &lineFeedLength);
        u32 lineLength = lineFeedPos == -1 ? line.calcLength() : lineFeedPos + lineFeedLength;

        const char* comment = line.cstr();
        bool hasComment = false;

        for (const char* end = comment + lineLength; comment < end; comment++) {
            if (comment[0] == '/' && comment[1] == '*') {
                lineLength = comment - line.cstr();
                hasComment = true;
                break;
            }
        }

        if (!hasComment) {
            comment = nullptr;
        }

        const char* lineTop = line.cstr();
        const char* p = lineTop;

        while (isSpace(*p)) {
            p++;
        }

        if (p == nullptr || *p == '\0' || *p == '#' || (p[0] == '/' && p[1] == '/')) {
            goto skip;
        }

        for (;;) {
            if (p > lineTop + lineLength) {
                goto skip;
            }

            if (isAlphabet(*p)) {
                if (sead::SafeString("uniform").comparen(p, 7) == 0) {
                    break;
                }

                do {
                    p++;
                } while (isIdentifierChar(*p));
            } else {
                const char c = *p++;

                if (c == '/' && (*p == '*' || *p == '/')) {
                    goto skip;
                }
            }

            while (isSpace(*p)) {
                p++;
            }

            if (*p == '\0') {
                goto skip;
            }
        }

        goto found;

    skip:
        offset += lineLength;
        line = getRest(line, lineLength);
        goto next;

    found: {
            const char* name = skipSpace(p + 7);

            if (name == nullptr) {
                goto skip;
            }

            for (const char* q = name;; q++) {
                const char c = *q;

                if (c == '\0' || c == ';') {
                    goto skip;
                }

                if (c == '\n' || c == '\r' || (c == '/' && q[1] == '/')) {
                    break;
                }
            }

            const char* brace = name;

            while (*brace != '{') {
                if (*brace == '\0') {
                    goto skip;
                }

                brace++;
            }

            const char* q = brace;
            s32 depth = 0;

            for (;;) {
                char c = *++q;

                if (c == '/' && q[1] == '/') {
                    do {
                        c = *++q;

                        if (c == '\0') {
                            goto skip;
                        }
                    } while (c != '\r' && c != '\n');
                } else if (c == '\0') {
                    goto skip;
                }

                if (c == '{') {
                    depth++;
                } else if (c == '}') {
                    if (depth == 0) {
                        break;
                    }

                    depth--;
                }
            }

            bool hasInstanceName = false;
            const char* end = q + 1;

            for (; *end != ';'; end++) {
                if (*end == '\0') {
                    goto skip;
                }

                hasInstanceName = hasInstanceName || isIdentifierHead(*end);
            }

            end++;

            text->append(pending, offset);
            pending = getRest(pending, end + offset - line.cstr());
            line = sead::SafeString(end);

            s32 semicolonNum = 0;

            for (const char* r = brace; r != end; r++) {
                if (*r == ';') {
                    semicolonNum++;
                }
            }

            const u32 size = s32(end - brace) + semicolonNum * 9 + text->calcLength() +
                             line.calcLength() + 1;
            char* work = new (pHeap, -4) char[size];
            u32 pos = sead::BufferedSafeString(work, size).copy(*text);

            const char* const stop = hasInstanceName ? end : end - 1;
            bool isLineHead = true;

            for (const char* r = brace + 1; r < stop; r++) {
                if (r[0] == '/' && r[1] == '/') {
                    const char* e = r;

                    while (*e != '\r' && *e != '\n') {
                        e++;
                    }

                    pos += sead::BufferedSafeString(work + pos, size - pos)
                               .append(sead::SafeString(r), e - r);
                    r = e;
                }

                if (r[0] == '/' && r[1] == '*') {
                    char prev = r[1];
                    const char* e = r + 2;

                    for (;;) {
                        const char cur = *e++;

                        if (prev == '*' && cur == '/') {
                            break;
                        }

                        prev = cur;
                    }

                    e++;

                    pos += sead::BufferedSafeString(work + pos, size - pos)
                               .append(sead::SafeString(r), e - r);
                    r = e;
                }

                const char c = *r;

                if (c == '{' || c == '}') {
                    continue;
                }

                bool keepLineHead = false;

                if (isLineHead) {
                    if (strncmp(r, "struct", 6) == 0) {
                        pos += sead::BufferedSafeString(work + pos, size - pos).append("uniform ");

                        while (*r != '{') {
                            if (*r == '\0') {
                                goto structEnd;
                            }

                            pos += sead::BufferedSafeString(work + pos, size - pos).append(*r++);
                        }

                        {
                            s32 structDepth = -1;

                            do {
                                pos += sead::BufferedSafeString(work + pos, size - pos).append(*r);

                                if (*r == '{') {
                                    structDepth++;
                                } else if (*r == '}') {
                                    if (structDepth == 0) {
                                        break;
                                    }

                                    structDepth--;
                                }
                            } while (*++r != '\0');
                        }

                    structEnd:
                        isLineHead = false;
                        continue;
                    }

                    keepLineHead = true;

                    if (isIdentifierHead(c)) {
                        const char* type = r;

                        if (sead::SafeString("uniform").comparen(r, 7) == 0 &&
                            IsDelimiter(r[7])) {
                            type = r + 7;

                            while (isSpace(*type)) {
                                type++;
                            }
                        }

                        if (skipTypeName(type) != nullptr) {
                            const char* s = type;
                            char next;

                            do {
                                next = *s++;
                            } while (isIdentifierChar(next));

                            while (isSpace(next)) {
                                next = *s++;
                            }

                            if (isIdentifierHead(next)) {
                                if (!(sead::SafeString("uniform").comparen(r, 7) == 0 &&
                                      IsDelimiter(r[7]))) {
                                    pos += sead::BufferedSafeString(work + pos, size - pos)
                                               .append("uniform ");
                                }

                                keepLineHead = false;
                            }
                        }
                    }
                }

                isLineHead = keepLineHead | (*r == ';');
                pos += sead::BufferedSafeString(work + pos, size - pos).append(*r);
            }

            delete[] text->cstr();
            delete text;
            text = new (pHeap, -4) sead::BufferedSafeString(work, size);
            offset = 0;
        }

    next:
        if (hasComment) {
            const char* q = comment;

            for (; *q != '\0'; q++) {
                if (*q == '*' && *++q == '/') {
                    q++;
                    break;
                }
            }

            offset += q - comment;
            line = sead::SafeString(q);
        }
    }

    text->append(pending, offset);

    auto* result = new (pHeap) sead::HeapSafeString(pHeap, text->cstr());
    delete[] text->cstr();
    delete text;

    return result;
}

/**
 * Checks whether a character separates tokens in shader source.
 * @param c character to check
 * @return whether the character is a delimiter
 */
WEAK bool IsDelimiter(char c) {
    switch (c) {
    case '\0':
    case '\t':
    case '\n':
    case '\r':
    case ' ':
    case '!':
    case '"':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case '-':
    case '.':
    case '/':
    case ':':
    case ';':
    case '<':
    case '=':
    case '>':
    case '?':
    case '@':
    case '[':
    case '\\':
    case ']':
    case '^':
    case '`':
    case '{':
    case '|':
    case '}':
    case '~':
        return true;
    default:
        return false;
    }
}

/**
 * Moves the plain uniform declarations of a vertex and a fragment shader into one shared
 * std140 uniform block, inserted where each shader declared its first uniform.
 * @param ppVertexText receives the converted vertex shader source
 * @param ppFragmentText receives the converted fragment shader source
 * @param rVertexSource vertex shader source
 * @param rFragmentSource fragment shader source
 * @param rBlockName name of the generated uniform block
 * @param pHeap heap to allocate the work buffers and the results from
 */
// NON_MATCHING: block layout of the comment handling and register allocation
void ShaderTextUtil::createUniformRegisterReplaceText(sead::SafeString** ppVertexText,
                                                      sead::SafeString** ppFragmentText,
                                                      const sead::SafeString& rVertexSource,
                                                      const sead::SafeString& rFragmentSource,
                                                      const sead::SafeString& rBlockName,
                                                      sead::Heap* pHeap) {
    UniformSource sources[2] = {{rVertexSource, nullptr, -1, ppVertexText},
                                {rFragmentSource, nullptr, -1, ppFragmentText}};

    const s32 vertexLength = rVertexSource.calcLength();
    const s32 fragmentLength = rFragmentSource.calcLength();
    const s32 maxLength = vertexLength > fragmentLength ? vertexLength : fragmentLength;

    auto* uniformText = new (pHeap, -4)
        sead::BufferedSafeString(new (pHeap, -4) char[maxLength * 2], maxLength * 2);
    auto* map = new (pHeap, -4) UniformMap;
    uniformText->clear();

    for (s32 i = 0; i < 2; i++) {
        UniformSource& source = sources[i];
        sead::SafeString line = source.mText;
        sead::SafeString pending = source.mText;
        const s32 length = source.mText.calcLength();

        source.mOutput = new (pHeap, -4)
            sead::BufferedSafeString(new (pHeap, -4) char[length * 2], length * 2);
        source.mOutput->clear();

        s64 offset = 0;

        while (!line.isEmpty()) {
            s32 lineFeedLength = 0;
            const s32 lineFeedPos = findLineFeedCode(line.cstr(), &lineFeedLength);
            const char* top = line.cstr();
            u32 lineLength = lineFeedPos == -1 ? line.calcLength() : lineFeedPos + lineFeedLength;

            const char* comment = line.cstr();
            bool hasComment = false;

            for (const char* end = comment + lineLength; comment < end; comment++) {
                if (comment[0] == '/' && comment[1] == '*') {
                    lineLength = comment - line.cstr();
                    hasComment = true;
                    break;
                }
            }

            if (!hasComment) {
                comment = nullptr;
            }

            const char* lineTop = line.cstr();
            const char* p = lineTop;

            while (isSpace(*p)) {
                p++;
            }

            if (p == nullptr || *p == '\0' || *p == '#' || (p[0] == '/' && p[1] == '/')) {
                goto skip;
            }

            for (;;) {
                if (p >= lineTop + lineLength) {
                    goto skip;
                }

                if (isAlphabet(*p)) {
                    if (sead::SafeString("uniform").comparen(p, 7) == 0) {
                        break;
                    }

                    do {
                        p++;
                    } while (isIdentifierChar(*p));
                } else {
                    const char c = *p++;

                    if (c == '/' && (*p == '*' || *p == '/')) {
                        goto skip;
                    }
                }

                while (isSpace(*p)) {
                    p++;
                }

                if (*p == '\0') {
                    goto skip;
                }
            }

            goto found;

        skip:
            offset += lineLength;
            line = getRest(line, lineLength);
            goto next;

        found: {
                const char* type = skipSpace(p + 7);

                if (type == nullptr) {
                    goto skip;
                }

                for (const char* q = type;; q++) {
                    const char c = *q;

                    if (c == '\0' || c == '\n' || c == '\r' || (c == '/' && q[1] == '/')) {
                        goto skip;
                    }

                    if (c == ';') {
                        break;
                    }
                }

                const char* typeEnd = skipTypeName(type);

                if (typeEnd == nullptr) {
                    goto skip;
                }

                const char* nameBegin = typeEnd;

                while (isSpace(*nameBegin)) {
                    nameBegin++;
                }

                if (*nameBegin == '\0') {
                    goto skip;
                }

                const char* nameEnd = nameBegin;

                while (isIdentifierChar(*nameEnd)) {
                    nameEnd++;
                }

                source.mOutput->append(pending, offset);

                if (source.mInsertPos == -1) {
                    source.mInsertPos = source.mOutput->calcLength();
                }

                sead::FixedSafeString<64> name;
                name.copy(sead::SafeString(nameBegin), nameEnd - nameBegin);

                if (map->find(name) == nullptr) {
                    sead::FixedSafeString<256> declaration;
                    declaration.copy(sead::SafeString(type), top + lineLength - type);

                    UniformInfo info = {type, typeEnd, nameBegin, nameEnd};
                    map->insert(name, info);
                    uniformText->appendWithFormat("\t%s", declaration.cstr());
                }

                line = getRest(line, lineLength);
                pending = line;
                offset = 0;
            }

        next:
            if (hasComment) {
                const char* q = comment;

                for (; *q != '\0'; q++) {
                    if (*q == '*' && *++q == '/') {
                        q++;
                        break;
                    }
                }

                line = sead::SafeString(q);
                offset += line.cstr() - comment;
            }
        }

        source.mOutput->append(pending, offset);
    }

    auto* blockText = new (pHeap, -4)
        sead::BufferedSafeString(new (pHeap, -4) char[maxLength * 2], maxLength * 2);
    blockText->format("layout( std140 ) uniform %s\r\n{\r\n%s};", rBlockName.cstr(),
                      uniformText->cstr());

    for (s32 i = 0; i < 2; i++) {
        UniformSource& source = sources[i];

        if (source.mInsertPos != -1) {
            uniformText->copy(*source.mOutput, source.mInsertPos);
            uniformText->appendWithFormat("\r\n%s\r\n%s", blockText->cstr(),
                                          source.mOutput->getPart(source.mInsertPos).cstr());
            *source.mppResult = new (pHeap) sead::HeapSafeString(pHeap, uniformText->cstr());
        } else {
            *source.mppResult = new (pHeap) sead::HeapSafeString(pHeap, source.mOutput->cstr());
        }

        delete source.mOutput->cstr();
        delete source.mOutput;
    }

    delete blockText->cstr();
    delete blockText;
    delete uniformText->cstr();
    delete uniformText;
    delete map;
}

/**
 * Constructs an empty analyze result.
 */
ShaderTextUtil::ShaderDumpTextAnalyzeResult::ShaderDumpTextAnalyzeResult()
    : mAluClauseInstNum(0), mTexClauseInstNum(0), mExportNum(0), mVaryingInNum(0),
      mVaryingOutNum(0), mDisassembly(nullptr), mDisassemblySize(0) {}

// NON_MATCHING: clause loop layout and register allocation in the symbol section
void ShaderTextUtil::analyzeShaderDumpText(const sead::SafeString& rText,
                                           ShaderDumpTextAnalyzeResult* pResult) {
    ShaderDumpTextAnalyzeResult result;

    const char* p = "";

    for (const char* text = rText.cstr(); *text != '\0'; text++) {
        if (text[0] == ';' && text[1] == ' ' && text[2] == '-' && text[3] == '-') {
            p = text;
            result.mDisassembly = text;
        }
    }

    s32 exportNum = 0;
    s32 clauseNum = 0;
    s32* counter = nullptr;

    while (*p != '\0') {
        if (p[0] == 'E' && clauseNum > 0) {
            if (p[1] == 'N' && p[2] == 'D' && p[3] == '_' && p[4] == 'O' && p[5] == 'F') {
                while (*p != '\n') {
                    p++;
                }

                result.mDisassemblySize = p - result.mDisassembly;
                break;
            }
        } else if ('0' <= *p && *p <= '9') {
            s32 digit[256];
            s32 digitNum;

            if (clauseNum < 100) {
                digit[0] = clauseNum / 10;
                digit[1] = clauseNum - digit[0] * 10;
                digitNum = 2;
            } else if (clauseNum < 1000) {
                digit[0] = clauseNum / 100;
                digit[1] = (clauseNum - digit[0] * 100) / 10;
                digit[2] = clauseNum - digit[0] * 100 - digit[1] * 10;
                digitNum = 3;
            } else {
                digit[0] = clauseNum / 1000;
                digit[1] = (clauseNum - digit[0] * 1000) / 100;
                digit[2] = clauseNum - digit[0] * 1000 - digit[1] * 100;
                digit[3] = clauseNum - digit[0] * 1000 - digit[1] * 100 - digit[2] * 10;
                digitNum = 4;
            }

            for (s32 i = 0; digit[i] == *p - '0'; p++) {
                if (++i >= digitNum) {
                    const char* type = p + 2;

                    if (type[0] == 'A' && type[1] == 'L' && type[2] == 'U') {
                        counter = &result.mAluClauseInstNum;
                    } else if (type[0] == 'E' && type[1] == 'X' && type[2] == 'P') {
                        exportNum++;
                    } else if (type[0] == 'T' && type[1] == 'E' && type[2] == 'X') {
                        counter = &result.mTexClauseInstNum;
                    } else {
                        counter = nullptr;
                    }

                    clauseNum++;
                    break;
                }
            }
        } else if (*p == ' ') {
            skipChar(' ', &p);

            if ('0' <= *p && *p <= '9') {
                while ('0' <= *p && *p <= '9') {
                    p++;
                }

                if (counter) {
                    (*counter)++;
                }
            }
        }

        char c;

        do {
            c = *p++;
        } while (c != '\0' && c != '\n');

        if (c != '\n') {
            p--;
        }
    }

    const sead::SafeString name = "Name: ";
    const sead::SafeString symbolType = "Symbol Type: ";
    const sead::SafeString dataType = "Data Type :";
    const sead::SafeString attrib = "ATTRIB";
    const sead::SafeString uniformBlock = "UNIFORM_BLOCK";
    const sead::SafeString uniform = "UNIFORM";
    const sead::SafeString varyingIn = "VARYING IN";
    const sead::SafeString varyingOut = "VARYING OUT";
    const sead::SafeString samplerImage = "SAMPLER_IMAGE";

    findFirstChar('-', &p);
    skipChar('-', &p);
    skipFirstMatchedString("Symbol Section ", &p);
    skipChar('-', &p);

    s32 varyingInNum = 0;
    s32 varyingOutNum = 0;

    while (*p != '\0') {
        skipChar(' ', &p);
        skipFirstMatchedString(name, &p);
        skipFirstMatchedString(symbolType, &p);

        if (matchString(attrib, p)) {
            skipString(attrib, &p);
        } else if (matchString(uniformBlock, p)) {
            skipString(uniformBlock, &p);
        } else if (matchString(uniform, p)) {
            skipString(uniform, &p);
            skipFirstMatchedString(dataType, &p);

            if (matchString(samplerImage, p)) {
                skipString(samplerImage, &p);
            }
        } else if (matchString(varyingIn, p)) {
            skipString(varyingIn, &p);
            varyingInNum++;
        } else if (matchString(varyingOut, p)) {
            skipString(varyingOut, &p);
            varyingOutNum++;
        }

        skipChar(' ', &p);
    }

    result.mExportNum = exportNum;
    result.mVaryingInNum = varyingInNum;
    result.mVaryingOutNum = varyingOutNum;
    *pResult = result;
}

/**
 * Advances a text pointer to the first occurrence of a character or the end of the text.
 * @param c character to find
 * @param ppText text pointer to advance
 */
void ShaderTextUtil::findFirstChar(char c, const char** ppText) {
    while (**ppText != c && **ppText != '\0') {
        (*ppText)++;
    }
}

/**
 * Advances a text pointer past all leading occurrences of a character.
 * @param c character to skip
 * @param ppText text pointer to advance
 */
void ShaderTextUtil::skipChar(char c, const char** ppText) {
    while (**ppText == c && **ppText != '\0') {
        (*ppText)++;
    }
}

bool ShaderTextUtil::skipFirstMatchedString(const sead::SafeString& rStr, const char** ppText) {
    s32 length = rStr.calcLength();
    char c = **ppText;

    if (c == '\0') {
        return false;
    }

    do {
        for (s32 i = 0; i < length; i++) {
            if (c == '\0') {
                return false;
            }

            if (rStr.at(i) != c) {
                break;
            }

            (*ppText)++;

            if (i == length - 1) {
                return true;
            }

            c = **ppText;
        }

        c = *++(*ppText);
    } while (c != '\0');
    return false;
}

/**
 * Checks whether a text starts with a string.
 * @param rStr string to match
 * @param pText text to check
 * @return whether the text starts with the string
 */
bool ShaderTextUtil::matchString(const sead::SafeString& rStr, const char* pText) {
    s32 length = rStr.calcLength();

    for (s32 i = 0; i < length; i++, pText++) {
        if (*pText == '\0' || *pText != rStr.at(i)) {
            return false;
        }

        if (i == length - 1) {
            return true;
        }
    }

    return false;
}

/**
 * Advances a text pointer over a string it starts with, stopping on its last character.
 * @param rStr string to skip
 * @param ppText text pointer to advance
 * @return whether the text started with the string
 */
bool ShaderTextUtil::skipString(const sead::SafeString& rStr, const char** ppText) {
    s32 length = rStr.calcLength();

    for (s32 i = 0; i < length; i++) {
        if (**ppText == '\0' || **ppText != rStr.at(i)) {
            return false;
        }

        if (i == length - 1) {
            return true;
        }

        (*ppText)++;
    }

    return false;
}

namespace {

/**
 * Skips a GLSL scalar, vector or matrix type name.
 * @param p text starting with the type name
 * @return end of the type name if it is followed by a space, otherwise nullptr
 */
const char* skipTypeName(const char* p) {
    const char* end = p;

    switch (*p) {
    case 'b':
        if (sead::SafeString(p + 1).comparen("vec", 3) == 0) {
            if (!isVectorSize(p[4])) {
                return nullptr;
            }

            end = p + 5;
        } else if (sead::SafeString(p).comparen("bool", 4) == 0) {
            end = p + 4;
        }

        break;
    case 'd':
        if (sead::SafeString(p + 1).comparen("vec", 3) == 0) {
            if (!isVectorSize(p[4])) {
                return nullptr;
            }

            end = p + 5;
        } else if (sead::SafeString(p + 1).comparen("mat", 3) == 0) {
            if (!isMatrixSize(p[4])) {
                return nullptr;
            }

            end = p + 5;

            if (*end == 'x') {
                if (!isMatrixSize(p[6])) {
                    return nullptr;
                }

                end = p + 7;
            }
        } else if (sead::SafeString(p).comparen("double", 6) == 0) {
            end = p + 6;
        }

        break;
    case 'f':
        if (sead::SafeString(p).comparen("float", 5) == 0) {
            end = p + 5;
        }

        break;
    case 'i':
        if (sead::SafeString(p + 1).comparen("vec", 3) == 0) {
            if (!isVectorSize(p[4])) {
                return nullptr;
            }

            end = p + 5;
        } else if (sead::SafeString(p).comparen("int", 3) == 0) {
            end = p + 3;
        }

        break;
    case 'm':
        if (sead::SafeString(p).comparen("mat", 3) == 0) {
            if (!isMatrixSize(p[3])) {
                return nullptr;
            }

            end = p + 4;

            if (*end == 'x') {
                if (!isMatrixSize(p[5])) {
                    return nullptr;
                }

                end = p + 6;
            }
        }

        break;
    case 'u':
        if (sead::SafeString(p + 1).comparen("vec", 3) == 0) {
            if (!isVectorSize(p[4])) {
                return nullptr;
            }

            end = p + 5;
        } else if (sead::SafeString(p + 1).comparen("int", 3) == 0) {
            end = p + 4;
        }

        break;
    case 'v':
        if (sead::SafeString(p).comparen("vec", 3) == 0) {
            if (!isVectorSize(p[3])) {
                return nullptr;
            }

            end = p + 4;
        }

        break;
    }

    return isSpace(*end) ? end : nullptr;
}

}  // namespace

}  // namespace agl::detail
