cmake_minimum_required(VERSION 3.25)
project(MixDoctorator VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
option(MIXDOCTORATOR_BUILD_VALIDATOR "Build Steinberg VST3 validator" OFF)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES ${MIXDOCTORATOR_BUILD_VALIDATOR} CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "" FORCE)
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG v3.8.1_build_84
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(vst3sdk)
add_subdirectory(src)


option(MIXDOCTORATOR_BUILD_TESTS "Build lightweight analysis tests" OFF)

if(MIXDOCTORATOR_BUILD_TESTS)
    enable_testing()

    add_executable(MaskingModelTests
        tests/MaskingModelTests.cpp)

    target_compile_features(
        MaskingModelTests
        PRIVATE cxx_std_17)

    add_test(
        NAME MaskingModelTests
        COMMAND MaskingModelTests)

    add_executable(SpectralAnalyzerTests
        tests/SpectralAnalyzerTests.cpp)

    target_compile_features(
        SpectralAnalyzerTests
        PRIVATE cxx_std_17)

    add_test(
        NAME SpectralAnalyzerTests
        COMMAND SpectralAnalyzerTests)

    add_executable(TimingModelTests
        tests/TimingModelTests.cpp)

    target_compile_features(
        TimingModelTests
        PRIVATE cxx_std_17)

    add_test(
        NAME TimingModelTests
        COMMAND TimingModelTests)

    add_executable(TransientModelTests
        tests/TransientModelTests.cpp)

    target_compile_features(
        TransientModelTests
        PRIVATE cxx_std_17)

    add_test(
        NAME TransientModelTests
        COMMAND TransientModelTests)

    add_executable(RoleAggregateTests
        tests/RoleAggregateTests.cpp)

    target_compile_features(
        RoleAggregateTests
        PRIVATE cxx_std_17)

    add_test(
        NAME RoleAggregateTests
        COMMAND RoleAggregateTests)

    add_executable(IPCModelTests
        tests/IPCModelTests.cpp)

    target_compile_features(
        IPCModelTests
        PRIVATE cxx_std_17)

    add_test(
        NAME IPCModelTests
        COMMAND IPCModelTests)

    add_executable(TransientInteractionTests
        tests/TransientInteractionTests.cpp)

    target_compile_features(
        TransientInteractionTests
        PRIVATE cxx_std_17)

    add_test(
        NAME TransientInteractionTests
        COMMAND TransientInteractionTests)

    add_executable(AttackFindingTests
        tests/AttackFindingTests.cpp)

    target_compile_features(
        AttackFindingTests
        PRIVATE cxx_std_17)

    add_test(
        NAME AttackFindingTests
        COMMAND AttackFindingTests)

    add_executable(TransportModelTests
        tests/TransportModelTests.cpp)

    target_compile_features(
        TransportModelTests
        PRIVATE cxx_std_17)

    add_test(
        NAME TransportModelTests
        COMMAND TransportModelTests)

    add_executable(PrimaryFindingTests
        tests/PrimaryFindingTests.cpp)

    target_compile_features(
        PrimaryFindingTests
        PRIVATE cxx_std_17)

    add_test(
        NAME PrimaryFindingTests
        COMMAND PrimaryFindingTests)
endif()
