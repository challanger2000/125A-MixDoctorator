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

    add_executable(AnalysisSafetyTests
        tests/AnalysisSafetyTests.cpp)

    target_compile_features(
        AnalysisSafetyTests
        PRIVATE cxx_std_17)

    add_test(
        NAME AnalysisSafetyTests
        COMMAND AnalysisSafetyTests)

    add_executable(IPCConcurrencyTests
        tests/IPCConcurrencyTests.cpp)

    target_compile_features(
        IPCConcurrencyTests
        PRIVATE cxx_std_17)

    add_test(
        NAME IPCConcurrencyTests
        COMMAND IPCConcurrencyTests)

    add_executable(TestBuildModeTests
        tests/TestBuildModeTests.cpp)

    target_compile_features(
        TestBuildModeTests
        PRIVATE cxx_std_17)

    add_test(
        NAME TestBuildModeTests
        COMMAND TestBuildModeTests)

    add_executable(AudioSafetyTests
        tests/AudioSafetyTests.cpp)

    target_compile_features(
        AudioSafetyTests
        PRIVATE cxx_std_17)

    add_test(
        NAME AudioSafetyTests
        COMMAND AudioSafetyTests)

    add_executable(CoachModelTests
        tests/CoachModelTests.cpp)

    target_compile_features(
        CoachModelTests
        PRIVATE cxx_std_17)

    add_test(
        NAME CoachModelTests
        COMMAND CoachModelTests)


    add_executable(RoleModelTests
        tests/RoleModelTests.cpp)

    target_compile_features(
        RoleModelTests
        PRIVATE cxx_std_17)

    add_test(
        NAME RoleModelTests
        COMMAND RoleModelTests)

    add_executable(RecommendationEngineTests
        tests/RecommendationEngineTests.cpp)

    target_compile_features(
        RecommendationEngineTests
        PRIVATE cxx_std_17)

    add_test(
        NAME RecommendationEngineTests
        COMMAND RecommendationEngineTests)

    add_executable(RecommendationScenarioTests
        tests/RecommendationScenarioTests.cpp)

    target_compile_features(
        RecommendationScenarioTests
        PRIVATE cxx_std_17)

    add_test(
        NAME RecommendationScenarioTests
        COMMAND RecommendationScenarioTests)

    add_executable(RecommendationMatrixTests
        tests/RecommendationMatrixTests.cpp)

    target_compile_features(
        RecommendationMatrixTests
        PRIVATE cxx_std_17)

    add_test(
        NAME RecommendationMatrixTests
        COMMAND RecommendationMatrixTests)

    add_executable(SpscQueueTests
        tests/SpscQueueTests.cpp)

    target_compile_features(
        SpscQueueTests
        PRIVATE cxx_std_17)

    add_test(
        NAME SpscQueueTests
        COMMAND SpscQueueTests)

    # CI builds tests in Release. Keep assert()-based regression checks active;
    # otherwise NDEBUG would compile most existing test assertions away.
    set(MIXDOCTORATOR_TEST_TARGETS
        MaskingModelTests
        SpectralAnalyzerTests
        TimingModelTests
        TransientModelTests
        RoleAggregateTests
        IPCModelTests
        TransientInteractionTests
        AttackFindingTests
        TransportModelTests
        PrimaryFindingTests
        AnalysisSafetyTests
        IPCConcurrencyTests
        TestBuildModeTests
        AudioSafetyTests
        CoachModelTests
        RoleModelTests
        RecommendationEngineTests
        RecommendationScenarioTests
        RecommendationMatrixTests
        SpscQueueTests)

    foreach(test_target IN LISTS MIXDOCTORATOR_TEST_TARGETS)
        if(MSVC)
            target_compile_options(
                ${test_target}
                PRIVATE /UNDEBUG)
        else()
            target_compile_options(
                ${test_target}
                PRIVATE -UNDEBUG)
        endif()
    endforeach()
endif()
