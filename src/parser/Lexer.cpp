/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "Escargot.h"
#include "parser/Lexer.h"

// These two must be the last because they overwrite the ASSERT macro.
#include "double-conversion.h"
#include "ieee.h"

using namespace Escargot::EscargotLexer;

namespace Escargot {

const char* Messages::InvalidHexEscapeSequence = "Invalid hexadecimal escape sequence";
const char* Messages::UnexpectedTokenIllegal = "Unexpected token ILLEGAL";
const char* Messages::UnterminatedRegExp = "Invalid regular expression: missing /";
const char* Messages::TemplateOctalLiteral = "Octal literals are not allowed in template strings.";

#define IDENT_RANGE_LONG 200

/* The largest code-point that an UTF16 surrogate pair can represent is 0x10ffff,
 * so any codepoint above this can be a valid value for empty. The UINT32_MAX is
 * chosen because it is a valid immediate for machine instructions. */
#define EMPTY_CODE_POINT UINT32_MAX

/* The largest octal value is 255, so any higher
 * value can represent an invalid octal value. */
#define NON_OCTAL_VALUE 256

char EscargotLexer::g_asciiRangeCharMap[128] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    LexerIsCharWhiteSpace,
    LexerIsCharLineTerminator,
    LexerIsCharWhiteSpace,
    LexerIsCharWhiteSpace,
    LexerIsCharLineTerminator,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    LexerIsCharWhiteSpace,
    0,
    0,
    0,
    LexerIsCharIdentStart | LexerIsCharIdent,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    LexerIsCharIdent,
    LexerIsCharIdent,
    LexerIsCharIdent,
    LexerIsCharIdent,
    LexerIsCharIdent,
    LexerIsCharIdent,
    LexerIsCharIdent,
    LexerIsCharIdent,
    LexerIsCharIdent,
    LexerIsCharIdent,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    0,
    LexerIsCharIdentStart | LexerIsCharIdent,
    0,
    0,
    LexerIsCharIdentStart | LexerIsCharIdent,
    0,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    LexerIsCharIdentStart | LexerIsCharIdent,
    0,
    0,
    0,
    0,
    0
};

NEVER_INLINE bool EscargotLexer::isWhiteSpaceSlowCase(char16_t ch)
{
    ASSERT(ch >= 0x80);

    if (LIKELY(ch < 0x1680)) {
        return (ch == 0xA0);
    }

    return (ch == 0x1680 || ch == 0x180E || ch == 0x2000 || ch == 0x2001
            || ch == 0x2002 || ch == 0x2003 || ch == 0x2004 || ch == 0x2005 || ch == 0x2006
            || ch == 0x2007 || ch == 0x2008 || ch == 0x2009 || ch == 0x200A || ch == 0x202F
            || ch == 0x205F || ch == 0x3000 || ch == 0xFEFF);
}

/* Starting codepoints of identifier ranges. */
static const uint16_t identRangeStart[429] = {
    170, 181, 183, 186, 192, 216, 248, 710, 736, 748, 750, 768, 886, 890, 895, 902, 908, 910, 931, 1015, 1155, 1162,
    1329, 1369, 1377, 1425, 1471, 1473, 1476, 1479, 1488, 1520, 1552, 1568, 1646, 1749, 1759, 1770, 1791, 1808, 1869,
    1984, 2042, 2048, 2112, 2208, 2276, 2406, 2417, 2437, 2447, 2451, 2474, 2482, 2486, 2492, 2503, 2507, 2519, 2524,
    2527, 2534, 2561, 2565, 2575, 2579, 2602, 2610, 2613, 2616, 2620, 2622, 2631, 2635, 2641, 2649, 2654, 2662, 2689,
    2693, 2703, 2707, 2730, 2738, 2741, 2748, 2759, 2763, 2768, 2784, 2790, 2817, 2821, 2831, 2835, 2858, 2866, 2869,
    2876, 2887, 2891, 2902, 2908, 2911, 2918, 2929, 2946, 2949, 2958, 2962, 2969, 2972, 2974, 2979, 2984, 2990, 3006,
    3014, 3018, 3024, 3031, 3046, 3072, 3077, 3086, 3090, 3114, 3133, 3142, 3146, 3157, 3160, 3168, 3174, 3201, 3205,
    3214, 3218, 3242, 3253, 3260, 3270, 3274, 3285, 3294, 3296, 3302, 3313, 3329, 3333, 3342, 3346, 3389, 3398, 3402,
    3415, 3424, 3430, 3450, 3458, 3461, 3482, 3507, 3517, 3520, 3530, 3535, 3542, 3544, 3558, 3570, 3585, 3648, 3664,
    3713, 3716, 3719, 3722, 3725, 3732, 3737, 3745, 3749, 3751, 3754, 3757, 3771, 3776, 3782, 3784, 3792, 3804, 3840,
    3864, 3872, 3893, 3895, 3897, 3902, 3913, 3953, 3974, 3993, 4038, 4096, 4176, 4256, 4295, 4301, 4304, 4348, 4682,
    4688, 4696, 4698, 4704, 4746, 4752, 4786, 4792, 4800, 4802, 4808, 4824, 4882, 4888, 4957, 4969, 4992, 5024, 5121,
    5743, 5761, 5792, 5870, 5888, 5902, 5920, 5952, 5984, 5998, 6002, 6016, 6103, 6108, 6112, 6155, 6160, 6176, 6272,
    6320, 6400, 6432, 6448, 6470, 6512, 6528, 6576, 6608, 6656, 6688, 6752, 6783, 6800, 6823, 6832, 6912, 6992, 7019,
    7040, 7168, 7232, 7245, 7376, 7380, 7416, 7424, 7676, 7960, 7968, 8008, 8016, 8025, 8027, 8029, 8031, 8064, 8118,
    8126, 8130, 8134, 8144, 8150, 8160, 8178, 8182, 8204, 8255, 8276, 8305, 8319, 8336, 8400, 8417, 8421, 8450, 8455,
    8458, 8469, 8472, 8484, 8486, 8488, 8490, 8508, 8517, 8526, 8544, 11264, 11312, 11360, 11499, 11520, 11559, 11565,
    11568, 11631, 11647, 11680, 11688, 11696, 11704, 11712, 11720, 11728, 11736, 11744, 12293, 12321, 12337, 12344,
    12353, 12441, 12449, 12540, 12549, 12593, 12704, 12784, 13312, 19968, 40960, 42192, 42240, 42512, 42560, 42612,
    42623, 42655, 42775, 42786, 42891, 42896, 42928, 42999, 43072, 43136, 43216, 43232, 43259, 43264, 43312, 43360,
    43392, 43471, 43488, 43520, 43584, 43600, 43616, 43642, 43739, 43744, 43762, 43777, 43785, 43793, 43808, 43816,
    43824, 43868, 43876, 43968, 44012, 44016, 44032, 55216, 55243, 63744, 64112, 64256, 64275, 64285, 64298, 64312,
    64318, 64320, 64323, 64326, 64467, 64848, 64914, 65008, 65024, 65056, 65075, 65101, 65136, 65142, 65296, 65313,
    65343, 65345, 65382, 65474, 65482, 65490, 65498, 65535
};

