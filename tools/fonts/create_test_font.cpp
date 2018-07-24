/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Running create_test_font generates ./tools/fonts/test_font_index.inc
// and ./tools/fonts/test_font_<generic name>.inc which are read by
// ./tools/fonts/sk_tool_utils_font.cpp

#include "SkFontStyle.h"
#include "SkOSFile.h"
#include "SkOSPath.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "SkStream.h"
#include "SkTArray.h"
#include "SkTSort.h"
#include "SkTypeface.h"
#include "SkUtils.h"
#include <stdio.h>

#define DEFAULT_FONT_NAME "sans-serif"

namespace {

struct NamedFontStyle {
    const char* fName;
    SkFontStyle fStyle;
};
constexpr NamedFontStyle normal     = {"Normal",     SkFontStyle::Normal()    };
constexpr NamedFontStyle bold       = {"Bold",       SkFontStyle::Bold()      };
constexpr NamedFontStyle italic     = {"Italic",     SkFontStyle::Italic()    };
constexpr NamedFontStyle bolditalic = {"BoldItalic", SkFontStyle::BoldItalic()};

struct FontDesc {
    char const * const fGenericName;
    NamedFontStyle const fNamedStyle;
    char const * const fFontName;
    char const * const fFile;
    // fFontIndex is mutable and will be set later.
    int fFontIndex;
} gFonts[] = {
    {"monospace",  normal,     "Liberation Mono",  "LiberationMono-Regular.ttf",     -1},
    {"monospace",  bold,       "Liberation Mono",  "LiberationMono-Bold.ttf",        -1},
    {"monospace",  italic,     "Liberation Mono",  "LiberationMono-Italic.ttf",      -1},
    {"monospace",  bolditalic, "Liberation Mono",  "LiberationMono-BoldItalic.ttf",  -1},
    {"sans-serif", normal,     "Liberation Sans",  "LiberationSans-Regular.ttf",     -1},
    {"sans-serif", bold,       "Liberation Sans",  "LiberationSans-Bold.ttf",        -1},
    {"sans-serif", italic,     "Liberation Sans",  "LiberationSans-Italic.ttf",      -1},
    {"sans-serif", bolditalic, "Liberation Sans",  "LiberationSans-BoldItalic.ttf",  -1},
    {"serif",      normal,     "Liberation Serif", "LiberationSerif-Regular.ttf",    -1},
    {"serif",      bold,       "Liberation Serif", "LiberationSerif-Bold.ttf",       -1},
    {"serif",      italic,     "Liberation Serif", "LiberationSerif-Italic.ttf",     -1},
    {"serif",      bolditalic, "Liberation Serif", "LiberationSerif-BoldItalic.ttf", -1},
};

const int gFontsCount = (int) SK_ARRAY_COUNT(gFonts);

const char gHeader[] =
"/*\n"
" * Copyright 2015 Google Inc.\n"
" *\n"
" * Use of this source code is governed by a BSD-style license that can be\n"
" * found in the LICENSE file.\n"
" */\n"
"\n"
"// Auto-generated by ";

} // namespace

static FILE* font_header(const char* family) {
    SkString outPath(SkOSPath::Join(".", "tools"));
    outPath = SkOSPath::Join(outPath.c_str(), "fonts");
    outPath = SkOSPath::Join(outPath.c_str(), "test_font_");
    SkString fam(family);
    do {
        int dashIndex = fam.find("-");
        if (dashIndex < 0) {
            break;
        }
        fam.writable_str()[dashIndex] = '_';
    } while (true);
    outPath.append(fam);
    outPath.append(".inc");
    FILE* out = fopen(outPath.c_str(), "w");
    fprintf(out, "%s%s\n\n", gHeader, SkOSPath::Basename(__FILE__).c_str());
    return out;
}

enum {
    kMaxLineLength = 80,
};

static ptrdiff_t last_line_length(const SkString& str) {
    const char* first = str.c_str();
    const char* last = first + str.size();
    const char* ptr = last;
    while (ptr > first && *--ptr != '\n')
        ;
    return last - ptr - 1;
}

static void output_fixed(SkScalar num, int emSize, SkString* out) {
    int hex = (int) (num * 65536 / emSize);
    out->appendf("0x%08x,", hex);
    *out += (int) last_line_length(*out) >= kMaxLineLength ? '\n' : ' ';
}

