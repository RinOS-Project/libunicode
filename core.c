#include "rin_unicode.h"

typedef struct RinUnicodeDecompositionEntry {
    uint32_t codepoint;
    uint8_t length;
    uint8_t compatibility;
    uint32_t decomposition[3];
} RinUnicodeDecompositionEntry;

#include "generated_data.h"

#define RIN_UNICODE_DECOMP_SEGMENT 12u

enum {
    RIN_UNICODE_WCTYPE_ALNUM = 1,
    RIN_UNICODE_WCTYPE_ALPHA = 2,
    RIN_UNICODE_WCTYPE_BLANK = 3,
    RIN_UNICODE_WCTYPE_CNTRL = 4,
    RIN_UNICODE_WCTYPE_DIGIT = 5,
    RIN_UNICODE_WCTYPE_GRAPH = 6,
    RIN_UNICODE_WCTYPE_LOWER = 7,
    RIN_UNICODE_WCTYPE_PRINT = 8,
    RIN_UNICODE_WCTYPE_PUNCT = 9,
    RIN_UNICODE_WCTYPE_SPACE = 10,
    RIN_UNICODE_WCTYPE_UPPER = 11,
    RIN_UNICODE_WCTYPE_XDIGIT = 12
};

enum {
    RIN_UNICODE_WCTRANS_TOLOWER = 1,
    RIN_UNICODE_WCTRANS_TOUPPER = 2
};

static int rin_unicode_cstring_length(const char* s, size_t* length_out) {
    size_t length;
    if (!s || !length_out) return 0;
    for (length = 0u; length < RIN_UNICODE_MAX_CSTRING_BYTES; ++length) {
        if (s[length] == '\0') {
            *length_out = length;
            return 1;
        }
    }
    return 0;
}

static int rin_unicode_wstring_length(const uint32_t* s,
                                      size_t* length_out) {
    size_t length;
    if (!s || !length_out) return 0;
    for (length = 0u; length < RIN_UNICODE_MAX_WSTRING_ELEMENTS;
         ++length) {
        if (s[length] == 0u) {
            *length_out = length;
            return 1;
        }
    }
    return 0;
}

static int rin_unicode_ascii_ieq(const char* lhs, const char* rhs) {
    size_t lhs_length;
    size_t rhs_length;
    size_t i;
    if (!rin_unicode_cstring_length(lhs, &lhs_length) ||
        !rin_unicode_cstring_length(rhs, &rhs_length) ||
        lhs_length != rhs_length)
        return 0;
    for (i = 0u; i < lhs_length; ++i) {
        char a = lhs[i];
        char b = rhs[i];
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
        if (a != b) return 0;
    }
    return 1;
}

static void rin_unicode_reset_state(rin_unicode_mbstate_t* ps) {
    if (!ps) return;
    ps->state = 0u;
    ps->codepoint = 0u;
}

int rin_unicode_is_valid_scalar(uint32_t cp) {
    if (cp > 0x10FFFFu) return 0;
    if (cp >= 0xD800u && cp <= 0xDFFFu) return 0;
    return 1;
}

static int rin_unicode_utf8_expected(uint8_t lead, uint32_t* out_cp, size_t* out_need, uint32_t* out_min) {
    if (lead < 0x80u) {
        if (out_cp) *out_cp = lead;
        if (out_need) *out_need = 0u;
        if (out_min) *out_min = 0u;
        return RIN_UNICODE_OK;
    }
    if (lead >= 0xC2u && lead <= 0xDFu) {
        if (out_cp) *out_cp = (uint32_t)(lead & 0x1Fu);
        if (out_need) *out_need = 1u;
        if (out_min) *out_min = 0x80u;
        return RIN_UNICODE_OK;
    }
    if (lead >= 0xE0u && lead <= 0xEFu) {
        if (out_cp) *out_cp = (uint32_t)(lead & 0x0Fu);
        if (out_need) *out_need = 2u;
        if (out_min) *out_min = 0x800u;
        return RIN_UNICODE_OK;
    }
    if (lead >= 0xF0u && lead <= 0xF4u) {
        if (out_cp) *out_cp = (uint32_t)(lead & 0x07u);
        if (out_need) *out_need = 3u;
        if (out_min) *out_min = 0x10000u;
        return RIN_UNICODE_OK;
    }
    return RIN_UNICODE_INVALID;
}

static uint32_t rin_unicode_utf8_min_scalar(size_t total_expected) {
    switch (total_expected) {
    case 2u: return 0x80u;
    case 3u: return 0x800u;
    case 4u: return 0x10000u;
    default: return 0u;
    }
}

int rin_unicode_decode_utf8(const char* s, size_t n, uint32_t* out_cp, size_t* out_len) {
    uint32_t cp = 0u;
    uint32_t min_value = 0u;
    size_t need = 0u;
    size_t i;
    if (!out_cp || !out_len || !s) return RIN_UNICODE_INVALID;
    if (n == 0u) return RIN_UNICODE_INCOMPLETE;
    if (rin_unicode_utf8_expected((uint8_t)s[0], &cp, &need, &min_value) != RIN_UNICODE_OK) {
        return RIN_UNICODE_INVALID;
    }
    if (need == 0u) {
        *out_cp = cp;
        *out_len = 1u;
        return RIN_UNICODE_OK;
    }
    if (n < need + 1u) return RIN_UNICODE_INCOMPLETE;
    for (i = 1u; i <= need; ++i) {
        uint8_t byte = (uint8_t)s[i];
        if ((byte & 0xC0u) != 0x80u) return RIN_UNICODE_INVALID;
        cp = (cp << 6u) | (uint32_t)(byte & 0x3Fu);
    }
    if (cp < min_value || !rin_unicode_is_valid_scalar(cp)) return RIN_UNICODE_INVALID;
    *out_cp = cp;
    *out_len = need + 1u;
    return RIN_UNICODE_OK;
}

int rin_unicode_decode_utf8_lossy(const char* s, size_t n,
                                  uint32_t* out_cp, size_t* out_len) {
    uint32_t cp = 0u;
    size_t consumed = 0u;
    size_t need = 0u;
    uint32_t ignored_cp = 0u;
    uint32_t ignored_min = 0u;
    size_t i;
    int status;
    if (!s || !out_cp || !out_len || n == 0u) return RIN_UNICODE_INCOMPLETE;
    status = rin_unicode_decode_utf8(s, n, &cp, &consumed);
    if (status == RIN_UNICODE_OK) {
        *out_cp = cp;
        *out_len = consumed;
        return RIN_UNICODE_OK;
    }

    /* Unicode's maximal-subpart behavior: consume the valid prefix of a
     * structurally plausible sequence, but never consume the next starter. */
    if (rin_unicode_utf8_expected((uint8_t)s[0], &ignored_cp, &need,
                                  &ignored_min) != RIN_UNICODE_OK) {
        consumed = 1u;
    } else {
        consumed = 1u;
        for (i = 1u; i <= need && i < n; ++i) {
            if (((uint8_t)s[i] & 0xC0u) != 0x80u) break;
            consumed++;
        }
    }
    *out_cp = 0xFFFDu;
    *out_len = consumed;
    return status;
}

int rin_unicode_is_combining(uint32_t cp) {
    return (cp >= 0x0300u && cp <= 0x036Fu) ||
           (cp >= 0x0483u && cp <= 0x0489u) ||
           (cp >= 0x0591u && cp <= 0x05BDu) || cp == 0x05BFu ||
           (cp >= 0x05C1u && cp <= 0x05C2u) ||
           (cp >= 0x0610u && cp <= 0x061Au) ||
           (cp >= 0x064Bu && cp <= 0x065Fu) || cp == 0x0670u ||
           (cp >= 0x06D6u && cp <= 0x06EDu) ||
           (cp >= 0x0900u && cp <= 0x0903u) ||
           (cp >= 0x093Au && cp <= 0x094Fu) ||
           (cp >= 0x0981u && cp <= 0x0983u) ||
           (cp >= 0x0E31u && cp <= 0x0E4Eu) ||
           (cp >= 0x1AB0u && cp <= 0x1AFFu) ||
           (cp >= 0x1DC0u && cp <= 0x1DFFu) ||
           (cp >= 0x20D0u && cp <= 0x20FFu) ||
           (cp >= 0xFE00u && cp <= 0xFE0Fu) ||
           (cp >= 0xFE20u && cp <= 0xFE2Fu) ||
           (cp >= 0xE0100u && cp <= 0xE01EFu) ||
           (cp >= 0x1F3FBu && cp <= 0x1F3FFu);
}

