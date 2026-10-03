project('RinUnicode', 'c', 'cpp', version: '1.0.0',
        default_options: ['c_std=c11', 'cpp_std=c++17', 'warning_level=3'])

unicode_inc = include_directories('.')
unicode_core = library('rinunicode-core', ['core.c', 'idna.cpp'],
                       include_directories: unicode_inc,
                       install: false)
unicode_core_dep = declare_dependency(link_with: unicode_core,
                                      include_directories: unicode_inc)

unicode_contract_tests = [
  ['collation', 'tests/collation_contract_test.c'],
  ['grapheme-property', 'tests/grapheme_property_contract_test.c'],
  ['line-break', 'tests/line_break_contract_test.c'],
  ['normalization-hangul', 'tests/normalization_hangul_contract_test.c'],
]
foreach contract : unicode_contract_tests
  test('rinunicode-' + contract[0],
       executable('rinunicode-' + contract[0] + '-test', contract[1],
                  dependencies: unicode_core_dep,
                  c_args: ['-Wall', '-Wextra', '-Wpedantic']))
endforeach
