cmake_minimum_required(VERSION 3.16)

project(CalcInterpreter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(calc_app
    main.cpp

    App.cpp
    Parser.cpp
    Evaluator.cpp
    AST.cpp

    App.h
    Parser.h
    Evaluator.h
    AST.h
    Error.h
    Lexer.h
    Token.h
)

target_include_directories(calc_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (MSVC)
    target_compile_options(calc_app PRIVATE /W4)
else()
    target_compile_options(calc_app PRIVATE -Wall -Wextra -pedantic)
endif()