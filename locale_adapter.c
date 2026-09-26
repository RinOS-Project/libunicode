#include "rin_unicode.h"
#include "../libc/locale.h"
#include <generated_locale_catalog.h>
#include <stdlib.h>

#define LC_CTYPE 0
#define LC_NUMERIC 1
#define LC_TIME 2
#define LC_COLLATE 3
#define LC_MONETARY 4
#define LC_MESSAGES 5
#define LC_ALL 6
#define LC_MAX 7

typedef RinUnicodeGeneratedLocale RinUnicodeLocale;

_Static_assert(sizeof(g_rin_unicode_generated_locales) /
                   sizeof(g_rin_unicode_generated_locales[0]) ==
                       RIN_UNICODE_GENERATED_LOCALE_COUNT,
               "RinOS product locale snapshot count disagrees with generated metadata");

typedef struct RinUnicodeParsedLocale {
    char language[16];
    char script[16];
    char region[16];
} RinUnicodeParsedLocale;

typedef enum RinUnicodeDigitSet {
    RIN_UNICODE_DIGITS_LATIN = 0,
    RIN_UNICODE_DIGITS_ARABIC_INDIC,
    RIN_UNICODE_DIGITS_EXTENDED_ARABIC_INDIC,
    RIN_UNICODE_DIGITS_DEVANAGARI,
    RIN_UNICODE_DIGITS_THAI,
    RIN_UNICODE_DIGITS_BENGALI,
    RIN_UNICODE_DIGITS_GUJARATI,
    RIN_UNICODE_DIGITS_GURMUKHI,
    RIN_UNICODE_DIGITS_KANNADA,
    RIN_UNICODE_DIGITS_KHMER,
    RIN_UNICODE_DIGITS_LAO,
    RIN_UNICODE_DIGITS_MALAYALAM,
    RIN_UNICODE_DIGITS_MYANMAR,
    RIN_UNICODE_DIGITS_ORIYA,
    RIN_UNICODE_DIGITS_TELUGU,
    RIN_UNICODE_DIGITS_TAMIL,
    RIN_UNICODE_DIGITS_TIBETAN,
    RIN_UNICODE_DIGITS_FULLWIDTH,
    RIN_UNICODE_DIGITS_HANIDEC
} RinUnicodeDigitSet;

static RinUnicodeLocale const* g_locale_root = &g_rin_unicode_generated_locales[0];
static RinUnicodeLocale const* g_current_locale[LC_MAX] = {
    &g_rin_unicode_generated_locales[0],
    &g_rin_unicode_generated_locales[0],
    &g_rin_unicode_generated_locales[0],
    &g_rin_unicode_generated_locales[0],
    &g_rin_unicode_generated_locales[0],
    &g_rin_unicode_generated_locales[0],
    &g_rin_unicode_generated_locales[0],
};
static RinUnicodeDigitSet g_current_digit_set[LC_MAX] = {
    RIN_UNICODE_DIGITS_LATIN,
    RIN_UNICODE_DIGITS_LATIN,
    RIN_UNICODE_DIGITS_LATIN,
    RIN_UNICODE_DIGITS_LATIN,
    RIN_UNICODE_DIGITS_LATIN,
    RIN_UNICODE_DIGITS_LATIN,
    RIN_UNICODE_DIGITS_LATIN,
};

/* The locale catalog is immutable, but the selected category pointers are
 * process-global state.  Publish a complete category update under one small
 * lock so readers never observe a partially applied LC_ALL change.  The
 * lock is deliberately allocation-free and does not depend on hosted pthread
 * APIs; it is therefore usable by both the freestanding and hosted libc
 * adapters. */
static volatile unsigned g_locale_state_lock;

static void rin_unicode_locale_lock(void)
{
    while (__sync_lock_test_and_set(&g_locale_state_lock, 1u) != 0u) {
#if defined(__i386__) || defined(__x86_64__)
        __asm__ volatile("pause");
#endif
    }
}

static void rin_unicode_locale_unlock(void)
{
    __sync_lock_release(&g_locale_state_lock);
}

static int rin_unicode_ascii_ieq(char const* lhs, char const* rhs)
{
    size_t i = 0u;
    if (!lhs || !rhs) return 0;
    while (lhs[i] != '\0' && rhs[i] != '\0') {
        char a = lhs[i];
        char b = rhs[i];
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
        if (a != b) return 0;
        ++i;
    }
    return lhs[i] == '\0' && rhs[i] == '\0';
}

static int rin_unicode_locale_name_length(char const* name, size_t* length_out)
{
    size_t length;
    if (!name || !length_out) return 0;
    for (length = 0u; length < RIN_UNICODE_MAX_LOCALE_NAME_BYTES; ++length) {
        if (name[length] == '\0') {
            *length_out = length;
            return 1;
        }
    }
    return 0;
}

static int rin_unicode_ascii_all_alpha(char const* text)
{
    size_t i = 0u;
    if (!text || text[0] == '\0') return 0;
    while (text[i] != '\0') {
        char ch = text[i];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))) return 0;
        ++i;
    }
    return 1;
}

static int rin_unicode_ascii_all_digit(char const* text)
{
    size_t i = 0u;
    if (!text || text[0] == '\0') return 0;
    while (text[i] != '\0') {
        char ch = text[i];
        if (ch < '0' || ch > '9') return 0;
        ++i;
    }
    return 1;
}

static void rin_unicode_ascii_copy_lower(char* dest, size_t cap, char const* src)
{
    size_t i = 0u;
    if (!dest || cap == 0u) return;
    while (src && src[i] != '\0' && i + 1u < cap) {
        char ch = src[i];
        if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
        dest[i] = ch;
        ++i;
    }
    dest[i] = '\0';
}

static void rin_unicode_ascii_copy_upper(char* dest, size_t cap, char const* src)
{
    size_t i = 0u;
    if (!dest || cap == 0u) return;
    while (src && src[i] != '\0' && i + 1u < cap) {
        char ch = src[i];
        if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
        dest[i] = ch;
        ++i;
    }
    dest[i] = '\0';
}

static void rin_unicode_ascii_copy_title(char* dest, size_t cap, char const* src)
{
    size_t i = 0u;
    if (!dest || cap == 0u) return;
    while (src && src[i] != '\0' && i + 1u < cap) {
        char ch = src[i];
        if (i == 0u) {
            if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
        } else if (ch >= 'A' && ch <= 'Z') {
            ch = (char)(ch - 'A' + 'a');
        }
        dest[i] = ch;
        ++i;
    }
    dest[i] = '\0';
}

static int rin_unicode_ascii_token_ieq(char const* text, size_t length,
                                       char const* expected)
{
    size_t index = 0u;
    if (!text || !expected) return 0;
    while (expected[index] != '\0') {
        char actual;
        char wanted = expected[index];
        if (index >= length) return 0;
        actual = text[index];
        if (actual >= 'A' && actual <= 'Z')
            actual = (char)(actual - 'A' + 'a');
        if (wanted >= 'A' && wanted <= 'Z')
            wanted = (char)(wanted - 'A' + 'a');
        if (actual != wanted) return 0;
        ++index;
    }
    return index == length;
}

static int rin_unicode_ascii_token_alnum(char const* text, size_t length)
{
    size_t index;
    if (!text || length == 0u) return 0;
    for (index = 0u; index < length; ++index) {
        char ch = text[index];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
              (ch >= '0' && ch <= '9')))
            return 0;
    }
    return 1;
}

static int rin_unicode_digit_set_from_token(char const* token, size_t length,
                                            RinUnicodeDigitSet* digit_set)
{
    if (!token || !digit_set || length < 3u || length > 8u ||
        !rin_unicode_ascii_token_alnum(token, length))
        return 0;
    if (rin_unicode_ascii_token_ieq(token, length, "latn"))
        *digit_set = RIN_UNICODE_DIGITS_LATIN;
    else if (rin_unicode_ascii_token_ieq(token, length, "arab"))
        *digit_set = RIN_UNICODE_DIGITS_ARABIC_INDIC;
    else if (rin_unicode_ascii_token_ieq(token, length, "arabext"))
        *digit_set = RIN_UNICODE_DIGITS_EXTENDED_ARABIC_INDIC;
    else if (rin_unicode_ascii_token_ieq(token, length, "deva"))
        *digit_set = RIN_UNICODE_DIGITS_DEVANAGARI;
    else if (rin_unicode_ascii_token_ieq(token, length, "thai"))
        *digit_set = RIN_UNICODE_DIGITS_THAI;
    else if (rin_unicode_ascii_token_ieq(token, length, "beng"))
        *digit_set = RIN_UNICODE_DIGITS_BENGALI;
    else if (rin_unicode_ascii_token_ieq(token, length, "gujr"))
        *digit_set = RIN_UNICODE_DIGITS_GUJARATI;
    else if (rin_unicode_ascii_token_ieq(token, length, "guru"))
        *digit_set = RIN_UNICODE_DIGITS_GURMUKHI;
    else if (rin_unicode_ascii_token_ieq(token, length, "knda"))
        *digit_set = RIN_UNICODE_DIGITS_KANNADA;
    else if (rin_unicode_ascii_token_ieq(token, length, "khmr"))
        *digit_set = RIN_UNICODE_DIGITS_KHMER;
    else if (rin_unicode_ascii_token_ieq(token, length, "laoo"))
        *digit_set = RIN_UNICODE_DIGITS_LAO;
    else if (rin_unicode_ascii_token_ieq(token, length, "mlym"))
        *digit_set = RIN_UNICODE_DIGITS_MALAYALAM;
    else if (rin_unicode_ascii_token_ieq(token, length, "mymr"))
        *digit_set = RIN_UNICODE_DIGITS_MYANMAR;
    else if (rin_unicode_ascii_token_ieq(token, length, "orya"))
        *digit_set = RIN_UNICODE_DIGITS_ORIYA;
    else if (rin_unicode_ascii_token_ieq(token, length, "telu"))
        *digit_set = RIN_UNICODE_DIGITS_TELUGU;
    else if (rin_unicode_ascii_token_ieq(token, length, "tamldec"))
        *digit_set = RIN_UNICODE_DIGITS_TAMIL;
    else if (rin_unicode_ascii_token_ieq(token, length, "tibt"))
        *digit_set = RIN_UNICODE_DIGITS_TIBETAN;
    else if (rin_unicode_ascii_token_ieq(token, length, "fullwide"))
        *digit_set = RIN_UNICODE_DIGITS_FULLWIDTH;
    else if (rin_unicode_ascii_token_ieq(token, length, "hanidec"))
        *digit_set = RIN_UNICODE_DIGITS_HANIDEC;
    else
        return 0;
    return 1;
}

