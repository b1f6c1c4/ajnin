# Copyright (C) 2021-2022 b1f6c1c4
#
# This file is part of ajnin.
#
# ajnin is free software: you can redistribute it and/or modify it under the
# terms of the GNU Affero General Public License as published by the Free
# Software Foundation, version 3.
#
# ajnin is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for
# more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with ajnin.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.16)
project(ajnin VERSION 0.5)

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

configure_file(include/config.h.in include/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

include(CTest)
enable_testing()
add_subdirectory(tests)

# required if linking to static library
add_definitions(-DANTLR4CPP_STATIC)

set(ANTLR4_ZIP_REPOSITORY https://github.com/antlr/antlr4/archive/refs/tags/4.11.1.zip)

# add external build for antlrcpp
include(ExternalAntlr4Cpp)
# add antrl4cpp artifacts to project environment
include_directories(${ANTLR4_INCLUDE_DIRS})

# set variable pointing to the antlr tool that supports C++
# this is not required if the jar file can be found under PATH environment
set(ANTLR_EXECUTABLE /usr/share/java/antlr-4.11.1-complete.jar CACHE STRING "Path of antlr-4.11.1-complete.jar")
# add macros to generate ANTLR Cpp code from grammar
find_package(ANTLR REQUIRED)

find_package(Boost 1.71.0 REQUIRED)

# Call macro to add lexer and grammar to your build dependencies.
antlr_target(TLexer TLexer.g4 LEXER
        PACKAGE parsing)
antlr_target(TParser TParser.g4 PARSER
        PACKAGE parsing
        DEPENDS_ANTLR TLexer
        COMPILE_FLAGS -lib ${ANTLR_TLexer_OUTPUT_DIR} -visitor)

# include generated files in project environment
include_directories(${ANTLR_TLexer_OUTPUT_DIR})
include_directories(${ANTLR_TParser_OUTPUT_DIR})
include_directories(${ANTLR_TVisitor_OUTPUT_DIR})

include_directories(include)

add_executable(ajnin main.cpp
        manager/aux.cpp
        manager/io.cpp
        manager/non-build.cpp
        manager/build.cpp
        ${ANTLR_TLexer_CXX_OUTPUTS}
        ${ANTLR_TParser_CXX_OUTPUTS})
target_link_libraries(ajnin antlr4_static)
target_link_libraries(ajnin boost_regex)

add_custom_target(link_target_an ALL COMMAND ${CMAKE_COMMAND} -E create_symlink ajnin an)
add_custom_target(link_target_sanity ALL COMMAND ${CMAKE_COMMAND} -E create_symlink ajnin sanity)

if(NOT EXISTS ${PANDOC_EXECUTABLE})
    find_program(PANDOC_EXECUTABLE pandoc)
    mark_as_advanced(PANDOC_EXECUTABLE)
    if(NOT EXISTS ${PANDOC_EXECUTABLE})
        message(FATAL_ERROR "Pandoc not found. Install Pandoc or set cache variable PANDOC_EXECUTABLE.")
    endif()
endif()
add_custom_command(OUTPUT ajnin.1
        COMMAND pandoc ARGS -s -t man -o ajnin.1 ${CMAKE_CURRENT_SOURCE_DIR}/man/ajnin.md
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/man/ajnin.md
        COMMENT "Using pandoc to generate man page: ajnin.1")
add_custom_command(OUTPUT an.1
        COMMAND pandoc ARGS -s -t man -o an.1 ${CMAKE_CURRENT_SOURCE_DIR}/man/an.md
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/man/an.md
        COMMENT "Using pandoc to generate man page: an.1")
add_custom_command(OUTPUT sanity.1
        COMMAND pandoc ARGS -s -t man -o sanity.1 ${CMAKE_CURRENT_SOURCE_DIR}/man/sanity.md
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/man/sanity.md
        COMMENT "Using pandoc to generate man page: sanity.1")
add_custom_target(manual DEPENDS ajnin.1 an.1 sanity.1)

install(TARGETS ajnin DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/an DESTINATION bin/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sanity DESTINATION bin/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ajnin.1 DESTINATION share/man/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/an.1 DESTINATION share/man/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sanity.1 DESTINATION share/man/)
install(FILES indent/ajnin.vim DESTINATION share/vim/vimfiles/indent/)
install(FILES syntax/ajnin.vim DESTINATION share/vim/vimfiles/syntax/)
install(FILES LICENSE DESTINATION share/licenses/ajnin/)

if(DEFINED ENV{COVERALLS_REPO_TOKEN})
    include(Coveralls)
    target_compile_options(ajnin PRIVATE -fprofile-arcs -ftest-coverage)
    target_link_libraries(ajnin gcov)
    target_link_options(ajnin PRIVATE --coverage)
    set(COVERAGE_SRCS
        main.cpp 
        manager/aux.cpp
        manager/build.cpp
        manager/io.cpp
        manager/non-build.cpp
        include/filter.hpp
        include/manager.hpp)
    coveralls_setup("${COVERAGE_SRCS}" ON)
endif()
