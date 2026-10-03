# RinUnicode public core

RinUnicode provides bounded, allocation-free Unicode scalar, UTF, grapheme,
line-break, normalization, collation, and property operations for ordinary
userspace callers.  The standalone core target does not own locale files,
filesystem access, or service publication; the locale adapter remains part of
the RinOS libc/runtime owner graph.

The line-break adapter includes a bounded UAX #14 subset: hard breaks,
whitespace, BA/B2, the dictionary BB stress-mark cases U+00B4/U+1FFD/U+02C8/
U+02CC/U+02DF, the LB8a zero-width-joiner no-break boundary, and CM-aware
Regional Indicator flag runs, bounded ASCII `HY × NU` numeric protection,
solidus URL/path breaks, common punctuation, and ideographic text.
It is not a full LineBreak property database or locale-tailored line-breaking
implementation.

The CMake contract tests are enabled with
`-DRIN_UNICODE_BUILD_TESTS=ON`; Meson exposes the same four
`rinunicode-*` tests.  They compile the public core with strict C11 warnings
and exercise the caller-visible Unicode contracts.