/* Parse only the supported Unicode locale-extension subset.  The base
 * locale parser intentionally remains product-catalog based, but an
 * explicit -u-nu-* request must not be silently ignored: unsupported or
 * malformed numbering systems fail the locale selection. */
static int rin_unicode_parse_numbering_system(char const* name,
                                              RinUnicodeDigitSet* digit_set,
                                              int* has_override)
{
    size_t length;
    size_t position = 0u;
    int in_unicode_extension = 0;
    int expecting_numbering_value = 0;
    int saw_numbering_key = 0;
    if (!digit_set || !has_override) return 0;
    *digit_set = RIN_UNICODE_DIGITS_LATIN;
    *has_override = 0;
    if (!name) return 1;
    if (!rin_unicode_locale_name_length(name, &length)) return 0;
    while (position < length &&
           (name[position] == ' ' || name[position] == '\t'))
        ++position;
    while (position < length && name[position] != '.' && name[position] != '@') {
        size_t token_start = position;
        size_t token_length;
        while (position < length && name[position] != '.' &&
               name[position] != '@' && name[position] != '-' &&
               name[position] != '_')
            ++position;
        token_length = position - token_start;
        if (token_length == 0u) {
            ++position;
            continue;
        }
        if (!in_unicode_extension) {
            if (token_length == 1u &&
                rin_unicode_ascii_token_ieq(name + token_start, token_length,
                                             "u"))
                in_unicode_extension = 1;
        } else if (expecting_numbering_value) {
            if (!rin_unicode_digit_set_from_token(name + token_start,
                                                   token_length, digit_set))
                return 0;
            *has_override = 1;
            expecting_numbering_value = 0;
        } else if (token_length == 2u &&
                   rin_unicode_ascii_token_ieq(name + token_start,
                                               token_length, "nu")) {
            if (*has_override || saw_numbering_key) return 0;
            saw_numbering_key = 1;
            expecting_numbering_value = 1;
        } else if (token_length == 1u) {
            /* A new singleton starts the next extension. */
            in_unicode_extension = 0;
        }
        if (position < length && (name[position] == '-' ||
                                  name[position] == '_'))
            ++position;
    }
    return !expecting_numbering_value;
}

static void rin_unicode_parse_locale_name(char const* name, RinUnicodeParsedLocale* parsed)
{
    size_t index = 0u;
    char token[16];
    int field_index = 0;
    if (!parsed) return;
    parsed->language[0] = '\0';
    parsed->script[0] = '\0';
    parsed->region[0] = '\0';
    if (!name || name[0] == '\0') return;
    if (rin_unicode_ascii_ieq(name, "C") ||
        rin_unicode_ascii_ieq(name, "POSIX") ||
        rin_unicode_ascii_ieq(name, "root")) {
        return;
    }

    while (*name == ' ' || *name == '\t') ++name;
    while (*name != '\0' && *name != '.' && *name != '@') {
        char ch = *name++;
        if (ch == '-' || ch == '_') {
            if (index == 0u) continue;
            token[index] = '\0';
            if (field_index == 0) {
                rin_unicode_ascii_copy_lower(parsed->language, sizeof(parsed->language), token);
            } else if (parsed->script[0] == '\0' && index == 4u && rin_unicode_ascii_all_alpha(token)) {
                rin_unicode_ascii_copy_title(parsed->script, sizeof(parsed->script), token);
            } else if (parsed->region[0] == '\0' &&
                       ((index == 2u && rin_unicode_ascii_all_alpha(token)) ||
                        (index == 3u && rin_unicode_ascii_all_digit(token)))) {
                rin_unicode_ascii_copy_upper(parsed->region, sizeof(parsed->region), token);
            }
            ++field_index;
            index = 0u;
            continue;
        }
        if (index + 1u < sizeof(token)) token[index++] = ch;
    }

    if (index != 0u) {
        token[index] = '\0';
        if (field_index == 0) {
            rin_unicode_ascii_copy_lower(parsed->language, sizeof(parsed->language), token);
        } else if (parsed->script[0] == '\0' && index == 4u && rin_unicode_ascii_all_alpha(token)) {
            rin_unicode_ascii_copy_title(parsed->script, sizeof(parsed->script), token);
        } else if (parsed->region[0] == '\0' &&
                   ((index == 2u && rin_unicode_ascii_all_alpha(token)) ||
                    (index == 3u && rin_unicode_ascii_all_digit(token)))) {
            rin_unicode_ascii_copy_upper(parsed->region, sizeof(parsed->region), token);
        }
    }
}

static RinUnicodeLocale const* rin_unicode_find_locale_by_parts(char const* language, char const* script, char const* region)
{
    RinUnicodeLocale const* language_match = (RinUnicodeLocale const*)0;
    RinUnicodeLocale const* script_match = (RinUnicodeLocale const*)0;
    size_t i;
    if (!language || language[0] == '\0') return g_locale_root;
    for (i = 0u; i < g_rin_unicode_generated_locale_count; ++i) {
        RinUnicodeLocale const* locale = &g_rin_unicode_generated_locales[i];
        if (locale->language[0] == '\0' || !rin_unicode_ascii_ieq(locale->language, language)) continue;
        if (region && region[0] != '\0' && rin_unicode_ascii_ieq(locale->territory, region)) {
            if (!script || script[0] == '\0' || rin_unicode_ascii_ieq(locale->script, script)) return locale;
        }
        if ((!region || region[0] == '\0') &&
            script && script[0] != '\0' &&
            rin_unicode_ascii_ieq(locale->script, script) &&
            script_match == (RinUnicodeLocale const*)0) {
            script_match = locale;
        }
        if (language_match == (RinUnicodeLocale const*)0) language_match = locale;
    }
    if (script_match) return script_match;
    if (language_match) return language_match;
    return (RinUnicodeLocale const*)0;
}

static RinUnicodeLocale const* rin_unicode_find_locale_base(char const* name)
{
    RinUnicodeParsedLocale parsed;
    size_t ignored_length;
    if (!name) return g_locale_root;
    if (!rin_unicode_locale_name_length(name, &ignored_length))
        return (RinUnicodeLocale const*)0;
    if (name[0] == '\0' ||
        rin_unicode_ascii_ieq(name, "C") ||
        rin_unicode_ascii_ieq(name, "POSIX") ||
        rin_unicode_ascii_ieq(name, "root")) {
        return g_locale_root;
    }
    rin_unicode_parse_locale_name(name, &parsed);
    if (parsed.language[0] == '\0') return g_locale_root;
    return rin_unicode_find_locale_by_parts(parsed.language, parsed.script, parsed.region);
}

static RinUnicodeDigitSet rin_unicode_default_digit_set(
    RinUnicodeLocale const* selected)
{
    if (!selected) return RIN_UNICODE_DIGITS_LATIN;
    if (rin_unicode_ascii_ieq(selected->language, "ar"))
        return RIN_UNICODE_DIGITS_ARABIC_INDIC;
    if (rin_unicode_ascii_ieq(selected->language, "fa"))
        return RIN_UNICODE_DIGITS_EXTENDED_ARABIC_INDIC;
    if (rin_unicode_ascii_ieq(selected->language, "hi"))
        return RIN_UNICODE_DIGITS_DEVANAGARI;
    if (rin_unicode_ascii_ieq(selected->language, "th"))
        return RIN_UNICODE_DIGITS_THAI;
    return RIN_UNICODE_DIGITS_LATIN;
}

static int rin_unicode_find_locale_selection(char const* name,
                                             RinUnicodeLocale const** selected,
                                             RinUnicodeDigitSet* digit_set)
{
    int has_override;
    if (!selected || !digit_set ||
        !rin_unicode_parse_numbering_system(name, digit_set, &has_override))
        return 0;
    *selected = rin_unicode_find_locale_base(name);
    if (!*selected) return 0;
    if (!has_override) *digit_set = rin_unicode_default_digit_set(*selected);
    return 1;
}

static RinUnicodeLocale const* rin_unicode_find_locale(char const* name)
{
    RinUnicodeLocale const* selected;
    RinUnicodeDigitSet ignored_digit_set;
    if (!rin_unicode_find_locale_selection(name, &selected, &ignored_digit_set))
        return (RinUnicodeLocale const*)0;
    return selected;
}