static void output_scalar(SkScalar num, int emSize, SkString* out) {
    num /= emSize;
    if (num == (int) num) {
       out->appendS32((int) num);
    } else {
        SkString str;
        str.printf("%1.6g", num);
        int width = (int) str.size();
        const char* cStr = str.c_str();
        while (cStr[width - 1] == '0') {
            --width;
        }
        str.remove(width, str.size() - width);
        out->appendf("%sf", str.c_str());
    }
    *out += ',';
    *out += (int) last_line_length(*out) >= kMaxLineLength ? '\n' : ' ';
}

static int output_points(const SkPoint* pts, int emSize, int count, SkString* ptsOut) {
    for (int index = 0; index < count; ++index) {
//        SkASSERT(floor(pts[index].fX) == pts[index].fX);
        output_scalar(pts[index].fX, emSize, ptsOut);
//        SkASSERT(floor(pts[index].fY) == pts[index].fY);
        output_scalar(pts[index].fY, emSize, ptsOut);
    }
    return count;
}

static void output_path_data(const SkPaint& paint,
        int emSize, SkString* ptsOut, SkTDArray<SkPath::Verb>* verbs,
        SkTDArray<unsigned>* charCodes, SkTDArray<SkScalar>* widths) {
    for (int ch = 0x00; ch < 0x7f; ++ch) {
        char str[1];
        str[0] = ch;
        const char* used = str;
        SkUnichar index = SkUTF8_NextUnichar(&used, str + 1);
        SkPath path;
        paint.getTextPath((const void*) &index, 2, 0, 0, &path);
        SkPath::RawIter iter(path);
        SkPath::Verb verb;
        SkPoint pts[4];
        while ((verb = iter.next(pts)) != SkPath::kDone_Verb) {
            *verbs->append() = verb;
            switch (verb) {
                case SkPath::kMove_Verb:
                    output_points(&pts[0], emSize, 1, ptsOut);
                    break;
                case SkPath::kLine_Verb:
                    output_points(&pts[1], emSize, 1, ptsOut);
                    break;
                case SkPath::kQuad_Verb:
                    output_points(&pts[1], emSize, 2, ptsOut);
                    break;
                case SkPath::kCubic_Verb:
                    output_points(&pts[1], emSize, 3, ptsOut);
                    break;
                case SkPath::kClose_Verb:
                    break;
                default:
                    SkDEBUGFAIL("bad verb");
                    SkASSERT(0);
            }
        }
        *verbs->append() = SkPath::kDone_Verb;
        *charCodes->append() = index;
        SkScalar width;
        SkDEBUGCODE(int charCount =) paint.getTextWidths((const void*) &index, 2, &width);
        SkASSERT(charCount == 1);
     // SkASSERT(floor(width) == width);  // not true for Hiragino Maru Gothic Pro
        *widths->append() = width;
        if (!ch) {
            ch = 0x1f;  // skip the rest of the control codes
        }
    }
}

static int offset_str_len(unsigned num) {
    if (num == (unsigned) -1) {
        return 10;
    }
    unsigned result = 1;
    unsigned ref = 10;
    while (ref <= num) {
        ++result;
        ref *= 10;
    }
    return result;
}

static SkString strip_spaces(const SkString& str) {
    SkString result;
    int count = (int) str.size();
    for (int index = 0; index < count; ++index) {
        char c = str[index];
        if (c != ' ' && c != '-') {
            result += c;
        }
    }
    return result;
}

static SkString strip_final(const SkString& str) {
    SkString result(str);
    if (result.endsWith("\n")) {
        result.remove(result.size() - 1, 1);
    }
    if (result.endsWith(" ")) {
        result.remove(result.size() - 1, 1);
    }
    if (result.endsWith(",")) {
        result.remove(result.size() - 1, 1);
    }
    return result;
}

