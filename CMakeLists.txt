cmake_minimum_required(VERSION 3.22...3.29)

project(
    gyou
  VERSION 0.0.0.2
  LANGUAGES CXX)

option(${PROJECT_NAME}_ENABLE_WARNINGS "Enable warnings" YES)
option(${PROJECT_NAME}_TESTS "Configure and build tests" YES)
# option(${PROJECT_NAME}_BUILD_FUZZER "Build fuzzer" YES)
option(BUILD_SHARED_LIBS "yes/no" NO)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

file(GLOB_RECURSE headers CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

add_library(${PROJECT_NAME}_lib ${headers} ${sources})

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_lib)

target_compile_definitions(${PROJECT_NAME}_lib
    PUBLIC BOOST_STACKTRACE_USE_BACKTRACE=1
    PUBLIC BOOST_ASIO_HAS_FILE=1
    PUBLIC BOOST_ASIO_HAS_IO_URING=1
    PUBLIC BOOST_PROCESS_V2_DISABLE_NOTIFY_FORK=1
    PUBLIC BOOST_PROCESS_USE_STD_FS=1
)

target_include_directories(
  ${PROJECT_NAME}_lib
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)


# target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address,leak)
# target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=address,leak)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/packages/addfmt.cmake)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC PkgConfig::fmt)
find_package(Boost REQUIRED COMPONENTS
            stacktrace_from_exception
            process
            url
    )
target_link_libraries(${PROJECT_NAME}_lib PUBLIC Boost::boost)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC Boost::process)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC Boost::url)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC Boost::stacktrace_from_exception)
# just add -lbacktrace
target_link_libraries(${PROJECT_NAME}_lib PUBLIC backtrace)
find_package(corral REQUIRED)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC corral::corral)
find_package(magic_enum REQUIRED)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC magic_enum::magic_enum)
find_package(glaze REQUIRED)
target_link_libraries(${PROJECT_NAME}_lib PRIVATE glaze::glaze)
find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC openssl::openssl)
find_package(CLI11 REQUIRED)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC CLI11::CLI11)
find_package(re2 REQUIRED)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC re2::re2)
find_package(liburing REQUIRED)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC liburing::liburing)
find_package(Reflex REQUIRED)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC Reflex::ReflexLib)
find_package(PCRE2 REQUIRED)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC pcre2::pcre2)

find_package(quill)
if(TARGET quill::quill)
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC quill::quill)
endif()
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs/quill_usage)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC quill_usage::quill_usage)

# find_package(constexpr-to-string REQUIRED)
# target_link_libraries(${PROJECT_NAME}_lib
#                       PUBLIC constexpr-to-string::constexpr-to-string)


include(${CMAKE_CURRENT_LIST_DIR}/cmake/comp_opts.cmake)
set_common_compile_options(${PROJECT_NAME}_lib)
set_common_compile_options(${PROJECT_NAME})

if(${${PROJECT_NAME}_TESTS})
    option(CTEST_OUTPUT_ON_FAILURE "Print failed assertions for failed tests" TRUE)
    enable_testing()

    file(GLOB_RECURSE unit_tests_sources CONFIGURE_DEPENDS
         "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit_tests/*.cpp")
    add_executable(${PROJECT_NAME}_unit_tests ${unit_tests_sources})
    target_link_libraries(${PROJECT_NAME}_unit_tests PRIVATE ${PROJECT_NAME}_lib)
    set_common_compile_options(${PROJECT_NAME}_unit_tests)

    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME}_unit_tests PUBLIC GTest::GTest)
    target_link_libraries(${PROJECT_NAME}_unit_tests PUBLIC GTest::gtest_main)
    target_link_libraries(${PROJECT_NAME}_unit_tests PUBLIC GTest::gmock)
    # add_test(NAME ${PROJECT_NAME}_lib COMMAND ${PROJECT_NAME})
    include(GoogleTest)
    gtest_discover_tests(${PROJECT_NAME}_unit_tests EXTRA_ARGS "--gtest_color=yes")
endif()


# ---- Create an installable target ----

string(TOLOWER ${PROJECT_NAME}_lib/version.h VERSION_HEADER_LOCATION)

# set_property(TARGET ${PROJECT_NAME}_lib PROPERTY VERSION ${PROJECT_VERSION})
# set_property(TARGET ${PROJECT_NAME}_lib PROPERTY SOVERSION 1)

install(TARGETS ${PROJECT_NAME})