/* Lengths of identifier ranges. */
static const uint8_t identRangeLength[428] = {
    1, 1, 1, 1, 23, 31, 200, 12, 5, 1, 1, 117, 2, 4, 1, 5, 1, 20, 83, 139, 5, 166, 38, 1, 39, 45, 1, 2, 2, 1, 27, 3,
    11, 74, 102, 8, 10, 19, 1, 59, 101, 54, 1, 46, 28, 19, 128, 10, 19, 8, 2, 22, 7, 1, 4, 9, 2, 4, 1, 2, 5, 12, 3, 6,
    2, 22, 7, 2, 2, 2, 1, 5, 2, 3, 1, 4, 1, 16, 3, 9, 3, 22, 7, 2, 5, 10, 3, 3, 1, 4, 10, 3, 8, 2, 22, 7, 2, 5, 9, 2,
    3, 2, 2, 5, 10, 1, 2, 6, 3, 4, 2, 1, 2, 2, 3, 12, 5, 3, 4, 1, 1, 10, 4, 8, 3, 23, 16, 8, 3, 4, 2, 2, 4, 10, 3, 8,
    3, 23, 10, 5, 9, 3, 4, 2, 1, 4, 10, 2, 3, 8, 3, 41, 8, 3, 5, 1, 4, 10, 6, 2, 18, 24, 9, 1, 7, 1, 6, 1, 8, 10, 2,
    58, 15, 10, 2, 1, 2, 1, 1, 4, 7, 3, 1, 1, 2, 13, 3, 5, 1, 6, 10, 4, 1, 2, 10, 1, 1, 1, 10, 36, 20, 18, 36, 1, 74,
    78, 38, 1, 1, 43, 201, 4, 7, 1, 4, 41, 4, 33, 4, 7, 1, 4, 15, 57, 4, 67, 3, 9, 16, 85, 202, 17, 26, 75, 11, 13, 7,
    21, 20, 13, 3, 2, 84, 1, 2, 10, 3, 10, 88, 43, 70, 31, 12, 12, 40, 5, 44, 26, 11, 28, 63, 29, 11, 10, 1, 14, 76,
    10, 9, 116, 56, 10, 49, 3, 35, 2, 203, 204, 6, 38, 6, 8, 1, 1, 1, 31, 53, 7, 1, 3, 7, 4, 6, 13, 3, 7, 2, 2, 1, 1,
    1, 13, 13, 1, 12, 1, 1, 10, 1, 6, 1, 1, 1, 16, 4, 5, 1, 41, 47, 47, 133, 9, 38, 1, 1, 56, 1, 24, 7, 7, 7, 7, 7, 7,
    7, 7, 32, 3, 15, 5, 5, 86, 7, 90, 4, 41, 94, 27, 16, 205, 206, 207, 46, 208, 28, 48, 10, 31, 83, 9, 103, 4, 30, 2,
    49, 52, 69, 10, 24, 1, 46, 36, 29, 65, 11, 31, 55, 14, 10, 23, 73, 3, 16, 5, 6, 6, 6, 7, 7, 43, 4, 2, 43, 2, 10,
    209, 23, 49, 210, 106, 7, 5, 12, 13, 5, 1, 2, 2, 108, 211, 64, 54, 12, 16, 14, 2, 3, 5, 135, 10, 26, 1, 26, 89, 6,
    6, 6, 3
};

/* Lengths of identifier ranges greater than IDENT_RANGE_LONG. */
static const uint16_t identRangeLongLength[12] = {
    458, 333, 620, 246, 282, 6582, 20941, 1165, 269, 11172, 366, 363
};

static NEVER_INLINE bool isIdentifierPartSlow(char32_t ch)
{
    int bottom = 0;
    int top = (sizeof(identRangeStart) / sizeof(uint16_t)) - 1;

    while (true) {
        int middle = (bottom + top) >> 1;
        char32_t rangeStart = identRangeStart[middle];

        if (ch >= rangeStart) {
            if (ch < identRangeStart[middle + 1]) {
                char32_t length = identRangeLength[middle];

                if (UNLIKELY(length >= IDENT_RANGE_LONG)) {
                    length = identRangeLongLength[length - IDENT_RANGE_LONG];
                }
                return ch < rangeStart + length;
            }

            bottom = middle + 1;
        } else {
            top = middle;
        }

        if (bottom == top) {
            return false;
        }
    }
}

static ALWAYS_INLINE bool isIdentifierPart(char32_t ch)
{
    if (LIKELY(ch < 128)) {
        return g_asciiRangeCharMap[ch] & LexerIsCharIdent;
    }

    return isIdentifierPartSlow(ch);
}

static ALWAYS_INLINE bool isIdentifierStart(char32_t ch)
{
    if (LIKELY(ch < 128)) {
        return g_asciiRangeCharMap[ch] & LexerIsCharIdentStart;
    }

    return isIdentifierPartSlow(ch);
}

static ALWAYS_INLINE bool isDecimalDigit(char16_t ch)
{
    return (ch >= '0' && ch <= '9');
}

static ALWAYS_INLINE bool isHexDigit(char16_t ch)
{
    return isDecimalDigit(ch) || ((ch | 0x20) >= 'a' && (ch | 0x20) <= 'f');
}

static ALWAYS_INLINE bool isOctalDigit(char16_t ch)
{
    return (ch >= '0' && ch <= '7');
}

static ALWAYS_INLINE char16_t octalValue(char16_t ch)
{
    ASSERT(isOctalDigit(ch));
    return ch - '0';
}

static ALWAYS_INLINE uint8_t toHexNumericValue(char16_t ch)
{
    return ch < 'A' ? ch - '0' : ((ch - 'A' + 10) & 0xF);
}

static int hexValue(char16_t ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }

    ASSERT((ch | 0x20) >= 'a' && (ch | 0x20) <= 'f');

    return (ch | 0x20) - ('a' - 10);
}

struct ParserCharPiece {
    char16_t data[3];
    size_t length;

    ParserCharPiece(const char32_t a)
    {
        if (a < 0x10000) {
            data[0] = a;
            data[1] = 0;
            length = 1;
        } else {
            data[0] = (char16_t)(0xD800 + ((a - 0x10000) >> 10));
            data[1] = (char16_t)(0xDC00 + ((a - 0x10000) & 1023));
            data[2] = 0;
            length = 2;
        }
    }
};

