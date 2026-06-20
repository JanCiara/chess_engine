cmake_minimum_required(VERSION 3.20)
project(chess_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug, Release, RelWithDebInfo)" FORCE)
endif()

# Shared engine sources (everything except main/uci entry points)
set(ENGINE_CORE_SOURCES
    board.cpp
    movegen.cpp
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
target_compile_options(chess_engine PRIVATE ${ENGINE_RELEASE_FLAGS} ${ENGINE_DEBUG_FLAGS})

add_executable(perft_test tests/perft_test.cpp ${ENGINE_CORE_SOURCES})
target_include_directories(perft_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(perft_test PRIVATE ${ENGINE_RELEASE_FLAGS} ${ENGINE_DEBUG_FLAGS})

add_executable(bench_test tests/bench_test.cpp ${ENGINE_CORE_SOURCES})
target_include_directories(bench_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(bench_test PRIVATE ${ENGINE_RELEASE_FLAGS} ${ENGINE_DEBUG_FLAGS})

add_executable(wac_test tests/wac_test.cpp uci.cpp ${ENGINE_CORE_SOURCES})
target_include_directories(wac_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(wac_test PRIVATE ${ENGINE_RELEASE_FLAGS} ${ENGINE_DEBUG_FLAGS})
target_compile_definitions(wac_test PRIVATE WAC_EPD_PATH="${CMAKE_SOURCE_DIR}/tests/wac.epd")

add_executable(eval_test tests/eval_test.cpp ${ENGINE_CORE_SOURCES})
target_include_directories(eval_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(eval_test PRIVATE ${ENGINE_RELEASE_FLAGS} ${ENGINE_DEBUG_FLAGS})

enable_testing()
add_test(NAME perft COMMAND perft_test)
add_test(NAME bench COMMAND bench_test)
add_test(NAME wac COMMAND wac_test)
add_test(NAME eval COMMAND eval_test)