static char const* rin_unicode_locale_environment_name(int category)
{
    switch (category) {
    case LC_CTYPE: return "LC_CTYPE";
    case LC_NUMERIC: return "LC_NUMERIC";
    case LC_TIME: return "LC_TIME";
    case LC_COLLATE: return "LC_COLLATE";
    case LC_MONETARY: return "LC_MONETARY";
    case LC_MESSAGES: return "LC_MESSAGES";
    default: return (char const*)0;
    }
}

static RinUnicodeLocale const* rin_unicode_locale_from_environment(
    int category, RinUnicodeDigitSet* digit_set_out)
{
    char const* name = getenv("LC_ALL");
    char const* category_name;
    RinUnicodeLocale const* selected;
    if (name && name[0] != '\0') {
        if (rin_unicode_find_locale_selection(name, &selected, digit_set_out))
            return selected;
        if (digit_set_out) *digit_set_out = RIN_UNICODE_DIGITS_LATIN;
        return g_locale_root;
    }
    category_name = rin_unicode_locale_environment_name(category);
    name = category_name ? getenv(category_name) : (char const*)0;
    if (!name || name[0] == '\0') name = getenv("LANG");
    if (!name || name[0] == '\0') {
        if (digit_set_out) *digit_set_out = RIN_UNICODE_DIGITS_LATIN;
        return g_locale_root;
    }
    if (rin_unicode_find_locale_selection(name, &selected, digit_set_out))
        return selected;
    if (digit_set_out) *digit_set_out = RIN_UNICODE_DIGITS_LATIN;
    return g_locale_root;
}

char* rin_unicode_setlocale(int category, char const* locale)
{
    int i;
    RinUnicodeLocale const* selected;
    RinUnicodeDigitSet digit_set;
    if (category < 0 || category >= LC_MAX) return (char*)0;
    rin_unicode_locale_lock();
    if (!locale) {
        char* result = (char*)g_current_locale[category]->name;
        rin_unicode_locale_unlock();
        return result;
    }
    if (locale[0] == '\0') {
        if (category == LC_ALL) {
            for (i = 0; i < LC_ALL; ++i)
                g_current_locale[i] = rin_unicode_locale_from_environment(
                    i, &g_current_digit_set[i]);
            g_current_locale[LC_ALL] = g_current_locale[LC_MESSAGES];
            g_current_digit_set[LC_ALL] = g_current_digit_set[LC_MESSAGES];
            selected = g_current_locale[LC_MESSAGES];
            rin_unicode_locale_unlock();
            return (char*)selected->name;
        }
        selected = rin_unicode_locale_from_environment(category, &digit_set);
        g_current_locale[category] = selected;
        g_current_digit_set[category] = digit_set;
        rin_unicode_locale_unlock();
        return (char*)selected->name;
    }
    if (!rin_unicode_find_locale_selection(locale, &selected, &digit_set)) {
        rin_unicode_locale_unlock();
        return (char*)0;
    }
    if (category == LC_ALL) {
        for (i = 0; i < LC_MAX; ++i) {
            g_current_locale[i] = selected;
            g_current_digit_set[i] = digit_set;
        }
    } else {
        g_current_locale[category] = selected;
        g_current_digit_set[category] = digit_set;
    }
    rin_unicode_locale_unlock();
    return (char*)selected->name;
}

rin_unicode_lconv_t* rin_unicode_localeconv(void)
{
    RinUnicodeLocale const* selected;
    rin_unicode_locale_lock();
    selected = g_current_locale[LC_NUMERIC];
    rin_unicode_locale_unlock();
    return (rin_unicode_lconv_t*)&selected->lconv;
}

static int rin_unicode_locale_append_digit(
    char* output, size_t capacity, size_t* written, char digit,
    RinUnicodeDigitSet digit_set)
{
    static const char hanidec[10][4] = {
        "\xE3\x80\x87", "\xE4\xB8\x80", "\xE4\xBA\x8C", "\xE4\xB8\x89",
        "\xE5\x9B\x9B", "\xE4\xBA\x94", "\xE5\x85\xAD", "\xE4\xB8\x83",
        "\xE5\x85\xab", "\xE4\xB9\x9D"
    };
    const char* text = NULL;
    uint32_t codepoint = 0u;
    size_t length = 0u;
    size_t index;
    if (!output || !written || digit < '0' || digit > '9')
        return 0;
    switch (digit_set) {
    case RIN_UNICODE_DIGITS_HANIDEC:
        text = hanidec[(unsigned)(digit - '0')];
        break;
    case RIN_UNICODE_DIGITS_LATIN:
        codepoint = (uint32_t)'0';
        break;
    case RIN_UNICODE_DIGITS_ARABIC_INDIC: codepoint = 0x0660u; break;
    case RIN_UNICODE_DIGITS_EXTENDED_ARABIC_INDIC: codepoint = 0x06f0u; break;
    case RIN_UNICODE_DIGITS_DEVANAGARI: codepoint = 0x0966u; break;
    case RIN_UNICODE_DIGITS_BENGALI: codepoint = 0x09e6u; break;
    case RIN_UNICODE_DIGITS_GURMUKHI: codepoint = 0x0a66u; break;
    case RIN_UNICODE_DIGITS_GUJARATI: codepoint = 0x0ae6u; break;
    case RIN_UNICODE_DIGITS_ORIYA: codepoint = 0x0b66u; break;
    case RIN_UNICODE_DIGITS_TAMIL: codepoint = 0x0be6u; break;
    case RIN_UNICODE_DIGITS_TELUGU: codepoint = 0x0c66u; break;
    case RIN_UNICODE_DIGITS_KANNADA: codepoint = 0x0ce6u; break;
    case RIN_UNICODE_DIGITS_MALAYALAM: codepoint = 0x0d66u; break;
    case RIN_UNICODE_DIGITS_THAI: codepoint = 0x0e50u; break;
    case RIN_UNICODE_DIGITS_LAO: codepoint = 0x0ed0u; break;
    case RIN_UNICODE_DIGITS_TIBETAN: codepoint = 0x0f20u; break;
    case RIN_UNICODE_DIGITS_MYANMAR: codepoint = 0x1040u; break;
    case RIN_UNICODE_DIGITS_KHMER: codepoint = 0x17e0u; break;
    case RIN_UNICODE_DIGITS_FULLWIDTH: codepoint = 0xff10u; break;
    default:
        return 0;
    }

    if (text != NULL) {
        while (text[length] != '\0') ++length;
        if (*written > capacity || length >= capacity - *written)
            return 0;
        for (index = 0u; index < length; ++index)
            output[(*written)++] = text[index];
        return 1;
    }

    codepoint += (uint32_t)(unsigned)(digit - '0');
    if (codepoint <= 0x7fu) {
        length = 1u;
    } else if (codepoint <= 0x7ffu) {
        length = 2u;
    } else if (codepoint <= 0xffffu) {
        length = 3u;
    } else if (codepoint <= 0x10ffffu) {
        length = 4u;
    } else {
        return 0;
    }
    if (*written > capacity || length >= capacity - *written)
        return 0;
    if (length == 1u) {
        output[(*written)++] = (char)codepoint;
    } else if (length == 2u) {
        output[(*written)++] = (char)(0xc0u | (codepoint >> 6u));
        output[(*written)++] = (char)(0x80u | (codepoint & 0x3fu));
    } else if (length == 3u) {
        output[(*written)++] = (char)(0xe0u | (codepoint >> 12u));
        output[(*written)++] = (char)(0x80u | ((codepoint >> 6u) & 0x3fu));
        output[(*written)++] = (char)(0x80u | (codepoint & 0x3fu));
    } else {
        output[(*written)++] = (char)(0xf0u | (codepoint >> 18u));
        output[(*written)++] = (char)(0x80u | ((codepoint >> 12u) & 0x3fu));
        output[(*written)++] = (char)(0x80u | ((codepoint >> 6u) & 0x3fu));
        output[(*written)++] = (char)(0x80u | (codepoint & 0x3fu));
    }
    return 1;
}

size_t rin_unicode_locale_format_integer(char* output, size_t output_capacity,
                                         int64_t value, char const* locale)
{
    RinUnicodeLocale const* selected;
    RinUnicodeDigitSet digit_set;
    char digits[32];
    unsigned char break_before[32] = {0};
    uint64_t magnitude;
    size_t digit_count = 0u;
    size_t remaining;
    size_t grouping_index = 0u;
    unsigned int group;
    size_t written = 0u;
    size_t index;
    char const* separator;
    size_t separator_length = 0u;

    if (output && output_capacity != 0u) output[0] = '\0';
    if (!output || output_capacity == 0u) return 0u;
    if (locale) {
        if (!rin_unicode_find_locale_selection(locale, &selected, &digit_set))
            goto failure;
    } else {
        rin_unicode_locale_lock();
        selected = g_current_locale[LC_NUMERIC];
        digit_set = g_current_digit_set[LC_NUMERIC];
        rin_unicode_locale_unlock();
    }
    if (!selected || !selected->lconv.thousands_sep ||
        !selected->lconv.grouping) goto failure;

    magnitude = value < 0 ? (uint64_t)(0u - (uint64_t)value)
                          : (uint64_t)value;
    do {
        digits[digit_count++] = (char)('0' + (magnitude % 10u));
        magnitude /= 10u;
    } while (magnitude != 0u && digit_count < sizeof(digits));
    if (magnitude != 0u) goto failure;

    separator = selected->lconv.thousands_sep;
    while (separator[separator_length] != '\0') ++separator_length;
    group = (unsigned char)selected->lconv.grouping[grouping_index];
    remaining = digit_count;
    while (group != 0u && group != (unsigned char)127 &&
           remaining > group) {
        remaining -= group;
        break_before[remaining] = 1u;
        if (selected->lconv.grouping[grouping_index + 1u] == '\0') {
            /* A zero grouping entry repeats the previous group size. */
        } else {
            ++grouping_index;
        }
        group = (unsigned char)selected->lconv.grouping[grouping_index];
        if (group == 0u) group = (unsigned char)127;
    }

    if (value < 0) {
        if (written + 1u >= output_capacity) goto failure;
        output[written++] = '-';
    }
    for (index = 0u; index < digit_count; ++index) {
        size_t separator_end;
        if (break_before[index]) {
            if (separator_length > output_capacity - written - 1u)
                goto failure;
            for (separator_end = 0u; separator_end < separator_length;
                 ++separator_end)
                output[written++] = separator[separator_end];
        }
        if (!rin_unicode_locale_append_digit(
                output, output_capacity, &written,
                digits[digit_count - index - 1u], digit_set))
            goto failure;
    }
    output[written] = '\0';
    return written;

failure:
    output[0] = '\0';
    return 0u;
}