static int rin_unicode_is_wide(uint32_t cp) {
    return cp >= 0x1100u &&
           (cp <= 0x115Fu || cp == 0x2329u || cp == 0x232Au ||
            (cp >= 0x2E80u && cp <= 0x303Eu) ||
            (cp >= 0x3040u && cp <= 0xA4CFu) ||
            (cp >= 0xAC00u && cp <= 0xD7A3u) ||
            (cp >= 0xF900u && cp <= 0xFAFFu) ||
            (cp >= 0xFE10u && cp <= 0xFE19u) ||
            (cp >= 0xFE30u && cp <= 0xFE6Fu) ||
            (cp >= 0xFF01u && cp <= 0xFF60u) ||
            (cp >= 0xFFE0u && cp <= 0xFFE6u) ||
            (cp >= 0x1F300u && cp <= 0x1FAFFu) ||
            (cp >= 0x20000u && cp <= 0x3FFFDu));
}

int rin_unicode_cell_width(uint32_t cp) {
    if (!rin_unicode_is_valid_scalar(cp)) return 1;
    if (cp == 0u || cp == 0x200Du || rin_unicode_is_combining(cp)) return 0;
    if (cp < 0x20u || (cp >= 0x7Fu && cp < 0xA0u)) return 0;
    return rin_unicode_is_wide(cp) ? 2 : 1;
}

static int rin_unicode_is_regional_indicator(uint32_t cp) {
    return cp >= 0x1F1E6u && cp <= 0x1F1FFu;
}

typedef enum RinGraphemeProperty {
    RIN_GB_OTHER = 0,
    RIN_GB_CR,
    RIN_GB_LF,
    RIN_GB_CONTROL,
    RIN_GB_EXTEND,
    RIN_GB_ZWJ,
    RIN_GB_SPACING_MARK,
    RIN_GB_PREPEND,
    RIN_GB_L,
    RIN_GB_V,
    RIN_GB_T,
    RIN_GB_LV,
    RIN_GB_LVT,
    RIN_GB_RI
} RinGraphemeProperty;

static int rin_unicode_is_spacing_mark(uint32_t cp) {
    return cp == 0x0903u || cp == 0x093Bu ||
           (cp >= 0x093Eu && cp <= 0x0940u) ||
           (cp >= 0x0949u && cp <= 0x094Cu) ||
           (cp >= 0x0982u && cp <= 0x0983u) ||
           (cp >= 0x09BEu && cp <= 0x09C0u) ||
           (cp >= 0x09C7u && cp <= 0x09C8u) ||
           (cp >= 0x09CBu && cp <= 0x09CCu) ||
           (cp >= 0x0A3Eu && cp <= 0x0A40u) ||
           cp == 0x0A83u || (cp >= 0x0ABEu && cp <= 0x0AC0u) ||
           cp == 0x0AC9u || (cp >= 0x0ACBu && cp <= 0x0ACCu) ||
           (cp >= 0x0B02u && cp <= 0x0B03u) ||
           (cp >= 0x0B3Eu && cp <= 0x0B40u) ||
           (cp >= 0x0B47u && cp <= 0x0B48u) ||
           (cp >= 0x0B4Bu && cp <= 0x0B4Cu) ||
           (cp >= 0x0BBEu && cp <= 0x0BC2u) ||
           (cp >= 0x0BC6u && cp <= 0x0BC8u) ||
           (cp >= 0x0BCAu && cp <= 0x0BCCu) ||
           (cp >= 0x0C01u && cp <= 0x0C03u) ||
           (cp >= 0x0C41u && cp <= 0x0C44u) ||
           (cp >= 0x0C82u && cp <= 0x0C83u) ||
           cp == 0x0CBEu || (cp >= 0x0CC0u && cp <= 0x0CC4u) ||
           (cp >= 0x0CC7u && cp <= 0x0CC8u) ||
           (cp >= 0x0CCAu && cp <= 0x0CCBu) ||
           (cp >= 0x0D02u && cp <= 0x0D03u) ||
           (cp >= 0x0D3Eu && cp <= 0x0D40u) ||
           (cp >= 0x0D46u && cp <= 0x0D48u) ||
           (cp >= 0x0D4Au && cp <= 0x0D4Cu) ||
           (cp >= 0x0F3Eu && cp <= 0x0F3Fu) ||
           cp == 0x102Bu || cp == 0x102Cu || cp == 0x1031u ||
           cp == 0x1038u || cp == 0x1062u ||
           (cp >= 0x17B6u && cp <= 0x17C8u) ||
           (cp >= 0x1A19u && cp <= 0x1A1Au) ||
           (cp >= 0xA823u && cp <= 0xA824u) || cp == 0xA827u;
}

static int rin_unicode_is_prepend(uint32_t cp) {
    return (cp >= 0x0600u && cp <= 0x0605u) || cp == 0x06DDu ||
           cp == 0x070Fu || (cp >= 0x0890u && cp <= 0x0891u) ||
           cp == 0x08E2u || cp == 0x0D4Eu || cp == 0x110BDu ||
           cp == 0x110CDu || (cp >= 0x111C2u && cp <= 0x111C3u) ||
           cp == 0x1193Fu || cp == 0x11941u || cp == 0x11A3Au ||
           (cp >= 0x11A84u && cp <= 0x11A89u) || cp == 0x11D46u;
}

static int rin_unicode_is_extended_pictographic(uint32_t cp) {
    return cp == 0x00A9u || cp == 0x00AEu || cp == 0x203Cu ||
           cp == 0x2049u || cp == 0x2122u || cp == 0x2139u ||
           (cp >= 0x2194u && cp <= 0x21FFu) ||
           (cp >= 0x2300u && cp <= 0x23FFu) ||
           (cp >= 0x2600u && cp <= 0x27BFu) ||
           (cp >= 0x1F000u && cp <= 0x1FAFFu);
}

static RinGraphemeProperty rin_unicode_grapheme_property(uint32_t cp) {
    if (cp == 0x000Du) return RIN_GB_CR;
    if (cp == 0x000Au) return RIN_GB_LF;
    if (cp == 0x200Du) return RIN_GB_ZWJ;
    if (cp == 0x200Cu) return RIN_GB_EXTEND;
    if (cp < 0x0020u || (cp >= 0x007Fu && cp <= 0x009Fu) ||
        cp == 0x00ADu || cp == 0x061Cu || cp == 0x180Eu ||
        (cp >= 0x200Bu && cp <= 0x200Fu) ||
        (cp >= 0x2028u && cp <= 0x202Eu) ||
        (cp >= 0x2060u && cp <= 0x206Fu) || cp == 0xFEFFu)
        return RIN_GB_CONTROL;
    if (rin_unicode_is_spacing_mark(cp)) return RIN_GB_SPACING_MARK;
    if (rin_unicode_is_prepend(cp)) return RIN_GB_PREPEND;
    if (rin_unicode_is_combining(cp) ||
        (cp >= 0xE0020u && cp <= 0xE007Fu)) return RIN_GB_EXTEND;
    if ((cp >= 0x1100u && cp <= 0x115Fu) ||
        (cp >= 0xA960u && cp <= 0xA97Cu)) return RIN_GB_L;
    if ((cp >= 0x1160u && cp <= 0x11A7u) ||
        (cp >= 0xD7B0u && cp <= 0xD7C6u)) return RIN_GB_V;
    if ((cp >= 0x11A8u && cp <= 0x11FFu) ||
        (cp >= 0xD7CBu && cp <= 0xD7FBu)) return RIN_GB_T;
    if (cp >= 0xAC00u && cp <= 0xD7A3u)
        return ((cp - 0xAC00u) % 28u) == 0u ? RIN_GB_LV : RIN_GB_LVT;
    if (rin_unicode_is_regional_indicator(cp)) return RIN_GB_RI;
    return RIN_GB_OTHER;
}

