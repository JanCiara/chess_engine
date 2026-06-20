cmake_minimum_required(VERSION 3.20)
project(chess_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug, Release, RelWithDebInfo)" FORCE)
endif()

add_executable(chess_engine
    main.cpp
    board.cpp
    movegen.cpp
    evaluate.cpp
    search.cpp
    tt.cpp
    draw.cpp
    uci.cpp
)

target_include_directories(chess_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(chess_engine PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/O2>
    )
else()
    target_compile_options(chess_engine PRIVATE
        $<$<CONFIG:Release>:-O2 -march=native>
        $<$<CONFIG:RelWithDebInfo>:-O2 -march=native>
        $<$<CONFIG:Debug>:-g>
    )
endif()
