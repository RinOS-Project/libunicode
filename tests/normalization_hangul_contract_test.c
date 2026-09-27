/* SPDX-License-Identifier: MIT */

#include <stddef.h>
#include <stdint.h>

#include "../rin_unicode.h"

int main(void)
{
    static const uint32_t syllable[] = {0xac01u, 0u};
    static const uint32_t jamo[] = {0x1100u, 0x1161u, 0x11a8u, 0u};
    uint32_t decomposed[4] = {0xfeedu, 0xfeedu, 0xfeedu, 0u};
    uint32_t composed[4] = {0xfeedu, 0xfeedu, 0xfeedu, 0u};
    uint32_t compatibility_decomposed[4] = {
        0xfeedu, 0xfeedu, 0xfeedu, 0u};
    uint32_t compatibility_composed[4] = {
        0xfeedu, 0xfeedu, 0xfeedu, 0u};

    if (rin_unicode_normalize_utf32(
            decomposed, 4u, syllable, (size_t)-1,
            RIN_UNICODE_NORMALIZE_NFD) != 3u ||
        decomposed[0] != 0x1100u || decomposed[1] != 0x1161u ||
        decomposed[2] != 0x11a8u || decomposed[3] != 0u)
        return 1;
    if (rin_unicode_normalize_utf32(
            composed, 4u, jamo, (size_t)-1,
            RIN_UNICODE_NORMALIZE_NFC) != 1u ||
        composed[0] != 0xac01u || composed[1] != 0u)
        return 2;
    if (rin_unicode_normalize_utf32(
            compatibility_decomposed, 4u, syllable, (size_t)-1,
            RIN_UNICODE_NORMALIZE_NFKD) != 3u ||
        compatibility_decomposed[0] != 0x1100u ||
        compatibility_decomposed[1] != 0x1161u ||
        compatibility_decomposed[2] != 0x11a8u ||
        compatibility_decomposed[3] != 0u)
        return 3;
    if (rin_unicode_normalize_utf32(
            compatibility_composed, 4u, jamo, (size_t)-1,
            RIN_UNICODE_NORMALIZE_NFKC) != 1u ||
        compatibility_composed[0] != 0xac01u ||
        compatibility_composed[1] != 0u)
        return 4;
    return 0;
}
