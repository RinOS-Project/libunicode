/*
 * Copyright (c) 2023, Simon Wanner <simon@skyrising.xyz>
 * Copyright (c) 2024, Tim Flynn <trflynn89@serenityos.org>
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Bounded Unicode 17.0 UTS #46 ToASCII implementation for public userland.
 */

#include "rin_unicode_idna.h"
#include "idna_data.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <utility>

namespace {

using namespace RinUnicode::IDNAData;

constexpr std::size_t kWorkspaceCodePoints = 131072u;
constexpr std::size_t kWorkspaceArrays = 3u;
constexpr std::size_t kMaxPunycodeLabel = 63u;
constexpr std::uint32_t kUnicodeLimit = 0x110000u;
constexpr std::uint32_t kHangulSyllableBase = 0xAC00u;
constexpr std::uint32_t kHangulLeadingBase = 0x1100u;
constexpr std::uint32_t kHangulVowelBase = 0x1161u;
constexpr std::uint32_t kHangulTrailingBase = 0x11A7u;
constexpr std::uint32_t kHangulVowelCount = 21u;
constexpr std::uint32_t kHangulTrailingCount = 28u;
constexpr std::uint32_t kHangulNCount = kHangulVowelCount * kHangulTrailingCount;
constexpr std::uint32_t kHangulSyllableCount = 19u * kHangulNCount;

const Uts46Row* mapping_row(std::uint32_t code_point)
{
    const auto it = std::upper_bound(
        uts46_rows.begin(), uts46_rows.end(), code_point,
        [](std::uint32_t value, const Uts46Row& row) {
            return value < row.start;
        });
    return it == uts46_rows.begin() ? nullptr : &*std::prev(it);
}

template<std::size_t N>
bool in_ranges(const std::array<Range, N>& ranges, std::uint32_t code_point)
{
    const auto it = std::upper_bound(
        ranges.begin(), ranges.end(), code_point,
        [](std::uint32_t value, const Range& range) {
            return value < range.first;
        });
    if (it == ranges.begin()) return false;
    const auto& range = *std::prev(it);
    return code_point >= range.first && code_point < range.end;
}

std::uint8_t combining_class(std::uint32_t code_point)
{
    const auto it = std::lower_bound(
        combining_classes.begin(), combining_classes.end(), code_point,
        [](const CombiningClass& row, std::uint32_t value) {
            return row.code_point < value;
        });
    return it != combining_classes.end() && it->code_point == code_point
        ? it->value : 0u;
}

std::uint8_t bidi_class(std::uint32_t code_point)
{
    const auto it = std::upper_bound(
        bidi_ranges.begin(), bidi_ranges.end(), code_point,
        [](std::uint32_t value, const BidiRange& range) {
            return value < range.first;
        });
    if (it == bidi_ranges.begin()) return 0u;
    const auto& range = *std::prev(it);
    return code_point < range.end ? range.value : 0u;
}

char joining_type(std::uint32_t code_point)
{
    const auto it = std::lower_bound(
        joining_types.begin(), joining_types.end(), code_point,
        [](const JoiningType& row, std::uint32_t value) {
            return row.code_point < value;
        });
    return it != joining_types.end() && it->code_point == code_point
        ? it->value : '\0';
}

std::uint32_t compose_pair(std::uint32_t first, std::uint32_t second)
{
    constexpr std::uint32_t leading_count = 19u;
    if (first >= kHangulLeadingBase &&
        first < kHangulLeadingBase + leading_count &&
        second >= kHangulVowelBase &&
        second < kHangulVowelBase + kHangulVowelCount) {
        return kHangulSyllableBase +
            ((first - kHangulLeadingBase) * kHangulVowelCount +
             (second - kHangulVowelBase)) * kHangulTrailingCount;
    }
    if (first >= kHangulSyllableBase &&
        first < kHangulSyllableBase + kHangulSyllableCount &&
        (first - kHangulSyllableBase) % kHangulTrailingCount == 0u &&
        second > kHangulTrailingBase &&
        second < kHangulTrailingBase + kHangulTrailingCount)
        return first + second - kHangulTrailingBase;

    const auto it = std::lower_bound(
        compositions.begin(), compositions.end(), std::pair{first, second},
        [](const Composition& row, const std::pair<std::uint32_t, std::uint32_t>& value) {
            return std::pair{row.first, row.second} < value;
        });
    return it != compositions.end() && it->first == first &&
            it->second == second ? it->composed : 0u;
}

bool append_code_point(std::uint32_t code_point, std::uint32_t* output,
                       std::size_t capacity, std::size_t& size)
{
    if (size >= capacity) return false;
    output[size++] = code_point;
    return true;
}

bool recursively_decompose(std::uint32_t code_point, std::uint32_t* output,
                           std::size_t capacity, std::size_t& size,
                           unsigned depth)
{
    if (depth > 32u) return false;
    if (code_point >= kHangulSyllableBase &&
        code_point < kHangulSyllableBase + kHangulSyllableCount) {
        const std::uint32_t index = code_point - kHangulSyllableBase;
        if (!append_code_point(kHangulLeadingBase + index / kHangulNCount,
                               output, capacity, size) ||
            !append_code_point(kHangulVowelBase +
                                   (index % kHangulNCount) /
                                       kHangulTrailingCount,
                               output, capacity, size))
            return false;
        const std::uint32_t trailing = index % kHangulTrailingCount;
        return trailing == 0u || append_code_point(
            kHangulTrailingBase + trailing, output, capacity, size);
    }

    const auto it = std::lower_bound(
        decompositions.begin(), decompositions.end(), code_point,
        [](const Decomposition& row, std::uint32_t value) {
            return row.code_point < value;
        });
    if (it == decompositions.end() || it->code_point != code_point)
        return append_code_point(code_point, output, capacity, size);
    for (std::size_t index = 0u; index < it->mapping_length; ++index) {
        if (!recursively_decompose(
                decomposition_mappings[it->mapping_offset + index], output,
                capacity, size, depth + 1u))
            return false;
    }
    return true;
}

bool normalize_nfc(const std::uint32_t* source, std::size_t source_size,
                   std::uint32_t* normalized, std::uint32_t* scratch,
                   std::size_t capacity, std::size_t& normalized_size)
{
    normalized_size = 0u;
    for (std::size_t index = 0u; index < source_size; ++index) {
        if (!recursively_decompose(source[index], normalized, capacity,
                                   normalized_size, 0u))
            return false;
    }

    /* Stable counting sort each non-starter run by canonical combining class.
     * The run is bounded by the workspace and sorting is linear in its size. */
    std::size_t cursor = 0u;
    while (cursor < normalized_size) {
        if (combining_class(normalized[cursor]) == 0u) {
            ++cursor;
            continue;
        }
        const std::size_t begin = cursor;
        while (cursor < normalized_size &&
               combining_class(normalized[cursor]) != 0u)
            ++cursor;
        const std::size_t end = cursor;
        std::array<std::size_t, 256u> counts{};
        for (std::size_t index = begin; index < end; ++index)
            ++counts[combining_class(normalized[index])];
        std::array<std::size_t, 256u> positions{};
        std::size_t position = begin;
        for (std::size_t ccc = 1u; ccc < positions.size(); ++ccc) {
            positions[ccc] = position;
            position += counts[ccc];
        }
        for (std::size_t index = begin; index < end; ++index) {
            const auto ccc = combining_class(normalized[index]);
            scratch[positions[ccc]++] = normalized[index];
        }
        std::memcpy(normalized + begin, scratch + begin,
                    (end - begin) * sizeof(std::uint32_t));
    }

    if (normalized_size == 0u) return true;
    std::size_t written = 1u;
    std::size_t starter_index = 0u;
    std::uint32_t starter = normalized[0];
    bool has_starter = combining_class(normalized[0]) == 0u;
    std::uint8_t previous_class = 0u;
    for (std::size_t index = 1u; index < normalized_size; ++index) {
        const std::uint32_t code_point = normalized[index];
        const std::uint8_t current_class = combining_class(code_point);
        const std::uint32_t composite = has_starter &&
                (previous_class == 0u || previous_class < current_class)
            ? compose_pair(starter, code_point) : 0u;
        if (composite != 0u) {
            normalized[starter_index] = composite;
            starter = composite;
            continue;
        }
        if (current_class == 0u) {
            starter_index = written;
            starter = code_point;
            has_starter = true;
        }
        normalized[written++] = code_point;
        previous_class = current_class;
    }
    normalized_size = written;
    return true;
}

bool append_mapped(std::uint32_t code_point, std::uint32_t* output,
                   std::size_t capacity, std::size_t& size)
{
    const Uts46Row* row = mapping_row(code_point);
    if (row == nullptr) return false;
    if (row->status == 'V' || row->status == 'D')
        return append_code_point(code_point, output, capacity, size);
    if (row->status == 'M') {
        if (static_cast<std::size_t>(row->mapping_offset) +
                row->mapping_length > uts46_mappings.size() ||
            size > capacity || row->mapping_length > capacity - size)
            return false;
        for (std::size_t index = 0u; index < row->mapping_length; ++index)
            output[size++] = uts46_mappings[row->mapping_offset + index];
        return true;
    }
    return row->status == 'I';
}

bool is_mark(std::uint32_t code_point)
{
    return in_ranges(mark_ranges, code_point);
}

bool context_j_valid(const std::uint32_t* label, std::size_t size,
                     std::size_t position)
{
    const std::uint32_t code_point = label[position];
    if (code_point == 0x200Du)
        return position > 0u && combining_class(label[position - 1u]) == 9u;
    if (code_point != 0x200Cu) return false;
    if (position > 0u && combining_class(label[position - 1u]) == 9u)
        return true;

    bool left = false;
    for (std::size_t index = position; index > 0u;) {
        const char type = joining_type(label[--index]);
        if (type == 'T') continue;
        left = type == 'L' || type == 'D';
        break;
    }
    if (!left) return false;
    for (std::size_t index = position + 1u; index < size; ++index) {
        const char type = joining_type(label[index]);
        if (type == 'T') continue;
        return type == 'R' || type == 'D';
    }
    return false;
}

bool context_o_valid(const std::uint32_t* label, std::size_t size,
                     std::size_t position)
{
    const std::uint32_t cp = label[position];
    if (cp == 0x00B7u)
        return position > 0u && position + 1u < size &&
               label[position - 1u] == 'l' && label[position + 1u] == 'l';
    if (cp == 0x0375u)
        return position + 1u < size && in_ranges(script_greek, label[position + 1u]);
    if (cp == 0x05F3u || cp == 0x05F4u)
        return position > 0u && in_ranges(script_hebrew, label[position - 1u]);
    if (cp == 0x30FBu) {
        for (std::size_t index = 0u; index < size; ++index) {
            const auto candidate = label[index];
            if (candidate != cp && (in_ranges(script_han, candidate) ||
                                    in_ranges(script_hiragana, candidate) ||
                                    in_ranges(script_katakana, candidate)))
                return true;
        }
        return false;
    }
    if (cp >= 0x0660u && cp <= 0x0669u) {
        for (std::size_t index = 0u; index < size; ++index)
            if (label[index] >= 0x06F0u && label[index] <= 0x06F9u)
                return false;
        return true;
    }
    if (cp >= 0x06F0u && cp <= 0x06F9u) {
        for (std::size_t index = 0u; index < size; ++index)
            if (label[index] >= 0x0660u && label[index] <= 0x0669u)
                return false;
        return true;
    }
    return false;
}

bool bidi_valid(const std::uint32_t* label, std::size_t size)
{
    bool rtl = false;
    for (std::size_t index = 0u; index < size; ++index) {
        const auto value = bidi_class(label[index]);
        if (value == 2u || value == 3u || value == 5u) {
            rtl = true;
            break;
        }
    }
    if (!rtl) return true;

    const auto first = bidi_class(label[0]);
    const bool right_to_left = first == 2u || first == 3u;
    if (!right_to_left && first != 1u) return false;
    bool valid_ending = false;
    std::uint8_t number_type = 0u;
    for (std::size_t index = 0u; index < size; ++index) {
        const auto value = bidi_class(label[index]);
        if (right_to_left) {
            if (value != 2u && value != 3u && value != 5u && value != 4u &&
                value != 6u && value != 7u && value != 8u && value != 9u &&
                value != 10u && value != 11u)
                return false;
            if (value == 2u || value == 3u || value == 4u || value == 5u)
                valid_ending = true;
            else if (value != 11u)
                valid_ending = false;
            if (value == 4u || value == 5u) {
                if (number_type != 0u && number_type != value) return false;
                number_type = value;
            }
        } else {
            if (value != 1u && value != 4u && value != 6u && value != 7u &&
                value != 8u && value != 9u && value != 10u && value != 11u)
                return false;
            if (value == 1u || value == 4u)
                valid_ending = true;
            else if (value != 11u)
                valid_ending = false;
        }
    }
    return valid_ending;
}

bool valid_label(const std::uint32_t* label, std::size_t size)
{
    if (size == 0u || is_mark(label[0]) || label[0] == '-' ||
        label[size - 1u] == '-' ||
        (size >= 4u && label[2] == '-' && label[3] == '-'))
        return false;
    for (std::size_t index = 0u; index < size; ++index) {
        const std::uint32_t cp = label[index];
        if (cp == '.') return false;
        if (in_ranges(contextj, cp)) {
            if (!context_j_valid(label, size, index)) return false;
            continue;
        }
        if (in_ranges(contexto, cp)) {
            if (!context_o_valid(label, size, index)) return false;
            continue;
        }
        const Uts46Row* row = mapping_row(cp);
        if (row == nullptr || (row->status != 'V' && row->status != 'D'))
            return false;
    }
    return bidi_valid(label, size);
}

bool ascii_ldh(char value)
{
    return (value >= 'a' && value <= 'z') ||
           (value >= '0' && value <= '9') || value == '-';
}

int punycode_digit(char value)
{
    if (value >= 'a' && value <= 'z') return value - 'a';
    if (value >= 'A' && value <= 'Z') return value - 'A';
    if (value >= '0' && value <= '9') return value - '0' + 26;
    return -1;
}

char encode_digit(std::uint32_t value)
{
    return value < 26u ? static_cast<char>('a' + value)
                       : static_cast<char>('0' + value - 26u);
}

std::uint32_t adapt_bias(std::uint64_t delta, std::uint64_t points,
                         bool first)
{
    if (points == 0u) return UINT32_MAX;
    delta = first ? delta / 700u : delta / 2u;
    if (delta > UINT64_MAX - delta / points) return UINT32_MAX;
    delta += delta / points;
    std::uint64_t k = 0u;
    while (delta > 455u) {
        delta /= 35u;
        k += 36u;
    }
    const std::uint64_t result = k + (36u * delta) / (delta + 38u);
    return result > UINT32_MAX ? UINT32_MAX : static_cast<std::uint32_t>(result);
}

std::uint32_t puny_threshold(std::uint32_t k, std::uint32_t bias)
{
    return k <= bias + 1u ? 1u : k >= bias + 26u ? 26u : k - bias;
}

bool punycode_decode(const char* input, std::size_t input_size,
                     std::uint32_t* output, std::size_t capacity,
                     std::size_t& output_size)
{
    output_size = 0u;
    std::size_t delimiter = input_size;
    for (std::size_t index = 0u; index < input_size; ++index)
        if (input[index] == '-') delimiter = index;
    std::size_t cursor = 0u;
    if (delimiter != input_size) {
        for (; cursor < delimiter; ++cursor) {
            const auto cp = static_cast<unsigned char>(input[cursor]);
            if (cp >= 0x80u || !ascii_ldh(static_cast<char>(cp)) ||
                !append_code_point(cp, output, capacity, output_size))
                return false;
        }
        cursor = delimiter + 1u;
    }
    std::uint32_t code_point = 128u;
    std::uint32_t bias = 72u;
    std::uint64_t index_value = 0u;
    while (cursor < input_size) {
        const std::uint64_t old_index = index_value;
        std::uint64_t weight = 1u;
        std::uint32_t k = 36u;
        for (;;) {
            if (cursor >= input_size) return false;
            const int digit = punycode_digit(input[cursor++]);
            if (digit < 0 || static_cast<std::uint64_t>(digit) >
                    (UINT64_MAX - index_value) / weight)
                return false;
            index_value += static_cast<std::uint64_t>(digit) * weight;
            const std::uint32_t threshold = puny_threshold(k, bias);
            if (static_cast<std::uint32_t>(digit) < threshold) break;
            if (weight > UINT64_MAX / (36u - threshold) ||
                k > UINT32_MAX - 36u)
                return false;
            weight *= 36u - threshold;
            k += 36u;
        }
        if (output_size >= capacity ||
            index_value / (output_size + 1u) > UINT32_MAX - code_point)
            return false;
        code_point += static_cast<std::uint32_t>(
            index_value / (output_size + 1u));
        const std::uint64_t delta = index_value - old_index;
        index_value %= output_size + 1u;
        if (code_point >= kUnicodeLimit ||
            (code_point >= 0xD800u && code_point <= 0xDFFFu))
            return false;
        for (std::size_t move = output_size; move > index_value; --move)
            output[move] = output[move - 1u];
        output[index_value] = code_point;
        ++output_size;
        ++index_value;
        bias = adapt_bias(delta, output_size, old_index == 0u);
        if (bias == UINT32_MAX) return false;
    }
    return output_size != 0u;
}

bool append_ascii(char value, char* output, std::size_t capacity,
                  std::size_t& size)
{
    if (size >= capacity) return false;
    output[size++] = value;
    return true;
}

bool punycode_encode(const std::uint32_t* input, std::size_t input_size,
                     char* output, std::size_t capacity,
                     std::size_t& output_size)
{
    output_size = 0u;
    std::size_t basic = 0u;
    for (std::size_t index = 0u; index < input_size; ++index) {
        if (input[index] >= 0x80u) continue;
        if (!ascii_ldh(static_cast<char>(input[index])) ||
            !append_ascii(static_cast<char>(input[index]), output, capacity,
                          output_size))
            return false;
        ++basic;
    }
    std::size_t handled = basic;
    const bool has_non_ascii = handled != input_size;
    if (basic != 0u && has_non_ascii &&
        !append_ascii('-', output, capacity, output_size))
        return false;
    std::uint32_t code_point = 128u;
    std::uint32_t bias = 72u;
    std::uint64_t delta = 0u;
    while (handled < input_size) {
        std::uint32_t minimum = UINT32_MAX;
        for (std::size_t index = 0u; index < input_size; ++index)
            if (input[index] >= code_point && input[index] < minimum)
                minimum = input[index];
        if (minimum == UINT32_MAX || minimum < code_point ||
            static_cast<std::uint64_t>(minimum - code_point) >
                (UINT64_MAX - delta) / (handled + 1u))
            return false;
        delta += static_cast<std::uint64_t>(minimum - code_point) *
                 (handled + 1u);
        code_point = minimum;
        for (std::size_t index = 0u; index < input_size; ++index) {
            if (input[index] < code_point) {
                if (delta == UINT64_MAX) return false;
                ++delta;
            }
            if (input[index] != code_point) continue;
            std::uint64_t quotient = delta;
            std::uint32_t k = 36u;
            for (;;) {
                const std::uint32_t threshold = puny_threshold(k, bias);
                if (quotient < threshold) break;
                const std::uint32_t digit = threshold +
                    static_cast<std::uint32_t>(
                        (quotient - threshold) % (36u - threshold));
                if (!append_ascii(encode_digit(digit), output, capacity,
                                  output_size))
                    return false;
                quotient = (quotient - threshold) / (36u - threshold);
                if (k > UINT32_MAX - 36u) return false;
                k += 36u;
            }
            if (quotient >= 36u ||
                !append_ascii(encode_digit(static_cast<std::uint32_t>(quotient)),
                              output, capacity, output_size))
                return false;
            bias = adapt_bias(delta, handled + 1u, handled == basic);
            if (bias == UINT32_MAX) return false;
            delta = 0u;
            ++handled;
        }
        if (delta == UINT64_MAX || code_point == UINT32_MAX) return false;
        ++delta;
        ++code_point;
    }
    return true;
}

bool same_ascii_label(const char* original, std::size_t original_size,
                      const std::uint32_t* normalized,
                      std::size_t normalized_size)
{
    char encoded[64] = {};
    std::size_t encoded_size = 0u;
    if (normalized_size == 0u) return false;
    encoded[encoded_size++] = 'x';
    encoded[encoded_size++] = 'n';
    encoded[encoded_size++] = '-';
    encoded[encoded_size++] = '-';
    std::size_t payload_size = 0u;
    if (!punycode_encode(normalized, normalized_size, encoded + encoded_size,
                         sizeof(encoded) - encoded_size, payload_size))
        return false;
    encoded_size += payload_size;
    return encoded_size == original_size &&
           std::memcmp(encoded, original, original_size) == 0;
}

} // namespace

