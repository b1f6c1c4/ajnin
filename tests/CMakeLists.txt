add_test(NAME help COMMAND ajnin --help)
set_tests_properties(help PROPERTIES PASS_REGULAR_EXPRESSION "GNU Affero")

file(GLOB tests "*.ajnin")
foreach(src ${tests})
    get_filename_component(T ${src} NAME_WLE)
    add_test(NAME ${T}:exe COMMAND ajnin --bare ${src} -o ${T}.ninja)
    add_test(NAME ${T}:cmp COMMAND ${CMAKE_COMMAND} -E compare_files ${T}.ninja ${src})
endforeach()