AtomicString keywordToString(::Escargot::Context* ctx, KeywordKind keyword)
{
    switch (keyword) {
    case IfKeyword:
        return ctx->staticStrings().stringIf;
    case InKeyword:
        return ctx->staticStrings().stringIn;
    case DoKeyword:
        return ctx->staticStrings().stringDo;
    case VarKeyword:
        return ctx->staticStrings().stringVar;
    case ForKeyword:
        return ctx->staticStrings().stringFor;
    case NewKeyword:
        return ctx->staticStrings().stringNew;
    case TryKeyword:
        return ctx->staticStrings().stringTry;
    case ThisKeyword:
        return ctx->staticStrings().stringThis;
    case ElseKeyword:
        return ctx->staticStrings().stringElse;
    case CaseKeyword:
        return ctx->staticStrings().stringCase;
    case VoidKeyword:
        return ctx->staticStrings().stringVoid;
    case WithKeyword:
        return ctx->staticStrings().stringWith;
    case EnumKeyword:
        return ctx->staticStrings().stringEnum;
    case WhileKeyword:
        return ctx->staticStrings().stringWhile;
    case BreakKeyword:
        return ctx->staticStrings().stringBreak;
    case CatchKeyword:
        return ctx->staticStrings().stringCatch;
    case ThrowKeyword:
        return ctx->staticStrings().stringThrow;
    case ConstKeyword:
        return ctx->staticStrings().stringConst;
    case ClassKeyword:
        return ctx->staticStrings().stringClass;
    case SuperKeyword:
        return ctx->staticStrings().stringSuper;
    case ReturnKeyword:
        return ctx->staticStrings().stringReturn;
    case TypeofKeyword:
        return ctx->staticStrings().stringTypeof;
    case DeleteKeyword:
        return ctx->staticStrings().stringDelete;
    case SwitchKeyword:
        return ctx->staticStrings().stringSwitch;
    case ExportKeyword:
        return ctx->staticStrings().stringExport;
    case ImportKeyword:
        return ctx->staticStrings().stringImport;
    case DefaultKeyword:
        return ctx->staticStrings().stringDefault;
    case FinallyKeyword:
        return ctx->staticStrings().stringFinally;
    case ExtendsKeyword:
        return ctx->staticStrings().stringExtends;
    case FunctionKeyword:
        return ctx->staticStrings().function;
    case ContinueKeyword:
        return ctx->staticStrings().stringContinue;
    case DebuggerKeyword:
        return ctx->staticStrings().stringDebugger;
    case InstanceofKeyword:
        return ctx->staticStrings().stringInstanceof;
    case ImplementsKeyword:
        return ctx->staticStrings().implements;
    case InterfaceKeyword:
        return ctx->staticStrings().interface;
    case PackageKeyword:
        return ctx->staticStrings().package;
    case PrivateKeyword:
        return ctx->staticStrings().stringPrivate;
    case ProtectedKeyword:
        return ctx->staticStrings().stringProtected;
    case PublicKeyword:
        return ctx->staticStrings().stringPublic;
    case StaticKeyword:
        return ctx->staticStrings().stringStatic;
    case YieldKeyword:
        return ctx->staticStrings().yield;
    case LetKeyword:
        return ctx->staticStrings().let;
    default:
        ASSERT_NOT_REACHED();
        return ctx->staticStrings().stringError;
    }
}

void ErrorHandler::throwError(size_t index, size_t line, size_t col, String* description, ErrorObject::Code code)
{
    UTF16StringDataNonGCStd msg = u"Line ";
    const size_t bufferLength = 64;
    char lineStringBuf[bufferLength];
    char* bufPtr = lineStringBuf + bufferLength - 2;

    /* Adds ": " at the end. */
    bufPtr[0] = ':';
    bufPtr[1] = ' ';

    size_t value = line;
    do {
        ASSERT(bufPtr > lineStringBuf);
        --bufPtr;
        *bufPtr = value % 10 + '0';
        value /= 10;
    } while (value > 0);

    msg += UTF16StringDataNonGCStd(bufPtr, lineStringBuf + bufferLength);

    if (description->length()) {
        msg += UTF16StringDataNonGCStd(description->toUTF16StringData().data());
    }

    esprima::Error* error = new (NoGC) esprima::Error(new UTF16String(msg.data(), msg.length()));
    error->index = index;
    error->lineNumber = line;
    error->column = col;
    error->description = description;
    error->errorCode = code;

    throw * error;
};

ParserStringView Scanner::SmallScannerResult::relatedSource(const ParserStringView& source) const
{
    return ParserStringView(source, this->start, this->end);
}

StringView Scanner::SmallScannerResult::relatedSource(const StringView& source) const
{
    return StringView(source, this->start, this->end);
}

ParserStringView Scanner::ScannerResult::relatedSource(const ParserStringView& source)
{
    return ParserStringView(source, this->start, this->end);
}

StringView Scanner::ScannerResult::relatedSource(const StringView& source)
{
    return StringView(source, this->start, this->end);
}

Value Scanner::ScannerResult::valueStringLiteralToValue(Scanner* scannerInstance)
{
    if (this->type == Token::KeywordToken) {
        return keywordToString(scannerInstance->escargotContext, this->valueKeywordKind).string();
    }

    if (this->hasAllocatedString) {
        if (!this->valueStringLiteralData.m_stringIfNewlyAllocated) {
            constructStringLiteral(scannerInstance);
        }
        return this->valueStringLiteralData.m_stringIfNewlyAllocated;
    }

    return new StringView(scannerInstance->sourceAsNormalView, this->valueStringLiteralData.m_start, this->valueStringLiteralData.m_end);
}

ParserStringView Scanner::ScannerResult::valueStringLiteral(Scanner* scannerInstance)
{
    if (this->type == Token::KeywordToken) {
        AtomicString as = keywordToString(scannerInstance->escargotContext, this->valueKeywordKind);
        return ParserStringView(as.string(), 0, as.string()->length());
    }
    if (this->hasAllocatedString) {
        if (!this->valueStringLiteralData.m_stringIfNewlyAllocated) {
            constructStringLiteral(scannerInstance);
        }
        return ParserStringView(this->valueStringLiteralData.m_stringIfNewlyAllocated);
    }
    return ParserStringView(scannerInstance->source, this->valueStringLiteralData.m_start, this->valueStringLiteralData.m_end);
}

double Scanner::ScannerResult::valueNumberLiteral(Scanner* scannerInstance)
{
    if (this->hasNonComputedNumberLiteral) {
        const auto& bd = scannerInstance->source.bufferAccessData();
        char* buffer;
        int length = this->end - this->start;

        if (bd.has8BitContent) {
            buffer = ((char*)bd.buffer) + this->start;
        } else {
            buffer = ALLOCA(this->end - this->start, char, ec);

            for (int i = 0; i < length; i++) {
                buffer[i] = bd.uncheckedCharAtFor16Bit(i + this->start);
            }
        }

        int lengthDummy;
        double_conversion::StringToDoubleConverter converter(double_conversion::StringToDoubleConverter::ALLOW_HEX
                                                                 | double_conversion::StringToDoubleConverter::ALLOW_LEADING_SPACES
                                                                 | double_conversion::StringToDoubleConverter::ALLOW_TRAILING_SPACES,
                                                             0.0, double_conversion::Double::NaN(),
                                                             "Infinity", "NaN");
        double ll = converter.StringToDouble(buffer, length, &lengthDummy);

        this->valueNumber = ll;
        this->hasNonComputedNumberLiteral = false;
    }
    return this->valueNumber;
}

void Scanner::ScannerResult::constructStringLiteralHelperAppendUTF16(Scanner* scannerInstance, char16_t ch, UTF16StringDataNonGCStd& stringUTF16, bool& isEveryCharLatin1)
{
    switch (ch) {
    case 'u':
    case 'x': {
        char32_t param;
        if (scannerInstance->peekChar() == '{') {
            ++scannerInstance->index;
            param = scannerInstance->scanUnicodeCodePointEscape();
        } else {
            param = scannerInstance->scanHexEscape(ch);
        }
        ParserCharPiece piece(param);
        stringUTF16.append(piece.data, piece.data + piece.length);
        if (piece.length != 1 || piece.data[0] >= 256) {
            isEveryCharLatin1 = false;
        }
        return;
    }
    case 'n':
        stringUTF16 += '\n';
        return;
    case 'r':
        stringUTF16 += '\r';
        return;
    case 't':
        stringUTF16 += '\t';
        return;
    case 'b':
        stringUTF16 += '\b';
        return;
    case 'f':
        stringUTF16 += '\f';
        return;
    case 'v':
        stringUTF16 += '\x0B';
        return;

    default:
        if (ch && isOctalDigit(ch)) {
            uint16_t octToDec = scannerInstance->octalToDecimal(ch, true);
            stringUTF16 += octToDec;
            ASSERT(octToDec < 256);
        } else {
            stringUTF16 += ch;
            if (ch >= 256) {
                isEveryCharLatin1 = false;
            }
        }
        return;
    }
}