size_t rin_unicode_grapheme_next(const char* s, size_t n, size_t offset) {
    uint32_t cp = 0u;
    uint32_t last_non_extend;
    RinGraphemeProperty previous_property;
    size_t consumed = 0u;
    size_t cursor;
    int regional_count = 0;
    int zwj_after_pictographic = 0;
    if (!s || offset >= n) return n;
    (void)rin_unicode_decode_utf8_lossy(s + offset, n - offset, &cp, &consumed);
    last_non_extend = cp;
    previous_property = rin_unicode_grapheme_property(cp);
    cursor = offset + consumed;
    if (previous_property == RIN_GB_RI) regional_count = 1;
    while (cursor < n) {
        size_t next_len = 0u;
        uint32_t next = 0u;
        RinGraphemeProperty next_property;
        int joins = 0;
        (void)rin_unicode_decode_utf8_lossy(s + cursor, n - cursor,
                                           &next, &next_len);
        if (next_len == 0u) break;
        next_property = rin_unicode_grapheme_property(next);

        /* UAX #29 extended grapheme cluster rules GB3 through GB13. */
        if (previous_property == RIN_GB_CR && next_property == RIN_GB_LF)
            joins = 1;
        else if (previous_property == RIN_GB_CR ||
                 previous_property == RIN_GB_LF ||
                 previous_property == RIN_GB_CONTROL ||
                 next_property == RIN_GB_CR || next_property == RIN_GB_LF ||
                 next_property == RIN_GB_CONTROL)
            joins = 0;
        else if (previous_property == RIN_GB_L &&
                 (next_property == RIN_GB_L || next_property == RIN_GB_V ||
                  next_property == RIN_GB_LV ||
                  next_property == RIN_GB_LVT))
            joins = 1;
        else if ((previous_property == RIN_GB_LV ||
                  previous_property == RIN_GB_V) &&
                 (next_property == RIN_GB_V || next_property == RIN_GB_T))
            joins = 1;
        else if ((previous_property == RIN_GB_LVT ||
                  previous_property == RIN_GB_T) &&
                 next_property == RIN_GB_T)
            joins = 1;
        else if (next_property == RIN_GB_EXTEND ||
                 next_property == RIN_GB_ZWJ ||
                 next_property == RIN_GB_SPACING_MARK)
            joins = 1;
        else if (previous_property == RIN_GB_PREPEND)
            joins = 1;
        else if (previous_property == RIN_GB_ZWJ &&
                 zwj_after_pictographic &&
                 rin_unicode_is_extended_pictographic(next))
            joins = 1;
        else if (previous_property == RIN_GB_RI &&
                 next_property == RIN_GB_RI &&
                 (regional_count & 1) != 0)
            joins = 1;
        if (!joins) break;

        cursor += next_len;
        if (next_property == RIN_GB_ZWJ) {
            zwj_after_pictographic =
                rin_unicode_is_extended_pictographic(last_non_extend);
        } else if (next_property != RIN_GB_EXTEND) {
            last_non_extend = next;
            if (next_property != RIN_GB_SPACING_MARK)
                zwj_after_pictographic = 0;
        }
        if (next_property == RIN_GB_RI) regional_count++;
        else if (next_property != RIN_GB_EXTEND) regional_count = 0;
        previous_property = next_property;
    }
    return cursor;
}

size_t rin_unicode_grapheme_prev(const char* s, size_t n, size_t offset) {
    size_t cursor = 0u;
    if (!s) return 0u;
    if (offset > n) offset = n;
    while (cursor < offset) {
        size_t next = rin_unicode_grapheme_next(s, n, cursor);
        if (next <= cursor || next >= offset) return cursor;
        cursor = next;
    }
    return cursor;
}

static int rin_unicode_line_break_is_hard(uint32_t cp) {
    return cp == 0x000Au || cp == 0x000Du || cp == 0x0085u ||
           cp == 0x2028u || cp == 0x2029u;
}

static int rin_unicode_line_break_is_space(uint32_t cp) {
    if (cp == 0x0009u || cp == 0x0020u || cp == 0x1680u ||
        (cp >= 0x2000u && cp <= 0x200Au) || cp == 0x202Fu ||
        cp == 0x205Fu || cp == 0x3000u)
        return 1;
    return 0;
}

static int rin_unicode_line_break_is_extend(uint32_t cp) {
    return rin_unicode_is_combining(cp) ||
           (cp >= 0xFE00u && cp <= 0xFE0Fu) ||
           (cp >= 0xE0100u && cp <= 0xE01EFu) ||
           (cp >= 0xE0020u && cp <= 0xE007Fu);
}

static int rin_unicode_line_break_is_ideographic(uint32_t cp) {
    return (cp >= 0x3040u && cp <= 0x30FFu) ||
           (cp >= 0x3400u && cp <= 0x4DBFu) ||
           (cp >= 0x4E00u && cp <= 0x9FFFu) ||
           (cp >= 0xF900u && cp <= 0xFAFFu) ||
           (cp >= 0xAC00u && cp <= 0xD7A3u) ||
           (cp >= 0x20000u && cp <= 0x2FA1Fu);
}

static int rin_unicode_line_break_is_open(uint32_t cp) {
    return cp == 0x0028u || cp == 0x005Bu || cp == 0x007Bu ||
           cp == 0x3008u || cp == 0x300Au || cp == 0x300Cu ||
           cp == 0x300Eu || cp == 0x3010u || cp == 0x3014u ||
           cp == 0x3016u || cp == 0x3018u || cp == 0x301Au ||
           cp == 0xFF08u || cp == 0xFF3Bu || cp == 0xFF5Bu;
}

static int rin_unicode_line_break_is_close(uint32_t cp) {
    return cp == 0x0029u || cp == 0x005Du || cp == 0x007Du ||
           cp == 0x002Cu || cp == 0x002Eu || cp == 0x003Au ||
           cp == 0x003Bu || cp == 0x0021u || cp == 0x003Fu ||
           cp == 0x3001u || cp == 0x3002u || cp == 0x3009u ||
           cp == 0x300Bu || cp == 0x300Du || cp == 0x300Fu ||
           cp == 0x3011u || cp == 0x3015u || cp == 0x3017u ||
           cp == 0x3019u || cp == 0x301Bu || cp == 0xFF09u ||
           cp == 0xFF0Cu || cp == 0xFF0Eu || cp == 0xFF3Du ||
           cp == 0xFF5Du;
}

static int rin_unicode_line_break_is_boundary(const char* s, size_t n,
                                               size_t offset,
                                               uint32_t* first,
                                               uint32_t* last) {
    size_t start;
    size_t cursor;
    int have_scalar = 0;
    if (!s || offset == 0u || offset > n) return 0;
    start = rin_unicode_grapheme_prev(s, n, offset);
    if (start >= offset || rin_unicode_grapheme_next(s, n, start) != offset)
        return 0;
    cursor = start;
    while (cursor < offset) {
        uint32_t cp = 0u;
        size_t consumed = 0u;
        (void)rin_unicode_decode_utf8_lossy(s + cursor, offset - cursor,
                                           &cp, &consumed);
        if (consumed == 0u || consumed > offset - cursor) return 0;
        if (!have_scalar) {
            if (first) *first = cp;
            have_scalar = 1;
        }
        if (last) *last = cp;
        cursor += consumed;
    }
    return have_scalar && cursor == offset;
}

int rin_unicode_line_break_opportunity(const char* s, size_t n, size_t offset) {
    uint32_t first = 0u;
    uint32_t previous = 0u;
    uint32_t next = 0u;
    size_t next_len = 0u;
    if (!rin_unicode_line_break_is_boundary(s, n, offset, &first, &previous))
        return RIN_UNICODE_LINE_BREAK_PROHIBITED;
    if (offset >= n) return RIN_UNICODE_LINE_BREAK_PROHIBITED;
    (void)rin_unicode_decode_utf8_lossy(s + offset, n - offset, &next,
                                       &next_len);
    if (next_len == 0u) return RIN_UNICODE_LINE_BREAK_PROHIBITED;
    if (rin_unicode_line_break_is_hard(previous))
        return RIN_UNICODE_LINE_BREAK_MANDATORY;
    if (rin_unicode_line_break_is_hard(next) ||
        rin_unicode_line_break_is_extend(next))
        return RIN_UNICODE_LINE_BREAK_PROHIBITED;
    if (first == 0x200Bu || first == 0x00ADu ||
        rin_unicode_line_break_is_space(first))
        return RIN_UNICODE_LINE_BREAK_ALLOWED;
    if (rin_unicode_line_break_is_open(first) ||
        rin_unicode_line_break_is_close(next))
        return RIN_UNICODE_LINE_BREAK_PROHIBITED;
    if (first == 0x002Du || first == 0x2010u || first == 0x2013u ||
        first == 0x30A0u || first == 0xFF0Du)
        return RIN_UNICODE_LINE_BREAK_ALLOWED;
    if (rin_unicode_line_break_is_ideographic(previous) &&
        rin_unicode_line_break_is_ideographic(next))
        return RIN_UNICODE_LINE_BREAK_ALLOWED;
    if (rin_unicode_line_break_is_close(previous) &&
        rin_unicode_line_break_is_ideographic(next))
        return RIN_UNICODE_LINE_BREAK_ALLOWED;
    return RIN_UNICODE_LINE_BREAK_PROHIBITED;
}

size_t rin_unicode_line_break_next(const char* s, size_t n, size_t offset) {
    size_t cursor;
    if (!s || offset >= n) return n;
    if (offset != 0u &&
        !rin_unicode_line_break_is_boundary(s, n, offset, NULL, NULL))
        return n;
    cursor = offset;
    while (cursor < n) {
        size_t next = rin_unicode_grapheme_next(s, n, cursor);
        if (next <= cursor) break;
        if (rin_unicode_line_break_opportunity(s, n, next) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED)
            return next;
        cursor = next;
    }
    return n;
}

