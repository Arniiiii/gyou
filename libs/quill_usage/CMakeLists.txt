project(quill_usage VERSION 1.0 LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
        overwrite_log_macros.h
        quill_static.h
        quill_static.cpp)

# Add include directories for this library
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/comp_opts.cmake)
set_common_compile_options(${PROJECT_NAME})

# Link quill dependency
target_link_libraries(${PROJECT_NAME} PUBLIC quill::quill)


add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
