/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>

#include "../rin_unicode.h"

int main(void)
{
    assert(rin_unicode_grapheme_property(0x1A55u) ==
           RIN_UNICODE_GRAPHEME_SPACING_MARK);
    assert(rin_unicode_grapheme_property(0x1E944u) ==
           RIN_UNICODE_GRAPHEME_EXTEND);
    assert(rin_unicode_grapheme_property(0x1193Fu) ==
           RIN_UNICODE_GRAPHEME_PREPEND);
    assert(rin_unicode_grapheme_property(0x061Cu) ==
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
    assert(!rin_unicode_is_extended_pictographic('A'));
    return 0;
}
