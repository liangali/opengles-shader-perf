cmake_minimum_required(VERSION 3.15)
project(shader_perf_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED ENV{ANGLE_DIR})
    message(FATAL_ERROR "Please set ANGLE_DIR environment variable to your ANGLE SDK directory")
endif()
set(ANGLE_DIR $ENV{ANGLE_DIR})

if(NOT DEFINED ENV{EGL_DIR})
    message(FATAL_ERROR "Please set EGL_DIR environment variable to your ANGLE SDK directory")
endif()
set(EGL_DIR $ENV{EGL_DIR})

if(NOT DEFINED ENV{OGL_DIR})
    message(FATAL_ERROR "Please set OGL_DIR environment variable to your ANGLE SDK directory")
endif()
set(OGL_DIR $ENV{OGL_DIR})

if(NOT DEFINED ENV{ZLIB_DIR})
    message(FATAL_ERROR "Please set ZLIB_DIR environment variable to your ANGLE SDK directory")
endif()
set(ZLIB_DIR $ENV{ZLIB_DIR})

include_directories(${ANGLE_DIR}/include)
include_directories(${EGL_DIR}/include)
include_directories(${OGL_DIR}/include)

link_directories(${ANGLE_DIR}/lib)

add_executable(shader_perf_test 
    main.cpp
    texture_utils.cpp
)

target_link_libraries(shader_perf_test
    ${ANGLE_DIR}/lib/libEGL.lib
    ${ANGLE_DIR}/lib/libGLESv2.lib
    dxgi.lib
)

add_custom_command(TARGET shader_perf_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ANGLE_DIR}/bin/libEGL.dll"
        "${ANGLE_DIR}/bin/libGLESv2.dll"
        "${ZLIB_DIR}/bin/zlib1.dll"
        $<TARGET_FILE_DIR:shader_perf_test>
) 