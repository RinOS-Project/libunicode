#ifndef RIN_UNICODE_H
#define RIN_UNICODE_H

/* LibUnicode is shared by the hosted tools and RinOS libc.  Do not pull a
 * second platform libc's fundamental typedefs into a translation unit that
 * has already selected RinOS headers (notably on LLP64 Windows hosts). */
#ifndef _STDDEF_H
#include <stddef.h>
#endif
#ifndef _STDINT_H
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rin_unicode_mbstate {
    uint32_t state;
    uint32_t codepoint;
} rin_unicode_mbstate_t;

typedef struct rin_unicode_lconv {
    char* decimal_point;
    char* thousands_sep;
    char* grouping;
    char* int_curr_symbol;
    char* currency_symbol;
    char* mon_decimal_point;
    char* mon_thousands_sep;
    char* mon_grouping;
    char* positive_sign;
    char* negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
    char int_p_cs_precedes;
    char int_p_sep_by_space;
    char int_n_cs_precedes;
    char int_n_sep_by_space;
    char int_p_sign_posn;
    char int_n_sign_posn;
} rin_unicode_lconv_t;

struct lconv;

enum {
    RIN_UNICODE_NORMALIZE_NFD = 0,
    RIN_UNICODE_NORMALIZE_NFC = 1,
    RIN_UNICODE_NORMALIZE_NFKD = 2,
    RIN_UNICODE_NORMALIZE_NFKC = 3
};

enum {
    RIN_UNICODE_OK = 0,
    RIN_UNICODE_INVALID = -1,
    RIN_UNICODE_INCOMPLETE = -2,
    RIN_UNICODE_NO_SPACE = -3
};

enum {
    RIN_UNICODE_LINE_BREAK_PROHIBITED = 0,
    RIN_UNICODE_LINE_BREAK_ALLOWED = 1,
    RIN_UNICODE_LINE_BREAK_MANDATORY = 2
};

/* Maximum bytes accepted by NUL-terminated locale-name entry points. */
#define RIN_UNICODE_MAX_LOCALE_NAME_BYTES 128u
/* Maximum sizes accepted by NUL-terminated text conversion entry points. */
#define RIN_UNICODE_MAX_CSTRING_BYTES (4u * 1024u * 1024u)
#define RIN_UNICODE_MAX_WSTRING_ELEMENTS (4u * 1024u * 1024u)

/* Return the Unicode data snapshot used by every generated property table in
 * this library.  The returned pointer is static storage owned by LibUnicode;
 * callers must not modify or retain it beyond the process lifetime. */
const char* rin_unicode_database_version(void);

int rin_unicode_is_valid_scalar(uint32_t cp);
int rin_unicode_validate_utf8(const char* s, size_t n, size_t* valid_prefix);

int rin_unicode_decode_utf8(const char* s, size_t n, uint32_t* out_cp, size_t* out_len);
int rin_unicode_encode_utf8(char* dest, size_t n, uint32_t cp, size_t* out_len);
/* Decode one scalar while replacing an ill-formed maximal subpart with
 * U+FFFD.  Unlike rin_unicode_decode_utf8(), this always consumes input when
 * n is non-zero, which makes it suitable for renderers and editors. */
int rin_unicode_decode_utf8_lossy(const char* s, size_t n,
                                  uint32_t* out_cp, size_t* out_len);
int rin_unicode_decode_utf16(const uint16_t* s, size_t n, uint32_t* out_cp, size_t* out_len);
int rin_unicode_encode_utf16(uint16_t* dest, size_t n, uint32_t cp, size_t* out_len);

/* Terminal/UI cell width and extended-grapheme helpers.  Cell width is 0 fo
 * combining/control scalars, 2 for East Asian wide/full-width scalars, and 1
 * otherwise.  Boundary functions return byte offsets into UTF-8 text. */
int rin_unicode_is_combining(uint32_t cp);
int rin_unicode_cell_width(uint32_t cp);
size_t rin_unicode_grapheme_next(const char* s, size_t n, size_t offset);
size_t rin_unicode_grapheme_prev(const char* s, size_t n, size_t offset);

/* Conservative, allocation-free line-break boundaries for UTF-8 text.  The
 * opportunity is queried at a grapheme boundary and describes the break
 * before the scalar at offset.  It covers hard breaks, whitespace, soft
 * hyphens, common punctuation, and ideographic text.  It is deliberately not
 * a complete UAX #14 property database; callers needing the full rule set
 * must keep that policy above this API. */
int rin_unicode_line_break_opportunity(const char* s, size_t n, size_t offset);
/* Return the first allowed or mandatory break after offset, or n when none is
 * available.  offset is normally a grapheme boundary. */
size_t rin_unicode_line_break_next(const char* s, size_t n, size_t offset);

