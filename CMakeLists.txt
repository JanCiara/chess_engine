cmake_minimum_required(VERSION 3.20)
project(chess_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug, Release, RelWithDebInfo)" FORCE)
endif()

option(ENABLE_LTO "Enable Link Time Optimization for Release builds" ON)

set(ENABLE_PGO "OFF" CACHE STRING "Profile-Guided Optimization: OFF, GENERATE, USE")
set_property(CACHE ENABLE_PGO PROPERTY STRINGS OFF GENERATE USE)

if(NOT ENABLE_PGO MATCHES "^(OFF|GENERATE|USE)$")
    message(FATAL_ERROR "ENABLE_PGO must be OFF, GENERATE, or USE (got \"${ENABLE_PGO}\")")
endif()

if(ENABLE_LTO)
    if(NOT ENABLE_PGO STREQUAL "OFF" AND MSVC)
        message(STATUS "LTO: using MSVC /LTCG via PGO (${ENABLE_PGO})")
    else()
        include(CheckIPOSupported)
        check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_error)
        if(_ipo_supported)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
            message(STATUS "LTO: enabled for Release and RelWithDebInfo")
        else()
            message(WARNING "LTO not supported by this toolchain: ${_ipo_error}")
        endif()
    endif()
endif()

set(ENGINE_PGO_COMPILE_FLAGS)
set(ENGINE_PGO_LINK_FLAGS)
if(NOT ENABLE_PGO STREQUAL "OFF")
    if(MSVC)
        list(APPEND ENGINE_PGO_COMPILE_FLAGS /GL)
        if(ENABLE_PGO STREQUAL "GENERATE")
            list(APPEND ENGINE_PGO_LINK_FLAGS /LTCG:PGINSTRUMENT)
        else()
            list(APPEND ENGINE_PGO_LINK_FLAGS /LTCG:PGOPTIMIZE)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if(ENABLE_PGO STREQUAL "GENERATE")
            list(APPEND ENGINE_PGO_COMPILE_FLAGS -fprofile-instr-generate)
            list(APPEND ENGINE_PGO_LINK_FLAGS -fprofile-instr-generate)
        else()
            set(_pgo_profdata "${CMAKE_BINARY_DIR}/default.profdata")
            if(NOT EXISTS "${_pgo_profdata}")
                message(WARNING "PGO USE: profile not found at ${_pgo_profdata}. "
                    "Run a GENERATE build, benchmark the engine, then merge profiles "
                    "(llvm-profdata merge -output=default.profdata *.profraw).")
            endif()
            list(APPEND ENGINE_PGO_COMPILE_FLAGS "-fprofile-instr-use=${_pgo_profdata}")
            list(APPEND ENGINE_PGO_LINK_FLAGS "-fprofile-instr-use=${_pgo_profdata}")
        endif()
    else()
        if(ENABLE_PGO STREQUAL "GENERATE")
            list(APPEND ENGINE_PGO_COMPILE_FLAGS -fprofile-generate)
            list(APPEND ENGINE_PGO_LINK_FLAGS -fprofile-generate)
        else()
            list(APPEND ENGINE_PGO_COMPILE_FLAGS -fprofile-use -fprofile-correction)
            list(APPEND ENGINE_PGO_LINK_FLAGS -fprofile-use)
        endif()
    endif()
    message(STATUS "PGO: ${ENABLE_PGO}")
endif()

set(ENGINE_PGO_COMPILE_OPTS
    $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:${ENGINE_PGO_COMPILE_FLAGS}>
)
set(ENGINE_PGO_LINK_OPTS
    $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:${ENGINE_PGO_LINK_FLAGS}>
)

# Shared engine sources (everything except main/uci entry points)
set(ENGINE_CORE_SOURCES
    board.cpp
    movegen.cpp
    attacks_data.cpp
    evaluate.cpp
    search.cpp
    tt.cpp
    draw.cpp
    bench.cpp
    epd.cpp
)

if(MSVC)
    set(ENGINE_RELEASE_FLAGS $<$<CONFIG:Release>:/O2> $<$<CONFIG:RelWithDebInfo>:/O2>)
    set(ENGINE_DEBUG_FLAGS)
else()
    set(ENGINE_RELEASE_FLAGS
        $<$<CONFIG:Release>:-O2 -march=native>
        $<$<CONFIG:RelWithDebInfo>:-O2 -march=native>
    )
    set(ENGINE_DEBUG_FLAGS $<$<CONFIG:Debug>:-g>)
endif()

add_executable(chess_engine
    main.cpp
    uci.cpp
    ${ENGINE_CORE_SOURCES}
)

target_include_directories(chess_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(chess_engine PRIVATE ${ENGINE_RELEASE_FLAGS} ${ENGINE_PGO_COMPILE_OPTS} ${ENGINE_DEBUG_FLAGS})
target_link_options(chess_engine PRIVATE ${ENGINE_PGO_LINK_OPTS})

add_executable(perft_test tests/perft_test.cpp ${ENGINE_CORE_SOURCES})
target_include_directories(perft_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(perft_test PRIVATE ${ENGINE_RELEASE_FLAGS} ${ENGINE_PGO_COMPILE_OPTS} ${ENGINE_DEBUG_FLAGS})
target_link_options(perft_test PRIVATE ${ENGINE_PGO_LINK_OPTS})

add_executable(bench_test tests/bench_test.cpp ${ENGINE_CORE_SOURCES})
target_include_directories(bench_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(bench_test PRIVATE ${ENGINE_RELEASE_FLAGS} ${ENGINE_PGO_COMPILE_OPTS} ${ENGINE_DEBUG_FLAGS})
target_link_options(bench_test PRIVATE ${ENGINE_PGO_LINK_OPTS})

add_executable(wac_test tests/wac_test.cpp uci.cpp ${ENGINE_CORE_SOURCES})
target_include_directories(wac_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(wac_test PRIVATE ${ENGINE_RELEASE_FLAGS} ${ENGINE_PGO_COMPILE_OPTS} ${ENGINE_DEBUG_FLAGS})
target_link_options(wac_test PRIVATE ${ENGINE_PGO_LINK_OPTS})
target_compile_definitions(wac_test PRIVATE WAC_EPD_PATH="${CMAKE_SOURCE_DIR}/tests/wac.epd")

add_executable(eval_test tests/eval_test.cpp ${ENGINE_CORE_SOURCES})
target_include_directories(eval_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(eval_test PRIVATE ${ENGINE_RELEASE_FLAGS} ${ENGINE_PGO_COMPILE_OPTS} ${ENGINE_DEBUG_FLAGS})
target_link_options(eval_test PRIVATE ${ENGINE_PGO_LINK_OPTS})

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(unit_test
    tests/unit_test.cpp
    board.cpp
    epd.cpp
    tt.cpp
    draw.cpp
)
target_include_directories(unit_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(unit_test PRIVATE GTest::gtest_main)
target_compile_options(unit_test PRIVATE ${ENGINE_DEBUG_FLAGS})

enable_testing()
add_test(NAME unit COMMAND unit_test)
add_test(NAME perft COMMAND perft_test)
add_test(NAME bench COMMAND bench_test)
add_test(NAME wac COMMAND wac_test)
add_test(NAME eval COMMAND eval_test)
