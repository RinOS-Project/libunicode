/* SPDX-License-Identifier: MIT */

#include <stddef.h>

#include "../rin_unicode.h"

int main(void) {
    static const char words[] = "hello world";
    static const char hard_break[] = "a\nb";
    static const char cjk[] = "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E";
    static const char family[] =
        "\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA9" "x";
    static const char punctuation[] = "(word)";
    size_t words_len = sizeof(words) - 1u;
    size_t hard_len = sizeof(hard_break) - 1u;
    size_t cjk_len = sizeof(cjk) - 1u;
    size_t family_len = sizeof(family) - 1u;
    size_t punctuation_len = sizeof(punctuation) - 1u;

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
    return 0;
}
