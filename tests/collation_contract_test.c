/* SPDX-License-Identifier: MIT */

#include <stddef.h>
#include <stdint.h>

#include "../rin_unicode.h"

int main(void) {
    static const char reordered_left[] = {
        'a', (char)0xcc, (char)0x95, (char)0xcc, (char)0x80, '\0'};
    static const char reordered_right[] = {
        'a', (char)0xcc, (char)0x80, (char)0xcc, (char)0x95, '\0'};
    static const char expected[] = {
        'a', (char)0xcc, (char)0x80, (char)0xcc, (char)0x95, '\0'};
    static const uint32_t invalid_scalar[] = {0xd800u, 0u};
    static const uint32_t valid_scalar[] = {0xfffdu, 0u};
    char transformed[sizeof(expected)] = {};
    size_t transformed_size;

    if (rin_unicode_compare_utf8("A", "a") != 0)
        return 1;
    if (rin_unicode_compare_utf8("\x80", "\xEF\xBF\xBD") != 0)
        return 2;
    if (rin_unicode_compare_utf32(invalid_scalar, valid_scalar) != 1)
        return 3;
    if (rin_unicode_compare_utf32(valid_scalar, invalid_scalar) != -1)
        return 4;
    if (rin_unicode_compare_utf32(invalid_scalar, invalid_scalar) != 0)
        return 5;
    if (rin_unicode_compare_utf8(reordered_left, reordered_right) != 0)
        return 6;
    transformed_size = rin_unicode_transform_utf8(
        transformed, sizeof(transformed), reordered_left);
    if (transformed_size != sizeof(expected) - 1u)
        return 7;
    for (size_t index = 0u; index < sizeof(expected); ++index)
        if (transformed[index] != expected[index])
            return 8;
    return 0;
}
