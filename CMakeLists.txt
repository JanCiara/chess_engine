cmake_minimum_required(VERSION 4.0)
project(chess_engine)

set(CMAKE_CXX_STANDARD 26)

add_executable(chess_engine main.cpp
        defs.h
        board.h
        board.cpp
        movegen.h
        movegen.cpp
        evaluate.h
        evaluate.cpp
        search.h
        search.cpp)
