project(quill_usage VERSION 1.0 LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
        include/quill_usage/global_logger_ptr.hpp
        include/quill_usage/overwrite_log_macros.hpp
        include/quill_usage/quill_setup.hpp
        src/quill_usage/quill_setup.cpp)

# Add include directories for this library
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/comp_opts.cmake)
set_common_compile_options(${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME}
    PUBLIC QUILL_DISABLE_NON_PREFIXED_MACROS=1
)

# Link quill dependency
target_link_libraries(${PROJECT_NAME} PUBLIC quill::quill)


add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