static void output_font(sk_sp<SkTypeface> face, const char* name, NamedFontStyle style, FILE* out) {
    int emSize = face->getUnitsPerEm() * 2;
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setTextAlign(SkPaint::kLeft_Align);
    paint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    paint.setTextSize(emSize);
    paint.setTypeface(std::move(face));
    SkTDArray<SkPath::Verb> verbs;
    SkTDArray<unsigned> charCodes;
    SkTDArray<SkScalar> widths;
    SkString ptsOut;
    output_path_data(paint, emSize, &ptsOut, &verbs, &charCodes, &widths);
    SkString fontnameStr(name);
    SkString strippedStr = strip_spaces(fontnameStr);
    strippedStr.appendf("%s", style.fName);
    const char* fontname = strippedStr.c_str();
    fprintf(out, "const SkScalar %sPoints[] = {\n", fontname);
    ptsOut = strip_final(ptsOut);
    fprintf(out, "%s", ptsOut.c_str());
    fprintf(out, "\n};\n\n");
    fprintf(out, "const unsigned char %sVerbs[] = {\n", fontname);
    int verbCount = verbs.count();
    int outChCount = 0;
    for (int index = 0; index < verbCount;) {
        SkPath::Verb verb = verbs[index];
        SkASSERT(verb >= SkPath::kMove_Verb && verb <= SkPath::kDone_Verb);
        SkASSERT((unsigned) verb == (unsigned char) verb);
        fprintf(out, "%u", verb);
        if (++index < verbCount) {
            outChCount += 3;
            fprintf(out, "%c", ',');
            if (outChCount >= kMaxLineLength) {
                outChCount = 0;
                fprintf(out, "%c", '\n');
            } else {
                fprintf(out, "%c", ' ');
            }
        }
    }
    fprintf(out, "\n};\n\n");

    // all fonts are now 0x00, 0x20 - 0xFE
    // don't need to generate or output character codes?
    fprintf(out, "const unsigned %sCharCodes[] = {\n", fontname);
    int offsetCount = charCodes.count();
    for (int index = 0; index < offsetCount;) {
        unsigned offset = charCodes[index];
        fprintf(out, "%u", offset);
        if (++index < offsetCount) {
            outChCount += offset_str_len(offset) + 2;
            fprintf(out, "%c", ',');
            if (outChCount >= kMaxLineLength) {
                outChCount = 0;
                fprintf(out, "%c", '\n');
            } else {
                fprintf(out, "%c", ' ');
            }
        }
    }
    fprintf(out, "\n};\n\n");

    SkString widthsStr;
    fprintf(out, "const SkFixed %sWidths[] = {\n", fontname);
    for (int index = 0; index < offsetCount; ++index) {
        output_fixed(widths[index], emSize, &widthsStr);
    }
    widthsStr = strip_final(widthsStr);
    fprintf(out, "%s\n};\n\n", widthsStr.c_str());

    fprintf(out, "const int %sCharCodesCount = (int) SK_ARRAY_COUNT(%sCharCodes);\n\n",
            fontname, fontname);

    SkPaint::FontMetrics metrics;
    paint.getFontMetrics(&metrics);
    fprintf(out, "const SkPaint::FontMetrics %sMetrics = {\n", fontname);
    SkString metricsStr;
    metricsStr.printf("0x%08x, ", metrics.fFlags);
    output_scalar(metrics.fTop, emSize, &metricsStr);
    output_scalar(metrics.fAscent, emSize, &metricsStr);
    output_scalar(metrics.fDescent, emSize, &metricsStr);
    output_scalar(metrics.fBottom, emSize, &metricsStr);
    output_scalar(metrics.fLeading, emSize, &metricsStr);
    output_scalar(metrics.fAvgCharWidth, emSize, &metricsStr);
    output_scalar(metrics.fMaxCharWidth, emSize, &metricsStr);
    output_scalar(metrics.fXMin, emSize, &metricsStr);
    output_scalar(metrics.fXMax, emSize, &metricsStr);
    output_scalar(metrics.fXHeight, emSize, &metricsStr);
    output_scalar(metrics.fCapHeight, emSize, &metricsStr);
    output_scalar(metrics.fUnderlineThickness, emSize, &metricsStr);
    output_scalar(metrics.fUnderlinePosition, emSize, &metricsStr);
    output_scalar(metrics.fStrikeoutThickness, emSize, &metricsStr);
    output_scalar(metrics.fStrikeoutPosition, emSize, &metricsStr);
    metricsStr = strip_final(metricsStr);
    fprintf(out, "%s\n};\n\n", metricsStr.c_str());
}

struct FontWritten {
    const char* fFontName;
    NamedFontStyle fNamedStyle;
};

static SkTDArray<FontWritten> gWritten;

static int written_index(const FontDesc& fontDesc) {
    for (int index = 0; index < gWritten.count(); ++index) {
        const FontWritten& writ = gWritten[index];
        if (!strcmp(fontDesc.fFontName, writ.fFontName) &&
            fontDesc.fNamedStyle.fStyle == writ.fNamedStyle.fStyle)
        {
            return index;
        }
    }
    return -1;
}