int rin_unicode_encode_utf8(char* dest, size_t n, uint32_t cp, size_t* out_len) {
    size_t needed = 0u;
    if (!out_len) return RIN_UNICODE_INVALID;
    if (!rin_unicode_is_valid_scalar(cp)) return RIN_UNICODE_INVALID;
    if (cp < 0x80u) {
        needed = 1u;
        if (dest && n < needed) return RIN_UNICODE_NO_SPACE;
        if (dest) dest[0] = (char)cp;
    } else if (cp < 0x800u) {
        needed = 2u;
        if (dest && n < needed) return RIN_UNICODE_NO_SPACE;
        if (dest) {
            dest[0] = (char)(0xC0u | (cp >> 6u));
            dest[1] = (char)(0x80u | (cp & 0x3Fu));
        }
    } else if (cp < 0x10000u) {
        needed = 3u;
        if (dest && n < needed) return RIN_UNICODE_NO_SPACE;
        if (dest) {
            dest[0] = (char)(0xE0u | (cp >> 12u));
            dest[1] = (char)(0x80u | ((cp >> 6u) & 0x3Fu));
            dest[2] = (char)(0x80u | (cp & 0x3Fu));
        }
    } else {
        needed = 4u;
        if (dest && n < needed) return RIN_UNICODE_NO_SPACE;
        if (dest) {
            dest[0] = (char)(0xF0u | (cp >> 18u));
            dest[1] = (char)(0x80u | ((cp >> 12u) & 0x3Fu));
            dest[2] = (char)(0x80u | ((cp >> 6u) & 0x3Fu));
            dest[3] = (char)(0x80u | (cp & 0x3Fu));
        }
    }
    *out_len = needed;
    return RIN_UNICODE_OK;
}

/* Append one UTF-8 scalar while retaining the full required-length count.
 * A short string destination receives neither a partial scalar nor an
 * out-of-range cursor; it remains available for the terminating NUL. */
static int rin_unicode_append_utf8_cstring(char* dest, size_t dest_cap,
                                           size_t* out_len, uint32_t cp) {
    size_t encoded = 0u;
    size_t written = 0u;
    size_t available;

    if (!out_len || rin_unicode_encode_utf8((char*)0, 0u, cp, &encoded) !=
        RIN_UNICODE_OK) {
        return RIN_UNICODE_INVALID;
    }
    if (*out_len > (size_t)-1 - encoded) return RIN_UNICODE_NO_SPACE;
    if (dest && dest_cap > 0u && *out_len < dest_cap - 1u) {
        available = dest_cap - 1u - *out_len;
        if (encoded <= available &&
            rin_unicode_encode_utf8(dest + *out_len, available, cp,
                                     &written) != RIN_UNICODE_OK) {
            return RIN_UNICODE_INVALID;
        }
    }
    *out_len += encoded;
    return RIN_UNICODE_OK;
}

int rin_unicode_decode_utf16(const uint16_t* s, size_t n, uint32_t* out_cp, size_t* out_len) {
    uint16_t first;
    if (!s || !out_cp || !out_len || n == 0u) return RIN_UNICODE_INCOMPLETE;
    first = s[0];
    if (first < 0xD800u || first > 0xDFFFu) {
        *out_cp = (uint32_t)first;
        *out_len = 1u;
        return RIN_UNICODE_OK;
    }
    if (first > 0xDBFFu) return RIN_UNICODE_INVALID;
    if (n < 2u) return RIN_UNICODE_INCOMPLETE;
    if (s[1] < 0xDC00u || s[1] > 0xDFFFu) return RIN_UNICODE_INVALID;
    *out_cp = 0x10000u + ((((uint32_t)first - 0xD800u) << 10u) | ((uint32_t)s[1] - 0xDC00u));
    *out_len = 2u;
    return RIN_UNICODE_OK;
}

int rin_unicode_encode_utf16(uint16_t* dest, size_t n, uint32_t cp, size_t* out_len) {
    if (!out_len || !rin_unicode_is_valid_scalar(cp)) return RIN_UNICODE_INVALID;
    if (cp < 0x10000u) {
        if (dest && n < 1u) return RIN_UNICODE_NO_SPACE;
        if (dest) dest[0] = (uint16_t)cp;
        *out_len = 1u;
        return RIN_UNICODE_OK;
    }
    if (dest && n < 2u) return RIN_UNICODE_NO_SPACE;
    cp -= 0x10000u;
    if (dest) {
        dest[0] = (uint16_t)(0xD800u + (cp >> 10u));
        dest[1] = (uint16_t)(0xDC00u + (cp & 0x3FFu));
    }
    *out_len = 2u;
    return RIN_UNICODE_OK;
}

int rin_unicode_validate_utf8(const char* s, size_t n, size_t* valid_prefix) {
    size_t offset = 0u;
    if (!s) {
        if (valid_prefix) *valid_prefix = 0u;
        return 0;
    }
    while (offset < n) {
        uint32_t cp = 0u;
        size_t consumed = 0u;
        int status;
        if (s[offset] == '\0') break;
        status = rin_unicode_decode_utf8(s + offset, n - offset, &cp, &consumed);
        if (status != RIN_UNICODE_OK) {
            if (valid_prefix) *valid_prefix = offset;
            return 0;
        }
        offset += consumed;
    }
    if (valid_prefix) *valid_prefix = offset;
    return 1;
}

static const RinUnicodeDecompositionEntry* rin_unicode_find_decomposition(uint32_t cp, int compatibility) {
    size_t i;
    for (i = 0u; i < g_rin_unicode_decomposition_count; ++i) {
        if (g_rin_unicode_decompositions[i].codepoint != cp) continue;
        if (!compatibility && g_rin_unicode_decompositions[i].compatibility) continue;
        return &g_rin_unicode_decompositions[i];
    }
    return (const RinUnicodeDecompositionEntry*)0;
}

static size_t rin_unicode_emit_scalar(uint32_t* dest, size_t dest_cap, size_t offset, uint32_t cp) {
    if (dest && offset < dest_cap) dest[offset] = cp;
    return offset + 1u;
}

static size_t rin_unicode_decompose_scalar(uint32_t cp, int compatibility, uint32_t* dest, size_t dest_cap, size_t offset) {
    const RinUnicodeDecompositionEntry* entry;
    size_t i;
    if (compatibility) {
        if (cp == 0x00A0u || cp == 0x3000u) return rin_unicode_emit_scalar(dest, dest_cap, offset, 0x0020u);
        if (cp == 0x00B5u) return rin_unicode_decompose_scalar(0x03BCu, compatibility, dest, dest_cap, offset);
        if (cp == 0x2126u) return rin_unicode_decompose_scalar(0x03A9u, compatibility, dest, dest_cap, offset);
        if (cp == 0x212Au) return rin_unicode_emit_scalar(dest, dest_cap, offset, 0x004Bu);
        if (cp == 0x212Bu) return rin_unicode_decompose_scalar(0x00C5u, compatibility, dest, dest_cap, offset);
        if (cp == 0xFB01u) {
            offset = rin_unicode_emit_scalar(dest, dest_cap, offset, 0x0066u);
            return rin_unicode_emit_scalar(dest, dest_cap, offset, 0x0069u);
        }
        if (cp == 0xFB02u) {
            offset = rin_unicode_emit_scalar(dest, dest_cap, offset, 0x0066u);
            return rin_unicode_emit_scalar(dest, dest_cap, offset, 0x006Cu);
        }
        if (cp >= 0xFF01u && cp <= 0xFF5Eu) return rin_unicode_emit_scalar(dest, dest_cap, offset, cp - 0xFEE0u);
    }
    entry = rin_unicode_find_decomposition(cp, compatibility);
    if (!entry) return rin_unicode_emit_scalar(dest, dest_cap, offset, cp);
    for (i = 0u; i < entry->length; ++i) {
        offset = rin_unicode_decompose_scalar(entry->decomposition[i], compatibility, dest, dest_cap, offset);
    }
    return offset;
}

static int rin_unicode_normalization_form_valid(int form) {
    return form >= RIN_UNICODE_NORMALIZE_NFD &&
           form <= RIN_UNICODE_NORMALIZE_NFKC;
}

static uint32_t rin_unicode_try_compose(uint32_t lhs, uint32_t rhs) {
    size_t i;
    for (i = 0u; i < g_rin_unicode_decomposition_count; ++i) {
        const RinUnicodeDecompositionEntry* entry = &g_rin_unicode_decompositions[i];
        if (entry->compatibility || entry->length != 2u) continue;
        if (entry->decomposition[0] == lhs && entry->decomposition[1] == rhs) return entry->codepoint;
    }
    return 0u;
}

static int rin_unicode_in_range(uint32_t cp, const uint32_t (*ranges)[2], size_t count) {
    size_t i;
    for (i = 0u; i < count; ++i) {
        if (cp >= ranges[i][0] && cp <= ranges[i][1]) return 1;
    }
    return 0;
}