static int rin_unicode_decimal_length(char const* text, size_t* length_out)
{
    size_t length;
    if (!text || !length_out) return 0;
    for (length = 0u; length < RIN_UNICODE_MAX_DECIMAL_BYTES; ++length) {
        if (text[length] == '\0') {
            *length_out = length;
            return 1;
        }
    }
    return 0;
}

static int rin_unicode_append_decimal_text(char* output, size_t capacity,
                                           size_t* written,
                                           char const* text)
{
    size_t length = 0u;
    size_t index;
    if (!output || !written || !text || *written >= capacity) return 0;
    while (length < RIN_UNICODE_MAX_DECIMAL_BYTES && text[length] != '\0')
        ++length;
    if (length == RIN_UNICODE_MAX_DECIMAL_BYTES ||
        length > capacity - *written - 1u)
        return 0;
    for (index = 0u; index < length; ++index)
        output[(*written)++] = text[index];
    return 1;
}

size_t rin_unicode_locale_format_decimal(char* output, size_t output_capacity,
                                         char const* number,
                                         char const* locale)
{
    RinUnicodeLocale const* selected;
    RinUnicodeDigitSet digit_set;
    char integer_digits[RIN_UNICODE_MAX_DECIMAL_BYTES];
    char fraction_digits[RIN_UNICODE_MAX_DECIMAL_BYTES];
    unsigned char break_before[RIN_UNICODE_MAX_DECIMAL_BYTES] = {0};
    size_t length = 0u;
    size_t integer_length = 0u;
    size_t fraction_length = 0u;
    size_t dot = (size_t)-1;
    size_t start = 0u;
    size_t index;
    size_t written = 0u;
    size_t grouping_index = 0u;
    size_t remaining;
    unsigned int group;
    char const* separator;
    char const* decimal_point;
    if (output && output_capacity != 0u) output[0] = '\0';
    if (!output || output_capacity == 0u ||
        !rin_unicode_decimal_length(number, &length) || length == 0u)
        return 0u;
    if (number[0] == '-') {
        start = 1u;
        if (start == length) goto failure;
    } else if (number[0] == '+') {
        goto failure;
    }
    for (index = start; index < length; ++index) {
        if (number[index] == '.') {
            if (dot != (size_t)-1 || index == start ||
                index + 1u >= length)
                goto failure;
            dot = index;
        } else if (number[index] < '0' || number[index] > '9') {
            goto failure;
        }
    }
    if (dot == (size_t)-1) dot = length;
    integer_length = dot - start;
    fraction_length = dot == length ? 0u : length - dot - 1u;
    if (integer_length == 0u || integer_length >= sizeof(integer_digits) ||
        fraction_length >= sizeof(fraction_digits))
        goto failure;
    for (index = 0u; index < integer_length; ++index)
        integer_digits[index] = number[start + index];
    for (index = 0u; index < fraction_length; ++index)
        fraction_digits[index] = number[dot + 1u + index];
    if (locale) {
        if (!rin_unicode_find_locale_selection(locale, &selected, &digit_set))
            goto failure;
    } else {
        rin_unicode_locale_lock();
        selected = g_current_locale[LC_NUMERIC];
        digit_set = g_current_digit_set[LC_NUMERIC];
        rin_unicode_locale_unlock();
    }
    if (!selected || !selected->lconv.decimal_point ||
        !selected->lconv.thousands_sep || !selected->lconv.grouping)
        goto failure;

    remaining = integer_length;
    group = (unsigned char)selected->lconv.grouping[grouping_index];
    while (group != 0u && group != (unsigned char)127 && remaining > group) {
        remaining -= group;
        break_before[remaining] = 1u;
        if (selected->lconv.grouping[grouping_index + 1u] != '\0')
            ++grouping_index;
        group = (unsigned char)selected->lconv.grouping[grouping_index];
        if (group == 0u) group = (unsigned char)127;
    }
    separator = selected->lconv.thousands_sep;
    decimal_point = selected->lconv.decimal_point;
    if (number[0] == '-' &&
        !rin_unicode_append_decimal_text(output, output_capacity, &written,
                                         "-"))
        goto failure;
    for (index = 0u; index < integer_length; ++index) {
        if (break_before[index] &&
            !rin_unicode_append_decimal_text(output, output_capacity, &written,
                                             separator))
            goto failure;
        if (!rin_unicode_locale_append_digit(
                output, output_capacity, &written, integer_digits[index],
                digit_set))
            goto failure;
    }
    if (fraction_length != 0u &&
        (!rin_unicode_append_decimal_text(output, output_capacity, &written,
                                          decimal_point) ||
         fraction_length > output_capacity - written - 1u))
        goto failure;
    for (index = 0u; index < fraction_length; ++index)
        if (!rin_unicode_locale_append_digit(
                output, output_capacity, &written, fraction_digits[index],
                digit_set))
            goto failure;
    output[written] = '\0';
    return written;

failure:
    if (output && output_capacity != 0u) output[0] = '\0';
    return 0u;
}

static int rin_unicode_currency_add_part(const char** parts, size_t capacity,
                                          size_t* count, const char* part)
{
    if (!parts || !count || !part || *count >= capacity) return 0;
    parts[(*count)++] = part;
    return 1;
}

