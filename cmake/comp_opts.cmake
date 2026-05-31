macro(set_common_compile_options target_name)
    # https://youtu.be/Nm9-xKsZoNI?t=912
    target_compile_options(${target_name}
                       PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")


    if(${${PROJECT_NAME}_ENABLE_WARNINGS})
        include(${CMAKE_CURRENT_LIST_DIR}/cmake/warnings.cmake)
        set_project_warnings(${target_name})
    endif()


    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target_name}
        PUBLIC -fconcepts-diagnostics-depth=100)
    endif()

    if(ENABLE_TEST_COVERAGE)
        target_compile_options(${target_naem} PUBLIC -O0 -g -fprofile-arcs
                                                -ftest-coverage)
        target_link_options(${target_name} PUBLIC -fprofile-arcs -ftest-coverage)
    endif()

endmacro()