uint32_t rin_unicode_tolower(uint32_t cp) {
    if (cp >= 0x0041u && cp <= 0x005Au) return cp + 0x20u;
    if ((cp >= 0x00C0u && cp <= 0x00D6u) || (cp >= 0x00D8u && cp <= 0x00DEu)) return cp + 0x20u;
    if (cp == 0x0178u) return 0x00FFu;
    if (cp == 0x0130u) return 0x0069u;
    if (cp == 0x1E9Eu) return 0x00DFu;
    if (cp >= 0x0100u && cp <= 0x017Eu && (cp & 1u) == 0u) return cp + 1u;
    if (cp >= 0x0391u && cp <= 0x03ABu && cp != 0x03A2u) return cp + 0x20u;
    if (cp == 0x0386u) return 0x03ACu;
    if (cp == 0x0388u) return 0x03ADu;
    if (cp == 0x0389u) return 0x03AEu;
    if (cp == 0x038Au) return 0x03AFu;
    if (cp == 0x038Cu) return 0x03CCu;
    if (cp == 0x038Eu) return 0x03CDu;
    if (cp == 0x038Fu) return 0x03CEu;
    if (cp == 0x03AAu) return 0x03CAu;
    if (cp == 0x03ABu) return 0x03CBu;
    if (cp >= 0x0400u && cp <= 0x040Fu) return cp + 0x50u;
    if (cp >= 0x0410u && cp <= 0x042Fu) return cp + 0x20u;
    if (cp >= 0x0460u && cp <= 0x052Eu && (cp & 1u) == 0u) return cp + 1u;
    if (cp == 0x212Au) return 0x006Bu;
    return cp;
}

uint32_t rin_unicode_toupper(uint32_t cp) {
    if (cp >= 0x0061u && cp <= 0x007Au) return cp - 0x20u;
    if ((cp >= 0x00E0u && cp <= 0x00F6u) || (cp >= 0x00F8u && cp <= 0x00FEu)) return cp - 0x20u;
    if (cp == 0x00FFu) return 0x0178u;
    if (cp == 0x0131u) return 0x0049u;
    if (cp == 0x00DFu) return 0x1E9Eu;
    if (cp >= 0x0101u && cp <= 0x017Fu && (cp & 1u) == 1u) return cp - 1u;
    if (cp >= 0x03B1u && cp <= 0x03CBu) return cp - 0x20u;
    if (cp == 0x03C2u) return 0x03A3u;
    if (cp == 0x03ACu) return 0x0386u;
    if (cp == 0x03ADu) return 0x0388u;
    if (cp == 0x03AEu) return 0x0389u;
    if (cp == 0x03AFu) return 0x038Au;
    if (cp == 0x03CCu) return 0x038Cu;
    if (cp == 0x03CDu) return 0x038Eu;
    if (cp == 0x03CEu) return 0x038Fu;
    if (cp == 0x03CAu) return 0x03AAu;
    if (cp == 0x03CBu) return 0x03ABu;
    if (cp >= 0x0450u && cp <= 0x045Fu) return cp - 0x50u;
    if (cp >= 0x0430u && cp <= 0x044Fu) return cp - 0x20u;
    if (cp >= 0x0461u && cp <= 0x052Fu && (cp & 1u) == 1u) return cp - 1u;
    if (cp == 0x03BCu || cp == 0x00B5u) return 0x039Cu;
    return cp;
}

size_t rin_unicode_casefold_full(uint32_t cp, uint32_t out[3]) {
    if (!out) return 0u;
    if (cp == 0x00DFu || cp == 0x1E9Eu) {
        out[0] = 0x0073u;
        out[1] = 0x0073u;
        return 2u;
    }
    if (cp == 0x03A3u || cp == 0x03C2u) {
        out[0] = 0x03C3u;
        return 1u;
    }
    out[0] = rin_unicode_tolower(cp);
    return 1u;
}

int rin_unicode_isdigit(uint32_t cp) {
    if (cp >= 0x0030u && cp <= 0x0039u) return 1;
    if (cp >= 0x0660u && cp <= 0x0669u) return 1;
    if (cp >= 0x06F0u && cp <= 0x06F9u) return 1;
    if (cp >= 0x0966u && cp <= 0x096Fu) return 1;
    if (cp >= 0x0E50u && cp <= 0x0E59u) return 1;
    if (cp >= 0xFF10u && cp <= 0xFF19u) return 1;
    return 0;
}

int rin_unicode_isalpha(uint32_t cp) {
    static const uint32_t alpha_ranges[][2] = {
        { 0x0041u, 0x005Au }, { 0x0061u, 0x007Au }, { 0x00AAu, 0x00AAu }, { 0x00B5u, 0x00B5u },
        { 0x00BAu, 0x00BAu }, { 0x00C0u, 0x00D6u }, { 0x00D8u, 0x00F6u }, { 0x00F8u, 0x024Fu },
        { 0x0250u, 0x02AFu },
        { 0x0370u, 0x03FFu }, { 0x0400u, 0x052Fu }, { 0x0620u, 0x063Fu }, { 0x0641u, 0x064Au },
        { 0x066Eu, 0x066Fu }, { 0x0671u, 0x06D3u }, { 0x06D5u, 0x06D5u }, { 0x06E5u, 0x06E6u },
        { 0x06EEu, 0x06EFu }, { 0x06FAu, 0x06FCu }, { 0x06FFu, 0x06FFu }, { 0x0904u, 0x0939u },
        { 0x093Du, 0x093Du }, { 0x0950u, 0x0950u }, { 0x0958u, 0x0961u }, { 0x0971u, 0x0980u },
        { 0x0E01u, 0x0E30u }, { 0x0E32u, 0x0E33u }, { 0x0E40u, 0x0E46u }, { 0x3041u, 0x3096u },
        { 0x309Du, 0x309Fu }, { 0x30A1u, 0x30FAu }, { 0x30FCu, 0x30FFu }, { 0x3400u, 0x4DBFu },
        { 0x4E00u, 0x9FFFu }, { 0xAC00u, 0xD7A3u }, { 0xF900u, 0xFAFFu }, { 0x20000u, 0x2A6DFu },
        { 0x2A700u, 0x2B73Fu }, { 0x2B740u, 0x2B81Fu }, { 0x2B820u, 0x2CEAFu }
    };
    return rin_unicode_in_range(cp, alpha_ranges, sizeof(alpha_ranges) / sizeof(alpha_ranges[0]));
}

int rin_unicode_isalnum(uint32_t cp) { return rin_unicode_isalpha(cp) || rin_unicode_isdigit(cp); }

int rin_unicode_isblank(uint32_t cp) {
    static const uint32_t blank_ranges[][2] = {
        { 0x0009u, 0x0009u }, { 0x0020u, 0x0020u }, { 0x00A0u, 0x00A0u }, { 0x1680u, 0x1680u },
        { 0x2000u, 0x200Au }, { 0x202Fu, 0x202Fu }, { 0x205Fu, 0x205Fu }, { 0x3000u, 0x3000u }
    };
    return rin_unicode_in_range(cp, blank_ranges, sizeof(blank_ranges) / sizeof(blank_ranges[0]));
}

int rin_unicode_iscntrl(uint32_t cp) { return (cp <= 0x001Fu) || (cp >= 0x007Fu && cp <= 0x009Fu); }

int rin_unicode_isspace(uint32_t cp) {
    static const uint32_t space_ranges[][2] = {
        { 0x0009u, 0x000Du }, { 0x0020u, 0x0020u }, { 0x0085u, 0x0085u }, { 0x00A0u, 0x00A0u },
        { 0x1680u, 0x1680u }, { 0x2000u, 0x200Au }, { 0x2028u, 0x2029u }, { 0x202Fu, 0x202Fu },
        { 0x205Fu, 0x205Fu }, { 0x3000u, 0x3000u }
    };
    return rin_unicode_in_range(cp, space_ranges, sizeof(space_ranges) / sizeof(space_ranges[0]));
}

int rin_unicode_isupper(uint32_t cp) { return rin_unicode_isalpha(cp) && rin_unicode_tolower(cp) != cp; }
int rin_unicode_islower(uint32_t cp) { return rin_unicode_isalpha(cp) && rin_unicode_toupper(cp) != cp; }
int rin_unicode_isprint(uint32_t cp) { return rin_unicode_is_valid_scalar(cp) && !rin_unicode_iscntrl(cp); }
int rin_unicode_isgraph(uint32_t cp) { return rin_unicode_isprint(cp) && !rin_unicode_isspace(cp); }
int rin_unicode_ispunct(uint32_t cp) { return rin_unicode_isgraph(cp) && !rin_unicode_isalnum(cp); }

