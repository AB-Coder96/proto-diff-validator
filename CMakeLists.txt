cmake_minimum_required(VERSION 3.20)

project(
    proto_diff_validator
    VERSION 0.1.0
    DESCRIPTION "C++20 toolkit for diffing and validating order-entry protocol upgrades"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_WARNINGS "Enable compiler warnings" ON)
option(BUILD_TESTS "Build unit tests" ON)

if(ENABLE_WARNINGS)
    if(MSVC)
        add_compile_options(/W4 /permissive-)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic)
    endif()
endif()

find_package(yaml-cpp REQUIRED)

add_library(vendor_schema
    src/vendor_schema_loader.cpp
    src/diff.cpp
    src/report.cpp

)

target_include_directories(vendor_schema
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(vendor_schema
    PUBLIC
        yaml-cpp
)

target_compile_features(vendor_schema
    PUBLIC
        cxx_std_20
)

if(BUILD_TESTS)
    enable_testing()

    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_executable(vendor_schema_loader_test
        tests/vendor_schema_loader_test.cpp
    )

    target_link_libraries(vendor_schema_loader_test
        PRIVATE
            vendor_schema
            GTest::gtest_main
    )

    target_compile_features(vendor_schema_loader_test
        PRIVATE
            cxx_std_20
    )

    gtest_discover_tests(vendor_schema_loader_test)
    add_executable(diff_test
    tests/diff_test.cpp
    )

    target_link_libraries(diff_test
        PRIVATE
            vendor_schema
            GTest::gtest_main
    )

    target_compile_features(diff_test
        PRIVATE
            cxx_std_20
    )

    gtest_discover_tests(diff_test)
    add_executable(report_test
    tests/report_test.cpp
    )

    target_link_libraries(report_test
        PRIVATE
            vendor_schema
            GTest::gtest_main
    )

    target_compile_features(report_test
        PRIVATE
            cxx_std_20
    )

    gtest_discover_tests(report_test)
endif()