void Scanner::ScannerResult::constructStringLiteral(Scanner* scannerInstance)
{
    size_t indexBackup = scannerInstance->index;
    size_t lineNumberBackup = scannerInstance->lineNumber;
    size_t lineStartBackup = scannerInstance->lineStart;

    scannerInstance->index = this->start;
    const size_t start = this->start;
    char16_t quote = scannerInstance->peekChar();
    ASSERT((quote == '\'' || quote == '"'));
    // 'String literal must starts with a quote');

    ++scannerInstance->index;
    bool isEveryCharLatin1 = true;

    UTF16StringDataNonGCStd stringUTF16;
    while (true) {
        char16_t ch = scannerInstance->peekChar();
        ++scannerInstance->index;
        if (ch == quote) {
            quote = '\0';
            break;
        } else if (UNLIKELY(ch == '\\')) {
            ch = scannerInstance->peekChar();
            ++scannerInstance->index;
            if (!ch || !isLineTerminator(ch)) {
                this->constructStringLiteralHelperAppendUTF16(scannerInstance, ch, stringUTF16, isEveryCharLatin1);
            } else {
                ++scannerInstance->lineNumber;
                char16_t bufferedChar = scannerInstance->peekChar();
                if ((ch == '\r' && bufferedChar == '\n') || (ch == '\n' && bufferedChar == '\r')) {
                    ++scannerInstance->index;
                }
                scannerInstance->lineStart = scannerInstance->index;
            }
        } else if (UNLIKELY(isLineTerminator(ch))) {
            break;
        } else {
            stringUTF16 += ch;
            if (ch >= 256) {
                isEveryCharLatin1 = false;
            }
        }
    }

    scannerInstance->index = indexBackup;
    scannerInstance->lineNumber = lineNumberBackup;
    scannerInstance->lineStart = lineStartBackup;

    String* newStr;
    if (isEveryCharLatin1) {
        newStr = new Latin1String(stringUTF16.data(), stringUTF16.length());
    } else {
        newStr = new UTF16String(stringUTF16.data(), stringUTF16.length());
    }
    this->valueStringLiteralData.m_stringIfNewlyAllocated = newStr;
}

Scanner::Scanner(::Escargot::Context* escargotContext, StringView code, size_t startLine, size_t startColumn)
    : source(code, 0, code.length())
    , sourceAsNormalView(code)
    , escargotContext(escargotContext)
    , sourceCodeAccessData(code.bufferAccessData())
    , length(code.length())
    , index(0)
    , lineNumber(((length > 0) ? 1 : 0) + startLine)
    , lineStart(startColumn)
{
    ASSERT(escargotContext != nullptr);
    // trackComment = false;
}

void Scanner::skipSingleLineComment(void)
{
    while (!this->eof()) {
        char16_t ch = this->peekCharWithoutEOF();
        ++this->index;

        if (isLineTerminator(ch)) {
            if (ch == 13 && this->peekCharWithoutEOF() == 10) {
                ++this->index;
            }
            ++this->lineNumber;
            this->lineStart = this->index;
            // return comments;
            return;
        }
    }
}

void Scanner::skipMultiLineComment(void)
{
    while (!this->eof()) {
        char16_t ch = this->peekCharWithoutEOF();
        ++this->index;

        if (isLineTerminator(ch)) {
            if (ch == 0x0D && this->peekCharWithoutEOF() == 0x0A) {
                ++this->index;
            }
            ++this->lineNumber;
            this->lineStart = this->index;
        } else if (ch == 0x2A && this->peekCharWithoutEOF() == 0x2F) {
            // Block comment ends with '*/'.
            ++this->index;
            return;
        }
    }

    throwUnexpectedToken();
}

char32_t Scanner::scanHexEscape(char prefix)
{
    size_t len = (prefix == 'u') ? 4 : 2;
    char32_t code = 0;

    for (size_t i = 0; i < len; ++i) {
        if (!this->eof() && isHexDigit(this->peekCharWithoutEOF())) {
            code = code * 16 + hexValue(this->peekCharWithoutEOF());
            ++this->index;
        } else {
            return EMPTY_CODE_POINT;
        }
    }

    return code;
}

char32_t Scanner::scanUnicodeCodePointEscape()
{
    // At least, one hex digit is required.
    if (this->eof() || this->peekCharWithoutEOF() == '}') {
        this->throwUnexpectedToken();
    }

    char32_t code = 0;
    char16_t ch;

    while (!this->eof()) {
        ch = this->peekCharWithoutEOF();
        ++this->index;
        if (!isHexDigit(ch)) {
            break;
        }
        code = code * 16 + hexValue(ch);
    }

    if (code > 0x10FFFF || ch != '}') {
        this->throwUnexpectedToken();
    }

    return code;
}

Scanner::ScanIDResult Scanner::getIdentifier()
{
    const size_t start = this->index;
    ++this->index;
    while (UNLIKELY(!this->eof())) {
        const char16_t ch = this->peekCharWithoutEOF();
        if (UNLIKELY(ch == 0x5C)) {
            // Blackslash (U+005C) marks Unicode escape sequence.
            this->index = start;
            return this->getComplexIdentifier();
        } else if (UNLIKELY(ch >= 0xD800 && ch < 0xDFFF)) {
            // Need to handle surrogate pairs.
            this->index = start;
            return this->getComplexIdentifier();
        }
        if (isIdentifierPart(ch)) {
            ++this->index;
        } else {
            break;
        }
    }

    const auto& srcData = this->source.bufferAccessData();
    StringBufferAccessData ad(srcData.has8BitContent, this->index - start,
                              srcData.has8BitContent ? reinterpret_cast<void*>(((LChar*)srcData.buffer) + start) : reinterpret_cast<void*>(((char16_t*)srcData.buffer) + start));

    return std::make_tuple(ad, nullptr);
}

Scanner::ScanIDResult Scanner::getComplexIdentifier()
{
    char32_t cp = this->codePointAt(this->index);
    ParserCharPiece piece = ParserCharPiece(cp);
    UTF16StringDataNonGCStd id(piece.data, piece.length);
    this->index += id.length();

    // '\u' (U+005C, U+0075) denotes an escaped character.
    char32_t ch;
    if (cp == 0x5C) {
        if (this->peekChar() != 0x75) {
            this->throwUnexpectedToken();
        }
        ++this->index;
        if (this->peekChar() == '{') {
            ++this->index;
            ch = this->scanUnicodeCodePointEscape();
        } else {
            ch = this->scanHexEscape('u');
            cp = ch;
            if (ch == EMPTY_CODE_POINT || ch == '\\' || !isIdentifierStart(cp)) {
                this->throwUnexpectedToken();
            }
        }
        id = ch;
    }

    while (!this->eof()) {
        cp = this->codePointAt(this->index);
        if (!isIdentifierPart(cp)) {
            break;
        }

        // ch = Character.fromCodePoint(cp);
        ch = cp;
        piece = ParserCharPiece(ch);
        id += UTF16StringDataNonGCStd(piece.data, piece.length);
        this->index += piece.length;

        // '\u' (U+005C, U+0075) denotes an escaped character.
        if (cp == 0x5C) {
            // id = id.substr(0, id.length - 1);
            id.erase(id.length() - 1);

            if (this->peekChar() != 0x75) {
                this->throwUnexpectedToken();
            }
            ++this->index;
            if (this->peekChar() == '{') {
                ++this->index;
                ch = this->scanUnicodeCodePointEscape();
            } else {
                ch = this->scanHexEscape('u');
                cp = ch;
                if (ch == EMPTY_CODE_POINT || ch == '\\' || !isIdentifierPart(cp)) {
                    this->throwUnexpectedToken();
                }
            }
            piece = ParserCharPiece(ch);
            id += UTF16StringDataNonGCStd(piece.data, piece.length);
        }
    }

    String* str = new UTF16String(id.data(), id.length());
    return std::make_tuple(str->bufferAccessData(), str);
}