int rin_unicode_isxdigit(uint32_t cp) {
    if (cp >= 0x0030u && cp <= 0x0039u) return 1;
    if (cp >= 0x0041u && cp <= 0x0046u) return 1;
    if (cp >= 0x0061u && cp <= 0x0066u) return 1;
    if (cp >= 0xFF10u && cp <= 0xFF19u) return 1;
    if (cp >= 0xFF21u && cp <= 0xFF26u) return 1;
    if (cp >= 0xFF41u && cp <= 0xFF46u) return 1;
    return 0;
}

static int rin_unicode_normalize_one(uint32_t* dest, size_t dest_cap, size_t* offset,
    uint32_t* last_cp, int* has_last, uint32_t cp, int form) {
    uint32_t segment[RIN_UNICODE_DECOMP_SEGMENT];
    size_t seg_len = 0u;
    size_t i;
    if (!offset) return RIN_UNICODE_INVALID;
    seg_len = rin_unicode_decompose_scalar(cp,
        form == RIN_UNICODE_NORMALIZE_NFKC || form == RIN_UNICODE_NORMALIZE_NFKD,
        segment, RIN_UNICODE_DECOMP_SEGMENT, 0u);
    if (form == RIN_UNICODE_NORMALIZE_NFC || form == RIN_UNICODE_NORMALIZE_NFKC) {
        for (i = 0u; i < seg_len; ++i) {
            uint32_t composed = 0u;
            if (has_last && *has_last) {
                composed = rin_unicode_try_compose(*last_cp, segment[i]);
                if (composed != 0u) {
                    *last_cp = composed;
                    if (dest && *offset > 0u && (*offset - 1u) < dest_cap) dest[*offset - 1u] = composed;
                    continue;
                }
            }
            if (dest && *offset < dest_cap) dest[*offset] = segment[i];
            if (last_cp) *last_cp = segment[i];
            if (has_last) *has_last = 1;
            (*offset)++;
        }
        return RIN_UNICODE_OK;
    }
    for (i = 0u; i < seg_len; ++i) {
        if (dest && *offset < dest_cap) dest[*offset] = segment[i];
        if (last_cp) *last_cp = segment[i];
        if (has_last) *has_last = 1;
        (*offset)++;
    }
    return RIN_UNICODE_OK;
}

size_t rin_unicode_normalize_utf32(uint32_t* dest, size_t dest_cap, const uint32_t* src, size_t src_len, int form) {
    size_t i;
    size_t out_len = 0u;
    uint32_t last_cp = 0u;
    int has_last = 0;
    if (!src) {
        if (dest && dest_cap > 0u) dest[0] = 0u;
        return 0u;
    }
    if (!rin_unicode_normalization_form_valid(form)) {
        if (dest && dest_cap > 0u) dest[0] = 0u;
        return (size_t)-1;
    }
    if (src_len != (size_t)-1 &&
        src_len > RIN_UNICODE_MAX_WSTRING_ELEMENTS) {
        if (dest && dest_cap > 0u) dest[0] = 0u;
        return (size_t)-1;
    }
    if (src_len == (size_t)-1 &&
        !rin_unicode_wstring_length(src, &src_len)) {
        if (dest && dest_cap > 0u) dest[0] = 0u;
        return (size_t)-1;
    }
    for (i = 0u; i < src_len; ++i) {
        if (!rin_unicode_is_valid_scalar(src[i])) {
            if (dest && dest_cap > 0u) dest[0] = 0u;
            return (size_t)-1;
        }
        rin_unicode_normalize_one(dest, dest_cap ? dest_cap - 1u : 0u, &out_len, &last_cp, &has_last, src[i], form);
    }
    if (dest && dest_cap > 0u) {
        size_t term = out_len < dest_cap ? out_len : dest_cap - 1u;
        dest[term] = 0u;
    }
    return out_len;
}

size_t rin_unicode_normalize_utf8(char* dest, size_t dest_cap, const char* src, int form) {
    size_t out_len = 0u;
    size_t cursor = 0u;
    size_t source_len;
    uint32_t pending = 0u;
    int has_pending = 0;
    if (!src) {
        if (dest && dest_cap > 0u) dest[0] = '\0';
        return 0u;
    }
    if (!rin_unicode_normalization_form_valid(form)) {
        if (dest && dest_cap > 0u) dest[0] = '\0';
        return (size_t)-1;
    }
    if (!rin_unicode_cstring_length(src, &source_len)) {
        if (dest && dest_cap > 0u) dest[0] = '\0';
        return (size_t)-1;
    }
    while (cursor < source_len) {
        uint32_t cp = 0u;
        uint32_t segment[RIN_UNICODE_DECOMP_SEGMENT];
        size_t consumed = 0u;
        size_t segment_len;
        size_t index;
        if (rin_unicode_decode_utf8(src + cursor, source_len - cursor,
                                    &cp, &consumed) != RIN_UNICODE_OK ||
            consumed == 0u) {
            if (dest && dest_cap > 0u) dest[0] = '\0';
            return (size_t)-1;
        }
        segment_len = rin_unicode_decompose_scalar(
            cp, form == RIN_UNICODE_NORMALIZE_NFKC ||
                    form == RIN_UNICODE_NORMALIZE_NFKD,
            segment, RIN_UNICODE_DECOMP_SEGMENT, 0u);
        for (index = 0u; index < segment_len; ++index) {
            uint32_t composed = 0u;
            if ((form == RIN_UNICODE_NORMALIZE_NFC ||
                 form == RIN_UNICODE_NORMALIZE_NFKC) && has_pending)
                composed = rin_unicode_try_compose(pending, segment[index]);
            if (composed != 0u) {
                pending = composed;
                continue;
            }
            if (has_pending && rin_unicode_append_utf8_cstring(
                    dest, dest_cap, &out_len, pending) != RIN_UNICODE_OK) {
                if (dest && dest_cap > 0u) dest[0] = '\0';
                return (size_t)-1;
            }
            pending = segment[index];
            has_pending = 1;
        }
        cursor += consumed;
    }
    if (has_pending && rin_unicode_append_utf8_cstring(
            dest, dest_cap, &out_len, pending) != RIN_UNICODE_OK) {
        if (dest && dest_cap > 0u) dest[0] = '\0';
        return (size_t)-1;
    }
    if (dest && dest_cap > 0u) {
        size_t term = out_len < dest_cap ? out_len : dest_cap - 1u;
        dest[term] = '\0';
    }
    return out_len;
}

typedef struct RinUnicodeTransformIterator {
    const char* utf8;
    const uint32_t* utf32;
    uint32_t queue[16];
    size_t queue_length;
    size_t queue_index;
    int utf8_mode;
} RinUnicodeTransformIterator;

static int rin_unicode_iterator_fill_utf8(RinUnicodeTransformIterator* it) {
    uint32_t cp = 0u;
    size_t source_length;
    size_t consumed = 0u;
    uint32_t segment[RIN_UNICODE_DECOMP_SEGMENT];
    size_t seg_len = 0u;
    size_t i;
    if (!it || !it->utf8 || *it->utf8 == '\0') return 0;
    if (!rin_unicode_cstring_length(it->utf8, &source_length) ||
        rin_unicode_decode_utf8(it->utf8, source_length, &cp, &consumed) !=
            RIN_UNICODE_OK) {
        it->utf8++;
        cp = 0xFFFDu;
        consumed = 0u;
    }
    seg_len = rin_unicode_decompose_scalar(cp, 1, segment, RIN_UNICODE_DECOMP_SEGMENT, 0u);
    it->queue_index = 0u;
    it->queue_length = 0u;
    for (i = 0u; i < seg_len; ++i) {
        uint32_t folded[3];
        size_t folded_len = rin_unicode_casefold_full(segment[i], folded);
        size_t j;
        for (j = 0u; j < folded_len && it->queue_length < 16u; ++j) {
            it->queue[it->queue_length++] = folded[j];
        }
    }
    it->utf8 += consumed ? consumed : 1u;
    return it->queue_length != 0u;
}

static int rin_unicode_iterator_fill_utf32(RinUnicodeTransformIterator* it) {
    uint32_t cp;
    uint32_t segment[RIN_UNICODE_DECOMP_SEGMENT];
    size_t seg_len;
    size_t i;
    if (!it || !it->utf32 || *it->utf32 == 0u) return 0;
    cp = *it->utf32++;
    seg_len = rin_unicode_decompose_scalar(cp, 1, segment, RIN_UNICODE_DECOMP_SEGMENT, 0u);
    it->queue_index = 0u;
    it->queue_length = 0u;
    for (i = 0u; i < seg_len; ++i) {
        uint32_t folded[3];
        size_t folded_len = rin_unicode_casefold_full(segment[i], folded);
        size_t j;
        for (j = 0u; j < folded_len && it->queue_length < 16u; ++j) {
            it->queue[it->queue_length++] = folded[j];
        }
    }
    return it->queue_length != 0u;
}