extern "C" size_t rin_unicode_idna_workspace_size(void)
{
    return kWorkspaceCodePoints * kWorkspaceArrays * sizeof(std::uint32_t);
}

extern "C" int rin_unicode_idna_to_ascii(
    const char* input, size_t input_length, char* output,
    size_t output_capacity, size_t* output_length, void* workspace,
    size_t workspace_size)
{
    if (output_length != nullptr) *output_length = 0u;
    if (output != nullptr && output_capacity != 0u) output[0] = '\0';
    if (input == nullptr || input_length == 0u ||
        input_length > RIN_UNICODE_IDNA_MAX_DOMAIN_BYTES || output == nullptr ||
        output_capacity == 0u || output_length == nullptr || workspace == nullptr ||
        workspace_size < rin_unicode_idna_workspace_size() ||
        (reinterpret_cast<std::uintptr_t>(workspace) % alignof(std::uint32_t)) != 0u)
        return RIN_UNICODE_IDNA_INVALID;

    auto* mapped = static_cast<std::uint32_t*>(workspace);
    auto* label = mapped + kWorkspaceCodePoints;
    auto* normalized = label + kWorkspaceCodePoints;
    std::size_t mapped_size = 0u;
    for (std::size_t offset = 0u; offset < input_length;) {
        std::uint32_t code_point = 0u;
        std::size_t consumed = 0u;
        if (rin_unicode_decode_utf8(input + offset, input_length - offset,
                                    &code_point, &consumed) != RIN_UNICODE_OK ||
            consumed == 0u || !append_mapped(code_point, mapped,
                                             kWorkspaceCodePoints, mapped_size))
            return RIN_UNICODE_IDNA_INVALID;
        offset += consumed;
    }
    if (mapped_size == 0u) return RIN_UNICODE_IDNA_INVALID;

    char candidate[RIN_UNICODE_IDNA_MAX_ASCII_BYTES + 1u] = {};
    std::size_t candidate_size = 0u;
    std::size_t label_start = 0u;
    for (std::size_t index = 0u; index <= mapped_size; ++index) {
        if (index != mapped_size && mapped[index] != '.') continue;
        const std::size_t source_size = index - label_start;
        if (source_size == 0u) return RIN_UNICODE_IDNA_INVALID;

        bool source_ascii = true;
        for (std::size_t cp_index = 0u; cp_index < source_size; ++cp_index)
            source_ascii &= mapped[label_start + cp_index] < 0x80u;
        const bool alabel = source_ascii && source_size >= 4u &&
            mapped[label_start] == 'x' && mapped[label_start + 1u] == 'n' &&
            mapped[label_start + 2u] == '-' && mapped[label_start + 3u] == '-';
        std::size_t normalized_size = 0u;
        if (alabel) {
            if (source_size > kMaxPunycodeLabel) return RIN_UNICODE_IDNA_INVALID;
            char source_label[kMaxPunycodeLabel + 1u] = {};
            for (std::size_t cp_index = 0u; cp_index < source_size; ++cp_index)
                source_label[cp_index] = static_cast<char>(mapped[label_start + cp_index]);
            if (!punycode_decode(source_label + 4u, source_size - 4u, label,
                                 kWorkspaceCodePoints, normalized_size))
                return RIN_UNICODE_IDNA_INVALID;
            bool has_non_ascii = false;
            for (std::size_t cp_index = 0u; cp_index < normalized_size; ++cp_index)
                has_non_ascii |= label[cp_index] >= 0x80u;
            if (!has_non_ascii || !normalize_nfc(label, normalized_size,
                    normalized, label, kWorkspaceCodePoints, normalized_size) ||
                normalized_size > kMaxPunycodeLabel - 4u ||
                !valid_label(normalized, normalized_size) ||
                !same_ascii_label(source_label, source_size, normalized,
                                  normalized_size))
                return RIN_UNICODE_IDNA_INVALID;
        } else {
            if (source_size > kWorkspaceCodePoints) return RIN_UNICODE_IDNA_INVALID;
            for (std::size_t cp_index = 0u; cp_index < source_size; ++cp_index)
                label[cp_index] = mapped[label_start + cp_index];
            if (!normalize_nfc(label, source_size, normalized, label,
                               kWorkspaceCodePoints, normalized_size))
                return RIN_UNICODE_IDNA_INVALID;
            bool normalized_ascii = true;
            for (std::size_t cp_index = 0u; cp_index < normalized_size; ++cp_index)
                normalized_ascii &= normalized[cp_index] < 0x80u;
            if ((!normalized_ascii && normalized_size > kMaxPunycodeLabel - 4u) ||
                !valid_label(normalized, normalized_size))
                return RIN_UNICODE_IDNA_INVALID;
        }

        char ascii_label[kMaxPunycodeLabel + 1u] = {};
        std::size_t ascii_size = 0u;
        if (alabel) {
            char source_label[kMaxPunycodeLabel + 1u] = {};
            for (std::size_t cp_index = 0u; cp_index < source_size; ++cp_index)
                source_label[cp_index] = static_cast<char>(mapped[label_start + cp_index]);
            std::memcpy(ascii_label, source_label, source_size);
            ascii_size = source_size;
        } else {
            bool all_ascii = true;
            for (std::size_t cp_index = 0u; cp_index < normalized_size; ++cp_index)
                all_ascii &= normalized[cp_index] < 0x80u;
            if (all_ascii) {
                if (normalized_size > sizeof(ascii_label))
                    return RIN_UNICODE_IDNA_INVALID;
                for (std::size_t cp_index = 0u; cp_index < normalized_size; ++cp_index)
                    ascii_label[ascii_size++] = static_cast<char>(normalized[cp_index]);
            } else {
                ascii_label[ascii_size++] = 'x';
                ascii_label[ascii_size++] = 'n';
                ascii_label[ascii_size++] = '-';
                ascii_label[ascii_size++] = '-';
                std::size_t payload_size = 0u;
                if (!punycode_encode(normalized, normalized_size,
                                     ascii_label + ascii_size,
                                     sizeof(ascii_label) - ascii_size,
                                     payload_size))
                    return RIN_UNICODE_IDNA_INVALID;
                ascii_size += payload_size;
            }
        }
        if (ascii_size == 0u || ascii_size > 63u ||
            ascii_label[0] == '-' || ascii_label[ascii_size - 1u] == '-')
            return RIN_UNICODE_IDNA_INVALID;
        if (candidate_size != 0u) {
            if (candidate_size >= RIN_UNICODE_IDNA_MAX_ASCII_BYTES)
                return RIN_UNICODE_IDNA_INVALID;
            candidate[candidate_size++] = '.';
        }
        if (ascii_size > RIN_UNICODE_IDNA_MAX_ASCII_BYTES - candidate_size)
            return RIN_UNICODE_IDNA_INVALID;
        std::memcpy(candidate + candidate_size, ascii_label, ascii_size);
        candidate_size += ascii_size;
        label_start = index + 1u;
    }
    if (candidate_size == 0u || candidate_size > RIN_UNICODE_IDNA_MAX_ASCII_BYTES)
        return RIN_UNICODE_IDNA_INVALID;
    if (output_capacity <= candidate_size) return RIN_UNICODE_IDNA_NO_SPACE;
    std::memcpy(output, candidate, candidate_size);
    output[candidate_size] = '\0';
    *output_length = candidate_size;
    return RIN_UNICODE_IDNA_OK;
}