uint16_t Scanner::octalToDecimal(char16_t ch, bool octal)
{
    // \0 is not octal escape sequence
    char16_t code = octalValue(ch);

    octal |= (ch != '0');

    if (!this->eof() && isOctalDigit(this->peekChar())) {
        octal = true;
        code = code * 8 + octalValue(this->peekChar());
        ++this->index;

        // 3 digits are only allowed when string starts
        // with 0, 1, 2, 3
        // if ('0123'.indexOf(ch) >= 0 && !this->eof() && Character.isOctalDigit(this->source.charCodeAt(this->index))) {
        if ((ch >= '0' && ch <= '3') && !this->eof() && isOctalDigit(this->peekChar())) {
            code = code * 8 + octalValue(this->peekChar());
            ++this->index;
        }
    }

    ASSERT(!octal || code < NON_OCTAL_VALUE);
    return octal ? code : NON_OCTAL_VALUE;
};

void Scanner::scanPunctuator(Scanner::ScannerResult* token, char16_t ch)
{
    const size_t start = this->index;
    PunctuatorKind kind;
    // Check for most common single-character punctuators.
    ++this->index;

    switch (ch) {
    case '(':
        kind = LeftParenthesis;
        break;

    case '{':
        kind = LeftBrace;
        break;

    case '.':
        kind = Period;
        if (this->peekChar() == '.' && this->sourceCharAt(this->index + 1) == '.') {
            // Spread operator "..."
            this->index += 2;
            kind = PeriodPeriodPeriod;
        }
        break;

    case '}':
        kind = RightBrace;
        break;
    case ')':
        kind = RightParenthesis;
        break;
    case ';':
        kind = SemiColon;
        break;
    case ',':
        kind = Comma;
        break;
    case '[':
        kind = LeftSquareBracket;
        break;
    case ']':
        kind = RightSquareBracket;
        break;
    case ':':
        kind = Colon;
        break;
    case '?':
        kind = GuessMark;
        break;
    case '~':
        kind = Wave;
        break;

    case '>':
        ch = this->peekChar();
        kind = RightInequality;

        if (ch == '>') {
            ++this->index;
            ch = this->peekChar();
            kind = RightShift;

            if (ch == '>') {
                ++this->index;
                kind = UnsignedRightShift;

                if (this->peekChar() == '=') {
                    ++this->index;
                    kind = UnsignedRightShiftEqual;
                }
            } else if (ch == '=') {
                kind = RightShiftEqual;
                ++this->index;
            }
        } else if (ch == '=') {
            kind = RightInequalityEqual;
            ++this->index;
        }
        break;

    case '<':
        ch = this->peekChar();
        kind = LeftInequality;

        if (ch == '<') {
            ++this->index;
            kind = LeftShift;

            if (this->peekChar() == '=') {
                kind = LeftShiftEqual;
                ++this->index;
            }
        } else if (ch == '=') {
            kind = LeftInequalityEqual;
            ++this->index;
        }
        break;

    case '=':
        ch = this->peekChar();
        kind = Substitution;

        if (ch == '=') {
            ++this->index;
            kind = Equal;

            if (this->peekChar() == '=') {
                kind = StrictEqual;
                ++this->index;
            }
        } else if (ch == '>') {
            kind = Arrow;
            ++this->index;
        }
        break;

    case '!':
        kind = ExclamationMark;

        if (this->peekChar() == '=') {
            ++this->index;
            kind = NotEqual;

            if (this->peekChar() == '=') {
                kind = NotStrictEqual;
                ++this->index;
            }
        }
        break;

    case '&':
        ch = this->peekChar();
        kind = BitwiseAnd;

        if (ch == '&') {
            kind = LogicalAnd;
            ++this->index;
        } else if (ch == '=') {
            kind = BitwiseAndEqual;
            ++this->index;
        }
        break;

    case '|':
        ch = this->peekChar();
        kind = BitwiseOr;

        if (ch == '|') {
            kind = LogicalOr;
            ++this->index;
        } else if (ch == '=') {
            kind = BitwiseOrEqual;
            ++this->index;
        }
        break;

    case '^':
        kind = BitwiseXor;

        if (this->peekChar() == '=') {
            kind = BitwiseXorEqual;
            ++this->index;
        }
        break;

    case '+':
        ch = this->peekChar();
        kind = Plus;

        if (ch == '+') {
            kind = PlusPlus;
            ++this->index;
        } else if (ch == '=') {
            kind = PlusEqual;
            ++this->index;
        }
        break;

    case '-':
        ch = this->peekChar();
        kind = Minus;

        if (ch == '-') {
            kind = MinusMinus;
            ++this->index;
        } else if (ch == '=') {
            kind = MinusEqual;
            ++this->index;
        }
        break;

    case '*':
        ch = this->peekChar();
        kind = Multiply;

        if (ch == '=') {
            kind = MultiplyEqual;
            ++this->index;
        } else if (ch == '*') {
            kind = Exponentiation;
            ++this->index;

            if (this->peekChar() == '=') {
                kind = ExponentiationEqual;
                ++this->index;
            }
        }
        break;

    case '/':
        kind = Divide;

        if (this->peekChar() == '=') {
            kind = DivideEqual;
            ++this->index;
        }
        break;

    case '%':
        kind = Mod;

        if (this->peekChar() == '=') {
            kind = ModEqual;
            ++this->index;
        }
        break;

    default:
        this->throwUnexpectedToken();
        kind = PunctuatorKindEnd;
        break;
    }

    token->setPunctuatorResult(this->lineNumber, this->lineStart, start, this->index, kind);
}

void Scanner::scanHexLiteral(Scanner::ScannerResult* token, size_t start)
{
    ASSERT(token != nullptr);
    uint64_t number = 0;
    double numberDouble = 0.0;
    bool shouldUseDouble = false;
    bool scanned = false;

    size_t shiftCount = 0;
    while (!this->eof()) {
        char16_t ch = this->peekCharWithoutEOF();
        if (!isHexDigit(ch)) {
            break;
        }
        if (shouldUseDouble) {
            numberDouble = numberDouble * 16 + toHexNumericValue(ch);
        } else {
            number = (number << 4) + toHexNumericValue(ch);
            if (++shiftCount >= 16) {
                shouldUseDouble = true;
                numberDouble = number;
                number = 0;
            }
        }
        this->index++;
        scanned = true;
    }

    if (!scanned) {
        this->throwUnexpectedToken();
    }

    if (isIdentifierStart(this->peekChar())) {
        this->throwUnexpectedToken();
    }

    if (shouldUseDouble) {
        ASSERT(number == 0);
        token->setNumericLiteralResult(numberDouble, this->lineNumber, this->lineStart, start, this->index, false);
    } else {
        ASSERT(numberDouble == 0.0);
        token->setNumericLiteralResult(number, this->lineNumber, this->lineStart, start, this->index, false);
    }
}

