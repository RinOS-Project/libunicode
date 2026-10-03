/* SPDX-License-Identifier: MIT */

#include <stddef.h>

#include "../rin_unicode.h"

int main(void) {
    static const char words[] = "hello world";
    static const char multiple_spaces[] = {'a', ' ', ' ', 'b'};
    static const char hard_break[] = "a\nb";
    static const char crlf_break[] = {'a', '\r', '\n', 'b'};
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
    static const char quotation[] = "\xE2\x80\x9Cword\xE2\x80\x9D";
    static const char japanese_nonstarter[] = {
        (char)0xE3, (char)0x81, (char)0x82,
        (char)0xE3, (char)0x81, (char)0x83, '\0'
    };
    static const char no_break_space[] =
        {'a', (char)0xC2, (char)0xA0, 'b'};
    static const char figure_space[] =
        {'a', (char)0xE2, (char)0x80, (char)0x87, 'b'};
    static const char narrow_no_break_space[] =
        {'a', (char)0xE2, (char)0x80, (char)0xAF, 'b'};
    static const char non_break_hyphen[] =
        {'a', (char)0xE2, (char)0x80, (char)0x91, 'b'};
    static const char zwj_after_break[] =
        {'a', '-', (char)0xE2, (char)0x80, (char)0x8D, 'b'};
    static const char zwj_before_break_before[] = {
        'a', '-', (char)0xE2, (char)0x80, (char)0x8D,
        (char)0xC2, (char)0xB4, 'b'};
    static const char zwnj_after_break[] =
        {'a', '-', (char)0xE2, (char)0x80, (char)0x8C, 'b'};
    static const char arabic_mark_after_break[] =
        {'a', '-', (char)0xD8, (char)0x9C, 'b'};
    static const char mongolian_mark_after_break[] =
        {'a', '-', (char)0xE1, (char)0xA0, (char)0x8E, 'b'};
    static const char word_joiner[] =
        {'a', (char)0xE2, (char)0x81, (char)0xA0, 'b'};
    static const char bom_joiner[] =
        {'a', (char)0xEF, (char)0xBB, (char)0xBF, 'b'};
    static const char malformed[] =
        {'a', (char)0xC2, ' ', 'b'};
    static const char word_joiners[][6] = {
        {'a', (char)0xE2, (char)0x81, (char)0xA1, 'b', '\0'},
        {'a', (char)0xE2, (char)0x81, (char)0xA2, 'b', '\0'},
        {'a', (char)0xE2, (char)0x81, (char)0xA3, 'b', '\0'},
        {'a', (char)0xE2, (char)0x81, (char)0xA4, 'b', '\0'}
    };
    static const char break_after[][7] = {
        {'a', (char)0xD6, (char)0x8A, 'b', '\0'},
        {'a', (char)0xD6, (char)0xBE, 'b', '\0'},
        {'a', (char)0xE1, (char)0x90, (char)0x80, 'b', '\0'},
        {'a', (char)0xE1, (char)0xA0, (char)0x86, 'b', '\0'},
        {'a', (char)0xE2, (char)0x80, (char)0x92, 'b', '\0'},
        {'a', (char)0xEF, (char)0xB8, (char)0xB1, 'b', '\0'},
        {'a', (char)0xF0, (char)0x90, (char)0xBA, (char)0xAD, 'b', '\0'}
    };
    static const char official_ba[] = {'a', '|', 'b', '\0'};
    static const char official_non_ascii_ba[] = {
        'a', (char)0xE0, (char)0xA5, (char)0xA4, 'b', '\0'};
    static const char break_before_after[][6] = {
        {'a', (char)0xE2, (char)0x80, (char)0x94, 'b', '\0'}
    };
    static const char break_before[][6] = {
        {'a', (char)0xC2, (char)0xB4, 'b', '\0'},
        {'a', (char)0xE1, (char)0xBF, (char)0xBD, 'b', '\0'},
        {'a', (char)0xCB, (char)0x88, 'b', '\0'},
        {'a', (char)0xCB, (char)0x8C, 'b', '\0'},
        {'a', (char)0xCB, (char)0x9F, 'b', '\0'}
    };
    static const size_t break_before_lengths[] = {4u, 5u, 4u, 4u, 4u};
    static const char horizontal_bar[] = {
        'a', (char)0xE2, (char)0x80, (char)0x95, 'b', '\0'};
    static const char regional_indicators[] = {
        (char)0xF0, (char)0x9F, (char)0x87, (char)0xAF,
        (char)0xF0, (char)0x9F, (char)0x87, (char)0xB5,
        (char)0xF0, (char)0x9F, (char)0x87, (char)0xA9,
        (char)0xF0, (char)0x9F, (char)0x87, (char)0xB5, '\0'};
    static const char regional_indicator_with_mark[] = {
        (char)0xF0, (char)0x9F, (char)0x87, (char)0xAF,
        (char)0xCC, (char)0x81,
        (char)0xF0, (char)0x9F, (char)0x87, (char)0xB5,
        (char)0xF0, (char)0x9F, (char)0x87, (char)0xA9, '\0'};
    static const char solidus[] = "a/b";
    static const char numeric_solidus[] = "1/2";
    static const char nonstarter[][8] = {
        {'a', '-', (char)0xE3, (char)0x80, (char)0xBB, 'b', '\0'},
        {'a', '-', (char)0xE3, (char)0x87, (char)0xB0, 'b', '\0'},
        {'a', '-', (char)0xEF, (char)0xBD, (char)0xA5, 'b', '\0'},
        {'a', '-', (char)0xEF, (char)0xBD, (char)0xA7, 'b', '\0'},
        {'a', '-', (char)0xE3, (char)0x80, (char)0x85, 'b', '\0'},
        {'a', '-', (char)0xE3, (char)0x80, (char)0xBC, 'b', '\0'},
        {'a', '-', (char)0xE3, (char)0x82, (char)0x9B, 'b', '\0'},
        {'a', '-', (char)0xE3, (char)0x82, (char)0x9C, 'b', '\0'},
        {'a', '-', (char)0xE3, (char)0x82, (char)0x9D, 'b', '\0'},
        {'a', '-', (char)0xE3, (char)0x82, (char)0x9E, 'b', '\0'},
        {'a', '-', (char)0xE3, (char)0x80, (char)0x9C, 'b', '\0'},
        {'a', '-', (char)0xE3, (char)0x82, (char)0xA0, 'b', '\0'},
        {'a', '-', (char)0xE2, (char)0x80, (char)0xBC, 'b', '\0'},
        {'a', '-', (char)0xE2, (char)0x80, (char)0xBD, 'b', '\0'},
        {'a', '-', (char)0xE2, (char)0x81, (char)0x87, 'b', '\0'},
        {'a', '-', (char)0xE2, (char)0x81, (char)0x88, 'b', '\0'},
        {'a', '-', (char)0xE2, (char)0x81, (char)0x89, 'b', '\0'},
        {'a', '-', (char)0xEF, (char)0xB8, (char)0x93, 'b', '\0'},
        {'a', '-', (char)0xEF, (char)0xB8, (char)0x94, 'b', '\0'},
        {'a', '-', (char)0xEF, (char)0xB9, (char)0x94, 'b', '\0'},
        {'a', '-', (char)0xEF, (char)0xB9, (char)0x95, 'b', '\0'},
        {'a', '-', (char)0xEF, (char)0xBE, (char)0x9E, 'b', '\0'},
        {'a', '-', (char)0xEF, (char)0xBE, (char)0x9F, 'b', '\0'}
    };
    static const char fullwidth_punctuation[][11] = {
        {(char)0xE6, (char)0x96, (char)0x87,
         (char)0xEF, (char)0xBC, (char)0x81,
         (char)0xE8, (char)0xAA, (char)0x9E, '\0'},
        {(char)0xE6, (char)0x96, (char)0x87,
         (char)0xEF, (char)0xBC, (char)0x9A,
         (char)0xE8, (char)0xAA, (char)0x9E, '\0'},
        {(char)0xE6, (char)0x96, (char)0x87,
         (char)0xEF, (char)0xBC, (char)0x9B,
         (char)0xE8, (char)0xAA, (char)0x9E, '\0'},
        {(char)0xE6, (char)0x96, (char)0x87,
         (char)0xEF, (char)0xBC, (char)0x9F,
         (char)0xE8, (char)0xAA, (char)0x9E, '\0'}
    };
    static const char fullwidth_white_parentheses[][11] = {
        {(char)0xE6, (char)0x96, (char)0x87,
         (char)0xEF, (char)0xBD, (char)0x9F,
         (char)0xE8, (char)0xAA, (char)0x9E, '\0'},
        {(char)0xE6, (char)0x96, (char)0x87,
         (char)0xEF, (char)0xBD, (char)0xA0,
         (char)0xE8, (char)0xAA, (char)0x9E, '\0'}
    };
    static const char additional_quote_and_angle[][12] = {
        {(char)0xE6, (char)0x96, (char)0x87,
         (char)0xE2, (char)0x80, (char)0x9A,
         (char)0xE8, (char)0xAA, (char)0x9E, '\0'},
        {(char)0xE6, (char)0x96, (char)0x87,
         (char)0xE2, (char)0x80, (char)0x9E,
         (char)0xE8, (char)0xAA, (char)0x9E, '\0'},
        {(char)0xE6, (char)0x96, (char)0x87,
         (char)0xE2, (char)0x8C, (char)0xA9,
         (char)0xE8, (char)0xAA, (char)0x9E, '\0'},
        {(char)0xE6, (char)0x96, (char)0x87,
         (char)0xE2, (char)0x8C, (char)0xAA,
         (char)0xE8, (char)0xAA, (char)0x9E, '\0'},
        {(char)0xE6, (char)0x96, (char)0x87,
         (char)0xE3, (char)0x80, (char)0x9D,
         (char)0xE8, (char)0xAA, (char)0x9E, '\0'},
        {(char)0xE6, (char)0x96, (char)0x87,
         (char)0xE3, (char)0x80, (char)0x9E,
         (char)0xE8, (char)0xAA, (char)0x9E, '\0'},
        {(char)0xE6, (char)0x96, (char)0x87,
         (char)0xE3, (char)0x80, (char)0x9F,
         (char)0xE8, (char)0xAA, (char)0x9E, '\0'}
    };
    static const size_t break_after_lengths[] = {4u, 4u, 5u, 5u,
                                                  5u, 5u, 6u};
    static const size_t break_after_offsets[] = {3u, 3u, 4u, 4u,
                                                  4u, 4u, 5u};
    size_t words_len = sizeof(words) - 1u;
    size_t multiple_spaces_len = sizeof(multiple_spaces);
    size_t hard_len = sizeof(hard_break) - 1u;
    size_t crlf_len = sizeof(crlf_break);
    size_t cjk_len = sizeof(cjk) - 1u;
    size_t family_len = sizeof(family) - 1u;
    size_t punctuation_len = sizeof(punctuation) - 1u;
    size_t quotation_len = sizeof(quotation) - 1u;
    size_t japanese_nonstarter_len = sizeof(japanese_nonstarter) - 1u;
    size_t no_break_space_len = sizeof(no_break_space);
    size_t figure_space_len = sizeof(figure_space);
    size_t narrow_no_break_space_len = sizeof(narrow_no_break_space);
    size_t non_break_hyphen_len = sizeof(non_break_hyphen);
    size_t word_joiner_len = sizeof(word_joiner);
    size_t bom_joiner_len = sizeof(bom_joiner);
    size_t malformed_len = sizeof(malformed);

    if (rin_unicode_line_break_opportunity(words, words_len, 5u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 1;
    if (rin_unicode_line_break_opportunity(words, words_len, 6u) !=
        RIN_UNICODE_LINE_BREAK_ALLOWED)
        return 2;
    if (rin_unicode_line_break_next(words, words_len, 0u) != 6u)
        return 3;
    if (rin_unicode_line_break_opportunity(multiple_spaces,
                                           multiple_spaces_len, 2u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(multiple_spaces,
                                           multiple_spaces_len, 3u) !=
            RIN_UNICODE_LINE_BREAK_ALLOWED)
        return 26;
    if (rin_unicode_line_break_opportunity(hard_break, hard_len, 2u) !=
        RIN_UNICODE_LINE_BREAK_MANDATORY)
        return 4;
    if (rin_unicode_line_break_next(hard_break, hard_len, 0u) != 2u)
        return 5;
    if (rin_unicode_line_break_opportunity(crlf_break, crlf_len, 2u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(crlf_break, crlf_len, 3u) !=
            RIN_UNICODE_LINE_BREAK_MANDATORY ||
        rin_unicode_line_break_next(crlf_break, crlf_len, 0u) != 3u)
        return 25;
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
    if (rin_unicode_line_break_opportunity(quotation, quotation_len, 3u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(quotation, quotation_len, 7u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 23;
    if (rin_unicode_line_break_opportunity(japanese_nonstarter,
                                           japanese_nonstarter_len, 3u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_next(japanese_nonstarter,
                                    japanese_nonstarter_len, 0u) !=
            japanese_nonstarter_len)
        return 24;
    if (rin_unicode_line_break_opportunity(NULL, 0u, 0u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 12;
    if (rin_unicode_line_break_opportunity(malformed, malformed_len, 3u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_next(malformed, malformed_len, 0u) !=
            malformed_len)
        return 28;
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
    if (rin_unicode_line_break_opportunity(zwj_after_break,
                                           sizeof(zwj_after_break), 2u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 31;
    if (rin_unicode_line_break_opportunity(zwj_before_break_before,
                                           sizeof(zwj_before_break_before), 5u) !=
        RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 38;
    if (rin_unicode_line_break_opportunity(zwnj_after_break,
                                           sizeof(zwnj_after_break), 2u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(arabic_mark_after_break,
                                           sizeof(arabic_mark_after_break),
                                           2u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(mongolian_mark_after_break,
                                           sizeof(mongolian_mark_after_break),
                                           2u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 32;
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
    for (size_t index = 0u; index < sizeof(break_after) /
                                    sizeof(break_after[0]); ++index) {
        if (rin_unicode_line_break_opportunity(
                break_after[index], break_after_lengths[index],
                break_after_offsets[index]) != RIN_UNICODE_LINE_BREAK_ALLOWED)
            return 27;
    }
    if (rin_unicode_line_break_opportunity(official_ba, sizeof(official_ba) - 1u,
                                           2u) != RIN_UNICODE_LINE_BREAK_ALLOWED ||
        rin_unicode_line_break_opportunity(official_non_ascii_ba,
                                           sizeof(official_non_ascii_ba) - 1u,
                                           4u) != RIN_UNICODE_LINE_BREAK_ALLOWED)
        return 32;
    if (rin_unicode_line_break_opportunity(regional_indicators,
                                           sizeof(regional_indicators) - 1u,
                                           8u) !=
            RIN_UNICODE_LINE_BREAK_ALLOWED ||
        rin_unicode_line_break_opportunity(regional_indicators,
                                           sizeof(regional_indicators) - 1u,
                                           4u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_next(regional_indicators,
                                    sizeof(regional_indicators) - 1u, 0u) != 8u)
        return 33;
    if (rin_unicode_line_break_opportunity(
            regional_indicator_with_mark,
            sizeof(regional_indicator_with_mark) - 1u, 10u) !=
        RIN_UNICODE_LINE_BREAK_ALLOWED)
        return 39;
    if (rin_unicode_line_break_opportunity(solidus, sizeof(solidus) - 1u, 1u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(solidus, sizeof(solidus) - 1u, 2u) !=
            RIN_UNICODE_LINE_BREAK_ALLOWED ||
        rin_unicode_line_break_next(solidus, sizeof(solidus) - 1u, 0u) != 2u ||
        rin_unicode_line_break_opportunity(numeric_solidus,
                                           sizeof(numeric_solidus) - 1u, 2u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 34;
    for (size_t index = 0u; index < sizeof(break_before_after) /
                                    sizeof(break_before_after[0]); ++index) {
        if (rin_unicode_line_break_opportunity(break_before_after[index], 5u,
                                               1u) !=
                RIN_UNICODE_LINE_BREAK_ALLOWED ||
            rin_unicode_line_break_opportunity(break_before_after[index], 5u,
                                               4u) !=
                RIN_UNICODE_LINE_BREAK_ALLOWED)
            return 29;
    }
    for (size_t index = 0u; index < sizeof(break_before) /
                                    sizeof(break_before[0]); ++index) {
        if (rin_unicode_line_break_opportunity(break_before[index],
                                               break_before_lengths[index], 1u) !=
                RIN_UNICODE_LINE_BREAK_ALLOWED ||
            rin_unicode_line_break_opportunity(break_before[index],
                                               break_before_lengths[index],
                                               break_before_lengths[index] -
                                                   1u) !=
                RIN_UNICODE_LINE_BREAK_PROHIBITED)
            return 37;
    }
    if (rin_unicode_line_break_opportunity(horizontal_bar,
                                           sizeof(horizontal_bar) - 1u, 1u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(horizontal_bar,
                                           sizeof(horizontal_bar) - 1u, 4u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED)
        return 36;
    for (size_t index = 0u; index < sizeof(nonstarter) /
                                    sizeof(nonstarter[0]); ++index) {
        if (rin_unicode_line_break_opportunity(nonstarter[index], 6u, 2u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED)
            return 30;
    }
    for (size_t index = 0u; index < sizeof(fullwidth_punctuation) /
                                    sizeof(fullwidth_punctuation[0]); ++index) {
        if (rin_unicode_line_break_opportunity(fullwidth_punctuation[index],
                                               9u, 3u) !=
                RIN_UNICODE_LINE_BREAK_PROHIBITED ||
            rin_unicode_line_break_opportunity(fullwidth_punctuation[index],
                                               9u, 6u) !=
                RIN_UNICODE_LINE_BREAK_ALLOWED)
            return 33;
    }
    if (rin_unicode_line_break_opportunity(fullwidth_white_parentheses[0],
                                           9u, 3u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(fullwidth_white_parentheses[0],
                                           9u, 6u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(fullwidth_white_parentheses[1],
                                           9u, 3u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(fullwidth_white_parentheses[1],
                                           9u, 6u) !=
            RIN_UNICODE_LINE_BREAK_ALLOWED)
        return 34;
    if (rin_unicode_line_break_opportunity(additional_quote_and_angle[0],
                                           9u, 3u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(additional_quote_and_angle[0],
                                           9u, 6u) !=
            RIN_UNICODE_LINE_BREAK_ALLOWED ||
        rin_unicode_line_break_opportunity(additional_quote_and_angle[1],
                                           9u, 3u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(additional_quote_and_angle[1],
                                           9u, 6u) !=
            RIN_UNICODE_LINE_BREAK_ALLOWED ||
        rin_unicode_line_break_opportunity(additional_quote_and_angle[2],
                                           9u, 3u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(additional_quote_and_angle[2],
                                           9u, 6u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(additional_quote_and_angle[3],
                                           9u, 3u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(additional_quote_and_angle[3],
                                           9u, 6u) !=
            RIN_UNICODE_LINE_BREAK_ALLOWED ||
        rin_unicode_line_break_opportunity(additional_quote_and_angle[4],
                                           9u, 3u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(additional_quote_and_angle[4],
                                           9u, 6u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(additional_quote_and_angle[5],
                                           9u, 3u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(additional_quote_and_angle[5],
                                           9u, 6u) !=
            RIN_UNICODE_LINE_BREAK_ALLOWED ||
        rin_unicode_line_break_opportunity(additional_quote_and_angle[6],
                                           9u, 3u) !=
            RIN_UNICODE_LINE_BREAK_PROHIBITED ||
        rin_unicode_line_break_opportunity(additional_quote_and_angle[6],
                                           9u, 6u) !=
            RIN_UNICODE_LINE_BREAK_ALLOWED)
        return 35;
    return 0;
}