static int rin_unicode_iterator_next(RinUnicodeTransformIterator* it, uint32_t* out_cp) {
    if (!it || !out_cp) return 0;
    while (it->queue_index >= it->queue_length) {
        if (it->utf8_mode) {
            if (!rin_unicode_iterator_fill_utf8(it)) return 0;
        } else {
            if (!rin_unicode_iterator_fill_utf32(it)) return 0;
        }
    }
    *out_cp = it->queue[it->queue_index++];
    return 1;
}

size_t rin_unicode_transform_utf32(uint32_t* dest, size_t dest_cap, const uint32_t* src) {
    RinUnicodeTransformIterator it;
    size_t out_len = 0u;
    size_t source_length;
    uint32_t cp = 0u;
    it.utf8 = (const char*)0;
    it.utf32 = src;
    it.queue_length = it.queue_index = 0u;
    it.utf8_mode = 0;
    if (src && !rin_unicode_wstring_length(src, &source_length)) {
        if (dest && dest_cap > 0u) dest[0] = 0u;
        return (size_t)-1;
    }
    out_len = 0u;
    while (rin_unicode_iterator_next(&it, &cp)) {
        if (dest && out_len + 1u < dest_cap) dest[out_len] = cp;
        out_len++;
    }
    if (dest && dest_cap > 0u) {
        size_t term = out_len < dest_cap ? out_len : dest_cap - 1u;
        dest[term] = 0u;
    }
    return out_len;
}

size_t rin_unicode_transform_utf8(char* dest, size_t dest_cap, const char* src) {
    RinUnicodeTransformIterator it;
    size_t out_len = 0u;
    size_t source_length;
    uint32_t cp = 0u;
    it.utf8 = src;
    it.utf32 = (const uint32_t*)0;
    it.queue_length = it.queue_index = 0u;
    it.utf8_mode = 1;
    if (src && !rin_unicode_cstring_length(src, &source_length)) {
        if (dest && dest_cap > 0u) dest[0] = '\0';
        return (size_t)-1;
    }
    out_len = 0u;
    while (rin_unicode_iterator_next(&it, &cp)) {
        if (rin_unicode_append_utf8_cstring(dest, dest_cap, &out_len, cp) !=
            RIN_UNICODE_OK) {
            if (dest && dest_cap > 0u) dest[0] = '\0';
            return (size_t)-1;
        }
    }
    if (dest && dest_cap > 0u) {
        size_t term = out_len < dest_cap ? out_len : dest_cap - 1u;
        dest[term] = '\0';
    }
    return out_len;
}

int rin_unicode_compare_utf32(const uint32_t* lhs, const uint32_t* rhs) {
    RinUnicodeTransformIterator a;
    RinUnicodeTransformIterator b;
    uint32_t left = 0u;
    uint32_t right = 0u;
    int has_left;
    int has_right;
    size_t ignored_length;
    if (!rin_unicode_wstring_length(lhs, &ignored_length))
        return rhs && rin_unicode_wstring_length(rhs, &ignored_length)
                   ? -1
                   : 0;
    if (!rin_unicode_wstring_length(rhs, &ignored_length)) return 1;
    a.utf8 = (const char*)0;
    a.utf32 = lhs;
    a.queue_length = a.queue_index = 0u;
    a.utf8_mode = 0;
    b.utf8 = (const char*)0;
    b.utf32 = rhs;
    b.queue_length = b.queue_index = 0u;
    b.utf8_mode = 0;
    for (;;) {
        has_left = rin_unicode_iterator_next(&a, &left);
        has_right = rin_unicode_iterator_next(&b, &right);
        if (!has_left || !has_right) break;
        if (left != right) return left < right ? -1 : 1;
    }
    if (has_left) return 1;
    if (has_right) return -1;
    return 0;
}

int rin_unicode_compare_utf8(const char* lhs, const char* rhs) {
    RinUnicodeTransformIterator a;
    RinUnicodeTransformIterator b;
    uint32_t left = 0u;
    uint32_t right = 0u;
    int has_left;
    int has_right;
    size_t ignored_length;
    if (!rin_unicode_cstring_length(lhs ? lhs : "", &ignored_length))
        return rhs && rin_unicode_cstring_length(rhs, &ignored_length)
                   ? -1
                   : 0;
    if (!rin_unicode_cstring_length(rhs ? rhs : "", &ignored_length)) return 1;
    a.utf8 = lhs ? lhs : "";
    a.utf32 = (const uint32_t*)0;
    a.queue_length = a.queue_index = 0u;
    a.utf8_mode = 1;
    b.utf8 = rhs ? rhs : "";
    b.utf32 = (const uint32_t*)0;
    b.queue_length = b.queue_index = 0u;
    b.utf8_mode = 1;
    for (;;) {
        has_left = rin_unicode_iterator_next(&a, &left);
        has_right = rin_unicode_iterator_next(&b, &right);
        if (!has_left || !has_right) break;
        if (left != right) return left < right ? -1 : 1;
    }
    if (has_left) return 1;
    if (has_right) return -1;
    return 0;
}

int rin_unicode_mbsinit(const rin_unicode_mbstate_t* ps) {
    return !ps || (ps->state == 0u && ps->codepoint == 0u);
}

size_t rin_unicode_mbrtowc32(uint32_t* out_wc, const char* s, size_t n, rin_unicode_mbstate_t* ps) {
    rin_unicode_mbstate_t local_state;
    uint32_t partial = 0u;
    uint32_t min_value = 0u;
    size_t total_expected = 0u;
    size_t need_class = 0u;
    size_t seen = 0u;
    size_t i;
    if (!ps) {
        rin_unicode_reset_state(&local_state);
        ps = &local_state;
    }
    if (!s) {
        rin_unicode_reset_state(ps);
        return 0u;
    }
    if (n == 0u) return (size_t)-2;
    if (ps->state == 0u) {
        if ((unsigned char)s[0] == 0u) {
            if (out_wc) *out_wc = 0u;
            return 0u;
        }
        if (rin_unicode_utf8_expected((uint8_t)s[0], &partial, &total_expected, &min_value) != RIN_UNICODE_OK) {
            rin_unicode_reset_state(ps);
            return (size_t)-1;
        }
        if (total_expected == 0u) {
            if (out_wc) *out_wc = (uint8_t)s[0];
            return 1u;
        }
        need_class = total_expected;
        total_expected += 1u;
        ps->codepoint = partial;
        ps->state = (uint32_t)((need_class << 16u) | (total_expected << 8u) | 1u);
        seen = 1u;
        i = 1u;
    } else {
        need_class = (size_t)((ps->state >> 16u) & 0xFFu);
        total_expected = (size_t)((ps->state >> 8u) & 0xFFu);
        seen = (size_t)(ps->state & 0xFFu);
        partial = ps->codepoint;
        i = 0u;
    }
    while (i < n && seen < total_expected) {
        uint8_t byte = (uint8_t)s[i];
        if ((byte & 0xC0u) != 0x80u) {
            rin_unicode_reset_state(ps);
            return (size_t)-1;
        }
        partial = (partial << 6u) | (uint32_t)(byte & 0x3Fu);
        seen++;
        i++;
    }
    if (seen < total_expected) {
        ps->codepoint = partial;
        ps->state = (uint32_t)((need_class << 16u) | (total_expected << 8u) | seen);
        return (size_t)-2;
    }
    min_value = rin_unicode_utf8_min_scalar(total_expected);
    if (partial < min_value || !rin_unicode_is_valid_scalar(partial)) {
        rin_unicode_reset_state(ps);
        return (size_t)-1;
    }
    if (out_wc) *out_wc = partial;
    rin_unicode_reset_state(ps);
    return i;
}

size_t rin_unicode_mbrlen(const char* s, size_t n, rin_unicode_mbstate_t* ps) {
    return rin_unicode_mbrtowc32((uint32_t*)0, s, n, ps);
}

size_t rin_unicode_wcrtomb32(char* dest, uint32_t wc, rin_unicode_mbstate_t* ps) {
    size_t written = 0u;
    (void)ps;
    /* Rin's UTF-8 encoding is state-independent.  The C wcrtomb contract
     * therefore returns zero for a NULL destination (state reset/query), not
     * a fabricated one-byte result.  A non-NULL destination still receives
     * the exact encoded scalar length. */
    if (!dest) return 0u;
    if (rin_unicode_encode_utf8(dest, 4u, wc, &written) != RIN_UNICODE_OK) return (size_t)-1;
    return written;
}

