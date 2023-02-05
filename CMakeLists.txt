cmake_minimum_required(VERSION 3.24)
set(PROJECT_NAME "FastCapture")
set(PROJECT_INJECT_DLL_NAME "FastCaptureInjectDll")
project(${PROJECT_NAME})

# 配置ccache
find_program(CCACHE_PATH ccache)

if(CCACHE_PATH)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

# 设置基类工程
add_library(PROJECT_BASE INTERFACE)

# 启用最大警告
target_compile_options(PROJECT_BASE INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
target_include_directories(PROJECT_BASE INTERFACE ./include)

# target_compile_definitions(PROJECT_BASE INTERFACE)
if(WIN32 OR MINGW)
    option(FAST_CAPTURE_USE_MT "MSVC运行时库类型" ON)

    if(FAST_CAPTURE_USE_MT)
        set_property(TARGET PROJECT_BASE PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

# 根据编译器设置特殊选项
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
# using Clang
# TODO: Clang配置为使用静态libc++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
    set_target_properties(PROJECT_BASE PROPERTIES LINK_SEARCH_START_STATIC ON)
    set_target_properties(PROJECT_BASE PROPERTIES LINK_SEARCH_END_STATIC ON)
    target_link_options(PROJECT_BASE INTERFACE -static-libgcc -static-libstdc++)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
# using Intel C++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
    target_compile_options(PROJECT_BASE INTERFACE /MP /utf-8)
endif()

# 配置外部库
include(FetchContent)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

macro(FORCE_SET option value)
    set(${option} ${value} CACHE INTERNAL "" FORCE)
endmacro()

# 配置OpenGL，只要头文件
find_package(OpenGL REQUIRED)
target_include_directories(PROJECT_BASE INTERFACE ${OPENGL_INCLUDE_DIR})

# 配置GLEW
find_package(GLEW 2.2.0)

if(GLEW_FOUND AND GLEW_USE_STATIC_LIBS)
    target_link_libraries(PROJECT_BASE INTERFACE GLEW::glew_s)
else()
    set(GLEW_USE_STATIC_LIBS TRUE)
    FetchContent_Declare(
        glew_2_2_0
        URL https://sourceforge.net/projects/glew/files/glew/2.2.0/glew-2.2.0.tgz/download
        SOURCE_SUBDIR build/cmake
    )
    FetchContent_MakeAvailable(glew_2_2_0)
    target_link_libraries(PROJECT_BASE INTERFACE glew_s)
    target_include_directories(PROJECT_BASE INTERFACE ${glew_2_2_0_SOURCE_DIR}/include)
endif()

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
    # Dobby暂不支持在Windows编译
    # 配置MinHook，不支持Windows Arm
    FetchContent_Declare(
        MinHook
        GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
        GIT_TAG 49d03ad118cf7f6768c79a8f187e14b8f2a07f94)
    FetchContent_MakeAvailable(MinHook)
    target_link_libraries(PROJECT_BASE INTERFACE minhook)
endif()

# # 配置Dobby

# FORCE_SET(DOBBY_GENERATE_SHARED OFF)

# if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
# set(CMAKE_CXX_FLAGS "-fpermissive ${CMAKE_CXX_FLAGS}")
# message("Notice: For Dobby, the C++ compile option \"-fpermissive\" is added.")
# set(ROLLBACK_COMPILE_OPTIONS ON)
# endif()

# FetchContent_Declare(
# Dobby
# GIT_REPOSITORY https://github.com/jmpews/Dobby.git
# GIT_TAG latest
# )

# FetchContent_MakeAvailable(Dobby)

# target_link_libraries(PROJECT_BASE INTERFACE dobby)

# if(ROLLBACK_COMPILE_OPTIONS)
# string(REPLACE "-fpermissive" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
# endif()

# 配置子项目
set(PROJECT_COMPONENTS_LIST)

macro(add_component NAME COMPONENT_DIR TYPE)
    aux_source_directory(${COMPONENT_DIR} COMPONENT_SOURCE_FILES)
    add_library(${NAME} ${TYPE} ${COMPONENT_SOURCE_FILES})
    target_link_libraries(${NAME} PRIVATE PROJECT_BASE)
    list(APPEND PROJECT_COMPONENTS_LIST ${NAME})
endmacro()

if(NOT TARGET_PLATFORM)
    # see https://cmake.org/cmake/help/latest/variable/CMAKE_HOST_SYSTEM_NAME.html#variable:CMAKE_HOST_SYSTEM_NAME
    set(TARGET_PLATFORM ${CMAKE_HOST_SYSTEM_NAME})
endif()

# 工程本体/客户端/用于被静态链接
add_component(${PROJECT_NAME} "./source/FastCapture/" STATIC)
aux_source_directory("./source/FastCapture/${TARGET_PLATFORM}" PROJECT_NAME_FILES)
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_NAME_FILES})

# 工程配套的DLL/服务端/用于注入到进程中
add_component(${PROJECT_INJECT_DLL_NAME} "./source/FastCaptureInjectDll/" SHARED)
aux_source_directory("./source/FastCaptureInjectDll/${TARGET_PLATFORM}" PROJECT_INJECT_DLL_NAME_FILES)
target_sources(${PROJECT_INJECT_DLL_NAME} PRIVATE ${PROJECT_INJECT_DLL_NAME_FILES})

set_property(
    TARGET ${PROJECT_COMPONENTS_LIST}
    PROPERTY CXX_STANDARD 20)
target_include_directories(${PROJECT_NAME} PUBLIC ./include)
