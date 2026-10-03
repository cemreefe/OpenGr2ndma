cmake_minimum_required(VERSION 3.13)
project(OpenGr2ndma VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(OPENGR2NDMA_BUILD_TOOLS "Build the gr2info tool" ${PROJECT_IS_TOP_LEVEL})
option(OPENGR2NDMA_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})

add_library(opengr2ndma STATIC src/runtime.cpp src/oodle1.c)
add_library(OpenGr2ndma::opengr2ndma ALIAS opengr2ndma)
target_include_directories(opengr2ndma PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
set_target_properties(opengr2ndma PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(OPENGR2NDMA_BUILD_TOOLS)
    add_executable(gr2info tools/gr2info.cpp)
    target_link_libraries(gr2info PRIVATE opengr2ndma)
    add_executable(gr2render tools/gr2render.cpp)
    target_link_libraries(gr2render PRIVATE opengr2ndma)
endif()

if(OPENGR2NDMA_BUILD_TESTS)
    enable_testing()
    add_executable(opengr2ndma_tests tests/api_test.cpp)
    target_link_libraries(opengr2ndma_tests PRIVATE opengr2ndma)
    add_test(NAME api COMMAND opengr2ndma_tests)
    if(OPENGR2NDMA_BUILD_TOOLS)
        add_test(NAME example COMMAND gr2render ${CMAKE_CURRENT_SOURCE_DIR}/examples/yarn_worm.gr2
                 ${CMAKE_CURRENT_BINARY_DIR}/yarn_worm_ 4 64)
    endif()
    if(DEFINED OPENGR2NDMA_SAMPLE_DIR)
        add_test(NAME samples COMMAND gr2info --check ${OPENGR2NDMA_SAMPLE_DIR})
    endif()
endif()

install(TARGETS opengr2ndma EXPORT OpenGr2ndmaTargets ARCHIVE DESTINATION lib)
install(FILES include/granny.h DESTINATION include)
install(EXPORT OpenGr2ndmaTargets NAMESPACE OpenGr2ndma:: DESTINATION lib/cmake/OpenGr2ndma)
