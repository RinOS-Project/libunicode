/* SPDX-License-Identifier: MIT */
/* Unicode 13.0.0 GraphemeBreakProperty exceptions to General_Category. */
#ifndef RIN_UNICODE_GRAPHEME_BREAK_DATA_H
#define RIN_UNICODE_GRAPHEME_BREAK_DATA_H

/* These Mc characters are GCB=Other rather than the generated Mc spacing
 * mark set. */
static const uint32_t g_rin_unicode_grapheme_other_overrides[][2] = {
    { 0x102Bu, 0x102Cu },
    { 0x1038u, 0x1038u },
    { 0x1062u, 0x1064u },
    { 0x1067u, 0x106Du },
    { 0x1083u, 0x1083u },
    { 0x1087u, 0x108Cu },
    { 0x108Fu, 0x108Fu },
    { 0x109Au, 0x109Cu },
    { 0x1A61u, 0x1A61u },
    { 0x1A63u, 0x1A64u },
    { 0xAA7Bu, 0xAA7Bu },
    { 0xAA7Du, 0xAA7Du },
};

/* These Mc characters are GCB=Extend rather than GCB=SpacingMark. */
static const uint32_t g_rin_unicode_grapheme_extend_overrides[][2] = {
    { 0x09BEu, 0x09BEu },
    { 0x09D7u, 0x09D7u },
    { 0x0B3Eu, 0x0B3Eu },
    { 0x0B57u, 0x0B57u },
    { 0x0BBEu, 0x0BBEu },
    { 0x0BD7u, 0x0BD7u },
    { 0x0CC2u, 0x0CC2u },
    { 0x0CD5u, 0x0CD6u },
    { 0x0D3Eu, 0x0D3Eu },
    { 0x0D57u, 0x0D57u },
    { 0x0DCFu, 0x0DCFu },
    { 0x0DDFu, 0x0DDFu },
    { 0x1B35u, 0x1B35u },
    { 0x302Eu, 0x302Fu },
    { 0xFF9Eu, 0xFF9Fu },
    { 0x1133Eu, 0x1133Eu },
    { 0x11357u, 0x11357u },
    { 0x114B0u, 0x114B0u },
    { 0x114BDu, 0x114BDu },
    { 0x115AFu, 0x115AFu },
    { 0x11930u, 0x11930u },
    { 0x1D165u, 0x1D165u },
    { 0x1D16Eu, 0x1D172u },
};

#endif
