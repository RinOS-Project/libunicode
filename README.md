# RinUnicode public core

RinUnicode provides bounded Unicode scalar, UTF, grapheme, line-break,
normalization, collation, property, and IDNA operations for ordinary userspace
callers. The IDNA ToASCII entry point uses Unicode 17.0.0 UTS #46 mapping,
NFC, Punycode, STD3, DNS length, derived-property, bidi, ContextJ, and ContextO
checks. Its caller-owned workspace keeps memory use bounded and avoids global
mutable state. The standalone core target does not own locale files,
filesystem access, or service publication; the locale adapter remains part of
the RinOS libc/runtime owner graph.

The UTF-8 and UTF-32 normalization APIs use generated Unicode 17.0.0
decomposition, composition-exclusion, and canonical-combining-class data.
`rin_unicode_normalization_database_version()` reports that table version
separately from `rin_unicode_database_version()`, which describes the other
generated core property and case-fold tables.

The line-break adapter includes a bounded UAX #14 subset: hard breaks,
whitespace, BA/B2 (including the two-sided EM DASH case), the dictionary BB
stress-mark cases U+00B4/U+1FFD/U+02C8/U+02CC/U+02DF, the LB8a zero-width-
joiner no-break boundary, and CM-aware Regional Indicator flag runs with
even-length scalar-boundary handling, bounded ASCII `HY × NU` numeric protection,
bounded ASCII word-initial-hyphen protection, solidus URL/path breaks, common
punctuation, contingent inline-object breaks (U+FFFC), LB21 hyphen/BA
no-break-before handling, and ideographic text.
It is not a full LineBreak property database or locale-tailored line-breaking
implementation.
UAX #14 LB26 is covered for Hangul JL/JV/JT pieces and derived H2/H3
syllable classes; complete precomposed syllables can break from one another,
while Jamo composition boundaries remain prohibited. Full LineBreak property
data and locale tailoring remain outside this subset.

The CMake contract tests are enabled with
`-DRIN_UNICODE_BUILD_TESTS=ON`; Meson exposes the same four
`rinunicode-*` tests.  They compile the public core with strict C11 warnings
and exercise the caller-visible Unicode contracts.

The IDNA snapshot is regenerated with `python generate_idna_data.py` from the
pinned Unicode 17.0.0 archives in `idna-data/`. Normalization data is
regenerated with `python generate_normalization_data.py` from the same UCD
archive. See
`IDNA-DATA-LICENSE.md` for Unicode data terms and source links.