void Scanner::scanBinaryLiteral(Scanner::ScannerResult* token, size_t start)
{
    ASSERT(token != nullptr);
    uint64_t number = 0;
    bool scanned = false;

    while (!this->eof()) {
        char16_t ch = this->peekCharWithoutEOF();
        if (ch != '0' && ch != '1') {
            break;
        }
        number = (number << 1) + ch - '0';
        this->index++;
        scanned = true;
    }

    if (!scanned) {
        // only 0b or 0B
        this->throwUnexpectedToken();
    }

    if (!this->eof()) {
        char16_t ch = this->peekCharWithoutEOF();
        /* istanbul ignore else */
        if (isIdentifierStart(ch) || isDecimalDigit(ch)) {
            this->throwUnexpectedToken();
        }
    }

    token->setNumericLiteralResult(number, this->lineNumber, this->lineStart, start, this->index, false);
}

void Scanner::scanOctalLiteral(Scanner::ScannerResult* token, char16_t prefix, size_t start)
{
    ASSERT(token != nullptr);
    uint64_t number = 0;
    bool scanned = false;
    bool octal = isOctalDigit(prefix);

    while (!this->eof()) {
        char16_t ch = this->peekCharWithoutEOF();
        if (!isOctalDigit(ch)) {
            break;
        }
        number = (number << 3) + ch - '0';
        this->index++;
        scanned = true;
    }

    if (!octal && !scanned) {
        // only 0o or 0O
        throwUnexpectedToken();
    }

    char16_t ch = this->peekChar();
    if (isIdentifierStart(ch) || isDecimalDigit(ch)) {
        throwUnexpectedToken();
    }

    token->setNumericLiteralResult(number, this->lineNumber, this->lineStart, start, this->index, false);
    token->octal = octal;
}

bool Scanner::isImplicitOctalLiteral()
{
    // Implicit octal, unless there is a non-octal digit.
    // (Annex B.1.1 on Numeric Literals)
    for (size_t i = this->index + 1; i < this->length; ++i) {
        const char16_t ch = this->sourceCharAt(i);
        if (ch == '8' || ch == '9') {
            return false;
        }
        if (!isOctalDigit(ch)) {
            return true;
        }
    }
    return true;
}

void Scanner::scanNumericLiteral(Scanner::ScannerResult* token)
{
    ASSERT(token != nullptr);
    const size_t start = this->index;
    char16_t ch = this->peekChar();
    char16_t startChar = ch;
    ASSERT(isDecimalDigit(ch) || (ch == '.'));
    // 'Numeric literal must start with a decimal digit or a decimal point');

    bool seenDotOrE = false;

    if (ch != '.') {
        auto number = this->peekChar();
        ++this->index;
        ch = this->peekChar();

        // Hex number starts with '0x'.
        // Octal number starts with '0'.
        // Octal number in ES6 starts with '0o'.
        // Binary number in ES6 starts with '0b'.
        if (number == '0') {
            if (ch == 'x' || ch == 'X') {
                ++this->index;
                return this->scanHexLiteral(token, start);
            }
            if (ch == 'b' || ch == 'B') {
                ++this->index;
                return this->scanBinaryLiteral(token, start);
            }
            if (ch == 'o' || ch == 'O') {
                ++this->index;
                return this->scanOctalLiteral(token, ch, start);
            }

            if (ch && isOctalDigit(ch) && this->isImplicitOctalLiteral()) {
                return this->scanOctalLiteral(token, ch, start);
            }
        }

        while (isDecimalDigit(this->peekChar())) {
            ++this->index;
        }
        ch = this->peekChar();
    }

    if (ch == '.') {
        seenDotOrE = true;
        ++this->index;
        while (isDecimalDigit(this->peekChar())) {
            ++this->index;
        }
        ch = this->peekChar();
    }

    if (ch == 'e' || ch == 'E') {
        seenDotOrE = true;
        ++this->index;

        ch = this->peekChar();
        if (ch == '+' || ch == '-') {
            ++this->index;
            ch = this->peekChar();
        }

        if (isDecimalDigit(ch)) {
            do {
                ++this->index;
                ch = this->peekChar();
            } while (isDecimalDigit(ch));
        } else {
            this->throwUnexpectedToken();
        }
    }

    if (!this->eof() && isIdentifierStart(this->peekChar())) {
        this->throwUnexpectedToken();
    }

    token->setNumericLiteralResult(0, this->lineNumber, this->lineStart, start, this->index, true);
    if (startChar == '0' && !seenDotOrE && (this->index - start) > 1) {
        token->startWithZero = true;
    }
}

void Scanner::scanStringLiteral(Scanner::ScannerResult* token)
{
    ASSERT(token != nullptr);
    const size_t start = this->index;
    char16_t quote = this->peekChar();
    ASSERT((quote == '\'' || quote == '"'));
    // 'String literal must starts with a quote');

    ++this->index;
    bool octal = false;
    bool isPlainCase = true;

    while (LIKELY(!this->eof())) {
        char16_t ch = this->peekCharWithoutEOF();
        ++this->index;
        if (ch == quote) {
            quote = '\0';
            break;
        } else if (UNLIKELY(ch == '\\')) {
            ch = this->peekChar();
            ++this->index;
            isPlainCase = false;
            if (!ch || !isLineTerminator(ch)) {
                switch (ch) {
                case 'u':
                    if (this->peekChar() == '{') {
                        ++this->index;
                        this->scanUnicodeCodePointEscape();
                    } else if (this->scanHexEscape(ch) == EMPTY_CODE_POINT) {
                        this->throwUnexpectedToken(Messages::InvalidHexEscapeSequence);
                    }
                    break;
                case 'x':
                    if (this->scanHexEscape(ch) == EMPTY_CODE_POINT) {
                        this->throwUnexpectedToken(Messages::InvalidHexEscapeSequence);
                    }
                    break;
                case 'n':
                case 'r':
                case 't':
                case 'b':
                case 'f':
                case 'v':
                    break;

                default:
                    if (ch && isOctalDigit(ch)) {
                        octal |= (this->octalToDecimal(ch, false) != NON_OCTAL_VALUE);
                    } else if (isDecimalDigit(ch)) {
                        octal = true;
                    }
                    break;
                }
            } else {
                ++this->lineNumber;
                if (ch == '\r' && this->peekChar() == '\n') {
                    ++this->index;
                } else if (ch == '\n' && this->peekChar() == '\r') {
                    ++this->index;
                }
                this->lineStart = this->index;
            }
        } else if (UNLIKELY(isLineTerminator(ch))) {
            break;
        }
    }

    if (quote != '\0') {
        this->index = start;
        this->throwUnexpectedToken();
    }

    if (isPlainCase) {
        token->setResult(Token::StringLiteralToken, start + 1, this->index - 1, this->lineNumber, this->lineStart, start, this->index, octal);
    } else {
        // build string if needs
        token->setResult(Token::StringLiteralToken, (String*)nullptr, this->lineNumber, this->lineStart, start, this->index, octal);
    }
}

bool Scanner::isFutureReservedWord(const ParserStringView& id)
{
    const StringBufferAccessData& data = id.bufferAccessData();
    switch (data.length) {
    case 4:
        return data.equalsSameLength("enum");
    case 5:
        return data.equalsSameLength("super");
    case 6:
        return data.equalsSameLength("export") || data.equalsSameLength("import");
    }
    return false;
}