static void generate_fonts(const char* basepath) {
    FILE* out = nullptr;
    for (int index = 0; index < gFontsCount; ++index) {
        FontDesc& fontDesc = gFonts[index];
        if ((index & 3) == 0) {
            out = font_header(fontDesc.fGenericName);
        }
        int fontIndex = written_index(fontDesc);
        if (fontIndex >= 0) {
            fontDesc.fFontIndex = fontIndex;
            continue;
        }
        SkString filepath(SkOSPath::Join(basepath, fontDesc.fFile));
        SkASSERTF(sk_exists(filepath.c_str()), "The file %s does not exist.", filepath.c_str());
        sk_sp<SkTypeface> resourceTypeface = SkTypeface::MakeFromFile(filepath.c_str());
        SkASSERTF(resourceTypeface, "The file %s is not a font.", filepath.c_str());
        output_font(std::move(resourceTypeface), fontDesc.fFontName, fontDesc.fNamedStyle, out);
        fontDesc.fFontIndex = gWritten.count();
        FontWritten* writ = gWritten.append();
        writ->fFontName = fontDesc.fFontName;
        writ->fNamedStyle = fontDesc.fNamedStyle;
        if ((index & 3) == 3) {
            fclose(out);
        }
    }
}

static const char* slant_to_string(SkFontStyle::Slant slant) {
    switch (slant) {
        case SkFontStyle::kUpright_Slant: return "SkFontStyle::kUpright_Slant";
        case SkFontStyle::kItalic_Slant : return "SkFontStyle::kItalic_Slant" ;
        case SkFontStyle::kOblique_Slant: return "SkFontStyle::kOblique_Slant";
        default: SK_ABORT("Unknown slant"); return "";
    }
}

static void generate_index(const char* defaultName) {
    FILE* out = font_header("index");
    fprintf(out, "static SkTestFontData gTestFonts[] = {\n");
    for (const FontWritten& writ : gWritten) {
        const char* name = writ.fFontName;
        SkString strippedStr = strip_spaces(SkString(name));
        strippedStr.appendf("%s", writ.fNamedStyle.fName);
        const char* strip = strippedStr.c_str();
        fprintf(out,
                "    {    %sPoints, %sVerbs, %sCharCodes,\n"
                "         %sCharCodesCount, %sWidths,\n"
                "         %sMetrics, \"Toy %s\", SkFontStyle(%d,%d,%s)\n"
                "    },\n",
                strip, strip, strip, strip, strip, strip, name,
                writ.fNamedStyle.fStyle.weight(), writ.fNamedStyle.fStyle.width(),
                slant_to_string(writ.fNamedStyle.fStyle.slant()));
    }
    fprintf(out, "};\n\n");
    fprintf(out, "const int gTestFontsCount = (int) SK_ARRAY_COUNT(gTestFonts);\n\n");
    fprintf(out,
                "struct SubFont {\n"
                "    const char* fFamilyName;\n"
                "    const char* fStyleName;\n"
                "    SkFontStyle fStyle;\n"
                "    SkTestFontData& fFont;\n"
                "    const char* fFile;\n"
                "};\n\n"
                "const SubFont gSubFonts[] = {\n");
    int defaultIndex = -1;
    for (int subIndex = 0; subIndex < gFontsCount; subIndex++) {
        const FontDesc& desc = gFonts[subIndex];
        if (defaultIndex < 0 && !strcmp(defaultName, desc.fGenericName)) {
            defaultIndex = subIndex;
        }
        fprintf(out,
                "    { \"%s\", \"%s\", SkFontStyle(%d,%d,%s), gTestFonts[%d], \"%s\" },\n",
                desc.fGenericName, desc.fNamedStyle.fName,
                desc.fNamedStyle.fStyle.weight(), desc.fNamedStyle.fStyle.width(),
                slant_to_string(desc.fNamedStyle.fStyle.slant()), desc.fFontIndex, desc.fFile);
    }
    for (int subIndex = 0; subIndex < gFontsCount; subIndex++) {
        const FontDesc& desc = gFonts[subIndex];
        fprintf(out,
                "    { \"Toy %s\", \"%s\", SkFontStyle(%d,%d,%s), gTestFonts[%d], \"%s\" },\n",
                desc.fFontName, desc.fNamedStyle.fName,
                desc.fNamedStyle.fStyle.weight(), desc.fNamedStyle.fStyle.width(),
                slant_to_string(desc.fNamedStyle.fStyle.slant()), desc.fFontIndex, desc.fFile);
    }
    fprintf(out, "};\n\n");
    fprintf(out, "const int gSubFontsCount = (int) SK_ARRAY_COUNT(gSubFonts);\n\n");
    SkASSERT(defaultIndex >= 0);
    fprintf(out, "const int gDefaultFontIndex = %d;\n", defaultIndex);
    fclose(out);
}

int main(int , char * const []) {
    generate_fonts("/Library/Fonts/"); // or /usr/share/fonts/truetype/ttf-liberation/
    generate_index(DEFAULT_FONT_NAME);
    return 0;
}
