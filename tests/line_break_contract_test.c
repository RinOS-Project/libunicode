/* SPDX-License-Identifier: MIT */

#include <stddef.h>

#include "../rin_unicode.h"

int main(void) {
    static const char words[] = "hello world";
    static const char hard_break[] = "a\nb";
    static const char vertical_break[] = {'a', '\v', 'b'};
    static const char form_break[] = {'a', '\f', 'b'};
    static const char cjk[] = {
        (char)0xE6, (char)0x97, (char)0xA5,
        (char)0xE6, (char)0x9C, (char)0xAC,
        (char)0xE8, (char)0xAA, (char)0x9E, '\0'
    };
    static const char family[] =
        "\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA9" "x";
    static const char punctuation[] = "(word)";
    static const char no_break_space[] =
        {'a', (char)0xC2, (char)0xA0, 'b'};
    static const char figure_space[] =
        {'a', (char)0xE2, (char)0x80, (char)0x87, 'b'};
    static const char narrow_no_break_space[] =
        {'a', (char)0xE2, (char)0x80, (char)0xAF, 'b'};
    static const char non_break_hyphen[] =
        {'a', (char)0xE2, (char)0x80, (char)0x91, 'b'};
    static const char word_joiner[] =
        {'a', (char)0xE2, (char)0x81, (char)0xA0, 'b'};
    static const char bom_joiner[] =
        {'a', (char)0xEF, (char)0xBB, (char)0xBF, 'b'};
    static const char word_joiners[][6] = {
        {'a', (char)0xE2, (char)0x81, (char)0xA1, 'b', '\0'},
        {'a', (char)0xE2, (char)0x81, (char)0xA2, 'b', '\0'},
        {'a', (char)0xE2, (char)0x81, (char)0xA3, 'b', '\0'},
        {'a', (char)0xE2, (char)0x81, (char)0xA4, 'b', '\0'}
    };
    size_t words_len = sizeof(words) - 1u;
    size_t hard_len = sizeof(hard_break) - 1u;
    size_t cjk_len = sizeof(cjk) - 1u;
    size_t family_len = sizeof(family) - 1u;
    size_t punctuation_len = sizeof(punctuation) - 1u;
    size_t no_break_space_len = sizeof(no_break_space);
    size_t figure_space_len = sizeof(figure_space);
    size_t narrow_no_break_space_len = sizeof(narrow_no_break_space);
    size_t non_break_hyphen_len = sizeof(non_break_hyphen);
    size_t word_joiner_len = sizeof(word_joiner);
    size_t bom_joiner_len = sizeof(bom_joiner);

    if (rin_unicode_line_break_opportunity(words, words_len, 5u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 1;
    if (rin_unicode_line_break_opportunity(words, words_len, 6u) !=
        RIN_UNICODE_LINE_BREAK_ALLOWED)
        return 2;
    if (rin_unicode_line_break_next(words, words_len, 0u) != 6u)
        return 3;
    if (rin_unicode_line_break_opportunity(hard_break, hard_len, 2u) !=
        RIN_UNICODE_LINE_BREAK_MANDATORY)
        return 4;
    if (rin_unicode_line_break_next(hard_break, hard_len, 0u) != 2u)
        return 5;
    if (rin_unicode_line_break_opportunity(vertical_break,
                                           sizeof(vertical_break), 2u) !=
        RIN_UNICODE_LINE_BREAK_MANDATORY ||
        rin_unicode_line_break_next(vertical_break,
                                    sizeof(vertical_break), 0u) != 2u)
        return 19;
    if (rin_unicode_line_break_opportunity(form_break,
                                           sizeof(form_break), 2u) !=
        RIN_UNICODE_LINE_BREAK_MANDATORY ||
        rin_unicode_line_break_next(form_break, sizeof(form_break), 0u) != 2u)
        return 20;
    if (rin_unicode_line_break_opportunity(cjk, cjk_len, 3u) !=
        RIN_UNICODE_LINE_BREAK_ALLOWED)
        return 6;
    if (rin_unicode_line_break_next(cjk, cjk_len, 0u) != 3u)
        return 7;
    if (rin_unicode_line_break_next(family, family_len, 0u) != family_len)
        return 8;
    if (rin_unicode_line_break_next(family, family_len, 3u) != family_len)
        return 9;
    if (rin_unicode_line_break_opportunity(punctuation, punctuation_len, 1u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 10;
    if (rin_unicode_line_break_opportunity(punctuation, punctuation_len, 5u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 11;
    if (rin_unicode_line_break_opportunity(NULL, 0u, 0u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 12;
    if (rin_unicode_line_break_opportunity(words, words_len, 1u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 13;
    if (rin_unicode_line_break_opportunity(no_break_space,
                                          no_break_space_len, 1u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(no_break_space,
                                           no_break_space_len, 3u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 14;
    if (rin_unicode_line_break_opportunity(figure_space,
                                          figure_space_len, 1u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(figure_space,
                                           figure_space_len, 4u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 15;
    if (rin_unicode_line_break_opportunity(narrow_no_break_space,
                                          narrow_no_break_space_len, 1u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(narrow_no_break_space,
                                           narrow_no_break_space_len, 4u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 16;
    if (rin_unicode_line_break_opportunity(non_break_hyphen,
                                          non_break_hyphen_len, 1u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(non_break_hyphen,
                                           non_break_hyphen_len, 4u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 21;
    if (rin_unicode_line_break_opportunity(word_joiner, word_joiner_len, 1u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(word_joiner, word_joiner_len, 4u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 17;
    if (rin_unicode_line_break_opportunity(bom_joiner, bom_joiner_len, 1u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(bom_joiner, bom_joiner_len, 4u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 18;
    for (size_t index = 0u; index < sizeof(word_joiners) /
                                    sizeof(word_joiners[0]); ++index) {
        if (rin_unicode_line_break_opportunity(word_joiners[index], 5u, 1u) !=
                RIN_UNICODE_LINE_BREAK_PROHIBITED ||
            rin_unicode_line_break_opportunity(word_joiners[index], 5u, 4u) !=
                RIN_UNICODE_LINE_BREAK_PROHIBITED)
            return 22;
    }
    return 0;
}
