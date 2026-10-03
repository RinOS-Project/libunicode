# RinUnicode public core

RinUnicode provides bounded, allocation-free Unicode scalar, UTF, grapheme,
line-break, normalization, collation, and property operations for ordinary
userspace callers.  The standalone core target does not own locale files,
filesystem access, or service publication; the locale adapter remains part of
the RinOS libc/runtime owner graph.

The CMake contract tests are enabled with
`-DRIN_UNICODE_BUILD_TESTS=ON`; Meson exposes the same four
`rinunicode-*` tests.  They compile the public core with strict C11 warnings
and exercise the caller-visible Unicode contracts.
