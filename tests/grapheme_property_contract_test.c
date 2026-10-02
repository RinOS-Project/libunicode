/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>

#include "../rin_unicode.h"

int main(void)
{
    assert(rin_unicode_grapheme_property(0x1A55u) ==
           RIN_UNICODE_GRAPHEME_SPACING_MARK);
    assert(rin_unicode_grapheme_property(0x0E33u) ==
           RIN_UNICODE_GRAPHEME_SPACING_MARK);
    assert(rin_unicode_grapheme_property(0x0EB3u) ==
           RIN_UNICODE_GRAPHEME_SPACING_MARK);
    assert(rin_unicode_grapheme_property(0x1E944u) ==
           RIN_UNICODE_GRAPHEME_EXTEND);
    assert(rin_unicode_grapheme_property(0x1193Fu) ==
           RIN_UNICODE_GRAPHEME_PREPEND);
    assert(rin_unicode_grapheme_property(0x061Cu) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0xFFF0u) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0xFFFBu) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0x13430u) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0x13438u) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0x1BCA0u) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0x1BCA3u) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0x1D173u) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0x1D17Au) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0xE0000u) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0xE001Fu) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0xE0080u) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0xE00FFu) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0xE01F0u) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0xE0FFFu) ==
           RIN_UNICODE_GRAPHEME_CONTROL);
    assert(rin_unicode_grapheme_property(0x1F1FAu) ==
           RIN_UNICODE_GRAPHEME_RI);
    assert(rin_unicode_grapheme_property(0x1100u) ==
           RIN_UNICODE_GRAPHEME_L);
    assert(rin_unicode_grapheme_property(0x1161u) ==
           RIN_UNICODE_GRAPHEME_V);
    assert(rin_unicode_grapheme_property(0x11A8u) ==
           RIN_UNICODE_GRAPHEME_T);
    assert(rin_unicode_grapheme_property(0xAC00u) ==
           RIN_UNICODE_GRAPHEME_LV);
    assert(rin_unicode_grapheme_property(0xAC01u) ==
           RIN_UNICODE_GRAPHEME_LVT);
    assert(rin_unicode_grapheme_property('A') ==
           RIN_UNICODE_GRAPHEME_OTHER);
    assert(rin_unicode_is_extended_pictographic(0x00A9u));
    assert(rin_unicode_is_extended_pictographic(0x1FAE0u));
    assert(rin_unicode_is_extended_pictographic(0x24C2u));
    assert(rin_unicode_is_extended_pictographic(0x2B50u));
    assert(rin_unicode_is_extended_pictographic(0x2934u));
    assert(rin_unicode_is_extended_pictographic(0x3030u));
    assert(rin_unicode_is_extended_pictographic(0x303Du));
    assert(rin_unicode_is_extended_pictographic(0x3297u));
    assert(rin_unicode_is_extended_pictographic(0x3299u));
    assert(!rin_unicode_is_extended_pictographic(0x1F1FAu));
    {
        static const char regional_zwj_pictograph[] =
            "\xF0\x9F\x87\xBA\xE2\x80\x8D\xF0\x9F\x98\x80";
        assert(rin_unicode_grapheme_next(
                   regional_zwj_pictograph,
                   sizeof(regional_zwj_pictograph) - 1u, 0u) == 7u);
    }
    assert(!rin_unicode_is_extended_pictographic('A'));
    return 0;
}
