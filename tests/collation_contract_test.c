/* SPDX-License-Identifier: MIT */

#include <stddef.h>

#include "../rin_unicode.h"

int main(void) {
    static const char reordered_left[] = {
        'a', (char)0xcc, (char)0x95, (char)0xcc, (char)0x80, '\0'};
    static const char reordered_right[] = {
        'a', (char)0xcc, (char)0x80, (char)0xcc, (char)0x95, '\0'};
    static const char expected[] = {
        'a', (char)0xcc, (char)0x80, (char)0xcc, (char)0x95, '\0'};
    char transformed[sizeof(expected)] = {};
    size_t transformed_size;

    if (rin_unicode_compare_utf8("A", "a") != 0)
        return 1;
    if (rin_unicode_compare_utf8(reordered_left, reordered_right) != 0)
        return 2;
    transformed_size = rin_unicode_transform_utf8(
        transformed, sizeof(transformed), reordered_left);
    if (transformed_size != sizeof(expected) - 1u)
        return 3;
    for (size_t index = 0u; index < sizeof(expected); ++index)
        if (transformed[index] != expected[index])
            return 4;
    return 0;
}