int rin_unicode_isalnum(uint32_t cp);
int rin_unicode_isalpha(uint32_t cp);
int rin_unicode_isblank(uint32_t cp);
int rin_unicode_iscntrl(uint32_t cp);
int rin_unicode_isdigit(uint32_t cp);
int rin_unicode_isgraph(uint32_t cp);
int rin_unicode_islower(uint32_t cp);
int rin_unicode_isprint(uint32_t cp);
int rin_unicode_ispunct(uint32_t cp);
int rin_unicode_isspace(uint32_t cp);
int rin_unicode_isupper(uint32_t cp);
int rin_unicode_isxdigit(uint32_t cp);
uint32_t rin_unicode_tolower(uint32_t cp);
uint32_t rin_unicode_toupper(uint32_t cp);
size_t rin_unicode_casefold_full(uint32_t cp, uint32_t out[3]);

/* Return the required output length.  A null source is treated as empty;
 * malformed input or an unsupported form returns (size_t)-1 and clears the
 * first destination element when a destination is supplied. */
size_t rin_unicode_normalize_utf32(uint32_t* dest, size_t dest_cap, const uint32_t* src, size_t src_len, int form);
size_t rin_unicode_normalize_utf8(char* dest, size_t dest_cap, const char* src, int form);
size_t rin_unicode_transform_utf32(uint32_t* dest, size_t dest_cap, const uint32_t* src);
size_t rin_unicode_transform_utf8(char* dest, size_t dest_cap, const char* src);
int rin_unicode_compare_utf32(const uint32_t* lhs, const uint32_t* rhs);
int rin_unicode_compare_utf8(const char* lhs, const char* rhs);

int rin_unicode_mbtowc32(uint32_t* out_wc, const char* s, size_t n);
int rin_unicode_wctomb32(char* dest, uint32_t wc);
size_t rin_unicode_mbstowcs32(uint32_t* dest, const char* src, size_t n);
size_t rin_unicode_wcstombs32(char* dest, const uint32_t* src, size_t n);
int rin_unicode_mbsinit(const rin_unicode_mbstate_t* ps);
size_t rin_unicode_mbrlen(const char* s, size_t n, rin_unicode_mbstate_t* ps);
size_t rin_unicode_mbrtowc32(uint32_t* out_wc, const char* s, size_t n, rin_unicode_mbstate_t* ps);
size_t rin_unicode_wcrtomb32(char* dest, uint32_t wc, rin_unicode_mbstate_t* ps);
size_t rin_unicode_mbsrtowcs32(uint32_t* dest, const char** src, size_t len, rin_unicode_mbstate_t* ps);
size_t rin_unicode_wcsrtombs32(char* dest, const uint32_t** src, size_t len, rin_unicode_mbstate_t* ps);

unsigned long rin_unicode_wctype(const char* property);
int rin_unicode_iswctype(uint32_t cp, unsigned long desc);
unsigned long rin_unicode_wctrans(const char* property);
uint32_t rin_unicode_towctrans(uint32_t cp, unsigned long desc);

char* rin_unicode_setlocale(int category, const char* locale);
rin_unicode_lconv_t* rin_unicode_localeconv(void);
const char* rin_unicode_locale_name(int category);
const char* rin_unicode_locale_environment_value(int category);
const char* rin_unicode_locale_language(void);
const char* rin_unicode_locale_territory(void);
const char* rin_unicode_locale_codeset(void);
int rin_unicode_locale_is_japanese(void);
int rin_unicode_locale_is_utf8(void);
const char* rin_unicode_locale_weekday(int weekday, int abbreviated);
const char* rin_unicode_locale_month(int month, int abbreviated);
const char* rin_unicode_locale_am_pm(int hour);
const char* rin_unicode_locale_time_format(char conversion);
int rin_unicode_strcoll(const char* s1, const char* s2);
size_t rin_unicode_strxfrm(char* dest, const char* src, size_t n);
int rin_unicode_wcscoll32(const uint32_t* s1, const uint32_t* s2);
size_t rin_unicode_wcsxfrm32(uint32_t* dest, const uint32_t* src, size_t n);

char* rin_setlocale(int category, const char* locale);
struct lconv* rin_localeconv(void);
const char* rin_locale_name(int category);
const char* rin_locale_environment_name(int category);
const char* rin_locale_language(void);
const char* rin_locale_territory(void);
const char* rin_locale_codeset(void);
int rin_locale_is_japanese(void);
int rin_locale_is_utf8(void);
const char* rin_locale_weekday(int weekday, int abbreviated);
const char* rin_locale_month(int month, int abbreviated);
const char* rin_locale_am_pm(int hour);
const char* rin_locale_time_format(char conversion);
int rin_isalnum(int c);
int rin_isalpha(int c);
int rin_isblank(int c);
int rin_iscntrl(int c);
int rin_isdigit(int c);
int rin_isgraph(int c);
int rin_islower(int c);
int rin_isprint(int c);
int rin_ispunct(int c);
int rin_isspace(int c);
int rin_isupper(int c);
int rin_isxdigit(int c);
int rin_tolower(int c);
int rin_toupper(int c);
int rin_strcoll(const char* s1, const char* s2);
size_t rin_strxfrm(char* dest, const char* src, size_t n);

#ifdef __cplusplus
}
#endif

#endif
