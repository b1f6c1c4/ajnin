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

add_test(NAME help COMMAND ajnin --help)
set_tests_properties(help PROPERTIES PASS_REGULAR_EXPRESSION "GNU Affero")

file(GLOB tests "*.ninja")
foreach(src ${tests})
    get_filename_component(T ${src} NAME_WLE)
    add_test(NAME ${T}:exe WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND ajnin --bare ${T}.ajnin -o ${CMAKE_CURRENT_BINARY_DIR}/${T}.ninja)
    add_test(NAME ${T}:cmp COMMAND ${CMAKE_COMMAND} -E compare_files
            ${CMAKE_CURRENT_SOURCE_DIR}/${T}.ninja ${CMAKE_CURRENT_BINARY_DIR}/${T}.ninja)
endforeach()