int rin_unicode_mbtowc32(uint32_t* out_wc, const char* s, size_t n) {
    rin_unicode_mbstate_t st;
    size_t rc;
    rin_unicode_reset_state(&st);
    if (!s) return 0;
    rc = rin_unicode_mbrtowc32(out_wc, s, n, &st);
    if (rc == (size_t)-1 || rc == (size_t)-2) return -1;
    return (int)rc;
}

int rin_unicode_wctomb32(char* dest, uint32_t wc) {
    size_t rc = rin_unicode_wcrtomb32(dest, wc, (rin_unicode_mbstate_t*)0);
    if (rc == (size_t)-1) return -1;
    return (int)rc;
}

size_t rin_unicode_mbstowcs32(uint32_t* dest, const char* src, size_t n) {
    const char* cursor = src;
    size_t out_len = 0u;
    size_t source_len;
    rin_unicode_mbstate_t st;
    rin_unicode_reset_state(&st);
    if (!src || !rin_unicode_cstring_length(src, &source_len))
        return (size_t)-1;
    while ((size_t)(cursor - src) < source_len &&
           (dest == (uint32_t*)0 || out_len < n)) {
        uint32_t cp = 0u;
        size_t rc = rin_unicode_mbrtowc32(
            &cp, cursor, source_len - (size_t)(cursor - src), &st);
        if (rc == (size_t)-1 || rc == (size_t)-2) return (size_t)-1;
        if (dest) dest[out_len] = cp;
        out_len++;
        cursor += rc;
    }
    if ((size_t)(cursor - src) == source_len && dest && out_len < n)
        dest[out_len] = 0u;
    return out_len;
}

size_t rin_unicode_wcstombs32(char* dest, const uint32_t* src, size_t n) {
    size_t out_len = 0u;
    size_t i = 0u;
    size_t source_len;
    if (!src || !rin_unicode_wstring_length(src, &source_len))
        return (size_t)-1;
    while (i < source_len) {
        size_t encoded = 0u;
        size_t written = 0u;
        if (rin_unicode_encode_utf8((char*)0, 0u, src[i], &encoded) !=
            RIN_UNICODE_OK) {
            return (size_t)-1;
        }
        if (dest) {
            if (encoded > n - out_len) break;
            if (rin_unicode_encode_utf8(dest + out_len,
                    n - out_len, src[i], &written) != RIN_UNICODE_OK) {
                return (size_t)-1;
            }
        }
        if (out_len > (size_t)-1 - encoded) return (size_t)-1;
        out_len += encoded;
        i++;
    }
    if (dest && i == source_len && out_len < n) dest[out_len] = '\0';
    return out_len;
}

size_t rin_unicode_mbsrtowcs32(uint32_t* dest, const char** src, size_t len, rin_unicode_mbstate_t* ps) {
    size_t out_len = 0u;
    size_t source_len;
    const char* cursor;
    if (!src) return (size_t)-1;
    cursor = *src;
    if (!cursor) return 0u;
    if (!rin_unicode_cstring_length(cursor, &source_len)) return (size_t)-1;
    while ((size_t)(cursor - *src) < source_len &&
           (dest == (uint32_t*)0 || out_len < len)) {
        uint32_t cp = 0u;
        size_t rc = rin_unicode_mbrtowc32(
            &cp, cursor, source_len - (size_t)(cursor - *src), ps);
        if (rc == (size_t)-1 || rc == (size_t)-2) return (size_t)-1;
        if (dest) dest[out_len] = cp;
        out_len++;
        cursor += rc;
    }
    if ((size_t)(cursor - *src) == source_len) {
        if (!dest || out_len < len) {
            if (dest) dest[out_len] = 0u;
            *src = (const char*)0;
        } else {
            /* The terminator was observed but did not fit.  The caller must
             * be able to resume at that unconverted source code unit. */
            *src = cursor;
        }
    } else {
        *src = cursor;
    }
    return out_len;
}

size_t rin_unicode_wcsrtombs32(char* dest, const uint32_t** src, size_t len, rin_unicode_mbstate_t* ps) {
    size_t out_len = 0u;
    const uint32_t* cursor;
    size_t source_len;
    (void)ps;
    if (!src) return (size_t)-1;
    cursor = *src;
    if (!cursor) return 0u;
    if (!rin_unicode_wstring_length(cursor, &source_len)) return (size_t)-1;
    while (out_len < (size_t)-1 &&
           (size_t)(cursor - *src) < source_len) {
        size_t encoded = 0u;
        size_t written = 0u;
        if (rin_unicode_encode_utf8((char*)0, 0u, *cursor, &encoded) !=
            RIN_UNICODE_OK) {
            return (size_t)-1;
        }
        if (dest && (out_len > len || encoded > len - out_len)) break;
        if (dest && rin_unicode_encode_utf8(dest + out_len,
                len - out_len, *cursor, &written) != RIN_UNICODE_OK) {
            return (size_t)-1;
        }
        if (out_len > (size_t)-1 - encoded) return (size_t)-1;
        out_len += encoded;
        cursor++;
    }
    if ((size_t)(cursor - *src) == source_len) {
        if (!dest || out_len < len) {
            if (dest) dest[out_len] = '\0';
            *src = (const uint32_t*)0;
        } else {
            /* A full destination leaves the source at its unconverted NUL. */
            *src = cursor;
        }
    } else {
        *src = cursor;
    }
    return out_len;
}

unsigned long rin_unicode_wctype(const char* property) {
    if (rin_unicode_ascii_ieq(property, "alnum")) return RIN_UNICODE_WCTYPE_ALNUM;
    if (rin_unicode_ascii_ieq(property, "alpha")) return RIN_UNICODE_WCTYPE_ALPHA;
    if (rin_unicode_ascii_ieq(property, "blank")) return RIN_UNICODE_WCTYPE_BLANK;
    if (rin_unicode_ascii_ieq(property, "cntrl")) return RIN_UNICODE_WCTYPE_CNTRL;
    if (rin_unicode_ascii_ieq(property, "digit")) return RIN_UNICODE_WCTYPE_DIGIT;
    if (rin_unicode_ascii_ieq(property, "graph")) return RIN_UNICODE_WCTYPE_GRAPH;
    if (rin_unicode_ascii_ieq(property, "lower")) return RIN_UNICODE_WCTYPE_LOWER;
    if (rin_unicode_ascii_ieq(property, "print")) return RIN_UNICODE_WCTYPE_PRINT;
    if (rin_unicode_ascii_ieq(property, "punct")) return RIN_UNICODE_WCTYPE_PUNCT;
    if (rin_unicode_ascii_ieq(property, "space")) return RIN_UNICODE_WCTYPE_SPACE;
    if (rin_unicode_ascii_ieq(property, "upper")) return RIN_UNICODE_WCTYPE_UPPER;
    if (rin_unicode_ascii_ieq(property, "xdigit")) return RIN_UNICODE_WCTYPE_XDIGIT;
    return 0u;
}

int rin_unicode_iswctype(uint32_t cp, unsigned long desc) {
    switch (desc) {
    case RIN_UNICODE_WCTYPE_ALNUM: return rin_unicode_isalnum(cp);
    case RIN_UNICODE_WCTYPE_ALPHA: return rin_unicode_isalpha(cp);
    case RIN_UNICODE_WCTYPE_BLANK: return rin_unicode_isblank(cp);
    case RIN_UNICODE_WCTYPE_CNTRL: return rin_unicode_iscntrl(cp);
    case RIN_UNICODE_WCTYPE_DIGIT: return rin_unicode_isdigit(cp);
    case RIN_UNICODE_WCTYPE_GRAPH: return rin_unicode_isgraph(cp);
    case RIN_UNICODE_WCTYPE_LOWER: return rin_unicode_islower(cp);
    case RIN_UNICODE_WCTYPE_PRINT: return rin_unicode_isprint(cp);
    case RIN_UNICODE_WCTYPE_PUNCT: return rin_unicode_ispunct(cp);
    case RIN_UNICODE_WCTYPE_SPACE: return rin_unicode_isspace(cp);
    case RIN_UNICODE_WCTYPE_UPPER: return rin_unicode_isupper(cp);
    case RIN_UNICODE_WCTYPE_XDIGIT: return rin_unicode_isxdigit(cp);
    default: return 0;
    }
}

unsigned long rin_unicode_wctrans(const char* property) {
    if (rin_unicode_ascii_ieq(property, "tolower")) return RIN_UNICODE_WCTRANS_TOLOWER;
    if (rin_unicode_ascii_ieq(property, "toupper")) return RIN_UNICODE_WCTRANS_TOUPPER;
    return 0u;
}

uint32_t rin_unicode_towctrans(uint32_t cp, unsigned long desc) {
    switch (desc) {
    case RIN_UNICODE_WCTRANS_TOLOWER: return rin_unicode_tolower(cp);
    case RIN_UNICODE_WCTRANS_TOUPPER: return rin_unicode_toupper(cp);
    default: return cp;
    }
}