size_t rin_unicode_locale_format_currency(char* output, size_t output_capacity,
                                          char const* number,
                                          char const* locale)
{
    RinUnicodeLocale const* selected;
    RinUnicodeDigitSet digit_set;
    char integer_digits[RIN_UNICODE_MAX_DECIMAL_BYTES];
    char fraction_digits[RIN_UNICODE_MAX_DECIMAL_BYTES];
    char amount[RIN_UNICODE_MAX_CURRENCY_BYTES];
    unsigned char break_before[RIN_UNICODE_MAX_DECIMAL_BYTES] = {0};
    const char* parts[12];
    const char* separator;
    const char* decimal_point;
    const char* symbol;
    const char* sign;
    size_t length = 0u;
    size_t integer_length = 0u;
    size_t fraction_length = 0u;
    size_t monetary_fraction_length;
    size_t dot = (size_t)-1;
    size_t start = 0u;
    size_t index;
    size_t amount_written = 0u;
    size_t written = 0u;
    size_t part_count = 0u;
    size_t grouping_index = 0u;
    unsigned int group;
    unsigned int symbol_precedes;
    unsigned int separator_by_space;
    unsigned int sign_position;
    int negative;
    static const char space[] = " ";
    static const char open[] = "(";
    static const char close[] = ")";

    if (output && output_capacity != 0u) output[0] = '\0';
    if (!output || output_capacity == 0u ||
        !rin_unicode_decimal_length(number, &length) || length == 0u)
        return 0u;

    negative = number[0] == '-';
    if (negative) {
        start = 1u;
        if (start == length) goto failure;
    } else if (number[0] == '+') {
        goto failure;
    }
    for (index = start; index < length; ++index) {
        if (number[index] == '.') {
            if (dot != (size_t)-1 || index == start ||
                index + 1u >= length)
                goto failure;
            dot = index;
        } else if (number[index] < '0' || number[index] > '9') {
            goto failure;
        }
    }
    if (dot == (size_t)-1) dot = length;
    integer_length = dot - start;
    fraction_length = dot == length ? 0u : length - dot - 1u;
    if (integer_length == 0u || integer_length >= sizeof(integer_digits) ||
        fraction_length >= sizeof(fraction_digits))
        goto failure;
    for (index = 0u; index < integer_length; ++index)
        integer_digits[index] = number[start + index];
    for (index = 0u; index < fraction_length; ++index)
        fraction_digits[index] = number[dot + 1u + index];

    if (locale) {
        if (!rin_unicode_find_locale_selection(locale, &selected, &digit_set))
            goto failure;
    } else {
        rin_unicode_locale_lock();
        selected = g_current_locale[LC_MONETARY];
        digit_set = g_current_digit_set[LC_MONETARY];
        rin_unicode_locale_unlock();
    }
    if (!selected || !selected->lconv.mon_decimal_point ||
        !selected->lconv.mon_thousands_sep || !selected->lconv.mon_grouping ||
        !selected->lconv.currency_symbol ||
        !selected->lconv.positive_sign || !selected->lconv.negative_sign)
        goto failure;
    monetary_fraction_length =
        (unsigned char)selected->lconv.frac_digits;
    if (monetary_fraction_length == 127u ||
        monetary_fraction_length >= sizeof(fraction_digits) ||
        fraction_length > monetary_fraction_length)
        goto failure;
    for (index = fraction_length; index < monetary_fraction_length; ++index)
        fraction_digits[index] = '0';

    separator = selected->lconv.mon_thousands_sep;
    decimal_point = selected->lconv.mon_decimal_point;
    symbol = selected->lconv.currency_symbol;
    sign = negative ? selected->lconv.negative_sign
                    : selected->lconv.positive_sign;
    symbol_precedes = (unsigned char)(negative
                                          ? selected->lconv.n_cs_precedes
                                          : selected->lconv.p_cs_precedes);
    separator_by_space = (unsigned char)(negative
                                             ? selected->lconv.n_sep_by_space
                                             : selected->lconv.p_sep_by_space);
    sign_position = (unsigned char)(negative
                                        ? selected->lconv.n_sign_posn
                                        : selected->lconv.p_sign_posn);
    if (symbol[0] == '\0' || symbol_precedes > 1u ||
        separator_by_space > 1u || sign_position > 4u)
        goto failure;

    group = (unsigned char)selected->lconv.mon_grouping[grouping_index];
    size_t remaining = integer_length;
    while (group != 0u && group != (unsigned char)127 && remaining > group) {
        remaining -= group;
        break_before[remaining] = 1u;
        if (selected->lconv.mon_grouping[grouping_index + 1u] != '\0')
            ++grouping_index;
        group = (unsigned char)selected->lconv.mon_grouping[grouping_index];
        if (group == 0u) group = (unsigned char)127;
    }
    for (index = 0u; index < integer_length; ++index) {
        if (break_before[index] &&
            !rin_unicode_append_decimal_text(amount, sizeof(amount),
                                             &amount_written, separator))
            goto failure;
        if (!rin_unicode_locale_append_digit(
                amount, sizeof(amount), &amount_written, integer_digits[index],
                digit_set))
            goto failure;
    }
    if (monetary_fraction_length != 0u &&
        (!rin_unicode_append_decimal_text(amount, sizeof(amount),
                                          &amount_written, decimal_point) ||
         monetary_fraction_length > sizeof(amount) - amount_written - 1u))
        goto failure;
    for (index = 0u; index < monetary_fraction_length; ++index)
        if (!rin_unicode_locale_append_digit(
                amount, sizeof(amount), &amount_written, fraction_digits[index],
                digit_set))
            goto failure;
    amount[amount_written] = '\0';

    if (negative && sign_position == 0u &&
        !rin_unicode_currency_add_part(parts, sizeof(parts) / sizeof(parts[0]),
                                       &part_count, open))
        goto failure;
    if (sign_position == 1u && sign[0] != '\0' &&
        !rin_unicode_currency_add_part(parts, sizeof(parts) / sizeof(parts[0]),
                                       &part_count, sign))
        goto failure;
    if (symbol_precedes != 0u) {
        if (!rin_unicode_currency_add_part(
                parts, sizeof(parts) / sizeof(parts[0]), &part_count, symbol))
            goto failure;
        if (sign_position == 2u && sign[0] != '\0' &&
            !rin_unicode_currency_add_part(
                parts, sizeof(parts) / sizeof(parts[0]), &part_count, sign))
            goto failure;
        if (separator_by_space != 0u &&
            !rin_unicode_currency_add_part(
                parts, sizeof(parts) / sizeof(parts[0]), &part_count, space))
            goto failure;
        if (sign_position == 3u && sign[0] != '\0' &&
            !rin_unicode_currency_add_part(
                parts, sizeof(parts) / sizeof(parts[0]), &part_count, sign))
            goto failure;
        if (!rin_unicode_currency_add_part(
                parts, sizeof(parts) / sizeof(parts[0]), &part_count, amount))
            goto failure;
        if (sign_position == 4u && sign[0] != '\0' &&
            !rin_unicode_currency_add_part(
                parts, sizeof(parts) / sizeof(parts[0]), &part_count, sign))
            goto failure;
    } else {
        if (sign_position == 3u && sign[0] != '\0' &&
            !rin_unicode_currency_add_part(
                parts, sizeof(parts) / sizeof(parts[0]), &part_count, sign))
            goto failure;
        if (!rin_unicode_currency_add_part(
                parts, sizeof(parts) / sizeof(parts[0]), &part_count, amount))
            goto failure;
        if (sign_position == 4u && sign[0] != '\0' &&
            !rin_unicode_currency_add_part(
                parts, sizeof(parts) / sizeof(parts[0]), &part_count, sign))
            goto failure;
        if (separator_by_space != 0u &&
            !rin_unicode_currency_add_part(
                parts, sizeof(parts) / sizeof(parts[0]), &part_count, space))
            goto failure;
        if (!rin_unicode_currency_add_part(
                parts, sizeof(parts) / sizeof(parts[0]), &part_count, symbol))
            goto failure;
        if (sign_position == 2u && sign[0] != '\0' &&
            !rin_unicode_currency_add_part(
                parts, sizeof(parts) / sizeof(parts[0]), &part_count, sign))
            goto failure;
    }
    if (negative && sign_position == 0u &&
        !rin_unicode_currency_add_part(parts, sizeof(parts) / sizeof(parts[0]),
                                       &part_count, close))
        goto failure;
    for (index = 0u; index < part_count; ++index) {
        if (!rin_unicode_append_decimal_text(output, output_capacity, &written,
                                             parts[index]))
            goto failure;
    }
    output[written] = '\0';
    return written;

failure:
    if (output && output_capacity != 0u) output[0] = '\0';
    return 0u;
}

char const* rin_unicode_locale_name(int category)
{
    RinUnicodeLocale const* selected;
    if (category < 0 || category >= LC_MAX) return g_locale_root->name;
    rin_unicode_locale_lock();
    selected = g_current_locale[category];
    rin_unicode_locale_unlock();
    return selected->name;
}

int rin_unicode_locale_canonicalize(char const* locale, char* output,
                                    size_t output_capacity)
{
    RinUnicodeLocale const* selected;
    size_t length = 0u;
    size_t i;
    if (output && output_capacity != 0u) output[0] = '\0';
    if (!output || output_capacity == 0u || !locale) return 0;
    selected = rin_unicode_find_locale(locale);
    if (!selected || !selected->locale_id) return 0;
    while (selected->locale_id[length] != '\0') ++length;
    if (length + 1u > output_capacity) return 0;
    for (i = 0u; i < length; ++i) output[i] = selected->locale_id[i];
    output[length] = '\0';
    return 1;
}

char const* rin_unicode_locale_environment_value(int category)
{
    RinUnicodeLocale const* selected;
    if (category < 0 || category >= LC_ALL) return (char const*)0;
    rin_unicode_locale_lock();
    selected = rin_unicode_locale_from_environment(category, (RinUnicodeDigitSet*)0);
    rin_unicode_locale_unlock();
    return selected->name;
}

char const* rin_unicode_locale_language(void)
{
    RinUnicodeLocale const* selected;
    rin_unicode_locale_lock();
    selected = g_current_locale[LC_MESSAGES];
    rin_unicode_locale_unlock();
    return selected->language;
}

char const* rin_unicode_locale_territory(void)
{
    RinUnicodeLocale const* selected;
    rin_unicode_locale_lock();
    selected = g_current_locale[LC_MESSAGES];
    rin_unicode_locale_unlock();
    return selected->territory;
}

char const* rin_unicode_locale_codeset(void)
{
    RinUnicodeLocale const* selected;
    rin_unicode_locale_lock();
    selected = g_current_locale[LC_CTYPE];
    rin_unicode_locale_unlock();
    return selected->codeset;
}

int rin_unicode_locale_is_japanese(void)
{
    RinUnicodeLocale const* selected;
    int result;
    rin_unicode_locale_lock();
    selected = g_current_locale[LC_MESSAGES];
    result = rin_unicode_ascii_ieq(selected->language, "ja");
    rin_unicode_locale_unlock();
    return result;
}

int rin_unicode_locale_is_utf8(void)
{
    RinUnicodeLocale const* selected;
    int result;
    rin_unicode_locale_lock();
    selected = g_current_locale[LC_CTYPE];
    result = !rin_unicode_ascii_ieq(selected->codeset, "ASCII");
    rin_unicode_locale_unlock();
    return result;
}

typedef struct RinUnicodeTimeNames {
    char const* language;
    char const* territory;
    char const* weekdays_short[7];
    char const* weekdays_long[7];
    char const* months_short[12];
    char const* months_long[12];
    char const* am_pm[2];
    char const* date_time_format;
    char const* date_format;
    char const* time_format;
} RinUnicodeTimeNames;