void Scanner::scanTemplate(Scanner::ScannerResult* token, bool head)
{
    ASSERT(token != nullptr);
    // TODO apply rope-string
    UTF16StringDataNonGCStd cooked;
    UTF16StringDataNonGCStd raw;
    bool terminated = false;
    size_t start = this->index;

    bool tail = false;

    while (!this->eof()) {
        char16_t ch = this->peekCharWithoutEOF();
        ++this->index;
        if (ch == '`') {
            tail = true;
            terminated = true;
            break;
        } else if (ch == '$') {
            if (this->peekChar() == '{') {
                ++this->index;
                terminated = true;
                break;
            }
            cooked += ch;
            raw += ch;
        } else if (ch == '\\') {
            raw += ch;
            ch = this->peekChar();
            if (!isLineTerminator(ch)) {
                auto currentIndex = this->index;
                ++this->index;
                switch (ch) {
                case 'n':
                    cooked += '\n';
                    break;
                case 'r':
                    cooked += '\r';
                    break;
                case 't':
                    cooked += '\t';
                    break;
                case 'u':
                    if (this->peekChar() == '{') {
                        ++this->index;
                        cooked += this->scanUnicodeCodePointEscape();
                    } else {
                        const size_t restore = this->index;
                        const char32_t unescaped = this->scanHexEscape(ch);
                        if (unescaped != EMPTY_CODE_POINT) {
                            ParserCharPiece piece(unescaped);
                            cooked += UTF16StringDataNonGCStd(piece.data, piece.length);
                        } else {
                            this->throwUnexpectedToken(Messages::InvalidHexEscapeSequence);
                        }
                    }
                    break;
                case 'x': {
                    const char32_t unescaped = this->scanHexEscape(ch);
                    if (unescaped == EMPTY_CODE_POINT) {
                        this->throwUnexpectedToken(Messages::InvalidHexEscapeSequence);
                    }
                    ParserCharPiece piece(unescaped);
                    cooked += UTF16StringDataNonGCStd(piece.data, piece.length);
                    break;
                }
                case 'b':
                    cooked += '\b';
                    break;
                case 'f':
                    cooked += '\f';
                    break;
                case 'v':
                    cooked += '\v';
                    break;
                default:
                    if (ch == '0') {
                        if (isDecimalDigit(this->peekChar())) {
                            // Illegal: \01 \02 and so on
                            this->throwUnexpectedToken(Messages::TemplateOctalLiteral);
                        }
                        cooked += (char16_t)'\0';
                    } else if (isOctalDigit(ch)) {
                        // Illegal: \1 \2
                        this->throwUnexpectedToken(Messages::TemplateOctalLiteral);
                    } else {
                        cooked += ch;
                    }
                    break;
                }
                auto endIndex = this->index;
                for (size_t i = currentIndex; i < endIndex; i++) {
                    raw += this->sourceCharAt(i);
                }
            } else {
                ++this->index;
                ++this->lineNumber;
                if (ch == '\r' && this->peekChar() == '\n') {
                    ++this->index;
                }
                if (ch == 0x2028 || ch == 0x2029) {
                    raw += ch;
                } else {
                    raw += '\n';
                }
                this->lineStart = this->index;
            }
        } else if (isLineTerminator(ch)) {
            ++this->lineNumber;
            if (ch == '\r' && this->peekChar() == '\n') {
                ++this->index;
            }
            if (ch == 0x2028 || ch == 0x2029) {
                raw += ch;
                cooked += ch;
            } else {
                raw += '\n';
                cooked += '\n';
            }
            this->lineStart = this->index;
        } else {
            cooked += ch;
            raw += ch;
        }
    }

    if (!terminated) {
        this->throwUnexpectedToken();
    }

    ScanTemplateResult* result = new ScanTemplateResult();
    result->head = head;
    result->tail = tail;
    result->valueRaw = UTF16StringData(raw.data(), raw.length());
    result->valueCooked = UTF16StringData(cooked.data(), cooked.length());

    if (head) {
        start--;
    }

    token->setTemplateTokenResult(result, this->lineNumber, this->lineStart, start, this->index);
}

String* Scanner::scanRegExpBody()
{
    char16_t ch = this->peekChar();
    ASSERT(ch == '/');
    // assert(ch == '/', 'Regular expression literal must start with a slash');

    // TODO apply rope-string
    char16_t ch0 = this->peekChar();
    ++this->index;
    UTF16StringDataNonGCStd str(&ch0, 1);
    bool classMarker = false;
    bool terminated = false;

    while (!this->eof()) {
        ch = this->peekCharWithoutEOF();
        ++this->index;
        str += ch;
        if (ch == '\\') {
            ch = this->peekChar();
            ++this->index;
            // ECMA-262 7.8.5
            if (isLineTerminator(ch)) {
                this->throwUnexpectedToken(Messages::UnterminatedRegExp);
            }
            str += ch;
        } else if (isLineTerminator(ch)) {
            this->throwUnexpectedToken(Messages::UnterminatedRegExp);
        } else if (classMarker) {
            if (ch == ']') {
                classMarker = false;
            }
        } else {
            if (ch == '/') {
                terminated = true;
                break;
            } else if (ch == '[') {
                classMarker = true;
            }
        }
    }

    if (!terminated) {
        this->throwUnexpectedToken(Messages::UnterminatedRegExp);
    }

    // Exclude leading and trailing slash.
    str = str.substr(1, str.length() - 2);
    if (isAllASCII(str.data(), str.length())) {
        return new ASCIIString(str.data(), str.length());
    }

    return new UTF16String(str.data(), str.length());
}

String* Scanner::scanRegExpFlags()
{
    // UTF16StringData str = '';
    UTF16StringDataNonGCStd flags;
    while (!this->eof()) {
        char16_t ch = this->peekCharWithoutEOF();
        if (!isIdentifierPart(ch)) {
            break;
        }

        ++this->index;
        if (ch == '\\' && !this->eof()) {
            ch = this->peekChar();
            if (ch == 'u') {
                ++this->index;
                const size_t restore = this->index;
                char32_t ch32 = this->scanHexEscape('u');
                if (ch32 != EMPTY_CODE_POINT) {
                    ParserCharPiece piece(ch32);
                    flags += UTF16StringDataNonGCStd(piece.data, piece.length);
                    /*
                    for (str += '\\u'; restore < this->index; ++restore) {
                        str += this->source[restore];
                    }*/
                } else {
                    this->index = restore;
                    flags += 'u';
                    // str += '\\u';
                }
                this->throwUnexpectedToken();
            } else {
                // str += '\\';
                this->throwUnexpectedToken();
            }
        } else {
            flags += ch;
            // str += ch;
        }
    }

    if (isAllASCII(flags.data(), flags.length())) {
        return new ASCIIString(flags.data(), flags.length());
    }

    return new UTF16String(flags.data(), flags.length());
}

void Scanner::scanRegExp(Scanner::ScannerResult* token)
{
    ASSERT(token != nullptr);
    const size_t start = this->index;

    String* body = this->scanRegExpBody();
    String* flags = this->scanRegExpFlags();
    // const value = this->testRegExp(body.value, flags.value);

    ScanRegExpResult result;
    result.body = body;
    result.flags = flags;
    token->setResult(Token::RegularExpressionToken, this->lineNumber, this->lineStart, start, this->index);
    token->valueRegexp = result;
}

