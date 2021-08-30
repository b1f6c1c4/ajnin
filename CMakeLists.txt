# Copyright (C) 2021 b1f6c1c4
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

cmake_minimum_required(VERSION 3.17)
project(ajnin VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

configure_file(include/config.h.in include/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

include(CTest)
enable_testing()
add_subdirectory(tests)

# required if linking to static library
add_definitions(-DANTLR4CPP_STATIC)

# add external build for antlrcpp
include(ExternalAntlr4Cpp)
# add antrl4cpp artifacts to project environment
include_directories(${ANTLR4_INCLUDE_DIRS})

# set variable pointing to the antlr tool that supports C++
# this is not required if the jar file can be found under PATH environment
set(ANTLR_EXECUTABLE /usr/share/java/antlr-4.9.2-complete.jar CACHE STRING "Path of antlr-4.9.2-complete.jar")
# add macros to generate ANTLR Cpp code from grammar
find_package(ANTLR REQUIRED)

find_package(Boost 1.75.0 REQUIRED)

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

add_custom_target(link_target ALL COMMAND ${CMAKE_COMMAND} -E create_symlink ajnin an)

install(TARGETS ajnin DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/an DESTINATION bin)