static RinUnicodeTimeNames const g_rin_unicode_time_names[] = {
    {
        "en", "US",
        {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
        {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"},
        {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"},
        {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"},
        {"AM", "PM"}, "%a %b %e %H:%M:%S %Y", "%m/%d/%Y", "%H:%M:%S"
    },
    {
        "ja", "JP",
        {"日", "月", "火", "水", "木", "金", "土"},
        {"日曜日", "月曜日", "火曜日", "水曜日", "木曜日", "金曜日", "土曜日"},
        {"1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月"},
        {"1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月"},
        {"午前", "午後"}, "%Y年%m月%d日 %H時%M分%S秒", "%Y年%m月%d日", "%H時%M分%S秒"
    },
    {
        "zh", "CN",
        {"周日", "周一", "周二", "周三", "周四", "周五", "周六"},
        {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"},
        {"1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月"},
        {"一月", "二月", "三月", "四月", "五月", "六月", "七月", "八月", "九月", "十月", "十一月", "十二月"},
        {"上午", "下午"}, "%Y年%m月%d日 %H时%M分%S秒", "%Y年%m月%d日", "%H时%M分%S秒"
    },
    {
        "zh", "TW",
        {"週日", "週一", "週二", "週三", "週四", "週五", "週六"},
        {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"},
        {"1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月"},
        {"一月", "二月", "三月", "四月", "五月", "六月", "七月", "八月", "九月", "十月", "十一月", "十二月"},
        {"上午", "下午"}, "%Y年%m月%d日 %H時%M分%S秒", "%Y年%m月%d日", "%H時%M分%S秒"
    },
    {
        "ko", "KR",
        {"일", "월", "화", "수", "목", "금", "토"},
        {"일요일", "월요일", "화요일", "수요일", "목요일", "금요일", "토요일"},
        {"1월", "2월", "3월", "4월", "5월", "6월", "7월", "8월", "9월", "10월", "11월", "12월"},
        {"1월", "2월", "3월", "4월", "5월", "6월", "7월", "8월", "9월", "10월", "11월", "12월"},
        {"오전", "오후"}, "%Y년 %m월 %d일 %H시 %M분 %S초", "%Y년 %m월 %d일", "%H시 %M분 %S초"
    },
    {
        "de", "DE",
        {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"},
        {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"},
        {"Jan", "Feb", "Mär", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"},
        {"Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"},
        {"AM", "PM"}, "%a %e. %b %Y %H:%M:%S", "%d.%m.%Y", "%H:%M:%S"
    },
    {
        "fr", "FR",
        {"dim.", "lun.", "mar.", "mer.", "jeu.", "ven.", "sam."},
        {"dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"},
        {"janv.", "févr.", "mars", "avr.", "mai", "juin", "juil.", "août", "sept.", "oct.", "nov.", "déc."},
        {"janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre", "octobre", "novembre", "décembre"},
        {"AM", "PM"}, "%a %e %b %Y %H:%M:%S", "%d/%m/%Y", "%H:%M:%S"
    },
    {
        "es", "ES",
        {"dom", "lun", "mar", "mié", "jue", "vie", "sáb"},
        {"domingo", "lunes", "martes", "miércoles", "jueves", "viernes", "sábado"},
        {"ene", "feb", "mar", "abr", "may", "jun", "jul", "ago", "sep", "oct", "nov", "dic"},
        {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto", "septiembre", "octubre", "noviembre", "diciembre"},
        {"AM", "PM"}, "%a %e de %b de %Y %H:%M:%S", "%d/%m/%Y", "%H:%M:%S"
    },
    {
        "it", "IT",
        {"dom", "lun", "mar", "mer", "gio", "ven", "sab"},
        {"domenica", "lunedì", "martedì", "mercoledì", "giovedì", "venerdì", "sabato"},
        {"gen", "feb", "mar", "apr", "mag", "giu", "lug", "ago", "set", "ott", "nov", "dic"},
        {"gennaio", "febbraio", "marzo", "aprile", "maggio", "giugno", "luglio", "agosto", "settembre", "ottobre", "novembre", "dicembre"},
        {"AM", "PM"}, "%a %e %b %Y %H:%M:%S", "%d/%m/%Y", "%H:%M:%S"
    },
    {
        "pt", "BR",
        {"dom", "seg", "ter", "qua", "qui", "sex", "sáb"},
        {"domingo", "segunda-feira", "terça-feira", "quarta-feira", "quinta-feira", "sexta-feira", "sábado"},
        {"jan", "fev", "mar", "abr", "mai", "jun", "jul", "ago", "set", "out", "nov", "dez"},
        {"janeiro", "fevereiro", "março", "abril", "maio", "junho", "julho", "agosto", "setembro", "outubro", "novembro", "dezembro"},
        {"AM", "PM"}, "%a %e de %b de %Y %H:%M:%S", "%d/%m/%Y", "%H:%M:%S"
    },
    {
        "es", "MX",
        {"dom", "lun", "mar", "mié", "jue", "vie", "sáb"},
        {"domingo", "lunes", "martes", "miércoles", "jueves", "viernes", "sábado"},
        {"ene", "feb", "mar", "abr", "may", "jun", "jul", "ago", "sep", "oct", "nov", "dic"},
        {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto", "septiembre", "octubre", "noviembre", "diciembre"},
        {"AM", "PM"}, "%a %e de %b de %Y %H:%M:%S", "%d/%m/%Y", "%H:%M:%S"
    },
    {
        "pt", "PT",
        {"dom", "seg", "ter", "qua", "qui", "sex", "sáb"},
        {"domingo", "segunda-feira", "terça-feira", "quarta-feira", "quinta-feira", "sexta-feira", "sábado"},
        {"jan", "fev", "mar", "abr", "mai", "jun", "jul", "ago", "set", "out", "nov", "dez"},
        {"janeiro", "fevereiro", "março", "abril", "maio", "junho", "julho", "agosto", "setembro", "outubro", "novembro", "dezembro"},
        {"AM", "PM"}, "%a %e de %b de %Y %H:%M:%S", "%d/%m/%Y", "%H:%M:%S"
    }
};

static RinUnicodeTimeNames const* rin_unicode_current_time_names_unlocked(void)
{
    RinUnicodeLocale const* locale = g_current_locale[LC_TIME];
    size_t i;
    for (i = 0u; i < sizeof(g_rin_unicode_time_names) / sizeof(g_rin_unicode_time_names[0]); ++i) {
        RinUnicodeTimeNames const* names = &g_rin_unicode_time_names[i];
        if (rin_unicode_ascii_ieq(locale->language, names->language) &&
            rin_unicode_ascii_ieq(locale->territory, names->territory)) return names;
    }
    return &g_rin_unicode_time_names[0];
}

char const* rin_unicode_locale_weekday(int weekday, int abbreviated)
{
    RinUnicodeTimeNames const* names;
    char const* result;
    rin_unicode_locale_lock();
    names = rin_unicode_current_time_names_unlocked();
    if (weekday < 0 || weekday >= 7) {
        rin_unicode_locale_unlock();
        return NULL;
    }
    result = abbreviated ? names->weekdays_short[weekday] : names->weekdays_long[weekday];
    rin_unicode_locale_unlock();
    return result;
}

char const* rin_unicode_locale_month(int month, int abbreviated)
{
    RinUnicodeTimeNames const* names;
    char const* result;
    rin_unicode_locale_lock();
    names = rin_unicode_current_time_names_unlocked();
    if (month < 0 || month >= 12) {
        rin_unicode_locale_unlock();
        return NULL;
    }
    result = abbreviated ? names->months_short[month] : names->months_long[month];
    rin_unicode_locale_unlock();
    return result;
}

char const* rin_unicode_locale_am_pm(int hour)
{
    RinUnicodeTimeNames const* names;
    char const* result;
    if (hour < 0 || hour >= 24) return NULL;
    rin_unicode_locale_lock();
    names = rin_unicode_current_time_names_unlocked();
    result = names->am_pm[hour >= 12 ? 1 : 0];
    rin_unicode_locale_unlock();
    return result;
}

char const* rin_unicode_locale_time_format(char conversion)
{
    RinUnicodeTimeNames const* names;
    char const* result;
    if (conversion != 'c' && conversion != 'x' && conversion != 'X')
        return NULL;
    rin_unicode_locale_lock();
    names = rin_unicode_current_time_names_unlocked();
    if (conversion == 'c') result = names->date_time_format;
    else if (conversion == 'x') result = names->date_format;
    else result = names->time_format;
    rin_unicode_locale_unlock();
    return result;
}

static int rin_unicode_datetime_leap_year(int year)
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

static int rin_unicode_datetime_days_in_month(int year, int month)
{
    static const unsigned char days[12] = {
        31u, 28u, 31u, 30u, 31u, 30u,
        31u, 31u, 30u, 31u, 30u, 31u
    };
    if (month < 1 || month > 12) return 0;
    if (month == 2 && rin_unicode_datetime_leap_year(year)) return 29;
    return days[month - 1];
}

static int rin_unicode_datetime_valid(const rin_unicode_datetime_t* value,
                                      unsigned* day_of_year)
{
    int month_days;
    unsigned ordinal = 0u;
    int month;
    if (!value || !day_of_year || value->year < 0 || value->year > 9999 ||
        value->weekday < 0 || value->weekday > 6 || value->hour < 0 ||
        value->hour > 23 || value->minute < 0 || value->minute > 59 ||
        value->second < 0 || value->second > 60) return 0;
    month_days = rin_unicode_datetime_days_in_month(value->year,
                                                     value->month);
    if (month_days == 0 || value->day < 1 || value->day > month_days)
        return 0;
    for (month = 1; month < value->month; ++month)
        ordinal += (unsigned)rin_unicode_datetime_days_in_month(
            value->year, month);
    *day_of_year = ordinal + (unsigned)value->day;
    return 1;
}

static int rin_unicode_datetime_append(char* output, size_t capacity,
                                       size_t* written, const char* text)
{
    size_t length = 0u;
    size_t index;
    if (!output || !written || !text || *written >= capacity) return 0;
    while (length < RIN_UNICODE_MAX_DATETIME_FORMAT_BYTES && text[length] != '\0')
        ++length;
    if (length == RIN_UNICODE_MAX_DATETIME_FORMAT_BYTES ||
        length > capacity - *written - 1u) return 0;
    for (index = 0u; index < length; ++index)
        output[(*written)++] = text[index];
    return 1;
}

static int rin_unicode_datetime_append_fixed(char* output, size_t capacity,
                                             size_t* written, unsigned value,
                                             unsigned width, int blank_pad)
{
    char digits[10];
    unsigned index;
    if (width == 0u || width > sizeof(digits) ||
        (width < sizeof(digits) && value >= 1000000000u)) return 0;
    for (index = width; index != 0u; --index) {
        digits[index - 1u] = (char)('0' + value % 10u);
        value /= 10u;
    }
    if (value != 0u) return 0;
    if (blank_pad && width == 2u && digits[0] == '0') {
        digits[0] = ' ';
    }
    if (!output || !written || *written >= capacity ||
        width > capacity - *written - 1u) return 0;
    for (index = 0u; index < width; ++index)
        output[(*written)++] = digits[index];
    return 1;
}

static int rin_unicode_datetime_append_pattern(
    char* output, size_t capacity, size_t* written,
    const char* pattern, const RinUnicodeTimeNames* names,
    const rin_unicode_datetime_t* value, unsigned day_of_year,
    unsigned depth)
{
    size_t index = 0u;
    if (!pattern || !names || !value || depth > 2u) return 0;
    while (pattern[index] != '\0') {
        char conversion;
        const char* nested = (const char*)0;
        if (pattern[index] != '%') {
            char literal[2] = {pattern[index], '\0'};
            if (!rin_unicode_datetime_append(output, capacity, written, literal))
                return 0;
            ++index;
            continue;
        }
        conversion = pattern[++index];
        if (conversion == '\0') return 0;
        switch (conversion) {
        case '%':
            if (!rin_unicode_datetime_append(output, capacity, written, "%"))
                return 0;
            break;
        case 'a':
            if (!rin_unicode_datetime_append(output, capacity, written,
                                             names->weekdays_short[value->weekday]))
                return 0;
            break;
        case 'A':
            if (!rin_unicode_datetime_append(output, capacity, written,
                                             names->weekdays_long[value->weekday]))
                return 0;
            break;
        case 'b':
        case 'h':
            if (!rin_unicode_datetime_append(output, capacity, written,
                                             names->months_short[value->month - 1]))
                return 0;
            break;
        case 'B':
            if (!rin_unicode_datetime_append(output, capacity, written,
                                             names->months_long[value->month - 1]))
                return 0;
            break;
        case 'C':
            if (!rin_unicode_datetime_append_fixed(
                    output, capacity, written, (unsigned)(value->year / 100),
                    2u, 0)) return 0;
            break;
        case 'd':
            if (!rin_unicode_datetime_append_fixed(
                    output, capacity, written, (unsigned)value->day, 2u, 0))
                return 0;
            break;
        case 'e':
            if (!rin_unicode_datetime_append_fixed(
                    output, capacity, written, (unsigned)value->day, 2u, 1))
                return 0;
            break;
        case 'F':
            if (!rin_unicode_datetime_append_fixed(
                    output, capacity, written, (unsigned)value->year, 4u, 0) ||
                !rin_unicode_datetime_append(output, capacity, written, "-") ||
                !rin_unicode_datetime_append_fixed(
                    output, capacity, written, (unsigned)value->month, 2u, 0) ||
                !rin_unicode_datetime_append(output, capacity, written, "-") ||
                !rin_unicode_datetime_append_fixed(
                    output, capacity, written, (unsigned)value->day, 2u, 0))
                return 0;
            break;
        case 'H':
            if (!rin_unicode_datetime_append_fixed(
                    output, capacity, written, (unsigned)value->hour, 2u, 0))
                return 0;
            break;
        case 'I': {
            unsigned hour = (unsigned)(value->hour % 12);
            if (hour == 0u) hour = 12u;
            if (!rin_unicode_datetime_append_fixed(output, capacity, written,
                                                   hour, 2u, 0)) return 0;
            break;
        }
        case 'j':
            if (!rin_unicode_datetime_append_fixed(output, capacity, written,
                                                   day_of_year, 3u, 0)) return 0;
            break;
        case 'm':
            if (!rin_unicode_datetime_append_fixed(
                    output, capacity, written, (unsigned)value->month, 2u, 0))
                return 0;
            break;
        case 'M':
            if (!rin_unicode_datetime_append_fixed(
                    output, capacity, written, (unsigned)value->minute, 2u, 0))
                return 0;
            break;
        case 'n':
            if (!rin_unicode_datetime_append(output, capacity, written, "\n"))
                return 0;
            break;
        case 'p':
            if (!rin_unicode_datetime_append(output, capacity, written,
                                             names->am_pm[value->hour >= 12]))
                return 0;
            break;
        case 'R':
            nested = "%H:%M";
            break;
        case 'r':
            nested = "%I:%M:%S %p";
            break;
        case 'S':
            if (!rin_unicode_datetime_append_fixed(
                    output, capacity, written, (unsigned)value->second, 2u, 0))
                return 0;
            break;
        case 'T':
            nested = "%H:%M:%S";
            break;
        case 't':
            if (!rin_unicode_datetime_append(output, capacity, written, "\t"))
                return 0;
            break;
        case 'u':
            if (!rin_unicode_datetime_append_fixed(
                    output, capacity, written,
                    (unsigned)(value->weekday == 0 ? 7 : value->weekday),
                    1u, 0)) return 0;
            break;
        case 'w':
            if (!rin_unicode_datetime_append_fixed(
                    output, capacity, written, (unsigned)value->weekday, 1u, 0))
                return 0;
            break;
        case 'x':
            nested = names->date_format;
            break;
        case 'X':
            nested = names->time_format;
            break;
        case 'y':
            if (!rin_unicode_datetime_append_fixed(
                    output, capacity, written, (unsigned)(value->year % 100),
                    2u, 0)) return 0;
            break;
        case 'Y':
            if (!rin_unicode_datetime_append_fixed(
                    output, capacity, written, (unsigned)value->year, 4u, 0))
                return 0;
            break;
        case 'c':
            nested = names->date_time_format;
            break;
        default:
            return 0;
        }
        if (nested && !rin_unicode_datetime_append_pattern(
                           output, capacity, written, nested, names, value,
                           day_of_year, depth + 1u)) return 0;
        ++index;
    }
    return 1;
}

size_t rin_unicode_locale_format_datetime(
    char* output, size_t output_capacity,
    const rin_unicode_datetime_t* value, char conversion)
{
    RinUnicodeTimeNames const* names;
    const char* pattern;
    unsigned day_of_year;
    size_t written = 0u;
    if (output && output_capacity != 0u) output[0] = '\0';
    if (!output || output_capacity == 0u || !value ||
        (conversion != 'c' && conversion != 'x' && conversion != 'X') ||
        !rin_unicode_datetime_valid(value, &day_of_year)) return 0u;
    rin_unicode_locale_lock();
    names = rin_unicode_current_time_names_unlocked();
    pattern = conversion == 'c' ? names->date_time_format
                                : conversion == 'x' ? names->date_format
                                                    : names->time_format;
    rin_unicode_locale_unlock();
    if (!rin_unicode_datetime_append_pattern(output, output_capacity, &written,
                                             pattern, names, value,
                                             day_of_year, 0u)) {
        output[0] = '\0';
        return 0u;
    }
    output[written] = '\0';
    return written;
}

int rin_unicode_strcoll(char const* s1, char const* s2) { return rin_unicode_compare_utf8(s1 ? s1 : "", s2 ? s2 : ""); }
size_t rin_unicode_strxfrm(char* dest, char const* src, size_t n) { return rin_unicode_transform_utf8(dest, n, src ? src : ""); }
int rin_unicode_wcscoll32(uint32_t const* s1, uint32_t const* s2) { return rin_unicode_compare_utf32(s1, s2); }
size_t rin_unicode_wcsxfrm32(uint32_t* dest, uint32_t const* src, size_t n) { return rin_unicode_transform_utf32(dest, n, src); }

char* rin_setlocale(int category, char const* locale) { return rin_unicode_setlocale(category, locale); }
struct lconv* rin_localeconv(void) { return (struct lconv*)rin_unicode_localeconv(); }
char const* rin_locale_name(int category) { return rin_unicode_locale_name(category); }
char const* rin_locale_environment_name(int category) { return rin_unicode_locale_environment_value(category); }
char const* rin_locale_language(void) { return rin_unicode_locale_language(); }
char const* rin_locale_territory(void) { return rin_unicode_locale_territory(); }
char const* rin_locale_codeset(void) { return rin_unicode_locale_codeset(); }
int rin_locale_is_japanese(void) { return rin_unicode_locale_is_japanese(); }
int rin_locale_is_utf8(void) { return rin_unicode_locale_is_utf8(); }
char const* rin_locale_weekday(int weekday, int abbreviated) { return rin_unicode_locale_weekday(weekday, abbreviated); }
char const* rin_locale_month(int month, int abbreviated) { return rin_unicode_locale_month(month, abbreviated); }
char const* rin_locale_am_pm(int hour) { return rin_unicode_locale_am_pm(hour); }
char const* rin_locale_time_format(char conversion) { return rin_unicode_locale_time_format(conversion); }
size_t rin_locale_format_datetime(char* output, size_t output_capacity,
                                  const rin_unicode_datetime_t* value,
                                  char conversion)
{
    return rin_unicode_locale_format_datetime(output, output_capacity, value,
                                              conversion);
}

/* `locale_t` is intentionally opaque at the public boundary, but the libc
 * locale object has a fixed 24-byte representation.  Keep this private view
 * in the adapter so *_l time functions can select LC_TIME data without
 * mutating the process-global Unicode locale. */
typedef struct RinLocaleObjectView {
    uint32_t magic;
    uint32_t version;
    uint8_t categories[6];
    uint8_t reserved[10];
} RinLocaleObjectView;

_Static_assert(sizeof(RinLocaleObjectView) == 24u,
               "locale object adapter layout drift");

typedef struct RinLocaleTimeNames {
    const char* short_weekdays[7];
    const char* long_weekdays[7];
    const char* short_months[12];
    const char* long_months[12];
    const char* am_pm[2];
    const char* date_time_format;
    const char* date_format;
    const char* time_format;
} RinLocaleTimeNames;

static const RinLocaleTimeNames rin_locale_time_c = {
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
    {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"},
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"},
    {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"},
    {"AM", "PM"}, "%a %b %e %H:%M:%S %Y", "%m/%d/%Y", "%H:%M:%S"
};

static const RinLocaleTimeNames rin_locale_time_ja = {
    {"日", "月", "火", "水", "木", "金", "土"},
    {"日曜日", "月曜日", "火曜日", "水曜日", "木曜日", "金曜日", "土曜日"},
    {"1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月"},
    {"1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月"},
    {"午前", "午後"}, "%Y年%m月%d日 %H時%M分%S秒", "%Y年%m月%d日", "%H時%M分%S秒"
};

static const RinLocaleTimeNames rin_locale_time_de = {
    {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"},
    {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"},
    {"Jan", "Feb", "Mär", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"},
    {"Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"},
    {"AM", "PM"}, "%a %e. %b %Y %H:%M:%S", "%d.%m.%Y", "%H:%M:%S"
};

static const RinLocaleTimeNames rin_locale_time_fr = {
    {"dim.", "lun.", "mar.", "mer.", "jeu.", "ven.", "sam."},
    {"dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"},
    {"janv.", "févr.", "mars", "avr.", "mai", "juin", "juil.", "août", "sept.", "oct.", "nov.", "déc."},
    {"janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre", "octobre", "novembre", "décembre"},
    {"AM", "PM"}, "%a %e %b %Y %H:%M:%S", "%d/%m/%Y", "%H:%M:%S"
};

static const RinLocaleTimeNames rin_locale_time_es = {
    {"dom", "lun", "mar", "mié", "jue", "vie", "sáb"},
    {"domingo", "lunes", "martes", "miércoles", "jueves", "viernes", "sábado"},
    {"ene", "feb", "mar", "abr", "may", "jun", "jul", "ago", "sep", "oct", "nov", "dic"},
    {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto", "septiembre", "octubre", "noviembre", "diciembre"},
    {"AM", "PM"}, "%a %e de %b de %Y %H:%M:%S", "%d/%m/%Y", "%H:%M:%S"
};

static const RinLocaleTimeNames rin_locale_time_it = {
    {"dom", "lun", "mar", "mer", "gio", "ven", "sab"},
    {"domenica", "lunedì", "martedì", "mercoledì", "giovedì", "venerdì", "sabato"},
    {"gen", "feb", "mar", "apr", "mag", "giu", "lug", "ago", "set", "ott", "nov", "dic"},
    {"gennaio", "febbraio", "marzo", "aprile", "maggio", "giugno", "luglio", "agosto", "settembre", "ottobre", "novembre", "dicembre"},
    {"AM", "PM"}, "%a %e %b %Y %H:%M:%S", "%d/%m/%Y", "%H:%M:%S"
};

static const RinLocaleTimeNames rin_locale_time_pt = {
    {"dom", "seg", "ter", "qua", "qui", "sex", "sáb"},
    {"domingo", "segunda-feira", "terça-feira", "quarta-feira", "quinta-feira", "sexta-feira", "sábado"},
    {"jan", "fev", "mar", "abr", "mai", "jun", "jul", "ago", "set", "out", "nov", "dez"},
    {"janeiro", "fevereiro", "março", "abril", "maio", "junho", "julho", "agosto", "setembro", "outubro", "novembro", "dezembro"},
    {"AM", "PM"}, "%a %e de %b de %Y %H:%M:%S", "%d/%m/%Y", "%H:%M:%S"
};

static const RinLocaleTimeNames* rin_locale_time_names_l(locale_t locobj) {
    const RinLocaleObjectView* view;
    uint8_t kind;
    uint32_t index;
    if (locobj == LC_GLOBAL_LOCALE) return NULL;
    if (!locobj) return NULL;
    view = (const RinLocaleObjectView*)locobj;
    if (view->magic != UINT32_C(0x434f4c52) ||
        view->version != UINT32_C(0x00010000)) return NULL;
    for (index = 0u; index < sizeof(view->reserved); ++index)
        if (view->reserved[index] != 0u) return NULL;
    kind = view->categories[LC_TIME];
    if (kind < 1u || kind > 19u) return NULL;
    if (kind == 4u || kind == 5u) return &rin_locale_time_ja;
    if (kind == 6u || kind == 7u) return &rin_locale_time_de;
    if (kind == 8u || kind == 9u) return &rin_locale_time_fr;
    if (kind == 10u || kind == 11u) return &rin_locale_time_es;
    if (kind == 12u || kind == 13u) return &rin_locale_time_it;
    if (kind == 14u || kind == 15u) return &rin_locale_time_pt;
    if (kind == 16u || kind == 17u) return &rin_locale_time_es;
    if (kind == 18u || kind == 19u) return &rin_locale_time_pt;
    return &rin_locale_time_c;
}

const char* rin_locale_l_weekday(locale_t locobj, int weekday, int abbreviated) {
    const RinLocaleTimeNames* names = rin_locale_time_names_l(locobj);
    if (locobj == LC_GLOBAL_LOCALE) return rin_locale_weekday(weekday, abbreviated);
    if (!names || weekday < 0 || weekday >= 7) return NULL;
    return abbreviated ? names->short_weekdays[weekday] : names->long_weekdays[weekday];
}

const char* rin_locale_l_month(locale_t locobj, int month, int abbreviated) {
    const RinLocaleTimeNames* names = rin_locale_time_names_l(locobj);
    if (locobj == LC_GLOBAL_LOCALE) return rin_locale_month(month, abbreviated);
    if (!names || month < 0 || month >= 12) return NULL;
    return abbreviated ? names->short_months[month] : names->long_months[month];
}

const char* rin_locale_l_am_pm(locale_t locobj, int hour) {
    const RinLocaleTimeNames* names = rin_locale_time_names_l(locobj);
    if (locobj == LC_GLOBAL_LOCALE) return rin_locale_am_pm(hour);
    if (!names || hour < 0 || hour > 23) return NULL;
    return names->am_pm[hour >= 12 ? 1 : 0];
}

const char* rin_locale_l_time_format(locale_t locobj, char conversion) {
    const RinLocaleTimeNames* names = rin_locale_time_names_l(locobj);
    if (locobj == LC_GLOBAL_LOCALE) return rin_locale_time_format(conversion);
    if (!names || (conversion != 'c' && conversion != 'x' && conversion != 'X'))
        return NULL;
    if (conversion == 'c') return names->date_time_format;
    if (conversion == 'x') return names->date_format;
    return names->time_format;
}

int rin_isalnum(int c) { return c >= 0 ? rin_unicode_isalnum((uint32_t)c) : 0; }
int rin_isalpha(int c) { return c >= 0 ? rin_unicode_isalpha((uint32_t)c) : 0; }
int rin_isblank(int c) { return c >= 0 ? rin_unicode_isblank((uint32_t)c) : 0; }
int rin_iscntrl(int c) { return c >= 0 ? rin_unicode_iscntrl((uint32_t)c) : 0; }
int rin_isdigit(int c) { return c >= 0 ? rin_unicode_isdigit((uint32_t)c) : 0; }
int rin_isgraph(int c) { return c >= 0 ? rin_unicode_isgraph((uint32_t)c) : 0; }
int rin_islower(int c) { return c >= 0 ? rin_unicode_islower((uint32_t)c) : 0; }
int rin_isprint(int c) { return c >= 0 ? rin_unicode_isprint((uint32_t)c) : 0; }
int rin_ispunct(int c) { return c >= 0 ? rin_unicode_ispunct((uint32_t)c) : 0; }
int rin_isspace(int c) { return c >= 0 ? rin_unicode_isspace((uint32_t)c) : 0; }
int rin_isupper(int c) { return c >= 0 ? rin_unicode_isupper((uint32_t)c) : 0; }
int rin_isxdigit(int c) { return c >= 0 ? rin_unicode_isxdigit((uint32_t)c) : 0; }
int rin_tolower(int c) { return c >= 0 ? (int)rin_unicode_tolower((uint32_t)c) : c; }
int rin_toupper(int c) { return c >= 0 ? (int)rin_unicode_toupper((uint32_t)c) : c; }
int rin_strcoll(char const* s1, char const* s2) { return rin_unicode_strcoll(s1, s2); }
size_t rin_strxfrm(char* dest, char const* src, size_t n) { return rin_unicode_strxfrm(dest, src, n); }
