cmake_minimum_required(VERSION 3.16)
project(RinUnicode VERSION 1.0.0 LANGUAGES C CXX)

option(RIN_UNICODE_BUILD_TESTS
       "Build RinUnicode public core contract tests"
       OFF)

add_library(RinUnicodeCore STATIC core.c idna.cpp)
add_library(RinUnicode::Core ALIAS RinUnicodeCore)
target_compile_features(RinUnicodeCore PUBLIC c_std_11)
target_compile_features(RinUnicodeCore PUBLIC cxx_std_17)
target_include_directories(RinUnicodeCore PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR})
if(NOT MSVC)
    target_compile_options(RinUnicodeCore PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(RIN_UNICODE_BUILD_TESTS)
    enable_testing()
    set(RIN_UNICODE_CONTRACT_TESTS
        collation_contract
        grapheme_property_contract
        line_break_contract
        normalization_hangul_contract)
    foreach(test_name IN LISTS RIN_UNICODE_CONTRACT_TESTS)
        add_executable(rinunicode-${test_name}-test
                       tests/${test_name}_test.c)
        target_link_libraries(rinunicode-${test_name}-test
                              PRIVATE RinUnicode::Core)
        target_compile_features(rinunicode-${test_name}-test PRIVATE c_std_11)
        if(NOT MSVC)
            target_compile_options(rinunicode-${test_name}-test PRIVATE
                                   -Wall -Wextra -Wpedantic)
        endif()
        add_test(NAME rinunicode-${test_name}
                 COMMAND rinunicode-${test_name}-test)
    endforeach()
endif()