// ECMA-262 11.6.2.1 Keywords
static ALWAYS_INLINE KeywordKind isKeyword(const StringBufferAccessData& data)
{
    // 'const' is specialized as Keyword in V8.
    // 'yield' and 'let' are for compatibility with SpiderMonkey and ES.next.
    // Some others are from future reserved words.

    size_t length = data.length;
    char16_t first = data.charAt(0);
    char16_t second;
    switch (first) {
    case 'b':
        if (length == 5 && data.equalsSameLength("break", 1)) {
            return BreakKeyword;
        }
        break;
    case 'c':
        if (length == 4) {
            if (data.equalsSameLength("case", 1)) {
                return CaseKeyword;
            }
        } else if (length == 5) {
            second = data.charAt(1);
            if (second == 'a' && data.equalsSameLength("catch", 2)) {
                return CatchKeyword;
            } else if (second == 'o' && data.equalsSameLength("const", 2)) {
                return ConstKeyword;
            } else if (second == 'l' && data.equalsSameLength("class", 2)) {
                return ClassKeyword;
            }
        } else if (length == 8 && data.equalsSameLength("continue", 1)) {
            return ContinueKeyword;
        }
        break;
    case 'd':
        if (length == 8) {
            if (data.equalsSameLength("debugger", 1)) {
                return DebuggerKeyword;
            }
        } else if (length == 2) {
            if (data.equalsSameLength("do", 1)) {
                return DoKeyword;
            }
        } else if (length == 6) {
            if (data.equalsSameLength("delete", 1)) {
                return DeleteKeyword;
            }
        } else if (length == 7) {
            if (data.equalsSameLength("default", 1)) {
                return DefaultKeyword;
            }
        }
        break;
    case 'e':
        if (length == 4) {
            second = data.charAt(1);
            if (second == 'l' && data.equalsSameLength("else", 2)) {
                return ElseKeyword;
            } else if (second == 'n' && data.equalsSameLength("enum", 2)) {
                return EnumKeyword;
            }
        } else if (length == 6 && data.equalsSameLength("export", 1)) {
            return ExportKeyword;
        } else if (length == 7 && data.equalsSameLength("extends", 1)) {
            return ExtendsKeyword;
        }
        break;
    case 'f':
        if (length == 3 && data.equalsSameLength("for", 1)) {
            return ForKeyword;
        } else if (length == 7 && data.equalsSameLength("finally", 1)) {
            return FinallyKeyword;
        } else if (length == 8 && data.equalsSameLength("function", 1)) {
            return FunctionKeyword;
        }
        break;
    case 'i':
        if (length == 2) {
            second = data.charAt(1);
            if (second == 'f') {
                return IfKeyword;
            } else if (second == 'n') {
                return InKeyword;
            }
        } else if (length == 6 && data.equalsSameLength("import", 1)) {
            return ImportKeyword;
        } else if (length == 10 && data.equalsSameLength("instanceof", 1)) {
            return InstanceofKeyword;
        }
        break;
    case 'l':
        if (length == 3 && data.equalsSameLength("let", 1)) {
            return LetKeyword;
        }
        break;
    case 'n':
        if (length == 3 && data.equalsSameLength("new", 1)) {
            return NewKeyword;
        }
        break;
    case 'r':
        if (length == 6 && data.equalsSameLength("return", 1)) {
            return ReturnKeyword;
        }
        break;
    case 's':
        if (length == 5 && data.equalsSameLength("super", 1)) {
            return SuperKeyword;
        } else if (length == 6 && data.equalsSameLength("switch", 1)) {
            return SwitchKeyword;
        }
        break;
    case 't':
        switch (length) {
        case 3:
            if (data.equalsSameLength("try", 1)) {
                return TryKeyword;
            }
            break;
        case 4:
            if (data.equalsSameLength("this", 1)) {
                return ThisKeyword;
            }
            break;
        case 5:
            if (data.equalsSameLength("throw", 1)) {
                return ThrowKeyword;
            }
            break;
        case 6:
            if (data.equalsSameLength("typeof", 1)) {
                return TypeofKeyword;
            }
            break;
        }
        break;
    case 'v':
        if (length == 3 && data.equalsSameLength("var", 1)) {
            return VarKeyword;
        } else if (length == 4 && data.equalsSameLength("void", 1)) {
            return VoidKeyword;
        }
        break;
    case 'w':
        if (length == 4 && data.equalsSameLength("with", 1)) {
            return WithKeyword;
        } else if (length == 5 && data.equalsSameLength("while", 1)) {
            return WhileKeyword;
        }
        break;
    case 'y':
        if (length == 5 && data.equalsSameLength("yield", 1)) {
            return YieldKeyword;
        }
        break;
    }
    return NotKeyword;
}

ALWAYS_INLINE void Scanner::scanIdentifier(Scanner::ScannerResult* token, char16_t ch0)
{
    ASSERT(token != nullptr);
    Token type;
    const size_t start = this->index;

    // Backslash (U+005C) starts an escaped character.
    ScanIDResult id = UNLIKELY(ch0 == 0x5C) ? this->getComplexIdentifier() : this->getIdentifier();
    const size_t end = this->index;

    // There is no keyword or literal with only one character.
    // Thus, it must be an identifier.
    KeywordKind keywordKind;
    const auto& data = std::get<0>(id);
    if (data.length == 1) {
        type = Token::IdentifierToken;
    } else if ((keywordKind = isKeyword(data))) {
        token->setKeywordResult(this->lineNumber, this->lineStart, start, this->index, keywordKind);
        return;
    } else if (data.length == 4) {
        if (data.equalsSameLength("null")) {
            type = Token::NullLiteralToken;
        } else if (data.equalsSameLength("true")) {
            type = Token::BooleanLiteralToken;
        } else {
            type = Token::IdentifierToken;
        }
    } else if (data.length == 5 && data.equalsSameLength("false")) {
        type = Token::BooleanLiteralToken;
    } else {
        type = Token::IdentifierToken;
    }

    if (UNLIKELY(std::get<1>(id) != nullptr)) {
        token->setResult(type, std::get<1>(id), this->lineNumber, this->lineStart, start, end);
    } else {
        token->setResult(type, start, end, this->lineNumber, this->lineStart, start, end);
    }
}

void Scanner::lex(Scanner::ScannerResult* token)
{
    ASSERT(token != nullptr);
    if (UNLIKELY(this->eof())) {
        token->setResult(Token::EOFToken, this->lineNumber, this->lineStart, this->index, this->index);
        return;
    }

    const char16_t cp = this->peekCharWithoutEOF();

    if (isIdentifierStart(cp)) {
        goto ScanID;
    }

    // String literal starts with single quote (U+0027) or double quote (U+0022).
    if (cp == 0x27 || cp == 0x22) {
        this->scanStringLiteral(token);
        return;
    }

    // Dot (.) U+002E can also start a floating-point number, hence the need
    // to check the next character.
    if (UNLIKELY(cp == 0x2E) && isDecimalDigit(this->sourceCharAt(this->index + 1))) {
        this->scanNumericLiteral(token);
        return;
    }

    if (isDecimalDigit(cp)) {
        this->scanNumericLiteral(token);
        return;
    }

    if (UNLIKELY(cp == '`')) {
        ++this->index;
        this->scanTemplate(token, true);
        return;
    }

    // Possible identifier start in a surrogate pair.
    if (UNLIKELY(cp >= 0xD800 && cp < 0xDFFF) && isIdentifierStart(this->codePointAt(this->index))) {
        goto ScanID;
    }
    this->scanPunctuator(token, cp);
    return;

ScanID:
    this->scanIdentifier(token, cp);
    return;
}
}
